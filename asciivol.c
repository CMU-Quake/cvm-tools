/*
 * mesh.c - Convert cvm etree into a binary mesh
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>

#include "etree.h"
#include "cvm.h"

#define CVMBUFFERSIZE 100

/* Mesh node for the binary output file */
typedef struct mesh_node_t {
    int   i;
    int   j;
    int   k;
    float Vp;
    float Vs;
    float rho;
    float Qp;
    float Qs;
} mesh_node_t;

void usage(char *arg)
{
    fprintf(stdout,"\n Usage:\n");
    fprintf(stdout,"\n\t Option 1: volume grid\n");
    fprintf(stdout,"\t\t %s option cvmetree output field spacing \n",arg);
    fprintf(stdout,"\n\t Option 2: horizontal cut\n");
    fprintf(stdout,"\t\t %s option cvmetree output field spacing depth \n",arg);
    fprintf(stdout,"\n\t Option 3: vertical cut\n");
    fprintf(stdout,"\t\t %s option cvmetree output field spacing depth x1 y1 x2 y2 dz\n",arg);
    fprintf(stdout,"\n\t Option 4: isosurface\n");
    fprintf(stdout,"\t\t %s option cvmetree output field spacing target dz \n",arg);
    fprintf(stdout,"\n\t Field: 1=Vp 2=Vs 3=rho 4=Qp 5=Qs\n\n");
}

int main(int argc, char **argv)
{
    etree_t        *cvm;
    dbctl_t        *meta;
    char           *cvmetree, *output;
    FILE           *os;

    int             i=0, j=0, k=0, spacing, res, count = 0;
    int             imax = 0, jmax = 0, kmax = 0;
    int             field = 0;
    int             option = 0;

    double          x, y, z;
    double          x1, y1, x2, y2;
    double          d;
    double          elapsed;
    double          tickSize;
    double          tolerancefence = 0.0001;
    double          depth;
    double          target, dz;

    float           fieldvalue=0;
    float           Vs_kms;

    cvmpayload_t    prop;
    mesh_node_t     node;
    etree_addr_t    queryAddr;
    struct timeval  start, end;

    /* Parse args */

    fprintf(stdout, "\nArguments:       %d\n", argc);

    /* checking amount of args */
    if( (argc < 6) || (argc > 8) ) {
      if ( argc != 12 ) {
        usage( argv[0] );
        exit(1);
      }
    }

    /* reading option */
    if ( sscanf(argv[1], "%d", &option) != 1){
        usage(argv[0]);
	exit(1);
    }
    fprintf(stdout, "Option selected: %d\n", option);

    /* reading etree and output */
    cvmetree = argv[2];
    output = argv[3];
    fprintf(stdout, "Etree:           %s\n", cvmetree);
    fprintf(stdout, "Outputfile:      %s\n", output);

    /* reading field */
    if(sscanf(argv[4], "%d", &field) != 1){
        usage(argv[0]);
        exit(1);
    }
    fprintf(stdout, "Field:           %d\n", field);

    /* reading spacing */
    if(sscanf(argv[5], "%d", &spacing) != 1){
        usage(argv[0]);
        exit(1);
    }
    fprintf(stdout, "Spacing:         %d\n", spacing);

    /* reading depth */
    if ( (option == 2) || (option == 3) ) {
        if(sscanf(argv[6], "%lf", &depth) != 1){
	    usage(argv[0]);
	    exit(1);
	}
	fprintf(stdout, "Depth:           %lf\n", depth);
    }

    /* reading coordinates for vertical slice */
    if ( (option == 3) ) {
        if( (sscanf(argv[7],  "%lf", &x1) != 1) || 
	    (sscanf(argv[8],  "%lf", &y1) != 1) || 
	    (sscanf(argv[9],  "%lf", &x2) != 1) || 
	    (sscanf(argv[10], "%lf", &y2) != 1) ||
	    (sscanf(argv[11], "%lf", &dz)  != 1) )
	{
	    usage(argv[0]);
	    exit(1);
	}
	fprintf(stdout, "(x1,y1):(x2,y2)      (%lf,%lf):(%lf,%lf)\n", x1,y1,x2,y2);
    }

    /* reading target value and depth-step for isosurface */
    if ( option == 4 ) {
        if( ( sscanf(argv[6], "%lf", &target) != 1 ) ||
	    ( sscanf(argv[7], "%lf", &dz) != 1 ) ) {
	    usage(argv[0]);
	    exit(1);
	}
	fprintf(stdout, "Target:          %lf\n", target);
	fprintf(stdout, "dz:              %lf\n", dz);
    }

    /* Open the cvm-etree */

    cvm = etree_open(cvmetree, O_RDONLY, CVMBUFFERSIZE, 0, 0);
    if ( !cvm ) {
        fprintf(stderr, "Cannot open CVM etree %s\n", cvmetree);
        exit(1);
    }

    /* Open output file */
   
    os = fopen(output,"w");
    if ( os == NULL ) {
        fprintf(stderr, "Cannot open the output file");
        exit(1);
    }

    /* Find limits of the cvmetree */

    meta = cvm_getdbctl(cvm);
    if( meta==NULL ) {
        fprintf(stderr,"Unable to get cvm etree metadata\n");
        exit(1);
    }

    tickSize = meta->region_length_east_m / meta->domain_endpoint_x;
    queryAddr.level = ETREE_MAXLEVEL;

    imax = meta->region_length_east_m / spacing+1;
    jmax = meta->region_length_north_m / spacing+1;

    switch ( option )
    {
        case 1:
	    kmax = (int)( ( meta->region_depth_deep_m - meta->region_depth_shallow_m ) / spacing ) + 1;
	    z = 0;
	    break;
        case 2:
            kmax = 1;
	    z = depth;
	    break;
        case 3:
            kmax = (int)( depth / dz ) + 1;
	    z = 0;
	    break;
        case 4:
            kmax = 1;
	    z = 0;
	    break;
        default:
	    fprintf(stderr, "Cannot assign correct option for kmax\n");
	    exit(1);
	    break;
    }

    cvm_freedbctl(meta);

    printf("Mesh dimensions: %d x %d x %d\n",imax,jmax,kmax);

    if ( (option == 1) || (option == 2) || (option == 4)) {
    /* Generate mesh */
    for (k = 0; k < kmax; k++) 
    {
        node.k = k;
	if ( (k == kmax-1) && (option == 1) ) 
	{
	    z = z - tolerancefence;
	}
	queryAddr.z = (etree_tick_t)(z / tickSize);

	gettimeofday(&start,NULL);

	y = 0;
	for (j = 0; j < jmax; j++) 
        {
            node.j = j;
	    if ( j == jmax-1 ) 
	    {
	        y = y - tolerancefence;
	    }
	    queryAddr.y = (etree_tick_t)(y / tickSize);


	    x = 0;
	    fprintf(stdout,".");
	    fflush(stdout);
	    for (i = 0; i < imax; i++) 
	    {
	        node.i = i;
		if ( i == imax-1 ) 
		{
		    x = x - tolerancefence;
		}
		queryAddr.x = (etree_tick_t)(x / tickSize);

	    OPTIONROLLBACK:

		res = etree_search(cvm, queryAddr, NULL, NULL, &prop);
		if(res != 0) {
   	            fprintf(stderr, "Cannot find the query point (%f,%f,%f)\n", x, y, z);
	            exit(1);
		}
	
		node.Vp  = prop.Vp;
		node.Vs  = prop.Vs;
		node.rho = prop.rho;

		/* Min Vs */
		/*
		if(node.Vs < 500)
		{
		    Vs_lt500++;
		    node.Vs = 500;
		    node.Vp = 1500;
		}
		*/
	
		/* Qp and Qs via Brocher's */
		Vs_kms  = node.Vs / 1000; /* In km/s */
		node.Qs = (8.2184 * Vs_kms * Vs_kms * Vs_kms)
		        - (25.225 * Vs_kms * Vs_kms)
		        + (104.13 * Vs_kms) - 16;
	        node.Qp = 2 * node.Qs;

		/* selecting field to print */

	        switch ( field ) 
                {
		    case 1:
		        fieldvalue = node.Vp;
			break;
		    case 2:
		        fieldvalue = node.Vs;
			break;
		    case 3:
		        fieldvalue = node.rho;
			break;
		    case 4:
		        fieldvalue = node.Qp;
			break;
		    case 5:
		        fieldvalue = node.Qs;
			break;
		    default:
		        fprintf(stderr, "Cannot assign correct field\n");
			exit(1);
			break;
		}

		if ( option == 4 ) {
		  if ( fieldvalue < target ) {
		    z = z + dz;
		    fprintf(stdout,"+");
		    queryAddr.z = (etree_tick_t)(z / tickSize);
		    goto OPTIONROLLBACK;
		  }
		  // z = 0;
		  // queryAddr.z = (etree_tick_t)(z / tickSize);
		}

		/* printing the node */

		switch ( option ) {
		    case 1:
		        if ( fprintf(os, "%12.4f %12.4f %12.4f %12.4f\n", x, y, z, fieldvalue) < 0 ) {
			    fprintf(stderr, "Error writing output values for (%d,%d,%d)", i, j, k);
			    exit(1);
			}
			break;
		    case 2:
		        if ( fprintf(os, "%12.4f %12.4f %12.4f\n", x, y, fieldvalue) < 0 ) {
			    fprintf(stderr, "Error writing output values for (%d,%d)", i, j);
			    exit(1);
			}
			break;
		    case 4:
		        if ( fprintf(os, "%12.4f %12.4f %12.4f\n", x, y, z) < 0 ) {
			    fprintf(stderr, "Error writing output values for (%d,%d)", i, j);
			    exit(1);
			}
		        z = 0;
			queryAddr.z = (etree_tick_t)(z / tickSize);
			break;
		}

		x += spacing;
		count++;
	    }

	    if ( j % 50 == 0 ) {
	      fprintf(stdout,"\n");
	      fflush(stdout);
	    }
	    y += spacing;
	}

	gettimeofday (&end, NULL);
	elapsed = ( end.tv_sec - start.tv_sec ) * 1000.0 
	        + ( end.tv_usec - start.tv_usec ) / 1000.0;

	printf("\n Finished slice %d (%dpts) in %.2fms %fpps\n",
	        k,(imax*jmax),elapsed,(imax*jmax)/(elapsed/1000));

	z += spacing;
    }
    }


    /* ----------------------------- */
    /* VERTICAL CUT BETWEEN 2 POINTS */
    /* ----------------------------- */

    if ( option == 3) {
    int l, lmax;

    d = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1);
    d = sqrt(d);

    //fprintf(stdout,"\n Value of kmax is: %d\n",kmax);
    //fprintf(stdout,"Value of d is: %f\n",d);
 
    lmax = (int)( d / spacing ) + 1;

    for (k = 0; k < kmax; k++) 
    {
    	node.k = k;
    	if ( k == kmax-1 )
    	{
    		z = z - tolerancefence;
    	}
    	queryAddr.z = (etree_tick_t)(z / tickSize);

    	for (l = 0; l < lmax; l++)
    	{
    		x = x1 + l * ( (x2 - x1) * spacing / d );
    		y = y1 + l * ( (y2 - y1) * spacing / d );

    		if ( x == meta->region_length_east_m ) {
    			x = x - tolerancefence;
    		}

    		if ( y == meta->region_length_north_m ) {
    			y = y - tolerancefence;
    		}

    		queryAddr.x = (etree_tick_t)(x / tickSize);
    		queryAddr.y = (etree_tick_t)(y / tickSize);

    		res = etree_search(cvm, queryAddr, NULL, NULL, &prop);
    		if(res != 0)
    		{
    			fprintf(stderr, "Cannot find the query point (%f,%f,%f)\n", x, y, z);
    			exit(1);
    		}

    		node.Vp  = prop.Vp;
    		node.Vs  = prop.Vs;
    		node.rho = prop.rho;

    		/* Qp and Qs via Brocher's */
    		Vs_kms  = node.Vs / 1000; /* In km/s */
    		node.Qs = (8.2184 * Vs_kms * Vs_kms * Vs_kms)
	            		- (25.225 * Vs_kms * Vs_kms)
	            		+ (104.13 * Vs_kms) - 16;
    		node.Qp = 2 * node.Qs;

    		/* selecting field to print */

    		switch ( field )
    		{
    		case 1:
    			fieldvalue = node.Vp;
    			break;
    		case 2:
    			fieldvalue = node.Vs;
    			break;
    		case 3:
    			fieldvalue = node.rho;
    			break;
    		case 4:
    			fieldvalue = node.Qp;
    			break;
    		case 5:
    			fieldvalue = node.Qs;
    			break;
    		default:
    			fprintf(stderr, "Cannot assign correct field\n");
    			exit(1);
    			break;
    		}

    		/* printing the node */

    		/* 	    if ( fprintf(os, "%d %12.4f %12.4f %12.4f %12.4f\n", spacing*l, x, y, z, fieldvalue) < 0 )  */
    		if ( fprintf(os, "%d %12.4f %12.4f\n", spacing*l, z, fieldvalue) < 0 )
    		{
    			fprintf(stderr, "Error writing output values for (%d,%d)", k, l);
    			exit(1);

    		}
    		fprintf(stdout,".");

    	}

    	z += dz;
    	fprintf(stdout,"\n");
    }
    }
  
    fclose(os);
    etree_close(cvm);

    printf("Total number of points: %d\n", count);

    return 0;
}
