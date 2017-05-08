#include <fstream>
#include <vector>
#include <iostream>

#include <QThread>

#include "dealDepthDlg.h"


#include <ccHObjectCaster.h>
#include <qsettings.h>
#include <ccPersistentSettings.h>
#include <FileIOFilter.h>
#include <qfiledialog.h>
#include <ccGLWindow.h>
#include <BinFilter.h>
#include <qoffscreensurface.h>

//#include "opencv2/imgproc/imgproc.hpp"
//#include "opencv2/highgui/highgui.hpp"
//#include "GL\glew.h"
//#include "glut.h"


//#include "patchDealer.h"
#include "newPatchDealer.h"
#include "PIheader.h"
#include "apss.h"
#include "MeshRender.h" 
//#include "MeshRenderQt.h"//this is not used cannot get depth out
#include "videoThread.h"



#define DEBUG_TEST

//#define USE_APSS


//default 'All files' file filter
static const QString s_allFilesFilter("All (*.*)");
//default file filter separator
static const QString s_fileFilterSeparator(";;");


//whether it is proper to put it here?
AlgebraicSurface as;


DealDepthDlg::DealDepthDlg(QWidget* parent/*=0*/)
	:QWidget(parent)
	, Ui::dealDepthDlg()
{
	setupUi(this);

	connect(addButton,		SIGNAL(clicked()), this, SLOT(addDepthImage()));
	connect(playButton,		SIGNAL(clicked()), this, SLOT(Buttonplay()));
	connect(newViewButton, SIGNAL(clicked()), this, SLOT(getMultiView()));
	connect(&vthread,		SIGNAL(play()),	this, SLOT(playVideo()));

	frame = 0;
	frameflag = -1;

	//mm
	bdstart = -30;
	bdend = 30;
	viewNum = 30;


	show();
}

void DealDepthDlg::connectOBJ(ccHObject* obj){
	this->m_obj = obj;
}

void DealDepthDlg::addDepthImage(){
	if (m_obj->isA(CC_TYPES::RGBD_IMAGE)){
		RGBDImage* rgbdi = ccHObjectCaster::ToRGBDImage(m_obj);

		//open a dialog to choose file
		//persistent settings
		QSettings settings;
		settings.beginGroup(ccPS::LoadFile());
		QString currentPath = settings.value(ccPS::CurrentPath(), QApplication::applicationDirPath()).toString();
		QString currentOpenDlgFilter = settings.value(ccPS::SelectedInputFilter(), BinFilter::GetFileFilter()).toString();

		// Add all available file I/O filters (with import capabilities)
		QStringList fileFilters;
		fileFilters.append(s_allFilesFilter);
		bool defaultFilterFound = false;
		{
			const FileIOFilter::FilterContainer& filters = FileIOFilter::GetFilters();
			for (size_t i = 0; i<filters.size(); ++i)
			{
				if (filters[i]->importSupported())
				{
					QStringList ff = filters[i]->getFileFilters(true);
					for (int j = 0; j<ff.size(); ++j)
					{
						fileFilters.append(ff[j]);
						//is it the (last) default filter?
						if (!defaultFilterFound && currentOpenDlgFilter == ff[j])
						{
							defaultFilterFound = true;
						}
					}
				}
			}
		}

		//default filter is still valid?
		if (!defaultFilterFound)
			currentOpenDlgFilter = s_allFilesFilter;

		//file choosing dialog
		QStringList selectedFiles = QFileDialog::getOpenFileNames(this,
			"Open file(s)",
			currentPath,
			fileFilters.join(s_fileFilterSeparator),
			&currentOpenDlgFilter
			//#ifdef QT_DEBUG
			, QFileDialog::DontUseNativeDialog
			//#endif
			);
		if (selectedFiles.isEmpty())
			return;
		//save last loading parameters
		currentPath = QFileInfo(selectedFiles[0]).absolutePath();
		settings.setValue(ccPS::CurrentPath(), currentPath);
		settings.setValue(ccPS::SelectedInputFilter(), currentOpenDlgFilter);
		settings.endGroup();

		//to use the same 'global shift' for multiple files
		CCVector3d loadCoordinatesShift(0, 0, 0);
		bool loadCoordinatesTransEnabled = false;

		FileIOFilter::LoadParameters parameters;
		{
			parameters.alwaysDisplayLoadDialog = true;
			parameters.shiftHandlingMode = ccGlobalShiftManager::DIALOG_IF_NECESSARY;
			parameters.coordinatesShift = &loadCoordinatesShift;
			parameters.coordinatesShiftEnabled = &loadCoordinatesTransEnabled;
			parameters.parentWidget = this;
		}

		//the same for 'addToDB' (if the first one is not supported, or if the scale remains too big)
		CCVector3d addCoordinatesShift(0, 0, 0);
		ccGLWindow* destWin = 0;

		if (selectedFiles.size() > 1)
		{
			ccLog::Warning(QString("can not add more than one depth image"));
			return;
		}
		//read file
		CC_FILE_ERROR result = CC_FERR_NO_ERROR;
		ccHObject* newGroup = FileIOFilter::LoadFromFile(selectedFiles[0], parameters, result, currentOpenDlgFilter);

		if (newGroup)
		{
			if (destWin)
			{
				newGroup->setDisplay_recursive(destWin);
			}
			//addToDB(newGroup, true, true, false);

		}

		if (!newGroup)
			return;

		//add child
		ccHObject* parentObject = newGroup->getChild(0)->getParent();
		parentObject = rgbdi;
		ccImage* depthImage = new ccImage(*(static_cast<ccImage*>(newGroup->getChild(0))));
		rgbdi->addChild(depthImage);
		//newGroup->detachChild(newGroup->getChild(0));
		delete newGroup;

		rgbdi->setHasDepth(true);
	}
}

void DealDepthDlg::Buttonplay(){
	if (video == NULL){
		int m = m_obj->getChildrenNumber();
		for (int i = 0; i < m; i++)
		{
			ccHObject* t = m_obj->getChild(i);
			QString n = t->getName();
			if (n == "VIDEO"){
				video = t;
			}
		}
	}
	if (video == NULL){
		ccLog::Print("No video detect!");
		return;
	}

	pause = !pause;
	QString bname = pause ? "Play Video" : "PAUSE";
	playButton->setText(bname);

	if (pause){
		vthread.stopplay();
	}
	else{
		vthread.startplay();
	}

}

void DealDepthDlg::playVideo(){
	
	ccHObject* f = video->getChild(frame);
	f->setVisible(false);
	int n = video->getChildrenNumber();
	if (frame == 0 || frame == n-1){
		frameflag *= -1;
	}
	frame += frameflag;
	frame = (frame + n) % (n);
	std::cout << frame << std::endl;
	f = video->getChild(frame);
	ccGenericGLDisplay* test = video->getDisplay();
	if (test == NULL)
		return;
	f->setDisplay(test);
	f->setVisible(true);
	f->prepareDisplayForRefresh();
	f->refreshDisplay();
}

void RGBtoDepth(int r, int g, int b, float *depth){
	*depth = 1.0f * ((r << 16) + (g << 8) + b) / 16777216.0f;
}

void RGBtoDepth(QRgb rgb, float *depth){
	int r = qRed(rgb);
	int g = qGreen(rgb);
	int b = qBlue(rgb);
	RGBtoDepth(r, g, b, depth);
}

QRgb DepthtoRGB(float depth, int *r = NULL, int *g = NULL, int *b = NULL){
	int d = depth * 16777216;
	int r_, g_, b_;
	if (r == NULL)
	{
		r = &r_;
	}
	if (g == NULL )
	{
		g = &g_;
	}
	if (b == NULL)
	{
		b = &b_;
	}
	*r = (d >> 16);
	*g = (d - ((*r) << 16)) >> 8;
	*b = d - ((*r) << 16) - ((*g) << 8);

	return qRgb(*r, *g, *b);
}

void storeOBJ(string name, QImage depthImg, QImage srcImg){
	//camera parameters
	cameraPara cam;
	float angleOfView = cam.angleOfView_vertical;
	float focalLength = cam.focalLength;
	float nearPlane = cam.nearPlane;
	float farPlane = cam.farPlane;
	int width = cam.width;
	int height = cam.height;
	float screenRatio = 1.0* width / height;
	//half of the image plane
	float y_length = focalLength * tan(angleOfView / 2 * M_PI / 180.0);
	float x_length = y_length * screenRatio;

	std::ofstream output(name);
	for (int row = 0; row < height; row++)
	{
		for (int col = 0; col < width; col++)
		{
			QRgb dep = depthImg.pixel(col, row);
			int r = qRed(dep);
			int g = qGreen(dep);
			int b = qBlue(dep);
			//if (r != 0)
			//{
			float depth = 1.0f * ((r << 16) + (g << 8) + b) / 16777216.0f;
			float z = -1.0 * (nearPlane + (farPlane - nearPlane) * (1 - depth));
			float x = 1.0*(col - (width / 2)) / (width / 2) * (-z) * tan(angleOfView / 2 * M_PI / 180.0) * screenRatio;
			float y = 1.0*((height / 2) - row) / (height / 2) * (-z) * tan(angleOfView / 2 * M_PI / 180.0);
			//out
			QRgb imgrgb = srcImg.pixel(col, row);
			if (depth<0.0001)
			{
				continue;
			}
			output << "v " << x << " " << y << " " << z << " " << qRed(imgrgb) << " " << qGreen(imgrgb) << " " << qBlue(imgrgb) << std::endl;
			//}
		}
	}
	output.close();

}

void adjustObjImg(QImage *srcImg, QImage *srcDepth, QImage *objImg, QImage *objDepth, QImage *objMask, int srcX, int objX, int posY, double weight){
	
	if (objX < objImg->width() && objX >= 0)
	{
		QRgb dep_dis = objDepth->pixel(objX, posY);
		float depth_dis;
		RGBtoDepth(dep_dis, &depth_dis);
		//暂时没管obj的depth的生成

		//mask part
		QRgb objM = objMask->pixel(objX, posY);
		int r_m = qRed(objM);
		int g_m = qGreen(objM);
		int b_m = qBlue(objM);
		
		r_m -= 255 * weight;
		g_m -= 255 * weight;
		b_m -= 255 * weight;
		
		r_m = r_m < 0 ? 0 : r_m;
		g_m = g_m < 0 ? 0 : g_m;
		b_m = b_m < 0 ? 0 : b_m;

		objMask->setPixel(objX, posY, qRgb(r_m, g_m, b_m));

		//image part
		QRgb srcRGB = srcImg->pixel(srcX, posY);
		int r = qRed(srcRGB);
		int g = qGreen(srcRGB);
		int b = qBlue(srcRGB);

		QRgb objRGB = objImg->pixel(objX, posY);
		int r_obj = qRed(objRGB);
		int g_obj = qGreen(objRGB);
		int b_obj = qBlue(objRGB);

		r_obj += r*weight;
		g_obj += g*weight;
		b_obj += b*weight;

		r_obj = r_obj > 255 ? 255 : r_obj;
		g_obj = g_obj > 255 ? 255 : g_obj;
		b_obj = b_obj > 255 ? 255 : b_obj;

		objImg->setPixel(objX, posY, qRgb(r_obj, g_obj, b_obj));
	}
}

void changeCamPara(QImage &srcImg, QImage &srcDepth, cameraPara &cam){
	double bd = cam.baseDistance;
	double f = cam.focalLength;

	//test
	double dep_h = 0;
	double dep_l = 123456;

	double dep_float_h = 0;
	double dep_float_l = 123456;
	//end
	for (size_t i = 0; i < srcImg.height(); i++)
	{
		for (size_t j = 0; j < srcImg.width(); j++)
		{
			QRgb dep = srcDepth.pixel(j, i);
			int r = qRed(dep);
			int g = qGreen(dep);
			int b = qBlue(dep);
			float depth = 1.0f * ((r << 16) + (g << 8) + b) / 16777216.0f;
			double z_abs = (cam.nearPlane + (cam.farPlane - cam.nearPlane) * (depth)); // 1779.14 -- 13981.10
			//test
			if (r != 0 || g != 0 || b != 0)
			{
				dep_h = dep_h < (z_abs) ? (z_abs) : dep_h;
				dep_l = dep_l >(z_abs) ? (z_abs) : dep_l;

				dep_float_h = dep_float_h < (depth) ? (depth) : dep_float_h;
				dep_float_l = dep_float_l >(depth) ? (depth) : dep_float_l;
			}
		}
	}

	//reset focal length
	f = dep_l * cam.f_ratio;
	//reset base distance
	bd = f * cam.bd_ratio;

	cam.baseDistance = bd;
	cam.focalLength = f;
}

void setCamPara(cameraPara &cam, double baseDistance, double focalLength){
	//bd 0.32503142759203918
	//6.5006285518407827
	cam.baseDistance = baseDistance;
	cam.focalLength = focalLength;
};

void forwardMappingWeightOverlaping(QImage &srcImg, QImage &depthImg, QImage &objImg, QImage &objDepth, QImage &objMask, cameraPara &cam){
	//use disparity to get new views

	double f = cam.focalLength;
	double bd = cam.baseDistance;
	double width_pixel = cam.width;
	double width_notpixel = 2.0 * f * tan(cam.angleOfView / 2 * M_PI / 180.0);
	
	for (int i = 0; i < srcImg.height(); i++)
	{
		for (int j = 0; j < srcImg.width(); j++)
		{
			QRgb dep = depthImg.pixel(j, i);
			int r = qRed(dep);
			int g = qGreen(dep);
			int b = qBlue(dep);
			float depth = 1.0f * ((r << 16) + (g << 8) + b) / 16777216.0f;

			double z_abs = (cam.nearPlane + (cam.farPlane - cam.nearPlane) * (depth)); // 1779.14 -- 13981.10
			double disparity = bd * (1 - (z_abs - f) / z_abs);
			double d_pixel = disparity / width_notpixel * width_pixel;
			int p_x = (int)(j - d_pixel);
			if (p_x == 0)
				p_x--; //when j - d_pixel < 0, (int) is different
			double weight = (double)j - d_pixel - p_x;
			p_x++;
			adjustObjImg(&srcImg, &depthImg, &objImg, &objDepth, &objMask, j, p_x, i, weight);
			p_x--;
			weight = 1 - weight;
			adjustObjImg(&srcImg, &depthImg, &objImg, &objDepth, &objMask, j, p_x, i, weight);
		}
	}
}

void findContour(QImage &srcDepth, QImage &contourMask, float contourThresh){
	QRgb tempRGB;
	float depth;
	float d_result;

	//float max_t = 0;
	//float min_t = 100;

	for (int i = 0; i < srcDepth.height(); i++)
	{
		for (int j = 0; j < srcDepth.width(); j++)
		{
			d_result = 0;
			//around (i, j)
			for (int m = -1; m < 2; m++)
			{
				for (int n = -1; n < 2; n++)
				{
					int x = j + n;
					int y = i + m;
					x = x < 0 ? (x + 1) : x;
					y = y < 0 ? (y + 1) : y;
					x = (x == srcDepth.width()) ? (x - 1) : x;
					y = (y == srcDepth.height()) ? (y - 1) : y;
					tempRGB = srcDepth.pixel(x, y);
					RGBtoDepth(tempRGB, &depth);
					d_result += depth;
				}
			}
			tempRGB = srcDepth.pixel(j, i);
			RGBtoDepth(tempRGB, &depth);
			d_result -= 9 * depth;

			//test
			//float t_d = abs(d_result);
			//max_t = max_t > t_d ? max_t : t_d;
			//min_t = min_t < t_d ? min_t : t_d;
			
			if (abs(d_result) > contourThresh)
			{
				contourMask.setPixel(j, i, 1);//1 for the removing contour 
				//expand one pixel
				for (int m = -1; m < 1; m++)
				{
					for (int n = -1; n < 1; n++)
					{
						int x = j + n;
						int y = i + m;
						if (x < 0 || y< 0||x >= srcDepth.width() || y >= srcDepth.height())
							continue;
						contourMask.setPixel(x, y, 1); 
					}
				}
			}
		}
	}

}

void doWarp(QImage &srcImg, QImage &srcDepth, QImage &contourMask, QImage &objImg, QImage &objDepth, QImage &objMask, cameraPara &cam){
	//use disparity to get new views

	double f = cam.focalLength;
	double bd = cam.baseDistance;

	double width_pixel = cam.width;
	double width_notpixel = 2.0 * f * tan(cam.angleOfView / 2 * M_PI / 180.0);

	bool isContour;
	for (int i = 0; i < srcImg.height(); i++)
	{
		for (int j = 0; j < srcImg.width(); j++)
		{
			isContour = (contourMask.pixel(j, i)) & 1;
			if (isContour)
				continue;

			QRgb dep = srcDepth.pixel(j, i);
			float depth;
			RGBtoDepth(dep, &depth);
			double z_abs = (cam.nearPlane + (cam.farPlane - cam.nearPlane) * (depth)); // 13 --- 2000
			double disparity = bd * (1 - (z_abs - f) / z_abs);
			double d_pixel = disparity / width_notpixel * width_pixel;


			int p_x = (int)(j - d_pixel);
			if (p_x < 0 || p_x >= srcImg.width())
				continue;

			QRgb dep_obj = objDepth.pixel(p_x, i);
			float depth_obj;
			RGBtoDepth(dep_obj, &depth_obj);
			if (depth < depth_obj)
			{
				objDepth.setPixel(p_x, i, dep);
				objImg.setPixel(p_x, i, srcImg.pixel(j, i));
				objMask.setPixel(p_x, i, qRgb(0, 0, 0));
			}
		}
	}

}

void eliminateCracks(QImage &srcImg, QImage &srcDepth, QImage &objImg, QImage &objDepth, QImage &objMask, float threshold, cameraPara &cam){
	vector<QPoint> cracks;
	vector<float> crackDepth;

	//median filter
	vector<float> depthBlock;
	QImage objDepthTemp = objDepth;
	QRgb d_RGB;
	float depth;
	for (int i = 0; i < objDepth.height(); i++)
	{
		for (int j = 0; j < objDepth.width(); j++)
		{
			QRgb temp = objMask.pixel(j, i);
			if (qRed(temp) == 0 && qGreen(temp) == 0 && qBlue(temp) == 0)
				continue;

			depthBlock.clear();
			for (int m = -1; m <= 1; m++)
			{
				for (int n = -1; n <= 1; n++)
				{
					int x = j + n;
					int y = i + m;
					x = x < 0 ? (x + 1) : x;
					y = y < 0 ? (y + 1) : y;
					x = (x == srcDepth.width()) ? (x - 1) : x;
					y = (y == srcDepth.height()) ? (y - 1) : y;
					d_RGB = objDepth.pixel(x, y);
					RGBtoDepth(d_RGB, &depth);
					depthBlock.push_back(depth);
				}
			}
			sort(depthBlock.begin(), depthBlock.end());
			d_RGB = objDepth.pixel(j, i);
			RGBtoDepth(d_RGB, &depth);
			if (abs(depth - depthBlock[4]) > threshold)
			{
				QPoint tPoint(j, i);
				cracks.push_back(tPoint);
				crackDepth.push_back(depthBlock[4]);//在crack 和contour交界的地方判断是不是crack不是很准确
			}
		}
	}

	//deal with cracks
	double bd = cam.baseDistance;
	double f = cam.focalLength;
	double width_pixel = cam.width;
	double width_notpixel = 2.0 * f * tan(cam.angleOfView / 2 * M_PI / 180.0);
	QRgb src1, src2;
	for (int i = 0; i < cracks.size(); i++)
	{
		depth = crackDepth[i];
		double z_abs = (cam.nearPlane + (cam.farPlane - cam.nearPlane) * (depth)); // 1779.14 -- 13981.10
		double disparity = -1 * bd * (1 - (z_abs - f) / z_abs);
		double d_pixel = disparity / width_notpixel * width_pixel;
		double p_x = cracks[i].x() - d_pixel;
		if ((int)p_x < 0 || (int)(p_x+1) >= srcImg.width())
			continue;
		double weight = 1 - (p_x - (int)p_x);
		src1 = srcImg.pixel((int)p_x, cracks[i].y());
		src2 = srcImg.pixel((int)(p_x + 1), cracks[i].y());
		int r = (int)(qRed(src1) * weight + qRed(src2) * (1 - weight));
		int g = (int)(qGreen(src1) * weight + qGreen(src2) * (1 - weight));
		int b = (int)(qBlue(src1) * weight + qBlue(src2) * (1 - weight));
		objImg.setPixel(cracks[i], qRgb(r, g, b));

		objMask.setPixel(cracks[i], qRgb(0, 0, 0));
		objDepth.setPixel(cracks[i], DepthtoRGB(depth, &r,&g,&b));
	}

}

//paper Free-viewpoint depth image based rendering
void forwardMappingDepthImageBase(QImage &srcImg, QImage &srcDepth, QImage &objImg, QImage &objDepth, QImage &objMask, cameraPara &cam){
	float contourThreshold = 0.1;
	QImage contourMask(srcDepth.width(),srcDepth.height(),QImage::Format_MonoLSB);// less significant bit (LSB) first
	contourMask.fill(0);
	//contourMask.setPixel(0, 0, 1);
	//bool n = (contourMask.pixel(0, 0)) & 1;
#if 1
	findContour(srcDepth, contourMask, contourThreshold);
#endif
	//test contour
	srcDepth.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\depth.png");
	contourMask.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\contour.png");
	/*QImage testimg(cam.width, cam.height, QImage::Format_RGB32);
	testimg.fill(qRgb(0, 0, 0));
	for (int i = 0; i < cam.height; i++)
	{
		for (int j = 0; j < cam.width; j++)
		{
			if ((contourMask.pixel(j, i)) & 1){
				testimg.setPixel(j, i, qRgb(255, 255, 255));
			}
		}
	}
	testimg.save("..\\data\\out\\contourTest.png");*/
	//test end
	doWarp(srcImg, srcDepth, contourMask, objImg, objDepth, objMask, cam);

	float crackThreshold = 0.0004;
	eliminateCracks(srcImg, srcDepth, objImg, objDepth, objMask, crackThreshold, cam);
}

void addAroundPoints(cameraPara &m_cam, newPatchDealer &patch, QImage &depth, int col, int row, double lengthPerPixel, double gapThreshold){
	//center
	QRgb dep = depth.pixel(col, row);
	float d;
	RGBtoDepth(dep, &d);
	float d_z = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (d));
	double x = (col - m_cam.width / 2)*lengthPerPixel *(d_z / m_cam.focalLength);
	double y = (m_cam.height / 2 - row)*lengthPerPixel *(d_z / m_cam.focalLength);
	patch.setCenterPoint(x, y, d_z* (-1));
	patch.addPoint(x, y, d_z* (-1), gapThreshold);

	int _col, _row;
	//surround 1
	static int round1[8][2] = {
		{ 0, -1 }, { 1, -1 }, { 1, 0 }, { 1, 1 },
		{ 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }
	};
	for (int i = 0; i < 8; i++)
	{
		_col = col + round1[i][0];
		_row = row + round1[i][1];
		if (_col < 0 || _col >= m_cam.width || _row < 0||_row >=m_cam.height)
		{
			continue;
		}
		dep = depth.pixel(_col, _row);
		RGBtoDepth(dep, &d);
		d_z = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (d));
		x = (_col - m_cam.width / 2)*lengthPerPixel *(d_z / m_cam.focalLength);
		y = (m_cam.height / 2 - _row)*lengthPerPixel *(d_z / m_cam.focalLength);
		patch.addPoint(x, y, d_z* (-1), gapThreshold);
	}

	//surround 2
	static int round2[16][2] = {
		{ 0, -2 }, { 1, -2 }, { 2, -2 }, 
		{ 2, -1 }, { 2, 0 }, { 2, 1 }, { 2, 2 },
		{ 1, 2 }, { 0, 2 }, { -1, 2 }, { -2, 2 },
		{ -2, 1 }, { -2, 0 }, { -2, -1 }, { -2, -2 },
		{ -1, -2 }
	};
	for (int i = 0; i < 16; i++)
	{
		_col = col + round2[i][0];
		_row = row + round2[i][1];
		if (_col < 0 || _col >= m_cam.width || _row < 0 || _row >= m_cam.height)
		{
			continue;
		}
		dep = depth.pixel(_col, _row);
		RGBtoDepth(dep, &d);
		d_z = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (d));
		x = (_col - m_cam.width / 2)*lengthPerPixel *(d_z / m_cam.focalLength);
		y = (m_cam.height / 2 - _row)*lengthPerPixel *(d_z / m_cam.focalLength);
		patch.addPoint(x, y, d_z* (-1), gapThreshold);
	}
}

void DealDepthDlg::doMarchingCube(ccImage* nview, double lengthPerPixel, cameraPara &m_cam, int dilateVoxelNum){
	ccLog::Print("start calculate the mesh.");
	QImage pic = nview->data();
	if (nview->getChild(0) == NULL)
	{
		ccLog::Error("[new view] didn't find depth image! ");
		return;
	}
	QImage depth = ccHObjectCaster::ToImage(nview->getChild(0))->data();

	//test
	int testx = 250;
	int testy = 200;
	for (int i = 0; i < 10; i++)
	{
		for (int j = 0; j < 10; j++)
		{
			//depth.setPixel(testx+i, testy+j, qRgb(0,0,0));
		}
	}
	//depth.setPixel(testx - 6, testy + 4, qRgb(255, 255, 255));
	//depth.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\test_d.png");


	newPatchDealer patch;
	//AlgebraicSurface as;
	int size_temp = 512;
	int size_ddd[3];
	size_ddd[0] = size_temp;
	size_ddd[1] = size_temp;
	size_ddd[2] = size_temp;
	// don't care about the margin
	as.initSpheres((pic.height()) * (pic.width()));
	as.setVolumeSize(size_ddd[0], size_ddd[1], size_ddd[2]);

#if 1
	AlgebraicSurface::point3 corner1, corner2;
	corner1.px = corner1.py = corner1.pz = 10000;
	corner2.px = corner2.py = corner2.pz = -10000;

	////test
	//std::string p = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\point.obj";
	//ofstream out(p);
	//QImage test_mask(pic.width(), pic.height(),QImage::Format_RGB32);
	//test_mask.fill(qRgb(0, 0, 0));

	//test
	/*float* bufferp = new float[(pic.height() - 2)*(pic.width() - 2) * 3];
	float* buffers = new float[(pic.height() - 2)*(pic.width() - 2) * 5];
	int countmark = 0;*/

	std::cout << "calculating spheres... ";
	float dtof[5];
	float gapThreshold = (m_cam.farPlane - m_cam.nearPlane) * 0.05;

	for (int i = 0; i < pic.height(); i++)
	{
		for (int j = 0; j < pic.width(); j++)
		{
			//ignore the background
			QRgb dep = depth.pixel(j, i);
			if (qRed(dep) == 255 && qGreen(dep) == 255 && qBlue(dep) == 255){
				//test
				/*bufferp[countmark * 3] = bufferp[countmark * 3 + 1] = bufferp[countmark * 3 + 2] = 0;
				buffers[countmark * 5] = buffers[countmark * 5 + 1] = buffers[countmark * 5 + 2] = buffers[countmark * 5 + 3] = buffers[countmark * 5 + 4] = 0;
				countmark++;*/
				continue;
			}

			
			////test
			//QRgb de = depth.pixel(j, i);
			//float d;
			//RGBtoDepth(de, &d);
			//float d_z = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (d));
			//double x = (j - m_cam.width / 2)*lengthPerPixel *(d_z / m_cam.focalLength);
			//double y = (i - m_cam.height / 2)*lengthPerPixel *(d_z / m_cam.focalLength);
			//out << "v " << x << " " << y << " " << d_z << " " << 255 << " " << 0 << " " << 0 << endl;

			//patch.setCenter(j, i);
			//int v = 0;
			//while (v < 9)
			//{
			//	int col = j + (v % 3) - 1;
			//	int row = i + (v / 3) - 1;

			//	QRgb dep = depth.pixel(col, row);
			//	float d;
			//	RGBtoDepth(dep, &d);
			//	float d_z = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (d));

			//	QRgb pixel = pic.pixel(col, row);
			//	int r = qRed(pixel);
			//	int g = qGreen(pixel);
			//	int b = qBlue(pixel);

			//	//use opengl coordinate frame: x axis to right, y axis to up, right hand
			//	double x = (col - m_cam.width / 2)*lengthPerPixel *(d_z / m_cam.focalLength);
			//	double y = (m_cam.height / 2 - row)*lengthPerPixel *(d_z / m_cam.focalLength);

			//	//set values
			//	patch.setColor(v, 0, r);
			//	patch.setColor(v, 1, g);
			//	patch.setColor(v, 2, b);
			//	patch.setDepth(v, (double)d_z * (-1));
			//	patch.setXY(v, x, y);

			//	v++;
			//}

			

			

			//patch.filterPoints(gapThreshold);

			/*int m = patch.getNumOfPoints();
			if (m < 5)
			{
			int n = 0;
			test_mask.setPixel(j, i, qRgb(255, 255, 255));
			}*/

			patch.clearData();

			addAroundPoints(m_cam, patch, depth, j, i, lengthPerPixel, gapThreshold);
			//test
			int in = as.getSphereNum();
			if (i/*y axis*/ == 200 && j/*x axis*/ == 250)
			{
				ofstream ooof("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\patchdata.txt");
				ooof << patch.getNumOfPoints() << endl;
				ooof << patch.CPoint[0] << " " << patch.CPoint[1] << " " << patch.CPoint[2] << " " << endl;
				for (int i = 0; i < patch.getNumOfPoints(); i++)
				{
					ooof << patch._points[i][0] << " " << patch._points[i][1] << " " << patch._points[i][2] << " " << endl;
				}
				ooof.close();
			}

			patch.calSphere();
			patch.renewNormals();
			patch.calSphereWithNormal();
			patch.adjustSphere();

			AlgebraicSurface::point3 pt;
			double ptx, pty, ptz;
			patch.getCenter(&ptx, &pty, &ptz);
			pt.px = ptx;
			pt.py = pty;
			pt.pz = ptz;
			int index = as.getSphereNum();
			for (int t = 0; t < 5; t++)
			{
				dtof[t] = patch.u[t];
			}
			as.setSphere(dtof, pt, index);

			//test
			/*bufferp[countmark * 3] = pt.px;
			bufferp[countmark * 3 + 1] = pt.py;
			bufferp[countmark * 3 + 2] = pt.pz;
			for (int t = 0; t < 5; t++)
			{
			buffers[countmark * 5 + t] = dtof[t];
			}
			countmark++;*/


			//comfirm the box
			corner1.px = corner1.px < pt.px ? corner1.px : pt.px;
			corner1.py = corner1.py < pt.py ? corner1.py : pt.py;
			corner1.pz = corner1.pz < pt.pz ? corner1.pz : pt.pz;

			corner2.px = corner2.px > pt.px ? corner2.px : pt.px;
			corner2.py = corner2.py > pt.py ? corner2.py : pt.py;
			corner2.pz = corner2.pz > pt.pz ? corner2.pz : pt.pz;
		}
	}

	std::cout << "done" << std::endl;
	//test_mask.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\test_mask.png");

	//out.close();

	std::cout << "corner1: " << corner1.px << " " << corner1.py << " " << corner1.pz << std::endl;
	std::cout << "corner2: " << corner2.px << " " << corner2.py << " " << corner2.pz << std::endl;

	//test
	/*ofstream out("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\allpoints.obj");
	AlgebraicSurface::point3 pt;
	for (int m = 0; m < as.getSphereNum(); m++){
	pt = as.getSpherePoint(m);
	out <<"v "<< pt.px << " " << pt.py << " " << pt.pz <<" "<< 255 << " " << 0 << " " << 0 << endl;
	}
	out.close();*/


	/*int loop = as.getSphereNum();
	cout << "number: " << loop << endl;
	cout << "count: " << countmark << endl;
	ofstream op("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\data_points.bi", ios::binary);
	ofstream osp("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\data_spheres.bi", ios::binary);

	float* temp;
	AlgebraicSurface::point3 pt;

	op.write((char*)bufferp, countmark * 3 * sizeof(float));
	osp.write((char*)buffers, countmark * 5 * sizeof(float));
	op.close();
	osp.close();

	int m = 1;
	pt = as.getSpherePoint(m);
	temp = as.getSphere(m);
	cout << pt.px << " " << pt.py << " " << pt.pz << endl;
	cout << bufferp[3 * m] << " " << bufferp[3 * m + 1] << " " << bufferp[3 * m + 2] << endl;
	for (int j = 0; j < 5; j++)
	{
	cout << temp[j] << " ";
	}
	cout << endl;
	for (int j = 0; j < 5; j++)
	{
	cout << buffers[5 * m + j] << " ";
	}
	cout << endl;

	delete[] bufferp;
	delete[] buffers;*/


	double co[3];
	double lth[3];
	lth[0] = corner2.px - corner1.px;
	lth[1] = corner2.py - corner1.py;
	lth[2] = corner2.pz - corner1.pz;
	co[0] = corner1.px - 0.5 * lth[0];
	co[1] = corner1.py - 0.5 * lth[1];
	co[2] = corner1.pz - 0.5 * lth[2];

	as.setVolumeBox(AlgebraicSurface::point3(co[0], co[1], co[2]), 2.0 * lth[0], 2.0 * lth[1], 2.0 * lth[2]);

	
	vector<vector<float>> z_depth;
	vector<float> temp_z;
	for (int row = 0; row < depth.height(); row++)
	{
		temp_z.clear();
		for (int col = 0; col < depth.width(); col++)
		{
			QRgb dep = depth.pixel(col, row);
			float d;
			RGBtoDepth(dep, &d);
			float d_z = (m_cam.nearPlane + (m_cam.farPlane - m_cam.nearPlane) * (d));
			d_z *= -1;
			temp_z.push_back(d_z);
		}
		z_depth.push_back(temp_z);
	}
	as.calVolumeSig(z_depth, m_cam.focalLength, lengthPerPixel, dilateVoxelNum);

	//test
	ofstream sptOut("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\spt_sig.obj");
	for (int i = 0; i < as.getSphereNum(); i++)
	{
	AlgebraicSurface::point3 pt = as.getSpherePoint(i);
	sptOut << "v " << pt.px << " " << pt.py << " " << pt.pz << " " << 255 << " " << 0 << " " << 0 << endl;
	}
	sptOut.close();

	/*
	ofstream vsigOut("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\vsig.obj");
	for (int x = 0; x < size_ddd[0]; x++)
	{
		for (int y = 0; y < size_ddd[1]; y++)
		{
			for (int z = 0; z < size_ddd[2]; z++)
			{
				if (*as.volumeSig(x, y, z) == true)
				{
					AlgebraicSurface::point3 pt = as.voluemCoor(x, y, z);
					vsigOut << "v " << pt.px << " " << pt.py << " " << pt.pz << " " << 0 << " " << 255 << " " << 0 << endl;
				}
			}
		}
	}
	vsigOut.close();*/
#endif

	
	as.calculateVolume(4);
#if 0
	std::string path1 = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\data_base.txt";
	std::string path2 = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\data_volume.binary";
	ifstream volume_in(path1);
	ifstream data_in(path2, ios::binary);
	float ori[3];
	float length[3];
	int size[3];
	string sss;
	getline(volume_in, sss);
	volume_in >> ori[0] >> ori[1] >> ori[2];
	getline(volume_in, sss);
	getline(volume_in, sss);
	volume_in >> length[0] >> length[1] >> length[2];
	getline(volume_in, sss);
	getline(volume_in, sss);
	volume_in >> size[0] >> size[1] >> size[2];
	data_in.read((char*)as.volume_, 512 * 512 * 512 * sizeof(float));
	as.setVolumeBox(AlgebraicSurface::point3(ori[0], ori[1], ori[2]), length[0], length[1], length[2]);
#endif
	//test get out volume value
	/*std::string path1 = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\data_base.txt";
	std::string path2 = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\data_volume.binary";
	ofstream vout(path1);
	ofstream vout_bi(path2, ios::binary);
	vout << "orig: " << endl;
	vout << as.originalCorner.px << " " << as.originalCorner.py << " " << as.originalCorner.pz << endl;
	vout << "length: " << endl;
	vout << as.length[0] << " " << as.length[1] << " " << as.length[2] << endl;
	vout << "size: " << endl;
	vout << as.size[0] << " " << as.size[1] << " " << as.size[2] << endl << endl;
	float va[512];
	for (int z = 0; z < as.size[2]; z++)
	{
	for (int y = 0; y < as.size[1]; y++)
	{
	for (int x = 0; x < as.size[0]; x++)
	{
	va[x] = *(as.volume(x, y, z));
	}
	vout_bi.write((char*)va, sizeof(va));
	}
	}
	vout.close();*/
	/*ofstream volumeOut("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\volume.obj");
	AlgebraicSurface::point3 pt;
	for (int x = 0; x < size_ddd[0]; x++)
	{
		cout <<"x: "<< x << endl;
		for (int y = 0; y < size_ddd[1]; y++)
		{
			for (int z = 0; z < size_ddd[2]; z++)
			{
				
				if (*as.volume(x, y, z) > 0 && *as.volume(x, y, z) < 1)
				{
					pt = as.voluemCoor(x, y, z);
					volumeOut << "v " << pt.px << " " << pt.py << " " << pt.pz << " " << 255 << " " << 0 << " " << 0 << endl;
				}
				else if (*as.volume(x, y, z) < 0 && *as.volume(x, y, z) > -1)
				{
					pt = as.voluemCoor(x, y, z);
					volumeOut << "v " << pt.px << " " << pt.py << " " << pt.pz << " " << 0 << " " << 255 << " " << 0 << endl;
				}
			}
		}
	}
	volumeOut.close();*/
	//cout << *(as.volume(127, 391, 279));


	as.marchingCube();//358839
	std::string path = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\t1_4n_kd_512_newPatch_sphere_2.obj";
	as.writeOBJ(path);
	ccLog::Print("marching cube done.");
}

void DealDepthDlg::getMultiView(){
	RGBDImage* rgbdi;
	if (m_obj->isA(CC_TYPES::RGBD_IMAGE)){
		rgbdi = ccHObjectCaster::ToRGBDImage(m_obj);
	}
	else{
		return;
	}


	ccHObject* t = new ccHObject;
	t->setName("VIDEO");

	double width_pixel = m_cam.width;
	double width_notpixel = 2.0 * m_cam.focalLength * tan(m_cam.angleOfView / 2 * M_PI / 180.0);
	double lengthPerPixel = width_notpixel / width_pixel;

	//use depth to simplify the volume calculation
	int dilateVoxelNum = 4;
#ifdef USE_APSS
	doMarchingCube(rgbdi, lengthPerPixel, m_cam, dilateVoxelNum);
#endif
	for (int i = 0; i < viewNum; i++){
		ccImage *result = new ccImage;
		double bd = bdstart + i * (bdend - bdstart) / (viewNum -1);
		//bd 0.32503142759203918
		//f 6.5006285518407827
		double f = 6.5;
		setCamPara(m_cam, bd, f);

		//get the result
		getNewView(*result);
		result->setName(QString::number(i));
		t->addChild(result);
		std::cout << "now to frame: " << i << std::endl;
		
		//save
		result->data().save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\video_out_scene3\\"+ QString::number(i) + ".png");

	}
	rgbdi->addChild(t);
}

void DealDepthDlg::getNewView(ccImage &result){
	if (m_obj->isA(CC_TYPES::RGBD_IMAGE)){
		RGBDImage* rgbdi = ccHObjectCaster::ToRGBDImage(m_obj);
		if (!rgbdi->isDepthGot())
		{	
			ccLog::Warning(QString("no depth image"));
			return;
		}

		QImage depthImg = ccHObjectCaster::ToImage(rgbdi->getChild(0))->data();
		QImage srcImg = rgbdi->data();

		/*cameraPara cam;
		m_cam = cam;*/

		QImage objImg(m_cam.width, m_cam.height, QImage::Format_RGB32);
		QImage objMask(m_cam.width, m_cam.height, QImage::Format_RGB32);
		QImage objDepth(m_cam.width, m_cam.height, QImage::Format_RGB32);
		objImg.fill(qRgb(0, 0, 0));
		objMask.fill(qRgb(255, 255, 255));
		objDepth.fill(qRgb(255, 255, 255));

		//try to get 3D point from depth map
		//storeOBJ("out_color.obj", depthImg, srcImg);

		//changeCamPara(srcImg, depthImg, m_cam);

		//disparity forward mapping
#if 0
		forwardMappingWeightOverlaping(srcImg, depthImg, objImg, objDepth, objMask, m_cam);
#else
		forwardMappingDepthImageBase(srcImg, depthImg, objImg, objDepth, objMask, m_cam);
#endif

#if 0
		//test
		QImage outtest = objImg;
		QRgb comp = qRgb(255, 255, 255);
		QRgb newc = qRgb(255, 0, 0);
		
		for (int h = 0; h < objMask.height(); h++)
		{
			for (int w = 0; w < objMask.width(); w++)
			{
				QRgb rgb = objMask.pixel(w, h);
				if (comp == rgb)
				{
					outtest.setPixel(w, h, qRgb(255, 0, 0));//screen has color aberration
				}
			}
		}
		outtest.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\warp_img.png",0,100);
		objMask.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\warp_img_mask.png");
		//read back to see if color is right
		/*QImage inputtest;
		inputtest.load("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\warp_img.png");
		for (int h = 0; h < inputtest.height(); h++)
		{
			for (int w = 0; w < inputtest.width(); w++)
			{
				QRgb rgb = objMask.pixel(w, h);
				if (comp == rgb)
				{
					QRgb re = inputtest.pixel(w, h);
					int r = qRed(re);
					int g = qGreen(re);
					int b = qBlue(re);
					if (r!= 255 || g != 0 || b!=0)
					{
						std::cout << w << " " << h << " " << r << " " << g << " " << b << " " << std::endl;
					}
				}
			}
		}*/
#endif


		ccImage* tempdi = new ccImage(objImg);
		tempdi->setName("newViewImg");
		ccImage* tempdd = new ccImage(objDepth);
		tempdd->setName("depth");
		tempdi->addChild(tempdd);
		ccImage* tempdm = new ccImage(objMask);
		tempdm->setName("mask");
		tempdi->addChild(tempdm);
		
		rgbdi->addChild(tempdi);
		ccLog::Print("get new view done.");

		double width_pixel = m_cam.width;
		double width_notpixel = 2.0 * m_cam.focalLength * tan(m_cam.angleOfView / 2 * M_PI / 180.0);
		double lengthPerPixel = width_notpixel / width_pixel;
		ccLog::Print("start deal holes.");
		//dealHoles(nview, lengthPerPixel);
		std::cout << objImg.save("img.png") << endl;
		dealHoles(rgbdi, objImg, objDepth,objMask, lengthPerPixel, result);
	}
}

float getDepth(vector<float> &depth_data, int x, int y, int width, int height){
	//the x y is in image frame
	return depth_data[(height - 1 - y)*width + x];

}

void getAroundFour(int x, int y, int width, int height, vector<QPoint> &result){
	if (x-1 >= 0)
	{
		result.push_back(QPoint(x - 1, y));
	}
	if (x+1 < width)
	{
		result.push_back(QPoint(x + 1, y));
	}
	if (y-1 >= 0)
	{
		result.push_back(QPoint(x, y - 1));
	}
	if (y+1 < height)
	{
		result.push_back(QPoint(x, y + 1));
	}
}

void getAroundEight(int x, int y, int width, int height, vector<QPoint> &result){
	int v = 0;
	int x_, y_;
	while (v < 9)
	{
		if (v == 4)
		{
			v++;
			continue;
		}
		x_ = x - 1 + v % 3;
		y_ = y - 1 + v / 3;
		if (x_ >= 0 && x_ < width && y_ >= 0 & y_ < height)
		{
			result.push_back(QPoint(x_, y_));
		}
		v++;
	}
}

bool isMargin(QImage &mask, vector<float> &depth, int x, int y){
	QRgb mrgb = mask.pixel(x, y);
	int w = mask.width();
	int h = mask.height();
	if (mrgb == qRgb(255, 255, 255))
	{
		//is a hole itself
		return false;
	}
	vector<QPoint> around;
	getAroundEight(x, y, w, h, around);
	float d;
	for (int i = 0; i < around.size(); i++)
	{
		//around has one hole and not the infinite far
		QRgb lrgb = mask.pixel(around[i]);
		d = getDepth(depth, around[i].x(), around[i].y(), w, h);
		if (lrgb == qRgb(255, 255, 255) && d < 0.999999)
			return true;
	}
	//no holes around
	return false;
	
}

float openglReadDataToDepth(float data, cameraPara &cam){
	double width_pixel = cam.width;
	double width_notpixel = 2.0 * cam.focalLength * tan(cam.angleOfView / 2 * M_PI / 180.0);
	double lengthPerPixel = width_notpixel / width_pixel;

	double A = -1 * (cam.farPlane + cam.nearPlane) / (cam.farPlane - cam.nearPlane);
	double B = -2 * (cam.farPlane * cam.nearPlane) / (cam.farPlane - cam.nearPlane);
	double C = -1;
	
	float z_ndc = (data - (cam.depth_far + cam.depth_near) / 2) / ((cam.depth_far - cam.depth_near) / 2);
	float Ze = B / (C * z_ndc - A);//now is opengl frame(ze is negtive)
	float Z_scale = (-Ze - cam.nearPlane) / (cam.farPlane - cam.nearPlane);
	return Z_scale;
}

void QImageToData(QImage& qimg, unsigned char* output, int width, int height, int channel){
	assert(qimg.width() == width && qimg.height() == height);
	assert(channel == 1 || channel == 3);
	int mark = 0;
	for (int h = 0; h < qimg.height(); h++)
	{
		for (int w = 0; w < qimg.width(); w++)
		{
			if (channel == 3)
			{
				QRgb f = qimg.pixel(w, h);
				int r = qRed(f);
				int g = qGreen(f);
				int b = qBlue(f);
				output[mark++] = b;
				output[mark++] = g; 
				output[mark++] = r;
			}
			else if (channel == 1)
			{
				QRgb f = qimg.pixel(w, h);
				int r = qRed(f);
				output[mark++] = (unsigned char)r;
			}
		}
	}
}

void depthDataToData(vector<float>& depthData, unsigned char* depth, int width, int height, int channel){
	//depth is [0,1];
	assert(channel == 1);
	assert(depthData.size() == (width * height));

	int mark = 0;
	for (int h = 0; h < height; h++)
	{
		for (int w = 0; w < width; w++)
		{
			float d = getDepth(depthData, w, h, width, height);
			depth[mark++] = (int)(d * 255);
		}
	}
}

void depthDataToData(QImage& depthData, unsigned char* depth, int width, int height, int channel){
	//depth is [0,1];
	assert(channel == 1);
	assert(depthData.width() * depthData.height() == (width * height));

	int mark = 0;
	for (int h = 0; h < height; h++)
	{
		for (int w = 0; w < width; w++)
		{
			float d;
			RGBtoDepth(depthData.pixel(w, h), &d);
			depth[mark++] = (int)(d * 255);
		}
	}
}

void DataToQImage(QImage& out, unsigned char* input, int width, int height, int channel){
	assert(out.height() == height && out.width() == width);
	assert(channel == 3);
	
	int mark = 0;
	for (int h = 0; h < out.height(); h++)
	{
		for (int w = 0; w < out.width(); w++)
		{
			int b = input[mark++];
			int g = input[mark++];
			int r = input[mark++];
			if (mark < 10)
				std::cout << b << " " << g << " " << r << std::endl;
			out.setPixel(w, h, qRgb(r, g, b));
		}
	}
}

void fillDepth(QImage& depth){
	//qrgb(255,255,255) for empty
	for (int y = 0; y < depth.height(); y++)
	{
		for (int x = 0; x < depth.width(); x++)
		{
			QRgb temp = depth.pixel(x, y);
			if (temp == qRgb(255,255,255))
			{
				//need to fill
				int left = x, right = x;
				while (left>=0)
				{
					if (depth.pixel(left, y) != qRgb(255, 255, 255))
					{
						break;
					}
					left--;
				}
				while (right < depth.width())
				{
					if (depth.pixel(right, y) != qRgb(255, 255, 255))
					{
						break;
					}
					right++;
				}
				//judge if both side is valid
				if (left >=0 &&right < depth.width())
				{
					float aall = right - left;
					float a1 = x - left;
					float a2 = right - x;
					float d1;
					RGBtoDepth(depth.pixel(left, y), &d1);
					float d2;
					RGBtoDepth(depth.pixel(right, y), &d2);
					float resultd = (d1 * a2 + d2 * a1) / aall;
#if 1
					resultd = d1 > d2 ? d1 : d2;
#endif
					depth.setPixel(x, y, DepthtoRGB(resultd));
				}
				else
				{
					if (left >= 0)
					{
						depth.setPixel(x, y, depth.pixel(left, y));
					}else if (right < depth.width())
					{
						depth.setPixel(x, y, depth.pixel(right, y));
					}
					else
					{
						//still remaining holes in depth
					}
				}
			}
		}
	}

}

void fillHoles(QImage &newImg, QImage &depth, QImage &mask, cameraPara &cam, vector<AlgebraicSurface::point3> &vertex){
	float depth_threshold = 0.05;
	int w = mask.width();//here mask white for inpainting
	int h = mask.height();
	unsigned char *depthPtr;
	depthPtr = new unsigned char[w*h];
#ifdef USE_APSS
	//do not effect the main window
	vector<float> depth_data;
	QOpenGLContext *m_pCtx = new QOpenGLContext;
	QSurfaceFormat surfaceFmt;
	if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL)
	{
		surfaceFmt.setRenderableType(QSurfaceFormat::OpenGL);
	}
	else
	{
		surfaceFmt.setRenderableType(QSurfaceFormat::OpenGLES);
	}
	m_pCtx->setFormat(surfaceFmt);
	bool b = m_pCtx->create();
	QOffscreenSurface *pSurface = new QOffscreenSurface;
	pSurface->setFormat(surfaceFmt);
	pSurface->create();
	m_pCtx->makeCurrent(pSurface);
	renderMesh(vertex, cam, depth_data);
	delete[] pSurface;
	delete[] m_pCtx;
	
	depthDataToData(depth_data, depthPtr, w, h, 1);
#else
	QString localPath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\";
	//just fill depth
	QImage outdepth(depth.size(), depth.format());
	outdepth.fill(qRgb(255, 0, 0));
	for (int i = 0; i < depth.height(); i++)
	{
		for (int j = 0; j < depth.width(); j++)
		{
			QRgb temp = depth.pixel(j, i);
			if (temp == qRgb(255, 255, 255))
				continue;
			float re;
			RGBtoDepth(temp, &re);
			int inputre = 255 * re;
			outdepth.setPixel(j, i, qRgb(inputre, inputre, inputre));
		}
	}
	outdepth.save(localPath + "outdepth1.png");

	fillDepth(depth);

	outdepth.fill(qRgb(255, 0, 0));
	for (int i = 0; i < depth.height(); i++)
	{
		for (int j = 0; j < depth.width(); j++)
		{
			QRgb temp = depth.pixel(j, i);
			if (temp == qRgb(255, 255, 255))
				continue;
			float re;
			RGBtoDepth(temp, &re);
			int inputre = 255 * re;
			outdepth.setPixel(j, i, qRgb(inputre, inputre, inputre));
		}
	}
	outdepth.save(localPath + "outdepth2.png");

	depthDataToData(depth, depthPtr, w, h, 1);
#endif
#if 1
	//doing inpainting
	
	//QImage to data
	//colorMat(h, w, CV_8UC3); maskMat(h, w, CV_8UC1); depthMat(h, w, CV_8UC1)//depth 0 for near, not sure
	unsigned char *colorPtr, *maskPtr, *outPtr;
	colorPtr = new unsigned char[w*h * 3];
	maskPtr = new unsigned char[w*h];
	outPtr = new unsigned char[w*h * 3];
	QImageToData(newImg, colorPtr, w, h, 3);
	QImageToData(mask, maskPtr, w, h, 1);
	mask.save(localPath + "maskout.png");
	
	//deal mask
	for (int i = 0; i < w*h; i++)
	{
		maskPtr[i] = maskPtr[i] == 255 ? 0 : 255;
	}
	std::cout << newImg.save("img.png") << endl;
	PatchInpaint pi;
	pi.mainLoop(colorPtr, maskPtr, depthPtr, outPtr, w, h);//mask black for inpainting

	std::cout << (int)outPtr[0] << " " << (int)outPtr[1] << std::endl;
	int m = outPtr[0];
	std::cout << m << std::endl;
	DataToQImage(newImg, outPtr, w, h, 3);
	delete[] colorPtr;
	delete[] maskPtr;
	delete[] depthPtr;
	delete[] outPtr;
#endif

#if 0 
	//simple pixel copy

	//y axis in depth_data is different from that in the image
	int w = mask.width();
	int h = mask.height();

	vector<QPoint> pos;
	int count = 0;
	do
	{
		cout << "fill round: " << count++ << endl;

		//test
		//newImg.save("img.png");
		//depth.save("depth.png");
		//mask.save("mask.png");


		pos.clear();
		for (int i = 0; i < h; i++)
		{
			for (int j = 0; j < w; j++)
			{
				if (isMargin(mask, depth_data, j, i))
				{
					pos.push_back(QPoint(j, i));
				}
			}
		}
		
		float maxd = -1, mind = 100;
		for (int i = 0; i < pos.size(); i++)
		{
			float self_depth = getDepth(depth_data, pos[i].x(), pos[i].y(), w, h);
			vector<QPoint> around;
			getAroundEight(pos[i].x(), pos[i].y(), w, h, around);
			for (int j = 0; j < around.size(); j++)
			{
				QRgb mrgb = mask.pixel(around[j]);
				if (mrgb != qRgb(255, 255, 255))
				{
					//is not a hole
					continue;
				}
				QPoint obj = around[j];
				float point_depth = getDepth(depth_data, obj.x(), obj.y(), mask.width(), mask.height());

				maxd = maxd > abs(point_depth - self_depth) ? maxd : abs(point_depth - self_depth);
				mind = mind < abs(point_depth - self_depth) ? mind : abs(point_depth - self_depth);


				if (abs(point_depth - self_depth) < depth_threshold)
				{
					newImg.setPixel(obj, newImg.pixel(pos[i]));
					int r, g, b;
					depth.setPixel(obj, DepthtoRGB(openglReadDataToDepth(point_depth, cam), &r, &g, &b));
					mask.setPixel(obj, qRgb(0, 0, 0));
				}
			}
		}
		cout << "max depth: " << maxd << endl;
		cout << "min depth: " << mind << endl;
	} while (!pos.empty());
	////test out put depth
	//QImage tempd(depth.width(),depth.height(),QImage::Format_RGB32);
	////cout << tempd.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\tempdepth1.png") << endl;

	//tempd.fill(QColor(0,0,0));//maybe wrong
	//for (int i = 0; i < depth.width(); i++)
	//{
	//	for (int j = 0; j < depth.height(); j++)
	//	{
	//		QRgb t = depth.pixel(i, j);
	//		float dt;
	//		RGBtoDepth(t, &dt);
	//		tempd.setPixel(i, j, qRgb(int(255 * dt), int(255 * dt), int(255 * dt)));
	//	}
	//}
	//cout << tempd.save("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\tempdepth.png") << endl;
#endif
	//test
	std::cout << newImg.save("img.png") << endl;//not right
	//depth.save("depth.png");
	//mask.save("mask.png");
}

void testReadin(string path, vector<AlgebraicSurface::point3> &vertex){
	string filepath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\mesh_input_2.obj";
	//string filepath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\test.obj";
	ifstream input(filepath);
	if (!input.is_open())
	{
		std::cout << "cannot open obj file!" << endl;
	}
	char c;
	float x, y, z;
	int id1, id2, id3;

	//vector<point3> vertex;
	vector<int> face;
	std::cout << "start read in..." << std::endl;
	char bb[100];
	while (!input.eof())
	{
		c = 'n';
		input >> c;
		if (c == '#')
		{
			input.getline(bb, 100);
			continue;
		}

		if (c == 'v')
		{
			input >> x >> y >> z;
			vertex.push_back(AlgebraicSurface::point3(x, y, z));
		}
		else if (c == 'f')
		{
			input >> id1 >> id2 >> id3;
			face.push_back(id1 - 1);
			face.push_back(id2 - 1);
			face.push_back(id3 - 1);
		}
	}
	input.close();
	vector<AlgebraicSurface::point3> temp = vertex;
	vertex.clear();
	for (int i = 0; i < face.size(); i++)
	{
		vertex.push_back(temp[face[i]]);
	}

}

void DealDepthDlg::dealHoles(ccImage* nview, QImage &newImage, QImage &depthImg, QImage &maskImg, double lengthPerPixel, ccImage &result){
	

	ccLog::Print("start filling the holes.");
	if (nview->getChild(1) == NULL)
	{
		ccLog::Error("[new view] didn't find new view image! ");
		return;
	}
	//QImage newImage = ccHObjectCaster::ToImage(nview->getChild(1))->data();
	//QImage depthImg = ccHObjectCaster::ToImage(nview->getChild(1)->getChild(0))->data();
	//QImage maskImg = ccHObjectCaster::ToImage(nview->getChild(1)->getChild(1))->data();

	//test
	//string filepath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\mesh_input_2.obj";
	//testReadin(filepath, as.allVertex);

	fillHoles(newImage, depthImg, maskImg, m_cam, as.allVertex);//mask white for empty needed to inpainting

	//add to root
	ccImage* tempdi = new ccImage(newImage);
	result = *tempdi;
	tempdi->setName("FinalImg");

	nview->addChild(tempdi);
	ccLog::Print("finish filling the holes.");
}