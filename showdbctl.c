/**
 * showdbctl.c - print material database control/meta data.
 * 
 * Copyright (c) 2004 Tiankai Tu
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
#include <string.h>

#include "etree.h"
#include "cvm.h"

int main(int argc, char ** argv)
{
    char *etreefile;
    dbctl_t *myctl;
    etree_t *ep;

    /* read command line argument */
    if (argc != 2) {
        fprintf(stderr, "Usage: showdbctl etreename\n");
        exit(1);
    }

    etreefile = argv[1];

    if ((ep = etree_open(etreefile, O_RDONLY, 0, 0, 0)) == NULL) {
        fprintf(stderr, "Fail to open the etree\n");
        exit(1);
    }

    myctl = cvm_newdbctl();
    if (myctl == NULL) {
        perror("cvm_newdbctl");
        exit(1);
    }

    if ((myctl = cvm_getdbctl(ep)) == NULL) {
        fprintf(stderr, "Cannot get the material database control data\n");
        exit(1);
    }

    printf("\n");
    printf("create_db_name:               %s\n", ep->pathname);
    printf("create_model_name:            %s\n", myctl->create_model_name);
    printf("create_author:                %s\n", myctl->create_author);
    printf("create_date:                  %s\n", myctl->create_date);
    printf("create_field_count:           %s\n", myctl->create_field_count);
    printf("create_field_names:           %s\n", myctl->create_field_names);
    printf("\n");

    printf("region_origin_latitude_deg:   %.8f\n", 
           myctl->region_origin_latitude_deg);
    printf("region_origin_longitude_deg:  %.8f\n", 
           myctl->region_origin_longitude_deg);
    printf("region_length_east_m:         %.8f\n", myctl->region_length_east_m);
    printf("region_length_north_m:        %.8f\n", myctl->region_length_north_m);
    printf("region_depth_shallow_m:       %.8f\n", 
           myctl->region_depth_shallow_m);
    printf("region_depth_deep_m:          %.8f\n", myctl->region_depth_deep_m);
    printf("\n");

    printf("domain_endpoint_x:            %u\n", myctl->domain_endpoint_x);
    printf("domain_endpoint_y:            %u\n", myctl->domain_endpoint_y);
    printf("domain_endpoint_z:            %u\n", myctl->domain_endpoint_z);

    cvm_freedbctl(myctl);
    
    etree_close(ep);
    
    return 0;
}

        
