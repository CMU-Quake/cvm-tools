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
//#include "cvm.h"

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
    char    *meshetree, *outputfile;
    etree_t *meshEp = NULL;
    double   east_m, north_m, depth_m, value, res;
    double   EAST = 600000, NORTH = 300000;
    double   tolerancefence = 0.0001;
    mdata_t  rawElem;
    int      resp, useId, field, i=0, j=0, imax, jmax;
    FILE    *fp;

    /* -------------------------------------------------------------------------
     * BASIC FOR ALL OPTIONS
     * -------------------------------------------------------------------------
     */

    if (argc < 3 ) {
        fprintf(stderr, "\nReview querymesh usage\n");
    } else {
        meshetree = argv[1];
        meshEp = etree_open(meshetree, O_RDONLY, CVMBUFFERSIZE, 0, 0);
        if (!meshEp) {
            fprintf(stderr, "Cannot open mesh etree %s\n", meshetree);
            exit(1);
        }
        sscanf(argv[2], "%d", &useId);
    }
    
    /* -------------------------------------------------------------------------
     * OPTION 1: SINGLE QUERY 
     * -------------------------------------------------------------------------
     */
     
    if (useId == 1) { /* Start option 1 */

    if (argc != 6) {
        fprintf(stderr, 
            "\nusage: querymesh meshetree use east_m north_m depth_m\n");
        exit(1);
    }

    sscanf(argv[3], "%lf", &east_m);
    sscanf(argv[4], "%lf", &north_m);
    sscanf(argv[5], "%lf", &depth_m);

    resp = mesh_query(meshEp, east_m, north_m, depth_m, NULL, &rawElem.nid[0]);
    
    if (resp != 0) {
        fprintf(stderr, "Cannot find the query point\n");
        exit(1);
    } else {
        fprintf(stdout,"%f %f %f %.4f %.4f %.4f\n", 
            east_m, north_m, depth_m, rawElem.Vp, rawElem.Vs, rawElem.rho);
    }

    } /* End Option 1 */
    
    /* -------------------------------------------------------------------------
     * OPTION 2: HORIZONTAL SLICE 
     * -------------------------------------------------------------------------
     */
    
    if (useId == 2) { /* Start option 2 */
    
    if (argc != 7) {
        fprintf(stderr, 
            "\nusage: querymesh meshetree use field res depth_m outputfile\n");
        exit(1);
    }
    
    sscanf(argv[3], "%d", &field);
    sscanf(argv[4],"%lf", &res);
    sscanf(argv[5],"%lf", &depth_m);
    outputfile = argv[6];

    fp = fopen(outputfile,"w");
    if ( fp == NULL ) {
        fprintf(stderr, "Cannot open the output file");
        exit(1);
    }

    imax = EAST  / res + 1;
    jmax = NORTH / res + 1;


    for (i = 0; i < imax; i++) {
 
	if ( i == imax - 1 ) {
	    east_m = EAST - tolerancefence;
	} else {
	    east_m = i * res;
	}

	for (j = 0; j < jmax; j++) {

   	    if ( j == jmax - 1 ) {
	        north_m = NORTH - tolerancefence;
  	    } else {
	        north_m = j * res;
	    }

            resp = mesh_query(meshEp, east_m, north_m, depth_m, 
			      NULL, &rawElem.nid[0]);
    
	    switch ( field ) 
            {
	        case 1:
		    value = rawElem.Vp;
		    break;
		case 2:
		    value = rawElem.Vs;
		    break;
		case 3:
		    value = rawElem.rho;
		    break;
		default:
		    fprintf(stderr, "Cannot assign correct field\n");
		    exit(1);
		    break;
	    }
   
	    if (resp != 0) {
 	        fprintf(stderr, "Cannot find the query point\n");
		exit(1);
	    } else {
	        fprintf(fp, "%12.0f %12.0f %12.4f\n", 
			east_m, north_m, value);
	    }
	} 
    }

    fclose(fp);

    } /* End Option 2 */
    
    /* -------------------------------------------------------------------------
     * OPTIONs END 
     * -------------------------------------------------------------------------
     */

    etree_close(meshEp);

    return 0;
}
        

