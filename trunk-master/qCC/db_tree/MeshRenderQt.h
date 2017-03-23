#ifndef MESH_RENDER_H_
#define MESH_RENDER_H
#include <cstdlib>

#include <qoffscreensurface.h>
#include <qopenglframebufferobject.h>
#include <qopenglpaintdevice.h>
#include <qpainter.h>

#include "D:\captainT\project_13\ImageMultiView\openglGetDepth\mydata.h"
#include "apss.h"

void makeFrustum(double fovY, double aspectRatio, double front, double back)
{
	const double DEG2RAD = 3.14159265 / 180;

	double tangent = tan(fovY / 2 * DEG2RAD);   // tangent of half fovY
	double height = front * tangent;          // half height of near plane
	double width = height * aspectRatio;      // half width of near plane

	// params: left, right, bottom, top, near, far
	glFrustum(-width, width, -height, height, front, back);
}


void setupRTQt(cameraPara &cam){

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, cam.width, cam.height);
	//gluPerspective(cam.angleOfView_vertical, cam.screenRatio, cam.nearPlane, cam.farPlane);
	makeFrustum(cam.angleOfView_vertical, cam.screenRatio, cam.nearPlane, cam.farPlane);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Load identity matrix
	//rotation
	//glRotated(-1.0 * cam.rotation[0], 1.0, 0.0, 0.0);
	//glRotated(-1.0 * cam.rotation[1], 0.0, 1.0, 0.0);
	//glRotated(-1.0 * cam.rotation[2], 0.0, 0.0, 1.0);
	//translate
	//glTranslated(-cam.translate[0], -cam.translate[1], -cam.translate[2]);
}

void drawQt(vector<AlgebraicSurface::point3> &vertex){
	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 0.0);

	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);


	glColor3f(1, 1, 1);
	for (int i = 0; i < vertex.size(); i += 3)
	{
		glBegin(GL_TRIANGLES);
		glVertex3f(vertex[i].px, vertex[i].py, vertex[i].pz);
		glVertex3f(vertex[i + 1].px, vertex[i + 1].py, vertex[i + 1].pz);
		glVertex3f(vertex[i + 2].px, vertex[i + 2].py, vertex[i + 2].pz);
		glEnd();
	}
	glFlush();
}

#ifndef TEST
#define TEST
#endif
#ifdef TEST

void TESTdrawQt(){
	

	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(4.0, -4.0, -1.0);
	glVertex3f(-4.0, -4.0, -1.0);
	glVertex3f(0, 4.0, -1.0);
	glEnd();

	glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_TRIANGLES);
	glVertex3f(2.0, -2.0, 0.0);
	glVertex3f(-2.0, -2.0, 0.0);
	glVertex3f(0, 2.0, 0.0);
	glEnd();

	glFlush();
}

void TESTSceneQt(cameraPara &cam){

	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear buffers  // 清除屏幕及深度缓存
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, cam.width, cam.height);
	makeFrustum(45, 1, 0.1, 10);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Load identity matrix
	glTranslated(0, 0, -5);

	TESTdrawQt();
}


void nativePainting(){
	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glBegin(GL_TRIANGLES);
	glColor3f(1.0, 0.0, 0.0);
	glVertex2f(1.0, 1.0);
	glColor3f(0.0, 1.0, 0.0);
	glVertex2f(0.0, 1.0);
	glColor3f(0.0, 0.0, 1.0);
	glVertex2f(0.0, 0.0);

	glEnd();
}

#endif

void fetchDepthQt(cameraPara &cam, vector<float> &depth){
	GLfloat* depth_data = new GLfloat[cam.width * cam.height];
	glReadPixels(0, 0, cam.width, cam.height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_data);
	/*for (int i = 0; i < cam.width * cam.height; i++)
	{
		depth.push_back(depth_data[i]);
	}*/
	delete[] depth_data;
}

void renderMeshQt(vector<AlgebraicSurface::point3> &vertex, cameraPara &cam, vector<float> &depth){
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

	vector<float> depth_data;

	const QRect drawRect(0, 0, cam.width, cam.height);
	const QSize drawRectSize = drawRect.size();

	/*QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setSamples(1);
	fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);

	QOpenGLFramebufferObject fbo(drawRectSize, fboFormat);
	fbo.bind();*/

	QOpenGLPaintDevice device(cam.width, cam.height);
	QPainter painter;
	painter.begin(&device);

	/*painter.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing);
	painter.fillRect(drawRect, Qt::blue);
	painter.setPen(QPen(Qt::green, 5));
	painter.setBrush(Qt::red);
	painter.drawEllipse(0, 100, 400, 200);
	painter.drawEllipse(100, 0, 200, 400);
	painter.setPen(QPen(Qt::white, 0));
	QFont font;
	font.setPointSize(24);
	painter.setFont(font);
	painter.drawText(drawRect, "Hello FBO", QTextOption(Qt::AlignCenter));*/

	painter.beginNativePainting();
#ifdef TEST
	//nativePainting();
	TESTSceneQt(cam);
#else
	setupRTQt(cam);
	drawQt(vertex);
#endif
	painter.endNativePainting();
	fetchDepthQt(cam, depth);
	painter.end();
	fetchDepthQt(cam, depth);
	//fbo.release();

	//QImage test = fbo.toImage();
	//test.save("testfbo.png");
	delete[] pSurface;
	delete[] m_pCtx;
}

#endif