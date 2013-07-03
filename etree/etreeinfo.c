/*
 * etreeinfo.c - Print the etree statistics, its schema if defined.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "etree.h"

/*
 * main
 *
 * - open the etree and perform a sequential scan
 * - count the leaf octants and index octants, respectively
 * - compare with the statsitcs
 * - print out results
 *
 */

int main(int argc, char ** argv)
{
    char *etreefile, *schematxt, *appmeta;
    int HaveSchema;
    etree_t *ep;

    /* read command line argument */
    if (argc != 2) {
        fprintf(stderr, "Usage: checketree etreename\n");
        exit(-1);
    }
    etreefile = argv[1];
    
    
    /* open the etree for read */
    if ((ep = etree_open(etreefile, O_RDONLY, 0, 0, 0)) == NULL) {
        fprintf(stderr, "Fail to open the etree\n");
        exit(-1);
    }   

    system("clear");
    printf("\n");
    printf("Status of etree %s\n\n", ep->pathname);

    printf("Dimension:\t%d\n", ep->dimensions);

    schematxt = etree_getschema(ep);
    if (schematxt != NULL) {
        HaveSchema = 1;
        printf("Schema:\t%s\n", schematxt);
        free(schematxt);
    } else {
        HaveSchema = 0;
        printf("Schema not defined.\n");
    }
    
    appmeta = etree_getappmeta(ep);
    if (appmeta != NULL) {
        printf("Application metadata:\t%s\n", appmeta);
        free(appmeta);
    } else {
        printf("Application metadata not defined.\n");
    }
    
    printf("Maximum leaf level %d\n", etree_getmaxleaflevel(ep));
    printf("Minimum leaf level %d\n", etree_getminleaflevel(ep));
    printf("Average leaf level %f\n", etree_getavgleaflevel(ep));

    /* release etree resources */
    etree_close(ep);

    return 0;
}


