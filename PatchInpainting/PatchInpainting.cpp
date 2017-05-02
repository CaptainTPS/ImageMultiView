#include <time.h>

#include "PatchInpainting.h"
#include "PIheader.h"

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

			// set the priority in priorityMat
			priorityMat.ptr<float>(point.y)[point.x] = std::abs((float)confidence * gradient.dot(normal) * lre);
			assert(priorityMat.ptr<float>(point.y)[point.x] >= 0);
		}
	}
}

/*
ignore the empty part in src image
*/
void CopyWithMask(cv::Mat& mat, const cv::Point& psiHatQ, const cv::Point& psiHatP, const cv::Mat& maskMat, cv::Mat& outputMask){
	
	// copy contents of psiHatQ to psiHatP with mask
	getPatch(mat, psiHatQ).copyTo(getPatch(mat, psiHatP), outputMask);
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

/*
* Runs template matching with tmplate and mask tmplateMask on source.
* Resulting Mat is stored in result.
*
*/
cv::Mat computeSSD(const cv::Mat& tmplate, const cv::Mat& source, const cv::Mat& depthtemp, const cv::Mat& depthSrc, cv::Mat& tmplateMask)
{
	assert(tmplate.type() == CV_32FC3 && source.type() == CV_32FC3);
	assert(tmplate.rows <= source.rows && tmplate.cols <= source.cols);
	assert(tmplateMask.size() == tmplate.size() && tmplate.type() == tmplateMask.type());

	float belta = 20;

	cv::Mat result(source.rows - tmplate.rows + 1, source.cols - tmplate.cols + 1, CV_32F, 0.0f);

	cv::matchTemplate(source,
		tmplate,
		result,
		CV_TM_SQDIFF,
		tmplateMask
		);

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

cv::Point getMatchPoint(const cv::Point& now, cv::Mat& result, cv::Mat erodedMask, const cv::Mat& depthMat){
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

	return q;
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



void InnerMainLoop(cv::Mat& colorMat, cv::Mat& maskMat, cv::Mat& depthMat, cv::Mat& outColor){
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
		std::string videoPath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\process1.avi";
		cv::VideoWriter vw(videoPath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), 20, colorMat.size());
		std::cout << "vw: " << vw.isOpened() << std::endl;
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
		result = computeSSD(psiHatPColor, colorMat, psiHatPDepth, depthMat, templateMask);

		// set all target regions to 1.1, which is over the maximum value possilbe
		// from SSD
		//result.setTo(1.1f, erodedMask == 0);
		// get minimum point of SSD between psiHatPColor and colorMat
		//cv::minMaxLoc(result, NULL, NULL, &psiHatQ);

		//psiHatQ = getMatchPoint(psiHatP, result, erodedMask, depthMat);
		psiHatQ = getMatchPoint(psiHatP, result, maskMat, depthMat);//try not use erode
		assert(psiHatQ != psiHatP);

		if (loop == 100)
		{
			std::cout << "Q and P: q to p" << std::endl;
			std::cout << psiHatQ << std::endl;
			std::cout << psiHatP << std::endl;
		}

		// updates
		// copy from psiHatQ to psiHatP for each colorspace
		cv::Mat outputMask;// not 0 stands for have been inpainted
		transferPatch(psiHatQ, psiHatP, grayMat, depthMat, (maskMat == 0), outputMask);
		transferPatch(psiHatQ, psiHatP, colorMat, depthMat, (maskMat == 0), outputMask);

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
			cv::Vec3f match = cv::Vec3f(0, 0, 0);
			for (int i = 0; i < drawMat.rows; i++)
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
			}

			if (loop == 99 || loop == 100)
			{
				cv::Rect boundR(RADIUS, RADIUS, colorMat.cols - 2 * RADIUS, colorMat.rows - 2 * RADIUS);
				cv::Mat getout = drawMat(boundR).clone();
				//std::cout<<type2str(colorMatcopy.type());//32FC3
				getout.convertTo(getout, CV_8UC3, 255);
				std::string ss("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\getout");
				cv::imwrite(ss+std::to_string(loop) +".png", getout);
			}

			cv::rectangle(drawMat, psiHatP - cv::Point(RADIUS, RADIUS), psiHatP + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(255, 0, 0));
			cv::rectangle(drawMat, psiHatQ - cv::Point(RADIUS, RADIUS), psiHatQ + cv::Point(RADIUS + 1, RADIUS + 1), cv::Scalar(0, 0, 255));

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
	cv::imwrite("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\output.png", outColor);

	showMat("final result", outColor, 0);
}

void PatchInpaint::mainLoop(std::string colorPath, std::string maskPath, std::string depthPath){

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
	InnerMainLoop(colorMat, maskMat, depthMat, outColor);
}

void PatchInpaint::mainLoop(unsigned char* color, unsigned char* mask, unsigned char* depth, unsigned char* out, int width, int height){
	cv::Mat colorMat(height, width, CV_8UC3, color);
	cv::Mat maskMat(height, width, CV_8UC1, mask);
	cv::Mat depthMat(height, width, CV_8UC1, depth);
	cv::Mat outColor(height, width, CV_8UC3, out);

	//test
	cv::String localPath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\";
	cv::imwrite(localPath + "colorMat.png", colorMat);
	cv::imwrite(localPath + "maskMat.png", maskMat);
	cv::imwrite(localPath + "depthMat.png", depthMat);
	cv::Mat nDepthMat(depthMat.size(), depthMat.type());
	cv::bilateralFilter(depthMat, nDepthMat, 5, 50, 50);
	cv::imwrite(localPath + "outMat.png", nDepthMat);

	//test
	int cnt = 0;
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
	cout << cnt << endl;

	InnerMainLoop(colorMat, maskMat, nDepthMat, outColor);
	cv::imwrite(localPath + "output2.png", outColor);
	unsigned char* ptr = outColor.data;
	for (int i = 0; i < outColor.cols * outColor.rows * outColor.channels(); i++)
	{
		out[i] = ptr[i];
	}
}