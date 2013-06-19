/**
 * mirrorrob.c
 *
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

/* 
 *  vector 2d struct
 *
 */
typedef struct vector2d_t{
  
  double x [ 2 ];
  
}vector2d_t;

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


/* 
 * compute_domain_coords_bilin: computes the domain (mesh) coordinates 
 *                              of a point based corners of the global 
 *                              coordinates (long lat) using a bilinear
 *                              interpolation
 *
 *                       csi(or X)
 *                        ^
 *                        |
 *                        |

 *              -         2 *************************** 3
 *              |         *                             *
 *              |         *                             *
 *              |         *                             *
 *      domainlengthetha  *                             *
 *              |         *                             *
 *              |         *                             *
 *              |         *                             *
 *              |         *                             *
 *              -         1 *************************** 4 -------->etha(or Y)
 *    
 *                        |------domainlenghtcsi--------|
 *
 *
 *
 *                       longcorner = lon of the nodes that define the box
 *                       latcorner  = lat             "
 *                  domainlengtheta = size of the vertical   edge (etha)
 *                  domainlengthcsi =       "     horizontal   "  (csi)  
 *
 */
vector2d_t compute_domain_coords_bilin(
		       double lon, double lat, 
                       double *longcorner, double *latcorner,
                       double domainlengthetha, double domainlengthcsi) {

    int i;
    double Ax,Ay,Bx,By,Cx,Cy,Dx,Dy,res,Xi[4],Yi[4],XN[2],M[2][2],F[2],DXN[2];
    double X,Y,tol;
    vector2d_t domainCoords;

    tol=1e-3;
  
    X=lat;
    Y=lon;
  
    for (i=0;i<4;i++){
        Xi[i]=latcorner[i];
	Yi[i]=longcorner[i];
    }
  
    Ax=4*X-(Xi[0]+Xi[1]+Xi[2]+Xi[3]);
    Ay=4*Y-(Yi[0]+Yi[1]+Yi[2]+Yi[3]);    

    Bx=-Xi[0]+Xi[1]+Xi[2]-Xi[3];
    By=-Yi[0]+Yi[1]+Yi[2]-Yi[3];
    Cx=-Xi[0]-Xi[1]+Xi[2]+Xi[3];
    Cy=-Yi[0]-Yi[1]+Yi[2]+Yi[3];  
    Dx= Xi[0]-Xi[1]+Xi[2]-Xi[3];
    Dy= Yi[0]-Yi[1]+Yi[2]-Yi[3];
   
    /*Initial values for csi and etha*/  
    XN[0]=0;
    XN[1]=0;
  
    res=1e10;
    
    while( res > tol ) {        
        M[0][0]=Bx+Dx*XN[1];
	M[0][1]=Cx+Dx*XN[0];
	M[1][0]=By+Dy*XN[1];
	M[1][1]=Cy+Dy*XN[0];
    
	F[0]=-Ax+Bx*XN[0]+Cx*XN[1]+Dx*XN[0]*XN[1];    
	F[1]=-Ay+By*XN[0]+Cy*XN[1]+Dy*XN[0]*XN[1];
    
	DXN[0]= - (F[0]*M[1][1]-F[1]*M[0][1]) /
                  (M[0][0]*M[1][1]-M[1][0]*M[0][1]);
	DXN[1]= - (F[1]*M[0][0]-F[0]*M[1][0]) /
	          (M[0][0]*M[1][1]-M[1][0]*M[0][1]);    

      //res=pow(F[0]*F[0]+F[1]*F[1],0.5);
        res=fabs(F[0])+fabs(F[1]);
   
	XN[0]=XN[0]+DXN[0];
	XN[1]=XN[1]+DXN[1];
    }
   
  //fprintf(stdout,"\n res=%e",res);
    domainCoords.x[0]=.5*(XN[0]+1)*domainlengthcsi;
    domainCoords.x[1]=.5*(XN[1]+1)*domainlengthetha;
  
    return domainCoords;  
}

int main(int argc, char **argv)
{
    char    *meshetree, *robIn, *robOut, *cmuOut, *diffile, *diffileA;
    etree_t *meshEp = NULL;
    double   east_m, north_m, depth_m, robVs, robVp, robRho, vsDiff;
    mdata_t  rawElem;
    int      rres=0, mres;//, i=0;

    FILE    *fpRobIn, *fpRobOut, *fpCmuOut, *fpDiff, *fpDiffA;

    double   lon, lat;
    double   domainLengthEta = 600e3;
    double   domainLengthCsi = 300e3;

    double   lonCorners[4] ={-121.0 ,-118.951292, -113.943965, -116.032285};
    double   latCorners[4] ={  34.5 ,  36.621696,   33.122341,   31.082920};

    vector2d_t meshCoords;

    domainLengthEta=600e3;
    domainLengthCsi=300e3;

    /* Capturing input data */

    if (argc != 7) {
        fprintf(stderr, 
            "\nusage: mirrorslice meshetree robfile cmuout robout diffile absfile\n");
        exit(1);
    }

    meshetree = argv[1];
    robIn     = argv[2];
    cmuOut    = argv[3];
    robOut    = argv[4];
    diffile   = argv[5];
    diffileA  = argv[6];

    /* Openning files */

    meshEp = etree_open(meshetree, O_RDONLY, CVMBUFFERSIZE, 0, 0);
    if (!meshEp) {
        fprintf(stderr, "Cannot open mesh etree %s\n", meshetree);
        exit(1);
    }

    fpRobIn = fopen(robIn,"r");
    if ( fpRobIn == NULL ) {
        fprintf(stderr, "Cannot open the Rob input file");
        exit(1);
    }

    fpRobOut = fopen(robOut,"w");
    if ( fpRobOut == NULL ) {
        fprintf(stderr, "Cannot open the Rob output file\n");
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

    while ( rres != EOF ) {
  //for ( i = 0; i < 10; i++ ) {

        rres = fscanf(fpRobIn,"%lf %lf %lf %lf %lf", 
		      &lon, &lat, &robVp, &robVs, &robRho);

	robVs = robVs * 1000;

	meshCoords = compute_domain_coords_bilin( 
		       lon, lat, 
                       lonCorners, latCorners,
                       domainLengthEta, domainLengthCsi);

	north_m = meshCoords.x[0];
	east_m  = meshCoords.x[1];
	depth_m = 0;

	mres = mesh_query(meshEp, east_m, north_m, depth_m, 
			  NULL, &rawElem.nid[0]);

	if (mres != 0) {
	    fprintf(stderr, "Cannot find the query point\n");
	    exit(1);
	}

	vsDiff = rawElem.Vs - robVs;

	fprintf(fpRobOut, "%12.0f %12.0f %12.4f\n",east_m, north_m, robVs);
	fprintf(fpCmuOut, "%12.0f %12.0f %12.4f\n",east_m, north_m, rawElem.Vs);
	fprintf(fpDiff,   "%12.0f %12.0f %12.4f\n",east_m, north_m, vsDiff);
	fprintf(fpDiffA,  "%12.0f %12.0f %12.4f\n",east_m, north_m, fabs(vsDiff));

    }
    
    /* Closing files */

    fclose(fpRobIn);
    fclose(fpRobOut);
    fclose(fpCmuOut);
    fclose(fpDiff);
    fclose(fpDiffA);
    etree_close(meshEp);

    return 0;
}
        

