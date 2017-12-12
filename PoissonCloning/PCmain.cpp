#include "PoissonClone.h"

#include "opencv2\highgui\highgui.hpp"
#include "opencv2\imgproc\imgproc.hpp"

#include <iostream>

using namespace cv;

int main1(){
	//offical demo
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

int main2(){
// this is a demo
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

int main(){
	int category = 5;
	int cnt = 120;
	String folder = "dataFiles//";
	String orifolder = "original//";
	String fakefolder = "fakes//";
	String maskfolder = "masks//";
	for (int ca = 1; ca <= category; ca++)
	{
		int fourcc = CV_FOURCC('M', 'J', 'P', 'G');
		std::string file = "seqs_" + std::to_string(ca) + "_poisson.avi";
		std::string file2 = "seqs_" + std::to_string(ca) + "_fake.avi";
		VideoWriter cloned(folder + file, fourcc, 20, Size(640, 480), true);
		VideoWriter fakes(folder + file2, fourcc, 20, Size(640, 480), true);
		for (int i = 0; i < cnt; i++){
			
			String srcfile = "disparityImg"+std::to_string(ca)+"_"+std::to_string(i)+".png";
			String patchfile = std::to_string((ca - 1) * cnt + i) + ".png";
			String maskfile = "disparityMask"+std::to_string(ca)+"_"+std::to_string(i)+".png";
			//String outfile = "output.png";
			Mat dest = imread(folder + orifolder + srcfile);
			Mat patch = imread(folder + fakefolder + patchfile);
			Mat mask = imread(folder + maskfolder + maskfile, IMREAD_GRAYSCALE);

			//if fake is not 640 * 480
			if (patch.size() != mask.size()){
				int h = mask.rows;
				int w = mask.cols;
				int hdiff = (patch.rows - h) / 2;
				int wdiff = (patch.cols - w) / 2;
				Rect rec(wdiff, hdiff, w, h);
				patch = patch(rec).clone();
			}

			if (1){
				//fix dest empty part
				Mat maskFloat, destFloat, patchFloat;
				std::vector <Mat> destFloat_channels, patchFloat_channels;
				mask.convertTo(maskFloat, CV_32FC1, 1.0 / 255.0);
				dest.convertTo(destFloat, CV_32FC3, 1.0 / 255.0);
				patch.convertTo(patchFloat, CV_32FC3, 1.0 / 255.0);

				split(destFloat, destFloat_channels);
				split(patchFloat, patchFloat_channels);

				for (int i = 0; i < 3; i++)
				{
					destFloat_channels[i] = destFloat_channels[i] + patchFloat_channels[i].mul(maskFloat);
				}

				merge(destFloat_channels, destFloat);

				destFloat.convertTo(dest, CV_8UC3, 255.0);
			
				//imwrite("testout.png", dest);
				//return 0;
			}


			fakes << patch;
			pc::DIY_Cloning dc;
			Mat output;
			dc.patchClone2(patch, dest, mask, output, cv::NORMAL_CLONE);

			//imwrite(outfile, output);
			cloned << output;
		}
		cloned.release();
		fakes.release();
		std::cout << "now to video: " << ca << std::endl;
	}
	return 0;
}

#if 0
int main(){
	//test
	int fourcc = CV_FOURCC('M', 'P', 'G', '4');
	std::string file = "seqs.mp4";
	Mat img = Mat::ones(Size(640, 480), CV_8UC3) * 255;
	VideoWriter cloned(file, fourcc, 20, Size(640, 480), true);
	
	for (int i = 0; i < 100; i++)
	{
		cloned << img;
	}
	cloned.release();
	return 0;
}
#endif