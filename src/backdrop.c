//-------------------------------------------------------------------------
//
// The MIT License (MIT)
//
// Copyright (c) 2019 Chintalagiri Shashank
// Copyright (c) 2013 Andrew Duncan
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//-------------------------------------------------------------------------

#define _GNU_SOURCE

#include <assert.h>
#include <ctype.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <raspidmx/common/backgroundLayer.h>
#include <raspidmx/common/imageLayer.h>
#include <raspidmx/common/key.h>
#include <raspidmx/common/loadpng.h>

#include "bcm_host.h"

//-------------------------------------------------------------------------

#define NDEBUG

//-------------------------------------------------------------------------

const char *program = NULL;

//-------------------------------------------------------------------------

volatile bool run = true;

//-------------------------------------------------------------------------

static void signalHandler(int signalNumber)
{
    switch (signalNumber)
    {
    case SIGINT:
    case SIGTERM:
        fclose(stdin);
        run = false;
        break;
    };
}

//-------------------------------------------------------------------------

void usage(void)
{
    fprintf(stderr, "Usage: %s ", program);
    fprintf(stderr, "[-d <number>] [-l <layer>] ");
    fprintf(stderr, "[-x <offset>] [-y <offset>] [-w <pixels>] [-h <pixels>]\n");
    fprintf(stderr, "    -d - Raspberry Pi display number\n");
    fprintf(stderr, "    -l - DispmanX layer number\n");
    fprintf(stderr, "    -x - offset (pixels from the left)\n");
    fprintf(stderr, "    -y - offset (pixels from the top)\n");
    fprintf(stderr, "    -w - width  (pixels)\n");
    fprintf(stderr, "    -h - height (pixels)\n");

    exit(EXIT_FAILURE);
}

//-------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    int32_t layer = 1;
    uint32_t displayNumber = 0;
    uint32_t xOffset = 200;
    uint32_t yOffset = 200;
    uint32_t xSize = 200;
    uint32_t ySize = 200;

    program = basename(argv[0]);

    //---------------------------------------------------------------------

    int opt = 0;

    while ((opt = getopt(argc, argv, "b:d:l:x:y:w:h:")) != -1)
    {
        switch(opt)
        {
        case 'd':

            displayNumber = strtol(optarg, NULL, 10);
            break;

        case 'l':

            layer = strtol(optarg, NULL, 10);
            break;

        case 'x':

            xOffset = strtol(optarg, NULL, 10);
            break;

        case 'y':

            yOffset = strtol(optarg, NULL, 10);
            break;
        
        case 'w':
            xSize = strtol(optarg, NULL, 10);
            break;

        case 'h':
            ySize = strtol(optarg, NULL, 10);
            break;

        default:

            usage();
            break;
        }
    }

    //---------------------------------------------------------------------

    if (signal(SIGINT, signalHandler) == SIG_ERR)
    {
        perror("installing SIGINT signal handler");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------

    if (signal(SIGTERM, signalHandler) == SIG_ERR)
    {
        perror("installing SIGTERM signal handler");
        exit(EXIT_FAILURE);
    }
    
    //---------------------------------------------------------------------

    bcm_host_init();

    //---------------------------------------------------------------------

    DISPMANX_DISPLAY_HANDLE_T display
        = vc_dispmanx_display_open(displayNumber);
    assert(display != 0);

    //---------------------------------------------------------------------

    DISPMANX_MODEINFO_T info;
    int result = vc_dispmanx_display_get_info(display, &info);
    assert(result == 0);

    //---------------------------------------------------------------------
    
    VC_IMAGE_TYPE_T type = VC_IMAGE_RGBA32;
    uint32_t vc_image_ptr;
    
    DISPMANX_RESOURCE_HANDLE_T resource =
        vc_dispmanx_resource_create(type, 2, 2, &vc_image_ptr);
    assert(resource != 0);
    
    VC_DISPMANX_ALPHA_T alpha =
    {
        DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS, 
        255, /*alpha 0->255*/
        0
    };
    
    VC_RECT_T src_rect;
    VC_RECT_T dst_rect;
    
    vc_dispmanx_rect_set(&src_rect, xOffset, yOffset, xSize, ySize);
    vc_dispmanx_rect_set(&dst_rect, xOffset, yOffset, xSize, ySize);
    
    //---------------------------------------------------------------------

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);
    assert(update != 0);

    DISPMANX_ELEMENT_HANDLE_T element =
        vc_dispmanx_element_add(update,
                                display,
                                layer, // layer
                                &dst_rect,
                                resource,
                                &src_rect,
                                DISPMANX_PROTECTION_NONE,
                                &alpha,
                                NULL, // clamp
                                DISPMANX_NO_ROTATE);
    assert(element != 0);
    
    //---------------------------------------------------------------------

    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);

    while (run)
    {
        
        scanf("%d,%d,%d,%d", &xOffset, &yOffset, &xSize, &ySize);
        
        update = vc_dispmanx_update_start(0);
        assert(update != 0);
        
        //-----------------------------------------------------------------
        
        result = vc_dispmanx_element_remove(update, element);
        assert(result == 0);
        
        //-----------------------------------------------------------------
        
        vc_dispmanx_rect_set(&src_rect, xOffset, yOffset, xSize, ySize);
        vc_dispmanx_rect_set(&dst_rect, xOffset, yOffset, xSize, ySize);
        
        element = vc_dispmanx_element_add(
                                update,
                                display,
                                layer, // layer
                                &dst_rect,
                                resource,
                                &src_rect,
                                DISPMANX_PROTECTION_NONE,
                                &alpha,
                                NULL, // clamp
                                DISPMANX_NO_ROTATE);
        assert(element != 0);

        //-----------------------------------------------------------------
        
        result = vc_dispmanx_update_submit_sync(update);
        assert(result == 0);
    }

    //---------------------------------------------------------------------

    keyboardReset();

    //---------------------------------------------------------------------
    update = vc_dispmanx_update_start(0);
    assert(update != 0);
    result = vc_dispmanx_element_remove(update, element);
    assert(result == 0);
    result = vc_dispmanx_update_submit_sync(update);
    assert(result == 0);
    //---------------------------------------------------------------------
    result = vc_dispmanx_resource_delete(resource);
    assert(result == 0);
    //---------------------------------------------------------------------
    result = vc_dispmanx_display_close(display);
    assert(result == 0);
    //---------------------------------------------------------------------

    return 0;
}

