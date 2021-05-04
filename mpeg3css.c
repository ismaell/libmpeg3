/*
 * Copyright (C) 2000 Derek Fawcus et. al.  <derek@spider.com>
 * Ported to libmpeg3 by et. al.
 *
 * This code may be used under the terms of Version 2 of the GPL,
 * read the file COPYING for details.
 *
 */

/*
 * These routines do some reordering of the supplied data before
 * calling engine() to do the main work.
 *
 * The reordering seems similar to that done by the initial stages of
 * the DES algorithm, in that it looks like it's just been done to
 * try and make software decoding slower.  I'm not sure that it
 * actually adds anything to the security.
 *
 * The nature of the shuffling is that the bits of the supplied
 * parameter 'varient' are reorganised (and some inverted),  and
 * the bytes of the parameter 'challenge' are reorganised.
 *
 * The reorganisation in each routine is different,  and the first
 * (CryptKey1) does not bother of play with the 'varient' parameter.
 *
 * Since this code is only run once per disk change,  I've made the
 * code table driven in order to improve readability.
 *
 * Since these routines are so similar to each other,  one could even
 * abstract them all to one routine supplied a parameter determining
 * the nature of the reordering it has to do.
 */


#ifdef HAVE_CSS

#include "mpeg3css.h"
#include "mpeg3private.h"
#include "mpeg3protos.h"


#include <unistd.h>
#include <fcntl.h>
// Must unlink /usr/include directories from the kernel source for this to work.
#include <linux/cdrom.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>


#ifndef FIBMAP
#define FIBMAP	   _IO(0x00,1)	/* bmap access */
//#define FIBMAP 1
#endif

/* =================================== TABLES ================================ */

static unsigned char mpeg3css_varients[] = 
{
	0xB7, 0x74, 0x85, 0xD0, 0xCC, 0xDB, 0xCA, 0x73,
	0x03, 0xFE, 0x31, 0x03, 0x52, 0xE0, 0xB7, 0x42,
	0x63, 0x16, 0xF2, 0x2A, 0x79, 0x52, 0xFF, 0x1B,
	0x7A, 0x11, 0xCA, 0x1A, 0x9B, 0x40, 0xAD, 0x01
};

static unsigned char mpeg3css_secret[] = {0x55, 0xD6, 0xC4, 0xC5, 0x28};

static unsigned char mpeg3css_table0[] = 
{
	0xB7, 0xF4, 0x82, 0x57, 0xDA, 0x4D, 0xDB, 0xE2,
	0x2F, 0x52, 0x1A, 0xA8, 0x68, 0x5A, 0x8A, 0xFF,
	0xFB, 0x0E, 0x6D, 0x35, 0xF7, 0x5C, 0x76, 0x12,
	0xCE, 0x25, 0x79, 0x29, 0x39, 0x62, 0x08, 0x24,
	0xA5, 0x85, 0x7B, 0x56, 0x01, 0x23, 0x68, 0xCF,
	0x0A, 0xE2, 0x5A, 0xED, 0x3D, 0x59, 0xB0, 0xA9,
	0xB0, 0x2C, 0xF2, 0xB8, 0xEF, 0x32, 0xA9, 0x40,
	0x80, 0x71, 0xAF, 0x1E, 0xDE, 0x8F, 0x58, 0x88,
	0xB8, 0x3A, 0xD0, 0xFC, 0xC4, 0x1E, 0xB5, 0xA0,
	0xBB, 0x3B, 0x0F, 0x01, 0x7E, 0x1F, 0x9F, 0xD9,
	0xAA, 0xB8, 0x3D, 0x9D, 0x74, 0x1E, 0x25, 0xDB,
	0x37, 0x56, 0x8F, 0x16, 0xBA, 0x49, 0x2B, 0xAC,
	0xD0, 0xBD, 0x95, 0x20, 0xBE, 0x7A, 0x28, 0xD0,
	0x51, 0x64, 0x63, 0x1C, 0x7F, 0x66, 0x10, 0xBB,
	0xC4, 0x56, 0x1A, 0x04, 0x6E, 0x0A, 0xEC, 0x9C,
	0xD6, 0xE8, 0x9A, 0x7A, 0xCF, 0x8C, 0xDB, 0xB1,
	0xEF, 0x71, 0xDE, 0x31, 0xFF, 0x54, 0x3E, 0x5E,
	0x07, 0x69, 0x96, 0xB0, 0xCF, 0xDD, 0x9E, 0x47,
	0xC7, 0x96, 0x8F, 0xE4, 0x2B, 0x59, 0xC6, 0xEE,
	0xB9, 0x86, 0x9A, 0x64, 0x84, 0x72, 0xE2, 0x5B,
	0xA2, 0x96, 0x58, 0x99, 0x50, 0x03, 0xF5, 0x38,
	0x4D, 0x02, 0x7D, 0xE7, 0x7D, 0x75, 0xA7, 0xB8,
	0x67, 0x87, 0x84, 0x3F, 0x1D, 0x11, 0xE5, 0xFC,
	0x1E, 0xD3, 0x83, 0x16, 0xA5, 0x29, 0xF6, 0xC7,
	0x15, 0x61, 0x29, 0x1A, 0x43, 0x4F, 0x9B, 0xAF,
	0xC5, 0x87, 0x34, 0x6C, 0x0F, 0x3B, 0xA8, 0x1D,
	0x45, 0x58, 0x25, 0xDC, 0xA8, 0xA3, 0x3B, 0xD1,
	0x79, 0x1B, 0x48, 0xF2, 0xE9, 0x93, 0x1F, 0xFC,
	0xDB, 0x2A, 0x90, 0xA9, 0x8A, 0x3D, 0x39, 0x18,
	0xA3, 0x8E, 0x58, 0x6C, 0xE0, 0x12, 0xBB, 0x25,
	0xCD, 0x71, 0x22, 0xA2, 0x64, 0xC6, 0xE7, 0xFB,
	0xAD, 0x94, 0x77, 0x04, 0x9A, 0x39, 0xCF, 0x7C
};

static unsigned char mpeg3css_table1[] = 
{
	0x8C, 0x47, 0xB0, 0xE1, 0xEB, 0xFC, 0xEB, 0x56,
	0x10, 0xE5, 0x2C, 0x1A, 0x5D, 0xEF, 0xBE, 0x4F,
	0x08, 0x75, 0x97, 0x4B, 0x0E, 0x25, 0x8E, 0x6E,
	0x39, 0x5A, 0x87, 0x53, 0xC4, 0x1F, 0xF4, 0x5C,
	0x4E, 0xE6, 0x99, 0x30, 0xE0, 0x42, 0x88, 0xAB,
	0xE5, 0x85, 0xBC, 0x8F, 0xD8, 0x3C, 0x54, 0xC9,
	0x53, 0x47, 0x18, 0xD6, 0x06, 0x5B, 0x41, 0x2C,
	0x67, 0x1E, 0x41, 0x74, 0x33, 0xE2, 0xB4, 0xE0,
	0x23, 0x29, 0x42, 0xEA, 0x55, 0x0F, 0x25, 0xB4,
	0x24, 0x2C, 0x99, 0x13, 0xEB, 0x0A, 0x0B, 0xC9,
	0xF9, 0x63, 0x67, 0x43, 0x2D, 0xC7, 0x7D, 0x07,
	0x60, 0x89, 0xD1, 0xCC, 0xE7, 0x94, 0x77, 0x74,
	0x9B, 0x7E, 0xD7, 0xE6, 0xFF, 0xBB, 0x68, 0x14,
	0x1E, 0xA3, 0x25, 0xDE, 0x3A, 0xA3, 0x54, 0x7B,
	0x87, 0x9D, 0x50, 0xCA, 0x27, 0xC3, 0xA4, 0x50,
	0x91, 0x27, 0xD4, 0xB0, 0x82, 0x41, 0x97, 0x79,
	0x94, 0x82, 0xAC, 0xC7, 0x8E, 0xA5, 0x4E, 0xAA,
	0x78, 0x9E, 0xE0, 0x42, 0xBA, 0x28, 0xEA, 0xB7,
	0x74, 0xAD, 0x35, 0xDA, 0x92, 0x60, 0x7E, 0xD2,
	0x0E, 0xB9, 0x24, 0x5E, 0x39, 0x4F, 0x5E, 0x63,
	0x09, 0xB5, 0xFA, 0xBF, 0xF1, 0x22, 0x55, 0x1C,
	0xE2, 0x25, 0xDB, 0xC5, 0xD8, 0x50, 0x03, 0x98,
	0xC4, 0xAC, 0x2E, 0x11, 0xB4, 0x38, 0x4D, 0xD0,
	0xB9, 0xFC, 0x2D, 0x3C, 0x08, 0x04, 0x5A, 0xEF,
	0xCE, 0x32, 0xFB, 0x4C, 0x92, 0x1E, 0x4B, 0xFB,
	0x1A, 0xD0, 0xE2, 0x3E, 0xDA, 0x6E, 0x7C, 0x4D,
	0x56, 0xC3, 0x3F, 0x42, 0xB1, 0x3A, 0x23, 0x4D,
	0x6E, 0x84, 0x56, 0x68, 0xF4, 0x0E, 0x03, 0x64,
	0xD0, 0xA9, 0x92, 0x2F, 0x8B, 0xBC, 0x39, 0x9C,
	0xAC, 0x09, 0x5E, 0xEE, 0xE5, 0x97, 0xBF, 0xA5,
	0xCE, 0xFA, 0x28, 0x2C, 0x6D, 0x4F, 0xEF, 0x77,
	0xAA, 0x1B, 0x79, 0x8E, 0x97, 0xB4, 0xC3, 0xF4
};

static unsigned char mpeg3css_table2[] = 
{
	0xB7, 0x75, 0x81, 0xD5, 0xDC, 0xCA, 0xDE, 0x66,
	0x23, 0xDF, 0x15, 0x26, 0x62, 0xD1, 0x83, 0x77,
	0xE3, 0x97, 0x76, 0xAF, 0xE9, 0xC3, 0x6B, 0x8E,
	0xDA, 0xB0, 0x6E, 0xBF, 0x2B, 0xF1, 0x19, 0xB4,
	0x95, 0x34, 0x48, 0xE4, 0x37, 0x94, 0x5D, 0x7B,
	0x36, 0x5F, 0x65, 0x53, 0x07, 0xE2, 0x89, 0x11,
	0x98, 0x85, 0xD9, 0x12, 0xC1, 0x9D, 0x84, 0xEC,
	0xA4, 0xD4, 0x88, 0xB8, 0xFC, 0x2C, 0x79, 0x28,
	0xD8, 0xDB, 0xB3, 0x1E, 0xA2, 0xF9, 0xD0, 0x44,
	0xD7, 0xD6, 0x60, 0xEF, 0x14, 0xF4, 0xF6, 0x31,
	0xD2, 0x41, 0x46, 0x67, 0x0A, 0xE1, 0x58, 0x27,
	0x43, 0xA3, 0xF8, 0xE0, 0xC8, 0xBA, 0x5A, 0x5C,
	0x80, 0x6C, 0xC6, 0xF2, 0xE8, 0xAD, 0x7D, 0x04,
	0x0D, 0xB9, 0x3C, 0xC2, 0x25, 0xBD, 0x49, 0x63,
	0x8C, 0x9F, 0x51, 0xCE, 0x20, 0xC5, 0xA1, 0x50,
	0x92, 0x2D, 0xDD, 0xBC, 0x8D, 0x4F, 0x9A, 0x71,
	0x2F, 0x30, 0x1D, 0x73, 0x39, 0x13, 0xFB, 0x1A,
	0xCB, 0x24, 0x59, 0xFE, 0x05, 0x96, 0x57, 0x0F,
	0x1F, 0xCF, 0x54, 0xBE, 0xF5, 0x06, 0x1B, 0xB2,
	0x6D, 0xD3, 0x4D, 0x32, 0x56, 0x21, 0x33, 0x0B,
	0x52, 0xE7, 0xAB, 0xEB, 0xA6, 0x74, 0x00, 0x4C,
	0xB1, 0x7F, 0x82, 0x99, 0x87, 0x0E, 0x5E, 0xC0,
	0x8F, 0xEE, 0x6F, 0x55, 0xF3, 0x7E, 0x08, 0x90,
	0xFA, 0xB6, 0x64, 0x70, 0x47, 0x4A, 0x17, 0xA7,
	0xB5, 0x40, 0x8A, 0x38, 0xE5, 0x68, 0x3E, 0x8B,
	0x69, 0xAA, 0x9B, 0x42, 0xA5, 0x10, 0x01, 0x35,
	0xFD, 0x61, 0x9E, 0xE6, 0x16, 0x9C, 0x86, 0xED,
	0xCD, 0x2E, 0xFF, 0xC4, 0x5B, 0xA0, 0xAE, 0xCC,
	0x4B, 0x3B, 0x03, 0xBB, 0x1C, 0x2A, 0xAC, 0x0C,
	0x3F, 0x93, 0xC7, 0x72, 0x7A, 0x09, 0x22, 0x3D,
	0x45, 0x78, 0xA9, 0xA8, 0xEA, 0xC9, 0x6A, 0xF7,
	0x29, 0x91, 0xF0, 0x02, 0x18, 0x3A, 0x4E, 0x7C
};

static unsigned char mpeg3css_table3[] = 
{
	0x73, 0x51, 0x95, 0xE1, 0x12, 0xE4, 0xC0, 0x58,
	0xEE, 0xF2, 0x08, 0x1B, 0xA9, 0xFA, 0x98, 0x4C,
	0xA7, 0x33, 0xE2, 0x1B, 0xA7, 0x6D, 0xF5, 0x30,
	0x97, 0x1D, 0xF3, 0x02, 0x60, 0x5A, 0x82, 0x0F,
	0x91, 0xD0, 0x9C, 0x10, 0x39, 0x7A, 0x83, 0x85,
	0x3B, 0xB2, 0xB8, 0xAE, 0x0C, 0x09, 0x52, 0xEA,
	0x1C, 0xE1, 0x8D, 0x66, 0x4F, 0xF3, 0xDA, 0x92,
	0x29, 0xB9, 0xD5, 0xC5, 0x77, 0x47, 0x22, 0x53,
	0x14, 0xF7, 0xAF, 0x22, 0x64, 0xDF, 0xC6, 0x72,
	0x12, 0xF3, 0x75, 0xDA, 0xD7, 0xD7, 0xE5, 0x02,
	0x9E, 0xED, 0xDA, 0xDB, 0x4C, 0x47, 0xCE, 0x91,
	0x06, 0x06, 0x6D, 0x55, 0x8B, 0x19, 0xC9, 0xEF,
	0x8C, 0x80, 0x1A, 0x0E, 0xEE, 0x4B, 0xAB, 0xF2,
	0x08, 0x5C, 0xE9, 0x37, 0x26, 0x5E, 0x9A, 0x90,
	0x00, 0xF3, 0x0D, 0xB2, 0xA6, 0xA3, 0xF7, 0x26,
	0x17, 0x48, 0x88, 0xC9, 0x0E, 0x2C, 0xC9, 0x02,
	0xE7, 0x18, 0x05, 0x4B, 0xF3, 0x39, 0xE1, 0x20,
	0x02, 0x0D, 0x40, 0xC7, 0xCA, 0xB9, 0x48, 0x30,
	0x57, 0x67, 0xCC, 0x06, 0xBF, 0xAC, 0x81, 0x08,
	0x24, 0x7A, 0xD4, 0x8B, 0x19, 0x8E, 0xAC, 0xB4,
	0x5A, 0x0F, 0x73, 0x13, 0xAC, 0x9E, 0xDA, 0xB6,
	0xB8, 0x96, 0x5B, 0x60, 0x88, 0xE1, 0x81, 0x3F,
	0x07, 0x86, 0x37, 0x2D, 0x79, 0x14, 0x52, 0xEA,
	0x73, 0xDF, 0x3D, 0x09, 0xC8, 0x25, 0x48, 0xD8,
	0x75, 0x60, 0x9A, 0x08, 0x27, 0x4A, 0x2C, 0xB9,
	0xA8, 0x8B, 0x8A, 0x73, 0x62, 0x37, 0x16, 0x02,
	0xBD, 0xC1, 0x0E, 0x56, 0x54, 0x3E, 0x14, 0x5F,
	0x8C, 0x8F, 0x6E, 0x75, 0x1C, 0x07, 0x39, 0x7B,
	0x4B, 0xDB, 0xD3, 0x4B, 0x1E, 0xC8, 0x7E, 0xFE,
	0x3E, 0x72, 0x16, 0x83, 0x7D, 0xEE, 0xF5, 0xCA,
	0xC5, 0x18, 0xF9, 0xD8, 0x68, 0xAB, 0x38, 0x85,
	0xA8, 0xF0, 0xA1, 0x73, 0x9F, 0x5D, 0x19, 0x0B,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x33, 0x72, 0x39, 0x25, 0x67, 0x26, 0x6D, 0x71,
	0x36, 0x77, 0x3C, 0x20, 0x62, 0x23, 0x68, 0x74,
	0xC3, 0x82, 0xC9, 0x15, 0x57, 0x16, 0x5D, 0x81
};

static struct mpeg3_playkey mpeg3_pkey1a1 = {0x36b, {0x51,0x67,0x67,0xc5,0xe0}};
static struct mpeg3_playkey mpeg3_pkey2a1 = {0x762, {0x2c,0xb2,0xc1,0x09,0xee}};
static struct mpeg3_playkey mpeg3_pkey1b1 = {0x36b, {0x90,0xc1,0xd7,0x84,0x48}};

static struct mpeg3_playkey mpeg3_pkey1a2 = {0x2f3, {0x51,0x67,0x67,0xc5,0xe0}};
static struct mpeg3_playkey mpeg3_pkey2a2 = {0x730, {0x2c,0xb2,0xc1,0x09,0xee}};
static struct mpeg3_playkey mpeg3_pkey1b2 = {0x2f3, {0x90,0xc1,0xd7,0x84,0x48}};

static struct mpeg3_playkey mpeg3_pkey1a3 = {0x235, {0x51,0x67,0x67,0xc5,0xe0}};
static struct mpeg3_playkey mpeg3_pkey1b3 = {0x235, {0x90,0xc1,0xd7,0x84,0x48}};

static struct mpeg3_playkey mpeg3_pkey3a1 = {0x249, {0xb7,0x3f,0xd4,0xaa,0x14}}; /* DVD specific ? */
static struct mpeg3_playkey mpeg3_pkey4a1 = {0x028, {0x53,0xd4,0xf7,0xd9,0x8f}}; /* DVD specific ? */

static struct mpeg3_playkey *mpeg3_playkeys[] = 
{
	&mpeg3_pkey1a1, &mpeg3_pkey2a1, &mpeg3_pkey1b1,
	&mpeg3_pkey1a2, &mpeg3_pkey2a2, &mpeg3_pkey1b2,
	&mpeg3_pkey1a3, &mpeg3_pkey1b3,
	&mpeg3_pkey3a1, &mpeg3_pkey4a1,
	NULL
};

/*
 *
 *  some tables used for descrambling sectors and/or decrypting title keys
 *
 */

static unsigned char csstab1[256]=
{
	0x33,0x73,0x3b,0x26,0x63,0x23,0x6b,0x76,0x3e,0x7e,0x36,0x2b,0x6e,0x2e,0x66,0x7b,
	0xd3,0x93,0xdb,0x06,0x43,0x03,0x4b,0x96,0xde,0x9e,0xd6,0x0b,0x4e,0x0e,0x46,0x9b,
	0x57,0x17,0x5f,0x82,0xc7,0x87,0xcf,0x12,0x5a,0x1a,0x52,0x8f,0xca,0x8a,0xc2,0x1f,
	0xd9,0x99,0xd1,0x00,0x49,0x09,0x41,0x90,0xd8,0x98,0xd0,0x01,0x48,0x08,0x40,0x91,
	0x3d,0x7d,0x35,0x24,0x6d,0x2d,0x65,0x74,0x3c,0x7c,0x34,0x25,0x6c,0x2c,0x64,0x75,
	0xdd,0x9d,0xd5,0x04,0x4d,0x0d,0x45,0x94,0xdc,0x9c,0xd4,0x05,0x4c,0x0c,0x44,0x95,
	0x59,0x19,0x51,0x80,0xc9,0x89,0xc1,0x10,0x58,0x18,0x50,0x81,0xc8,0x88,0xc0,0x11,
	0xd7,0x97,0xdf,0x02,0x47,0x07,0x4f,0x92,0xda,0x9a,0xd2,0x0f,0x4a,0x0a,0x42,0x9f,
	0x53,0x13,0x5b,0x86,0xc3,0x83,0xcb,0x16,0x5e,0x1e,0x56,0x8b,0xce,0x8e,0xc6,0x1b,
	0xb3,0xf3,0xbb,0xa6,0xe3,0xa3,0xeb,0xf6,0xbe,0xfe,0xb6,0xab,0xee,0xae,0xe6,0xfb,
	0x37,0x77,0x3f,0x22,0x67,0x27,0x6f,0x72,0x3a,0x7a,0x32,0x2f,0x6a,0x2a,0x62,0x7f,
	0xb9,0xf9,0xb1,0xa0,0xe9,0xa9,0xe1,0xf0,0xb8,0xf8,0xb0,0xa1,0xe8,0xa8,0xe0,0xf1,
	0x5d,0x1d,0x55,0x84,0xcd,0x8d,0xc5,0x14,0x5c,0x1c,0x54,0x85,0xcc,0x8c,0xc4,0x15,
	0xbd,0xfd,0xb5,0xa4,0xed,0xad,0xe5,0xf4,0xbc,0xfc,0xb4,0xa5,0xec,0xac,0xe4,0xf5,
	0x39,0x79,0x31,0x20,0x69,0x29,0x61,0x70,0x38,0x78,0x30,0x21,0x68,0x28,0x60,0x71,
	0xb7,0xf7,0xbf,0xa2,0xe7,0xa7,0xef,0xf2,0xba,0xfa,0xb2,0xaf,0xea,0xaa,0xe2,0xff
};

static unsigned char lfsr1_bits0[256]=
{
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09,0x08,0x0b,0x0a,0x0d,0x0c,0x0f,0x0e,
	0x12,0x13,0x10,0x11,0x16,0x17,0x14,0x15,0x1b,0x1a,0x19,0x18,0x1f,0x1e,0x1d,0x1c,
	0x24,0x25,0x26,0x27,0x20,0x21,0x22,0x23,0x2d,0x2c,0x2f,0x2e,0x29,0x28,0x2b,0x2a,
	0x36,0x37,0x34,0x35,0x32,0x33,0x30,0x31,0x3f,0x3e,0x3d,0x3c,0x3b,0x3a,0x39,0x38,
	0x49,0x48,0x4b,0x4a,0x4d,0x4c,0x4f,0x4e,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,
	0x5b,0x5a,0x59,0x58,0x5f,0x5e,0x5d,0x5c,0x52,0x53,0x50,0x51,0x56,0x57,0x54,0x55,
	0x6d,0x6c,0x6f,0x6e,0x69,0x68,0x6b,0x6a,0x64,0x65,0x66,0x67,0x60,0x61,0x62,0x63,
	0x7f,0x7e,0x7d,0x7c,0x7b,0x7a,0x79,0x78,0x76,0x77,0x74,0x75,0x72,0x73,0x70,0x71,
	0x92,0x93,0x90,0x91,0x96,0x97,0x94,0x95,0x9b,0x9a,0x99,0x98,0x9f,0x9e,0x9d,0x9c,
	0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x89,0x88,0x8b,0x8a,0x8d,0x8c,0x8f,0x8e,
	0xb6,0xb7,0xb4,0xb5,0xb2,0xb3,0xb0,0xb1,0xbf,0xbe,0xbd,0xbc,0xbb,0xba,0xb9,0xb8,
	0xa4,0xa5,0xa6,0xa7,0xa0,0xa1,0xa2,0xa3,0xad,0xac,0xaf,0xae,0xa9,0xa8,0xab,0xaa,
	0xdb,0xda,0xd9,0xd8,0xdf,0xde,0xdd,0xdc,0xd2,0xd3,0xd0,0xd1,0xd6,0xd7,0xd4,0xd5,
	0xc9,0xc8,0xcb,0xca,0xcd,0xcc,0xcf,0xce,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,
	0xff,0xfe,0xfd,0xfc,0xfb,0xfa,0xf9,0xf8,0xf6,0xf7,0xf4,0xf5,0xf2,0xf3,0xf0,0xf1,
	0xed,0xec,0xef,0xee,0xe9,0xe8,0xeb,0xea,0xe4,0xe5,0xe6,0xe7,0xe0,0xe1,0xe2,0xe3
};

static unsigned char lfsr1_bits1[512]=
{
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,
	0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff,0x00,0x24,0x49,0x6d,0x92,0xb6,0xdb,0xff
};

/* Reverse the order of the bits within a byte.
 */
static unsigned char bit_reverse[256]=
{
	0x00,0x80,0x40,0xc0,0x20,0xa0,0x60,0xe0,0x10,0x90,0x50,0xd0,0x30,0xb0,0x70,0xf0,
	0x08,0x88,0x48,0xc8,0x28,0xa8,0x68,0xe8,0x18,0x98,0x58,0xd8,0x38,0xb8,0x78,0xf8,
	0x04,0x84,0x44,0xc4,0x24,0xa4,0x64,0xe4,0x14,0x94,0x54,0xd4,0x34,0xb4,0x74,0xf4,
	0x0c,0x8c,0x4c,0xcc,0x2c,0xac,0x6c,0xec,0x1c,0x9c,0x5c,0xdc,0x3c,0xbc,0x7c,0xfc,
	0x02,0x82,0x42,0xc2,0x22,0xa2,0x62,0xe2,0x12,0x92,0x52,0xd2,0x32,0xb2,0x72,0xf2,
	0x0a,0x8a,0x4a,0xca,0x2a,0xaa,0x6a,0xea,0x1a,0x9a,0x5a,0xda,0x3a,0xba,0x7a,0xfa,
	0x06,0x86,0x46,0xc6,0x26,0xa6,0x66,0xe6,0x16,0x96,0x56,0xd6,0x36,0xb6,0x76,0xf6,
	0x0e,0x8e,0x4e,0xce,0x2e,0xae,0x6e,0xee,0x1e,0x9e,0x5e,0xde,0x3e,0xbe,0x7e,0xfe,
	0x01,0x81,0x41,0xc1,0x21,0xa1,0x61,0xe1,0x11,0x91,0x51,0xd1,0x31,0xb1,0x71,0xf1,
	0x09,0x89,0x49,0xc9,0x29,0xa9,0x69,0xe9,0x19,0x99,0x59,0xd9,0x39,0xb9,0x79,0xf9,
	0x05,0x85,0x45,0xc5,0x25,0xa5,0x65,0xe5,0x15,0x95,0x55,0xd5,0x35,0xb5,0x75,0xf5,
	0x0d,0x8d,0x4d,0xcd,0x2d,0xad,0x6d,0xed,0x1d,0x9d,0x5d,0xdd,0x3d,0xbd,0x7d,0xfd,
	0x03,0x83,0x43,0xc3,0x23,0xa3,0x63,0xe3,0x13,0x93,0x53,0xd3,0x33,0xb3,0x73,0xf3,
	0x0b,0x8b,0x4b,0xcb,0x2b,0xab,0x6b,0xeb,0x1b,0x9b,0x5b,0xdb,0x3b,0xbb,0x7b,0xfb,
	0x07,0x87,0x47,0xc7,0x27,0xa7,0x67,0xe7,0x17,0x97,0x57,0xd7,0x37,0xb7,0x77,0xf7,
	0x0f,0x8f,0x4f,0xcf,0x2f,0xaf,0x6f,0xef,0x1f,0x9f,0x5f,0xdf,0x3f,0xbf,0x7f,0xff
};

/* ================================= Functions ====================================== */

/*
 * We use two LFSR's (seeded from some of the input data bytes) to
 * generate two streams of pseudo-random bits.  These two bit streams
 * are then combined by simply adding with carry to generate a final
 * sequence of pseudo-random bits which is stored in the buffer that
 * 'output' points to the end of - len is the size of this buffer.
 *
 * The first LFSR is of degree 25,  and has a polynomial of:
 * x^13 + x^5 + x^4 + x^1 + 1
 *
 * The second LSFR is of degree 17,  and has a (primitive) polynomial of:
 * x^15 + x^1 + 1
 *
 * I don't know if these polynomials are primitive modulo 2,  and thus
 * represent maximal-period LFSR's.
 *
 *
 * Note that we take the output of each LFSR from the new shifted in
 * bit,  not the old shifted out bit.  Thus for ease of use the LFSR's
 * are implemented in bit reversed order.
 *
 */

#define  BIT0(x) ((x) & 1)
#define  BIT1(x) (((x) >> 1) & 1)

static void generate_bits(unsigned char *output, int len, struct mpeg3_block const *s)
{
	unsigned long lfsr0, lfsr1;
	unsigned char carry;

	/* In order to ensure that the LFSR works we need to ensure that the
	 * initial values are non-zero.  Thus when we initialise them from
	 * the seed,  we ensure that a bit is set.
	 */
	lfsr0 = (s->b[0] << 17) | (s->b[1] << 9) | ((s->b[2] & ~7) << 1) | 8 | (s->b[2] & 7);
	lfsr1 = (s->b[3] << 9) | 0x100 | s->b[4];

	++output;

	carry = 0;
	do{
		int bit;
		unsigned char val;

		for (bit = 0, val = 0; bit < 8; ++bit) 
		{
			unsigned char o_lfsr0, o_lfsr1;	/* Actually only 1 bit each */
			unsigned char combined;

			o_lfsr0 = ((lfsr0 >> 24) ^ (lfsr0 >> 21) ^ (lfsr0 >> 20) ^ (lfsr0 >> 12)) & 1;
			  lfsr0 = (lfsr0 << 1) | o_lfsr0;

			o_lfsr1 = ((lfsr1 >> 16) ^ (lfsr1 >> 2)) & 1;
			  lfsr1 = (lfsr1 << 1) | o_lfsr1;

			combined = !o_lfsr1 + carry + !o_lfsr0;
			carry = BIT1(combined);
			val |= BIT0(combined) << bit;
		}
	
		*--output = val;
	}while (--len > 0);
}


/*
 * This encryption engine implements one of 32 variations
 * one the same theme depending upon the choice in the
 * varient parameter (0 - 31).
 *
 * The algorithm itself manipulates a 40 bit input into
 * a 40 bit output.
 * The parameter 'input' is 80 bits.  It consists of
 * the 40 bit input value that is to be encrypted followed
 * by a 40 bit seed value for the pseudo random number
 * generators.
 */
static void css_engine(int varient, unsigned char const *input, struct mpeg3_block *output)
{
	unsigned char cse, term, index;
	struct mpeg3_block temp1;
	struct mpeg3_block temp2;
	unsigned char bits[30];

	int i;

/* Feed the secret into the input values such that
 * we alter the seed to the LFSR's used above,  then
 * generate the bits to play with.
 */
	for(i = 5; --i >= 0; )
		temp1.b[i] = input[5 + i] ^ mpeg3css_secret[i] ^ mpeg3css_table2[i];

	generate_bits(&bits[29], sizeof bits, &temp1);

	/* This term is used throughout the following to
	 * select one of 32 different variations on the
	 * algorithm.
	 */
	cse = mpeg3css_varients[varient] ^ mpeg3css_table2[varient];

	/* Now the actual blocks doing the encryption.  Each
	 * of these works on 40 bits at a time and are quite
	 * similar.
	 */
	for(i = 5, term = 0; --i >= 0; term = input[i]) 
	{
		index = bits[25 + i] ^ input[i];
		index = mpeg3css_table1[index] ^ ~mpeg3css_table2[index] ^ cse;

		temp1.b[i] = mpeg3css_table2[index] ^ mpeg3css_table3[index] ^ term;
	}
	temp1.b[4] ^= temp1.b[0];

	for(i = 5, term = 0; --i >= 0; term = temp1.b[i]) 
	{
		index = bits[20 + i] ^ temp1.b[i];
		index = mpeg3css_table1[index] ^ ~mpeg3css_table2[index] ^ cse;

		temp2.b[i] = mpeg3css_table2[index] ^ mpeg3css_table3[index] ^ term;
	}
	temp2.b[4] ^= temp2.b[0];

	for (i = 5, term = 0; --i >= 0; term = temp2.b[i]) 
	{
		index = bits[15 + i] ^ temp2.b[i];
		index = mpeg3css_table1[index] ^ ~mpeg3css_table2[index] ^ cse;
		index = mpeg3css_table2[index] ^ mpeg3css_table3[index] ^ term;

		temp1.b[i] = mpeg3css_table0[index] ^ mpeg3css_table2[index];
	}
	temp1.b[4] ^= temp1.b[0];

	for (i = 5, term = 0; --i >= 0; term = temp1.b[i]) 
	{
		index = bits[10 + i] ^ temp1.b[i];
		index = mpeg3css_table1[index] ^ ~mpeg3css_table2[index] ^ cse;

		index = mpeg3css_table2[index] ^ mpeg3css_table3[index] ^ term;

		temp2.b[i] = mpeg3css_table0[index] ^ mpeg3css_table2[index];
	}
	temp2.b[4] ^= temp2.b[0];

	for (i = 5, term = 0; --i >= 0; term = temp2.b[i]) 
	{
		index = bits[5 + i] ^ temp2.b[i];
		index = mpeg3css_table1[index] ^ ~mpeg3css_table2[index] ^ cse;

		temp1.b[i] = mpeg3css_table2[index] ^ mpeg3css_table3[index] ^ term;
	}
	temp1.b[4] ^= temp1.b[0];

	for (i = 5, term = 0; --i >= 0; term = temp1.b[i]) 
	{
		index = bits[i] ^ temp1.b[i];
		index = mpeg3css_table1[index] ^ ~mpeg3css_table2[index] ^ cse;

		output->b[i] = mpeg3css_table2[index] ^ mpeg3css_table3[index] ^ term;
	}
}

static void crypt_key1(mpeg3_css_t *css, int varient, unsigned char const *challenge, struct mpeg3_block *key)
{
	static unsigned char perm_challenge[] = {1, 3, 0, 7, 5, 2, 9, 6, 4, 8};

	unsigned char scratch[10];
	int i;

	for (i = 9; i >= 0; i--)
		scratch[i] = challenge[perm_challenge[i]];

	css_engine(varient, scratch, key);
}

/* This shuffles the bits in varient to make perm_varient such that
 *                4 -> !3
 *                3 ->  4
 * varient bits:  2 ->  0  perm_varient bits
 *                1 ->  2
 *                0 -> !1
 */
static void crypt_key2(mpeg3_css_t *css, int varient, unsigned char const *challenge, struct mpeg3_block *key)
{
	static unsigned char perm_challenge[] = {6, 1, 9, 3, 8, 5, 7, 4, 0, 2};

	static unsigned char perm_varient[] = 
	{
		0x0a, 0x08, 0x0e, 0x0c, 0x0b, 0x09, 0x0f, 0x0d,
		0x1a, 0x18, 0x1e, 0x1c, 0x1b, 0x19, 0x1f, 0x1d,
		0x02, 0x00, 0x06, 0x04, 0x03, 0x01, 0x07, 0x05,
		0x12, 0x10, 0x16, 0x14, 0x13, 0x11, 0x17, 0x15
	};

	unsigned char scratch[10];
	int i;

	for(i = 9; i >= 0; i--)
		scratch[i] = css->challenge[perm_challenge[i]];

	css_engine(perm_varient[varient], scratch, key);
}

/* This shuffles the bits in varient to make perm_varient such that
 *                4 ->  0
 *                3 -> !1
 * varient bits:  2 -> !4  perm_varient bits
 *                1 ->  2
 *                0 ->  3
 */
static void crypt_bus_key(mpeg3_css_t *css, int varient, unsigned char const *challenge, struct mpeg3_block *key)
{
	static unsigned char perm_challenge[] = {4,0,3,5,7, 2,8,6,1,9};
	static unsigned char perm_varient[] = {
		0x12, 0x1a, 0x16, 0x1e, 0x02, 0x0a, 0x06, 0x0e,
		0x10, 0x18, 0x14, 0x1c, 0x00, 0x08, 0x04, 0x0c,
		0x13, 0x1b, 0x17, 0x1f, 0x03, 0x0b, 0x07, 0x0f,
		0x11, 0x19, 0x15, 0x1d, 0x01, 0x09, 0x05, 0x0d};

	unsigned char scratch[10];
	int i;

	for(i = 9; i >= 0; i--)
		scratch[i] = css->challenge[perm_challenge[i]];

	css_engine(perm_varient[varient], scratch, key);
}

static int get_asf(mpeg3_css_t *css)
{
	dvd_authinfo ai;

	ai.type = DVD_LU_SEND_ASF;
	ai.lsasf.agid = 0;
	ai.lsasf.asf = 0;

	if(ioctl(css->fd, DVD_AUTH, &ai))
	{
/* Exit here for a hard drive or unencrypted CD-ROM. */
		return 1;
	}

	return 0;
}

static int authenticate_drive(mpeg3_css_t *css, const unsigned char *key)
{
	int i;

	for(i = 0; i < 5; i++)
		css->key1.b[i] = key[4 - i];

	for(i = 0; i < 32; ++i)
	{
		crypt_key1(css, i, css->challenge, &(css->keycheck));
		if(memcmp(css->keycheck.b, css->key1.b, 5) == 0)
		{
			css->varient = i;
			return 0;
		}
	}

	if (css->varient == -1) return 1;

	return 0;
}

/* Simulation of a non-CSS compliant host (i.e. the authentication fails,
 * but idea is here for a real CSS compliant authentication scheme). */
static int hostauth(mpeg3_css_t *css, dvd_authinfo *ai)
{
	int i;

	switch(ai->type) 
	{
/* Host data receive (host changes state) */
		case DVD_LU_SEND_AGID:
			ai->type = DVD_HOST_SEND_CHALLENGE;
			break;

		case DVD_LU_SEND_KEY1:
/* printf("Key 1: %02x %02x %02x %02x %02x\n",  */
/* 			ai->lsk.key[4], ai->lsk.key[3], ai->lsk.key[2], ai->lsk.key[1], ai->lsk.key[0]); */
			if(authenticate_drive(css, ai->lsk.key)) 
			{
				ai->type = DVD_AUTH_FAILURE;
				return 1;
			}
			ai->type = DVD_LU_SEND_CHALLENGE;
			break;

		case DVD_LU_SEND_CHALLENGE:
			for(i = 0; i < 10; i++)
				css->challenge[i] = ai->hsc.chal[9-i];
			crypt_key2(css, css->varient, css->challenge, &(css->key2));
			ai->type = DVD_HOST_SEND_KEY2;
			break;

/* Host data send */
		case DVD_HOST_SEND_CHALLENGE:
			for(i = 0; i < 10; i++)
				ai->hsc.chal[9 - i] = css->challenge[i];
/* Returning data, let LU change state */
			break;

		case DVD_HOST_SEND_KEY2:
			for(i = 0; i < 5; i++)
			{
				ai->hsk.key[4 - i] = css->key2.b[i];
			}
/* printf("Key 2: %02x %02x %02x %02x %02x\n",  */
/* 			ai->hsk.key[4], ai->hsk.key[3], ai->hsk.key[2], ai->hsk.key[1], ai->hsk.key[0]); */
/* Returning data, let LU change state */
			break;

		default:
			fprintf(stderr, "Got invalid state %d\n", ai->type);
			return 1;
	}

	return 0;
}

static int get_title_key(mpeg3_css_t *css, int agid, int lba, unsigned char *key)
{
	dvd_authinfo ai;
	int i;

	ai.type = DVD_LU_SEND_TITLE_KEY;

	ai.lstk.agid = agid;
	ai.lstk.lba = lba;

	if(ioctl(css->fd, DVD_AUTH, &ai))
	{
		//perror("GetTitleKey");
		return 1;
	}

	for (i = 0; i < 5; i++)
	{
		ai.lstk.title_key[i] ^= key[4 - (i % 5)];
	}

/* Save the title key */
	for(i = 0; i < 5; i++)
	{
		css->title_key[i] = ai.lstk.title_key[i];
	}

	return 0;
}

static int get_disk_key(mpeg3_css_t *css, int agid, unsigned char *key)
{
	dvd_struct s;
	int	index, i;

	s.type = DVD_STRUCT_DISCKEY;
	s.disckey.agid = agid;
	memset(s.disckey.value, 0, MPEG3_DVD_PACKET_SIZE);
	if(ioctl(css->fd, DVD_READ_STRUCT, &s) < 0)
	{
		/*perror("get_disk_key"); */
		return 1;
	}

	for(index = 0; index < sizeof s.disckey.value; index ++)
		s.disckey.value[index] ^= key[4 - (index%5)];

/* Save disk key */
	for(i = 0; i < MPEG3_DVD_PACKET_SIZE; i++)
		css->disk_key[i] = s.disckey.value[i];

	return 0;
}

static int validate(mpeg3_css_t *css, int lba, int do_title)
{
	dvd_authinfo ai;
	dvd_struct dvds;
	int result = 0;
	int i, rv, tries, agid;
	
	memset(&ai, 0, sizeof (ai));
	memset(&dvds, 0, sizeof (dvds));
	
	if(get_asf(css)) return 1;

/* Init sequence, request AGID */
	for(tries = 1, rv = -1; rv == -1 && tries < 4; tries++)
	{
		ai.type = DVD_LU_SEND_AGID;
		ai.lsa.agid = 0;
		rv = ioctl(css->fd, DVD_AUTH, &ai);
		if(rv == -1)
		{
/*			perror("validate: request AGID"); */
			ai.type = DVD_INVALIDATE_AGID;
			ai.lsa.agid = 0;
			ioctl(css->fd, DVD_AUTH, &ai);
		}
	}
	if(tries >= 4) return 1;

	for(i = 0; i < 10; i++) css->challenge[i] = i;

/* Send AGID to host */
	if(hostauth(css, &ai)) return 1;

/* Get challenge from host */
	if(hostauth(css, &ai)) return 1;
	agid = ai.lsa.agid;

/* Send challenge to LU */
	if(ioctl(css->fd, DVD_AUTH, &ai) < 0) return 1;

/* Get key1 from LU */
	if(ioctl(css->fd, DVD_AUTH, &ai) < 0) return 1;

/* Send key1 to host */
	if(hostauth(css, &ai)) return 1;

/* Get challenge from LU */
	if(ioctl(css->fd, DVD_AUTH, &ai) < 0) return 1;

/* Send challenge to host */
	if(hostauth(css, &ai)) return 1;

/* Get key2 from host */
	if(hostauth(css, &ai)) return 1;

/* Send key2 to LU */
	if(ioctl(css->fd, DVD_AUTH, &ai) < 0) 
	{
		perror("validate: Send key2 to LU");
		return 1;
	}

	if(ai.type == DVD_AUTH_FAILURE)
	{
		fprintf(stderr, "validate: authorization failed\n");
		return 1;
	}
	memcpy(css->challenge, css->key1.b, 5);
	memcpy(css->challenge + 5, css->key2.b, 5);
	crypt_bus_key(css, css->varient, css->challenge, &(css->keycheck));

	get_asf(css);

	if(do_title)
		return get_title_key(css, agid, lba, css->keycheck.b);
	else
		return get_disk_key(css, agid, css->keycheck.b);

	return 0;
}

static int validate_path(mpeg3_css_t *css, int do_title)
{
	int result = 0;
	int lba = 0, file_fd;

	if(do_title)
	{
		if((file_fd = open(css->path, O_RDONLY)) == -1)
		{
			perror("validate_path: open");
			return 1;
		}

		if(ioctl(file_fd, FIBMAP, &lba) != 0)
		{
			perror("validate_path: FIBMAP");
			close(file_fd);
			return 1;
		}

		close(file_fd);
	}

	result = mpeg3io_device(css->path, css->device_path);

//printf("validate_path 1 %d\n", result);
	if(!result) result = (css->fd = open(css->device_path, O_RDONLY | O_NONBLOCK)) < 0;
//printf("validate_path 2 %d\n", result);

	if(!result) result = validate(css, lba, do_title);

//printf("validate_path 3 %d\n", result);
/* Definitely encrypted if we got here. */


	if(!result) css->encrypted = 1;

	close(css->fd);
	return result;
}

/*
 *
 * this function is only used internally when decrypting title key
 *
 */
static void title_key(unsigned char *key, unsigned char *im, unsigned char invert)
{
	unsigned int lfsr1_lo, lfsr1_hi, lfsr0, combined;
	unsigned char o_lfsr0, o_lfsr1;
	unsigned char k[5];
	int i;

	lfsr1_lo = im[0] | 0x100;
	lfsr1_hi = im[1];

	lfsr0 = ((im[4] << 17) | (im[3] << 9) | (im[2] << 1)) + 8 - (im[2]&7);
	lfsr0 = (bit_reverse[lfsr0 & 0xff] << 24) | (bit_reverse[(lfsr0 >> 8) & 0xff] << 16)
		  | (bit_reverse[(lfsr0 >> 16) & 0xff] << 8) | bit_reverse[(lfsr0 >> 24) & 0xff];

	combined = 0;
	for (i = 0; i < 5; ++i) {
		o_lfsr1		= lfsr1_bits0[lfsr1_hi] ^ lfsr1_bits1[lfsr1_lo];
		  lfsr1_hi	= lfsr1_lo>>1;
		  lfsr1_lo	= ((lfsr1_lo&1)<<8) ^ o_lfsr1;
		o_lfsr1		= bit_reverse[o_lfsr1];

		/*o_lfsr0 = (lfsr0>>7)^(lfsr0>>10)^(lfsr0>>11)^(lfsr0>>19);*/
		o_lfsr0 = (((((((lfsr0>>8)^lfsr0)>>1)^lfsr0)>>3)^lfsr0)>>7);
		  lfsr0 = (lfsr0>>8)|(o_lfsr0<<24);

		combined += (o_lfsr0 ^ invert) + o_lfsr1;
		k[i] = combined & 0xff;
		combined >>= 8;
	}

	key[4] = k[4] ^ csstab1[key[4]] ^ key[3];
	key[3] = k[3] ^ csstab1[key[3]] ^ key[2];
	key[2] = k[2] ^ csstab1[key[2]] ^ key[1];
	key[1] = k[1] ^ csstab1[key[1]] ^ key[0];
	key[0] = k[0] ^ csstab1[key[0]] ^ key[4];

	key[4] = k[4] ^ csstab1[key[4]] ^ key[3];
	key[3] = k[3] ^ csstab1[key[3]] ^ key[2];
	key[2] = k[2] ^ csstab1[key[2]] ^ key[1];
	key[1] = k[1] ^ csstab1[key[1]] ^ key[0];
	key[0] = k[0] ^ csstab1[key[0]];
}

/*
 *
 * this function decrypts a title key with the specified disk key
 *
 * tkey: the unobfuscated title key (XORed with BusKey)
 * dkey: the unobfuscated disk key (XORed with BusKey)
 *       2048 bytes in length (though only 5 bytes are needed, see below)
 *
 * use the result returned in tkey with css_descramble
 *
 */

static int decrypt_title_key(mpeg3_css_t *css, unsigned char *dkey, unsigned char *tkey)
{
	unsigned char test[5], pretkey[5];
	int i = 0;

	for(i = 0; mpeg3_playkeys[i]; i++)
	{
		memcpy(pretkey, dkey + mpeg3_playkeys[i]->offset, 5);
		title_key(pretkey, mpeg3_playkeys[i]->key, 0);

		memcpy(test, dkey, 5);
		title_key(test, pretkey, 0);

		if(memcmp(test, pretkey, 5) == 0)
			break;
	}

	if(!mpeg3_playkeys[i])
	{
		fprintf(stderr, "mpeg3_decrypttitlekey: Shit - Need key %d\n", i + 1);
		return 1;
	}

	title_key(css->title_key, pretkey, 0xff);

	return 0;
}

/*
 *
 * The descrambling core
 *
 * sec: encrypted sector (2048 bytes)
 * key: decrypted title key obtained from css_decrypttitlekey
 *
 */

#define SALTED(i) (key[i] ^ sec[0x54 - offset + (i)])

static void descramble(unsigned char *sec, unsigned char *key, int offset)
{
	unsigned int lfsr1_lo, lfsr1_hi, lfsr0, combined;
	unsigned char o_lfsr0, o_lfsr1;
	unsigned char *end = sec + 0x800 - offset;

	if(offset > 0x54)
		fprintf(stderr, "mpeg3css.c: descramble: offset > 0x54 offset=%x\n", offset);

	lfsr1_lo = SALTED(0) | 0x100;
	lfsr1_hi = SALTED(1);

	lfsr0 = ((SALTED(4) << 17) | (SALTED(3) << 9) | (SALTED(2) << 1)) + 8 - (SALTED(2) & 7);
	lfsr0 = (bit_reverse[lfsr0 & 0xff] << 24) | (bit_reverse[(lfsr0 >> 8) & 0xff] << 16)
		  | (bit_reverse[(lfsr0 >> 16) & 0xff] << 8) | bit_reverse[(lfsr0 >> 24) & 0xff];

	sec += 0x80 - offset;
	combined = 0;
	while(sec != end)
	{
		o_lfsr1		= lfsr1_bits0[lfsr1_hi] ^ lfsr1_bits1[lfsr1_lo];
		  lfsr1_hi	= lfsr1_lo >> 1;
		  lfsr1_lo	= ((lfsr1_lo&1) << 8) ^ o_lfsr1;
		o_lfsr1		= bit_reverse[o_lfsr1];

		/*o_lfsr0 = (lfsr0 >> 7) ^ (lfsr0 >> 10) ^ (lfsr0 >> 11) ^ (lfsr0 >> 19);*/
		o_lfsr0 = (((((((lfsr0 >> 8) ^ lfsr0) >> 1) ^ lfsr0) >> 3) ^ lfsr0) >> 7);
		  lfsr0 = (lfsr0 >> 8) | (o_lfsr0 << 24);

		combined += o_lfsr0 + (unsigned char)~o_lfsr1;

		*sec = csstab1[*sec] ^ (combined & 0xff);
		sec++;

		combined >>= 8;
	}
//printf("descramble\n");
}

/* =============================== Entry Points ================================= */

mpeg3_css_t* mpeg3_new_css()
{
	mpeg3_css_t *css = calloc(1, sizeof(mpeg3_css_t));
	css->varient = -1;
	return css;
}

int mpeg3_delete_css(mpeg3_css_t *css)
{
	free(css);
	return 0;
}

int mpeg3_get_keys(mpeg3_css_t *css, char *path)
{
	int result = 0;

	strcpy(css->path, path);
/* Get disk key */
	result = validate_path(css, 0);
/* Get title key */
	if(!result) result = validate_path(css, 1);
/* Descramble the title key */
	if(!result) result = decrypt_title_key(css, css->disk_key, css->title_key);

	return css->encrypted ? result : 0;
}

/* sector is the full 2048 byte sector */
int mpeg3_decrypt_packet(mpeg3_css_t *css, unsigned char *sector, int offset)
{
//printf("mpeg3_decrypt_packet %d\n", css->encrypted);
	if(!css->encrypted) return 0;     /* Not encrypted */
	descramble(sector, css->title_key, offset);
	return 0;
}

#else // HAVE_CSS

#include "mpeg3css_fake.c"

#endif
