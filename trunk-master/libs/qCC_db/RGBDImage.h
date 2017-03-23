#ifndef RGBD_IMAGE_HEADER
#define RGBD_IMAGE_HEADER

#include "ccImage.h"

//tps
class QCC_DB_LIB_API RGBDImage : public ccImage{
public:
	//! Default constructor
	RGBDImage();

	//! Constructor from QImage
	RGBDImage(QImage& image, QString& name = QString("unknown"));

	bool isDepthGot();

	void setHasDepth(bool flag);

	//! Returns unique class ID
	virtual CC_CLASS_ENUM getClassID() const override { return CC_TYPES::RGBD_IMAGE; }

protected:
	bool hasDepth;
};

#endif