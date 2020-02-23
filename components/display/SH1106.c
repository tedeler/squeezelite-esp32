/**
 * Copyright (c) 2017-2018 Tara Keeling
 *				 2020 Philippe G. 
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <esp_heap_caps.h>
#include <esp_log.h>

#include "gds.h"
#include "gds_private.h"

#define SHADOW_BUFFER

static char TAG[] = "SH1106";

// Functions are not declared to minimize # of lines

static void SetColumnAddress( struct GDS_Device* Device, uint8_t Start, uint8_t End ) {
	// well, unfortunately this driver is 132 colums but most displays are 128...
	if (Device->Width != 132) Start += 2;
	Device->WriteCommand( Device, 0x10 | (Start >> 4) );
	Device->WriteCommand( Device, 0x00 | (Start & 0x0f) );
}

static void SetPageAddress( struct GDS_Device* Device, uint8_t Start, uint8_t End ) {
	Device->WriteCommand( Device, 0xB0 | Start );	
}	

static void Update( struct GDS_Device* Device ) {
#ifdef SHADOW_BUFFER
	// not sure the compiler does not have to redo all calculation in for loops, so local it is
	int width = Device->Width, rows = Device->Height / 8;
	uint8_t *optr = Device->Shadowbuffer, *iptr = Device->Framebuffer;
	
	// by row, find first and last columns that have been updated
	for (int r = 0; r < rows; r++) {
		uint8_t first = 0, last;	
		for (int c = 0; c < width; c++) {
			if (*iptr != *optr) {
				if (!first) first = c + 1;
				last = c ;
			}	
			*optr++ = *iptr++;
		}
		
		// now update the display by "byte rows"
		if (first--) {
			SetColumnAddress( Device, first, last );
			SetPageAddress( Device, r, r);
			Device->WriteData( Device, Device->Shadowbuffer + r*width + first, last - first + 1);
		}
	}	
#else	
	// SH1106 requires a page-by-page update and has no end Page/Column
	for (int i = 0; i < Device->Height / 8 ; i++) {
		SH1106_SetPageAddress( Device, i, 0);
		SH1106_SetColumnAddress( Device, 0, 0);			
		SH1106_WriteData( Device, Device->Framebuffer + i*Device->Width, Device->Width );
	}	
#endif	
}

static void SetHFlip( struct GDS_Device* Device, bool On ) { Device->WriteCommand( Device, On ? 0xA1 : 0xA0 ); }
static void SetVFlip( struct GDS_Device *Device, bool On ) { Device->WriteCommand( Device, On ? 0xC8 : 0xC0 ); }
static void DisplayOn( struct GDS_Device* Device ) { Device->WriteCommand( Device, 0xAF ); }
static void DisplayOff( struct GDS_Device* Device ) { Device->WriteCommand( Device, 0xAE ); }

static void SetContrast( struct GDS_Device* Device, uint8_t Contrast ) {
    Device->WriteCommand( Device, 0x81 );
    Device->WriteCommand( Device, Contrast );
}

static bool Init( struct GDS_Device* Device ) {
	Device->FramebufferSize = ( Device->Width * Device->Height ) / 8;	
	Device->Framebuffer = calloc( 1, Device->FramebufferSize );
    NullCheck( Device->Framebuffer, return false );
	
#ifdef SHADOW_BUFFER	
	if (Device->IF == IF_I2C) Device->Shadowbuffer = malloc( Device->FramebufferSize );
	else Device->Shadowbuffer = heap_caps_malloc( Device->FramebufferSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA );
	NullCheck( Device->Shadowbuffer, return false );
	memset(Device->Shadowbuffer, 0xFF, Device->FramebufferSize);
#endif	
		
	// need to be off and disable display RAM
	Device->DisplayOff( Device );
    Device->WriteCommand( Device, 0xA5 );
	
	// charge pump regulator, do direct init
	Device->WriteCommand( Device, 0xAD );
	Device->WriteCommand( Device, 0x8B ); 
			
	// COM pins HW config (alternative:EN) - some display might need something difference
	Device->WriteCommand( Device, 0xDA );
	Device->WriteCommand( Device, 1 << 4);
		
	// MUX Ratio
    Device->WriteCommand( Device, 0xA8 );
    Device->WriteCommand( Device, Device->Height - 1);
	// Display Offset
    Device->WriteCommand( Device, 0xD3 );
    Device->WriteCommand( Device, 0 );
	// Display Start Line
    Device->WriteCommand( Device, 0x40 + 0x00 );
	Device->SetContrast( Device, 0x7F );
	// set flip modes
	Device->SetVFlip( Device, false );
	Device->SetHFlip( Device, false );
	// no Display Inversion
    Device->WriteCommand( Device, 0xA6 );
	// set Clocks
    Device->WriteCommand( Device, 0xD5 );
    Device->WriteCommand( Device, ( 0x08 << 4 ) | 0x00 );
		
	// gone with the wind
	Device->WriteCommand( Device, 0xA4 );
	Device->DisplayOn( Device );
	Device->Update( Device );
	
	return true;
}	

static const struct GDS_Device SH1106 = {
	.DisplayOn = DisplayOn, .DisplayOff = DisplayOff, .SetContrast = SetContrast,
	.SetVFlip = SetVFlip, .SetHFlip = SetHFlip,
	.DrawPixel = GDS_DrawPixel, .DrawPixelFast = GDS_DrawPixelFast,
	.Update = Update, .Init = Init,
};	

struct GDS_Device* SH1106_Detect(char *Driver, struct GDS_Device* Device) {
	if (!strcasestr(Driver, "SH1106")) return NULL;
	
	if (!Device) Device = calloc(1, sizeof(struct GDS_Device));
	*Device = SH1106;	
	ESP_LOGI(TAG, "SH1106 driver");
	
	return Device;
}