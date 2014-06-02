#ifndef GPUNUFFT_OPERATOR_H_INCLUDED
#define GPUNUFFT_OPERATOR_H_INCLUDED

#include "gpuNUFFT_types.hpp"
#include "gpuNUFFT_kernels.hpp"
#include "config.hpp"
#include <cstdlib>
#include <iostream>

namespace gpuNUFFT
{
  class GpuNUFFTOperator 
  {
  public:

    GpuNUFFTOperator(IndType kernelWidth, IndType sectorWidth, DType osf, Dimensions imgDims, bool loadKernel = true, OperatorType operatorType = DEFAULT): 
        osf(osf), kernelWidth(kernelWidth), sectorWidth(sectorWidth),imgDims(imgDims),operatorType(operatorType),gpuMemAllocated(false)
        {
          if (loadKernel)
            initKernel();	

          sectorDims.width = sectorWidth;
          sectorDims.height = sectorWidth;
          if (imgDims.depth > 0)
            sectorDims.depth = sectorWidth;
        }

        ~GpuNUFFTOperator()
        {
          free(this->kernel.data);
        }

        friend class GpuNUFFTOperatorFactory;

        // SETTER 
        void setOsf(DType osf)			{this->osf = osf;}

        void setKSpaceTraj(Array<DType> kSpaceTraj)				{this->kSpaceTraj = kSpaceTraj;}
        void setSectorCenters(Array<IndType> sectorCenters)		{this->sectorCenters = sectorCenters;}
        void setSectorDataCount(Array<IndType> sectorDataCount)	{this->sectorDataCount = sectorDataCount;}
        void setDataIndices(Array<IndType> dataIndices)			{this->dataIndices = dataIndices;}
        void setSens(Array<DType2> sens)						{this->sens = sens;}
        void setDens(Array<DType> dens)							{this->dens = dens;}

        void setImageDims(Dimensions dims)						{this->imgDims = dims;}
        void setGridSectorDims(Dimensions dims)						{this->gridSectorDims = dims;}

        // GETTER
        Array<DType>  getKSpaceTraj()	{return this->kSpaceTraj;}

        Array<DType2>	getSens()			{return this->sens;}
        Array<DType>	getDens()			{return this->dens;}
        Array<DType>    getKernel()			{return this->kernel;}
        Array<IndType>  getSectorDataCount(){return this->sectorDataCount;}

        IndType getKernelWidth()		{return this->kernelWidth;}
        IndType getSectorWidth()		{return this->sectorWidth;}

        Dimensions getImageDims() {return this->imgDims;}
        Dimensions getGridDims() {return this->imgDims * osf;}

        Dimensions getGridSectorDims() {return this->gridSectorDims;}
        Dimensions getSectorDims() {return this->sectorDims;}

        Array<IndType> getSectorCenters() {return this->sectorCenters;}
        IndType* getSectorCentersData() {return reinterpret_cast<IndType*>(this->sectorCenters.data);}

        Array<IndType>  getDataIndices()		{return this->dataIndices;}

        bool is2DProcessing() {return this->imgDims.depth == 0;}
        bool is3DProcessing() {return !is2DProcessing();}

        int getImageDimensionCount() {return (is2DProcessing() ? 2 : 3);}

        // OPERATIONS

        //adjoint gpuNUFFT
        Array<CufftType> performGpuNUFFTAdj(Array<DType2> kspaceData);
        virtual void     performGpuNUFFTAdj(Array<DType2> kspaceData, Array<CufftType>& imgData, GpuNUFFTOutput gpuNUFFTOut = DEAPODIZATION);
        Array<CufftType> performGpuNUFFTAdj(Array<DType2> kspaceData, GpuNUFFTOutput gpuNUFFTOut);

        //forward gpuNUFFT
        Array<CufftType> performForwardGpuNUFFT(Array<DType2> imgData);
        virtual void     performForwardGpuNUFFT(Array<DType2> imgData,Array<CufftType>& kspaceData, GpuNUFFTOutput gpuNUFFTOut = DEAPODIZATION);
        Array<CufftType> performForwardGpuNUFFT(Array<DType2> imgData,GpuNUFFTOutput gpuNUFFTOut);

        bool applyDensComp(){return (this->dens.data != NULL && this->dens.count()>1);}

        bool applySensData(){return (this->sens.data != NULL && this->sens.count()>1);}

        virtual OperatorType getType() {return operatorType;}
  protected:
    OperatorType operatorType;

    IndType getGridWidth() {return (IndType)(this->getGridDims().width);}

    Array<DType> kernel;

    // simple array
    // dimensions: n dimensions * dataCount
    Array<DType> kSpaceTraj;

    // complex array
    // dimensions: imgDim * chnCount
    Array<DType2> sens;

    // density compensation
    // dimensions: dataCount
    Array<DType> dens;

    // sector centers
    Array<IndType> sectorCenters;

    // dataCount per sector
    Array<IndType> sectorDataCount;

    // assignment of data index to according sector
    Array<IndType> dataIndices;

    // oversampling factor
    DType osf;

    // width of kernel in grid units
    IndType kernelWidth;

    // sector size in grid units
    IndType sectorWidth;

    Dimensions imgDims;

    // amount of sectors per grid direction
    Dimensions gridSectorDims;

    // size of one sector in grid
    Dimensions sectorDims;

    template <typename T>
    T* selectOrdered(Array<T>& dataArray, int offset=0);

    template <typename T>
    void writeOrdered(Array<T>& destArray, T* sortedArray, int offset=0);

    virtual void initKernel();

    GpuNUFFTInfo* initGpuNUFFTInfo();
    virtual GpuNUFFTInfo* initAndCopyGpuNUFFTInfo();

    virtual void adjConvolution(DType2* data_d, 
      DType* crds_d, 
      CufftType* gdata_d,
      DType* kernel_d, 
      IndType* sectors_d,
      IndType* sector_centers_d,
      gpuNUFFT::GpuNUFFTInfo* gi_host);

    virtual void forwardConvolution(CufftType*		data_d, 
      DType*			crds_d, 
      CufftType*		gdata_d,
      DType*			kernel_d, 
      IndType*		sectors_d, 
      IndType*		sector_centers_d,
      gpuNUFFT::GpuNUFFTInfo* gi_host);
    virtual void initLookupTable();
    virtual void freeLookupTable();

    GpuNUFFTInfo* gi_host;

    //GPU Device Members
    bool gpuMemAllocated;

    DType2* sens_d;
    DType* crds_d;
    DType* density_comp_d;
    DType* deapo_d;
    CufftType *gdata_d;
    IndType* sector_centers_d;
    IndType* sectors_d;
    IndType* data_indices_d;
    DType2* data_sorted_d;
    
    cufftHandle fft_plan;

    //for timing tests
    
    cudaEvent_t start, stop;
    void startTiming();
    float stopTiming();

    //
    void initDeviceMemory(int n_coils);
    void freeDeviceMemory(int n_coils);

  };

}

#endif //GPUNUFFT_OPERATOR_H_INCLUDED