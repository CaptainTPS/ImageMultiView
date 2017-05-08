#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include "opencv2\opencv.hpp"

void DIYmatchTemplate(const cv::Mat& source, const cv::Mat& tmplate, cv::Mat& result, const cv::Mat& tmplateMask, const cv::Mat& srcMask);

void addVector(int *c, const int *b, const int *a, const int size); 