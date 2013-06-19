/**
 * querymesh.c
 *
 * Copyright (c) 2008 Ricardo Taborda, 2004 Tiankai Tu
 * All rights reserved.  May not be used, modified, or copied 
 * without permission.
 *
 * Contact:
 * Ricardo Taborda
 * Civil and Environmental Engineering
 * Carnegie Mellon University
 * 5000 Forbes Avenue
 * Pittsburgh, PA 15213
 * rtaborda@andrew.cmu.edu
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "etree.h"

#define CVMBUFFERSIZE 100 /* performance knob */

/**
 * mdata_t: Mesh database record payload (excluing the locational code).
 *
 */
typedef struct mdata_t {
    int64_t nid[8];
    float edgesize, Vp, Vs, rho;
} mdata_t;


/**
 * mesh_query:
 *
 * - return 0 if OK, -1 on error
 *
 */
int mesh_query(etree_t *ep, double east_m, double north_m, double depth_m,
              const char *field, void *payload)
{
    double       tickSize;
    etree_addr_t queryAddr;

    /* new factor of 2 at the end... who knows why but works */
    tickSize = 600000.0 / ( 2147483648.0 / 2 ); 

    queryAddr.y = (etree_tick_t)(east_m  / tickSize);
    queryAddr.x = (etree_tick_t)(north_m / tickSize);
    queryAddr.z = (etree_tick_t)(depth_m / tickSize);

    queryAddr.level = ETREE_MAXLEVEL;

    if (etree_search(ep, queryAddr, NULL, field, payload) != 0) {
        fprintf(stderr, "mesh_query: %s\n",etree_strerror(etree_errno(ep)));
        return -1;
    }

    return 0;
}


int main(int argc, char **argv)
{
    char    *meshetree, *kimIn, *kimOut, *cmuOut, *diffile, *diffileA;
    etree_t *meshEp = NULL;
    double   east_m, north_m, depth_m, kimVs, vsDiff;
    //double   EAST = 600000, NORTH = 300000;
    double   dxy;
    //double   tolerancefence = 0.0001;
    mdata_t  rawElem;
    int      kres=0, mres, pres1, pres2;//, i=0;
    FILE    *fpKimIn, *fpKimOut, *fpCmuOut, *fpDiff, *fpDiffA;

    /* Capturing input data */

    if (argc != 8) {
        fprintf(stderr, 
            "\nusage: mirrorslice meshetree kimfile cmuout kimout diffile absfile dxy\n");
        exit(1);
    }

    meshetree = argv[1];
    kimIn     = argv[2];
    cmuOut    = argv[3];
    kimOut    = argv[4];
    diffile   = argv[5];
    diffileA  = argv[6];

    sscanf(argv[7], "%lf", &dxy);

    /* Openning files */

    meshEp = etree_open(meshetree, O_RDONLY, CVMBUFFERSIZE, 0, 0);
    if (!meshEp) {
        fprintf(stderr, "Cannot open mesh etree %s\n", meshetree);
        exit(1);
    }

    fpKimIn = fopen(kimIn,"r");
    if ( fpKimIn == NULL ) {
        fprintf(stderr, "Cannot open the Kim input file");
        exit(1);
    }

    fpKimOut = fopen(kimOut,"w");
    if ( fpKimOut == NULL ) {
        fprintf(stderr, "Cannot open the Kim output file\n");
        exit(1);
    }

    fpCmuOut = fopen(cmuOut,"w");
    if ( fpCmuOut == NULL ) {
        fprintf(stderr, "Cannot open the CMU output file\n");
        exit(1);
    }

    fpDiff = fopen(diffile,"w");
    if ( fpDiff == NULL ) {
        fprintf(stderr, "Cannot open the Diff output file\n");
        exit(1);
    }
    
    fpDiffA = fopen(diffileA,"w");
    if ( fpDiffA == NULL ) {
        fprintf(stderr, "Cannot open the Diff output file\n");
        exit(1);
    }

    while ( kres != EOF ) {
    //for ( i = 0; i < 100; i++ ) {

        kres = fscanf(fpKimIn,"%lf %lf %lf %lf", 
		      &east_m, &north_m, &depth_m, &kimVs);

	pres1 = (int)east_m % (int)dxy;
	pres2 = (int)north_m % (int)dxy;

	if ( (pres1 == 0) && (pres2 == 0) ) { 

	mres = mesh_query(meshEp, east_m, north_m, depth_m, 
			  NULL, &rawElem.nid[0]);

	if (mres != 0) {
	    fprintf(stderr, "Cannot find the query point\n");
	    exit(1);
	}

	vsDiff = rawElem.Vs - kimVs;

	fprintf(fpKimOut, "%12.0f %12.0f %12.4f\n",east_m, north_m, kimVs);
	fprintf(fpCmuOut, "%12.0f %12.0f %12.4f\n",east_m, north_m, rawElem.Vs);
	fprintf(fpDiff,   "%12.0f %12.0f %12.4f\n",east_m, north_m, vsDiff);
	fprintf(fpDiffA,  "%12.0f %12.0f %12.4f\n",east_m, north_m, fabs(vsDiff));

	}
    }
    
    /* Closing files */

    fclose(fpKimIn);
    fclose(fpKimOut);
    fclose(fpCmuOut);
    fclose(fpDiff);
    fclose(fpDiffA);
    etree_close(meshEp);

    return 0;
}
        

