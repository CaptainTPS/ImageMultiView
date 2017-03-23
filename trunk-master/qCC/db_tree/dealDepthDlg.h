#ifndef DEALDEPTH_DIALOG_HEADER
#define DEALDEPTH_DIALOG_HEADER


//Qt
#include <QWidget>

//cc
#include "ccHObject.h"
#include "RGBDImage.h"

#include <ui_dealDepthDlg.h>

#include "..\..\openglGetDepth\mydata.h"

#include "videoThread.h"



class DealDepthDlg :public QWidget, public Ui::dealDepthDlg
{
	Q_OBJECT

public:
	explicit DealDepthDlg(QWidget* parent = 0);
	
	public slots:

	void addDepthImage();

	void Buttonplay();

	void playVideo();

	void getMultiView();

	void doMarchingCube(ccImage* nview, double lengthPerPixel, cameraPara &m_cam, int dilateVoxelNum);

	void getNewView(ccImage &result);

	void dealHoles(ccImage* newView, QImage &newImage, QImage &depthImg, QImage &maskImg, double lengthPerPixel, ccImage &result);

	

	void connectOBJ(ccHObject* obj);

protected:
	ccHObject* m_obj;
	cameraPara m_cam;
	

	double bdstart;
	double bdend;
	int viewNum;

	ccHObject* video = NULL;
	bool pause = true;
	videoThread vthread;
	int frame;
	int frameflag;

	

};
#endif