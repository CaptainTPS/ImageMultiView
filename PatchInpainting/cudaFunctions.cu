#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "cudaFunctions.cuh"
#include <iostream>
#include "opencv2\opencv.hpp"
#include "opencv2\core.hpp"

__global__ void calSqDiffKernal(float* source, float* tmplate, float* result, float* tmplateMask, float* srcMask, int rlt_width, int rlt_height, int mw, int mh, int mchannel){
	int x_ = blockIdx.x * blockDim.x + threadIdx.x;
	int y_ = blockIdx.y * blockDim.y + threadIdx.y;

	if (x_ >= rlt_width || y_ >= rlt_height)
	{
		return;
	}

	int totalPixels = mw * mh * mchannel;
	int cnt = 0;
	float re = 0.0f;
	for (int tx = 0; tx < mw; tx++)
	{
		for (int ty = 0; ty < mh; ty++)
		{
			for (int channel = 0; channel < mchannel; channel++)
			{
				if (tmplateMask[ty * mw * mchannel + tx * mchannel + channel] != 0 && srcMask[(y_ + ty) * (rlt_width + mw - 1) *mchannel + (x_ + tx)*mchannel + channel] != 0)
				{
					float temp = tmplate[ty * mw * mchannel + tx * mchannel + channel] - source[(y_ + ty) * (rlt_width + mw - 1) * mchannel + (x_ + tx)*mchannel + channel];
					re += temp * temp;
					cnt++;
				}
			}

		}
	}
	if (cnt == 0)
	{
		re = 1.1f * totalPixels;//result above max to be 1
	}
	else
	{
		re = re / (float)cnt * totalPixels;
	}

	result[y_ * (rlt_width) + x_] = re;
}

__global__ void testKernal(float* source, float* tmplate, float* result, float* tmplateMask, float* srcMask, int rlt_width, int rlt_height, int mw, int mh, int mchannel){
	int x_ = blockIdx.x * blockDim.x + threadIdx.x;
	int y_ = blockIdx.y * blockDim.y + threadIdx.y;

	if (x_ >= rlt_width || y_ >= rlt_height)
	{
		return;
	}
	result[y_ * (rlt_width)+x_] = source[(y_) * (rlt_width + mw - 1) * mchannel + (x_ )*mchannel + 0];
}

void printDevProp(cudaDeviceProp devProp)
{
	printf("Major revision number:         %d\n", devProp.major);
	printf("Minor revision number:         %d\n", devProp.minor);
	printf("Name:                          %s\n", devProp.name);
	printf("Total global memory:           %lu\n", devProp.totalGlobalMem);
	printf("Total shared memory per block: %lu\n", devProp.sharedMemPerBlock);
	printf("Total registers per block:     %d\n", devProp.regsPerBlock);
	printf("Warp size:                     %d\n", devProp.warpSize);
	printf("Maximum memory pitch:          %lu\n", devProp.memPitch);
	printf("Maximum threads per block:     %d\n", devProp.maxThreadsPerBlock);
	for (int i = 0; i < 3; ++i)
		printf("Maximum dimension %d of block:  %d\n", i, devProp.maxThreadsDim[i]);
	for (int i = 0; i < 3; ++i)
		printf("Maximum dimension %d of grid:   %d\n", i, devProp.maxGridSize[i]);
	printf("Clock rate:                    %d\n", devProp.clockRate);
	printf("Total constant memory:         %lu\n", devProp.totalConstMem);
	printf("Texture alignment:             %lu\n", devProp.textureAlignment);
	printf("Concurrent copy and execution: %s\n", (devProp.deviceOverlap ? "Yes" : "No"));
	printf("Number of multiprocessors:     %d\n", devProp.multiProcessorCount);
	printf("Kernel execution timeout:      %s\n", (devProp.kernelExecTimeoutEnabled ? "Yes" : "No"));
	return;
}

void DIYmatchTemplate(const cv::Mat& source, const cv::Mat& tmplate, cv::Mat& result, const cv::Mat& tmplateMask, const cv::Mat& srcMask){
	//just use cv_tm_sqdiff method
	assert(source.size() == srcMask.size() && source.channels() == srcMask.channels());
	assert(tmplate.size() == tmplateMask.size() && tmplate.channels() == tmplateMask.channels());
	assert(tmplate.type() == source.type());
	
	cudaError_t cudaStatus;
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
		return;
	}
#if 0
	cudaDeviceProp devProp;
	cudaGetDeviceProperties(&devProp, 0);
	printDevProp(devProp);
#endif
	//copydata !the memory may not be continuous in Mat ! Mat MUST be row major!
	const uchar* ptr;

	float* src = nullptr;
	cudaStatus = cudaMalloc((void**)&src, source.rows * source.cols * source.channels() * sizeof(float));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	for (int i = 0; i < source.rows; i++)
	{
		ptr = source.ptr(i);
		float *src_ptr = src + (source.cols * source.channels()) * i;
		cudaStatus = cudaMemcpy(src_ptr, (void*)ptr, source.cols * source.channels() * sizeof(float), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess)
			fprintf(stderr, "cudaCopy failed!");
	}

	float* tmpl = nullptr;
	cudaStatus = cudaMalloc((void**)&tmpl, tmplate.rows * tmplate.cols * tmplate.channels() * sizeof(float));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	for (int i = 0; i < tmplate.rows; i++)
	{
		ptr = tmplate.ptr(i);
		float *tmpl_ptr = tmpl + (tmplate.cols * tmplate.channels()) * i;
		cudaStatus = cudaMemcpy(tmpl_ptr, (void*)ptr, tmplate.cols * tmplate.channels() * sizeof(float), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess)
			fprintf(stderr, "cudaCopy failed!");
	}

	float* rlt = nullptr;
	ptr = result.ptr(0);
	cudaStatus = cudaMalloc((void**)&rlt, result.rows * result.cols * result.channels() * sizeof(float));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	float* tmplM = nullptr;
	cudaStatus = cudaMalloc((void**)&tmplM, tmplateMask.rows * tmplateMask.cols * tmplateMask.channels() * sizeof(float));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	for (int i = 0; i < tmplateMask.rows; i++)
	{
		ptr = tmplateMask.ptr(i);
		float *tmplM_ptr = tmplM + (tmplateMask.cols * tmplateMask.channels()) * i;
		cudaStatus = cudaMemcpy(tmplM_ptr, (void*)ptr, tmplateMask.cols * tmplateMask.channels() * sizeof(float), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess)
			fprintf(stderr, "cudaCopy failed!");
	}

	float* srcM = nullptr;
	cudaStatus = cudaMalloc((void**)&srcM, srcMask.rows * srcMask.cols * srcMask.channels() * sizeof(float));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");
	for (int i = 0; i < srcMask.rows; i++)
	{
		ptr = srcMask.ptr(i);
		float *srcM_ptr = srcM + (srcMask.cols * srcMask.channels()) * i;
		cudaStatus = cudaMemcpy(srcM_ptr, (void*)ptr, srcMask.cols * srcMask.channels() * sizeof(float), cudaMemcpyHostToDevice);
		if (cudaStatus != cudaSuccess)
			fprintf(stderr, "cudaCopy failed!");
	}
	

	int width = result.cols;
	int height = result.rows;
	int maskWidth = tmplate.cols;
	int maskHeight = tmplate.rows;
	int maskCh = tmplate.channels();

	//run kernal
	dim3 blocks(result.cols / 16 + 1, result.rows / 16 + 1);
	dim3 threads(16, 16);
	calSqDiffKernal << <blocks, threads >> >(src, tmpl, rlt, tmplM, srcM, width, height, maskWidth, maskHeight, maskCh);
	//testKernal << <blocks, threads >> >(src, tmpl, rlt, tmplM, srcM, width, height, maskWidth, maskHeight, maskCh);

	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "Kernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
	}

	//copy back
	for (int i = 0; i < result.rows; i++)
	{
		ptr = result.ptr(i);
		float *rlt_ptr = rlt + (result.cols * result.channels()) * i;
		cudaStatus = cudaMemcpy((void*)ptr, rlt_ptr, result.cols * result.channels() * sizeof(float), cudaMemcpyDeviceToHost);
		if (cudaStatus != cudaSuccess)
			fprintf(stderr, "cudaCopyBack failed!");
	}
	cudaFree(src);
	cudaFree(tmpl);
	cudaFree(rlt);
	cudaFree(tmplM); 
	cudaFree(srcM);
}

__global__ void addKernel(int *c, const int *a, const int *b)
{
	int i = threadIdx.x;
	c[i] = a[i] + b[i];
	printf("%d", i);
}

void addVector(int *c, const int *b, const int *a, const int size){
	int *dev_a = 0;
	int *dev_b = 0;
	int *dev_c = 0;
	cudaError_t cudaStatus;

	// Choose which GPU to run on, change this on a multi-GPU system.  
	cudaStatus = cudaSetDevice(0);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaSetDevice failed!  Do you have a CUDA-capable GPU installed?");
	}

	// Allocate GPU buffers for three vectors (two input, one output)    .  
	cudaStatus = cudaMalloc((void**)&dev_c, size * sizeof(int));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	cudaStatus = cudaMalloc((void**)&dev_a, size * sizeof(int));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	cudaStatus = cudaMalloc((void**)&dev_b, size * sizeof(int));
	if (cudaStatus != cudaSuccess)
		fprintf(stderr, "cudaMalloc failed!");

	cudaStatus = cudaMemcpy(dev_a, a, size * sizeof(int), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
	}

	cudaStatus = cudaMemcpy(dev_b, b, size * sizeof(int), cudaMemcpyHostToDevice);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
	}

	addKernel <<<1, size >>>(dev_c, dev_a, dev_b);

	// Check for any errors launching the kernel  
	cudaStatus = cudaGetLastError();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "addKernel launch failed: %s\n", cudaGetErrorString(cudaStatus));
	}

	// cudaDeviceSynchronize waits for the kernel to finish, and returns  
	// any errors encountered during the launch.  
	cudaStatus = cudaDeviceSynchronize();
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaDeviceSynchronize returned error code %d after launching addKernel!\n", cudaStatus);
	}

	// Copy output vector from GPU buffer to host memory.  
	cudaStatus = cudaMemcpy(c, dev_c, size * sizeof(int), cudaMemcpyDeviceToHost);
	if (cudaStatus != cudaSuccess) {
		fprintf(stderr, "cudaMemcpy failed!");
	}


	cudaFree(dev_c);
	cudaFree(dev_a);
	cudaFree(dev_b);
}