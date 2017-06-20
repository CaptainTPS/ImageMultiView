#include <time.h>
#include <fstream>
#include "cudaFunctions.cuh"

#include "PatchInpainting.h"
#include "PIheader.h"

#include "D:\captainT\project_13\ImageMultiView\PoissonCloning\PoissonClone.h"

#ifndef DEBUG
#define DEBUG 1
#endif

#ifdef DEBUG
using std::cout;
using std::endl;
#endif

/*
* Return a % b where % is the mathematical modulus operator.
*/
int mod(int a, int b) {
	return ((a % b) + b) % b;
}


cv::Point2f operator/(cv::Point2f p, double d){
	return cv::Point2f(p.x / d, p.y / d);
}

/*
* Load the color, mask, grayscale images with a border of size
* radius around every image to prevent boundary collisions when taking patches
*/
void loadInpaintingImages(
	const std::string& colorFilename,
	const std::string& depthFilename,
	const std::string& maskFilename,
	cv::Mat& colorMat,
	cv::Mat& maskMat,
	cv::Mat& depthMat)
{
	assert(colorFilename.length() && maskFilename.length());

	colorMat = cv::imread(colorFilename, 1); // color
	maskMat = cv::imread(maskFilename, 0);  // grayscale
	depthMat = cv::imread(depthFilename, cv::IMREAD_GRAYSCALE);
}

void initInpaintingImages(
	cv::Mat& colorMat,
	cv::Mat& maskMat,
	cv::Mat& grayMat,
	cv::Mat& depthMat)
{
	assert(colorMat.size() == maskMat.size() && colorMat.size() == depthMat.size());
	assert(!colorMat.empty() && !maskMat.empty() && !depthMat.empty());

	// convert colorMat to depth CV_32F for colorspace conversions
	colorMat.convertTo(colorMat, CV_32F);
	colorMat /= 255.0f;

	depthMat.convertTo(depthMat, CV_32F);
	depthMat /= 255.0f;

	// add border around colorMat
	cv::copyMakeBorder(
		colorMat,
		colorMat,
		RADIUS,
		RADIUS,
		RADIUS,
		RADIUS,
		cv::BORDER_CONSTANT,
		cv::Scalar_<float>(0, 0, 0)
		);
	//to depthMat
	cv::copyMakeBorder(
		depthMat,
		depthMat,
		RADIUS,
		RADIUS,
		RADIUS,
		RADIUS,
		cv::BORDER_CONSTANT,
		cv::Scalar_<float>(0, 0, 0)
		);

	cv::cvtColor(colorMat, grayMat, CV_BGR2GRAY);
}


/*
* Show a Mat object quickly. For testing purposes only.
*/
void showMat(const cv::String& winname, const cv::Mat& mat, int time/*= 5*/)
{
	assert(!mat.empty());
	cv::namedWindow(winname);
	cv::imshow(winname, mat);
	cv::waitKey(time);
	cv::destroyWindow(winname);
}


/*
* Extract closed boundary from mask.
*/
void getContours(cv::Mat& mask,
	contours_t& contours,
	hierarchy_t& hierarchy
	)
{
	assert(mask.type() == CV_8UC1);
	//add boarder
	cv::copyMakeBorder(
		mask,
		mask,
		RADIUS,
		RADIUS,
		RADIUS,
		RADIUS,
		cv::BORDER_CONSTANT,
		cv::Scalar_<float>(0)
		);
	cv::findContours(mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);
}


/*
* Get a patch of size RAIDUS around point p in mat.
*/
cv::Mat getPatch(const cv::Mat& mat, const cv::Point& p)
{
	assert(RADIUS <= p.x && p.x < mat.cols - RADIUS && RADIUS <= p.y && p.y < mat.rows - RADIUS);
	return  mat(
		cv::Range(p.y - RADIUS, p.y + RADIUS + 1),
		cv::Range(p.x - RADIUS, p.x + RADIUS + 1)
		);
}


// get the x and y derivatives of a patch centered at patchCenter in image
// computed using a 3x3 Scharr filter
void getDerivatives(const cv::Mat& grayMat, cv::Mat& dx, cv::Mat& dy)
{
	assert(grayMat.type() == CV_32FC1);

	cv::Sobel(grayMat, dx, -1, 1, 0, -1);
	cv::Sobel(grayMat, dy, -1, 0, 1, -1);
}


/*
* Get the unit normal of a dense list of boundary points centered around point p.
*/
cv::Point2f getNormal(const contour_t& contour, const cv::Point& point)
{
	int sz = (int)contour.size();

	assert(sz != 0);

	int pointIndex = (int)(std::find(contour.begin(), contour.end(), point) - contour.begin());

	assert(pointIndex != contour.size());

	if (sz == 1)
	{
		return cv::Point2f(1.0f, 0.0f);
	}
	else if (sz < 2 * BORDER_RADIUS + 1)
	{
		// Too few points in contour to use LSTSQ regression
		// return the normal with respect to adjacent neigbourhood
		cv::Point adj = contour[(pointIndex + 1) % sz] - contour[pointIndex];
		return cv::Point2f(adj.y, -adj.x) / cv::norm(adj);
	}

	// Use least square regression
	// create X and Y mat to SVD
	cv::Mat X(cv::Size(2, 2 * BORDER_RADIUS + 1), CV_32F);
	cv::Mat Y(cv::Size(1, 2 * BORDER_RADIUS + 1), CV_32F);

	assert(X.rows == Y.rows && X.cols == 2 && Y.cols == 1 && X.type() == Y.type()
		&& Y.type() == CV_32F);

	int i = mod((pointIndex - BORDER_RADIUS), sz);

	float* Xrow;
	float* Yrow;

	int count = 0;
	int countXequal = 0;
	while (count < 2 * BORDER_RADIUS + 1)
	{
		Xrow = X.ptr<float>(count);
		Xrow[0] = contour[i].x;
		Xrow[1] = 1.0f;

		Yrow = Y.ptr<float>(count);
		Yrow[0] = contour[i].y;

		if (Xrow[0] == contour[pointIndex].x)
		{
			++countXequal;
		}

		i = mod(i + 1, sz);
		++count;
	}

	if (countXequal == count)
	{
		return cv::Point2f(1.0f, 0.0f);
	}
	// to find the line of best fit
	cv::Mat sol;
	cv::solve(X, Y, sol, cv::DECOMP_SVD);

	assert(sol.type() == CV_32F);

	float slope = sol.ptr<float>(0)[0];
	cv::Point2f normal(-slope, 1);

	return normal / cv::norm(normal);
}


/*
* Return the confidence of confidencePatch
*/
double computeConfidence(const cv::Mat& confidencePatch)
{
	return cv::sum(confidencePatch)[0] / (double)confidencePatch.total();
}


/*
* Iterate over every contour point in contours and compute the
* priority of path centered at point using grayMat and confidenceMat
*/
void computePriority(const contours_t& contours, const cv::Mat& grayMat, const cv::Mat& confidenceMat, const cv::Mat& depthMat, cv::Mat& priorityMat)
{
	assert(grayMat.type() == CV_32FC1 &&
		priorityMat.type() == CV_32FC1 &&
		confidenceMat.type() == CV_32FC1 &&
		depthMat.type() == CV_32FC1
		);

	// define some patches
	cv::Mat confidencePatch;
	cv::Mat magnitudePatch;
	cv::Mat regPatch;

	cv::Point2f normal;
	cv::Point maxPoint;
	cv::Point2f gradient;

	double confidence;

	// get the derivatives and magnitude of the greyscale image
	cv::Mat dx, dy, magnitude;
	getDerivatives(grayMat, dx, dy);
	cv::magnitude(dx, dy, magnitude);

	// mask the magnitude
	cv::Mat maskedMagnitude(magnitude.size(), magnitude.type(), cv::Scalar_<float>(0));
	magnitude.copyTo(maskedMagnitude, (confidenceMat != 0.0f));
	cv::erode(maskedMagnitude, maskedMagnitude, cv::Mat());

	assert(maskedMagnitude.type() == CV_32FC1);

	//test
	//cv::Mat testout(priorityMat.size(), CV_32F, 0.0f);

	// for each point in contour
	cv::Point point;

	for (int i = 0; i < contours.size(); ++i)
	{
		contour_t contour = contours[i];

		for (int j = 0; j < contour.size(); ++j)
		{

			point = contour[j];

			confidencePatch = getPatch(confidenceMat, point);

			// get confidence of patch
			confidence = cv::sum(confidencePatch)[0] / (double)confidencePatch.total();
			assert(0 <= confidence && confidence <= 1.0f);

			// get the normal to the border around point
			normal = getNormal(contour, point);

			// get the maximum gradient in source around patch
			magnitudePatch = getPatch(maskedMagnitude, point);
			cv::minMaxLoc(magnitudePatch, NULL, NULL, NULL, &maxPoint);
			gradient = cv::Point2f(
				-getPatch(dy, point).ptr<float>(maxPoint.y)[maxPoint.x],
				getPatch(dx, point).ptr<float>(maxPoint.y)[maxPoint.x]
				);

			// get level regularity term
			regPatch = getPatch(depthMat, point).clone();
			float zp = regPatch.total();
			float z_mean = cv::mean(regPatch)[0];
			cv::pow(regPatch - z_mean, 2, regPatch);
			float var = cv::sum(regPatch)[0];
			float lre = zp / (zp + var);

			//background first
			float dterm = depthMat.at<float>(point);

			// set the priority in priorityMat
			priorityMat.ptr<float>(point.y)[point.x] = std::abs( /*(float)confidence * */ /* gradient.dot(normal) * */ lre * dterm);
			assert(priorityMat.ptr<float>(point.y)[point.x] >= 0);

			//testout.ptr<float>(point.y)[point.x] = std::abs(gradient.dot(normal));
		}
	}
	//cv::normalize(testout, testout, 0.0, 1.0, cv::NORM_MINMAX);
	//testout.convertTo(testout, CV_8UC1, 255, 0);
	//cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\isophote.png", testout);
}

/*
ignore the empty part in src image
*/
void CopyWithMask(cv::Mat& mat, const cv::Point& psiHatQ, const cv::Point& psiHatP, const cv::Mat& maskMat, cv::Mat& outputMask){
	
	//minimize difference
	cv::Mat temp = getPatch(mat, psiHatQ).clone();

	/*cv::Mat diff = temp - getPatch(mat, psiHatP);
	auto diff_mean = cv::mean(diff, outputMask == 0);
	temp = temp - diff_mean;*/

	// copy contents of psiHatQ to psiHatP with mask
	//getPatch(mat, psiHatQ).copyTo(getPatch(mat, psiHatP), outputMask);
	temp.copyTo(getPatch(mat, psiHatP), outputMask);
}

/*
* Transfer the values from patch centered at psiHatQ to patch centered at psiHatP in
* mat according to maskMat.
*/
void transferPatch(const cv::Point& psiHatQ, const cv::Point& psiHatP, cv::Mat& mat, cv::Mat& depthMat, const cv::Mat& maskMat, cv::Mat& outputMask)
{
	assert(maskMat.type() == CV_8U);
	assert(mat.size() == maskMat.size());
	assert(RADIUS <= psiHatQ.x && psiHatQ.x < mat.cols - RADIUS && RADIUS <= psiHatQ.y && psiHatQ.y < mat.rows - RADIUS);
	assert(RADIUS <= psiHatP.x && psiHatP.x < mat.cols - RADIUS && RADIUS <= psiHatP.y && psiHatP.y < mat.rows - RADIUS);

	outputMask = getPatch(maskMat, psiHatP);

	//empty parts in src cannnot be transfered
	cv::Mat srcMask = getPatch(maskMat, psiHatQ).clone();
	for (int h = 0; h < srcMask.rows; h++)
	{
		for (int w = 0; w < srcMask.cols; w++)
		{
			if (srcMask.at<unsigned char>(h, w) != 0)
			{
				outputMask.at<uchar>(h, w) = 0;
			}
		}
	}

	//redesign mask base on depth
	cv::Mat depth_base = getPatch(depthMat, psiHatP);
	cv::Mat depthMask = getPatch(depthMat, psiHatQ);
	for (int i = 0; i < outputMask.cols; i++)
	{
		for (int j = 0; j < outputMask.rows; j++)
		{
			if (abs(depthMask.at<float>(j, i) - depth_base.at<float>(j, i)) > DEPTH_THRESHOLD)
			{
				outputMask.at<uchar>(j, i) = 0;
			}
		}
	}

	CopyWithMask(mat, psiHatQ, psiHatP, maskMat, outputMask);
}

void transferPatchWithPoisson(const cv::Point& psiHatQ, const cv::Point& psiHatP, cv::Mat& mat, cv::Mat& depthMat, const cv::Mat& maskMat, cv::Mat& outputMask){
	assert(maskMat.type() == CV_8U);
	assert(mat.size() == maskMat.size());
	assert(RADIUS <= psiHatQ.x && psiHatQ.x < mat.cols - RADIUS && RADIUS <= psiHatQ.y && psiHatQ.y < mat.rows - RADIUS);
	assert(RADIUS <= psiHatP.x && psiHatP.x < mat.cols - RADIUS && RADIUS <= psiHatP.y && psiHatP.y < mat.rows - RADIUS);

	outputMask = getPatch(maskMat, psiHatP);
	if (mat.channels() == 3)
	{
		//for rgb image
		cv::Mat dest = getPatch(mat, psiHatP).clone();
		cv::Mat tempmask = (outputMask == 0);
		pc::DIY_Cloning dc;
		cv::Mat src = getPatch(mat, psiHatQ);
		dc.patchClone(dest, getPatch(mat, psiHatQ), tempmask);

		cv::Mat depth_base = getPatch(depthMat, psiHatP);
		cv::Mat depthMask = getPatch(depthMat, psiHatQ);
		for (int i = 0; i < outputMask.cols; i++)
		{
			for (int j = 0; j < outputMask.rows; j++)
			{
				if (abs(depthMask.at<float>(j, i) - depth_base.at<float>(j, i)) > DEPTH_THRESHOLD)
				{
					outputMask.at<uchar>(j, i) = 0;
				}
			}
		}

		dest.copyTo(getPatch(mat, psiHatP), outputMask);
	}
	else
	{
		//for gray img
		cv::Mat depth_base = getPatch(depthMat, psiHatP);
		cv::Mat depthMask = getPatch(depthMat, psiHatQ);
		for (int i = 0; i < outputMask.cols; i++)
		{
			for (int j = 0; j < outputMask.rows; j++)
			{
				if (abs(depthMask.at<float>(j, i) - depth_base.at<float>(j, i)) > DEPTH_THRESHOLD)
				{
					outputMask.at<uchar>(j, i) = 0;
				}
			}
		}
		getPatch(mat, psiHatQ).copyTo(getPatch(mat, psiHatP), outputMask);
	}


}

void DIYmatchTemplate_notuse(const cv::Mat& source, const cv::Mat& tmplate, cv::Mat& result, const cv::Mat& tmplateMask, const cv::Mat& srcMask){
	//just use cv_tm_sqdiff method
	for (int r = 0; r < result.rows; r++)
	{
		for (int c = 0; c < result.cols; c++)
		{
			float re = 0.0f;
			bool flag = true;
			for (int tx = 0; tx < tmplate.cols; tx++)
			{
				for (int ty = 0; ty < tmplate.rows; ty++)
				{
					for (int channel = 0; channel < tmplate.channels(); channel++)
					{
						if (tmplateMask.at<cv::Vec3f>(ty, tx)[channel] != 0 && srcMask.at<uchar>(r + ty, c + tx) != 0)
						{
							float temp = tmplate.at<cv::Vec3f>(ty, tx)[channel] - source.at<cv::Vec3f>(r + ty, c + tx)[channel];
							re += temp * temp;
							flag = false;
						}
					}
					
				}
			}
			if (flag)
			{
				re = 3.1f * tmplate.cols * tmplate.rows * tmplate.channels();//result above max to be 3
			}

			result.at<float>(r, c) = re;
		}
	}
}

/*
* Runs template matching with tmplate and mask tmplateMask on source.
* Resulting Mat is stored in result.
*
*/
cv::Mat computeSSD(const cv::Mat& tmplate, const cv::Mat& source, const cv::Mat& depthtemp, const cv::Mat& depthSrc, cv::Mat& tmplateMask, const cv::Mat& srcMask)
{
	assert(tmplate.type() == CV_32FC3 && source.type() == CV_32FC3);
	assert(tmplate.rows <= source.rows && tmplate.cols <= source.cols);
	assert(tmplateMask.size() == tmplate.size() && tmplate.type() == tmplateMask.type());

	//float belta = 10;
	float belta = 0;

	cv::Mat result(source.rows - tmplate.rows + 1, source.cols - tmplate.cols + 1, CV_32F, 0.0f);

#if 0
	cv::matchTemplate(source,
		tmplate,
		result,
		CV_TM_SQDIFF,
		tmplateMask
		);
#else
	DIYmatchTemplate(source, tmplate, result, tmplateMask, srcMask);
#endif
	cv::Mat result2(depthSrc.rows - depthtemp.rows + 1, depthSrc.cols - depthtemp.cols + 1, CV_32F, 0.0f);
	cv::Mat tpMask[3];
	cv::split(tmplateMask, tpMask);
	cv::matchTemplate(depthSrc,
		depthtemp,
		result2,
		CV_TM_SQDIFF
		/*tpMask[0]*/
		);
	/*double min1;
	double max1;
	cv::minMaxLoc(result, &min1, &max1, NULL,NULL);
	double min2;
	double max2;
	cv::minMaxLoc(result2, &min2, &max2, NULL, NULL);*/

	result = result + belta * result2;

	cv::normalize(result, result, 0, 1, cv::NORM_MINMAX);
	cv::copyMakeBorder(result, result, RADIUS, RADIUS, RADIUS, RADIUS, cv::BORDER_CONSTANT, 1.1f);

	return result;
}

cv::Point getMatchPoint(const cv::Point& now, cv::Mat& result, cv::Mat erodedMask, const cv::Mat& depthMat, int selectTopNum = 1){
	cv::Point q;
	// set all target regions to 1.1, which is over the maximum value possilbe
	// from SSD
	result.setTo(1.1f, erodedMask == 0);

#if 0
	//set the p patch to 1.1, to avoid being used
	cv::Mat pPatch = result.colRange(cv::Range(now.x - RADIUS, now.x + RADIUS + 1)).rowRange(cv::Range(now.y - RADIUS, now.y + RADIUS + 1));
	pPatch.setTo(1.1f);
#endif

	// get minimum point of SSD between psiHatPColor and colorMat
	float dnow, dq;
	dnow = depthMat.at<float>(now);
	do
	{
		cv::minMaxLoc(result, NULL, NULL, &q);
		dq = depthMat.at<float>(q);
		result.at<float>(q) = 1.1;
	} while (abs(dnow - dq) > DEPTH_THRESHOLD);

	//from top N find the nearest one
	cv::Point qreturn = q;
	float distance = (q - now).x * (q - now).x + (q - now).y * (q - now).y;
	for (int i = 1; i < selectTopNum; i++)
	{
		do
		{
			cv::minMaxLoc(result, NULL, NULL, &q);
			dq = depthMat.at<float>(q);
			result.at<float>(q) = 1.1;
		} while (abs(dnow - dq) > DEPTH_THRESHOLD);
		float d = (q - now).x * (q - now).x + (q - now).y * (q - now).y;
		if (d < distance)
		{
			qreturn = q;
			distance = d;
		}
	}

	return qreturn;
}

cv::Point getMatchPointNoEmpty(const cv::Point& now, cv::Mat& result, cv::Mat erodedMask, const cv::Mat& depthMat, int selectTopNum = 1){
	cv::Mat eroded = (erodedMask != 0);
	cv::erode(eroded, eroded, cv::Mat(), cv::Point(-1, -1), RADIUS);

	cv::Point q;

	result.setTo(1.1f, eroded == 0);
	float dnow, dq;
	dnow = depthMat.at<float>(now);
	do
	{
		cv::minMaxLoc(result, NULL, NULL, &q);
		dq = depthMat.at<float>(q);
		result.at<float>(q) = 1.1;
	} while (abs(dnow - dq) > DEPTH_THRESHOLD);

	//from top N find the nearest one
	cv::Point qreturn = q;
#if 0
	float distance = (q - now).x * (q - now).x + (q - now).y * (q - now).y;
	for (int i = 1; i < selectTopNum; i++)
	{
		do
		{
			cv::minMaxLoc(result, NULL, NULL, &q);
			dq = depthMat.at<float>(q);
			result.at<float>(q) = 1.1;
		} while (abs(dnow - dq) > DEPTH_THRESHOLD);
		float d = (q - now).x * (q - now).x + (q - now).y * (q - now).y;
		if (d < distance)
		{
			qreturn = q;
			distance = d;
		}
	}
#endif
	return qreturn;
}

cv::Point getMatchPointNearby(const cv::Point& now, cv::Mat& result, cv::Mat erodedMask, const cv::Mat& depthMat, bool emptyInLeft = true, int selectTopNum = 1){
	cv::Mat eroded = (erodedMask != 0);
	cv::erode(eroded, eroded, cv::Mat(), cv::Point(-1, -1), RADIUS);

	cv::Mat rect = cv::Mat::ones(erodedMask.size(), CV_8U);
	int rectLength = 300;
	if (emptyInLeft)
	{
		int le = now.x - rectLength / 2;
		le = le < 0 ? 0 : le;
		int ri = now.x + rectLength / 2;
		ri = ri >= result.cols ? result.cols - 1 : ri;
		int up = now.y - rectLength / 2;
		up = up < 0 ? 0 : up;
		int down = now.y + rectLength / 2;
		down = down >= result.rows ? result.rows - 1 : down;
		rect.colRange(le, ri).rowRange(up, down) = 0;
	}
	else
	{
		int le = now.x - rectLength / 2;
		le = le < 0 ? 0 : le;
		int ri = now.x + rectLength / 2;
		ri = ri >= result.cols ? result.cols - 1 : ri;
		int up = now.y - rectLength / 2;
		up = up < 0 ? 0 : up;
		int down = now.y + rectLength / 2;
		down = down >= result.rows ? result.rows - 1 : down;
		rect.colRange(le, ri).rowRange(up, down) = 0;
	}

	cv::Point q;
	result.setTo(1.1f, rect != 0);
	result.setTo(1.1f, eroded == 0);
	float dnow, dq;
	dnow = depthMat.at<float>(now);
	do
	{
		cv::minMaxLoc(result, NULL, NULL, &q);
		dq = depthMat.at<float>(q);
		result.at<float>(q) = 1.1;
	} while (abs(dnow - dq) > DEPTH_THRESHOLD);

	//from top N find the nearest one
	cv::Point qreturn = q;
#if 0
	float distance = (q - now).x * (q - now).x + (q - now).y * (q - now).y;
	for (int i = 1; i < selectTopNum; i++)
	{
		do
		{
			cv::minMaxLoc(result, NULL, NULL, &q);
			dq = depthMat.at<float>(q);
			result.at<float>(q) = 1.1;
		} while (abs(dnow - dq) > DEPTH_THRESHOLD);
		float d = (q - now).x * (q - now).x + (q - now).y * (q - now).y;
		if (d < distance)
		{
			qreturn = q;
			distance = d;
		}
	}
#endif
	return qreturn;
}

float computeSumd(cv::Mat& src, cv::Mat& mask){
	cv::Mat m;
	if (mask.channels() == 3)
	{
		cv::Mat t[3];
		cv::split(mask, t);
		m = t[0].clone();
	}
	else
	{
		m = mask.clone();
	}
	cv::erode(m, m, cv::Mat(), cv::Point(-1, -1), 1);
	//dx
	cv::Mat kernel = cv::Mat::zeros(1, 3, CV_8S);
	kernel.at<char>(0, 2) = 1;
	kernel.at<char>(0, 1) = -1;
	cv::Mat gx;
	filter2D(src, gx, CV_32FC3, kernel);
	kernel = cv::Mat::zeros(3, 1, CV_8S);
	kernel.at<char>(2, 0) = 1;
	kernel.at<char>(1, 0) = -1;
	cv::Mat gy;
	filter2D(src, gy, CV_32FC3, kernel);

	cv::multiply(gx, gx, gx);
	cv::multiply(gy, gy, gy);

	float sum = 0;
	for (size_t ch = 0; ch < src.channels(); ch++)
	{
		for (size_t row = 0; row < src.rows; row++)
		{
			for (size_t col = 0; col < src.cols; col++)
			{
				if (m.at<float>(row,col) != 0)
				{
					sum += gx.at <cv::Vec3f>(row, col)[ch] + gy.at <cv::Vec3f>(row, col)[ch];
				}
			}
		}
	}

	float a = cv::countNonZero(m);
	float b = m.cols * m.rows;

	return (sum * b / a);
}

cv::Point getMatchPointSimilarGradient(const cv::Point& now, cv::Mat& result, cv::Mat erodedMask, const cv::Mat& depthMat, cv::Mat& colorMat, cv::Mat& tplMask, cv::Mat& srcMask, int selectTopNum = 1){
	cv::Mat eroded = (erodedMask != 0);
	cv::erode(eroded, eroded, cv::Mat(), cv::Point(-1, -1), RADIUS);
	cv::Point q;
	result.setTo(1.1f, eroded == 0);
	
	cv::Mat rect = cv::Mat::ones(erodedMask.size(), CV_8U);
	int rectLength = 150;
	int le = now.x - rectLength / 2;
	le = le < 0 ? 0 : le;
	int ri = now.x + rectLength / 2;
	ri = ri >= result.cols ? result.cols - 1 : ri;
	int up = now.y - rectLength / 2;
	up = up < 0 ? 0 : up;
	int down = now.y + rectLength / 2;
	down = down >= result.rows ? result.rows - 1 : down;
	rect.colRange(le, ri).rowRange(up, down) = 0;
	result.setTo(1.1f, rect != 0);

	float dnow, dq;
	dnow = depthMat.at<float>(now);
	do
	{
		cv::minMaxLoc(result, NULL, NULL, &q);
		dq = depthMat.at<float>(q);
		result.at<float>(q) = 1.1;
	} while (abs(dnow - dq) > DEPTH_THRESHOLD);

	//from top N find the silimar gradient one
	cv::Point qreturn = q;

	cv::Mat src = getPatch(colorMat, now);
	float re_src = computeSumd(src, tplMask);

	cv::Mat match = getPatch(colorMat, q);
	cv::Mat mask = getPatch(srcMask, q);
	float re_match = computeSumd(match, mask);
	float diff = abs(re_match - re_src);
	for (int i = 1; i < selectTopNum; i++)
	{
		do
		{
			cv::minMaxLoc(result, NULL, NULL, &q);
			dq = depthMat.at<float>(q);
			result.at<float>(q) = 1.1;
		} while (abs(dnow - dq) > DEPTH_THRESHOLD);
		
		match = getPatch(colorMat, q);
		mask = getPatch(srcMask, q);
		float re_match = computeSumd(match, mask);
		float d = abs(re_match - re_src);

		if (d < diff)
		{
			qreturn = q;
			diff = d;
		}
	}
	return qreturn;
}

void changeHalfMask(cv::Mat& mask, bool left){
	int w = mask.cols;
	int half = (w - 1) / 2;
	int center = half;
	if (left)
	{
		mask.colRange(center + 1, center + half + 1) = 0;
	}
	else
	{
		mask.colRange(0, center) = 0;
	}
	
}

std::string type2str(int type) {
	std::string r;

	uchar depth = type & CV_MAT_DEPTH_MASK;
	uchar chans = 1 + (type >> CV_CN_SHIFT);

	switch (depth) {
	case CV_8U:  r = "8U"; break;
	case CV_8S:  r = "8S"; break;
	case CV_16U: r = "16U"; break;
	case CV_16S: r = "16S"; break;
	case CV_32S: r = "32S"; break;
	case CV_32F: r = "32F"; break;
	case CV_64F: r = "64F"; break;
	default:     r = "User"; break;
	}

	r += "C";
	r += (chans + '0');

	return r;
}



void InnerMainLoop(cv::Mat& colorMat, cv::Mat& maskMat, cv::Mat& depthMat, cv::Mat& outColor, bool isLeft){
	// ---------------- read the images ------------------------
	// colorMat     - color picture + border
	// maskMat      - mask picture + border
	// grayMat      - gray picture + border
	cv::Mat grayMat;
	initInpaintingImages(
		colorMat,
		maskMat,
		grayMat,
		depthMat
		);

	//cv::imshow("color", colorMat);
	//cv::imshow("mask", maskMat);
	//cv::imshow("gray", grayMat);
	//cv::waitKey();

	// confidenceMat - confidence picture + border
	cv::Mat confidenceMat;
	maskMat.convertTo(confidenceMat, CV_32F);
	confidenceMat /= 255.0f;



	// add borders around maskMat and confidenceMat
	cv::copyMakeBorder(maskMat, maskMat,
		RADIUS, RADIUS, RADIUS, RADIUS,
		cv::BORDER_CONSTANT, 0);//border part should be removed from mask in main loop
	cv::copyMakeBorder(confidenceMat, confidenceMat,
		RADIUS, RADIUS, RADIUS, RADIUS,
		cv::BORDER_CONSTANT, 0.0f);//border part have no confidence
	// ---------------- start the algorithm -----------------

	contours_t contours;            // mask contours
	hierarchy_t hierarchy;          // contours hierarchy


	// priorityMat - priority values for all contour points + border
	cv::Mat priorityMat(
		confidenceMat.size(),
		CV_32FC1
		);  // priority value matrix for each contour point

	assert(
		colorMat.size() == grayMat.size() &&
		colorMat.size() == confidenceMat.size() &&
		colorMat.size() == maskMat.size()
		);

	cv::Point psiHatP;          // psiHatP - point of highest confidence

	cv::Mat psiHatPColor;       // color patch around psiHatP

	cv::Mat psiHatPConfidence;  // confidence patch around psiHatP
	double confidence;          // confidence of psiHatPConfidence

	cv::Mat psiHatPDepth;  // depth patch around psiHatP

	cv::Point psiHatQ;          // psiHatQ - point of closest patch

	cv::Mat result;             // holds result from template matching
	cv::Mat erodedMask;         // eroded mask

	cv::Mat templateMask;       // mask for template match (3 channel)

	cv::Mat srcMask;

	// eroded mask is used to ensure that psiHatQ is not overlapping with target
	cv::erode(maskMat, erodedMask, cv::Mat(), cv::Point(-1, -1), RADIUS);

	cv::Mat drawMat;

	clock_t t1 = clock();
	clock_t t2;
	int loop = 0;
	// main loop
	cv::Rect boundMask(RADIUS, RADIUS, colorMat.cols - 2 * RADIUS, colorMat.rows - 2 * RADIUS);
	cv::Mat roiMask = maskMat(boundMask);
	const size_t area = roiMask.total();

#ifdef DEBUG
	std::string videoPath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\process_b";
	videoPath += std::to_string(RADIUS);
	videoPath += ".avi";
	//cv::VideoWriter vw(videoPath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 20, colorMat.size());
	cv::VideoWriter vw(videoPath, cv::VideoWriter::fourcc('M', 'S', 'V', 'C'), 20, colorMat.size());
	//std::cout << "vw: " << vw.isOpened() << std::endl;
#endif

	while (cv::countNonZero(roiMask) != area)   // end when target is filled
	{
		
		if (DEBUG && (loop++) % 50 == 0) {
			t2 = clock();
			float seconds = ((float)(t2 - t1)) / CLOCKS_PER_SEC;
			std::cout << ((float)(t2 - t1)) / CLOCKS_PER_SEC << " : loop " << loop << std::endl;
		}
		if (DEBUG)
		{
			//test
			/*int cnt = 0;
			cv::Vec3f match = cv::Vec3f(0, 0, 0);
			for (int i = 0; i < drawMat.rows; i++)
			{
				for (int j = 0; j < drawMat.cols; j++)
				{
					cv::Vec3f t1 = drawMat.at<cv::Vec3f>(i, j);
					uchar t2 = maskMat.at<uchar>(i, j);
					if (t2 != 0 && t1 == match)
					{
						cnt++;
						drawMat.at<cv::Vec3f>(i, j) = cv::Vec3f(0, 0, 1);
					}
				}
			}*/
			/*if (cnt > 11)
			{
				cout << type2str(drawMat.type()) << endl;
				cout << cnt << endl;
				cout << "detected: "<<loop << endl;
				showMat("test", drawMat, 0);
			}*/
		}

		// set priority matrix to -.1, lower than 0 so that border area is never selected
		priorityMat.setTo(-0.1f);
		
		// get the contours of mask
		getContours(cv::Mat(roiMask == 0), contours, hierarchy);

		// compute the priority for all contour points
		computePriority(contours, grayMat, confidenceMat, depthMat, priorityMat);
		
		// get the patch with the greatest priority
		cv::minMaxLoc(priorityMat, NULL, NULL, NULL, &psiHatP);
		psiHatPColor = getPatch(colorMat, psiHatP);
		psiHatPConfidence = getPatch(confidenceMat, psiHatP);
		psiHatPDepth = getPatch(depthMat, psiHatP);

		cv::Mat confInv = (psiHatPConfidence != 0.0f);
		confInv.convertTo(confInv, CV_32F);
		confInv /= 255.0f;
		// get the patch in source with least distance to psiHatPColor wrt source of psiHatP
		cv::Mat mergeArrays[3] = { confInv, confInv, confInv };
		cv::merge(mergeArrays, 3, templateMask);

		cv::Mat allConfInv = (confidenceMat != 0.0f);
		allConfInv.convertTo(allConfInv, CV_32F);
		allConfInv /= 255.0f;
		cv::Mat mergeArr[3] = { allConfInv, allConfInv, allConfInv };
		cv::merge(mergeArr, 3, srcMask);

		//only use the left part to match
		changeHalfMask(templateMask, isLeft);

		result = computeSSD(psiHatPColor, colorMat, psiHatPDepth, depthMat, templateMask, srcMask);

		// set all target regions to 1.1, which is over the maximum value possilbe
		// from SSD
		//result.setTo(1.1f, erodedMask == 0);
		// get minimum point of SSD between psiHatPColor and colorMat
		//cv::minMaxLoc(result, NULL, NULL, &psiHatQ);
		cv::Mat reCopy;
		if (loop == 150){
			reCopy = result.clone();
		}
		//psiHatQ = getMatchPoint(psiHatP, result, erodedMask, depthMat);
		int topN = 6;
#define NO_POISSON

#ifdef NO_POISSON
		//psiHatQ = getMatchPoint(psiHatP, result, maskMat, depthMat, topN);//try not use erode
		//psiHatQ = getMatchPointNoEmpty(psiHatP, result, maskMat, depthMat, topN);
		//psiHatQ = getMatchPointNearby(psiHatP, result, maskMat, depthMat, topN);
		psiHatQ = getMatchPointSimilarGradient(psiHatP,result,maskMat,depthMat, colorMat, templateMask,srcMask, topN);
#else
		psiHatQ = getMatchPointNoEmpty(psiHatP, result, maskMat, depthMat, topN);
#endif
		assert(psiHatQ != psiHatP);

		if (0 && loop == 150){
			std::string ss150("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\");
			std::ofstream fo(ss150 + "data150.txt");
			cv::Mat cp = getPatch(colorMat, psiHatP).clone();
			cv::Mat cq = getPatch(colorMat, psiHatQ).clone();

			cv::Mat mp = getPatch(maskMat, psiHatP).clone();
			cv::Mat mq = getPatch(maskMat, psiHatQ).clone();

			fo << "cp: " << std::endl << cp << std::endl << std::endl;
			fo << "cq: " << std::endl << cq << std::endl << std::endl;
			fo << "mp: " << std::endl << mp << std::endl << std::endl;
			fo << "mq: " << std::endl << mq << std::endl << std::endl;
			fo << "150result: " << reCopy.at<float>(psiHatQ) << std::endl;
			fo.close();
		}

		// updates
		// copy from psiHatQ to psiHatP for each colorspace
		cv::Mat outputMask;// not 0 stands for have been inpainted
#ifdef NO_POISSON
		transferPatch(psiHatQ, psiHatP, grayMat, depthMat, (maskMat == 0), outputMask);
		transferPatch(psiHatQ, psiHatP, colorMat, depthMat, (maskMat == 0), outputMask);
#else
		if (loop == 6){
			int tetest = 0;

		}
		transferPatchWithPoisson(psiHatQ, psiHatP, grayMat, depthMat, (maskMat == 0), outputMask);
		transferPatchWithPoisson(psiHatQ, psiHatP, colorMat, depthMat, (maskMat == 0), outputMask);
#endif
		// fill in confidenceMat with confidences C(pixel) = C(psiHatP)
		confidence = computeConfidence(psiHatPConfidence);

		assert(0 <= confidence && confidence <= 1.0f);
		// update confidence
		psiHatPConfidence.setTo(confidence, outputMask);
		// update maskMat
		maskMat = (confidenceMat != 0.0f);
		
		//test
		/*if (DEBUG && loop % 50 ==0)
		{
			cv::Mat testM = colorMat.clone();
			for (int i = 0; i < maskMat.rows; i++)
			{
				for (int j = 0; j < maskMat.cols; j++)
				{
					if (maskMat.at<uchar>(i, j) == 0)
					{
						testM.at<cv::Vec3f>(i, j) = cv::Vec3f(0, 0, 1);
					}
				}
			}
			showMat("testm", testM, 0);
		}*/

		/*if (DEBUG && maskMat.at<uchar>(354, 236) != 0)
		{
			std::cout << "special: " << loop << std::endl;
		}*/

		if (DEBUG) {
			drawMat = colorMat.clone();
		}
		if (DEBUG /*&& (loop) %500 == 0*/) {
			//change mask part into red
			/*for (int i = 0; i < drawMat.rows; i++)
			{
				for (int j = 0; j < drawMat.cols; j++)
				{
					cv::Vec3f t1 = drawMat.at<cv::Vec3f>(i, j);
					uchar t2 = maskMat.at<uchar>(i, j);
					if (t2 == 0)
					{
						drawMat.at<cv::Vec3f>(i, j) = cv::Vec3f(0, 0, 1);
					}
				}
			}*/

			drawMat.setTo(cv::Vec3f(0, 0, 1), (maskMat == 0));

			if (0 && loop >= 146 && loop <= 152)
			{
				std::cout << "loop: " << loop << std::endl;
				std::cout << "Q and P: q to p" << std::endl;
				std::cout << psiHatQ << std::endl;
				std::cout << psiHatP << std::endl;

				cv::Rect boundR(RADIUS, RADIUS, colorMat.cols - 2 * RADIUS, colorMat.rows - 2 * RADIUS);
				cv::Mat getout = drawMat(boundR).clone();
				//std::cout<<type2str(colorMatcopy.type());//32FC3
				getout.convertTo(getout, CV_8UC3, 255);
				std::string ss("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\getout");
				cv::imwrite(ss+std::to_string(loop) +".png", getout);
			}

			cv::rectangle(drawMat, psiHatP - cv::Point(RADIUS, RADIUS), psiHatP + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(255, 0, 0));
			cv::rectangle(drawMat, psiHatQ - cv::Point(RADIUS, RADIUS), psiHatQ + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(0, 255, 0));

			drawMat.convertTo(drawMat, CV_8UC3, 255);
			cv::putText(drawMat, std::to_string(loop), cv::Point(0, drawMat.rows - 1), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0));

			vw << drawMat;
			//cv::imshow("mask", maskMat);
			//showMat("red - psiHatQ", drawMat, 0);
			//cv::destroyWindow("mask");
		}
	}
#ifdef DEBUG
	vw.release();
#endif
	//store result
	cv::Rect boundR(RADIUS, RADIUS, colorMat.cols - 2 * RADIUS, colorMat.rows - 2 * RADIUS);
	outColor = colorMat(boundR).clone();
	//std::cout<<type2str(colorMatcopy.type());//32FC3
	outColor.convertTo(outColor, CV_8UC3, 255);
	//showMat("test", colorMatcopy, 0);
	std::string spath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\output_b";
	spath += std::to_string(RADIUS);
#ifdef NO_POISSON
	spath += "nPoisson";
#endif
	spath += ".png";
	cv::imwrite(spath, outColor);

	//showMat("final result", outColor, 0);
}

void PatchInpaint::mainLoop(std::string colorPath, std::string maskPath, std::string depthPath, bool isLeft){

	std::string colorFilename, maskFilename, depthFilename;

	colorFilename = colorPath;
	//"D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_color.png";
	maskFilename = maskPath;
	//"D:\\captainT\\project_13\\ImageMultiView\\PatchInpainting\\data\\test_mask.png";
	depthFilename = depthPath;

	/*if (argc == 3) {
	colorFilename = argv[1];
	maskFilename = argv[2];
	}
	else {
	std::cerr << "Usage: ./inpainting colorImageFile maskImageFile" << std::endl;
	return -1;
	}*/

	// ---------------- read the images ------------------------
	// colorMat     - color picture + border
	// maskMat      - mask picture + border
	// grayMat      - gray picture + border
	cv::Mat colorMat, maskMat, depthMat, outColor;
	loadInpaintingImages(
		colorFilename,
		depthFilename,
		maskFilename,
		colorMat,
		maskMat,
		depthMat
		);
	InnerMainLoop(colorMat, maskMat, depthMat, outColor, isLeft);
}

void PreProcess(cv::Mat& colorMat, cv::Mat& maskMat){
	bool f = false;
	if (colorMat.type() == CV_8UC3)
	{
		colorMat.convertTo(colorMat, CV_32FC3, 1.0 / 255.0);
		f = true;
	}

	for (int row = 0; row < colorMat.rows; row++)
	{
		for (int col = 0; col < colorMat.cols; col++)
		{
			if (maskMat.at<uchar>(row, col) == 0)
			{
				int toRight = 0;
				while (col + toRight < colorMat.cols && maskMat.at<uchar>(row, col + toRight) == 0)
					toRight++;
				//cout << "row : " << row << " col: " << col << endl;
				if (toRight >= 4)
				{
					col = col + toRight;
					continue;
				}
				bool left = true;
				bool right = true;
				if (col - 1 < 0) left = false;
				if (col + toRight + 1 >= colorMat.cols) right = false;

				cv::Vec3f lf, rf;
				if (left && right)
				{
					lf = colorMat.at<cv::Vec3f>(row, col -1);
					rf = colorMat.at<cv::Vec3f>(row, col + toRight);
				}
				else if (left)
				{
					rf = lf = colorMat.at<cv::Vec3f>(row, col - 1);
				}
				else if (right)
				{
					lf = rf = colorMat.at<cv::Vec3f>(row, col + toRight);
				}
				else
				{
					continue;
				}
				for (size_t i = 1; i <= toRight; i++)
				{
					float alpha = (float)i / (toRight + 1);
					colorMat.at<cv::Vec3f>(row, col + i - 1) = lf * (1.0 - alpha) + rf * alpha;
					maskMat.at<uchar>(row, col + i - 1) = 255;
				}
			}
		}
	}
	if (f)
	{
		colorMat.convertTo(colorMat, CV_8UC3, 255.0);
	}
}

void fixMarginEmpty(cv::Mat& colorMat, cv::Mat& maskMat, bool left){
	int col, step;
	if (left)
	{
		col = 0;
		step = 1;
	}
	else
	{
		col = colorMat.cols - 1;
		step = -1;
	}

	for (size_t row = 0; row < colorMat.rows; row++)
	{
		if (maskMat.at<uchar>(row, col) != 0)
		{
			continue;
		}

		int notEmpty = col;
		while (maskMat.at<uchar>(row, notEmpty) == 0)
		{
			notEmpty += step;
		}
		int fill = notEmpty;
		do
		{
			if (maskMat.at<uchar>(row, notEmpty) != 0){
				fill -= step;
				maskMat.at<uchar>(row, fill) = maskMat.at<uchar>(row, notEmpty);
				colorMat.at<cv::Vec3b>(row, fill) = colorMat.at<cv::Vec3b>(row, notEmpty);
				notEmpty += step;
			}
			else{
				notEmpty += step;
			}
		} while (fill != col);
	}
}

double fs(int delta_x, int delta_y, double sigma){
	//spatial
	double inner = -1.0 / sigma * sqrt(delta_x * delta_x + delta_y * delta_y);
	return exp(inner);
}

double fI(cv::Scalar a, cv::Scalar b, double sigma){
	//RGB
	double inner = 0;
	for (size_t i = 0; i < 3; i++)
	{
		inner += (a[i] - b[i])*(a[i] - b[i]);
	}
	inner = -0.1 / sigma * sqrt(inner);
	return exp(inner);
}

double fd(cv::Scalar a, cv::Scalar b, double sigma){
	//depth
	double inner = abs(a[0] - b[0]);
	inner = -0.1 / sigma * inner;
	return exp(inner);
}

void PostProcess(cv::Mat& colorMat, cv::Mat& depthMat){
	//trilateral filter
	double sigma1 = 1.0, sigma2 = 0.25, sigma3 = 0.15;
	//window size is (2*boarder + 1)
	int border = 3;

	cv::Mat tempc = colorMat;
	cv::Mat tempd = depthMat;
	int borderType = cv::BORDER_DEFAULT;
	copyMakeBorder(tempc, tempc, border, border, border, border, borderType);
	copyMakeBorder(tempd, tempd, border, border, border, border, borderType);

	tempc.convertTo(tempc, CV_64FC3);
	tempd.convertTo(tempd, CV_64F);

	cv::Mat resultc = tempc.clone();

	for (size_t i = 0; i < colorMat.rows; i++)
	{
		for (size_t j = 0; j < colorMat.cols; j++)
		{
			int r = i + border;
			int c = j + border;
			cv::Vec3d s(0,0,0);
			double weight = 0;
			for (int bx = -border; bx <= border; bx++)
			{
				for (int by = -border; by <= border; by++)
				{
					double alpha = fs(bx, by, sigma1);
					alpha *= fI(tempc.at<cv::Vec3d>(r, c), tempc.at<cv::Vec3d>(r + by, c + bx), sigma2);
					alpha *= fd(tempd.at<double>(r, c), tempd.at<double>(r + by, c + bx), sigma3);
					s += alpha * tempc.at<cv::Vec3d>(r, c);
					weight += alpha;
				}
			}
			s /= weight;
			resultc.at<cv::Vec3d>(r, c) = s;
		}
	}
	resultc.convertTo(resultc, CV_8UC3);
	cv::Rect boundR(border, border, resultc.cols - 2 * border, resultc.rows - 2 * border);
	colorMat = resultc(boundR).clone();
}

void PatchInpaint::mainLoop(unsigned char* color, unsigned char* mask, unsigned char* depth, unsigned char* out, int width, int height, bool isLeft){
	cv::Mat colorMat(height, width, CV_8UC3, color);
	cv::Mat maskMat(height, width, CV_8UC1, mask);
	cv::Mat depthMat(height, width, CV_8UC1, depth);
	cv::Mat outColor(height, width, CV_8UC3, out);
	
	//bilateral filter
	cv::Mat nDepthMat(depthMat.size(), depthMat.type());
	cv::bilateralFilter(depthMat, nDepthMat, 5, 50, 50);
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\7DepthFill.png", nDepthMat);

	//test
	/*int cnt = 0;
	cv::Vec3b match = cv::Vec3b(0, 0, 0);
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			cv::Vec3b t1 = colorMat.at<cv::Vec3b>(i, j);
			uchar t2 = maskMat.at<uchar>(i, j);
			if (t2 != 0 && t1 == match)
			{
				cnt++;
				
			}
		}
	}
	cout << cnt << endl;*/


	//test
	//cv::String localPath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\";
	//cv::imwrite(localPath + "colorMat.png", colorMat);
	//cv::imwrite(localPath + "maskMat.png", maskMat);

	PreProcess(colorMat, maskMat);
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\8ImgBeforeInpain.png", colorMat);
	//for later only use half to match
	fixMarginEmpty(colorMat, maskMat, isLeft);

	cv::erode(maskMat, maskMat, cv::Mat(), cv::Point(-1,-1), 3);
	//cv::imwrite(localPath + "colorMat2.png", colorMat);
	//cv::imwrite(localPath + "maskMat2.png", maskMat);

	InnerMainLoop(colorMat, maskMat, nDepthMat, outColor, isLeft);
#if 1
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\9ImgAfterInpain.png", outColor);
	PostProcess(outColor, nDepthMat);
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\10ImgTrilateral.png", outColor);
#endif
	unsigned char* ptr = outColor.data;
	for (int i = 0; i < outColor.cols * outColor.rows * outColor.channels(); i++)
	{
		out[i] = ptr[i];
	}
}