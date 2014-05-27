#include "balanced_texture_gpuNUFFT_operator.hpp"

gpuNUFFT::GpuNUFFTInfo* gpuNUFFT::BalancedTextureGpuNUFFTOperator::initAndCopyGpuNUFFTInfo()
{
  gpuNUFFT::GpuNUFFTInfo* gi_host = initGpuNUFFTInfo();

  gi_host->sectorsToProcess = sectorProcessingOrder.count();
  gi_host->interpolationType = interpolationType;

  if (DEBUG)
    printf("copy GpuNUFFT Info to symbol memory... size = %ld \n",sizeof(gpuNUFFT::GpuNUFFTInfo));

  initConstSymbol("GI",gi_host,sizeof(gpuNUFFT::GpuNUFFTInfo));

  if (DEBUG)
    printf("...done!\n");
  return gi_host;
}


void gpuNUFFT::BalancedTextureGpuNUFFTOperator::adjConvolution(DType2* data_d, 
      DType* crds_d, 
      CufftType* gdata_d,
      DType* kernel_d, 
      IndType* sectors_d, 
      IndType* sector_centers_d,
  gpuNUFFT::GpuNUFFTInfo* gi_host)
{
  bindTo1DTexture("texDATA",data_d,this->kSpaceTraj.count());

  //call balanced texture kernel
  performTextureConvolution(data_d,crds_d,gdata_d,kernel_d,sectors_d,sector_processing_order_d,sector_centers_d,gi_host);

  unbindTexture("texDATA");
}

void gpuNUFFT::BalancedTextureGpuNUFFTOperator::forwardConvolution(CufftType*		data_d, 
  DType*			crds_d, 
  CufftType*		gdata_d,
  DType*			kernel_d, 
  IndType*		sectors_d, 
  IndType*		sector_centers_d,
  gpuNUFFT::GpuNUFFTInfo* gi_host)
{
  bindTo1DTexture("texGDATA",gdata_d,gi_host->grid_width_dim);

  //call balanced texture kernel
  performTextureForwardConvolution(data_d,crds_d,gdata_d,kernel_d,sectors_d,sector_processing_order_d,sector_centers_d,gi_host);

  unbindTexture("texGDATA");
}


// Adds behaviour of GpuNUFFTOperator by 
// adding a sector processing order 
void gpuNUFFT::BalancedTextureGpuNUFFTOperator::performGpuNUFFTAdj(gpuNUFFT::Array<DType2> kspaceData, gpuNUFFT::Array<CufftType>& imgData, GpuNUFFTOutput gpuNUFFTOut)
{
  if (DEBUG)
    printf("allocate and copy sector processing order of size %d...\n",this->sectorProcessingOrder.count());
  allocateAndCopyToDeviceMem<IndType2>(&sector_processing_order_d,this->sectorProcessingOrder.data,this->sectorProcessingOrder.count());

  TextureGpuNUFFTOperator::performGpuNUFFTAdj(kspaceData,imgData,gpuNUFFTOut);

  freeTotalDeviceMemory(sector_processing_order_d,NULL);//NULL as stop token
}

void gpuNUFFT::BalancedTextureGpuNUFFTOperator::performForwardGpuNUFFT(gpuNUFFT::Array<DType2> imgData,gpuNUFFT::Array<CufftType>& kspaceData, GpuNUFFTOutput gpuNUFFTOut)
{
  if (DEBUG)
    printf("allocate and copy sector processing order of size %d...\n",this->sectorProcessingOrder.count());
  allocateAndCopyToDeviceMem<IndType2>(&sector_processing_order_d,this->sectorProcessingOrder.data,this->sectorProcessingOrder.count());
  
  TextureGpuNUFFTOperator::performForwardGpuNUFFT(imgData,kspaceData,gpuNUFFTOut);

  freeTotalDeviceMemory(sector_processing_order_d,NULL);//NULL as stop token
}
