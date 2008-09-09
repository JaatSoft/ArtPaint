/*
 * Copyright 2003, Heikki Suhonen
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *
 */
#ifndef	FLAGS_H
#define	FLAGS_H


const int32 flagWidth = 15;
const int32 flagHeight = 10;
const color_space flagColorSpace = B_COLOR_8_BIT;

const unsigned char flagGermanBits [] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,
	0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xeb,0xff,
	0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0xff,
	0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0xff,
	0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0x9d,0xff,
	0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xff,
	0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xff,
	0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xf9,0xff
};


const unsigned char flagBritishBits [] = {
	0x7b,0x1b,0xce,0x23,0x23,0x23,0x82,0x2a,0xa2,0x23,0x23,0x23,0xd4,0xa2,0xd9,0xff,
	0xf4,0xa8,0x5a,0x86,0xd4,0x23,0xa2,0x2a,0x82,0x23,0x23,0xa8,0x5a,0x82,0xb2,0xff,
	0x23,0x23,0xce,0x7b,0x1b,0x8e,0x16,0x2a,0x82,0xb4,0x82,0x82,0xb2,0x23,0x23,0xff,
	0x86,0x82,0x86,0x16,0xdb,0x3f,0x5a,0x2a,0x5a,0xd9,0x3f,0x1b,0x12,0x16,0x86,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0xff,
	0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0xff,
	0x16,0x80,0x14,0x1b,0x3f,0x1b,0x5a,0x2a,0x5a,0x3f,0xdb,0x80,0x68,0x80,0x82,0xff,
	0x23,0x23,0xcc,0x82,0x5a,0xd4,0x83,0x2a,0x82,0x92,0x82,0x82,0xd4,0x23,0x23,0xff,
	0xb2,0x82,0xd9,0xa8,0x23,0x23,0xa2,0x2a,0xc2,0x23,0xd2,0x12,0x5a,0xa8,0xf4,0xff,
	0xd9,0xc2,0xd4,0x23,0x23,0x23,0x82,0x2a,0x82,0x23,0x23,0x23,0xae,0x1b,0xc3,0xff
};


const unsigned char flagFrenchBits [] = {
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
	0x21,0x21,0x21,0x21,0x21,0x1f,0x1f,0x1f,0x1f,0x1f,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a
};


#endif
