﻿/*

Nintendont (Kernel) - Playing Gamecubes in Wii mode on a Wii U

Copyright (C) 2013  crediar

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "Patch.h"
#include "string.h"
#include "ff.h"
#include "dol.h"
#include "elf.h"
#include "PatchCodes.h"
#include "Config.h"
#include "global.h"
#include "patches.c"
#include "SI.h"

//#define DEBUG_DSP  // Very slow!! Replace with raw dumps?

#define GAME_ID		(read32(0))
#define TITLE_ID	(GAME_ID >> 8)

u32 CardLowestOff = 0;
u32 POffset = 0x2F00;
vu32 Region = 0;
extern FIL GameFile;
extern vu32 TRIGame;
extern u32 SystemRegion;

extern int dbgprintf( const char *fmt, ...);

//osGetCount, double from 1.1574 to 0.7716
#define DBL_1_1574		0x3ff284b5dcc63f14ull
#define DBL_0_7716		0x3fe8b0f27bb2fec5ull

unsigned char OSReportDM[] =
{
	0x7C, 0x08, 0x02, 0xA6, 0x90, 0x01, 0x00, 0x04, 0x90, 0xE1, 0x00, 0x08, 0x3C, 0xE0, 0xC0, 0x00, 
	0x90, 0x67, 0x18, 0x60, 0x90, 0x87, 0x18, 0x64, 0x90, 0xA7, 0x18, 0x68, 0x90, 0xC7, 0x18, 0x6C, 
	0x90, 0xE7, 0x18, 0x70, 0x91, 0x07, 0x18, 0x74, 0x80, 0x07, 0x18, 0x60, 0x7C, 0x00, 0x18, 0x00, 
	0x41, 0x82, 0xFF, 0xF8, 0x80, 0xE1, 0x00, 0x08, 0x80, 0x01, 0x00, 0x04, 0x7C, 0x08, 0x03, 0xA6, 
	0x4E, 0x80, 0x00, 0x20, 
} ;

const unsigned char DSPHashes[][0x14] =
{
	{
		0xC9, 0x7D, 0x1E, 0xD0, 0x71, 0x90, 0x47, 0x3F, 0x6A, 0x66, 0x42, 0xB2, 0x7E, 0x4A, 0xDB, 0xCD, 0xB6, 0xF8, 0x8E, 0xC3,			//	0 Dolphin=0x86840740=Zelda WW
	},
	{
		0x21, 0xD0, 0xC0, 0xEE, 0x25, 0x3D, 0x8C, 0x9E, 0x02, 0x58, 0x66, 0x7F, 0x3C, 0x1B, 0x11, 0xBC, 0x90, 0x1F, 0x33, 0xE2,			//	1 Dolphin=0x56d36052=Super Mario Sunshine
	},
	{
		0xBA, 0xDC, 0x60, 0x15, 0x33, 0x33, 0x28, 0xED, 0xB1, 0x0E, 0x72, 0xF2, 0x5B, 0x5A, 0xFB, 0xF3, 0xEF, 0x90, 0x30, 0x90,			//	2 Dolphin=0x4e8a8b21=Sonic Mega Collection
	},
	{
		0xBC, 0x17, 0x36, 0x81, 0x7A, 0x14, 0xB2, 0x1C, 0xCB, 0xF7, 0x3A, 0xD6, 0x8F, 0xDA, 0x57, 0xF8, 0x43, 0x78, 0x1A, 0xAE,			//	3 Dolphin=0xe2136399=Mario Party 5
	},
	{
		0xD9, 0x39, 0x63, 0xE3, 0x91, 0xD1, 0xA8, 0x5E, 0x4D, 0x5F, 0xD9, 0xC2, 0x9A, 0xF9, 0x3A, 0xA9, 0xAF, 0x8D, 0x4E, 0xF2,			//	4 Dolphin=0x07f88145=Zelda:OOT
	},
	{
		0xD8, 0x12, 0xAC, 0x09, 0xDC, 0x24, 0x50, 0x6B, 0x0D, 0x73, 0x3B, 0xF5, 0x39, 0x45, 0x1A, 0x23, 0x85, 0xF3, 0xC8, 0x79,			//	5 Dolphin=0x2fcdf1ec=Mario Kart DD
	},	
	{
		0x37, 0x44, 0xC3, 0x82, 0xC8, 0x98, 0x42, 0xD4, 0x9D, 0x68, 0x83, 0x1C, 0x2B, 0x06, 0x7E, 0xC7, 0xE8, 0x64, 0x32, 0x44,			//	6 Dolphin=0x3daf59b9=Star Fox Assault
	},
	{
		0xDA, 0x39, 0xA3, 0xEE, 0x5E, 0x6B, 0x4B, 0x0D, 0x32, 0x55, 0xBF, 0xEF, 0x95, 0x60, 0x18, 0x90, 0xAF, 0xD8, 0x07, 0x09,			//	7 0-Length DSP - Error
	},	 
	{
		0x8E, 0x5C, 0xCA, 0xEA, 0xA9, 0x84, 0x87, 0x02, 0xFB, 0x5C, 0x19, 0xD4, 0x18, 0x6E, 0xA7, 0x7B, 0xE5, 0xB8, 0x71, 0x78,			//	8 Dolphin=0x6CA33A6D=Donkey Kong Jungle Beat
	},	 
	{
		0xBC, 0x1E, 0x0A, 0x75, 0x09, 0xA6, 0x3E, 0x7C, 0xE6, 0x30, 0x44, 0xBE, 0xCC, 0x8D, 0x67, 0x1F, 0xA7, 0xC7, 0x44, 0xE5,			//	9 Dolphin=0x3ad3b7ac=Paper Mario The Thousand Year Door
	},	 
	{
		0x14, 0x93, 0x40, 0x30, 0x0D, 0x93, 0x24, 0xE3, 0xD3, 0xFE, 0x86, 0xA5, 0x68, 0x2F, 0x4C, 0x13, 0x38, 0x61, 0x31, 0x6C,			//	10 Dolphin=0x4be6a5cb=AC
	},	
	{
		0x09, 0xF1, 0x6B, 0x48, 0x57, 0x15, 0xEB, 0x3F, 0x67, 0x3E, 0x19, 0xEF, 0x7A, 0xCF, 0xE3, 0x60, 0x7D, 0x2E, 0x4F, 0x02,			//	11 Dolphin=0x42f64ac4=Luigi
	},
	{
		0x80, 0x01, 0x60, 0xDF, 0x89, 0x01, 0x9E, 0xE3, 0xE8, 0xF7, 0x47, 0x2C, 0xE0, 0x1F, 0xF6, 0x80, 0xE9, 0x85, 0xB0, 0x24,			//	12 Dolphin=0x267fd05a=Pikmin PAL
	},
	{
		0xB4, 0xCB, 0xC0, 0x0F, 0x51, 0x2C, 0xFE, 0xE5, 0xA4, 0xBA, 0x2A, 0x59, 0x60, 0x8A, 0xEB, 0x8C, 0x86, 0xC4, 0x61, 0x45,			//	13 Dolphin=0x6ba3b3ea=IPL NTSC 1.2
	},
	{
		0xA5, 0x13, 0x45, 0x90, 0x18, 0x30, 0x00, 0xB1, 0x34, 0x44, 0xAE, 0xDB, 0x61, 0xC5, 0x12, 0x0A, 0x72, 0x66, 0x07, 0xAA,			//	14 Dolphin=0x24b22038=IPL NTSC 1.0
	},
	{
		0x9F, 0x3C, 0x9F, 0x9E, 0x05, 0xC7, 0xD5, 0x0B, 0x38, 0x49, 0x2F, 0x2C, 0x68, 0x75, 0x30, 0xFD, 0xE8, 0x6F, 0x9B, 0xCA,			//	15 Dolphin=0x3389a79e=Metroid Prime Trilogy Wii (Needed?)
	},
};

const unsigned char DSPPattern[][0x10] =
{
	{
		0x02, 0x9F, 0x00, 0x12, 0x00, 0x00, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00,		//	0 Hash 12, 1, 0, 5, 8
	},
	{
		0x02, 0x9F, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00, 0x02, 0xFF, 0x00, 0x00,		//	1 Hash 14, 13, 11, 10
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x02, 0x9F, 0x0C, 0x10, 0x02, 0x9F, 0x0C, 0x1F, 0x02, 0x9F, 0x0C, 0x3B,		//	2 Hash 2
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x02, 0x9F, 0x0E, 0x76, 0x02, 0x9F, 0x0E, 0x85, 0x02, 0x9F, 0x0E, 0xA1,		//	3 Hash 3
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x02, 0x9F, 0x0E, 0xB3, 0x02, 0x9F, 0x0E, 0xC2, 0x02, 0x9F, 0x0E, 0xDE,		//	4 Hash 4
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x02, 0x9F, 0x0E, 0x71, 0x02, 0x9F, 0x0E, 0x80, 0x02, 0x9F, 0x0E, 0x9C,		//	5 Hash 9
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x02, 0x9F, 0x0E, 0x88, 0x02, 0x9F, 0x0E, 0x97, 0x02, 0x9F, 0x0E, 0xB3,		//	6 Hash 6
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x02, 0x9F, 0x0D, 0xB0, 0x02, 0x9F, 0x0D, 0xBF, 0x02, 0x9F, 0x0D, 0xDB,		//	7 Hash 15
	},
};

typedef struct DspMatch
{
	u32 Length;
	u32 Pattern;
	u32 SHA1;
} DspMatch;

const DspMatch DspMatches[] =
{
	// Order Patterns together by increasing Length
	// Length, Pattern,   SHA1
	{ 0x00001A60,    0,     12 },
	{ 0x00001CE0,    0,      1 },
	{ 0x00001D20,    0,      0 },
	{ 0x00001D20,    0,      5 },
	{ 0x00001F00,    0,      8 },
	{ 0x00001280,    1,     14 },
	{ 0x00001760,    1,     13 },
	{ 0x000017E0,    1,     11 },
	{ 0x00001A00,    1,     10 },
	{ 0x000019E0,    2,      2 },
	{ 0x00001EC0,    3,      3 },
	{ 0x00001F20,    4,      4 },
	{ 0x00001EC0,    5,      9 },
	{ 0x00001F00,    6,      6 },
	{ 0x00001D40,    7,     15 },
};

#define AX_DSP_NO_DUP3 (0xFFFF)
void PatchAX_Dsp(u32 ptr, u32 Dup1, u32 Dup2, u32 Dup3, u32 Dup2Offset)
{
	static const u32 MoveLength = 0x22;
	static const u32 CopyLength = 0x12;
	static const u32 CallLength = 0x2 + 0x2; // call (2) ; jmp (2)
	static const u32 New1Length = 0x1 * 3 + 0x2 + 0x7; // 3 * orc (1) ; jmp (2) ; patch (7)
	static const u32 New2Length = 0x1; // ret
	u32 SourceAddr = Dup1 + MoveLength;
	u32 DestAddr = Dup2 + CallLength + CopyLength + New2Length;
	u32 Dup2PAddr = DestAddr; // Dup2+0x17 (0xB Patch fits before +0x22)
	u32 Tmp;

	DestAddr--;
	W16((u32)ptr + (DestAddr)* 2, 0x02DF);  // ret
	while (SourceAddr >= Dup1 + 1)  // ensure include Dup1 below
	{
		SourceAddr -= 2;
		Tmp = R32((u32)ptr + (SourceAddr)* 2); // original instructions
		if ((Tmp & 0xFFFFFFFF) == 0x195E2ED1) // lrri        $AC0.M srs         @SampleFormat, $AC0.M
		{
			DestAddr -= 7;
			W32((u32)ptr + (DestAddr + 0x0) * 2, 0x03400003); // andi        $AC1.M, #0x0003
			W32((u32)ptr + (DestAddr + 0x2) * 2, 0x8100009E); // clr         $ACC0 -
			W32((u32)ptr + (DestAddr + 0x4) * 2, 0x200002CA); // lri         $AC0.M, 0x2000 lsrn
			W16((u32)ptr + (DestAddr + 0x6) * 2, 0x1FFE);     // mrr     $AC1.M, $AC0.M
			Tmp = Tmp | 0x00010100; // lrri        $AC1.M srs         @SampleFormat, $AC1.M
		}
		DestAddr -= 2;
		W32((u32)ptr + (DestAddr)* 2, Tmp); // unchanged
		if ((Tmp & 0x0000FFF1) == 0x00002ED0) // srs @ACxAH, $AC0.M
		{
			DestAddr -= 1;
			W32((u32)ptr + (DestAddr)* 2, (Tmp & 0xFFFF0000) | 0x3E00); //  orc AC0.M AC1.M
		}
		if (DestAddr == Dup2 + CallLength) // CopyLength must be even
		{
			DestAddr = Dup1 + CallLength + MoveLength - CopyLength + New1Length;
			DestAddr -= 2;
			W32((u32)ptr + (DestAddr)* 2, 0x029F0000 | (Dup2 + CallLength)); // jmp Dup2+4
		}
	}
	W32((u32)ptr + (Dup1 + 0x00) * 2, 0x02BF0000 | (Dup1 + CallLength)); // call Dup1+4
	W32((u32)ptr + (Dup1 + 0x02) * 2, 0x029F0000 | (Dup1 + MoveLength)); // jmp Dup1+0x22
	W32((u32)ptr + (Dup2 + 0x00) * 2, 0x02BF0000 | (Dup1 + CallLength)); // call Dup1+4
	W32((u32)ptr + (Dup2 + 0x02) * 2, 0x029F0000 | (Dup2 + MoveLength)); // jmp Dup2+0x22
	Tmp = R32((u32)ptr + (Dup1 + 0x98) * 2); // original instructions
	W32((u32)ptr + (Dup1 + 0x98) * 2, 0x02BF0000 | (Dup2PAddr)); // call Dup2PAddr
	W32((u32)ptr + (Dup2 + Dup2Offset) * 2, 0x02BF0000 | (Dup2PAddr)); // call Dup2PAddr

	W16((u32)ptr + (Dup2PAddr + 0x00) * 2, Tmp >> 16); //  original instructions (Start of Dup2PAddr [0xB long])
	W32((u32)ptr + (Dup2PAddr + 0x01) * 2, 0x27D10340); // lrs         $AC1.M, @SampleFormat -
	W32((u32)ptr + (Dup2PAddr + 0x03) * 2, 0x00038100); // andi        $AC1.M, #0x0003 clr         $ACC0
	W32((u32)ptr + (Dup2PAddr + 0x05) * 2, 0x009E1FFF); // lri         $AC0.M, #0x1FFF
	W16((u32)ptr + (Dup2PAddr + 0x07) * 2, 0x02CA);     // lsrn
	W16((u32)ptr + (Dup2PAddr + 0x08) * 2, Tmp & 0xFFFF); //  original instructions
	W32((u32)ptr + (Dup2PAddr + 0x09) * 2, 0x3D0002DF); // andc        $AC1.M, $AC0.M ret

	if (Dup3 != AX_DSP_NO_DUP3)
	{
		W32((u32)ptr + (Dup3 + 0x00) * 2, 0x02BF0000 | (Dup1 + CallLength)); // call Dup1+4
		W32((u32)ptr + (Dup3 + 0x02) * 2, 0x029F0000 | (Dup3 + MoveLength)); // jmp Dup3+0x22

		W32((u32)ptr + (Dup3 + 0x04) * 2, 0x27D10340); // lrs         $AC1.M, @SampleFormat -
		W32((u32)ptr + (Dup3 + 0x06) * 2, 0x00038100); // andi        $AC1.M, #0x0003 clr         $ACC0
		W32((u32)ptr + (Dup3 + 0x08) * 2, 0x009E1FFF); // lri         $AC0.M, #0x1FFF
		W16((u32)ptr + (Dup3 + 0x0A) * 2, 0x02CA);     // lsrn
		Tmp = R32((u32)ptr + (Dup3 + 0x5D) * 2); // original instructions
		W16((u32)ptr + (Dup3 + 0x0B) * 2, Tmp >> 16); //  original instructions
		W16((u32)ptr + (Dup3 + 0x0C) * 2, 0x3D00); // andc        $AC1.M, $AC0.M
		W16((u32)ptr + (Dup3 + 0x0D) * 2, Tmp & 0xFFFF); //  original instructions
		Tmp = R32((u32)ptr + (Dup3 + 0x5F) * 2); // original instructions
		W32((u32)ptr + (Dup3 + 0x0E) * 2, Tmp); //  original instructions includes ret

		W32((u32)ptr + (Dup3 + 0x5D) * 2, 0x029F0000 | (Dup3 + CallLength)); // jmp Dup3+0x4
	}
	return;
}

void PatchZelda_Dsp(u32 DspAddress, u32 PatchAddr, u32 OrigAddress, bool Split, bool KeepOrig)
{
	u32 Tmp = R32(DspAddress + (OrigAddress + 0) * 2); // original instructions at OrigAddress
	if (Split)
	{
		W32(DspAddress + (PatchAddr + 0) * 2, (Tmp & 0xFFFF0000) | 0x00000260); // split original instructions at OrigAddress
		W32(DspAddress + (PatchAddr + 2) * 2, 0x10000000 | (Tmp & 0x0000FFFF)); // ori $AC0.M, 0x1000 in between
	}
	else
	{
		W32(DspAddress + (PatchAddr + 0) * 2, 0x02601000); // ori $AC0.M, 0x1000
		W32(DspAddress + (PatchAddr + 2) * 2, Tmp);        // original instructions at OrigAddress
	}
	if (KeepOrig)
	{
		Tmp = R32(DspAddress + (PatchAddr + 4) * 2);       // original instructions at end of patch
		Tmp = (Tmp & 0x0000FFFF) | 0x02DF0000;             // ret/
	}
	else
		Tmp = 0x02DF02DF;                                  // ret/ret
	W32(DspAddress + (PatchAddr + 4) * 2, Tmp);
	W32(DspAddress + (OrigAddress + 0) * 2, 0x02BF0000 | PatchAddr);  // call PatchAddr
}
void PatchZeldaInPlace_Dsp(u32 DspAddress, u32 OrigAddress)
{
	W32(DspAddress + (OrigAddress + 0) * 2, 0x009AFFFF); // lri $AX0.H, #0xffff
	W32(DspAddress + (OrigAddress + 2) * 2, 0x2AD62AD7); // srs @ACEAH, $AX0.H/srs @ACEAL, $AX0.H
	W32(DspAddress + (OrigAddress + 4) * 2, 0x02601000); // ori $AC0.M, 0x1000
}

void DoDSPPatch( char *ptr, u32 Version )
{
	switch (Version)
	{
		case 0:		// Zelda:WW
		{
			// 5F8 - part of halt routine
			PatchZelda_Dsp( (u32)ptr, 0x05F8, 0x05B1, true, false );
			PatchZeldaInPlace_Dsp( (u32)ptr, 0x0566 );
		} break;
		case 1:		// Mario Sunshine
		{
			// E66 - unused
			PatchZelda_Dsp( (u32)ptr, 0xE66, 0x531, false, false );
			PatchZelda_Dsp( (u32)ptr, 0xE66, 0x57C, false, false );  // same orig instructions
		} break;
		case 2:		// SSBM
		{
			PatchAX_Dsp( (u32)ptr, 0x5A8, 0x65D, 0x707, 0x8F );
		} break;
		case 3:		// Mario Party 5
		{
			PatchAX_Dsp( (u32)ptr, 0x6A3, 0x758, 0x802, 0x8F );
		} break;
		case 4:		// Beyond Good and Evil
		{
			PatchAX_Dsp( (u32)ptr, 0x6E0, 0x795, 0x83F, 0x8F );
		} break;
		case 5:		// Mario Kart Double Dash
		{		
			// 5F8 - part of halt routine
			PatchZelda_Dsp( (u32)ptr, 0x05F8, 0x05B1, true, false );
			PatchZeldaInPlace_Dsp( (u32)ptr, 0x0566 );
		} break;
		case 6:		// Star Fox Assault
		{
			PatchAX_Dsp( (u32)ptr, 0x69E, 0x753, 0x814, 0xA4 );
		} break;
		case 7:		// 0 Length DSP
		{
			dbgprintf( "Error! 0 Length DSP\r\n" );
		} break;
		case 8:		// Donkey Kong Jungle Beat
		{
			// 6E5 - part of halt routine
			PatchZelda_Dsp( (u32)ptr, 0x06E5, 0x069E, true, false );
			PatchZeldaInPlace_Dsp( (u32)ptr, 0x0653 );
		} break; 
		case 9:		// Paper Mario The Thousand Year Door
		{
			PatchAX_Dsp( (u32)ptr, 0x69E, 0x753, 0x7FD, 0x8F );
		} break;
		case 10:	// Animal Crossing
		{
			// CF4 - unused
			PatchZelda_Dsp( (u32)ptr, 0x0CF4, 0x00C0, false, false );
			PatchZelda_Dsp( (u32)ptr, 0x0CF4, 0x010B, false, false );  // same orig instructions
		} break;
		case 11:	// Luigi
		{
			// BE8 - unused
			PatchZelda_Dsp( (u32)ptr, 0x0BE8, 0x00BE, false, false );
			PatchZelda_Dsp( (u32)ptr, 0x0BE8, 0x0109, false, false );  // same orig instructions
		} break;
		case 12:	// Pikmin PAL
		{
			// D2B - unused
			PatchZelda_Dsp( (u32)ptr, 0x0D2B, 0x04A8, false, true );
			PatchZelda_Dsp( (u32)ptr, 0x0D2B, 0x04F3, false, true );  // same orig instructions
		} break;
		case 13:	// IPL NTSC v1.2
		{
			// BA8 - unused
			PatchZelda_Dsp( (u32)ptr, 0x0BA8, 0x00BE, false, false );
			PatchZelda_Dsp( (u32)ptr, 0x0BA8, 0x0109, false, false );  // same orig instructions
		} break;
		case 14:	// IPL NTSC v1.0
		{
			// 938 - unused
			PatchZelda_Dsp( (u32)ptr, 0x0938, 0x00BE, false, false );
			PatchZelda_Dsp( (u32)ptr, 0x0938, 0x0109, false, false );  // same orig instructions
		} break;
		case 15:	// Metroid Prime Trilogy Wii (needed?)
		{
			PatchAX_Dsp( (u32)ptr, 0x69E, 0x753, AX_DSP_NO_DUP3, 0xA4 );
		} break;
		default:
		{
		} break;
	}
}

static bool write32A( u32 Offset, u32 Value, u32 CurrentValue, u32 ShowAssert )
{
	if( read32(Offset) != CurrentValue )
	{
		#ifdef DEBUG_PATCH
		//if( ShowAssert)
			//dbgprintf("AssertFailed: Offset:%08X is:%08X should:%08X\r\n", Offset, read32(Offset), CurrentValue );
		#endif
		return false;
	}

	write32( Offset, Value );
	return true;
}
static bool write64A( u32 Offset, u64 Value, u64 CurrentValue )
{
	if( read64(Offset) != CurrentValue )
		return false;
	write64( Offset, Value );
	return true;
}

void PatchB( u32 dst, u32 src )
{
	u32 newval = (dst - src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000000;
	*(vu32*)src = newval;	
}
void PatchBL( u32 dst, u32 src )
{
	u32 newval = (dst - src);
	newval&= 0x03FFFFFC;
	newval|= 0x48000001;
	*(vu32*)src = newval;	
}
/*
	This offset gets randomly overwritten, this workaround fixes that problem.
*/
void Patch31A0( void )
{
	POffset -= sizeof(u32) * 5;
	
	u32 i;
	for (i = 0; i < (4 * 0x04); i+=0x04)
	{
		u32 Orig = *(vu32*)(0x319C + i);
		if ((Orig & 0xF4000002) == 0x40000000)
		{
			u32 NewAddr = (Orig & 0x3FFFFC) + 0x319C - POffset;
			Orig = (Orig & 0xFC000003) | NewAddr;
#ifdef DEBUG_PATCH
			dbgprintf("[%08X] Patch31A0 %08X: 0x%08X\r\n", 0x319C + i, POffset + i, Orig);
#endif
		}
		*(vu32*)(POffset + i) = Orig;
	}

	PatchB( POffset, 0x319C );
	PatchB( 0x31AC, POffset+0x10 );
}

void PatchFuncInterface( char *dst, u32 Length )
{
	int i;

	u32 LISReg=-1;
	u32 LISOff=-1;

	for( i=0; i < Length; i+=4 )
	{
		u32 op = read32( (u32)dst + i );

		if( (op & 0xFC1FFFFF) == 0x3C00CC00 )	// lis rX, 0xCC00
		{
			LISReg = (op & 0x3E00000) >> 21;
			LISOff = (u32)dst + i;
		}

		if( (op & 0xFC000000) == 0x38000000 )	// li rX, x
		{
			u32 src = (op >> 16) & 0x1F;
			u32 dst = (op >> 21) & 0x1F;
			if ((src != LISReg) && (dst == LISReg))
			{
				LISReg = -1;
				LISOff = (u32)dst + i;
			}
		}

		if( (op & 0xFC00FF00) == 0x38006C00 ) // addi rX, rY, 0x6C00 (ai)
		{
			u32 src = (op >> 16) & 0x1F;
			if( src == LISReg )
			{
				write32( (u32)LISOff, (LISReg<<21) | 0x3C00CD00 );	// Patch to: lis rX, 0xCD00
				#ifdef DEBUG_PATCH
				dbgprintf("AI:[%08X] %08X: lis r%u, 0xCD00\r\n", (u32)LISOff, read32( (u32)LISOff), LISReg );
				#endif
				LISReg = -1;
			}
			u32 dst = (op >> 21) & 0x1F;
			if( dst == LISReg )
				LISReg = -1;
		}
		else if( (op & 0xFC00FF00) == 0x38006400 ) // addi rX, rY, 0x6400 (si)
		{
			u32 src = (op >> 16) & 0x1F;
			if( src == LISReg )
			{
				write32( (u32)LISOff, (LISReg<<21) | 0x3C00D302 );	// Patch to: lis rX, 0xD302
				dbgprintf("SI:[%08X] %08X: lis r%u, 0xD302\r\n", (u32)LISOff, read32( (u32)LISOff), LISReg );
				LISReg = -1;
			}
			u32 dst = (op >> 21) & 0x1F;
			if( dst == LISReg )
				LISReg = -1;
		}

		if(    ( (op & 0xF8000000 ) == 0x80000000 )   // lwz and lwzu
		    || ( (op & 0xF8000000 ) == 0x90000000 ) ) // stw and stwu
		{
			u32 src = (op >> 16) & 0x1F;
			u32 dst = (op >> 21) & 0x1F;
			u32 val = op & 0xFFFF;
			
			if( src == LISReg )
			{
				if( (val & 0xFF00) == 0x6C00 ) // case with 0x6CXY(rZ) (ai)
				{
					write32( (u32)LISOff, (LISReg<<21) | 0x3C00CD00 );	// Patch to: lis rX, 0xCD00
					dbgprintf("AI:[%08X] %08X: lis r%u, 0xCD00\r\n", (u32)LISOff, read32( (u32)LISOff), LISReg );
					LISReg = -1;
				}
				else if((val & 0xFF00) == 0x6400) // case with 0x64XY(rZ) (si)
				{
					write32( (u32)LISOff, (LISReg<<21) | 0x3C00D302 );	// Patch to: lis rX, 0xD302
					dbgprintf("SI:[%08X] %08X: lis r%u, 0xD302\r\n", (u32)LISOff, read32( (u32)LISOff), LISReg );
					LISReg = -1;
				}
			}

			if( dst == LISReg )
				LISReg = -1;
		}


		if( op == 0x4E800020 )	// blr
		{
			LISReg=-1;
		}
	}
}

void PatchFunc( char *ptr )
{
	u32 i	= 0;
	u32 reg=-1;

	while(1)
	{
		u32 op = read32( (u32)ptr + i );

		if( op == 0x4E800020 )	// blr
			break;

		if( (op & 0xFC00FFFF) == 0x3C00CC00 )	// lis rX, 0xCC00
		{
			reg = (op & 0x3E00000) >> 21;
			
			write32( (u32)ptr + i, (reg<<21) | 0x3C00C000 );	// Patch to: lis rX, 0xC000
			#ifdef DEBUG_PATCH
			dbgprintf("[%08X] %08X: lis r%u, 0xC000\r\n", (u32)ptr+i, read32( (u32)ptr+i), reg );
			#endif
		}
		
		if( (op & 0xFC00FFFF) == 0x3C00A800 )	// lis rX, 0xA800
		{			
			write32( (u32)ptr + i, (op & 0x3E00000) | 0x3C00A700 );		// Patch to: lis rX, 0xA700
			#ifdef DEBUG_PATCH
			dbgprintf("[%08X] %08X: lis rX, 0xA700\r\n", (u32)ptr+i, read32( (u32)ptr+i) );
			#endif
		}

		// Triforce
		if( (op & 0xFC00FFFF) == 0x380000A8 )	// li rX, 0xA8
		{			
			write32( (u32)ptr + i, (op & 0x3E00000) | 0x380000A7 );		// Patch to: li rX, 0xA7
			#ifdef DEBUG
		  dbgprintf("[%08X] %08X: li rX, 0xA7\r\n", (u32)ptr+i, read32( (u32)ptr+i) );
			#endif
		}

		if( (op & 0xFC00FFFF) == 0x38006000 )	// addi rX, rY, 0x6000
		{
			u32 src = (op >> 16) & 0x1F;
			u32 dst = (op >> 21) & 0x1F;

			if( src == reg )
			{
				write32( (u32)ptr + i, (dst<<21) | (src<<16) | 0x38002F00 );	// Patch to: addi rX, rY, 0x2F00
				#ifdef DEBUG_PATCH
				dbgprintf("[%08X] %08X: addi r%u, r%u, 0x2F00\r\n", (u32)ptr+i, read32( (u32)ptr+i), dst, src );
				#endif

			}
		}

		if( (op & 0xFC000000 ) == 0x90000000 )
		{
			u32 src = (op >> 16) & 0x1F;
			u32 dst = (op >> 21) & 0x1F;
			u32 val = op & 0xFFFF;
			
			if( src == reg )
			{
				if( (val & 0xFF00) == 0x6000 )	// case with 0x60XY(rZ)
				{
					write32( (u32)ptr + i,  (dst<<21) | (src<<16) | 0x2F00 | (val&0xFF) | 0x90000000 );	// Patch to: stw rX, 0x2FXY(rZ)
					#ifdef DEBUG_PATCH
					dbgprintf("[%08X] %08X: stw r%u, 0x%04X(r%u)\r\n", (u32)ptr+i, read32( (u32)ptr+i), dst, 0x2F00 | (val&0xFF), src );
					#endif
				}
			}			
		}

		i += 4;
	}
}
void MPattern( u8 *Data, u32 Length, FuncPattern *FunctionPattern )
{
	u32 i;

	memset32( FunctionPattern, 0, sizeof(FuncPattern) );

	for( i = 0; i < Length; i+=4 )
	{
		u32 word = *(u32*)(Data + i);
		
		if( (word & 0xFC000003) ==  0x48000001 )
			FunctionPattern->FCalls++;

		if( (word & 0xFC000003) ==  0x48000000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x40800000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x41800000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x40810000 )
			FunctionPattern->Branch++;
		if( (word & 0xFFFF0000) ==  0x41820000 )
			FunctionPattern->Branch++;
		
		if( (word & 0xFC000000) ==  0x80000000 )
			FunctionPattern->Loads++;
		if( (word & 0xFF000000) ==  0x38000000 )
			FunctionPattern->Loads++;
		if( (word & 0xFF000000) ==  0x3C000000 )
			FunctionPattern->Loads++;

		if( (word & 0xFC000000) ==  0x90000000 )
			FunctionPattern->Stores++;
		if( (word & 0xFC000000) ==  0x94000000 )
			FunctionPattern->Stores++;

		if( (word & 0xFF000000) ==  0x7C000000 )
			FunctionPattern->Moves++;

		if( word == 0x4E800020 )
			break;
	}

	FunctionPattern->Length = i;
}
int CPattern( FuncPattern *FPatA, FuncPattern *FPatB  )
{
	return ( memcmp( FPatA, FPatB, sizeof(u32) * 6 ) == 0 );
}
void SRAM_Checksum( unsigned short *buf, unsigned short *c1, unsigned short *c2) 
{ 
	u32 i; 
    *c1 = 0; *c2 = 0; 
    for (i = 0;i<4;++i) 
    { 
        *c1 += buf[0x06 + i]; 
        *c2 += (buf[0x06 + i] ^ 0xFFFF); 
    }
	//dbzprintf("New Checksum: %04X %04X\r\n", *c1, *c2 );
} 
#define PATCH_STATE_NONE  0
#define PATCH_STATE_LOAD  1
#define PATCH_STATE_PATCH 2
#define PATCH_STATE_DONE  4

u32 PatchState = PATCH_STATE_NONE;
u32 PSOHack    = 0;
u32 DOLSize	   = 0;
u32 DOLReadSize= 0;
u32 DOLMinOff  = 0;
u32 DOLMaxOff  = 0;
u32 DOLOffset  = 0;
vu32 TRIGame   = 0;

void DoPatches( char *Buffer, u32 Length, u32 DiscOffset )
{
	int i, j, k;
	u32 read;
	u32 value = 0;
	
	if( (u32)Buffer >= 0x01800000 )
	{
		if((u32)Buffer != 0x13002EE0)
			return;
	}

	if( ((u32)Buffer <= 0x31A0)  && ((u32)Buffer + Length >= 0x31A0) )
	{
		#ifdef DEBUG_PATCH
		//dbgprintf("Patch:[Patch31A0]\r\n");
		#endif
		Patch31A0();
	}

	// PSO 1&2
	if( (TITLE_ID) == 0x47504F )
	{
		switch( DiscOffset )
		{
			case 0x56B8E7E0:	// AppSwitcher	[EUR]
			case 0x56C49600:	// [USA] v1.1
			case 0x56C4C980:	// [USA] v1.0
			{
				PatchState	= PATCH_STATE_PATCH;
				DOLSize		= Length;
				DOLOffset	= (u32)Buffer;
				#ifdef DEBUG_PATCH
				dbgprintf("Patch: PSO 1&2 loading AppSwitcher:0x%p %u\r\n", Buffer, Length );
				#endif

			} break;
			case 0x5668FE20:	// psov3.dol [EUR]
			case 0x56750660:	// [USA] v1.1
			case 0x56753EC0:	// [USA] v1.0
			{
				#ifdef DEBUG_PATCH
				dbgprintf("Patch: PSO 1&2 loading psov3.dol:0x%p %u\r\n", Buffer, Length );
				#endif

				PSOHack = 1;
			} break;
		}
	}

	if( (PatchState & 0x3) == PATCH_STATE_NONE )
	{
		if( Length == 0x100 || PSOHack )
		{
			if( read32( (u32)Buffer ) == 0x100 )
			{
				//quickly calc the size
				DOLSize = sizeof(dolhdr);
				dolhdr *dol = (dolhdr*)Buffer;

				for( i=0; i < 7; ++i )
					DOLSize += dol->sizeText[i];
				for( i=0; i < 11; ++i )
					DOLSize += dol->sizeData[i];
						
				DOLReadSize = Length;

				DOLMinOff=0x81800000;
				DOLMaxOff=0;

				for( i=0; i < 7; ++i )
				{
					if( dol->addressText[i] == 0 )
						continue;

					if( DOLMinOff > dol->addressText[i])
						DOLMinOff = dol->addressText[i];

					if( DOLMaxOff < dol->addressText[i] + dol->sizeText[i] )
						DOLMaxOff = dol->addressText[i] + dol->sizeText[i];
				}

				for( i=0; i < 11; ++i )
				{
					if( dol->addressData[i] == 0 )
						continue;

					if( DOLMinOff > dol->addressData[i])
						DOLMinOff = dol->addressData[i];

					if( DOLMaxOff < dol->addressData[i] + dol->sizeData[i] )
						DOLMaxOff = dol->addressData[i] + dol->sizeData[i];
				}

				DOLMinOff -= 0x80000000;
				DOLMaxOff -= 0x80000000;	

				if( PSOHack )
				{
					DOLMinOff = (u32)Buffer;
					DOLMaxOff = (u32)Buffer + DOLSize;
				}
#ifdef DEBUG_DI
				dbgprintf("DIP:DOLSize:%d DOLMinOff:0x%08X DOLMaxOff:0x%08X\r\n", DOLSize, DOLMinOff, DOLMaxOff );
#endif

				PatchState |= PATCH_STATE_LOAD;
			}
						
			PSOHack = 0;
		} else if( read32( (u32)Buffer ) == 0x7F454C46 && ((Elf32_Ehdr*)Buffer)->e_phnum )
		{
#ifdef DEBUG_DI
			dbgprintf("DIP:Game is loading an ELF 0x%08x (%u)\r\n", DiscOffset, Length );
#endif
			DOLMinOff = (u32)Buffer;
			DOLOffset = DiscOffset;
			DOLSize	  = 0;

			if( Length > 0x1000 )
				DOLReadSize = Length;
			else
				DOLReadSize = 0;

			Elf32_Ehdr *ehdr = (Elf32_Ehdr*)Buffer;
#ifdef DEBUG_DI
			dbgprintf("DIP:ELF Programheader Entries:%u\r\n", ehdr->e_phnum );	
#endif
			for( i=0; i < ehdr->e_phnum; ++i )
			{
				Elf32_Phdr phdr;
									
				f_lseek( &GameFile, DOLOffset + ehdr->e_phoff + i * sizeof(Elf32_Phdr) );
				f_read( &GameFile, &phdr, sizeof(Elf32_Phdr), &read );				

				DOLSize += (phdr.p_filesz+31) & (~31);	// align by 32byte
			}
			
#ifdef DEBUG_DI
			dbgprintf("DIP:ELF size:%u\r\n", DOLSize );
#endif

			PatchState |= PATCH_STATE_LOAD;
		}
	} 
	else if ( PatchState & PATCH_STATE_LOAD )
	{
		if( DOLReadSize == 0 )
			DOLMinOff = (u32)Buffer;

		DOLReadSize += Length;
		#ifdef DEBUG_PATCH
		dbgprintf("DIP:DOLsize:%d DOL read:%d\r\n", DOLSize, DOLReadSize );
		#endif
		if( DOLReadSize >= DOLSize )
		{
			DOLMaxOff = DOLMinOff + DOLSize;
			PatchState |= PATCH_STATE_PATCH;
			PatchState &= ~PATCH_STATE_LOAD;
		}
		if ((PatchState & PATCH_STATE_DONE) && (DOLMaxOff == DOLMinOff))
			PatchState = PATCH_STATE_DONE;
	}

	if (!(PatchState & PATCH_STATE_PATCH))
		return;

	sync_after_write(Buffer, Length);

	Buffer = (char*)DOLMinOff;
	Length = DOLMaxOff - DOLMinOff;

#ifdef DEBUG_PATCH
	dbgprintf("Patch: Offset:0x%08X EOffset:0x%08X Length:%08X\r\n", Buffer, DOLMaxOff, Length );
	dbgprintf("Patch:  Game ID = %x\r\n", read32(0));
#endif

	sync_before_read(Buffer, Length);

	// HACK: PokemonXD and Pokemon Colosseum low memory clear patch
	if(( TITLE_ID == 0x475858 ) || ( TITLE_ID == 0x474336 ))
	{
		// patch out initial memset(0x1800, 0, 0x1800)
		if( (read32(0) & 0xFF) == 0x4A )	// JAP
			write32( 0x560C, 0x60000000 );
		else								// EUR/USA
			write32( 0x5614, 0x60000000 );

		// patch memset to jump to test function
		write32(0x00005498, 0x4BFFABF0);

		// patch in test < 0x3000 function
		write32(0x00000088, 0x3D008000);
		write32(0x0000008C, 0x61083000);
		write32(0x00000090, 0x7C044000);
		write32(0x00000094, 0x4180542C);
		write32(0x00000098, 0x90E40004);
		write32(0x0000009C, 0x48005400);

		// skips __start init of debugger mem
		write32(0x00003194, 0x48000028);
	}

	if (read32(0x02856EC) == 0x386000A8)
	{
		dbgprintf("TRI:Mario kart GP 2\r\n");
		TRIGame = 1;
		SystemRegion = REGION_JAPAN;

		//Skip device test
		write32( 0x002E340, 0x60000000 );
		write32( 0x002E34C, 0x60000000 );

		//Disable wheel
		write32( 0x007909C, 0x98650022 );

		//Disable CARD
		write32( 0x0073BF4, 0x98650023 );
		write32( 0x0073C10, 0x98650023 );

		//Disable cam
		write32( 0x0073BD8, 0x98650025 );

		//VS wait patch 
		write32( 0x0084FC4, 0x4800000C );
		write32( 0x0085000, 0x60000000 );

		//Game vol
		write32( 0x00790E8, 0x39000009 );
		//Attract vol
		write32( 0x00790F4, 0x38C0000C );

		//Disable Commentary (sets volume to 0 )
		write32( 0x001B6510, 0x38800000 );
		
		//Patches the analog input count
		write32( 0x000392F4, 0x38000003 );

		//if( ConfigGetConfig(NIN_CFG_HID) )
		//{
		//	POffset -= sizeof(PADReadTriHID);
		//	memcpy( (void*)POffset, PADReadTriHID, sizeof(PADReadTriHID) );
		//	PatchB( POffset, 0x0038EF0 );

		//	POffset -= sizeof(PADReadSteerTriHID);
		//	memcpy( (void*)POffset, PADReadSteerTriHID, sizeof(PADReadSteerTriHID) );
		//	PatchBL( POffset, 0x00392DC );
		//
		//} else {

			POffset -= sizeof(PADReadB);
			memcpy( (void*)POffset, PADReadB, sizeof(PADReadB) );
			PatchB( POffset, 0x0038EF0 );

			POffset -= sizeof(PADReadSteer);
			memcpy( (void*)POffset, PADReadSteer, sizeof(PADReadSteer) );
			PatchBL( POffset, 0x00392DC );

		//}

		//memcpy( (void*)0x002CE3C, OSReportDM, sizeof(OSReportDM) );
		//memcpy( (void*)0x002CE8C, OSReportDM, sizeof(OSReportDM) );
		//write32( 0x002CEF8, 0x60000000 );
	}
	
	if( read32( 0x0210C08 ) == 0x386000A8 )	// Virtua Striker 4 Ver 2006 (EXPORT)
	{ 
		dbgprintf("TRI:Virtua Striker 4 Ver 2006 (EXPORT)\n");
		TRIGame = 2;
		SystemRegion = REGION_USA;

		memcpy( (void*)0x0208314, PADReadSteerVSSimple, sizeof(PADReadSteerVSSimple) );	
		memcpy( (void*)0x0208968, PADReadVSSimple, sizeof(PADReadVSSimple) );

		//memcpy( (void*)0x001C2B80, OSReportDM, sizeof(OSReportDM) );
	}

	if( read32(0x01851C4) == 0x386000A8 )	// FZero AX
	{
		dbgprintf("TRI:FZero AX\n");
		TRIGame = 3;
		SystemRegion = REGION_JAPAN;
		
		//Reset loop
		write32( 0x01B5410, 0x60000000 );
		//DBGRead fix
		write32( 0x01BEF38, 0x60000000 );

		//Disable CARD
		write32( 0x017B2BC, 0x38C00000 );

		//Motor init patch
		write32( 0x0175710, 0x60000000 );
		write32( 0x0175714, 0x60000000 );
		write32( 0x01756AC, 0x60000000 );

		//Goto Test menu
		//write32( 0x00DF3D0, 0x60000000 );

		//English 
		write32( 0x000DF698, 0x38000000 );

		//GXProg patch
		memcpy( (void*)0x0302AC0, (void*)0x0302A84, 0x3C );
		memcpy( (void*)0x021D3EC, (void*)0x0302A84, 0x3C );

		//Unkreport
		PatchB( 0x01882C0, 0x0191B54 );
		PatchB( 0x01882C0, 0x01C53CC );
		PatchB( 0x01882C0, 0x01CC684 );

		//memcpy( (void*)0x01CAACC, patch_fwrite_GC, sizeof(patch_fwrite_GC) );
		//memcpy( (void*)0x01882C0, OSReportDM, sizeof(OSReportDM) );
	}
	if( read32(0x023D240) == 0x386000A8 )	// Mario kart GP (TRI)
	{
		dbgprintf("TRI:Mario kart GP\n");
		TRIGame = 4;
		SystemRegion = REGION_JAPAN;
		
		//Reset skip
		write32( 0x024F95C, 0x60000000 );

		//Unlimited CARD uses		
		write32( 0x01F5C44, 0x60000000 );	
		
		//Disable cam
		write32( 0x00790A0, 0x98650025 );
		
		//Disable CARD
		write32( 0x00790B4, 0x98650023 );
		write32( 0x00790CC, 0x98650023 );

		//Disable wheel/handle
		write32( 0x007909C, 0x98650022 );
			
		//VS wait
		write32( 0x00BE10C, 0x4800002C );

		//cam loop
		write32( 0x009F1E0, 0x60000000 );
		
		//Skip device test
		write32( 0x0031BF0, 0x60000000 );
		write32( 0x0031BFC, 0x60000000 );

		//GXProg patch
		memcpy( (void*)0x036369C, (void*)0x040EB88, 0x3C );

		POffset -= sizeof(PADReadB);
		memcpy( (void*)POffset, PADReadB, sizeof(PADReadB) );
		PatchB( POffset, 0x003C6EC );

		POffset -= sizeof(PADReadSteer);
		memcpy( (void*)POffset, PADReadSteer, sizeof(PADReadSteer) );
		PatchBL( POffset, 0x003CAD4 );

		//some report check skip
		//write32( 0x00307CC, 0x60000000 );

		//memcpy( (void*)0x00330BC, OSReportDM, sizeof(OSReportDM) );
		//memcpy( (void*)0x0030710, OSReportDM, sizeof(OSReportDM) );
		//memcpy( (void*)0x0030760, OSReportDM, sizeof(OSReportDM) );
		//memcpy( (void*)0x020CBCC, OSReportDM, sizeof(OSReportDM) );	// UNkReport
		//memcpy( (void*)0x02281B8, OSReportDM, sizeof(OSReportDM) );	// UNkReport4
	}

	PatchFuncInterface( Buffer, Length );

	CardLowestOff = 0;
	
	u32 PatchCount = 64|128|1;
#ifdef CHEATS
	if( IsWiiU )
	{
		if( ConfigGetConfig(NIN_CFG_CHEATS) )
			PatchCount &= ~64;

	} else {

		if( ConfigGetConfig(NIN_CFG_DEBUGGER|NIN_CFG_CHEATS) )
			PatchCount &= ~64;
	}
#endif
	if( ConfigGetConfig(NIN_CFG_FORCE_PROG) || (ConfigGetVideoMode() & NIN_VID_FORCE) )
		PatchCount &= ~128;

	u8 *SHA1i = (u8*)malloca( 0x60, 0x40 );
	u8 *hash  = (u8*)malloca( 0x14, 0x40 );

	for( i=0; i < Length; i+=4 )
	{
		if( (PatchCount & 2048) == 0  )	// __OSDispatchInterrupt
		{
			if( read32((u32)Buffer + i + 0 ) == 0x3C60CC00 &&
				read32((u32)Buffer + i + 4 ) == 0x83E33000  
				)
			{
				#ifdef DEBUG_PATCH
				dbgprintf("Patch:[__OSDispatchInterrupt] 0x%08X\r\n", (u32)Buffer + i );
				#endif
				POffset -= sizeof(FakeInterrupt);
				memcpy( (void*)(POffset), FakeInterrupt, sizeof(FakeInterrupt) );
				PatchBL( POffset, (u32)Buffer + i + 4 );
				
				// EXI Device 0 Control Register
				write32A( (u32)Buffer+i+0x114, 0x3C60C000, 0x3C60CC00, 1 );
				write32A( (u32)Buffer+i+0x118, 0x80830010, 0x80836800, 1 );

				// EXI Device 2 Control Register (Trifroce)
				write32A( (u32)Buffer+i+0x188, 0x3C60C000, 0x3C60CC00, 1 );
				write32A( (u32)Buffer+i+0x18C, 0x38630018, 0x38636800, 1 );
				write32A( (u32)Buffer+i+0x190, 0x80830000, 0x80830028, 1 );

				PatchCount |= 2048;
			}
		}

		if( (PatchCount & 1) == 0 )	// 803463EC 8034639C
		if( (read32( (u32)Buffer + i )		& 0xFC00FFFF)	== 0x5400077A &&
			(read32( (u32)Buffer + i + 4 )	& 0xFC00FFFF)	== 0x28000000 &&
			 read32( (u32)Buffer + i + 8 )								== 0x41820008 &&
			(read32( (u32)Buffer + i +12 )	& 0xFC00FFFF)	== 0x64002000
			)  
		{
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Found [__OSDispatchInterrupt]: 0x%08X 0x%08X\r\n", (u32)Buffer + i + 0, (u32)Buffer + i + 0x1A8 );
			#endif
			write32( (u32)Buffer + i + 0x1A8,	(read32( (u32)Buffer + i + 0x1A8 )	& 0xFFFF0000) | 0x0463 );
			
			PatchCount |= 1;
		}

		if( (PatchCount & 2) == 0 )
		if( read32( (u32)Buffer + i )		== 0x5480056A &&
			read32( (u32)Buffer + i + 4 )	== 0x28000000 &&
			read32( (u32)Buffer + i + 8 )	== 0x40820008 &&
			(read32( (u32)Buffer + i +12 )&0xFC00FFFF) == 0x60000004
			) 
		{
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Found [SetInterruptMask]: 0x%08X\r\n", (u32)Buffer + i + 12 );
			#endif

			write32( (u32)Buffer + i + 12, (read32( (u32)Buffer + i + 12 ) & 0xFFFF0000) | 0x4004 );

			PatchCount |= 2;
		}

		if( (PatchCount & 4) == 0 )
		if( (read32( (u32)Buffer + i + 0 ) & 0xFFFF) == 0x6000 &&
			(read32( (u32)Buffer + i + 4 ) & 0xFFFF) == 0x002A &&
			(read32( (u32)Buffer + i + 8 ) & 0xFFFF) == 0x0054 
			) 
		{
			u32 Offset = (u32)Buffer + i - 8;
			u32 HwOffset = Offset;
			if ((read32(Offset + 4) & 0xFFFF) == 0xCC00)	// Loader
				HwOffset = Offset + 4;
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Found [__DVDIntrruptHandler]: 0x%08X (0x%08X)\r\n", Offset, HwOffset );
			#endif
			POffset -= sizeof(DCInvalidateRange);
			memcpy( (void*)(POffset), DCInvalidateRange, sizeof(DCInvalidateRange) );
			PatchBL(POffset, (u32)HwOffset);
							
			Offset += 92;
						
			if( (read32(Offset-8) & 0xFFFF) == 0xCC00 )	// Loader
			{
				Offset -= 8;
			}
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:[__DVDInterruptHandler] 0x%08X\r\n", Offset );	
			#endif
			
			value = *(vu32*)Offset;
			value&= 0xFFFF0000;
			value|= 0x0000CD80;
			*(vu32*)Offset = value;

			PatchCount |= 4;
		}
		
		if( (PatchCount & 8) == 0 )
		{
			if( (read32( (u32)Buffer + i + 0 ) & 0xFFFF) == 0xCC00 &&			// Game
				(read32( (u32)Buffer + i + 4 ) & 0xFFFF) == 0x6000 &&
				(read32( (u32)Buffer + i +12 ) & 0xFFFF) == 0x001C 
				) 
			{
				u32 Offset = (u32)Buffer + i;
				#ifdef DEBUG_PATCH
				dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset );
				#endif
				value = *(vu32*)(Offset+8);
				value&= 0x03E00000;
				value|= 0x38000000;
				*(vu32*)(Offset+8) = value;
				
				// done below (+0x4C)
				//if( TRIGame )
				//{
				//	write32A( Offset+0x218,	0x3C60C000, 0x3C60CC00, 0 );
				//	write32A( Offset+0x21C,	0x80032F50, 0x80036020, 0 );
				//
				//	write32A( Offset+0x228,	0x3C60C000, 0x3C60CC00, 0 );
				//	write32A( Offset+0x22C,	0x38632F30, 0x38636000, 0 );

				//	write32A( Offset+0x284,	0x3C60C000, 0x3C60CC00, 0 );
				//	write32A( Offset+0x288,	0x80032F50, 0x80036020, 0 );
				//}

				u32 SearchIndex = 0;
				for (SearchIndex = 0; SearchIndex < 2; SearchIndex++)
				{
					if (write32A(Offset + 0x1CC, 0x3C60C000, 0x3C60CC00, 0))
						dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset + 0x1CC);
					if (write32A(Offset + 0x1D0, 0x80032f50, 0x80036020, 0))
						dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset + 0x1D0);

					if (write32A(Offset + 0x1DC, 0x3C60C000, 0x3C60CC00, 0))
						dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset + 0x1DC);
					if (write32A(Offset + 0x1E0, 0x38632f30, 0x38636000, 0))
						dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset + 0x1E0);

					if (write32A(Offset + 0x238, 0x3C60c000, 0x3C60CC00, 0))
						dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset + 0x238);
					if (write32A(Offset + 0x23C, 0x80032f50, 0x80036020, 0))
						dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset + 0x23C);
					Offset += 0x4C;
				}

				PatchCount |= 8;

			} else if(	(read32( (u32)Buffer + i + 0 ) & 0xFFFF) == 0xCC00 && // Loader
						(read32( (u32)Buffer + i + 4 ) & 0xFFFF) == 0x6018 &&
						(read32( (u32)Buffer + i +12 ) & 0xFFFF) == 0x001C 
				)
			{
				u32 Offset = (u32)Buffer + i;
				#ifdef DEBUG_PATCH			
				dbgprintf("Patch:[cbForStateBusy] 0x%08X\r\n", Offset );
				#endif
				write32( Offset, 0x3C60C000 );
				write32( Offset+4, 0x80832F48 );
		
				PatchCount |= 8;
				
			}
		}

		if( (PatchCount & 16) == 0 )
		{
			if( read32( (u32)Buffer + i + 0 ) == 0x3C608000 )
			{
				if( ((read32( (u32)Buffer + i + 4 ) & 0xFC1FFFFF ) == 0x800300CC) && ((read32( (u32)Buffer + i + 8 ) >> 24) == 0x54 ) )
				{
					#ifdef DEBUG_PATCH
					dbgprintf( "Patch:[VIConfgiure] 0x%08X\r\n", (u32)(Buffer+i) );
					#endif
					write32( (u32)Buffer + i + 4, 0x5400F0BE | ((read32( (u32)Buffer + i + 4 ) & 0x3E00000) >> 5	) );
					PatchCount |= 16;
				}
			}
		}

		if( (PatchCount & 32) == 0 )
		{
			if( read32( (u32)Buffer + i + 0 ) == 0xA063501A )
			{
				if( read32( (u32)Buffer + i + 0x10 ) == 0x80010014  )
				{
					write32( (u32)Buffer + i + 0x10, 0x3800009C );
					#ifdef DEBUG_PATCH
					dbgprintf( "Patch:[ARInit] 0x%08X\r\n", (u32)(Buffer+i) );
					#endif
				}
//				else
//				{
					#ifdef DEBUG_PATCH
//					dbgprintf("Patch skipped:[ARInit] 0x%08X\r\n", (u32)(Buffer + i));
					#endif
//				}

				PatchCount |= 32;
			}
		}
#ifdef CHEATS
		if( (PatchCount & 64) == 0 )
		{
			//OSSleepThread(Pattern 1)
			if( read32((u32)Buffer + i + 0 ) == 0x3C808000 &&
				( read32((u32)Buffer + i + 4 ) == 0x3C808000 || read32((u32)Buffer + i + 4 ) == 0x808400E4 ) &&
				( read32((u32)Buffer + i + 8 ) == 0x38000004 || read32((u32)Buffer + i + 8 ) == 0x808400E4 )
			)
			{
				int j = 12;

				while( *(vu32*)(Buffer+i+j) != 0x4E800020 )
					j+=4;


				u32 DebuggerHook = (u32)Buffer + i + j;
				#ifdef DEBUG_PATCH
				dbgprintf("Patch:[Hook:OSSleepThread] at 0x%08X\r\n", DebuggerHook | 0x80000000 );
				#endif

				u32 DBGSize;

				FIL fs;
				if( f_open( &fs, "/sneek/kenobiwii.bin", FA_OPEN_EXISTING|FA_READ ) != FR_OK )
				{
					#ifdef DEBUG_PATCH
					dbgprintf( "Patch:Could not open:\"%s\", this file is required for debugging!\r\n", "/sneek/kenobiwii.bin" );
					#endif

				} else {
										
					if( fs.fsize != 0 )
					{
						DBGSize = fs.fsize;

						//Read file to memory
						s32 ret = f_read( &fs, (void*)0x1800, fs.fsize, &read );
						if( ret != FR_OK )
						{
							#ifdef DEBUG_PATCH
							dbgprintf( "Patch:Could not read:\"%s\":%d\r\n", "/sneek/kenobiwii.bin", ret );
							#endif
							f_close( &fs );

						} else {

							f_close( &fs );

							if( IsWiiU )
							{
								*(vu32*)(P2C(*(vu32*)0x1808)) = 0;

							} else {

								if( ConfigGetConfig(NIN_CFG_DEBUGWAIT) )
									*(vu32*)(P2C(*(vu32*)0x1808)) = 1;
								else
									*(vu32*)(P2C(*(vu32*)0x1808)) = 0;
							}

							memcpy( (void *)0x1800, (void*)0, 6 );				

							u32 newval = 0x18A8 - DebuggerHook;
								newval&= 0x03FFFFFC;
								newval|= 0x48000000;

							*(vu32*)(DebuggerHook) = newval;

							if( ConfigGetConfig( NIN_CFG_CHEATS ) )
							{
								char *path = (char*)malloc( 128 );

								if( ConfigGetConfig(NIN_CFG_CHEAT_PATH) )
								{
									_sprintf( path, "%s", ConfigGetCheatPath() );
								} else {
									_sprintf( path, "/games/%.6s/%.6s.gct", (char*)0x1800, (char*)0x1800 );
								}

								FIL CodeFD;
								u32 read;

								if( f_open( &CodeFD, path, FA_OPEN_EXISTING|FA_READ ) == FR_OK )
								{
									if( CodeFD.fsize >= 0x2E60 - (0x1800+DBGSize-8) )
									{
										#ifdef DEBUG_PATCH
										dbgprintf("Patch:Cheatfile is too large, it must not be large than %d bytes!\r\n", 0x2E60 - (0x1800+DBGSize-8));
										#endif
									} else {
										if( f_read( &CodeFD, (void*)(0x1800+DBGSize-8), CodeFD.fsize, &read ) == FR_OK )
										{
											#ifdef DEBUG_PATCH
											dbgprintf("Patch:Copied cheat file to memory\r\n");
											#endif
											write32( 0x1804, 1 );
										} else {
											#ifdef DEBUG_PATCH
											dbgprintf("Patch:Failed to read cheat file:\"%s\"\r\n", path );
											#endif
										}
									}

									f_close( &CodeFD );

								} else {
									#ifdef DEBUG_PATCH
									dbgprintf("Patch:Failed to open/find cheat file:\"%s\"\r\n", path );
									#endif
								}

								free(path);
							}
						}
					}
				}

				PatchCount |= 64;
			}
		}
#endif
		if( (PatchCount & 128) == 0  )
		{
			u32 k=0;
			for( j=0; j <= GXNtsc480Int; ++j )	// Don't replace the Prog struct
			{
				if( memcmp( Buffer+i, GXMObjects[j], 0x3C ) == 0 )
				{
					#ifdef DEBUG_PATCH
					dbgprintf("Patch:Found GX pattern %u at %08X\r\n", j, (u32)Buffer+i );
					#endif
					if( ConfigGetConfig(NIN_CFG_FORCE_PROG) )
					{
						switch( ConfigGetVideoMode() & NIN_VID_FORCE_MASK )
						{
							case NIN_VID_FORCE_NTSC:
							{
								memcpy( Buffer+i, GXMObjects[GXNtsc480Prog], 0x3C );
							} break;
							case NIN_VID_FORCE_PAL50:
							{
								memcpy( Buffer+i, GXMObjects[GXPal528Prog], 0x3C );
							} break;
							case NIN_VID_FORCE_PAL60:
							{
								memcpy( Buffer+i, GXMObjects[GXEurgb60Hz480Prog], 0x3C );
							} break;
						}

					} else {

						if( ConfigGetVideoMode() & NIN_VID_FORCE )
						{
							switch( ConfigGetVideoMode() & NIN_VID_FORCE_MASK )
							{
								case NIN_VID_FORCE_NTSC:
								{
									memcpy( Buffer+i, GXMObjects[GXNtsc480IntDf], 0x3C );
								} break;
								case NIN_VID_FORCE_PAL50:
								{
									memcpy( Buffer+i, GXMObjects[GXPal528IntDf], 0x3C );
								} break;
								case NIN_VID_FORCE_PAL60:
								{
									memcpy( Buffer+i, GXMObjects[GXEurgb60Hz480IntDf], 0x3C );
								} break;
								case NIN_VID_FORCE_MPAL:
								{
									memcpy( Buffer+i, GXMObjects[GXMpal480IntDf], 0x3C );
								} break;
							}
						
						} else {

							switch( Region )
							{
								default:
								case 1:
								case 0:
								{
									memcpy( Buffer+i, GXMObjects[GXNtsc480IntDf], 0x3C );
								} break;
								case 2:
								{
									memcpy( Buffer+i, GXMObjects[GXEurgb60Hz480IntDf], 0x3C );
								} break;
							}
						}
					}
					k++;
				}
			}

			if( k > 4 )
			{
				PatchCount |= 128;
			}	
		}

		if( (PatchCount & 256) == 0 )	//DVDLowStopMotor
		{
			if( read32( (u32)Buffer + i ) == 0x3C00E300 )
			{
				u32 Offset = (u32)Buffer + i;
				#ifdef DEBUG_PATCH
				dbgprintf("Patch:[DVDLowStopMotor] 0x%08X\r\n", Offset );
				#endif
				
				value = *(vu32*)(Offset-12);
				value&= 0xFFFF0000;
				value|= 0x0000C000;
				*(vu32*)(Offset-12) = value;

				value = *(vu32*)(Offset-8);
				value&= 0xFFFF0000;
				value|= 0x00002F00;
				*(vu32*)(Offset-8) = value;

				value = *(vu32*)(Offset+4);
				value&= 0xFFFF0000;
				value|= 0x00002F08;
				*(vu32*)(Offset+4) = value;		

				PatchCount |= 256;
			}
		}

		if( (PatchCount & 512) == 0 )	//DVDLowReadDiskID
		{
			if( (read32( (u32)Buffer + i ) & 0xFC00FFFF ) == 0x3C00A800 && (read32( (u32)Buffer + i + 4 ) & 0xFC00FFFF ) == 0x38000040 )
			{
				u32 Offset = (u32)Buffer + i;
				#ifdef DEBUG_PATCH
				dbgprintf("Patch:[DVDLowReadDiskID] 0x%08X\r\n", Offset );
				#endif

				value = *(vu32*)(Offset);
				value&= 0xFFFF0000;
				value|= 0x0000A700;
				*(vu32*)(Offset) = value;

				value = *(vu32*)(Offset+0x20);
				value&= 0xFFFF0000;
				value|= 0x0000C000;
				*(vu32*)(Offset+0x20) = value;
				
				value = *(vu32*)(Offset+0x24);
				value&= 0xFFFF0000;
				value|= 0x00002F00;
				*(vu32*)(Offset+0x24) = value;

				value = *(vu32*)(Offset+0x2C);
				value&= 0xFFFF0000;
				value|= 0x00002F08;
				*(vu32*)(Offset+0x2C) = value;

				PatchCount |= 512;				
			}
		}

		if( (PatchCount & 1024) == 0  )	// DSP patch
		{
			u32 l,Known=-1;

			u32 UseLast = 0;
			bool PatternMatch = false;
			// for each pattern/length combo
			for( l=0; l < sizeof(DspMatches) / sizeof(DspMatch); ++l )
			{
				if (UseLast == 0)
					PatternMatch = ( memcmp( (void*)(Buffer+i), DSPPattern[DspMatches[l].Pattern], 0x10 ) == 0 );
				if (PatternMatch)
				{
					#ifdef DEBUG_PATCH
					dbgprintf("Patch:[DSPPattern] 0x%08X v%u\r\n", (u32)Buffer + i, l );
					#endif

					if (DspMatches[l].Length-UseLast > 0)
						memcpy( (void*)0x12E80000+UseLast, (unsigned char*)Buffer+i+UseLast, DspMatches[l].Length-UseLast );
					
					if (DspMatches[l].Length != UseLast)
					{
						sha1( SHA1i, NULL, 0, 0, NULL );
						sha1( SHA1i, (void*)0x12E80000, DspMatches[l].Length, 2, hash );
					}

					if( memcmp( DSPHashes[DspMatches[l].SHA1], hash, 0x14 ) == 0 )
					{
						Known = DspMatches[l].SHA1;
#ifdef DEBUG_DSP
						dbgprintf("DSP before Patch\r\n");
						hexdump((void*)(Buffer + i), DspMatches[l].Length);
#endif
						DoDSPPatch(Buffer + i, Known);
#ifdef DEBUG_DSP
						dbgprintf("DSP after Patch\r\n");
						hexdump((void*)(Buffer + i), DspMatches[l].Length);
#endif

						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[DSPROM] DSPv%u\r\n", Known );
						#endif
						PatchCount |= 1024;
						break;
					}
				}
				if ( (l < sizeof(DspMatches) / sizeof(DspMatch)) && (DspMatches[l].Pattern == DspMatches[l+1].Pattern) )
					UseLast = DspMatches[l].Length;
				else
					UseLast = 0;
			}
		}

		if( PatchCount == (1|2|4|8|16|32|64|128|256|512|1024|2048) )
			break;
	}
	free(hash);
	free(SHA1i);
	#ifdef DEBUG_PATCH
	dbgprintf("PatchCount:%08X\r\n", PatchCount );

	if( (PatchCount & 1024) == 0  )	
	{
		dbgprintf("Patch:Unknown DSP ROM\r\n");
	}
	#endif

	if( ((PatchCount & (1|2|4|8|2048)) != (1|2|4|8|2048)) )
	{
		#ifdef DEBUG_PATCH
		dbgprintf("Patch:Could not apply all required patches!\r\n");
		#endif
		Shutdown();
	}
	for (j = 0; j < sizeof(FPatterns) / sizeof(FuncPattern); ++j)
		FPatterns[j].Found = 0;

	for( i=0; i < Length; i+=4 )
	{
		if( *(u32*)(Buffer + i) != 0x4E800020 )
			continue;

		i+=4;

		FuncPattern fp;
		MPattern( (u8*)(Buffer+i), Length, &fp );
		//if ((((u32)Buffer + i) & 0x7FFFFFFF) == 0x00000000) //(FuncPrint)
		//	dbgprintf("FuncPattern: 0x%X, %d, %d, %d, %d, %d\r\n", fp.Length, fp.Loads, fp.Stores, fp.FCalls, fp.Branch, fp.Moves);

		for( j=0; j < sizeof(FPatterns)/sizeof(FuncPattern); ++j )
		{
			if( FPatterns[j].Found ) //Skip already found patches
				continue;
							
			if( CPattern( &fp, &(FPatterns[j]) ) )
			{
				u32 FOffset = (u32)Buffer + i;

				FPatterns[j].Found = FOffset;
				#ifdef DEBUG_PATCH
				dbgprintf("Patch:Found [%s]: 0x%08X\r\n", FPatterns[j].Name, FOffset );
				#endif

				switch( FPatterns[j].PatchLength )
				{
					case FCODE_PatchFunc:	// DVDLowRead
					{
						PatchFunc( (char*)FOffset );
					} break;
					case FCODE_Return:	// __AI_set_stream_sample_rate
					{
						write32( FOffset, 0x4E800020 );
					} break;
					case FCODE_AIResetStreamSampleCount:	// Audiostreaming hack
					{
						switch( TITLE_ID )
						{
							case 0x474544:	// Eternal Darkness
							break;
							default:
							{
								write32( FOffset + 0xB4, 0x60000000 );
								write32( FOffset + 0xC0, 0x60000000 );
							} break;
						}
					} break;
					case FCODE_GXInitTlutObj:	// GXInitTlutObj
					{
						if( read32( FOffset+0x34 ) == 0x5400023E )
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[GXInitTlutObj] 0x%08X\r\n", FOffset );
							#endif
							write32( FOffset+0x34, 0x5400033E );

						} else if( read32( FOffset+0x28 ) == 0x5004C00E )
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[GXInitTlutObj] 0x%08X\r\n", FOffset );
							#endif
							write32( FOffset+0x28, 0x5004C016 );
						} else {
							FPatterns[j].Found = 0; // False hit
						}
					} break;
					case FCODE___ARChecksize_A:	// __ARChecksize A
					case FCODE___ARChecksize_B:	// __ARChecksize B
					case FCODE___ARChecksize_C:	// __ARChecksize C
					{
						if (FPatterns[j].PatchLength == FCODE___ARChecksize_B)
						{
							u32 PatchVal = read32(FOffset + 0x370);
							if ((PatchVal & 0xFFE0FFFF) == 0x40801464) // bne +0x1464
							{
								write32(FOffset + 0x370, 0x48001464); // b +0x1464
								#ifdef DEBUG_PATCH
								dbgprintf("Patch:[__ARChecksize B] 0x%08X\r\n", FOffset + 0x370 );
								#endif
								break;
							}
						}
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[__ARChecksize] 0x%08X\r\n", FOffset );
						#endif
						u32 EndOffset = FOffset + FPatterns[j].Length;
						u32 Register = (*(vu32*)(EndOffset-24) >> 21) & 0x1F;

						*(vu32*)(FOffset)	= 0x3C000100 | ( Register << 21 );
						*(vu32*)(FOffset+4)	= *(vu32*)(EndOffset-28);
						*(vu32*)(FOffset+8) = *(vu32*)(EndOffset-24);
						*(vu32*)(FOffset+12)= *(vu32*)(EndOffset-20);
						*(vu32*)(FOffset+16)= 0x4E800020;

					} break;
// Widescreen hack by Extrems
					case FCODE_C_MTXPerspective:	//	C_MTXPerspective
					{
						if( !ConfigGetConfig(NIN_CFG_FORCE_WIDE) )
							break;
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[C_MTXPerspective] 0x%08X\r\n", FOffset );
						#endif
						*(volatile float *)0x50 = 0.5625f;

						memcpy((void*)(FOffset+ 28),(void*)(FOffset+ 36),44);
						memcpy((void*)(FOffset+188),(void*)(FOffset+192),16);

						*(unsigned int*)(FOffset+52) = 0x48000001 | ((*(unsigned int*)(FOffset+52) & 0x3FFFFFC) + 8);
						*(unsigned int*)(FOffset+72) = 0x3C600000 | (0x80000050 >> 16);		// lis		3, 0x8180
						*(unsigned int*)(FOffset+76) = 0xC0230000 | (0x80000050 & 0xFFFF);	// lfs		1, -0x1C (3)
						*(unsigned int*)(FOffset+80) = 0xEC240072;							// fmuls	1, 4, 1

					} break;
					case FCODE_C_MTXLightPerspective:	//	C_MTXLightPerspective
					{
						if( !ConfigGetConfig(NIN_CFG_FORCE_WIDE) )
							break;
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[C_MTXLightPerspective] 0x%08X\r\n", FOffset );
						#endif
						*(volatile float *)0x50 = 0.5625f;

						*(u32*)(FOffset+36) = *(u32*)(FOffset+32);

						memcpy((void*)(FOffset+ 28),(void*)(FOffset+ 36),60);
						memcpy((void*)(FOffset+184),(void*)(FOffset+188),16);

						*(u32*)(FOffset+68) += 8;
						*(u32*)(FOffset+88) = 0x3C600000 | (0x80000050 >> 16); 		// lis		3, 0x8180
						*(u32*)(FOffset+92) = 0xC0230000 | (0x80000050 & 0xFFFF);	// lfs		1, -0x90 (3)
						*(u32*)(FOffset+96) = 0xEC240072;							// fmuls	1, 4, 1

					} break;
					case FCODE_J3DUClipper_clip:	//	clip
					{
						if( !ConfigGetConfig(NIN_CFG_FORCE_WIDE) )
							break;
						
						write32( FOffset,   0x38600000 );
						write32( FOffset+4, 0x4E800020 );
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[Clip] 0x%08X\r\n", FOffset );
						#endif
					} break;
					case FCODE___OSReadROM:	//	__OSReadROM
					{
						memcpy( (void*)FOffset, __OSReadROM, sizeof(__OSReadROM) );
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[__OSReadROM] 0x%08X\r\n", FOffset );
						#endif
					} break;
					case FCODE___GXSetVAT:	// Patch for __GXSetVAT, fixes the dungeon map freeze in Wind Waker
					{
						switch( TITLE_ID )
						{
							case 0x505A4C:	// The Legend of Zelda: Collector's Edition
								if( !(DOLSize == 3847012 || DOLSize == 3803812) )			// only patch the main.dol of the Zelda:ww game 
									break;
							case 0x475A4C:	// The Legend of Zelda: The Wind Waker
							{
								write32(FOffset, (read32(FOffset) & 0xff00ffff) | 0x00220000);
								memcpy( (void *)(FOffset + 4), __GXSetVAT_patch, sizeof(__GXSetVAT_patch) );	
								#ifdef DEBUG_PATCH
								dbgprintf("Patch:Applied __GXSetVAT patch\r\n");
								#endif
							} break;
							default:
							break;
						}			
					} break;
					case FCODE_EXIIntrruptHandler:
					{
						u32 PatchOffset = 0x4;
						while ((read32(FOffset + PatchOffset) != 0x90010004) && (PatchOffset < 0x20)) // MaxSearch=0x20
							PatchOffset += 4;
						if (read32(FOffset + PatchOffset) != 0x90010004) 	// stw r0, 0x0004(sp)
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Skipped **IntrruptHandler patch 0x%X (PatchOffset=Not Found) \r\n", FOffset, PatchOffset);
							#endif
							break;
						}

						POffset -= sizeof(TCIntrruptHandler);
						memcpy((void*)(POffset), TCIntrruptHandler, sizeof(TCIntrruptHandler));

						write32(POffset, read32((FOffset + PatchOffset)));

						PatchBL( POffset, (FOffset + PatchOffset) );						
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:Applied **IntrruptHandler patch 0x%X (PatchOffset=0x%X) \r\n", FOffset, PatchOffset);
						#endif
					} break;
					case FCODE_EXIDMA:	// EXIDMA
					{
						u32 off		= 0;
						u32 reg		=-1;
						u32 valueB	= 0;

						while(1)
						{
							off += 4;

							u32 op = read32( (u32)FOffset + off );
								
							if( reg == -1 )
							if( (op & 0xFC1F0000) == 0x3C000000 )	// lis rX, 0xCC00
							{
								reg = (op & 0x3E00000) >> 21;
								if( reg == 3 )
								{
									value  = read32( FOffset + off ) & 0x0000FFFF;
									#ifdef DEBUG_PATCH
									//dbgprintf("lis:%08X value:%04X\r\n", FOffset+off, value );
									#endif
								}
							}

							if( reg != -1 )
							if( (op & 0xFC000000) == 0x38000000 )	// addi rX, rY, z
							{
								u32 src = (op >> 16) & 0x1F;

								if( src == reg )
								{
									valueB = read32( FOffset + off ) & 0x0000FFFF;
									#ifdef DEBUG_PATCH
									//dbgprintf("addi:%08X value:%04X\r\n", FOffset+off, valueB);
									#endif
									break;
								}
							}

							if( op == 0x4E800020 )	// blr
								break;
						}
						
						if( valueB & 0x8000 )
							value =  (( value << 16) - (((~valueB) & 0xFFFF)+1) );
						else
							value =  (value << 16) + valueB;
						#ifdef DEBUG_PATCH
						//dbgprintf("F:%08X\r\n", value );
						#endif

						memcpy( (void*)(FOffset), EXIDMA, sizeof(EXIDMA) );
						
						valueB = 0x90E80000 | (value&0xFFFF);
						value  = 0x3D000000 | (value>>16);
						
						if( valueB & 0x8000 )
							value++;

						valueB+=4;
						
						write32( FOffset+0x10, value );
						write32( FOffset+0x14, valueB );

					} break;
					case FCODE___CARDStat_A:	// CARD timeout
					{
						write32( FOffset+0x124, 0x60000000 );	
						write32( FOffset+0x18C, 0x60000000 );	

					} break;
					case FCODE___CARDStat_B:	// CARD timeout
					{
						write32( FOffset+0x118, 0x60000000 );	
						write32( FOffset+0x180, 0x60000000 );	

					} break;
					case FCODE_ARStartDMA:	//	ARStartDMA
					{
						if( (TITLE_ID) == 0x47384D ||	// Paper Mario
							(TITLE_ID) == 0x475951 )	// Mario Superstar Baseball
						{
							memcpy( (void*)FOffset, ARStartDMA_PM, sizeof(ARStartDMA_PM) );
						}
						else if((TITLE_ID) == 0x474832) // NFS: HP2
						{
							memcpy( (void*)FOffset, ARStartDMA_NFS, sizeof(ARStartDMA_NFS) );
						}
						else if((TITLE_ID) == 0x47564A) // Viewtiful Joe
						{
							memcpy( (void*)FOffset, ARStartDMA_VJ, sizeof(ARStartDMA_VJ) );
						}
						else
						{
							memcpy( (void*)FOffset, ARStartDMA, sizeof(ARStartDMA) );
							if( (TITLE_ID) != 0x475852 &&	// Mega Man X Command Mission
								(TITLE_ID) != 0x474146 &&	// Animal Crossing
								(TITLE_ID) != 0x474156 &&	// Avatar Last Airbender
								(TITLE_ID) != 0x47504E )	// P.N.03
							{
								u32 PatchOffset = 0;
								for (PatchOffset = 0; PatchOffset < sizeof(ARStartDMA); PatchOffset += 4)
								{
									if (*(u32*)(ARStartDMA + PatchOffset) == 0x90C35028)	// 	stw		%r6,	AR_DMA_CNT@l(%r3)
									{
									#ifdef DEBUG_PATCH
										dbgprintf("Patch:[ARStartDMA] Length 0\r\n", FOffset );
									#endif
										write32(FOffset + PatchOffset, 0x90E35028);			// 	stw		%r7,	AR_DMA_CNT@l(%r3)
										break;
									}
								}
							}
						}
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:[ARStartDMA] 0x%08X\r\n", FOffset );
						#endif
					} break;
					case FCODE_GCAMSendCommand:
					{
						if( TRIGame == 1 )
						{
							POffset-= sizeof( GCAMSendCommand2 );
							memcpy( (void*)POffset, GCAMSendCommand2, sizeof(GCAMSendCommand2) );
							PatchB( POffset, FOffset );
							break;
						}
						if( TRIGame == 2 )
						{
							POffset-= sizeof( GCAMSendCommandVSExp );
							memcpy( (void*)POffset, GCAMSendCommandVSExp, sizeof(GCAMSendCommandVSExp) );
							PatchB( POffset, FOffset );
							break;
						}
						if( TRIGame == 3 )
						{
							POffset-= sizeof( GCAMSendCommandF );
							memcpy( (void*)POffset, GCAMSendCommandF, sizeof(GCAMSendCommandF) );
							PatchB( POffset, FOffset );
							break;
						}
						if( TRIGame == 4 )
						{
							POffset-= sizeof( GCAMSendCommand );
							memcpy( (void*)POffset, GCAMSendCommand, sizeof(GCAMSendCommand) );
							PatchB( POffset, FOffset );
							break;
						}
					} break;
					case FCODE___fwrite:	// patch_fwrite_Log
					case FCODE___fwrite_D:	// patch_fwrite_LogB
					{
						if( ConfigGetConfig( NIN_CFG_DEBUGGER ) || !ConfigGetConfig(NIN_CFG_OSREPORT) )
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Skipped [patch_fwrite_Log]\r\n");
							#endif
							break;
						}

						if( IsWiiU )
						{
							if( FPatterns[j].Patch == patch_fwrite_GC ) // patch_fwrite_Log works fine
							{
								#ifdef DEBUG_PATCH
								dbgprintf("Patch:Skipped [patch_fwrite_GC]\r\n");
								#endif
								break;
							}
						}
						memcpy((void*)FOffset, patch_fwrite_Log, sizeof(patch_fwrite_Log));
						if (FPatterns[j].PatchLength == FCODE___fwrite_D)
						{
							write32(FOffset, 0x7C852379); // mr.     %r5,%r4
							write32(FOffset + sizeof(patch_fwrite_Log) - 0x8, 0x38600000); // li      %r3,0
						}
						break;
					}
					case FCODE__SITransfer:	//	_SITransfer
					{
						//e.g. Mario Strikers
						if (write32A(FOffset + 0x60, 0x7CE70078, 0x7CE70038, 0)) // clear errors - andc r7,r7,r0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[_SITransfer] 0x%08X\r\n", FOffset );
							#endif
						}
						//e.g. Billy Hatcher
						if (write32A(FOffset + 0xF8, 0x7F390078, 0x7F390038, 0)) // clear errors - andc r7,r7,r0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[_SITransfer C] 0x%08X\r\n", FOffset );
							#endif
						}

						//e.g. Mario Strikers
						if (write32A(FOffset + 0x148, 0x5400007E, 0x50803E30, 0)) // clear tc - rlwinm r0,r0,0,1,31
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[_SITransfer] 0x%08X\r\n", FOffset );
							#endif
						}
						//e.g. Luigi's Mansion
						if (write32A(FOffset + 0x140, 0x5400007E, 0x50A03E30, 0)) // clear tc - rlwinm r0,r0,0,1,31
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[_SITransfer] 0x%08X\r\n", FOffset );
							#endif
						}
						//e.g. Billy Hatcher
						if (write32A(FOffset + 0x168, 0x5400007E, 0x50603E30, 0)) // clear tc - rlwinm r0,r0,0,1,31
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[_SITransfer C] 0x%08X\r\n", FOffset );
							#endif
						}
					} break;
					case FCODE_CompleteTransfer:	//	CompleteTransfer
					{
						//e.g. Mario Strikers
						if (write32A(FOffset + 0x38, 0x5400007C, 0x5400003C, 0)) // clear  tc - rlwinm r0,r0,0,1,30
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[CompleteTransfer] 0x%08X\r\n", FOffset );
							#endif
						}

						//e.g. Billy Hatcher (func before CompleteTransfer does this)
						if (write32A(FOffset - 0x18, 0x57FF007C, 0x57FF003C, 0)) // clear  tc - rlwinm r31,r31,0,1,30
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[CompleteTransfer] 0x%08X\r\n", FOffset );
							#endif
						}

						//e.g. Luigi's Mansion
						if (write32A(FOffset + 0x10, 0x3C000000, 0x3C008000, 0)) // clear  tc - lis r0,0x0000
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[CompleteTransfer] 0x%08X\r\n", FOffset );
							#endif
						}
					} break;
					case FCODE_SIInterruptHandler:	//	SIInterruptHandler
					{
						u32 PatchOffset = 0x4;
						while ((read32(FOffset + PatchOffset) != 0x90010004) && (PatchOffset < 0x20)) // MaxSearch=0x20
							PatchOffset += 4;
						if (read32(FOffset + PatchOffset) != 0x90010004) 	// stw r0, 0x0004(sp)
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Skipped **IntrruptHandler patch 0x%X (PatchOffset=Not Found) \r\n", FOffset, PatchOffset);
							#endif
							break;
						}

						POffset -= sizeof(SIIntrruptHandler);
						memcpy((void*)(POffset), SIIntrruptHandler, sizeof(SIIntrruptHandler));

						write32(POffset, read32((FOffset + PatchOffset)));

						PatchBL( POffset, (FOffset + PatchOffset) );						
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:Applied **IntrruptHandler patch 0x%X (PatchOffset=0x%X) \r\n", FOffset, PatchOffset);
						#endif

						//e.g. Billy Hatcher
						if (write32A(FOffset + 0xB4, 0x7F7B0078, 0x7F7B0038, 0)) // clear  tc - andc r27,r27,r0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIInterruptHandler] 0x%08X\r\n", FOffset );
							#endif
						}
						if (FPatterns[j].Length >= 0x134)
						{
							//e.g. Mario Strikers
							if (write32A(FOffset + 0x134, 0x7cA50078, 0x7cA50038, 0)) // clear  tc - andc r5,r5,r0
							{
								#ifdef DEBUG_PATCH
								dbgprintf("Patch:[SIInterruptHandler] 0x%08X\r\n", FOffset );
								#endif
							}
						}
					} break;
					case FCODE_SIInit:	//	SIInit
					{
						//e.g. Mario Strikers
						if (write32A(FOffset + 0x60, 0x3C000000, 0x3C008000, 0)) // clear tc - lis r0,0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIInit] 0x%08X\r\n", FOffset );
							#endif
						}
						//e.g. Luigi's Mansion
						if (write32A(FOffset + 0x44, 0x3C000000, 0x3C008000, 0)) // clear tc - lis r0,0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIInit] 0x%08X\r\n", FOffset );
							#endif
						}
						//e.g. Animal Crossing
						if (write32A(FOffset + 0x54, 0x3C000000, 0x3C008000, 0)) // clear tc - lis r0,0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIInit] 0x%08X\r\n", FOffset );
							#endif
						}
						//e.g. Billy Hatcher
						if (write32A(FOffset + 0x5C, 0x3C000000, 0x3C008000, 0)) // clear tc - lis r0,0
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIInit] 0x%08X\r\n", FOffset );
							#endif
						}
					} break;
					case FCODE_SIEnablePollingInterrupt:	//	SIEnablePollingInterrupt
					{
						//e.g. SSBM
						if (write32A(FOffset + 0x68, 0x60000000, 0x54A50146, 0)) // leave rdstint alone - nop
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIEnablePollingInterrupt A] 0x%08X\r\n", FOffset );
							#endif
						}
						if (write32A(FOffset + 0x6C, 0x60000000, 0x54A5007C, 0)) // leave tcinit tcstart alone - nop
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIEnablePollingInterrupt A] 0x%08X\r\n", FOffset );
							#endif
						}

						//e.g. Billy Hatcher
						if (write32A(FOffset + 0x78, 0x60000000, 0x57FF0146, 0)) // leave rdstint alone - nop
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIEnablePollingInterrupt B] 0x%08X\r\n", FOffset );
							#endif
						}
						if (write32A(FOffset + 0x7C, 0x60000000, 0x57FF007C, 0)) // leave tcinit tcstart alone - nop
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:[SIEnablePollingInterrupt B] 0x%08X\r\n", FOffset );
							#endif
						}
					} break;
					case FCODE_PI_FIFO_WP_A:	//	PI_FIFO_WP A
					{
						//e.g. F-Zero
						if (write32A(FOffset + 0x68, 0x54C600C2, 0x54C60188, 0))
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Patched [PI_FIFO_WP A] 0x%08X\r\n", FOffset + 0x68);
							#endif
						}
						if (write32A(FOffset + 0xE4, 0x540000C2, 0x54000188, 0))
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Patched [PI_FIFO_WP A] 0x%08X\r\n", FOffset + 0xE4);
							#endif
						}
					} break;
					case FCODE_PI_FIFO_WP_B:	//	PI_FIFO_WP B
					{
						//e.g. F-Zero
						if (write32A(FOffset + 0x4C, 0x540300C2, 0x54030188, 0))
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Patched [PI_FIFO_WP B] 0x%08X\r\n", FOffset + 0x4C);
							#endif
						}
					} break;
					case FCODE_PI_FIFO_WP_C:	//	PI_FIFO_WP C
					{
						//e.g. F-Zero
						if (write32A(FOffset + 0x14, 0x540600C2, 0x54060188, 0))
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Patched [PI_FIFO_WP C] 0x%08X\r\n", FOffset + 0x14);
							#endif
						}
					} break;
					case FCODE_PI_FIFO_WP_D:	//	PI_FIFO_WP D
					{
						//e.g. Paper Mario
						if (write32A(FOffset + 0x40, 0x540400C2, 0x54040188, 0))
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Patched [PI_FIFO_WP D] 0x%08X\r\n", FOffset + 0x40);
							#endif
						}
					} break;
					case FCODE_PI_FIFO_WP_E:	//	PI_FIFO_WP E
					{
						//e.g. Paper Mario
						if (write32A(FOffset + 0x34, 0x541E1FFE, 0x541E37FE, 0))
						{
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:Patched [PI_FIFO_WP E] 0x%08X\r\n", FOffset + 0x34);
							#endif
						}
					} break;
					case FCODE_RADTimerRead:
					{
						u32 res = 0;
						res += write32A(FOffset + 0x48, 0x6000142A, 0x60009E40, 0); // 0x8A15 << 1
						res += write32A(FOffset + 0x58, 0x6084ED4E, 0x60849E34, 0); // TB_BUS_CLOCK / 4000
						res += write32A(FOffset + 0x60, 0x3CC08A15, 0x3CC0CF20, 0);
						res += write32A(FOffset + 0x68, 0x60C6866C, 0x60C649A1 ,0);
						#ifdef DEBUG_PATCH
						dbgprintf("Patch:Patched [RADTimerRead] %u/4 times\r\n", res);
						#endif
					} break;
					default:
					{
						if( FPatterns[j].Patch == (u8*)ARQPostRequest )
						{
							if (   (TITLE_ID) != 0x474D53  // Super Mario Sunshine
								&& (TITLE_ID) != 0x474C4D  // Luigis Mansion 
								&& (TITLE_ID) != 0x474346  // Pokemon Colosseum
								&& (TITLE_ID) != 0x475049  // Pikmin
								&& (TITLE_ID) != 0x474146  // Animal Crossing
								&& (TITLE_ID) != 0x474C56) // Chronicles of Narnia
							{
								#ifdef DEBUG_PATCH
								dbgprintf("Patch:Skipped [ARQPostRequest]\r\n");
								#endif
								break;
							}
						}
						if( FPatterns[j].Patch == (u8*)SITransfer )
						{
							if( TRIGame == 0 )
							{
								#ifdef DEBUG_PATCH
								dbgprintf("Patch:Skipped [SITransfer]\r\n");
								#endif
								break;
							}
						}
						if( FPatterns[j].Patch == (u8*)DVDGetDriveStatus )
						{
							if( (TITLE_ID) != 0x474754 &&	// Chibi-Robo!
								(TITLE_ID) != 0x475041 )	// Pok駑on Channel
								break;
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:DVDGetDriveStatus\r\n");
							#endif
						}
						if( FPatterns[j].Patch == (u8*)PADIsBarrel )
						{
							if(memcmp((void*)(FOffset), PADIsBarrelOri, sizeof(PADIsBarrelOri)) != 0)
								break;
							#ifdef DEBUG_PATCH
							dbgprintf("Patch:PADIsBarrel\r\n");
							#endif
						}
						if( (FPatterns[j].Length >> 16) == (FCODES  >> 16) )
						{
							#ifdef DEBUG_PATCH
							dbgprintf("DIP:Unhandled dead case:%08X\r\n", FPatterns[j].Length );
							#endif
							
						} else
						{

							memcpy( (void*)(FOffset), FPatterns[j].Patch, FPatterns[j].PatchLength );
						}

					} break;
				}

				// If this is a patch group set all others of this group as found aswell
				if( FPatterns[j].Group )
				{
					for( k=0; k < sizeof(FPatterns)/sizeof(FuncPattern); ++k )
					{
						if( FPatterns[k].Group == FPatterns[j].Group )
						{
							if( !FPatterns[k].Found )		// Don't overwrite the offset!
								FPatterns[k].Found = -1;	// Usually this holds the offset, to determinate it from a REALLY found pattern we set it -1 which still counts a logical TRUE
							#ifdef DEBUG_PATCH
							//dbgprintf("Setting [%s] to found!\r\n", FPatterns[k].Name );
							#endif
						}
					}
				}
			}
		}
	}

	PatchState = PATCH_STATE_DONE;

	/*if(GAME_ID == 0x47365145) //Megaman Collection
	{
		memcpy((void*)0x5A110, OSReportDM, sizeof(OSReportDM));
		sync_after_write((void*)0x5A110, sizeof(OSReportDM));

		memcpy((void*)0x820FC, OSReportDM, sizeof(OSReportDM));
		sync_after_write((void*)0x820FC, sizeof(OSReportDM));

		#ifdef DEBUG_PATCH
		dbgprintf("Patch:Patched Megaman Collection\r\n");
		#endif
	}*/

	if(write64A(0x00141C00, DBL_0_7716, DBL_1_1574))
	{
		#ifdef DEBUG_PATCH
		dbgprintf("Patch:Patched Majoras Mask NTSC-U\r\n");
		#endif
	}
	else if(write64A(0x00130860, DBL_0_7716, DBL_1_1574))
	{
		#ifdef DEBUG_PATCH
		dbgprintf("Patch:Patched Majoras Mask PAL\r\n");
		#endif
	}

	if( (GAME_ID & 0xFFFFFF00) == 0x475A4C00 )	// GZL=Wind Waker
	{
		//Anti FrameDrop Panic
		/* NTSC-U Final */
		if(*(vu32*)(0x00221A28) == 0x40820034 && *(vu32*)(0x00256424) == 0x41820068)
		{
			//	write32( 0x03945B0, 0x8039408C );	// Test menu
			*(vu32*)(0x00221A28) = 0x48000034;
			*(vu32*)(0x00256424) = 0x48000068;
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Patched WW NTSC-U\r\n");
			#endif
		}
		/* NTSC-U Demo */
		if(*(vu32*)(0x0021D33C) == 0x40820034 && *(vu32*)(0x00251EF8) == 0x41820068)
		{
			*(vu32*)(0x0021D33C) = 0x48000034;
			*(vu32*)(0x00251EF8) = 0x48000068;
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Patched WW NTSC-U Demo\r\n");
			#endif
		}
		/* PAL Final */
		if(*(vu32*)(0x001F1FE0) == 0x40820034 && *(vu32*)(0x0025B5C4) == 0x41820068)
		{
			*(vu32*)(0x001F1FE0) = 0x48000034;
			*(vu32*)(0x0025B5C4) = 0x48000068;
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Patched WW PAL\r\n");
			#endif
		}
		/* NTSC-J Final */
		if(*(vu32*)(0x0021EDD4) == 0x40820034 && *(vu32*)(0x00253BCC) == 0x41820068)
		{
			*(vu32*)(0x0021EDD4) = 0x48000034;
			*(vu32*)(0x00253BCC) = 0x48000068;
			#ifdef DEBUG_PATCH
			dbgprintf("Patch:Patched WW NTSC-J\r\n");
			#endif
		}
		/*if( ConfigGetConfig( NIN_CFG_OSREPORT ) )
		{
			*(vu32*)(0x00006858) = 0x60000000;		//OSReport disabled patch
			PatchB( 0x00006950, 0x003191BC );
		}*/
	}
	/*else if( ((GAME_ID & 0xFFFFFF00) == 0x47414C00) || GAME_ID == 0x474C4D45 ) //Melee or NTSC Luigis Mansion
	{
		#ifdef DEBUG_PATCH
		dbgprintf("Patch:Skipping Wait for Handler\r\n");
		#endif
		SkipHandlerWait = true; //some patch doesnt get applied so without this it would freeze
	}*/

	for( j=0; j < sizeof(FPatterns)/sizeof(FuncPattern); ++j )
	{
		if( FPatterns[j].Found ) //Skip already found patches
			continue;

		#ifdef DEBUG_PATCH
		dbgprintf("Patch: [%s] not found\r\n", FPatterns[j].Name );
		#endif
	}

	return;
}
