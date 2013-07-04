/**
 * findpixel - Query an etree for the pixel.
 *
 * Copyright (c) 2003 Tiankai Tu
 * All rights reserved.  May not be used, modified, or copied 
 * without permission.
 *
 * Tiankai Tu
 * Computer Science Department
 * Carnegie Mellon University
 * 5000 Forbes Avenue
 * Pittsburgh, PA 15213
 * tutk@cs.cmu.edu
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "etree.h"


int main(int argc, char **argv)
{
    char *etreeName;
    etree_t *ep;
    etree_addr_t pixelAddr, hitAddr;

    if (argc != 5) {
        printf("usage: findpixel etreename x y z\n");
        exit(-1);
    }

    etreeName = argv[1];
    sscanf(argv[2], "%u", &pixelAddr.x);
    sscanf(argv[3], "%u", &pixelAddr.y);
    sscanf(argv[4], "%u", &pixelAddr.z);
    pixelAddr.level = ETREE_MAXLEVEL;

    if ((ep = etree_open(etreeName, O_RDONLY, 0, 0, 0))  == NULL) {
        printf("Fail to open etree %s\n", etreeName);
        exit(-1);
    }

    if (etree_search(ep, pixelAddr, &hitAddr, NULL, NULL) != 0) {
        printf("etree_search: %s\n", etree_strerror(etree_errno(ep)));
        exit(-1);
    }

    printf("Pixel %u %u %u is contained in octant %u %u %u %d\n",
           pixelAddr.x, pixelAddr.y, pixelAddr.z,
           hitAddr.x, hitAddr.y, hitAddr.z, hitAddr.level);
    
    etree_close(ep);
    
    return 0;
}
