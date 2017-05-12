#include "PoissonClone.h"

#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

using namespace cv;

int main(){
	Mat src = imread("airplane.jpg");
	Mat dst = imread("sky2.jpg");
	Mat new_src;
	resize(src, new_src, Size(src.cols / 4, src.rows / 4));
	
	// Create an all white mask
	Mat src_mask = 255 * Mat::ones(new_src.rows, new_src.cols, new_src.depth());
	// The location of the center of the src in the dst
	Point center(800, 150);

	// Seamlessly clone src into dst and put the results in output
	Mat normal_clone;
	Mat mixed_clone;

	Mat roi = dst.colRange(center.x - 140, center.x + 140).rowRange(center.y - 90, center.y + 90);
	roi = 0;

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