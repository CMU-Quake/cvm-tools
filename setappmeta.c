/*
 * setappmeta.c - Opens an etree with no schema and sets it as read.
 * 
 * by: Ricardo Taborda
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include "etree.h"

void usage()
{
    printf("\n\t Usage:");
    printf("\n\t [0]setappmeta [1]etree [2]title [3]author [4]date");
    printf("\n\t [5]length [6]width [7]depth");
    printf("\n\t [8]originLatitude [9]originLongitude");
    printf("\n\t [10]lengthticks, [11]widthticks, [12]depthticks\n\n");
}

int main ( int argc, char **argv ) {

    etree_t      *ep;
    char          appmeta[1024]; 
    char         *etree;
    char         *title,  *author, *date;
    double        length=0, width, depth;
    double        latitude, longitude;
    etree_tick_t  lengthticks,
                  widthticks,
                  depthticks;
    long temp;

    etree   = argv[1];
    title   = argv[2];
    author  = argv[3];
    date    = argv[4];

    if(sscanf(argv[5], "%lf", &length) != 1){
        usage();
	exit(1);
    }

    if(sscanf(argv[6], "%lf", &width) != 1){
        usage();
	exit(1);
    }

    if(sscanf(argv[7], "%lf", &depth) != 1){
        usage();
	exit(1);
    }

    if(sscanf(argv[8], "%lf", &latitude) != 1){
        usage();
	exit(1);
    }

    if(sscanf(argv[9], "%lf", &longitude) != 1){
        usage();
	exit(1);
    }

    if(sscanf(argv[10], "%ld", &temp) != 1){
      usage();
      exit(1);
    }

    lengthticks = (etree_tick_t)temp;

    if(sscanf(argv[11], "%ld", &temp) != 1){
      usage();
      exit(1);
    }

    widthticks = (etree_tick_t)temp;

    if(sscanf(argv[12], "%ld", &temp) != 1){
      usage();
      exit(1);
    }

    depthticks = (etree_tick_t)temp;

    /*
     * lengthticks = 1<<ETREE_MAXLEVEL;
     * widthticks  = lengthticks * (width / length);
     * depthticks  = lengthticks * (depth / length);
     */

    /* open the unpacked etree */
    ep = etree_open(etree, O_RDWR, 0, 0, 3);
    if (ep == NULL) {
        fprintf(stderr, "\tFailed to open the %s etree (unpacked)\n", 
                etree);
        exit(-1);
    }

    sprintf(appmeta,
            "Title:%s Author:%s Date:%s %u %s %lf %lf %lf %lf %lf %lf %u %u %u", 
            title, author, date,
            3, "Vp(float);Vs(float);density(float)",
            latitude, longitude,
            length, width, 0.000000, depth,
            lengthticks, widthticks, depthticks);

    /* applies the metadata to the etree */
    if (etree_setappmeta(ep, appmeta) != 0) 
    {
        fprintf(stderr, "%s\n", etree_strerror(etree_errno(ep)));
        exit(-1);
    }

    /* closes the etree */
    if (etree_close(ep) != 0) 
    {
        fprintf(stderr, "Error closing etree\n");
        exit(-1);
    }

    return 0;
}
