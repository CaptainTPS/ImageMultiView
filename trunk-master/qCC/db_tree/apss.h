#ifndef APSS_H_
#define APSS_H_

//APSS volume and marching cube
#include <vector>

#include "flann\flann.h"
#include "..\..\openglGetDepth\mydata.h"

class AlgebraicSurface
{
public:
	struct point3
	{
		float px, py, pz;
		point3() {
			px = 0;
			py = 0;
			pz = 0;
		}

		point3(float x, float y, float z) {
			px = x; py = y; pz = z;
		}

		point3 operator- (point3 a) {
			return point3(px - a.px, py - a.py, pz - a.pz);
		}

		point3 operator+ (point3 a) {
			return point3(px + a.px, py + a.py, pz + a.pz);
		}

		point3 operator* (float a) {
			return point3(px*a, py*a, pz*a);
		}

	};

	AlgebraicSurface();
	~AlgebraicSurface();
	//volume
	float* volume(int x, int y, int z);
	bool* volumeSig(int x, int y, int z);
	void setVolumeSize(int x, int y, int z);
	void setVolumeBox(point3 ori, float length_x, float length_y, float length_z);
	point3 voluemCoor(int x, int y, int z);

	//spheres
	void initSpheres(int num);
	void setSphere(float* u, float x, float y, float z, int index);
	void setSphere(float* u, point3 pt, int index);
	float* getSphere(int index);
	point3 getSpherePoint(int index);

	//calculate

	//judge the voxels needed to be marching
	void calVolumeSig(vector<vector<float>> depth, float focalLength, float lperPixel, int dilateVoxelNum);
	//the nearest point and its sphere
	void findNeighbor(point3 p1, std::vector<int> *neighbor, int number);
	void findNeighborKDtree(point3 p1, std::vector<int> *neighbor, int number);
	//the nearest surface and its sphere
	void findNeighborSphere(point3 p1, std::vector<int> *neighbor, int number);
	void calculateVolume(int neighborNum);
	void marchingCube();

	//write out
	void writeOBJ(std::string path);
	int getSphereNum();

	//depth out
	void renderTriangles(cameraPara &cam, vector<float> &depth);

	//marching cube
	std::vector<point3> allVertex;

//private:
public:
	float* volume_;
	bool* volumeSig_;// true stands for needing marching cube
	int size[3];
	float length[3];
	point3 originalCorner;

	float* u_;
	point3* pt_;
	int sphereNum;

	

	//flann
	flann::Index<flann::L2_3D<float>> *index_;
	Matrix<float> dataset;
	Matrix<float> query;
	Matrix<int> indices;
	Matrix<float> dists;
};

#endif