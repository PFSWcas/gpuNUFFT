#include <limits.h>

#include "gtest/gtest.h"

#include "gridding_operator_factory.hpp"

#define epsilon 0.0001f

#define get3DC2lin(_x,_y,_z,_width) ((_x) + (_width) * ( (_y) + (_z) * (_width)))

TEST(TestGPUGriddingForwardConv,KernelCall1Sector)
{
	int kernel_width = 5;

	//oversampling ratio
	float osr = 1.25f;

	long kernel_entries = calculateGrid3KernelSize(osr,kernel_width/2.0f);
	DType *kern = (DType*) calloc(kernel_entries,sizeof(DType));
	loadGrid3Kernel(kern,kernel_entries);

	//Image
	int im_width = 32;

	//Data
	int data_entries = 1;

	//Coords
	//Scaled between -0.5 and 0.5
	//in triplets (x,y,z)
    DType* coords = (DType*) calloc(3*data_entries,sizeof(DType));//3* x,y,z
	coords[0] = -0.31719f; //should result in 7,7,7 center
	coords[1] = -0.38650f;
	coords[2] = 0;

	DType2* im_data;
	unsigned long dims_g[4];
    dims_g[0] = 2; /* complex */
	dims_g[1] = (unsigned long)(im_width); 
    dims_g[2] = (unsigned long)(im_width);
    dims_g[3] = (unsigned long)(im_width);

	long im_size = dims_g[1]*dims_g[2]*dims_g[3];

	im_data = (DType2*) calloc(im_size,sizeof(DType2));
	
	for (int x=0;x<im_size;x++)
	{
		im_data[x].x = 1.0f;
		im_data[x].y = 1.0f;
	}
	
	long grid_width = (unsigned long)(im_width * osr);

	//sectors of data, count and start indices
	int sector_width = 8;
	
	const int sector_count = 125;
	int* sectors = (int*) calloc(2*sector_count,sizeof(int));
	sectors[0]=0;
	sectors[1]=0;
	sectors[2]=1;

	/*int* sector_centers = (int*) calloc(3*sector_count,sizeof(int));
	sector_centers[0] = 5;
	sector_centers[1] = 5;
	sector_centers[2] = 5;*/
	int sector_centers[3 * sector_count] = {4,4,4,4,4,12,4,4,20,4,4,28,4,4,36,4,12,4,4,12,12,4,12,20,4,12,28,4,12,36,4,20,4,4,20,12,4,20,20,4,20,28,4,20,36,4,28,4,4,28,12,4,28,20,4,28,28,4,28,36,4,36,4,4,36,12,4,36,20,4,36,28,4,36,36,12,4,4,12,4,12,12,4,20,12,4,28,12,4,36,12,12,4,12,12,12,12,12,20,12,12,28,12,12,36,12,20,4,12,20,12,12,20,20,12,20,28,12,20,36,12,28,4,12,28,12,12,28,20,12,28,28,12,28,36,12,36,4,12,36,12,12,36,20,12,36,28,12,36,36,20,4,4,20,4,12,20,4,20,20,4,28,20,4,36,20,12,4,20,12,12,20,12,20,20,12,28,20,12,36,20,20,4,20,20,12,20,20,20,20,20,28,20,20,36,20,28,4,20,28,12,20,28,20,20,28,28,20,28,36,20,36,4,20,36,12,20,36,20,20,36,28,20,36,36,28,4,4,28,4,12,28,4,20,28,4,28,28,4,36,28,12,4,28,12,12,28,12,20,28,12,28,28,12,36,28,20,4,28,20,12,28,20,20,28,20,28,28,20,36,28,28,4,28,28,12,28,28,20,28,28,28,28,28,36,28,36,4,28,36,12,28,36,20,28,36,28,28,36,36,36,4,4,36,4,12,36,4,20,36,4,28,36,4,36,36,12,4,36,12,12,36,12,20,36,12,28,36,12,36,36,20,4,36,20,12,36,20,20,36,20,28,36,20,36,36,28,4,36,28,12,36,28,20,36,28,28,36,28,36,36,36,4,36,36,12,36,36,20,36,36,28,36,36,36};

    GriddingND::Array<DType> kSpaceData;
    kSpaceData.data = coords;
    kSpaceData.dim.length = data_entries;

	GriddingND::Array<CufftType> dataArray;

	GriddingND::Array<DType2> im_dataArray;
	im_dataArray.data = im_data;
	im_dataArray.dim.width = im_width;
	im_dataArray.dim.height = im_width;
	im_dataArray.dim.depth = im_width;

	GriddingND::Array<IndType> sectorsArray;
	sectorsArray.data = (IndType*)sectors;
	sectorsArray.dim.length = sector_count;
	GriddingND::Array<IndType> sectorCentersArray;
	sectorCentersArray.data = (IndType*)sector_centers;

	GriddingND::Dimensions imgDims;
	imgDims.width = im_width;
	imgDims.height = im_width;
	imgDims.depth = im_width;

    //GriddingND::GriddingOperator *griddingOp = new GriddingND::GriddingOperator(kernel_width,sector_width,osr);
    GriddingND::GriddingOperator *griddingOp = GriddingND::GriddingOperatorFactory::getInstance().createGriddingOperator(kSpaceData,kernel_width,sector_width,osr,imgDims);

	dataArray = griddingOp->performForwardGridding(im_dataArray);

	for (int j=0; j<dataArray.count(); j++)
	{
		printf("%.4f %.4f \n",dataArray.data[j].x,dataArray.data[j].y);
	}
	EXPECT_NEAR(0.158092f, dataArray.data[0].x,epsilon);

	printf("\n");

	free(coords);
	free(im_data);
	free(kern);
	free(sectors);
	//free(sector_centers);
	delete griddingOp;
}

TEST(TestGPUGriddingForwardConv,KernelCall1Sector2Channels)
{
	int kernel_width = 5;
	float osr = 1.25f;
	int im_width = 32;

	//Data
	int data_entries = 1;

	//Coords
	//Scaled between -0.5 and 0.5
	//in triplets (x,y,z)
    DType* coords = (DType*) calloc(3*data_entries,sizeof(DType));//3* x,y,z
	coords[0] = -0.31719f; //should result in 7,7,7 center
	coords[1] = -0.38650f;
	coords[2] = 0;

	GriddingND::Array<DType2> im_dataArray;
	im_dataArray.data = NULL; 
	im_dataArray.dim.width = im_width;
	im_dataArray.dim.height = im_width;
	im_dataArray.dim.depth = im_width;
	im_dataArray.dim.channels = 2;

	im_dataArray.data = (DType2*) calloc(im_dataArray.count(),sizeof(DType2));
	
	for (int x=0;x<im_dataArray.count();x++)
	{
		im_dataArray.data[x].x = 1.0f;
		im_dataArray.data[x].y = 1.0f;
	}
	
	//sectors of data, count and start indices
	int sector_width = 8;

    GriddingND::Array<DType> kSpaceData;
    kSpaceData.data = coords;
    kSpaceData.dim.length = data_entries;

	GriddingND::Array<CufftType> dataArray;

	//imgDims -> no Channel size here
	GriddingND::Dimensions imgDims;
	imgDims.width = im_dataArray.dim.width;
    imgDims.height = im_dataArray.dim.height;
	imgDims.depth  = im_dataArray.dim.depth;

    GriddingND::GriddingOperator *griddingOp = GriddingND::GriddingOperatorFactory::getInstance().createGriddingOperator(kSpaceData,kernel_width,sector_width,osr,imgDims);

	dataArray = griddingOp->performForwardGridding(im_dataArray);

	std::cout << "result count : " << dataArray.count() << std::endl;

	for (int j=0; j<dataArray.count(); j++)
	{
		printf("%.4f %.4f \n",dataArray.data[j].x,dataArray.data[j].y);
	}
	EXPECT_NEAR(0.158092f, dataArray.data[0].x,epsilon);
	EXPECT_EQ(dataArray.data[1].x, dataArray.data[0].x);
	EXPECT_EQ(dataArray.data[1].y, dataArray.data[0].y);

	printf("\n");

	free(coords);
	free(im_dataArray.data);
	delete griddingOp;
}

