//
//  main.c
//  Extension
//
//  Created by Dave Hayden on 7/30/14.
//  Copyright (c) 2014 Panic, Inc. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8_jit.h"
#include "pd_api.h"

static int update(void* userdata);
LCDBitmap* bitmap = NULL;
Chip8* VM_state = NULL;
#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed
	pd->display->setRefreshRate(0.0f);
	if ( event == kEventInit )
	{
	    init_from_header(&VM_state, pd);
		bitmap =  pd->graphics->newBitmap(64,32,kColorBlack);
	    //pd->file->mkdir("/ROMS");
		pd->system->setUpdateCallback(update, pd);
		pd->display->setScale(4);

	}

	return 0;
}


#define TEXT_WIDTH 86
#define TEXT_HEIGHT 16

int x = (400-TEXT_WIDTH)/2;
int y = (240-TEXT_HEIGHT)/2;
int dx = 1;
int dy = 2;
const int rowBytes = 8;
static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;
	run_frame(VM_state, pd);
	pd->graphics->clear(kColorBlack);
	if(bitmap){
	    uint8_t* bitmapData;
	    pd->graphics->getBitmapData(bitmap, 0, 0,&rowBytes, NULL,  &bitmapData);
		memcpy(bitmapData, VM_state->disp, 256);
		pd->graphics->drawBitmap(bitmap,32,0,0);
	}
	pd->system->drawFPS(0,0);

	return 1;
}
