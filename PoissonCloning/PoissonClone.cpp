//rewrite by TPS (opencv3  seamless cloning

#include "PoissonClone.h"

using namespace cv;
using namespace pc;
using namespace std;


void DIY_Cloning::computeGradientX(const Mat &img, Mat &gx)
{
	Mat kernel = Mat::zeros(1, 3, CV_8S);
	kernel.at<char>(0, 2) = 1;
	kernel.at<char>(0, 1) = -1;

	if (img.channels() == 3)
	{
		filter2D(img, gx, CV_32F, kernel);
	}
	else if (img.channels() == 1)
	{
		Mat tmp[3];
		for (int chan = 0; chan < 3; ++chan)
		{
			filter2D(img, tmp[chan], CV_32F, kernel);
		}
		merge(tmp, 3, gx);
	}
}

void DIY_Cloning::computeGradientY(const Mat &img, Mat &gy)
{
	Mat kernel = Mat::zeros(3, 1, CV_8S);
	kernel.at<char>(2, 0) = 1;
	kernel.at<char>(1, 0) = -1;

	if (img.channels() == 3)
	{
		filter2D(img, gy, CV_32F, kernel);
	}
	else if (img.channels() == 1)
	{
		Mat tmp[3];
		for (int chan = 0; chan < 3; ++chan)
		{
			filter2D(img, tmp[chan], CV_32F, kernel);
		}
		merge(tmp, 3, gy);
	}
}

void DIY_Cloning::computeLaplacianX(const Mat &img, Mat &laplacianX)
{
	Mat kernel = Mat::zeros(1, 3, CV_8S);
	kernel.at<char>(0, 0) = -1;
	kernel.at<char>(0, 1) = 1;
	filter2D(img, laplacianX, CV_32F, kernel);
}

void DIY_Cloning::computeLaplacianY(const Mat &img, Mat &laplacianY)
{
	Mat kernel = Mat::zeros(3, 1, CV_8S);
	kernel.at<char>(0, 0) = -1;
	kernel.at<char>(1, 0) = 1;
	filter2D(img, laplacianY, CV_32F, kernel);
}

void DIY_Cloning::dst(const Mat& src, Mat& dest, bool invert)
{
	Mat temp = Mat::zeros(src.rows, 2 * src.cols + 2, CV_32F);

	int flag = invert ? DFT_ROWS + DFT_SCALE + DFT_INVERSE : DFT_ROWS;

	src.copyTo(temp(Rect(1, 0, src.cols, src.rows)));

	for (int j = 0; j < src.rows; ++j)
	{
		float * tempLinePtr = temp.ptr<float>(j);
		const float * srcLinePtr = src.ptr<float>(j);
		for (int i = 0; i < src.cols; ++i)
		{
			tempLinePtr[src.cols + 2 + i] = -srcLinePtr[src.cols - 1 - i];
		}
	}

	Mat planes[] = { temp, Mat::zeros(temp.size(), CV_32F) };
	Mat complex;

	merge(planes, 2, complex);
	dft(complex, complex, flag);
	split(complex, planes);
	temp = Mat::zeros(src.cols, 2 * src.rows + 2, CV_32F);

	for (int j = 0; j < src.cols; ++j)
	{
		float * tempLinePtr = temp.ptr<float>(j);
		for (int i = 0; i < src.rows; ++i)
		{
			float val = planes[1].ptr<float>(i)[j + 1];
			tempLinePtr[i + 1] = val;
			tempLinePtr[temp.cols - 1 - i] = -val;
		}
	}

	Mat planes2[] = { temp, Mat::zeros(temp.size(), CV_32F) };

	merge(planes2, 2, complex);
	dft(complex, complex, flag);
	split(complex, planes2);

	temp = planes2[1].t();
	dest = Mat::zeros(src.size(), CV_32F);
	temp(Rect(0, 1, src.cols, src.rows)).copyTo(dest);
}

void DIY_Cloning::idst(const Mat& src, Mat& dest)
{
	dst(src, dest, true);
}

void DIY_Cloning::solve(const Mat &img, Mat& mod_diff, Mat &result)
{
	const int w = img.cols;
	const int h = img.rows;

	Mat res;
	dst(mod_diff, res);

	for (int j = 0; j < h - 2; j++)
	{
		float * resLinePtr = res.ptr<float>(j);
		for (int i = 0; i < w - 2; i++)
		{
			resLinePtr[i] /= (filter_X[i] + filter_Y[j] - 4);
		}
	}

	idst(res, mod_diff);

	unsigned char *  resLinePtr = result.ptr<unsigned char>(0);
	const unsigned char * imgLinePtr = img.ptr<unsigned char>(0);
	const float * interpLinePtr = NULL;

	//first col
	for (int i = 0; i < w; ++i)
		result.ptr<unsigned char>(0)[i] = img.ptr<unsigned char>(0)[i];

	for (int j = 1; j < h - 1; ++j)
	{
		resLinePtr = result.ptr<unsigned char>(j);
		imgLinePtr = img.ptr<unsigned char>(j);
		interpLinePtr = mod_diff.ptr<float>(j - 1);

		//first row
		resLinePtr[0] = imgLinePtr[0];

		for (int i = 1; i < w - 1; ++i)
		{
			//saturate cast is not used here, because it behaves differently from the previous implementation
			//most notable, saturate_cast rounds before truncating, here it's the opposite.
			float value = interpLinePtr[i - 1];
			if (value < 0.)
				resLinePtr[i] = 0;
			else if (value > 255.0)
				resLinePtr[i] = 255;
			else
				resLinePtr[i] = static_cast<unsigned char>(value);
		}

		//last row
		resLinePtr[w - 1] = imgLinePtr[w - 1];
	}

	//last col
	resLinePtr = result.ptr<unsigned char>(h - 1);
	imgLinePtr = img.ptr<unsigned char>(h - 1);
	for (int i = 0; i < w; ++i)
		resLinePtr[i] = imgLinePtr[i];
}

void DIY_Cloning::poissonSolver(const Mat &img, Mat &laplacianX, Mat &laplacianY, Mat &result)
{
	const int w = img.cols;
	const int h = img.rows;

	Mat lap = Mat(img.size(), CV_32FC1);

	lap = laplacianX + laplacianY;

	Mat bound = img.clone();

	rectangle(bound, Point(1, 1), Point(img.cols - 2, img.rows - 2), Scalar::all(0), -1);
	Mat boundary_points;
	Laplacian(bound, boundary_points, CV_32F);

	boundary_points = lap - boundary_points;

	Mat mod_diff = boundary_points(Rect(1, 1, w - 2, h - 2));

	solve(img, mod_diff, result);
}

void DIY_Cloning::initVariables(const Mat &destination, const Mat &binaryMask)
{
	destinationGradientX = Mat(destination.size(), CV_32FC3);
	destinationGradientY = Mat(destination.size(), CV_32FC3);
	patchGradientX = Mat(destination.size(), CV_32FC3);
	patchGradientY = Mat(destination.size(), CV_32FC3);

	binaryMaskFloat = Mat(binaryMask.size(), CV_32FC1);
	binaryMaskFloatInverted = Mat(binaryMask.size(), CV_32FC1);

	//init of the filters used in the dst
	const int w = destination.cols;
	filter_X.resize(w - 2);
	for (int i = 0; i < w - 2; ++i)
		filter_X[i] = 2.0f * std::cos(static_cast<float>(CV_PI)* (i + 1) / (w - 1));

	const int h = destination.rows;
	filter_Y.resize(h - 2);
	for (int j = 0; j < h - 2; ++j)
		filter_Y[j] = 2.0f * std::cos(static_cast<float>(CV_PI)* (j + 1) / (h - 1));
}

void DIY_Cloning::computeDerivatives(const Mat& destination, const Mat &patch, const Mat &binaryMask)
{
	initVariables(destination, binaryMask);

	computeGradientX(destination, destinationGradientX);
	computeGradientY(destination, destinationGradientY);

	computeGradientX(patch, patchGradientX);
	computeGradientY(patch, patchGradientY);

	Mat Kernel(Size(3, 3), CV_8UC1);
	Kernel.setTo(Scalar(1));
	dilate(binaryMask, binaryMask, Kernel, Point(-1, -1), 3);

	binaryMask.convertTo(binaryMaskFloat, CV_32FC1, 1.0 / 255.0);
}

void DIY_Cloning::scalarProduct(Mat mat, float r, float g, float b)
{
	vector <Mat> channels;
	split(mat, channels);
	multiply(channels[2], r, channels[2]);
	multiply(channels[1], g, channels[1]);
	multiply(channels[0], b, channels[0]);
	merge(channels, mat);
}

void DIY_Cloning::arrayProduct(const cv::Mat& lhs, const cv::Mat& rhs, cv::Mat& result) const
{
	vector <Mat> lhs_channels;
	vector <Mat> result_channels;

	split(lhs, lhs_channels);
	split(result, result_channels);

	for (int chan = 0; chan < 3; ++chan)
		multiply(lhs_channels[chan], rhs, result_channels[chan]);

	merge(result_channels, result);
}

void DIY_Cloning::poisson(const Mat &destination)
{
	Mat laplacianX = Mat(destination.size(), CV_32FC3);
	Mat laplacianY = Mat(destination.size(), CV_32FC3);

	laplacianX = destinationGradientX + patchGradientX;
	laplacianY = destinationGradientY + patchGradientY;

	computeLaplacianX(laplacianX, laplacianX);
	computeLaplacianY(laplacianY, laplacianY);

	split(laplacianX, rgbx_channel);
	split(laplacianY, rgby_channel);

	split(destination, output);

	for (int chan = 0; chan < 3; ++chan)
	{
		poissonSolver(output[chan], rgbx_channel[chan], rgby_channel[chan], output[chan]);
	}
}

void DIY_Cloning::evaluate(const Mat &I, const Mat &wmask, const Mat &cloned)
{
	bitwise_not(wmask, wmask);

	wmask.convertTo(binaryMaskFloatInverted, CV_32FC1, 1.0 / 255.0);

	arrayProduct(destinationGradientX, binaryMaskFloatInverted, destinationGradientX);
	arrayProduct(destinationGradientY, binaryMaskFloatInverted, destinationGradientY);

	poisson(I);

	merge(output, cloned);
}

void DIY_Cloning::normalClone(const Mat &destination, const Mat &patch, const Mat &binaryMask, Mat &cloned, int flag)
{
	/*
	patch: needed to be cloned src colors(3 channels), zero for empty parts; size is the same as destinantion;
	binary mask: mask for destination, parts of non zero needed to be cloned; 
				zero for all other parts, should be the same as empty parts above;
	*/
	const int w = destination.cols;
	const int h = destination.rows;
	const int channel = destination.channels();
	const int n_elem_in_line = w * channel;

	computeDerivatives(destination, patch, binaryMask);

	switch (flag)
	{
	case NORMAL_CLONE:
		arrayProduct(patchGradientX, binaryMaskFloat, patchGradientX);
		arrayProduct(patchGradientY, binaryMaskFloat, patchGradientY);
		break;

	case MIXED_CLONE:
	{
		AutoBuffer<int> maskIndices(n_elem_in_line);
		for (int i = 0; i < n_elem_in_line; ++i)
			maskIndices[i] = i / channel;

		for (int i = 0; i < h; i++)
		{
			float * patchXLinePtr = patchGradientX.ptr<float>(i);
			float * patchYLinePtr = patchGradientY.ptr<float>(i);
			const float * destinationXLinePtr = destinationGradientX.ptr<float>(i);
			const float * destinationYLinePtr = destinationGradientY.ptr<float>(i);
			const float * binaryMaskLinePtr = binaryMaskFloat.ptr<float>(i);

			for (int j = 0; j < n_elem_in_line; j++)
			{
				int maskIndex = maskIndices[j];

				if (abs(patchXLinePtr[j] - patchYLinePtr[j]) >
					abs(destinationXLinePtr[j] - destinationYLinePtr[j]))
				{
					patchXLinePtr[j] *= binaryMaskLinePtr[maskIndex];
					patchYLinePtr[j] *= binaryMaskLinePtr[maskIndex];
				}
				else
				{
					patchXLinePtr[j] = destinationXLinePtr[j]
						* binaryMaskLinePtr[maskIndex];
					patchYLinePtr[j] = destinationYLinePtr[j]
						* binaryMaskLinePtr[maskIndex];
				}
			}
		}
	}
	break;

	case MONOCHROME_TRANSFER:
		Mat gray = Mat(patch.size(), CV_8UC1);
		cvtColor(patch, gray, COLOR_BGR2GRAY);

		computeGradientX(gray, patchGradientX);
		computeGradientY(gray, patchGradientY);

		arrayProduct(patchGradientX, binaryMaskFloat, patchGradientX);
		arrayProduct(patchGradientY, binaryMaskFloat, patchGradientY);
		break;

	}

	evaluate(destination, binaryMask, cloned);
}

void DIY_Cloning::localColorChange(Mat &I, Mat &mask, Mat &wmask, Mat &cloned, float red_mul = 1.0,
	float green_mul = 1.0, float blue_mul = 1.0)
{
	computeDerivatives(I, mask, wmask);

	arrayProduct(patchGradientX, binaryMaskFloat, patchGradientX);
	arrayProduct(patchGradientY, binaryMaskFloat, patchGradientY);
	scalarProduct(patchGradientX, red_mul, green_mul, blue_mul);
	scalarProduct(patchGradientY, red_mul, green_mul, blue_mul);

	evaluate(I, wmask, cloned);
}

void DIY_Cloning::illuminationChange(Mat &I, Mat &mask, Mat &wmask, Mat &cloned, float alpha, float beta)
{
	computeDerivatives(I, mask, wmask);

	arrayProduct(patchGradientX, binaryMaskFloat, patchGradientX);
	arrayProduct(patchGradientY, binaryMaskFloat, patchGradientY);

	Mat mag = Mat(I.size(), CV_32FC3);
	magnitude(patchGradientX, patchGradientY, mag);

	Mat multX, multY, multx_temp, multy_temp;

	multiply(patchGradientX, pow(alpha, beta), multX);
	pow(mag, -1 * beta, multx_temp);
	multiply(multX, multx_temp, patchGradientX);
	patchNaNs(patchGradientX);

	multiply(patchGradientY, pow(alpha, beta), multY);
	pow(mag, -1 * beta, multy_temp);
	multiply(multY, multy_temp, patchGradientY);
	patchNaNs(patchGradientY);

	Mat zeroMask = (patchGradientX != 0);

	patchGradientX.copyTo(patchGradientX, zeroMask);
	patchGradientY.copyTo(patchGradientY, zeroMask);

	evaluate(I, wmask, cloned);
}

void DIY_Cloning::textureFlatten(Mat &I, Mat &mask, Mat &wmask, float low_threshold,
	float high_threshold, int kernel_size, Mat &cloned)
{
	computeDerivatives(I, mask, wmask);

	Mat out = Mat(mask.size(), CV_8UC1);
	Canny(mask, out, low_threshold, high_threshold, kernel_size);

	Mat zeros(patchGradientX.size(), CV_32FC3);
	zeros.setTo(0);
	Mat zerosMask = (out != 255);
	zeros.copyTo(patchGradientX, zerosMask);
	zeros.copyTo(patchGradientY, zerosMask);

	arrayProduct(patchGradientX, binaryMaskFloat, patchGradientX);
	arrayProduct(patchGradientY, binaryMaskFloat, patchGradientY);

	evaluate(I, wmask, cloned);
}

void DIY_Cloning::seamlessClone(InputArray _src, InputArray _dst, InputArray _mask, Point p, OutputArray _blend, int flags)
{
	const Mat src = _src.getMat();
	const Mat dest = _dst.getMat();
	const Mat mask = _mask.getMat();
	_blend.create(dest.size(), CV_8UC3);
	Mat blend = _blend.getMat();

	int minx = INT_MAX, miny = INT_MAX, maxx = INT_MIN, maxy = INT_MIN;
	int h = mask.size().height;
	int w = mask.size().width;

	Mat gray = Mat(mask.size(), CV_8UC1);
	Mat dst_mask = Mat::zeros(dest.size(), CV_8UC1);
	Mat cs_mask = Mat::zeros(src.size(), CV_8UC3);
	Mat cd_mask = Mat::zeros(dest.size(), CV_8UC3);

	if (mask.channels() == 3)
		cvtColor(mask, gray, COLOR_BGR2GRAY);
	else
		gray = mask;

	for (int i = 0; i<h; i++)
	{
		for (int j = 0; j<w; j++)
		{
			if (gray.at<uchar>(i, j) == 255)
			{
				minx = std::min(minx, i);
				maxx = std::max(maxx, i);
				miny = std::min(miny, j);
				maxy = std::max(maxy, j);
			}
		}
	}

	int lenx = maxx - minx;
	int leny = maxy - miny;

	Mat patch = Mat::zeros(Size(leny, lenx), CV_8UC3);

	int minxd = p.y - lenx / 2;
	int maxxd = p.y + lenx / 2;
	int minyd = p.x - leny / 2;
	int maxyd = p.x + leny / 2;

	CV_Assert(minxd >= 0 && minyd >= 0 && maxxd <= dest.rows && maxyd <= dest.cols);

	Rect roi_d(minyd, minxd, leny, lenx);
	Rect roi_s(miny, minx, leny, lenx);

	Mat destinationROI = dst_mask(roi_d);
	Mat sourceROI = cs_mask(roi_s);

	gray(roi_s).copyTo(destinationROI);
	src(roi_s).copyTo(sourceROI, gray(roi_s));
	src(roi_s).copyTo(patch, gray(roi_s));

	destinationROI = cd_mask(roi_d);
	cs_mask(roi_s).copyTo(destinationROI);


	DIY_Cloning obj;
	obj.normalClone(dest, cd_mask, dst_mask, blend, flags);

}

void DIY_Cloning::patchClone2(InputArray _src, InputArray _dst, InputArray _mask, OutputArray _blend, int flags){
	const Mat src = _src.getMat();
	const Mat dest = _dst.getMat();
	const Mat mask = _mask.getMat();
	_blend.create(dest.size(), CV_8UC3);
	Mat blend = _blend.getMat();

	Mat gray = Mat(mask.size(), CV_8UC1);

	
	if (mask.channels() == 3)
		cvtColor(mask, gray, COLOR_BGR2GRAY);
	else
		gray = mask;

	if (1)
		gray = 255 - gray;


	DIY_Cloning obj;
	obj.normalClone(dest, src, gray, blend, flags);
}



void constructAXb(Mat& dx, Mat& dy, Mat& srcWithEmpty, Mat& mask, Mat& A, Mat& b){
	Mat initA(dx.rows * dx.cols + dy.rows * dy.cols, srcWithEmpty.rows * srcWithEmpty.cols, CV_32F);
	initA = 0;
	//about boarder, using BORDER_DEFAULT = BORDER_REFLECT_101 = 4, //!< `gfedcb|abcdefgh|gfedcba`
	//using rowMajor expand

	int srcW = srcWithEmpty.cols;
	int srcH = srcWithEmpty.rows;
	//for dx
	int px = 0;
	for (size_t i = 0; i < dx.rows; i++)
	{
		for (size_t j = 0; j < dx.cols; j++)
		{
			if (j != dx.cols - 1)
			{
				initA.at<float>(px, px) = -1;
				initA.at<float>(px, px + 1) = 1;
			}
			else
			{
				initA.at<float>(px, px - 1) = 1;
				initA.at<float>(px, px) = -1;
			}
			px++;
		}
	}
	//for dy
	int py_row = px;
	int py_col = 0;
	int offset = srcW;
	for (size_t i = 0; i < dy.rows; i++)
	{
		for (size_t j = 0; j < dy.cols; j++)
		{
			if (i != dy.rows - 1)
			{
				initA.at<float>(py_row, py_col) = -1;
				initA.at<float>(py_row, py_col + offset) = 1;
			}
			else
			{
				initA.at<float>(py_row, py_col - offset) = 1;
				initA.at<float>(py_row, py_col) = -1;
			}
			py_row++;
			py_col++;
		}
	}

	//construct b
	int totalH = dx.rows * dx.cols + dy.rows * dy.cols;
	int totalW = srcWithEmpty.rows * srcWithEmpty.cols;
	int cntKnown = 0;
	int cntX = 0;

	bool *flags = new bool[srcWithEmpty.rows * srcWithEmpty.cols];
	int index = 0;
	for (int i = 0; i < mask.rows; i++)
	{
		for (int j = 0; j < mask.cols; j++)
		{
			flags[index] = (mask.at<uchar>(i, j) != 0);
			if (flags[index])
			{
				cntKnown++;
			}
			else
			{
				cntX++;
			}
			index++;
		}
	}

	Mat btemp = Mat::zeros(dx.rows * dx.cols + dy.rows * dy.cols, 1, CV_32F);
	int pb = 0;
	for (int i = 0; i < dx.rows; ++i)
	{
		for (int j = 0; j < dx.cols; ++j)
		{
			btemp.at<float>(pb, 0) = dx.at<float>(i, j);
			pb++;
		}
	}
	for (int i = 0; i < dy.rows; ++i)
	{
		for (int j = 0; j < dy.cols; ++j)
		{
			btemp.at<float>(pb, 0) = dy.at<float>(i, j);
			pb++;
		}
	}

	Mat AX(totalH, cntX, CV_32F);
	Mat AKnown(totalH, cntKnown, CV_32F);
	int xnow = 0;
	int know = 0;
	for (int i = 0; i < totalW; ++i)
	{
		if (flags[i])
		{
			initA.col(i).copyTo(AKnown.col(know));
			know++;
		}
		else{
			initA.col(i).copyTo(AX.col(xnow));
			xnow++;
		}
	}

	Mat srcKnow(cntKnown, 1, CV_32F);
	int snow = 0;
	int pushnow = 0;
	for (int i = 0; i < srcWithEmpty.rows; ++i)
	{
		for (int j = 0; j < srcWithEmpty.cols; ++j)
		{
			if (flags[snow])
			{
				srcKnow.at<float>(pushnow, 0) = srcWithEmpty.at<float>(i, j);
				pushnow++;
			}
			snow++;
		}
	}

	A = AX.clone();
	b = (btemp - AKnown * srcKnow);

	delete[] flags;
}

void recoverDest(Mat& dest, Mat& mask, Mat& x){
	int now = 0;
	for (int i = 0; i < mask.rows; i++)
	{
		for (int j = 0; j < mask.cols; j++)
		{
			if (mask.ptr(i)[j] == 0)
			{
				dest.at<float>(i, j) = x.at<float>(now, 0);
				now++;
			}
		}
	}
}

void DIY_Cloning::patchClone(Mat& dest, Mat& patch, Mat& dest_mask){
	assert(dest.size() == patch.size() && dest.size() == dest_mask.size());
	assert(dest.channels() == 3 && patch.channels() == 3);

	if (dest.type() != CV_32FC3)
	{
		dest.convertTo(dest, CV_32FC3, 1.0 / 255.0);
	}
	if (patch.type() != CV_32FC3)
	{
		patch.convertTo(patch, CV_32FC3, 1.0 / 255.0);
	}

	dest_dx = Mat(dest.size(), CV_32FC3);
	dest_dy = Mat(dest.size(), CV_32FC3);
	patch_dx = Mat(patch.size(), CV_32FC3);
	patch_dy = Mat(patch.size(), CV_32FC3);
	dx = Mat(dest.size(), CV_32FC3);
	dy = Mat(dest.size(), CV_32FC3);
	binaryMaskFloat = Mat(dest_mask.size(), CV_32FC1);
	binaryMaskFloatInverted = Mat(dest_mask.size(), CV_32FC1);

	Mat Kernel(Size(3, 3), CV_8UC1);
	Kernel.setTo(Scalar(1));
	Mat temp_dest_mask;
	erode(dest_mask, temp_dest_mask, Kernel, Point(-1, -1), 1);
	temp_dest_mask.convertTo(binaryMaskFloat, CV_32FC1, 1.0 / 255.0);

	Mat wmask = temp_dest_mask.clone();
	bitwise_not(wmask, wmask);
	wmask.convertTo(binaryMaskFloatInverted, CV_32FC1, 1.0 / 255.0);
	//get dx dy
	computeGradientX(dest, dest_dx);
	computeGradientY(dest, dest_dy);
	computeGradientX(patch, patch_dx);
	computeGradientY(patch, patch_dy);
	//integrate
	arrayProduct(dest_dx, binaryMaskFloat, dest_dx);
	arrayProduct(dest_dy, binaryMaskFloat, dest_dy);
	arrayProduct(patch_dx, binaryMaskFloatInverted, patch_dx);
	arrayProduct(patch_dy, binaryMaskFloatInverted, patch_dy);
	dx = dest_dx + patch_dx;
	dy = dest_dy + patch_dy;
	//solve
	Mat dx_channel[3];
	Mat dy_channel[3];
	Mat dest_channel[3];
	split(dx, dx_channel);
	split(dy, dy_channel);
	split(dest, dest_channel);

	for (int i = 0; i < dest.channels(); i++)
	{
		Mat A, b;
		constructAXb(dx_channel[i], dy_channel[i], dest_channel[i], dest_mask, A, b);
		Mat x;
		cv::solve(A, b, x, DECOMP_SVD);
		recoverDest(dest_channel[i], dest_mask, x);
	}
	merge(dest_channel, 3, dest);
}