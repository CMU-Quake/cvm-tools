/**
 * pickrecord.c: 
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
#include <inttypes.h>
#include <string.h>

#include "xplatform.h"

int main(int argc, char **argv)
{
    FILE *cvmfp;
    off_t offset;
    int ini, inj, ink;
    int i, j, k;
    endian_t targetformat, myformat;
    
    if (argc != 4) {
        printf("\nusage: pickrecord cvmfile format offset\n");
        exit(1);
    }

    cvmfp = fopen(argv[1], "r");
    if (cvmfp == NULL) {
        perror("fopen");
        exit(1);
    }

    if (strcmp(argv[2], "little") == 0) {
        targetformat = little;
    } else if (strcmp(argv[2], "big") == 0) {
        targetformat = big;
    } else {
        fprintf(stderr, "Unknown target data format\n");
        exit(1);
    }


    sscanf(argv[3], "%qd", &offset);


#ifdef BIGBEN
    if (fseek(cvmfp, offset, SEEK_SET) != 0) {
        perror("fseeko");
        exit(1);
    }
#else 
    if (fseeko(cvmfp, offset, SEEK_SET) != 0) {
        perror("fseeko");
        exit(1);
    }
#endif

    if ((fread(&ini, 4, 1, cvmfp) != 1) ||
        (fread(&inj, 4, 1, cvmfp) != 1) ||
        (fread(&ink, 4, 1, cvmfp) != 1)) {
        perror("fread");
        exit(1);
    }

    myformat = xplatform_testendian();
    
    if (myformat == targetformat) {
        i = ini;
        j = inj;
        k = ink;
    } else {
        xplatform_swapbytes(&i, &ini, 4);
        xplatform_swapbytes(&j, &inj, 4);
        xplatform_swapbytes(&k, &ink, 4);
    }

    printf("(i, j, k) = {%d, %d, %d}\n", i, j, k);
    
    fclose(cvmfp);
    return 0;
}
