#include <cstdlib>
#include <iostream>
#include <GL/glut.h> 
#include <vector>
#include <fstream>
#include <sstream>
#include <opencv2\opencv.hpp>

#define _USE_MATH_DEFINES
#include "math.h"

#include "mydata.h"

#define DEG2RAD  (3.1415926/180.0)

using namespace std;

PIC m_pic;

cameraPara cam;

void ReadPIC(string name)
{
	ifstream ifs(name);//cube bunny Eight
	string s;
	Mian *f;
	POINT3 *v;
	FaXiangLiang *vn;
	WenLi    *vt;

	int ccc = 0;
	while (getline(ifs, s))
	{
#if 1
		ccc++;
		if (ccc % 1000 == 0)
		{
			cout << "go to line: " << ccc << endl;
		}
#endif
		if (s.length()<2)continue;
		if (s[0] == 'v'){
			if (s[1] == 't'){//vt 0.581151 0.979929 纹理
				istringstream in(s);
				vt = new WenLi();
				string head;
				in >> head >> vt->TU >> vt->TV;
				m_pic.VT.push_back(*vt);
			}
			else if (s[1] == 'n'){//vn 0.637005 -0.0421857 0.769705 法向量
				istringstream in(s);
				vn = new FaXiangLiang();
				string head;
				in >> head >> vn->NX >> vn->NY >> vn->NZ;
				m_pic.VN.push_back(*vn);
			}
			else{//v -53.0413 158.84 -135.806 点
				istringstream in(s);
				v = new POINT3();
				string head;
				in >> head >> v->X >> v->Y >> v->Z;
				m_pic.V.push_back(*v);
			}
		}
		else if (s[0] == 'f'){//f 2443//2656 2442//2656 2444//2656 面 f Vertex1/Texture1/Normal1
			for (int k = s.size() - 1; k >= 0; k--){
				if (s[k] == '/')s[k] = ' ';
			}
			istringstream in(s);
			f = new Mian();
			string head;
			in >> head;
			int i = 0;
			while (i<3)
			{
				if (m_pic.V.size() != 0)
				{
					in >> f->V[i];
					f->V[i] -= 1;
				}
				if (m_pic.VT.size() != 0)
				{
					in >> f->T[i];
					f->T[i] -= 1;
				}
				if (m_pic.VN.size() != 0)
				{
					in >> f->N[i];
					f->N[i] -= 1;
				}
				i++;
			}
			m_pic.F.push_back(*f);
		}
	}
}

#define YU 1//800//2

float position_x = 0;
float position_y = 0;
float position_z = 0;

void getDepth(int button, int state, int x, int y);

void GlAxis(){

	//x
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex3f(position_x + 0.0f, position_y + 0.0f, position_z + 0.0f);
	glVertex3f(position_x + 20.0, position_y + 0.0f, position_z + 0.0f);
	glEnd();
	glPushMatrix();
	glTranslatef(position_x + 20.0, position_y + 0.0f, position_z + 0.0f);
	glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
	glutWireCone(1.0, 10.0, 10, 10);
	glPopMatrix();

	//y
	glBegin(GL_LINES);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex3f(position_x + 0.0f, position_y + 0.0f, position_z + 0.0f);
	glVertex3f(position_x + 0.0, position_y + 20.0f, position_z + 0.0f);
	glEnd();
	glPushMatrix();
	glTranslatef(position_x + 0.0, position_y + 20.0f, position_z + 0.0f);
	glRotatef(270.0f, 1.0f, 0.0f, 0.0f);
	glutWireCone(1.0, 10.0, 10, 10);
	glPopMatrix();

	//z
	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex3f(position_x + 0.0f, position_y + 0.0f, position_z + 0.0f);
	glVertex3f(position_x + 0.0, position_y + 0.0f, position_z + 20.0f);
	glEnd();
	glPushMatrix();
	glTranslatef(position_x + 0.0, position_y + 0.0f, position_z + 20.0f);
	glutWireCone(1.0, 10.0, 10, 10);
	glPopMatrix();
}

void GLCube()
{
	GLboolean tt;
	glGetBooleanv(GL_DEPTH_TEST, &tt);
	for (int i = 0; i<m_pic.F.size(); i++)
	{
		//if (m_pic.F[i].V[0] == 0)
		//	glColor3f(1, 0, 0);
		//	//continue;
		//if (m_pic.F[i].V[0] == 3)
		//	glColor3f(0, 1, 0);
		glBegin(GL_TRIANGLES);                            // 绘制三角形GL_TRIANGLES;GL_LINE_LOOP;GL_LINES;GL_POINTS
		if (m_pic.VT.size() != 0)glTexCoord2f(m_pic.VT[m_pic.F[i].T[0]].TU, m_pic.VT[m_pic.F[i].T[0]].TV);  //纹理    
		if (m_pic.VN.size() != 0)glNormal3f(m_pic.VN[m_pic.F[i].N[0]].NX, m_pic.VN[m_pic.F[i].N[0]].NY, m_pic.VN[m_pic.F[i].N[0]].NZ);//法向量
		glVertex3d(m_pic.V[m_pic.F[i].V[0]].X / YU, m_pic.V[m_pic.F[i].V[0]].Y / YU, m_pic.V[m_pic.F[i].V[0]].Z / YU);        // 上顶点

		if (m_pic.VT.size() != 0)glTexCoord2f(m_pic.VT[m_pic.F[i].T[1]].TU, m_pic.VT[m_pic.F[i].T[1]].TV);  //纹理
		if (m_pic.VN.size() != 0)glNormal3f(m_pic.VN[m_pic.F[i].N[1]].NX, m_pic.VN[m_pic.F[i].N[1]].NY, m_pic.VN[m_pic.F[i].N[1]].NZ);//法向量
		glVertex3d(m_pic.V[m_pic.F[i].V[1]].X / YU, m_pic.V[m_pic.F[i].V[1]].Y / YU, m_pic.V[m_pic.F[i].V[1]].Z / YU);        // 左下

		if (m_pic.VT.size() != 0)glTexCoord2f(m_pic.VT[m_pic.F[i].T[2]].TU, m_pic.VT[m_pic.F[i].T[2]].TV);  //纹理
		if (m_pic.VN.size() != 0)glNormal3f(m_pic.VN[m_pic.F[i].N[2]].NX, m_pic.VN[m_pic.F[i].N[2]].NY, m_pic.VN[m_pic.F[i].N[2]].NZ);//法向量
		glVertex3d(m_pic.V[m_pic.F[i].V[2]].X / YU, m_pic.V[m_pic.F[i].V[2]].Y / YU, m_pic.V[m_pic.F[i].V[2]].Z / YU);        // 右下
		glEnd();// 三角形绘制结束    


		/*if(m_pic.VN.size()!=0){
		glBegin(GL_LINES);                            // 绘制三角形
		glVertex3f(m_pic.V[m_pic.F[i].V[0]].X/YU,m_pic.V[m_pic.F[i].V[0]].Y/YU, m_pic.V[m_pic.F[i].V[0]].Z/YU);        // 上顶点
		glVertex3f(m_pic.V[m_pic.F[i].V[0]].X/YU+m_pic.VN[m_pic.F[i].N[0]].NX
		,m_pic.V[m_pic.F[i].V[0]].Y/YU+m_pic.VN[m_pic.F[i].N[0]].NY
		, m_pic.V[m_pic.F[i].V[0]].Z/YU+m_pic.VN[m_pic.F[i].N[0]].NZ);                    // 左下
		glEnd();                                // 三角形绘制结束
		}*/
	}
}

void myDisplay(void)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glRectf(-0.5f, -0.5f, 0.5f, 0.5f);
	glFlush();
}

void DrawScene()
{
	// TODO: Replace the following sample code with your code to draw the scene.
	glClearDepth(1.0);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glDepthFunc(GL_LESS);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear buffers  // 清除屏幕及深度缓存
	glEnable(GL_DEPTH_TEST);
	GLboolean tt;
	glGetBooleanv(GL_DEPTH_TEST, &tt);

	GLdouble* proMatrix = new GLdouble[16];
	glGetDoublev(GL_PROJECTION_MATRIX, proMatrix);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, cam.width, cam.height);
	gluPerspective(cam.angleOfView_vertical, cam.screenRatio, cam.nearPlane, cam.farPlane);
	glGetDoublev(GL_PROJECTION_MATRIX, proMatrix);

	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity(); // Load identity matrix
	//rotation
	glRotated(-1.0 * cam.rotation[0], 1.0, 0.0, 0.0);
	glRotated(-1.0 * cam.rotation[1], 0.0, 1.0, 0.0);
	glRotated(-1.0 * cam.rotation[2], 0.0, 0.0, 1.0);
	//translate
	glTranslated(-cam.translate[0], -cam.translate[1], -cam.translate[2]);

	// Add a light source
	//GLfloat glfLight[] = { -4.0f, 4.0f, 4.0f, 0.0f };
	//glLightfv(GL_LIGHT0, GL_POSITION, glfLight);

	// Draw a cube
	GLCube();
	/*float d = -9;
	glBegin(GL_POLYGON);
	glVertex3d(50.0f, 50.0f, d);
	glVertex3d(-50.0f, 50.0f, d);
	glVertex3d(-50.0f, 0.0f, d);
	glVertex3d(50.0f, 0.0f, d);
	glEnd();*/
	//GlAxis();

	glFlush();
	/*GLfloat* depth_data = new GLfloat[cam.width * cam.height];
	glReadPixels(0, 0, cam.width, cam.height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_data);
	for (int i = 0; i < cam.width * cam.height; i++)
	{
		if (depth_data[i] <1)
		{
			cout << i << endl;
		}
	}
	delete depth_data;*/
	//getDepth(GLUT_LEFT_BUTTON, GLUT_DOWN, 0, 0);
}

void getDepth(int button, int state, int x, int y){
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		GLfloat dp_scalar;
		GLfloat dp_bias;
		glGetFloatv(GL_DEPTH_SCALE, &dp_scalar);
		glGetFloatv(GL_DEPTH_BIAS, &dp_bias);

		GLfloat* depth_data = new GLfloat[cam.width * cam.height];
		GLdouble* proMatrix = new GLdouble[16];
		glGetDoublev(GL_PROJECTION_MATRIX, proMatrix);
		GLdouble rotationMatrix[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, rotationMatrix);
		//double A = proMatrix[10];
		//double B = proMatrix[14];
		//double C = proMatrix[11];

		double A = -1*(cam.farPlane + cam.nearPlane)/(cam.farPlane - cam.nearPlane);
		double B = -2*(cam.farPlane * cam.nearPlane)/(cam.farPlane - cam.nearPlane);
		double C = -1;

		glReadPixels(0, 0, cam.width, cam.height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_data);
		cv::Mat image(cam.height, cam.width, CV_8UC3, cv::Scalar(255,255,255));

		//test
		double row_major[16] = {
			proMatrix[0], proMatrix[4], proMatrix[8], proMatrix[12],
			proMatrix[1], proMatrix[5], proMatrix[9], proMatrix[13],
			proMatrix[2], proMatrix[6], proMatrix[10], proMatrix[14],
			proMatrix[3], proMatrix[7], proMatrix[11], proMatrix[15]
		};
		cv::Mat pm(4, 4, CV_64F, &row_major);
		cv::Mat pm_v = pm.inv();

		double row_major_r[16] = {
			rotationMatrix[0], rotationMatrix[4], rotationMatrix[8], rotationMatrix[12],
			rotationMatrix[1], rotationMatrix[5], rotationMatrix[9], rotationMatrix[13],
			rotationMatrix[2], rotationMatrix[6], rotationMatrix[10], rotationMatrix[14],
			rotationMatrix[3], rotationMatrix[7], rotationMatrix[11], rotationMatrix[15]
		};
		cv::Mat rm(4, 4, CV_64F, &row_major_r);
		cv::Mat rm_v = rm.inv();

		ofstream gloutput("gl_inv_output123.obj");

		float depth_low = 20;
		float depth_high = -10;
		//test end
		for (int i = 0; i < cam.height; i++)
		{
#if 1
			if (i % 10 ==0)
			cout << "scan img row: " << i << endl;
#endif
			for (int j = 0; j < cam.width; j++)
			{
				float z_window = depth_data[i*cam.width + j];
				if (z_window > 0.99999)
				{
					continue;
				}
				depth_low = z_window < depth_low ? z_window : depth_low;
				depth_high = z_window > depth_high ? z_window : depth_high;

				float x_window = j;
				float y_window = i;
				float x_ndc = (x_window - cam.width / 2) / (cam.width / 2);
				float y_ndc = (y_window - cam.height / 2) / (cam.height / 2);
				float z_ndc = (z_window - (cam.depth_far + cam.depth_near) / 2) / ((cam.depth_far - cam.depth_near) / 2);

				float Ze = B / (C * z_ndc - A);
				float Z_scale = (-Ze - cam.nearPlane) / (cam.farPlane - cam.nearPlane);
				int depth = Z_scale * 16777216;
				int r = (depth >> 16);
				int g = (depth - (r << 16)) >> 8;
				int b = depth - (r << 16) - (g << 8);

				image.at<cv::Vec3b>(cam.height - i - 1, j) = cv::Vec3b(b, g, r);
				//test
				double x = 1.0 * (j - cam.width / 2) / (cam.width / 2);
				double y = 1.0 * (i - cam.height / 2 ) / (cam.height / 2);
				double pos[4] = { x_ndc, y_ndc, z_ndc, 1 };
				cv::Mat ndc_pos(4, 1, CV_64F, &pos);
				cv::Mat camera_pos = rm_v * pm_v * ndc_pos;
				camera_pos = camera_pos / (camera_pos.at<double>(3, 0));
				double test = camera_pos.at<double>(2, 0);
				
				gloutput << "v " << (camera_pos.at<double>(0, 0)) << " " << (camera_pos.at<double>(1, 0)) << " " << (camera_pos.at<double>(2, 0));
				gloutput << " " << 255 << " " << 0 << " " << 0 << endl;
				//test end

			}
		}
		delete depth_data;
		delete proMatrix;

		gloutput.close();
		//image.at<cv::Vec3b>(0, 0) = cv::Vec3b(255, 255, 255);
		//image.at<cv::Vec3b>(0, 1) = cv::Vec3b(255, 255, 255);
		//image.at<cv::Vec3b>(1, 0) = cv::Vec3b(255, 255, 255);
		//image.at<cv::Vec3b>(1, 1) = cv::Vec3b(255, 255, 255);
		cv::Vec3b test1 = image.at<cv::Vec3b>(2, 0);
		test1 = image.at<cv::Vec3b>(0, 2);
		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(0);
		string filePath = "F:\\CaptainT\\data\\SphereTest\\";
		string fileName = "SphereTest_depth.png";
		cv::imwrite(filePath+fileName, image, compression_params);
	}
}

int main(int argc, char *argv[])
{
	//string filePath = "D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\maya\\";
	//string fileName = "dragon_meshlab.obj";
	string filePath = "F:\\CaptainT\\data\\SphereTest\\";
	string fileName = "meshlab_out.obj";
	ReadPIC(filePath + fileName);
#if 0
	ofstream objOut("justReadAndOut.obj");
	for (int i = 0; i < m_pic.V.size(); i++)
	{
		objOut << "v " << m_pic.V[i].X << " " << m_pic.V[i].Y<< " " << m_pic.V[i].Z << endl;
	}
	for (int i = 0; i < m_pic.F.size(); i++)
	{
		objOut << "f " << m_pic.F[i].V[0] + 1 << " " << m_pic.F[i].V[1] + 1 << " " << m_pic.F[i].V[2] + 1 << endl;
	}
	objOut.close();
#endif

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE | GLUT_DEPTH);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(cam.width, cam.height);
	glutCreateWindow("depth");
	glutDisplayFunc(&DrawScene);
	glutMouseFunc(&getDepth);
	
	//const GLubyte* name = glGetString(GL_VENDOR); //返回负责当前OpenGL实现厂商的名字
	//const GLubyte* biaoshifu = glGetString(GL_RENDERER); //返回一个渲染器标识符，通常是个硬件平台
	//const GLubyte* OpenGLVersion = glGetString(GL_VERSION); //返回当前OpenGL实现的版本号
	//const GLubyte* gluVersion = gluGetString(GLU_VERSION); //返回当前GLU工具库版本
	//printf("OpenGL实现厂商的名字：%s\n", name);
	//printf("渲染器标识符：%s\n", biaoshifu);
	//printf("OpenGL实现的版本号：%s\n", OpenGLVersion);
	//printf("OGLU工具库版本：%s\n", gluVersion);
	////return 0;

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_ALWAYS); 
	glDepthRange(cam.depth_near, cam.depth_far);
	//glEnable(GL_LIGHTING);
	glutMainLoop();
	return 0;
}

