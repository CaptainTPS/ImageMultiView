//rewrite by TPS (opencv3  seamless cloning

#ifndef _POISSON_CLONE_H_
#define _POISSON_CLONE_H_

//#include "D:\Opencv3\opencv\build\include\opencv2/core/private.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/photo.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/photo.hpp"
#include "opencv2\core\mat.hpp"

#include <vector>

namespace pc
{

	class DIY_Cloning
	{
	public:
		void normalClone(const cv::Mat& destination, const cv::Mat &mask, const cv::Mat &wmask, cv::Mat &cloned, int flag);
		void illuminationChange(cv::Mat &I, cv::Mat &mask, cv::Mat &wmask, cv::Mat &cloned, float alpha, float beta);
		void localColorChange(cv::Mat &I, cv::Mat &mask, cv::Mat &wmask, cv::Mat &cloned, float red_mul, float green_mul, float blue_mul);
		void textureFlatten(cv::Mat &I, cv::Mat &mask, cv::Mat &wmask, float low_threshold, float high_threhold, int kernel_size, cv::Mat &cloned);

		void seamlessClone(cv::InputArray src, cv::InputArray dst, cv::InputArray mask, cv::Point p,
			cv::OutputArray blend, int flags);

		void patchClone(cv::Mat& dest, cv::Mat& patch, cv::Mat& mask);

	protected:

		void initVariables(const cv::Mat &destination, const cv::Mat &binaryMask);
		void computeDerivatives(const cv::Mat &destination, const cv::Mat &patch, const cv::Mat &binaryMask);
		void scalarProduct(cv::Mat mat, float r, float g, float b);
		void poisson(const cv::Mat &destination);
		void evaluate(const cv::Mat &I, const cv::Mat &wmask, const cv::Mat &cloned);
		void dst(const cv::Mat& src, cv::Mat& dest, bool invert = false);
		void idst(const cv::Mat& src, cv::Mat& dest);
		void solve(const cv::Mat &img, cv::Mat& mod_diff, cv::Mat &result);

		void poissonSolver(const cv::Mat &img, cv::Mat &gxx, cv::Mat &gyy, cv::Mat &result);

		void arrayProduct(const cv::Mat& lhs, const cv::Mat& rhs, cv::Mat& result) const;

		void computeGradientX(const cv::Mat &img, cv::Mat &gx);
		void computeGradientY(const cv::Mat &img, cv::Mat &gy);
		void computeLaplacianX(const cv::Mat &img, cv::Mat &gxx);
		void computeLaplacianY(const cv::Mat &img, cv::Mat &gyy);

	private:
		std::vector <cv::Mat> rgbx_channel, rgby_channel, output;
		cv::Mat destinationGradientX, destinationGradientY;
		cv::Mat patchGradientX, patchGradientY;
		cv::Mat binaryMaskFloat, binaryMaskFloatInverted;

		std::vector<float> filter_X, filter_Y;

		//added TPS
		cv::Mat dest_dx, dest_dy;
		cv::Mat patch_dx, patch_dy;
		cv::Mat dx, dy;
	};
}
#endif