/**
 * scancvm.c: Scan the content of the CVM database and print statistics
 *
 * Copyright (c) 2005 Tiankai Tu
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "etree.h"
#include "cvm.h"


#define KEYSIZE 13

int main(int argc, char **argv)
{
    char * cvmetree;
    etree_t *cvmEp;
    cvmpayload_t rawElem;
    int64_t totalcount;
    int32_t mycount;
    struct timeval starttime, endtime;
    int scantime;

    if (argc != 2) {
        printf("\nusage: scancvm cvmetree\n");
        exit(1);
    }

    cvmetree = argv[1];
    cvmEp = etree_open(cvmetree, O_RDONLY, 0, 0, 0);
    if (!cvmEp) {
        fprintf(stderr, "Cannot open CVM material database %s\n", cvmetree);
        exit(1);
    }

    /* go through all the records stored in the underlying btree */
    totalcount = 0;
    mycount = 0;
    memset(cvmEp->key, 0, KEYSIZE);

    gettimeofday(&starttime, NULL);

    if (btree_initcursor(cvmEp->bp, cvmEp->key) != 0) {
        fprintf(stderr, "Cannot set cursor in the underlying database\n");
        exit(1);
    }

    do {
        if (btree_getcursor(cvmEp->bp, cvmEp->hitkey, "*", &rawElem) != 0) {
            fprintf(stderr, "Read cursor error\n");
            exit(1);
        } 
        
        totalcount++;
        mycount++;
        if (mycount == 1000000) {
            fprintf(stderr, "1 million records scanned\n");
            mycount = 0;
        }
    } while (btree_advcursor(cvmEp->bp) == 0);
    
    gettimeofday(&endtime, NULL);

    scantime = (endtime.tv_sec - starttime.tv_sec);
    printf("Scanned the CVM database in %d seconds\n", scantime);
    printf("Scanned %qd octants\n", totalcount);
 
    etree_close(cvmEp);

    return 0;
}

            
    

