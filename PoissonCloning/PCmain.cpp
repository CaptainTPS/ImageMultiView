#include "PoissonClone.h"

#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

using namespace cv;

int main1(){
	Mat src = imread("test1.jpg");
	Mat dst = imread("sky2.jpg");
	Mat new_src;
	resize(src, new_src, Size(src.cols / 4, src.rows / 4));
	
	// Create an all white mask
	Mat src_mask = 255 * Mat::ones(new_src.rows, new_src.cols, new_src.depth());
	// The location of the center of the src in the dst
	Point center(50, 50);

	// Seamlessly clone src into dst and put the results in output
	Mat normal_clone;
	Mat mixed_clone;

	Mat roi = dst.colRange(0, 100).rowRange(0, 100);
	roi = 0;

	//rectangle(dst, Rect(0, 0, dst.cols, dst.rows), Scalar(0, 0, 0),2);

	pc::DIY_Cloning dc;

	dc.seamlessClone(new_src, dst, src_mask, center, normal_clone, NORMAL_CLONE);
	dc.seamlessClone(new_src, dst, src_mask, center, mixed_clone, MIXED_CLONE);

	imshow("normal_clone", normal_clone);
	imshow("mixed_clone", mixed_clone);
	waitKey();
	// Save results
	//imwrite("opencv-normal-clone-example.jpg", normal_clone);
	//imwrite("opencv-mixed-clone-example.jpg", mixed_clone);

	return 0;
}

int main(){
#if 0
	Mat src = imread("test2.png");
	//Mat fullPatch = imread("sky2.jpg");
	Mat dest = src.rowRange(0, 20).colRange(535, 555).clone();
	Mat patch = src.rowRange(0, 20).colRange(537, 557).clone();
	//Mat patch = fullPatch.rowRange(920, 940).colRange(0, 20).clone();
	Mat mask = Mat::ones(dest.size(), CV_8UC1) * 255;

	Rect r(0, 0, 10, 10);
	dest(r) = 0;
	mask(r) = 0;
#else
	Mat dest = imread("desttemp.png");
	Mat patch = imread("srctemp.png");
	Mat mask = imread("tempmask.png", IMREAD_GRAYSCALE);

	Mat temp = Mat::ones(dest.size(), CV_8UC3) * 255;
	temp.copyTo(dest, (mask == 0));
#endif
	pc::DIY_Cloning dc;
	dc.patchClone(dest, patch, mask);

	return 0;
}