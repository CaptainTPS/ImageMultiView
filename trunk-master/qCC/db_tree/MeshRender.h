#ifndef MESH_RENDER_H_
#define MESH_RENDER_H_

#include <cstdlib>
//#include <gl\GL.h>
//#include <gl\GLU.h>

#include "D:\captainT\project_13\ImageMultiView\openglGetDepth\mydata.h"

//#include "..\..\openglGetDepth\mydata.h"

#include "apss.h"

//void renderMesh(vector<AlgebraicSurface::point3> &vertex, cameraPara &cam, vector<float> &depth);

namespace test{
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
}
void renderMesh(vector<AlgebraicSurface::point3> &vertex, cameraPara &cam, vector<float> &depth);


#endif