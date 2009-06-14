/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 * 		Karsten Heimrich <host.haiku@gmx.de>
 *
 */
#define DEBUG 1

#include <Alert.h>
#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <NodeInfo.h>
#include <PictureButton.h>
#include <Path.h>
#include <Roster.h>
#include <stdio.h>
#include <string.h>
#include <TextView.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <Window.h>
#include <Clipboard.h>


#include "FloaterManager.h"
#include "BitmapUtilities.h"
#include "PaintApplication.h"
#include "PaintWindow.h"
#include "MessageConstants.h"
//#include "Tools.h"
#include "ColorPalette.h"
#include "FileIdentificationStrings.h"
#include "ToolImages.h"
#include "ToolSelectionWindow.h"
#include "ToolSetupWindow.h"
//#include "DrawingTools.h"
#include "LayerWindow.h"
#include "RefFilters.h"
#include "FilePanels.h"
#include "ImageView.h"
#include "Settings.h"
#include "HSStack.h"
#include "BrushStoreWindow.h"
#include "ManipulatorServer.h"
#include "ProjectFileFunctions.h"
#include "StringServer.h"
#include "ToolManager.h"
#include "UndoQueue.h"
#include "UtilityClasses.h"
#include "Image.h"


// application constructor function
PaintApplication::PaintApplication()
	: BApplication("application/x-vnd.hsuhonen-artpaint")
	, fImageOpenPanel(NULL)
	, fProjectOpenPanel(NULL)
	, fGlobalSettings(NULL)
{
	// Some of the things in this function depend on the previously initialized
	// things, so the order may be important. This should be fixed in future.

	// create the settings
	fGlobalSettings = new global_settings();
	_ReadPreferences();

	// Set the language
	StringServer::SetLanguage(languages(fGlobalSettings->language));

	// Set the tool
	tool_manager->ChangeTool(fGlobalSettings->primary_tool);

	// Set the undo-queue to right depth
	UndoQueue::SetQueueDepth(fGlobalSettings->undo_queue_depth);

//	// the first untitled window will have number 1
//	untitled_window_number = 1;

	// create the tool-images here
	ToolImages::createToolImages();

	// Read the add-ons. They will be read in another thread by the manipulator
	// server. This should be the last thing to read so that it does not
	// interfere with other reading.
	ManipulatorServer::ReadAddOns();
}


PaintApplication::~PaintApplication()
{
	if (fImageOpenPanel) {
		delete fImageOpenPanel->RefFilter();
		delete fImageOpenPanel;
	}

	delete fProjectOpenPanel;

	_WritePreferences();
	delete fGlobalSettings;

	ToolManager::DestroyToolManager();
}


void
PaintApplication::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case HS_NEW_PAINT_WINDOW: {
			// issued from paint-window's menubar->"Window"->"New Paint Window"
			PaintWindow::createPaintWindow();
		}	break;

		case HS_SHOW_IMAGE_OPEN_PANEL: {
			// issued from paint-window's menubar->"File"->"Open"->"Open Image…"
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (fImageOpenPanel == NULL) {
				entry_ref ref;
				get_ref_for_path(fGlobalSettings->image_open_path, &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				fImageOpenPanel = new BFilePanel(B_OPEN_PANEL, &app, &ref,
					B_FILE_NODE, true, NULL, new ImageFilter());
			}

			fImageOpenPanel->SetMessage(&filePanelMessage);
			fImageOpenPanel->Window()->SetTitle(BString("ArtPaint: ")
				.Append(StringServer::ReturnString(OPEN_IMAGE_STRING)).String());
			fImageOpenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(fImageOpenPanel);
			fImageOpenPanel->Show();
		}	break;

		case HS_SHOW_PROJECT_OPEN_PANEL: {
			BMessage filePanelMessage(B_REFS_RECEIVED);
			if (fProjectOpenPanel == NULL) {
				entry_ref ref;
				get_ref_for_path(fGlobalSettings->project_open_path, &ref);
				filePanelMessage.AddBool("from_filepanel", true);

				BMessenger app(this);
				fProjectOpenPanel = new BFilePanel(B_OPEN_PANEL, &app, &ref,
					B_FILE_NODE);
			}

			fProjectOpenPanel->SetMessage(&filePanelMessage);
			fProjectOpenPanel->Window()->SetTitle(BString("ArtPaint: ")
				.Append(StringServer::ReturnString(OPEN_PROJECT_STRING)).String());
			fProjectOpenPanel->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);

			set_filepanel_strings(fProjectOpenPanel);
			fProjectOpenPanel->Show();
		}	break;

		case HS_SHOW_USER_DOCUMENTATION: {
			// issued from paint-window's menubar->"Help"->"User Documentation"
			BRoster roster;
			entry_ref mimeHandler;
			if (roster.FindApp("text/html", &mimeHandler) == B_OK) {
				BPath homePath;
				PaintApplication::HomeDirectory(homePath);

				// Force normalization of the path to check validity.
				if (homePath.Append("Documentation/", true) == B_OK) {
					const char* documentName;
					message->FindString("document", &documentName);
					if (homePath.Append(documentName, true) == B_OK) {
						BString url = "file://";
						url.Append(homePath.Path());
						const char* argv = url.String();
						roster.Launch(&mimeHandler, 1, &argv);
					}
				}
			}
		}	break;

		case B_PASTE: {
			if (be_clipboard->Lock()) {
				BMessage* data = be_clipboard->Data();
				if (data) {
					BMessage message;
					if (data->FindMessage("image/bitmap", &message) == B_OK) {
						BBitmap* pastedBitmap = new BBitmap(&message);
						if (pastedBitmap && pastedBitmap->IsValid()) {
							char name[] = "Clip 1";
							PaintWindow::createPaintWindow(pastedBitmap, name);
						}
					}
				}
				be_clipboard->Unlock();
			}
		}	break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


bool
PaintApplication::QuitRequested()
{
	// Here we must collect information about the window's that are still open
	// because they will be closed in BApplication::QuitRequested().
	bool layer_window_visible = fGlobalSettings->layer_window_visible;
	bool tool_setup_window_visible = fGlobalSettings->tool_setup_window_visible;
	bool tool_select_window_visible = fGlobalSettings->tool_select_window_visible;
	bool palette_window_visible = fGlobalSettings->palette_window_visible;
	bool brush_window_visible = fGlobalSettings->brush_window_visible;

	if (BApplication::QuitRequested()) {
		// We will quit.
		fGlobalSettings->layer_window_visible = layer_window_visible;
		fGlobalSettings->tool_setup_window_visible = tool_setup_window_visible;
		fGlobalSettings->tool_select_window_visible = tool_select_window_visible;
		fGlobalSettings->palette_window_visible = palette_window_visible;
		fGlobalSettings->brush_window_visible = brush_window_visible;
		return true;
	}
	return false;
}


void
PaintApplication::ReadyToRun()
{
	// Open here the ToolSelectionWindow
	if (fGlobalSettings->tool_select_window_visible)
		ToolSelectionWindow::showWindow();

	// Open here the ToolSetupWindow
	if (fGlobalSettings->tool_setup_window_visible)
		ToolSetupWindow::showWindow(fGlobalSettings->setup_window_tool);

	// Test here the brush store window
	if (fGlobalSettings->brush_window_visible) {
		BrushStoreWindow* brush_window = new BrushStoreWindow();
		brush_window->Show();
	}

	if (fGlobalSettings->palette_window_visible)
		ColorPaletteWindow::showPaletteWindow(false);

	if (fGlobalSettings->layer_window_visible)
		LayerWindow::showLayerWindow();

	// Here we will open a PaintWindow if no image was loaded on startup. This
	// should be the last window opened so that it will be the active window.
	if (PaintWindow::CountPaintWindows() == 0)
		PaintWindow::createPaintWindow();
}


void
PaintApplication::RefsReceived(BMessage* message)
{
	// here we will determine which type of file was opened
	// and then initiate the right function for opening it
	// the files will be checked for their mime-types and some
	// also additionally for their file-identification strings
	uint32 type;
	int32 count;
	entry_ref ref;
	BMessage* to_be_sent;
	BAlert* alert;

	message->GetInfo("refs", &type, &count);
	if ( type == B_REF_TYPE ) {
		for ( long i = --count; i >= 0; i-- ) {
			if ( message->FindRef("refs", i, &ref) == B_OK ) {
				BFile file;
				if ( file.SetTo(&ref, B_READ_ONLY) == B_OK ) {
					BNodeInfo node(&file);
					char mime_type[B_MIME_TYPE_LENGTH];
					// initialize the mime-type string
					strcpy(mime_type,"");
					node.GetType(mime_type);

					// here compare the mime-type to all possible
					// mime-types for the types we have created
					// type-strings are defined in FileIdentificationStrings.h
					if ((strcmp(mime_type,HS_PALETTE_MIME_STRING) == 0) ||
						(strcmp(mime_type,_OLD_HS_PALETTE_MIME_STRING) == 0)) {
						// Call the static showPaletteWindow-function.
						// Giving it an argument containing refs makes
						// it also load a palette.
						to_be_sent = new BMessage(HS_PALETTE_OPEN_REFS);
						to_be_sent->AddRef("refs",&ref);
						ColorPaletteWindow::showPaletteWindow(to_be_sent);
						delete to_be_sent;
					}
					else if ((strcmp(mime_type,HS_PROJECT_MIME_STRING) == 0) ||
							 (strcmp(mime_type,_OLD_HS_PROJECT_MIME_STRING) == 0)) {
						// We should read the project file and open a new window.
						// Read the first four bytes from the file to see if it is really
						// a project file.
						BEntry input_entry = BEntry(&ref);

						BPath input_path;
						input_entry.GetPath(&input_path);

						int32 file_id;
						int32 lendian;
						if (file.Read(&lendian,sizeof(int32)) == sizeof(int32)) {
							if (file.Read(&file_id,sizeof(int32)) == sizeof(int32)) {
								fGlobalSettings->insert_recent_project_path(input_path.Path());
								if (uint32(lendian) == 0xFFFFFFFF)
									file_id = B_LENDIAN_TO_HOST_INT32(file_id);
								else
									file_id = B_BENDIAN_TO_HOST_INT32(file_id);

								if (file_id == PROJECT_FILE_ID) {
									file.Seek(0,SEEK_SET);	// Rewind the file.
									bool store_path;
									if (message->FindBool("from_filepanel",&store_path) == B_OK) {
										if (store_path == true) {
											BEntry entry(&ref);
											entry.GetParent(&entry);
											BPath path;
											if ((entry.GetPath(&path) == B_OK) && (path.Path() != NULL)) {
												strcpy(fGlobalSettings->project_open_path,path.Path());
											}
										}
									}
									_ReadProject(file,ref);
								}
								else {
									bool store_path;
									if (message->FindBool("from_filepanel",&store_path) == B_OK) {
										if (store_path == true) {
											BEntry entry(&ref);
											entry.GetParent(&entry);
											BPath path;
											if ((entry.GetPath(&path) == B_OK) && (path.Path() != NULL)) {
												strcpy(fGlobalSettings->project_open_path,path.Path());
											}
										}
									}
									file.Seek(0,SEEK_SET);
									_ReadProjectOldStyle(file,ref);
								}
							}
						}
					}
					// The file was not one of ArtPaint's file types. Perhaps it is an image-file.
					// Try to read it using the Translation-kit.
					else if ((strncmp(mime_type,"image/",6) == 0) || (strcmp(mime_type,"") == 0)) {
						BEntry input_entry = BEntry(&ref);

						BPath input_path;
						input_entry.GetPath(&input_path);
						// The returned bitmap might be in 8-bit format. If that is the case, we should
						// convert to 32-bit.
						BBitmap* input_bitmap = BTranslationUtils::GetBitmapFile(input_path.Path());
						if (input_bitmap == NULL) {
							// even the translators did not work
							char alert_string[255];
							sprintf(alert_string,StringServer::ReturnString(UNSUPPORTED_FILE_TYPE_STRING),ref.name);
							alert = new BAlert("title",alert_string,StringServer::ReturnString(OK_STRING),NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
							alert->Go();
						}
						else {
							fGlobalSettings->insert_recent_image_path(input_path.Path());

							input_bitmap = BitmapUtilities::ConvertColorSpace(input_bitmap,B_RGBA32);
							BitmapUtilities::FixMissingAlpha(input_bitmap);

							BTranslatorRoster* roster = BTranslatorRoster::Default();

							translator_info in_info;
							translator_info test_info;

							roster->Identify(&file,NULL,&in_info);
							printf("%s\n",in_info.MIME);
							// Check here if the reverse translation can also be done.
							BBitmapStream image_buffer(input_bitmap);
							status_t output_error;

							PaintWindow* the_window;

							if ((output_error = roster->Identify(&image_buffer,NULL,&test_info,0,NULL,in_info.type)) == B_OK) {
								// Reverse translation is possible
								image_buffer.DetachBitmap(&input_bitmap);
								the_window = PaintWindow::createPaintWindow(input_bitmap,ref.name,in_info.type,ref,test_info.translator);
							}
							else {
								// Reverse translation is not possible
								image_buffer.DetachBitmap(&input_bitmap);
								the_window = PaintWindow::createPaintWindow(input_bitmap,ref.name,0,ref);
							}

							the_window->readAttributes(file);

							// Record the new image_open_path if necessary.
							bool store_path;
							if (message->FindBool("from_filepanel",&store_path) == B_OK) {
								BEntry entry(&ref);
								entry.GetParent(&entry);
								BPath path;
								if ((entry.GetPath(&path) == B_OK) && (path.Path() != NULL)) {
									strcpy(fGlobalSettings->image_open_path,path.Path());
								}
							}
							else {
							}
						}
					}
					else {
						char alert_string[255];
						sprintf(alert_string,StringServer::ReturnString(UNSUPPORTED_FILE_TYPE_STRING),ref.name);
						alert = new BAlert("title",alert_string,StringServer::ReturnString(OK_STRING),NULL,NULL,B_WIDTH_AS_USUAL,B_WARNING_ALERT);
						alert->Go();
					}
				}
			}
		}
	}
}


// These functions get and set color for particular button. The ability to have
// different colors for each button is removed and thus only foreground and
// background-color can be defined.
rgb_color
PaintApplication::Color(bool foreground) const
{
	// here we return the tool that corresponds to button
	if (foreground)
		return fGlobalSettings->primary_color;
	return fGlobalSettings->secondary_color;
}


void
PaintApplication::SetColor(rgb_color color, bool foreground)
{
	if (foreground)
		fGlobalSettings->primary_color = color;
	else
		fGlobalSettings->secondary_color = color;
}


void
PaintApplication::_ReadPreferences()
{
	bool createDefaultTools = true;
	bool createDefaultColorset = true;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append("ArtPaint");
		BDirectory settingsDir(path.Path());

		HomeDirectory(path);
		path.Append("settings");
		BDirectory spareDir(path.Path());

		bool spareDirExists = spareDir.InitCheck() == B_OK;
		if (settingsDir.InitCheck() == B_OK || spareDirExists) {
			BEntry entry;
			status_t status = settingsDir.FindEntry("brushes", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("brushes", &entry, true);

			if (status == B_OK) {
				BFile brushes(&entry, B_READ_ONLY);
				status = brushes.InitCheck();
				if (status == B_OK)
					BrushStoreWindow::readBrushes(brushes);
			}

			if (status != B_OK)
				;// We might create some default brushes.

			status = settingsDir.FindEntry("main_preferences",&entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("main_preferences",&entry, true);

			if (status == B_OK) {
				BFile mainPreferences(&entry, B_READ_ONLY);
				status = mainPreferences.InitCheck();
				if (status == B_OK)
					status = fGlobalSettings->read_from_file(mainPreferences);
			}

			if (status != B_OK)
				;// Settings have the default values.

			// Here set the language for the StringServer
			StringServer::SetLanguage(languages(fGlobalSettings->language));

			// Create a tool-manager object. Depends on the language being set.
			ToolManager::CreateToolManager();

			status = settingsDir.FindEntry("tool_preferences", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("tool_preferences", &entry, true);

			if (status == B_OK) {
				BFile tools(&entry, B_READ_ONLY);
				if (tools.InitCheck() == B_OK) {
					if (tool_manager->ReadToolSettings(tools) == B_OK)
						createDefaultTools = false;
				}
			}

			status = settingsDir.FindEntry("color_preferences", &entry, true);
			if ((status != B_OK) && spareDirExists)
				status = spareDir.FindEntry("color_preferences", &entry, true);

			if (status == B_OK) {
				BFile colors(&entry, B_READ_ONLY);
				if (colors.InitCheck() == B_OK) {
					if (ColorSet::readSets(colors) == B_OK)
						createDefaultColorset = false;
				}
			}
		}
	}

	if (createDefaultColorset) {
		// We might look into apps directory and palette directory for some
		// colorsets (TODO: this looks starnge, maybe implement static init)
		new ColorSet(16);
	}

	// Create a tool-manager object.
	if (createDefaultTools)
		ToolManager::CreateToolManager();
}


void
PaintApplication::_WritePreferences()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		BDirectory settingsDir(path.Path());
		status_t status = settingsDir.CreateDirectory("./ArtPaint", &settingsDir);
		if (status == B_FILE_EXISTS)
			status = settingsDir.SetTo(&settingsDir, "ArtPaint/");

		if (status == B_OK) {
			// Here we create several preferences files. One for each internal
			// manipulator, main preferences file and brushes and palettes
			// files. Later we might add even more files for example a file for
			// each tool. Now all the tool-settings are stored in one file.
			// Actually the manipulator preference files will be created by
			// ManipulatorServer.
			BFile brushes;
			if (settingsDir.CreateFile("brushes", &brushes, false) == B_OK)
				BrushStoreWindow::writeBrushes(brushes);

			BFile mainPreferences;
			if (settingsDir.CreateFile("main_preferences",
				&mainPreferences, false) == B_OK) {
					fGlobalSettings->write_to_file(mainPreferences);
			}

			BFile tools;
			if (settingsDir.CreateFile("tool_preferences", &tools, false) == B_OK)
				tool_manager->WriteToolSettings(tools);

			BFile colors;
			if (settingsDir.CreateFile("color_preferences", &colors, false) == B_OK)
				ColorSet::writeSets(colors);
		} else {
			fprintf(stderr, "Could not write preferences.\n");
		}
	}
}


status_t
PaintApplication::_ReadProject(BFile& file, entry_ref& ref)
{
	// This is the new way of reading a structured project file. The possibility
	// to read old project files is still maintained through readProjectOldStyle.

	int32 lendian;
	file.Read(&lendian, sizeof(int32));

	bool isLittleEndian = true;
	if (lendian == 0x00000000) {
		isLittleEndian = false;
	} else if (uint32(lendian) == 0xFFFFFFFF) {
		isLittleEndian = true;
	} else {
		return B_ERROR;
	}

	int64 length = FindProjectFileSection(file, PROJECT_FILE_DIMENSION_SECTION_ID);
	if (length != (2 * sizeof(int32)))
		return B_ERROR;

	int32 width;
	if (file.Read(&width, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	int32 height;
	if (file.Read(&height, sizeof(int32)) != sizeof(int32))
		return B_ERROR;

	if (isLittleEndian) {
		width = B_LENDIAN_TO_HOST_INT32(width);
		height = B_LENDIAN_TO_HOST_INT32(height);
	} else {
		width = B_BENDIAN_TO_HOST_INT32(width);
		height = B_BENDIAN_TO_HOST_INT32(height);
	}

	// Create a paint-window using the width and height
	PaintWindow* paintWindow = PaintWindow::createPaintWindow(NULL, ref.name);

	paintWindow->OpenImageView(width,height);
	// Then read the layer-data. Rewind the file and put the image-view to read
	// the data.
	ImageView* image_view = paintWindow->ReturnImageView();
	image_view->ReturnImage()->ReadLayers(file);

	// This must be before the image-view is added
	paintWindow->SetProjectEntry(BEntry(&ref, true));
	paintWindow->AddImageView();

	// As last thing read the attributes from the file
	paintWindow->readAttributes(file);

	return B_OK;
}


status_t
PaintApplication::_ReadProjectOldStyle(BFile& file, entry_ref& ref)
{
// This old version of file reading will be copied to the conversion utility.
	// The structure of a project file is following.
	// 	1.	Identification string HS_PROJECT_ID_STRING
	//	2.	ArtPaint version number ARTPAINT_VERSION
	//	3.	The length of stored window_settings struct
	//	4.	The settings for the project (a window_settings struct stored that is)
	//	5.	The width and height of image (in pixels) stored as uint32s
	//	6.	How many layers are stored.
	//	7.	Data for layers. Will be read by layer's readLayer.
	// We can assume that the project_file is a valid file.
	char alert_text[256];
	char file_name[256];
	strncpy(file_name,ref.name,256);

	BAlert* alert;
	char file_id[256]; 		// The identification string.
	ssize_t bytes_read = file.Read(file_id, strlen(HS_PROJECT_ID_STRING));
	if (bytes_read < 0 || uint32(bytes_read) != strlen(HS_PROJECT_ID_STRING)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;
	}
	file_id[bytes_read] = '\0';
	if (strcmp(file_id, HS_PROJECT_ID_STRING) != 0) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;

	}
	uint32 version_number; 	// The version number of program with which this was written.
							// We actually do not care about the version number, because it is
							// known to be 1.0.0
	if (file.Read(&version_number,sizeof(uint32)) != sizeof(uint32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.

		return B_ERROR;
	}
	version_number = B_BENDIAN_TO_HOST_INT32(version_number);


	int32 settings_length;	// If the settings are of different length than the settings_struct, we
							// cannot use them.
	if (file.Read(&settings_length,sizeof(int32)) != sizeof(int32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.

		return B_ERROR;
	}
	settings_length = B_BENDIAN_TO_HOST_INT32(settings_length);
	// We skip the settings, no need to bother converting them
	if (true) {
		file.Seek(settings_length,SEEK_CUR);
	}

	uint32 width,height;
	if (file.Read(&width,sizeof(uint32)) != sizeof(uint32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;
	}
	if (file.Read(&height,sizeof(uint32)) != sizeof(uint32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		return B_ERROR;
	}

	width = B_BENDIAN_TO_HOST_INT32(width);
	height = B_BENDIAN_TO_HOST_INT32(height);
	// We should create a PaintWindow and also an ImageView for it.
	PaintWindow* the_window = PaintWindow::createPaintWindow(NULL,file_name);
	the_window->OpenImageView(width,height);


//	*project_entry = BEntry(&ref);

	// Here we are ready to read layers from the file.
	// First read how many layers there are.
	int32 layer_count;

	if (file.Read(&layer_count,sizeof(int32)) != sizeof(int32)) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		// Free the allocated memory.
		delete the_window->ReturnImageView();
		the_window->Lock();
		the_window->Quit();
		return B_ERROR;
	}
	layer_count = B_BENDIAN_TO_HOST_INT32(layer_count);
	if (the_window->ReturnImageView()->ReturnImage()->ReadLayersOldStyle(file,layer_count) != B_OK) {
		sprintf(alert_text,"Project file %s structure corrupted.",file_name);
		alert = new BAlert("",alert_text,"OK");
		alert->Go();	// Alert deletes itself before returning.
		delete the_window->ReturnImageView();
		the_window->Lock();
		the_window->Quit();
		//printf("layers corrupt\n");

		return B_ERROR;
	}

	// This must be before the image-view is added
	the_window->SetProjectEntry(BEntry(&ref, true));

	the_window->AddImageView();
	// Change the zoom-level to be correct
//	the_window->Lock();
//	the_window->displayMag(the_window->Settings()->zoom_level);
//	the_window->Unlock();

	// As last thing read the attributes from the file
	the_window->readAttributes(file);

	return B_OK;
}


void
PaintApplication::HomeDirectory(BPath &path)
{
	// this from the newsletter 81
	app_info info;
	be_app->GetAppInfo(&info);
	BEntry appentry;
	appentry.SetTo(&info.ref);
	appentry.GetPath(&path);
	path.GetParent(&path);
}


int
main(int argc, char* argv[])
{
	PaintApplication* paintApp = new PaintApplication();
	if (paintApp) {
		paintApp->Run();
		delete paintApp;
	}

	return B_OK;
}


filter_result
AppKeyFilterFunction(BMessage* message,BHandler** handler, BMessageFilter*)
{
	const char* bytes;
	if ((!(modifiers() & B_COMMAND_KEY)) && (!(modifiers() & B_CONTROL_KEY))) {
		if (message->FindString("bytes", &bytes) == B_OK) {
			if (bytes[0] == B_TAB) {
				BView* view = dynamic_cast<BView*>(*handler);
				if (view && !(view->Flags() & B_NAVIGABLE)) {
					if (dynamic_cast<BTextView*>(*handler) == NULL)
						FloaterManager::ToggleFloaterVisibility();
				} else {
					FloaterManager::ToggleFloaterVisibility();
				}
			}
		}
	}
	return B_DISPATCH_MESSAGE;
}
