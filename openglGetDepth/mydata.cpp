#include "mydata.h"
#define _USE_MATH_DEFINES
#include "math.h"

cameraPara::cameraPara(){
#if 0
	//camera parameters
	angleOfView = 54.43;//seems horizontal
	focalLength = 35.0;
	nearPlane = 0.1;
	farPlane = 2000.0;
	width = 1280;
	height = 720;
#endif
	//for kinect data
	angleOfView = 31.3633 * 2;//angle not radian
	focalLength = 35.0;
	nearPlane = 0.1;
	farPlane = 3000.0;
	width = 640;
	height = 480;

	screenRatio = 1.0* width / height;
	//cal vertical fov
	angleOfView_vertical =atan( tan(angleOfView / 2 * M_PI / 180.0) / screenRatio) / M_PI * 180.0 * 2;

	//half of the image plane
	y_length = focalLength * tan(angleOfView / 2 * M_PI / 180.0);
	x_length = y_length * screenRatio;

	depth_near = 0;
	depth_far = 1;

	//translate[0] = 869.178;
	//translate[1] = 1005.695;
	//translate[2] = 2164.855;
	//rotation[0] = -21.338;//x
	//rotation[1] = 20.20;//y
	//rotation[2] = 0.000;//z

	translate[0] = 0.000;
	translate[1] = 0.000;
	translate[2] = 20.000;
	rotation[0] = 0.000;//x
	rotation[1] = 0.000;//y
	rotation[2] = 0.000;//z

	//for generate new view
	baseDistance = 10.0;//this value is usually useless
	f_ratio = 0.5;
	bd_ratio = 0.05;
}