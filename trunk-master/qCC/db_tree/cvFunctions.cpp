#include <cstdlib>
#include <iostream>
#include <vector>

#include "opencv2\opencv.hpp"
#include "cvFunctions.h"

using namespace cv;
using std::cout;

cvFunctions::cvFunctions()
{
}

cvFunctions::~cvFunctions()
{
}

void asymmetricSmoothing(Mat &src, Mat &dst, int x, int y){
	//size:: (2x+1, 2y+1)
	Mat kernelX = getGaussianKernel(x * 2 + 1, -1);
	Mat kernelY = getGaussianKernel(y * 2 + 1, -1);
	Mat kd = kernelY * kernelX.t();

	filter2D(src, dst, -1, kd, Point(x, y));

}

void cvFunctions::filterDepth(unsigned char* d, int width, int height, int channel){
	assert(channel == 1);
	Mat depth(height, width, CV_8UC1, d);
	Mat dst;

#if 0
	bilateralFilter(depth, dst, 9, 50, 50);
#elif 1
	asymmetricSmoothing(depth, dst, 15, 60);
#endif

	unsigned char* ptr = dst.data;
	for (int i = 0; i < dst.cols * dst.rows * dst.channels(); i++)
	{
		d[i] = ptr[i];
	}
}

void cvFunctions::seeImage(unsigned char* d, int width, int height, int channel){
	assert(channel == 1 || channel == 3);
	Mat img;
	if (channel == 1)
	{
		img = Mat(height, width, CV_8UC1, d);
	}
	else if (channel == 3)
	{
		img = Mat(height, width, CV_8UC3, d);
	}

	imshow("image", img);
	waitKey();
}