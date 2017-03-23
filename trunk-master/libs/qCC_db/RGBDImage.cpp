#include "RGBDImage.h"

//tps
RGBDImage::RGBDImage() : ccImage()
{
	hasDepth = false;
}

RGBDImage::RGBDImage(QImage& image, QString& name)
	: ccImage(image, name)
{
	hasDepth = false;
}

bool RGBDImage::isDepthGot()
{
	return hasDepth;
}

void RGBDImage::setHasDepth(bool flag = true){
	hasDepth = flag;
}