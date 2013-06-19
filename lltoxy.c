
/*
 * bilinear2d.c: Computes the transformations in a bilinear element
 *
 * Copyright (c) 2005 Leonardo Ramirez-Guzman
 *
 *
 * All rights reserved.  May not be used, modified, or copied
 * without permission.
 *
 * Contact:
 * Leonardo Ramirez-Guzman
 * CEE, Carnegie Mellon University
 * 5000 Forbes Avenue
 * Pittsburgh, PA 15213
 * lramirez@andrew.cmu.edu
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>


/* 
 *  vector 2d struct
 *
 */
typedef struct vector2d_t{
  
  double x [ 2 ];
  
}vector2d_t;


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
vector2d_t compute_domain_coords_bilin(double lon ,             double lat, 
                       double *longcorner,      double *latcorner,
                       double domainlengthetha, double domainlengthcsi){

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
    
  while( res > tol ){        
    M[0][0]=Bx+Dx*XN[1];
    M[0][1]=Cx+Dx*XN[0];
    M[1][0]=By+Dy*XN[1];
    M[1][1]=Cy+Dy*XN[0];
    
    F[0]=-Ax+Bx*XN[0]+Cx*XN[1]+Dx*XN[0]*XN[1];    
    F[1]=-Ay+By*XN[0]+Cy*XN[1]+Dy*XN[0]*XN[1];
    
    DXN[0]=-(F[0]*M[1][1]-F[1]*M[0][1])/
            (M[0][0]*M[1][1]-M[1][0]*M[0][1]);
    DXN[1]=-(F[1]*M[0][0]-F[0]*M[1][0])/
            (M[0][0]*M[1][1]-M[1][0]*M[0][1]);    

    //res=pow(F[0]*F[0]+F[1]*F[1],0.5);
    res=fabs(F[0])+fabs(F[1]);
    
    XN[0]=XN[0]+DXN[0];
    XN[1]=XN[1]+DXN[1];
  }
   
  fprintf(stdout,"\n res=%e",res);
  domainCoords.x[0]=.5*(XN[0]+1)*domainlengthcsi;
  domainCoords.x[1]=.5*(XN[1]+1)*domainlengthetha;
  
  return domainCoords;  
}


/* Example for the Terashake domain */

int main(int argc, char **argv){   
    
    double lon,lat, domainLengthEta,domainLengthCsi;
    double lonCorners[4] ={-121.0 ,-118.951292, -113.943965, -116.032285};
    double latCorners[4] ={  34.5 ,  36.621696,   33.122341,   31.082920};
    vector2d_t meshCoords;

    if(argc!=3) {
        fprintf(stderr,"Usage: %s <longitude> <latitude> \n\n",argv[0]);
        return 1;
    }

    // Use arguments if specified
    if(argc==3){
        if(sscanf(argv[1],"%lf",&lon)!=1) {
            fprintf(stderr,"Invalid x coordinate: %s\n",argv[1]);
            return 1;
        }

        if(sscanf(argv[2],"%lf",&lat)!=1) {
            fprintf(stderr,"Invalid y coordinate: %s\n",argv[2]);
            return 1;
        }
    }

    domainLengthEta=600e3;
    domainLengthCsi=300e3;

    meshCoords=compute_domain_coords_bilin( lon, lat, 
                            lonCorners,      latCorners,
                            domainLengthEta, domainLengthCsi);

    fprintf(stdout,"\n lon=%f lat=%f X=%f Y=%f\n", lon, lat, meshCoords.x[0], meshCoords.x[1]);

    return 0;
}
