#include <windows.h>

#include <cstdlib>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include "OpenNI.h"

#include "NuiApi.h"
#include "NuiImageCamera.h"
#include "NuiSensor.h"

#include "opencv2\opencv.hpp"
#include "opencv2\imgproc\imgproc.hpp"

#include "math.h"

#include "D:\captainT\project_13\ImageMultiView\openglGetDepth\mydata.h"

#define WIDTH 640
#define HEIGHT 480

//! Pi
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

// Kinect variables
HANDLE rgbStream;              // The identifier of the Kinect's RGB Camera
HANDLE depthStream;
INuiSensor* sensor;            // The kinect sensor

//struct color_rgb{
//	int r, g, b;
//	color_rgb(int _a, int _b, int _c){
//		r = _a;
//		g = _b;
//		b = _c;
//	}
//
//	color_rgb(){
//		r = g = b = 0;
//	}
//};
//
//template<class T>
//struct OneImg
//{
//	T data[HEIGHT][WIDTH];
//};

void readImgOut(string file, string pathout){
	openni::Device device_;
	openni::VideoStream color_;
	openni::VideoFrameRef colorf_;

	openni::OpenNI::initialize();
	const char *cstr = file.c_str();
	device_.open(cstr);
	device_.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	color_.create(device_, openni::SENSOR_COLOR);
	color_.start();
	color_.readFrame(&colorf_);

	int w = colorf_.getWidth();
	int h = colorf_.getHeight();
	int num = colorf_.getFrameIndex();
	const openni::RGB888Pixel *cdata = (openni::RGB888Pixel *)colorf_.getData();
	cv::Mat colorImg(h, w, CV_8UC3, (void*)cdata);
	cv::cvtColor(colorImg, colorImg, CV_BGR2RGB);
	stringstream ss;
	ss << num << ".png";
	pathout += ss.str();
	bool flag = cv::imwrite(pathout, colorImg);
	cv::imshow("color", colorImg);
	cv::waitKey();
}

void averageFilter(string file, cv::Mat &outDepth, cv::Mat &outColor){
	openni::Device device_;
	openni::VideoStream color_;
	openni::VideoStream depth_;
	openni::VideoFrameRef colorf_;
	openni::VideoFrameRef depthf_;
	openni::OpenNI::initialize();
	const char *cstr = file.c_str();
	device_.open(cstr);
	device_.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
	depth_.create(device_, openni::SENSOR_DEPTH);
	depth_.start();
	color_.create(device_, openni::SENSOR_COLOR);
	color_.start();

	long long *temp = new long long[WIDTH * HEIGHT];
	fill(temp, temp + WIDTH * HEIGHT, 0);
	int *cnt = new int[WIDTH * HEIGHT];
	fill(cnt, cnt + WIDTH * HEIGHT, 0);

	int num = 0;
	while (true){
		openni::Status flag1 = depth_.readFrame(&depthf_);
		openni::Status flag2 = color_.readFrame(&colorf_);
		int w = colorf_.getWidth();
		int h = colorf_.getHeight();

		cv::Mat colorImg(h, w, CV_8UC3, (void*)colorf_.getData());
		cv::cvtColor(colorImg, colorImg, CV_BGR2RGB);
		cv::Mat depthImg(h, w, CV_16UC1, (void*)depthf_.getData());

		//depth
		for (int x = 0; x < WIDTH; x++)
		{
			for (int y = 0; y < HEIGHT; y++)
			{
				short d = depthImg.at<short>(y, x);
				if (num ==0)
				{
					temp[y * WIDTH + x] += depthImg.at<short>(y, x);
					cnt[y * WIDTH + x]++;
				}
				else
				{
					if (abs(d - temp[y * WIDTH + x] / cnt[y * WIDTH + x]) < 100){
						temp[y * WIDTH + x] += depthImg.at<short>(y, x);
						cnt[y * WIDTH + x]++;
					}
				}
				
			}
		}

		//color
		if (0 == num){
			outColor = colorImg.clone();
		}

		if (num > 0 && depthf_.getFrameIndex() == 1){
			break;
		}
		num++;
		cout << depthf_.getFrameIndex() << endl;
	}
	
	for (int x = 0; x < WIDTH; x++)
	{
		for (int y = 0; y < HEIGHT; y++)
		{
			if (cnt[y * WIDTH + x] == 0)
				outDepth.at<short>(y, x) = 0;
			else
				outDepth.at<short>(y, x) = (short)(temp[y * WIDTH + x] / cnt[y * WIDTH + x]);
		}
	}

	delete[] temp;
}

void storeFile(string file){
	openni::Device device_;
	openni::VideoStream color_;
	openni::VideoStream depth_;
	openni::VideoFrameRef colorf_;
	openni::VideoFrameRef depthf_;
	openni::OpenNI::initialize();
	const char *cstr = file.c_str();
	device_.open(cstr);
	device_.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
	depth_.create(device_, openni::SENSOR_DEPTH);
	depth_.start();
	color_.create(device_, openni::SENSOR_COLOR);
	color_.start();

	int num = 0;
	
	openni::Status flag1 = depth_.readFrame(&depthf_);
	openni::Status flag2 = color_.readFrame(&colorf_);
	int w = colorf_.getWidth();
	int h = colorf_.getHeight();

	cv::Mat colorImg(h, w, CV_8UC3, (void*)colorf_.getData());
	cv::cvtColor(colorImg, colorImg, CV_BGR2RGB);
	cv::Mat depthImg(h, w, CV_16UC1, (void*)depthf_.getData());

	ofstream output("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\depth_data_scene3.txt");
	//depth
	for (int y = 0; y < HEIGHT; y++)
	{
		for (int x = 0; x < WIDTH; x++)
		{
			short d = depthImg.at<short>(y, x);
			output << d << " ";
		}
	}
	output.close();
	//color
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\colorImg_scene3.png", colorImg);
}

#if 0

void initdata(string file, cv::Mat &color, cv::Mat &depth){
	openni::Device device_;
	openni::VideoStream color_;
	openni::VideoStream depth_;
	openni::VideoFrameRef colorf_;
	openni::VideoFrameRef depthf_;

	openni::OpenNI::initialize();
	const char *cstr = file.c_str();
	device_.open(cstr);
	device_.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);

	depth_.create(device_, openni::SENSOR_DEPTH);
	depth_.start();

	color_.create(device_, openni::SENSOR_COLOR);
	color_.start();

	int num = 0;
	while (true){
		openni::Status flag1 = depth_.readFrame(&depthf_);
		openni::Status flag2 = color_.readFrame(&colorf_);
		int w = colorf_.getWidth();
		int h = colorf_.getHeight();

		cv::Mat colorImg(h, w, CV_8UC3, (void*)colorf_.getData());
		cv::cvtColor(colorImg, colorImg, CV_BGR2RGB);
		cv::Mat depthImg(h, w, CV_16UC1, (void*)depthf_.getData());

		OneImg<short> td;
		OneImg<color_rgb> tc;
		for (int i = 0; i < w; i++)
		{
			for (int j = 0; j < h; j++)
			{
				td.data[j][i] = depthImg.at<short>(j, i);
				cv::Vec3b temp = colorImg.at<cv::Vec3b>(j, i);
				tc.data[j][i] = color_rgb(temp[2], temp[1], temp[0]);
			}
		}

		color.push_back(tc);
		depth.push_back(td);
		if (num > 0 && depthf_.getFrameIndex() == 1){
			break;
		}
		num++;
		cout << depthf_.getFrameIndex() << endl;
	}
}
#endif

void oni_test(){

	string file = "D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\scene3.oni";
	string pathout = "D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\";

	//readImgOut(file, pathout);
	//return 0;

	int w = WIDTH;
	int h = HEIGHT;

	cv::Mat finaldepth(h, w, CV_16UC1);
	cv::Mat finalcolor(h, w, CV_8UC3);

	storeFile(file);

	averageFilter(file, finaldepth, finalcolor);

	float f = 1050.0* w / 1280;
	float f_inv = 1.0 / f;
	float cx = w / 2;
	float cy = h / 2;

	ofstream output("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\out.obj");

	for (int x = 0; x < w; x++){
		for (int y = 0; y < h; y++)
		{
			//mm
			short dep = finaldepth.at<short>(y, x);
			float x3d = (x - cx) * dep * f_inv;
			float y3d = (y - cy) * dep * f_inv;
			float z3d = dep;

			cv::Vec3b c = finalcolor.at<cv::Vec3b>(y, x);

			output << "v " << x3d << " " << y3d << " " << z3d << " " << (int)c[2] << " " << (int)c[1] << " " << (int)c[0] << " " << endl;
		}
	}

	output.close();
}

HRESULT initKinect() {
	// Get a working kinect sensor
	int numSensors;
	if (NuiGetSensorCount(&numSensors) < 0 || numSensors < 1) return false;
	if (NuiCreateSensorByIndex(0, &sensor) < 0) return false;

	// Initialize sensor
	sensor->NuiInitialize(NUI_INITIALIZE_FLAG_USES_DEPTH | NUI_INITIALIZE_FLAG_USES_COLOR);

	HRESULT flag = sensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,            // Depth camera or rgb camera?
		NUI_IMAGE_RESOLUTION_640x480,    // Image resolution
		0,      // Image stream flags, e.g. near mode
		2,      // Number of frames to buffer
		NULL,   // Event handle
		&rgbStream);
	if (flag == S_OK){
		flag = sensor->NuiImageStreamOpen(
			NUI_IMAGE_TYPE_DEPTH,                     // Depth camera or rgb camera?
			NUI_IMAGE_RESOLUTION_640x480,             // Image resolution
			0,      // Image stream flags, e.g. near mode
			2,      // Number of frames to buffer
			NULL,   // Event handle
			&depthStream);
	}
	return flag;
}

bool getKinectData(char* RGBdest, unsigned short* Ddest, NUI_DEPTH_IMAGE_POINT* &_mappedDepthLocations) {
	NUI_IMAGE_FRAME imageFrame;
	NUI_LOCKED_RECT LockedRect1;
	
	//color
	if (sensor->NuiImageStreamGetNextFrame(rgbStream, 0, &imageFrame) < 0) return false;
	INuiFrameTexture* texture1 = imageFrame.pFrameTexture;
	texture1->LockRect(0, &LockedRect1, NULL, 0);

	if (LockedRect1.Pitch != 0)
	{
		const BYTE* RGBcurr = (const BYTE*)LockedRect1.pBits;
		const BYTE* RGBEnd = RGBcurr + (WIDTH*HEIGHT) * 4;

		while (RGBcurr < RGBEnd) {
			*RGBdest++ = *RGBcurr++;
		}
	}
	texture1->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(rgbStream, &imageFrame);

	//depth
	NUI_IMAGE_FRAME depthFrame;
	NUI_LOCKED_RECT LockedRect2;
	if (sensor->NuiImageStreamGetNextFrame(depthStream, 0, &depthFrame) < 0) return false;
	INuiFrameTexture* texture2 = depthFrame.pFrameTexture;
	texture2->LockRect(0, &LockedRect2, NULL, 0);

	if (LockedRect2.Pitch != 0)
	{
		const USHORT* Dcurr = (const USHORT*)LockedRect2.pBits;
		const USHORT* DEnd = Dcurr + (WIDTH*HEIGHT);

		while (Dcurr < DEnd) {
			// Get depth in millimeters
			USHORT depth = NuiDepthPixelToDepth(*Dcurr++);
			*Ddest++ = depth;
		}

		//align
		BOOL nearMode;
		INuiFrameTexture* pColorToDepthTexture;
		sensor->NuiImageFrameGetDepthImagePixelFrameTexture(
			depthStream, &depthFrame, &nearMode, &pColorToDepthTexture);
		NUI_LOCKED_RECT ColorToDepthLockRect;
		pColorToDepthTexture->LockRect(0, &ColorToDepthLockRect, NULL, 0);

		INuiCoordinateMapper* pMapper;

		//设置KINECT实例的空间坐标系
		HRESULT hr = sensor->NuiGetCoordinateMapper(&pMapper);

		pMapper->MapColorFrameToDepthFrame(
			NUI_IMAGE_TYPE_COLOR, 
			NUI_IMAGE_RESOLUTION_640x480, 
			NUI_IMAGE_RESOLUTION_640x480,
			640 * 480, 
			(NUI_DEPTH_IMAGE_PIXEL*)ColorToDepthLockRect.pBits,
			640 * 480, 
			_mappedDepthLocations
			);
	}

	texture2->UnlockRect(0);
	sensor->NuiImageStreamReleaseFrame(depthStream, &depthFrame);
	return true;
}

int kinect_test(){
	HRESULT flag = initKinect();
	if (flag != S_OK){
		cout << "init failed" << endl;
		return -1;
	}

	char *data = new char[WIDTH * HEIGHT * 4];
	fill(data, data + WIDTH*HEIGHT * 4, 0);
	unsigned short *depth = new unsigned short[WIDTH * HEIGHT];
	fill(depth, depth + WIDTH*HEIGHT, 0);
	NUI_DEPTH_IMAGE_POINT* _mappedDepthLocations = new NUI_DEPTH_IMAGE_POINT[HEIGHT * WIDTH];

	while (!getKinectData(data, depth, _mappedDepthLocations)){
		Sleep(1000);
	}


	cv::Mat colorImg(HEIGHT, WIDTH, CV_8UC4, data);
	cv::cvtColor(colorImg, colorImg, CV_RGBA2RGB);
	cv::Mat depthImg(HEIGHT, WIDTH, CV_8U);
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			int d = depth[i * WIDTH + j];
			depthImg.at<char>(i, j) = (d / 10) % 256;
		}
	}

	cv::imshow("test", colorImg);
	cv::imshow("depth", depthImg);

	cv::waitKey();

	int w = WIDTH;
	int h = HEIGHT;

	//store
	
	//just depth data
	ofstream dataout("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\depth_data.txt");
	for (int i = 0; i < WIDTH * HEIGHT; i++)
	{
		dataout << _mappedDepthLocations[i].depth << " ";
	}
	dataout.close();

	//color & depth
	/*
	nearplane = 0.1;
	farplane = 5000;
	focallength = 
	*/
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\colorImg.png", colorImg);
	cv::Mat outDepth(h, w, CV_8U);
	cv::Mat mixed(h, w, CV_8UC3);
	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			outDepth.at<char>(i, j) = (_mappedDepthLocations[i * w + j].depth /12)%256;
				
			cv::Vec3b te = colorImg.at<cv::Vec3b>(i, j);
			te[0] = te[0] * 0.5 + (int)outDepth.at<char>(i, j) * 0.5;
			te[1] = te[1] * 0.5 + (int)outDepth.at<char>(i, j) * 0.5;
			te[2] = te[2] * 0.5 + (int)outDepth.at<char>(i, j) * 0.5;
			mixed.at<cv::Vec3b>(i, j) = te;
		}
	}

	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\outDepth.png", outDepth);
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\mixed.png", mixed);


	//point cloud
	float f = 1050.0* w / 1280;
	float f_inv = 1.0 / f;
	float cx = w / 2;
	float cy = h / 2;

	ofstream output("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\out_kinect.obj");

	for (int x = 0; x < w; x++){
		for (int y = 0; y < h; y++)
		{
			//mm
			short dep = _mappedDepthLocations[y*w + x].depth;
			float x3d = (x - cx) * dep * f_inv;
			float y3d = (y - cy) * dep * f_inv;
			float z3d = dep;

			cv::Vec4b c = colorImg.at<cv::Vec4b>(y, x);

			output << "v " << x3d << " " << y3d << " " << z3d << " " << (int)c[2] << " " << (int)c[1] << " " << (int)c[0] << " " << endl;
		}
	}

	output.close();

	delete[] data;
	delete[] _mappedDepthLocations;
	return 0;
}

float g_dis = 0;
float g_sim = 0;
float g_d_m = 4000;
float g_s_m = 4000;

int filterResult(int *data, int x, int y, int number){
	
	float *filter = new float[4*number*number];
	float weight = 0;
	float n2 = number * number;
	for (int i = -number	; i < number; i++)
	{
		for (int j = -number; j < number; j++)
		{
			int x_now = x + i;
			int y_now = y + j;
			int index = (i + number) + 2 * number * (j + number);
			x_now = x_now < 0 ? 0 : x_now;
			y_now = y_now < 0 ? 0 : y_now;
			x_now = x_now >= WIDTH ? WIDTH - 1 : x_now;
			y_now = y_now >= HEIGHT ? HEIGHT - 1 : y_now;

			if (data[y_now*WIDTH + x_now] == 0){
				continue;
			}

			float dis_w = n2 - abs((x_now - x)*(y_now - y));
			dis_w *= 10;
			
			g_dis = dis_w > g_dis ? dis_w : g_dis;
			g_d_m = dis_w < g_d_m ? dis_w : g_d_m;
			
			float similar_w = 2000 - abs(data[y*WIDTH + x] - data[y_now*WIDTH + x_now]);
			similar_w = similar_w < 1900 ? 0.001 : similar_w;
			
			g_sim = g_sim > similar_w ? g_sim : similar_w;
			g_s_m = g_s_m < similar_w ? g_s_m : similar_w;

			filter[index] = dis_w * similar_w;
			weight += filter[index];
		}
	}
	if (weight < 0.001){
		return 0;
	}

	float w_inv = 1.0 / weight;
	for (int i = 0; i < 4 * number*number; i++)
	{
		filter[i] *= w_inv;
	}

	float result = 0;
	for (int i = -number; i < number; i++)
	{
		for (int j = -number; j < number; j++)
		{
			int x_now = x + i;
			int y_now = y + j;
			int index = (i + number) + 2 * number * (j + number);
			x_now = x_now < 0 ? 0 : x_now;
			y_now = y_now < 0 ? 0 : y_now;
			x_now = x_now >= WIDTH ? WIDTH - 1 : x_now;
			y_now = y_now >= HEIGHT ? HEIGHT - 1 : y_now;
			if (data[y_now*WIDTH + x_now] == 0){
				continue;
			}
			result += data[y_now*WIDTH + x_now] * filter[index];
		}
	}

	delete[] filter;

	return (int)result;
}

int* bilateralFilter(int *data, cv::Mat &mask){
	int nearN = 2;
	int *ndata = new int[WIDTH*HEIGHT];
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			if (mask.at<char>(i, j) != 0)
			{
				ndata[i * WIDTH + j] = 0;
				continue;
			}
			ndata[i * WIDTH + j] = filterResult(data, j, i, nearN);
		}
	}

	delete[] data;
	return ndata;
}

#if 0
//test
double
mygetThreshVal_Otsu_8u(const cv::Mat& _src)
{
	cv::Size size = _src.size();
	if (_src.isContinuous())
	{
		size.width *= size.height;
		size.height = 1;
	}
	const int N = 256;
	int i, j, h[N] = { 0 };
	for (i = 0; i < size.height; i++)
	{
		const uchar* src = _src.data + _src.step*i;
		j = 0;
#if CV_ENABLE_UNROLLED
		for (; j <= size.width - 4; j += 4)
		{
			int v0 = src[j], v1 = src[j + 1];
			h[v0]++; h[v1]++;
			v0 = src[j + 2]; v1 = src[j + 3];
			h[v0]++; h[v1]++;
		}
#endif
		for (; j < size.width; j++)
			h[src[j]]++;
	}

	double mu = 0, scale = 1. / (size.width*size.height);
	for (i = 0; i < N; i++)
		mu += i*(double)h[i];

	mu *= scale;
	double mu1 = 0, q1 = 0;
	double max_sigma = 0, max_val = 0;

	for (i = 0; i < N; i++)
	{
		double p_i, q2, mu2, sigma;

		p_i = h[i] * scale;
		mu1 *= q1;
		q1 += p_i;
		q2 = 1. - q1;

		if (std::min(q1, q2) < FLT_EPSILON || std::max(q1, q2) > 1. - FLT_EPSILON)
			continue;

		mu1 = (mu1 + i*p_i) / q1;
		mu2 = (mu - q1*mu1) / q2;
		sigma = q1*q2*(mu1 - mu2)*(mu1 - mu2);
		if (sigma > max_sigma)
		{
			max_sigma = sigma;
			max_val = i;
		}
	}

	return max_val;
}
#endif

void cutMargin(int* data, cv::Mat img, cv::Mat &mask){
	cv::Mat inDepth(HEIGHT, WIDTH, CV_8U);
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			inDepth.at<char>(i, j) = (data[i * WIDTH + j] / 10) % 256;
		}
	}
	cv::Mat outDepth;
	cv::Mat temp;

	//cv::cvtColor(img, img, CV_RGB2GRAY);
	
	//mygetThreshVal_Otsu_8u(inDepth);
	double otsu_thresh_val = cv::threshold(inDepth, temp, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	double low_threshold = otsu_thresh_val * 0.5;
	//cv::Canny(inDepth, outDepth, low_threshold, otsu_thresh_val);
	cv::Canny(inDepth, outDepth, 25, 50);
	//cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\edges.png", outDepth);
	
	//inflate 2 pixels
	mask = outDepth.clone();
	int cnt = 2;
	for (int i = 0; i < outDepth.rows; i++)
	{
		for (int j = 0; j < outDepth.cols; j++)
		{
			if (outDepth.at<char>(i, j) != 0){
				
				for (int x = -cnt; x < cnt; x++)
				{
					for (int y = -cnt; y < cnt; y++)
					{
						int x_now = j + x;
						int y_now = i + y;
						if (x_now < 0 || x_now >= outDepth.cols || y_now < 0 || y_now >= outDepth.rows)
							continue;

						mask.at<char>(y_now, x_now) = 255;
					}
				}

			}
		}
	}

	//cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\edges_inflation.png", mask);
}

int testData(){
	int *data = new int[WIDTH*HEIGHT];

	cv::Mat colorImg = cv::imread("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\colorImg_scene3.png");
	ifstream input("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\depth_data_scene3.txt");
	for (int i = 0; i < WIDTH*HEIGHT; i++)
	{
		input >> data[i];
	}
	input.close();

	cv::Mat mask;
	cutMargin(data, colorImg, mask);
	data = bilateralFilter(data, mask);


	cv::Mat outDepth(HEIGHT, WIDTH, CV_8U);
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			outDepth.at<char>(i, j) = (data[i * WIDTH + j] / 12) % 256;
		}
	}
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\outDepth_scene3.png", outDepth);

	
	ofstream output("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\out_scene3.obj");
	float f = 1050.0* WIDTH / 1280;
	float f_inv = 1.0 / f;
	float cx = WIDTH / 2;
	float cy = HEIGHT / 2;
	for (int x = 0; x < WIDTH; x++){
		for (int y = 0; y < HEIGHT; y++)
		{
			//mm
			short dep = data[y*WIDTH + x];
			float x3d = (x - cx) * dep * f_inv;
			float y3d = (y - cy) * dep * f_inv;
			float z3d = dep;

			cv::Vec3b c = colorImg.at<cv::Vec3b>(y, x);

			output << "v " << x3d << " " << y3d << " " << z3d << " " << (int)c[2] << " " << (int)c[1] << " " << (int)c[0] << " " << endl;
		}
	}

	output.close();

	delete[] data;



	return 0;
}

void DepthtoRGB(float depth, int *r, int *g, int *b){
	int d = depth * 16777216;
	*r = (d >> 16);
	*g = (d - ((*r) << 16)) >> 8;
	*b = d - ((*r) << 16) - ((*g) << 8);
}

void RGBtoDepth(int r, int g, int b, float *depth){
	*depth = 1.0f * ((r << 16) + (g << 8) + b) / 16777216.0f;
}

void outDepthImage(){
	//read in
	int *data = new int[WIDTH*HEIGHT];
	ifstream input("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\depth_data_scene3.txt");
	for (int i = 0; i < WIDTH*HEIGHT; i++)
	{
		input >> data[i];
	}
	input.close();

	//standard Depth image
	/*
	nearplane = 0.1;
	farplane = 3000;
	focallength =35.0;
	angleOfView = 31.3633 * 2;//horizontal angle not radian

	float depth = (z - near) / (far - near)
	*/

	cameraPara m_cam;

	double width_pixel = m_cam.width;
	double width_notpixel = 2.0 * m_cam.focalLength * tan(m_cam.angleOfView / 2 * M_PI / 180.0);
	double lengthPerPixel = width_notpixel / width_pixel;

	cv::Mat outDepth(HEIGHT, WIDTH, CV_8UC3);
	float nearplane = m_cam.nearPlane;
	float farplane = m_cam.farPlane;

	//ofstream outpppt("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\ppppt.obj");

	for (int row = 0; row < outDepth.rows; row++)
	{
		for (int col = 0; col < outDepth.cols; col++)
		{
			int r, g, b;
			int z = data[row * WIDTH + col];
			if (z == 0){
				outDepth.at<cv::Vec3b>(row, col) = cv::Vec3b(255, 255, 255);
				continue;
			}
			float depth = ((float)z - nearplane) / (farplane - nearplane);
			DepthtoRGB(depth, &r, &g, &b);
			outDepth.at<cv::Vec3b>(row, col) = cv::Vec3b(b, g, r);//opencv is bgr!

			//double x = (col - m_cam.width / 2)*lengthPerPixel *(depth / m_cam.focalLength);
			//double y = (m_cam.height / 2 - row)*lengthPerPixel *(depth / m_cam.focalLength);

			/*int colt = col;
			int rowt = row;
			cv::Vec3b ddd = outDepth.at<cv::Vec3b>(row, col);
			float dt;
			RGBtoDepth(ddd[0],ddd[1],ddd[2], &dt);
			float d_zt = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (dt));
			double xt = (colt - m_cam.width / 2)*lengthPerPixel *(d_zt / m_cam.focalLength);
			double yt = (m_cam.height / 2 - rowt)*lengthPerPixel *(d_zt / m_cam.focalLength);
			outpppt << "v " << xt << " " << yt << " " << d_zt << " " << endl;*/

		}
	}

	//outpppt.close();

	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\in\\scene3depth.png", outDepth);
}

void readYUV(string imgPath, string depthPath, int frameN){
	int imgWidth = 1920;
	int imgHeight = 1088;

	int cnt = imgWidth * (imgHeight + imgHeight / 2);//bytes
	char* data = new char[cnt+2];
	char* ddata = new char[cnt + 2];
	ifstream input(imgPath, ios_base::binary);
	ifstream dinput(depthPath, ios_base::binary);
	int frame = 0;
	cv::Mat re;
	cv::Mat re_d;
	while (!input.eof()||!dinput.eof())
	{
		cout << frame++ << endl;
		input.read(data, cnt);
		dinput.read(ddata, cnt);
		cv::Mat myuv(imgHeight + imgHeight / 2, imgWidth, CV_8UC1, data);
		cv::Mat mbgr(imgHeight, imgWidth, CV_8UC3);
		cv::cvtColor(myuv, mbgr, CV_YUV2BGR_IYUV);

		cv::Mat myuvd(imgHeight + imgHeight / 2, imgWidth, CV_8UC1, ddata);
		cv::Mat mbgrd(imgHeight, imgWidth, CV_8UC3);
		cv::cvtColor(myuvd, mbgrd, CV_YUV2BGR_IYUV);

		//cv::imshow("test", mbgrd);
		//cv::waitKey(10);
		if (frame == frameN)
		{
			re = mbgr.clone();
			re_d = mbgrd.clone();
			break;
		}
	}
	if (input.eof() || dinput.eof())
	{
		cout << "no such frame!" << endl;
		return;
	}

	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\color.png", re);
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\depth.png", re_d);

	//recover the point cloud
	float inM[9] = {
		2607.339177	,0.000000	,960.000000,
		0.000000	,2607.339177	,544.000000,
		0.000000	,0.000000	,1.000000
	};
	cv::Mat intriMatrix(3, 3, CV_32F,inM);
	cv::Mat inv_intri = intriMatrix.inv();
	float z_range[2] = { 0.987432,	851.166600 };//265 frame only
	float cshift = 960;

	float X[3];
	X[2] = 1;

	//test
	/*ofstream out("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\points1.obj");

	for (int row = 0; row < re.rows; row++)
	{
		for (int col = 0; col < re.cols; col++)
		{
			cv::Vec3b color = re.at<cv::Vec3b>(row, col);

			float depth = re_d.at<cv::Vec3b>(row, col)[0];
			depth = (1.0 - depth / 255.0) * (z_range[1] - z_range[0]) + z_range[0] + cshift;

			X[0] = col;
			X[1] = row;
			cv::Mat imagePos(3, 1, CV_32F, X);
			cv::Mat worldPos = inv_intri * imagePos;

			worldPos = worldPos * (depth / worldPos.at<float>(2, 0));

			out << "v " << worldPos.at<float>(0, 0) << " " << worldPos.at<float>(1, 0) << " " << worldPos.at<float>(2, 0) << " " << (int)color[2] << " " << (int)color[1] << " " << (int)color[0] << " " << endl;
		}
	}
	out.close();*/

	//output new depth data
	cv::Mat outD(re_d.rows, re_d.cols, CV_8UC3);
	for (int row = 0; row < re_d.rows; row++)
	{
		for (int col = 0; col < re_d.cols; col++)
		{
			float depth = re_d.at<cv::Vec3b>(row, col)[0];
			depth = (1.0 - depth / 255.0);
			if (depth > 0.9999999)
			{
				depth = 0.9999999;
			}
			int r, g, b;
			DepthtoRGB(depth, &r, &g, &b);
			outD.at<cv::Vec3b>(row, col) = cv::Vec3b(r,g,b);
		}
	}
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\RGBDdata\\data\\imgOut\\color_depth.png", outD);

	cout << "done" << endl;
	delete[] data;
	delete[] ddata;
	input.close();
}

int main(){
	//oni_test();
	//kinect_test();
	
	//testData();
	//outDepthImage();

	string path = "F:\\MicroWorld\\MicroWorld_1\\MicroWorld_1.yuv";
	string depthPath = "F:\\MicroWorld\\depth_MicroWorld_1\\depth_MicroWorld_1.yuv";
	int frameN = 265;
	readYUV(path, depthPath, frameN);
	return 0;
	
}