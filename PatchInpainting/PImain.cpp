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

/*
* Note: This program uses C assert() statements, define NDEBUG marco to
* disable assertions.
*/

#ifndef DEBUG
#define DEBUG 0
#endif

int main(int argc, char** argv) {
	// --------------- read filename strings ------------------
	std::string colorFilename, maskFilename, depthf;

	colorFilename = "D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_c2.png";
	maskFilename = "D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_mask.png";
	depthf = "D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_depth.png";
	PatchInpaint pi;
	pi.mainLoop(colorFilename, maskFilename, depthf);

	return 0;
}