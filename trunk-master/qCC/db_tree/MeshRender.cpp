#include <cstdlib>
#include "GL\glew.h"
#include <GLUT\glut.h>
#include "MeshRender.h"


//void renderMesh(vector<AlgebraicSurface::point3> &vertex, cameraPara &cam, vector<float> &depth){
//	
//}
enum { Color, Depth, NumRenderbuffers };
GLuint framebuffer_ms, renderbuffer_ms[NumRenderbuffers];

void init(){
	if (glewInit() == GLEW_OK)
	{
		printf("glew ok\n\n");
	}
}

void setupRT(cameraPara &cam){
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, cam.width, cam.height);
	gluPerspective(cam.angleOfView_vertical, cam.screenRatio, cam.nearPlane, cam.farPlane);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Load identity matrix
	//rotation
	//glRotated(-1.0 * cam.rotation[0], 1.0, 0.0, 0.0);
	//glRotated(-1.0 * cam.rotation[1], 0.0, 1.0, 0.0);
	//glRotated(-1.0 * cam.rotation[2], 0.0, 0.0, 1.0);
	//translate
	glTranslated(-cam.baseDistance, 0, 0);
}

void draw(vector<AlgebraicSurface::point3> &vertex){
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
			glVertex3f(vertex[i+1].px, vertex[i+1].py, vertex[i+1].pz);
			glVertex3f(vertex[i+2].px, vertex[i+2].py, vertex[i+2].pz);
		glEnd();
	}
	glFlush();
}

#ifdef TEST
#define TEST
#endif
#ifdef TEST

void TESTdraw(){
	glBegin(GL_TRIANGLES);
	glVertex3f(4.0, -4.0, 0.0);
	glVertex3f(-4.0, -4.0, 0.0);
	glVertex3f(0, 4.0, 0.0);
	glEnd();
	glFlush();
}

void TESTScene(cameraPara &cam){
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_ms);

	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear buffers  // 清除屏幕及深度缓存
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, cam.width, cam.height);
	gluPerspective(45, 1, 0.1, 10);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Load identity matrix
	glTranslated(0, 0, -5);

	TESTdraw();
}

#endif

void fetchDepth(cameraPara &cam, vector<float> &depth){
	GLfloat* depth_data = new GLfloat[cam.width * cam.height];
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_ms);
	glReadPixels(0, 0, cam.width, cam.height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_data);
	for (int i = 0; i < cam.width * cam.height; i++)
	{
		depth.push_back(depth_data[i]);
	}
	delete[] depth_data;
}

void prepareRender(int window_width_, int window_height_) {
	int render_width = window_width_, render_height = window_height_;

	// generate color and depth render buffers and allocate storage for the multi-sampled FBO
	glGenRenderbuffersEXT(NumRenderbuffers, renderbuffer_ms);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_ms[Color]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_RGBA8, render_width, render_height);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, renderbuffer_ms[Depth]);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, render_width, render_height);

	// generate frame buffer object for the multi-sampled FBO
	glGenFramebuffersEXT(1, &framebuffer_ms);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer_ms);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_EXT, renderbuffer_ms[Color]);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, renderbuffer_ms[Depth]);
	
	if (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT){
		printf("FBO is wrong");
	}

	//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void afterRender(){
	// delete the created frame buffer objects and render buffer objects
	glDeleteRenderbuffers(NumRenderbuffers, renderbuffer_ms);
	glDeleteFramebuffers(1, &framebuffer_ms);
	// bind the read and draw frame buffer to the default (window)
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void renderMesh(vector<AlgebraicSurface::point3> &vertex, cameraPara &cam, vector<float> &depth){

	init();
	prepareRender(cam.width, cam.height);
	

#ifdef TEST
	TESTScene(cam);
#else
	setupRT(cam);
	draw(vertex);
#endif

	fetchDepth(cam, depth);
	afterRender();
}