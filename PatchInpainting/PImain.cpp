//
//  main.cpp
//  An example main function showcasing how to use the inpainting function.
//
//  Created by Sooham Rafiz on 2016-05-16.

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"

#include <iostream>
#include <string>

#include "PatchInpainting.h"
#include "PIheader.h"

#include "cudaFunctions.cuh"

/*
* Note: This program uses C assert() statements, define NDEBUG marco to
* disable assertions.
*/

#ifndef DEBUG
#define DEBUG 0
#endif

int main1(int argc, char** argv) {
	// --------------- read filename strings ------------------
	std::string colorFilename, maskFilename, depthf;

	colorFilename = "D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_c2.png";
	maskFilename = "D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_mask.png";
	depthf = "D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_depth.png";
	PatchInpaint pi;
	pi.mainLoop(colorFilename, maskFilename, depthf);

	return 0;
}

int main(){
	//test for cuda
#if 0
	int a[4] = { 1, 2, 3, 4 };
	int b[4] = { 1, 2, 3, 4 };
	int c[4];

	addVector(c, b, a, 4);
#endif
#if 1
	int boarder = 4;

	cv::Mat src = cv::imread("D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\3.jpg");
	cv::Mat srcMask(src.size(), CV_32FC3, cv::Scalar(1,1,1));
	src.convertTo(src, CV_32FC3);
	src /= 255.0f;

	cv::Mat tmpl = src.colRange(0, boarder).rowRange(0, boarder);
	//cv::Mat tmplMask = cv::Mat::ones(tmpl.size(), CV_32FC3);
	cv::Mat tmplMask(tmpl.size(), CV_32FC3, cv::Scalar(1, 1, 1));
	cv::Mat result(src.rows - boarder + 1, src.cols - boarder + 1, CV_32FC1);
#else
	int boarder = 2;
	float data[] = {
		1,0,1,1,
		1,0,1,1,
		1,2,1,3,
		1,2,1,3
	};
	unsigned char maskdata[] = {
		1, 0, 1, 0,
		1, 0, 1, 0,
		1, 0, 1, 0,
		1, 0, 1, 0
	};
	cv::Mat src(4, 4, CV_32F, data);
	cv::Mat srcMask = cv::Mat::ones(src.size(), CV_8UC1);

	cv::Mat tmpl = src.colRange(0, boarder).rowRange(0, boarder);
	cv::Mat tmplMask = cv::Mat::ones(tmpl.size(), CV_8UC1);

	std::cout << tmpl << std::endl;
	std::cout << tmplMask << std::endl;

	cv::Mat result(src.rows - boarder + 1, src.cols - boarder + 1, CV_32FC1);
#endif
	DIYmatchTemplate(src, tmpl, result, tmplMask, srcMask);
	cv::Mat result2(src.rows - boarder + 1, src.cols - boarder + 1, CV_32FC1, 0.0f);
	cv::matchTemplate(src,
		tmpl,
		result2,
		CV_TM_SQDIFF,
		tmplMask
		);
	cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);
	return 0;
}