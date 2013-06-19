/**
 * querycvm.c
 *
 * Copyright (c) 2004 Tiankai Tu
 * All rights reserved.  May not be used, modified, or copied 
 * without permission.
 *
 * Contact:
 * Tiankai Tu
 * Computer Science Department
 * Carnegie Mellon University
 * 5000 Forbes Avenue
 * Pittsburgh, PA 15213
 * tutk@cs.cmu.edu
 *
 */
#include "stdio.h"
#include "stdlib.h"

#include "etree.h"
#include "cvm.h"

#define CVMBUFFERSIZE 100 /* performance knob */

int main(int argc, char **argv)
{
    char * cvmetree;
    etree_t *cvmEp;
    double east_m, north_m, depth_m;
    cvmpayload_t rawElem;
    int res;

    if (argc != 4) {
        printf("\nusage: querycvm east_m north_m depth_m\n\n");
        exit(1);
    }
    sscanf(argv[1], "%lf", &east_m);
    sscanf(argv[2], "%lf", &north_m);
    sscanf(argv[3], "%lf", &depth_m);

    
    cvmetree = getenv("CVMDB_PATH");
    if (!cvmetree) {
        fprintf(stderr, "Environment variable CVMDB_PATH not set\n");
        exit(1);
    }

    cvmEp = etree_open(cvmetree, O_RDONLY, CVMBUFFERSIZE, 0, 0);
    if (!cvmEp) {
        fprintf(stderr, "Cannot open CVM material database %s\n", cvmetree);
        exit(1);
    }

    res = cvm_query(cvmEp, east_m, north_m, depth_m, "*", &rawElem.Vp);
    if (res != 0) {
        fprintf(stderr, "Cannot find the query point\n");
        exit(1);
    } else {
        fprintf(stdout,"%f %f %f %.4f %.4f %.4f\n", east_m, north_m, depth_m, rawElem.Vp, rawElem.Vs, rawElem.rho);
	/*
        printf("\nMaterial property for\n(%f East, %f North, %f Depth)\n", 
	east_m, north_m, depth_m);
	printf("Vp =      %.4f\n", rawElem.Vp);
        printf("Vs =      %.4f\n", rawElem.Vs);
        printf("density = %.4f\n", rawElem.rho);
        printf("\n"); */
    }

    etree_close(cvmEp);

    return 0;
}
        

