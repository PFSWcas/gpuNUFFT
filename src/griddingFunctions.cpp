#include "griddingFunctions.hpp"

/*BEGIN Zwart*/
/**************************************************************************
 *  FROM GRID_UTILS.C
 *
 *  Author: Nick Zwart, Dallas Turley, Ken Johnson, Jim Pipe 
 *  Date: 2011 apr 11
 *  Rev: 2011 aug 21
 * ...
*/

/************************************************************************** KERNEL */

/* 
 *	Summary: Allocates the 3D spherically symmetric kaiser-bessel function 
 *	         for kernel table lookup.
 *  
 *	         This lookup table is with respect to the radius squared.
 *	         and is based on the work described in Beatty et al. MRM 24, 2005
 */
static float i0( float x )
{
	float ax = fabs(x);
	float ans;
	float y;

	if (ax < 3.75f) 
    {
		y=x/3.75f,y=y*y;
		ans=1.0f+y*(3.5156229f+y*(3.0899424f+y*(1.2067492f
			   +y*(0.2659732f+y*(0.360768e-1f+y*0.45813e-2f)))));
	} 
    else 
    {
		y=3.75f/ax;
		ans=(exp(ax)/sqrt(ax))*(0.39894228f+y*(0.1328592e-1f
				+y*(0.225319e-2f+y*(-0.157565e-2f+y*(0.916281e-2f
				+y*(-0.2057706e-1f+y*(0.2635537e-1f+y*(-0.1647633e-1f
				+y*0.392377e-2f))))))));
	}
	return (ans);
}


/* LOADGRID3KERNEL()
 * Loads a radius of the circularly symmetric kernel into a 1-D array, with
 * respect to the kernel radius squared.
 */
#define sqr(__se) ((__se)*(__se))
//#define BETA (M_PI*sqrt(sqr(DEFAULT_KERNEL_WIDTH/DEFAULT_OVERSAMPLING_RATIO*(DEFAULT_OVERSAMPLING_RATIO-0.5))-0.8))
//#define I0_BETA	(i0(BETA))
//#define kernel(__radius) (i0 (BETA * sqrt (1 - sqr(__radius))) / I0_BETA)

#define BETA(__kw,__osr) (M_PI*sqrt(sqr(__kw/__osr*(__osr-0.5f))-0.8f))
#define I0_BETA(__kw,__osr)	(i0(BETA(__kw,__osr)))
#define kernel(__radius,__kw,__osr) (i0 (BETA(__kw,__osr) * sqrt (1 - sqr(__radius))) / I0_BETA(__kw,__osr))

/*END Zwart*/

long calculateGrid3KernelSize()
{
	return calculateGrid3KernelSize(DEFAULT_OVERSAMPLING_RATIO,DEFAULT_KERNEL_RADIUS);
}

long calculateGrid3KernelSize(float osr, float kernel_radius)
{
	//calculate kernel density (per grid/kernel unit) 
	//nearest neighbor with maximum aliasing error
	//of 0.001
	
	long kernel_osf = (long)(floor(0.91f/(osr * MAXIMUM_ALIASING_ERROR)));

	float kernel_radius_osr = static_cast<float>(osr * kernel_radius);

    return (long)(kernel_osf * kernel_radius_osr);
}

void loadGrid3Kernel(float *kernTab)
{
	loadGrid3Kernel(kernTab,calculateGrid3KernelSize(),DEFAULT_KERNEL_WIDTH,DEFAULT_OVERSAMPLING_RATIO);
}

void loadGrid3Kernel(float *kernTab,long kernel_entries)
{
	loadGrid3Kernel(kernTab,kernel_entries,DEFAULT_KERNEL_WIDTH,DEFAULT_OVERSAMPLING_RATIO);
}

void loadGrid3Kernel(float *kernTab,long kernel_entries, int kernel_width, float osr)	
{
    /* check input data */
    assert( kernTab != NULL );
	long i;

	float rsqr = 0.0f;
    /* load table */
	for (i=1; i<kernel_entries-1; i++)	
    {
		rsqr = sqrt(i/(float)(kernel_entries-1));//*(i/(float)(size-1));
		kernTab[i] = static_cast<float>(kernel(rsqr,kernel_width,osr)); /* kernel table for radius squared */
		//assert(kernTab[i]!=kernTab[i]); //check is NaN
	}

    /* ensure center point is 1 */
    kernTab[0] = 1.0f;

    /* ensure last point is zero */
    kernTab[kernel_entries-1] = 0.0f;
} /* end loadGrid3Kernel() */


/* set bounds for current data point based on kernel and grid limits *
DEVICEHOST void set_minmax (double x, int *min, int *max, int maximum, double radius)	
{

}*/


/*DEVICEHOST  bool isOutlier(int x, int y, int z, int center_x, int center_y, int center_z, int width, int sector_offset)
{
	return ((center_x - sector_offset + x) >= width ||
						(center_x - sector_offset + x) < 0 ||
						(center_y - sector_offset + y) >= width ||
						(center_y - sector_offset + y) < 0 ||
						(center_z - sector_offset + z) >= width ||
						(center_z - sector_offset + z) < 0);
}*/

/*void gridding3D(float* data, float* crds, float* gdata, float* kernel, int* sectors, int sector_count, int* sector_centers, int sector_width, int kernel_width, int kernel_count, int width)
{
	int imin, imax, jmin, jmax, kmin, kmax, i, j, k, ind;
	float x, y, z, ix, jy, kz;

    /* kr 
	float dx_sqr, dy_sqr, dz_sqr, val;
	int center_x, center_y, center_z, max_x, max_y, max_z;
	
	float kernel_radius = static_cast<float>(kernel_width) / 2.0f;
	float radius = kernel_radius / static_cast<float>(width);

	printf("radius rel. to grid width %f\n",radius);
	float width_inv = 1.0f / width;
	float radiusSquared = radius * radius;
	float kernelRadius_invSqr = 1 / radiusSquared;

	float dist_multiplier = (kernel_count - 1) * kernelRadius_invSqr;
	//int sector_width = 10;
	
	
	//TODO passt das?
	int sector_pad_width = sector_width + 2*(int)(floor(kernel_width / 2.0f));
	int sector_dim = sector_pad_width  * sector_pad_width  * sector_pad_width ;
	int sector_offset = (int)(floor(sector_pad_width / 2.0f));

	printf("sector offset = %d",sector_offset);
	float** sdata =  (float**)malloc(sector_count*sizeof(float*));

	assert(sectors != NULL);

	for (int sec = 0; sec < sector_count; sec++)
	{
		sdata[sec] = (float *) calloc(sector_dim * 2, sizeof(float)); // 5*5*5 * 2
		assert(sdata[sec] != NULL);

		center_x = sector_centers[sec * 3];
		center_y = sector_centers[sec * 3 + 1];
		center_z = sector_centers[sec * 3 + 2];

		printf("\nhandling center (%d,%d,%d) in sector %d\n",center_x,center_y,center_z,sec);

		for (int data_cnt = sectors[sec]; data_cnt < sectors[sec+1];data_cnt++)
		{
			printf("handling %d data point = %f\n",data_cnt+1,data[data_cnt]);

			x = crds[3*data_cnt];
			y = crds[3*data_cnt +1];
			z = crds[3*data_cnt +2];
			printf("data k-space coords (%f, %f, %f)\n",x,y,z);
			
			max_x = sector_pad_width-1;
			max_y = sector_pad_width-1;
			max_z = sector_pad_width-1;

			/* set the boundaries of final dataset for gridding this point 
			ix = (x + 0.5f) * (width) - center_x + sector_offset;
			set_minmax(ix, &imin, &imax, max_x, kernel_radius);
			jy = (y + 0.5f) * (width) - center_y + sector_offset;
			set_minmax(jy, &jmin, &jmax, max_y, kernel_radius);
			kz = (z + 0.5f) * (width) - center_z + sector_offset;
			set_minmax(kz, &kmin, &kmax, max_z, kernel_radius);

			printf("sector grid position of data point: %f,%f,%f\n",ix,jy,kz);
			
			/* grid this point onto the neighboring cartesian points 
			for (k=kmin; k<=kmax; k++)	
			{
				kz = static_cast<float>((k + center_z - sector_offset)) / static_cast<float>((width)) - 0.5f;//(k - center_z) *width_inv;
				dz_sqr = kz - z;
				dz_sqr *= dz_sqr;
				if (dz_sqr < radiusSquared)
				{
					for (j=jmin; j<=jmax; j++)	
					{
						jy = static_cast<float>(j + center_y - sector_offset) / static_cast<float>((width)) - 0.5f;   //(j - center_y) *width_inv;
						dy_sqr = jy - y;
						dy_sqr *= dy_sqr;
						if (dy_sqr < radiusSquared)	
						{
							for (i=imin; i<=imax; i++)	
							{
								ix = static_cast<float>(i + center_x - sector_offset) / static_cast<float>((width)) - 0.5f;// (i - center_x) *width_inv;
								dx_sqr = ix - x;
								dx_sqr *= dx_sqr;
								if (dx_sqr < radiusSquared)	
								{
									/* get kernel value 
									//Berechnung mit Separable Filters 
									val = kernel[(int) round(dz_sqr * dist_multiplier)] *
										  kernel[(int) round(dy_sqr * dist_multiplier)] *
										  kernel[(int) round(dx_sqr * dist_multiplier)];
									ind = getIndex(i,j,k,sector_pad_width);
								
									/* multiply data by current kernel val 
									/* grid complex or scalar 
									sdata[sec][2*ind] += val * data[2*data_cnt];
									sdata[sec][2*ind+1] += val * data[2*data_cnt+1];
								} /* kernel bounds check x, spherical support 
							} /* x 	 
						} /* kernel bounds check y, spherical support 
					} /* y 
				} /*kernel bounds check z 
			} /* z 
		} /*data points per sector
	
	}/*sectors
	
	//TODO copy data from sectors to original grid
	int max_im_index = width;
	for (int sec = 0; sec < sector_count; sec++)
	{
		printf("DEBUG: showing entries of sector %d in z = 5 plane...\n",sec);
		center_x = sector_centers[sec * 3];
		center_y = sector_centers[sec * 3 + 1];
		center_z = sector_centers[sec * 3 + 2];
		
		int sector_ind_offset = getIndex(center_x - sector_offset,center_y - sector_offset,center_z - sector_offset,width);

		printf("sector index offset in resulting grid: %d\n", sector_ind_offset);
		for (int z = 0; z < sector_pad_width; z++)
			for (int y = 0; y < sector_pad_width; y++)
			{
				for (int x = 0; x < sector_pad_width; x++)
				{
					int s_ind = 2*getIndex(x,y,z,sector_pad_width) ;
					ind = 2*(sector_ind_offset + getIndex(x,y,z,width));
					//if (z==3)
					//	printf("%.4f ",sdata[sec][s_ind]);
					//TODO auslagern
					if (isOutlier(x,y,z,center_x,center_y,center_z,width,sector_offset))
						continue;
					
					gdata[ind] += sdata[sec][s_ind]; //Re
					gdata[ind+1] += sdata[sec][s_ind+1];//Im
				}
				//if (z==3) printf("\n");
			}
			//printf("----------------------------------------------------\n");
		free(sdata[sec]);
	}
	free(sdata);
}*/
