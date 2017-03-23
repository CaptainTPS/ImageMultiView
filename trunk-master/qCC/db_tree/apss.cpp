#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "apss.h"
#include "MeshRender.h"

#define ISOVALUE 0
#define MINVALUE 1e-9
#define MAXDISTANCE 10

/*
15 cases of different marching cube
Matt's Webcorner Stanford 2014 Marching Cubes
										index								value
									8 vertex(0 for positive)			12 edge(1 for interpolate)(center back front)
case 1 (0 different point):			0000 0000; 1111 1111;				0000 0000 0000;
case 2 (1 different point):			0000 0001; 0000 0010;				0001 0000 1001;
									0000 0100; ...
case 3 (2 different point):			0000 0011; 0000 0110;				0011 0000 1010;
									0000 1100; ...
case 4 (2 different point):			0000 0101; 0000 1010;				0101 0000 1111;
									1010 0000; ...
case 5 (3 different point):
case 6 (4 different point):
case 7 (4 different point):
case 8 (4 different point):
case 9 (4 different point):
case 10(4 different point):
case 11(2 different point):			0100 0001; 1000 0010;				0101 0110 1001;
									0001 0100; 0010 1000;
case 12(3 different point):
case 13(3 different point):
case 14(4 different point):
case 15(4 different point):

*/
//not exactly the same as above, 128 values totally

int mcEdgeTable[256] = {
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 };

int mcTriTable[256][16] = {
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
	{ 8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1 },
	{ 3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1 },
	{ 4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
	{ 4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1 },
	{ 9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1 },
	{ 10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1 },
	{ 5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1 },
	{ 5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1 },
	{ 8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1 },
	{ 2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
	{ 2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1 },
	{ 11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1 },
	{ 5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1 },
	{ 11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1 },
	{ 11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1 },
	{ 2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1 },
	{ 6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
	{ 3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1 },
	{ 6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1 },
	{ 6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1 },
	{ 8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1 },
	{ 7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1 },
	{ 3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1 },
	{ 0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1 },
	{ 9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1 },
	{ 8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1 },
	{ 5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1 },
	{ 0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1 },
	{ 6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1 },
	{ 10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
	{ 1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1 },
	{ 0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1 },
	{ 3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1 },
	{ 6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1 },
	{ 9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1 },
	{ 8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1 },
	{ 3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1 },
	{ 6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1 },
	{ 10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1 },
	{ 10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
	{ 2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1 },
	{ 7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1 },
	{ 7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1 },
	{ 2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1 },
	{ 1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1 },
	{ 11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1 },
	{ 8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1 },
	{ 0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1 },
	{ 7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1 },
	{ 7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1 },
	{ 10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1 },
	{ 0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1 },
	{ 7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1 },
	{ 6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1 },
	{ 6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1 },
	{ 4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1 },
	{ 10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1 },
	{ 8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1 },
	{ 1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1 },
	{ 10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1 },
	{ 10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1 },
	{ 9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1 },
	{ 7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1 },
	{ 3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1 },
	{ 7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1 },
	{ 3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1 },
	{ 6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1 },
	{ 9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1 },
	{ 1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1 },
	{ 4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1 },
	{ 7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1 },
	{ 6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1 },
	{ 0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1 },
	{ 6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1 },
	{ 0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1 },
	{ 11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1 },
	{ 6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1 },
	{ 5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1 },
	{ 9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1 },
	{ 1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1 },
	{ 10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1 },
	{ 0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1 },
	{ 10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1 },
	{ 11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1 },
	{ 9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1 },
	{ 7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1 },
	{ 2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1 },
	{ 9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1 },
	{ 9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1 },
	{ 1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
	{ 5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1 },
	{ 0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1 },
	{ 10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1 },
	{ 2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1 },
	{ 0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1 },
	{ 0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1 },
	{ 9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1 },
	{ 5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1 },
	{ 5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1 },
	{ 8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1 },
	{ 9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1 },
	{ 1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1 },
	{ 3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1 },
	{ 4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1 },
	{ 9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1 },
	{ 11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1 },
	{ 11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1 },
	{ 2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1 },
	{ 9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1 },
	{ 3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1 },
	{ 1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1 },
	{ 4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1 },
	{ 0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1 },
	{ 9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1 },
	{ 1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ 0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
	{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 } 
};



AlgebraicSurface::AlgebraicSurface()
{
	for (int i = 0; i < 3; i++) {
		size[i] = 0;
		length[i] = 1.0;
	}
	sphereNum = 0;
	volume_ = NULL;
	volumeSig_ = NULL;
	u_ = NULL;
	pt_ = NULL;
	index_ = NULL;
	//store one point for current search
	query = flann::Matrix<float>(new float[3], 1, 3);
}

AlgebraicSurface::~AlgebraicSurface()
{
	if (volume_ != NULL)
		delete[] volume_;
	if (volumeSig_ != NULL)
		delete[] volumeSig_;
	if (u_ != NULL)
		delete[] u_;
	if (pt_ != NULL)
		delete[] pt_;
	if (index_ != NULL){
		delete[] dataset.ptr();
		delete index_;
	}
	delete[] query.ptr();
}

std::ostream & operator<< (std::ostream &out, AlgebraicSurface::point3& t){
	out << "(" << t.px << ", " << t.py << ", " << t.pz<<")";
	return out;
};

float* AlgebraicSurface::volume(int x, int y, int z) {
	if (x >= size[0] || y >= size[1] || z >= size[2])
		return NULL;
	return &volume_[x + y*(size[0]) + z*(size[0] * size[1])];
}

bool* AlgebraicSurface::volumeSig(int x, int y, int z){
	if (x >= size[0] || y >= size[1] || z >= size[2])
		return NULL;
	return &volumeSig_[x + y*(size[0]) + z*(size[0] * size[1])];
}

void AlgebraicSurface::setVolumeSize(int x, int y, int z) {
	volume_ = new float[x*y*z];
	volumeSig_ = new bool[x*y*z];
	size[0] = x;
	size[1] = y;
	size[2] = z;
	for (int i = 0; i < x; i++)
	{
		for (int j = 0; j < y; j++)
		{
			for (int k = 0; k < z; k++)
			{
				*(volume(i, j, k)) = 0;
				*(volumeSig(i, j, k)) = false;
			}
		}
	}
}

void AlgebraicSurface::setVolumeBox(point3 ori, float length_x, float length_y, float length_z)
{
	originalCorner = ori;
	length[0] = length_x;
	length[1] = length_y;
	length[2] = length_z;
}

AlgebraicSurface::point3 AlgebraicSurface::voluemCoor(int x, int y, int z)
{
	float xv, yv, zv;
	xv = originalCorner.px + x* length[0] / size[0];
	yv = originalCorner.py + y* length[1] / size[1];
	zv = originalCorner.pz + z* length[2] / size[2];
	return point3(xv, yv, zv);
}

void AlgebraicSurface::initSpheres(int num) {
	u_ = new float[5 * num];
	pt_ = new point3[num];
	sphereNum = 0;
}

void AlgebraicSurface::setSphere(float * u, float x, float y, float z, int index)
{
	if (u_ == NULL)
		return;
	for (int i = 0; i < 5; i++)
	{
		u_[5 * index + i] = u[i];
	}
	pt_[index].px = x;
	pt_[index].py = y;
	pt_[index].pz = z;

	sphereNum++;
}

void AlgebraicSurface::setSphere(float * u, point3 pt, int index){
	setSphere(u, pt.px, pt.py, pt.pz, index);
}

float* AlgebraicSurface::getSphere(int index)
{
	if (u_ == NULL)
		return NULL;
	return &u_[5 * index];
}

AlgebraicSurface::point3 AlgebraicSurface::getSpherePoint(int index)
{
	if (pt_ == NULL)
		return point3();
	return pt_[index];
}

void ptToImageCoor(AlgebraicSurface::point3 pt, int width, int height, float f, float lperPixel, int &x, int &y){
	float ratio = f / abs(pt.pz);
	float x_img = pt.px * ratio;
	float y_img = pt.py * ratio;
	x = (int)(x_img / lperPixel + width / 2);
	y = (int)(height / 2 - y_img / lperPixel);
}

void AlgebraicSurface::calVolumeSig(vector<vector<float>> depth, float f, float lperPixel, int dilateVoxelNum){
	// under opengl frame, depth is negtive
	int img_height = depth.size();
	int img_width = depth[0].size();
	vector<int> xv, yv, zv;
	for (int x = 0; x < size[0]; x++)
		for (int y = 0; y < size[1]; y++)
			for (int z = 0; z < size[2]; z++)
			{
				point3 pt = voluemCoor(x, y, z);
				int posx, posy;
				ptToImageCoor(pt, img_width, img_height, f, lperPixel, posx, posy);
				if (posx >= 0 && posx < img_width && posy >= 0 && posy < img_height)
				{
					if (abs(pt.pz - depth[posy][posx]) < (length[2]/size[2]))
					{
						xv.push_back(x);
						yv.push_back(y);
						zv.push_back(z);
					}
				}
			}

	//dilate
	for (int i = 0; i < xv.size(); i++)
	{
		for (int mx = -dilateVoxelNum; mx < dilateVoxelNum; mx++)
		{
			for (int my = -dilateVoxelNum; my < dilateVoxelNum; my++)
			{
				for (int mz = -dilateVoxelNum; mz < dilateVoxelNum; mz++)
				{
					int x_d = xv[i] + mx;
					int y_d = yv[i] + my;
					int z_d = zv[i] + mz;
					if (x_d < 0 || y_d < 0 || z_d < 0 || x_d >= size[0] || y_d >= size[1] || z_d >= size[2])
						continue;
					if (*volumeSig(x_d, y_d, z_d) == false)
					{
						*volumeSig(x_d, y_d, z_d) = true;
					}
				}
			}
		}
	}
}

float distancePoint3(AlgebraicSurface::point3 p1, AlgebraicSurface::point3 p2) {
	float t = 0;
	t += (p1.px - p2.px)*(p1.px - p2.px);
	t += (p1.py - p2.py)*(p1.py - p2.py);
	t += (p1.pz - p2.pz)*(p1.pz - p2.pz);
	return sqrt(t);
}

void AlgebraicSurface::findNeighbor(point3 p1, std::vector<int>* neighbor, int number)
{
	if (number > sphereNum)
		return;
	struct pointAindex
	{
		float d;
		int index;

		bool operator < (pointAindex a) {
			return (d < a.d);
		}
	};

	pointAindex tempPI;
	std::vector<pointAindex> tp;
	for (int i = 0; i < sphereNum; i++)
	{
		tempPI.d = distancePoint3(p1, getSpherePoint(i));
		tempPI.index = i;
		tp.push_back(tempPI);
	}
	std::sort(tp.begin(), tp.end());
	for (int i = 0; i < number; i++)
	{
		neighbor->push_back(tp[i].index);
	}
}

void AlgebraicSurface::findNeighborKDtree(point3 p1, std::vector<int> *neighbor, int number){
	if (index_ == NULL)
	{
		//build kd tree

		int dims[2];
		dims[0] = sphereNum;
		dims[1] = 3;
		dataset = flann::Matrix<float>(new float[dims[0] * dims[1]], dims[0], dims[1]);
		float* data_ptr;
		point3 pt;
		for (int n = 0; n < sphereNum; n++)
		{
			data_ptr = dataset[n];
			pt = getSpherePoint(n);
			*(data_ptr++) = pt.px;
			*(data_ptr++) = pt.py;
			*(data_ptr++) = pt.pz;
		}
		index_ = new flann::Index<flann::L2_3D<float>>(dataset, flann::KDTreeIndexParams(1));
		(*index_).buildIndex();
	}
	float* ptr = query.ptr();
	*(ptr++) = p1.px;
	*(ptr++) = p1.py;
	*(ptr++) = p1.pz;

	indices = Matrix<int>(new int[query.rows*number], query.rows, number);
	dists = Matrix<float>(new float[query.rows*number], query.rows, number);

	(*index_).knnSearch(query, indices, dists, number, flann::SearchParams(flann::FLANN_CHECKS_UNLIMITED));

	int *ptr_int = indices.ptr();
	for (int i = 0; i < number; i++)
	{
		neighbor->push_back((*ptr_int++));
	}
	delete[] indices.ptr();
	delete[] dists.ptr();
}

void AlgebraicSurface::findNeighborSphere(point3 p1, std::vector<int>* neighbor, int number)
{
	if (number > sphereNum)
		return;
	struct pointAindex
	{
		float d;
		int index;

		bool operator < (pointAindex a) {
			return (d < a.d);
		}
	};

	pointAindex tempPI;
	std::vector<pointAindex> tp;
	float* paraU;
	for (int i = 0; i < sphereNum; i++)
	{
		paraU = getSphere(i);
		// if it is a sphere
		if (paraU[4] > MINVALUE)
		{
			point3 sphereCenter = point3(-1 * paraU[1] / (2 * paraU[4]), -1 * paraU[2] / (2 * paraU[4]), -1 * paraU[3] / (2 * paraU[4]));
			float sphereRadius = sqrt(sphereCenter.px* sphereCenter.px + sphereCenter.py* sphereCenter.py + sphereCenter.pz* sphereCenter.pz - paraU[0] / paraU[4]);
			tempPI.d = distancePoint3(p1, sphereCenter);
			tempPI.d -= sphereRadius;
			//if it inside one sphere, then it should be inside even if it's outside another one
			//tempPI.d = abs(tempPI.d);
			tempPI.index = i;
			tp.push_back(tempPI);
		}
		//if it is a plane
		else
		{
			float a = paraU[1];
			float b = paraU[2];
			float c = paraU[3];
			tempPI.d = abs(a * p1.px + b * p1.py + c * p1.pz) / sqrt(a*a + b*b + c*c);
			tempPI.index = i;
			tp.push_back(tempPI);
		}
	}
	std::sort(tp.begin(), tp.end());
	for (int i = 0; i < number; i++)
	{
		neighbor->push_back(tp[i].index);
	}
}

void AlgebraicSurface::calculateVolume(int neighborNum)
{
#define USE_POINT_NEIGHBOR
	if (neighborNum > sphereNum)
	{
		std::cout << "Volume: not enough sphere !" << std::endl;
		return;
	}


	std::vector<int> neighborPoint;
	std::vector<int> neighborPoint_test;
	//int neighborNum = 1;
	point3 vc;
	float weight_sum;
	std::cout << "calculating volume values...";
	for (int x = 0; x < size[0]; x++)
	{
		//std::cout << "volume x:" << x << std::endl;
		for (int y = 0; y < size[1]; y++)
		{
			for (int z = 0; z < size[2]; z++)
			{
				if (*volumeSig(x, y, z) == false)
					continue;

				neighborPoint.clear();
				vc = voluemCoor(x, y, z);

				
#ifdef USE_POINT_NEIGHBOR
				//neighborPoint_test.clear();
				//findNeighbor(vc, &neighborPoint_test, neighborNum);
				
				findNeighborKDtree(vc, &neighborPoint, neighborNum);//not exactly the nearest
				//for (int i = 0; i < neighborNum; i++)
				//{
				//	//test
				//	if (neighborPoint[i] != neighborPoint_test[i])
				//	{
				//		point3 p1 = getSpherePoint(neighborPoint_test[i]);
				//		point3 p2 = getSpherePoint(neighborPoint[i]);
				//		std::cout << "wrong neighbor" << x << " " << y << " " << z << std::endl;
				//		std::cout << "vc " << vc << std::endl;
				//		std::cout << "sort: " << p1 << std::endl;
				//		std::cout<<"distance: "<<distancePoint3(vc,p1)<<std::endl;
				//		std::cout << "kdtree: " << p2 << std::endl;
				//		std::cout << "distance: " << distancePoint3(vc, p2) << std::endl;
				//		std::cout << std::endl;
				//	}
				//}
#else
				findNeighborSphere(vc, &neighborPoint, neighborNum);
#endif
				weight_sum = 0;


				for (int n = 0; n < neighborNum; n++)
				{
					int index = neighborPoint[n];
#ifdef USE_POINT_NEIGHBOR
					float dis = distancePoint3(vc, getSpherePoint(index));
#else
					//use the distance to the surface
					float dis;
					float* paraU = getSphere(index);
					// if it is a sphere
					if (paraU[4] > MINVALUE)
					{
						point3 sphereCenter = point3(-1 * paraU[1] / (2 * paraU[4]), -1 * paraU[2] / (2 * paraU[4]), -1 * paraU[3] / (2 * paraU[4]));
						float sphereRadius = sqrt(sphereCenter.px* sphereCenter.px + sphereCenter.py* sphereCenter.py + sphereCenter.pz* sphereCenter.pz - paraU[0] / paraU[4]);
						dis = distancePoint3(vc, sphereCenter);
						dis -= sphereRadius;
						dis = abs(dis);
					}
					//if it is a plane
					else
					{
						float a = paraU[1];
						float b = paraU[2];
						float c = paraU[3];
						dis = abs(a * vc.px + b * vc.py + c * vc.pz) / sqrt(a*a + b*b + c*c);
					}
#endif
					if (dis > MAXDISTANCE)
					{
						continue;
					}
					float* u;
					u = getSphere(index);
					float value = u[0];
					value += vc.px * u[1] + vc.py * u[2] + vc.pz * u[3];
					value += u[4] * (vc.px *vc.px + vc.py *vc.py + vc.pz * vc.pz);
					if (abs(value) >10)
					{
						value = value > 0 ? 10 : -10;
						//value = 1;
					}
					//test
					//if (value < 0 && abs(vc.px - (-2.288460)) < 0.1 && abs(vc.py - (-0.303836)) < 0.1 && abs(vc.pz - (-14.641600)) < 0.1)
					//{
					//	//test
					//	/*ofstream output("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\neighbor.obj");
					//	output << "v " << vc.px << " " << vc.py << " " << vc.pz << " " << 0 << " " << 255 << " " << 0 << endl<<endl;
					//	for (int kkk = 0; kkk < neighborPoint.size(); kkk++){
					//		output << "#" << neighborPoint[kkk] << endl;
					//		output << "v " << getSpherePoint(neighborPoint[kkk]).px << " " << getSpherePoint(neighborPoint[kkk]).py << " " << getSpherePoint(neighborPoint[kkk]).pz << " " << 0 << " " << 255 << " " << 0 << endl;
					//	}
					//	output.close();*/
					//}
					dis = dis < MINVALUE ? MINVALUE : dis;
					float weight = 1.0 / dis;
					weight = pow(weight, 4);

					*(volume(x, y, z)) += weight * value;
					weight_sum += weight;
				}
				
				if (weight_sum > 0)
				{
					*(volume(x, y, z)) /= weight_sum;
				}
				
			}
		}
	}
	std::cout << "done" << std::endl;
}

AlgebraicSurface::point3 interpolEdge(AlgebraicSurface::point3 p1, float value1, AlgebraicSurface::point3 p2, float value2) {
	float isoValue = ISOVALUE;
#if 0
	float epsilon = MINVALUE;
	if (fabs(isoValue - value1) < epsilon)
		return p1;
	if (fabs(isoValue - value2) < epsilon)
		return p2;
	if (fabs(value1 - value2) < epsilon)
		return p1;
#endif
	float a = (isoValue - value1) / (value2 - value1);
	return (p1 + (p2 - p1) * a);
}

void AlgebraicSurface::marchingCube()
{
	//std::vector<point3> testVertex;

	float* p[8];
	point3 p_coor[8];
	float isoValue = ISOVALUE;
	int ci[3];
	for (ci[0] = 0; ci[0] < size[0]-1; ci[0]++)
		for (ci[1] = 0; ci[1] < size[1]-1; ci[1]++)
			for (ci[2] = 0; ci[2] < size[2]-1; ci[2]++)
			{
				

				p[0] = volume(ci[0],		ci[1],		ci[2]);
				p[1] = volume(ci[0] + 1,	ci[1],		ci[2]);
				p[2] = volume(ci[0] + 1,	ci[1],		ci[2] + 1);
				p[3] = volume(ci[0],		ci[1],		ci[2] + 1);
				p[4] = volume(ci[0],		ci[1] + 1,	ci[2]);
				p[5] = volume(ci[0] + 1,	ci[1] + 1,	ci[2]);//
				p[6] = volume(ci[0] + 1,	ci[1] + 1,	ci[2] + 1);
				p[7] = volume(ci[0],		ci[1] + 1,	ci[2] + 1);

				//test
				/*if (280 == ci[0] && 391 == ci[1] && 127 == ci[2])
				{
					for (int i = 0; i < 8; i++)
					{
						std::cout << "pvalue: " << *p[i] << std::endl;
					}
				}*/
				

				int mask = 0;
				if (*(p[0]) < isoValue) mask |= 1;
				if (*(p[1]) < isoValue) mask |= 2;
				if (*(p[2]) < isoValue) mask |= 4;
				if (*(p[3]) < isoValue) mask |= 8;
				if (*(p[4]) < isoValue) mask |= 16;
				if (*(p[5]) < isoValue) mask |= 32;
				if (*(p[6]) < isoValue) mask |= 64;
				if (*(p[7]) < isoValue) mask |= 128;
				
				if (mcEdgeTable[mask] != 0) {
					p_coor[0] = voluemCoor(ci[0], ci[1], ci[2]);
					p_coor[1] = voluemCoor(ci[0] + 1, ci[1], ci[2]);
					p_coor[2] = voluemCoor(ci[0] + 1, ci[1], ci[2] + 1);
					p_coor[3] = voluemCoor(ci[0], ci[1], ci[2] + 1);
					p_coor[4] = voluemCoor(ci[0], ci[1] + 1, ci[2]);
					p_coor[5] = voluemCoor(ci[0] + 1, ci[1] + 1, ci[2]);
					p_coor[6] = voluemCoor(ci[0] + 1, ci[1] + 1, ci[2] + 1);
					p_coor[7] = voluemCoor(ci[0], ci[1] + 1, ci[2] + 1);
				
					point3 edges[12];
					if (mcEdgeTable[mask] & 1)
						edges[0] = interpolEdge(p_coor[0], *p[0], p_coor[1], *p[1]);
					if (mcEdgeTable[mask] & 2)
						edges[1] = interpolEdge(p_coor[1], *p[1], p_coor[2], *p[2]);
					if (mcEdgeTable[mask] & 4)
						edges[2] = interpolEdge(p_coor[2], *p[2], p_coor[3], *p[3]);
					if (mcEdgeTable[mask] & 8)
						edges[3] = interpolEdge(p_coor[3], *p[3], p_coor[0], *p[0]);
					if (mcEdgeTable[mask] & 16)
						edges[4] = interpolEdge(p_coor[4], *p[4], p_coor[5], *p[5]);
					if (mcEdgeTable[mask] & 32)
						edges[5] = interpolEdge(p_coor[5], *p[5], p_coor[6], *p[6]);
					if (mcEdgeTable[mask] & 64)
						edges[6] = interpolEdge(p_coor[6], *p[6], p_coor[7], *p[7]);
					if (mcEdgeTable[mask] & 128)
						edges[7] = interpolEdge(p_coor[7], *p[7], p_coor[4], *p[4]);
					if (mcEdgeTable[mask] & 256)
						edges[8] = interpolEdge(p_coor[0], *p[0], p_coor[4], *p[4]);
					if (mcEdgeTable[mask] & 512)
						edges[9] = interpolEdge(p_coor[1], *p[1], p_coor[5], *p[5]);
					if (mcEdgeTable[mask] & 1024)
						edges[10] = interpolEdge(p_coor[2], *p[2], p_coor[6], *p[6]);
					if (mcEdgeTable[mask] & 2048)
						edges[11] = interpolEdge(p_coor[3], *p[3], p_coor[7], *p[7]);

					for (int i = 0; mcTriTable[mask][i] != -1; i += 3)
					{
						//didn't deal the degenerated triangle and repeating vertex
						for (int j = 0; j<3; ++j)
						{
							point3 pe = edges[mcTriTable[mask][i + j]];
							allVertex.push_back(pe);

						}
					}
					//test
					/*if (280 == ci[0] && 391 == ci[1] && 127 == ci[2])
					{
						std::ofstream output("D:\\captainT\\project_13\\ImageMultiView\\Build\\data\\out\\testOut.obj");
						for (int i = 0; i < testVertex.size(); i++)
						{
							output << "v " << testVertex[i].px << " " << testVertex[i].py << " " << testVertex[i].pz << std::endl;
						}
						output << std::endl;
						for (int i = 0; i < testVertex.size(); i += 3)
						{
							output << "f " << i + 1 << " " << i + 2 << " " << i + 3 << std::endl;
						}
						output.close();
					}*/
				}
			}
}

void AlgebraicSurface::writeOBJ(std::string path)
{
	std::ofstream output(path);
	for (int i = 0; i < allVertex.size(); i++)
	{
		output << "v " << allVertex[i].px << " " << allVertex[i].py << " " << allVertex[i].pz << std::endl;
	}
	output << std::endl;
	for (int i = 0; i < allVertex.size(); i+=3)
	{
		output << "f " << i + 1 << " " << i + 2 << " " << i + 3 << std::endl;
	}
	output.close();
}

int AlgebraicSurface::getSphereNum(){
	return sphereNum;
}

void AlgebraicSurface::renderTriangles(cameraPara &cam, vector<float> &depth){
	//renderMesh(allVertex, cam, depth);
}

//for test 
void getSphereU(float c1, float c2, float c3, float r, float* u) {
	u[0] = c1*c1 + c2*c2 + c3*c3 - r*r;
	u[1] = -2 * c1;
	u[2] = -2 * c2;
	u[3] = -2 * c3;
	u[4] = 1;
}

#if 0
int main() {
	//(x-2)^2 + (y-2)^2 + (z-2)^2 = 1; pt (1,2,2)
	AlgebraicSurface as;
	as.setVolumeSize(20, 20, 20);
	as.setVolumeBox(AlgebraicSurface::point3(0, 0, 0), 5, 5, 5);
	int sphereNum = 2;
	int useNearestSphere = 1;
	
	/*as.initSpheres(sphereNum);
	float u[5] = {
		-5, 1, 1, 1, 0
	};
	as.setSphere(u, 1, 2, 2, 0);*/

	as.initSpheres(sphereNum);
	float spherePara[2][4] = {//center & radius
		2, 2, 2, 1,
		3.5, 2, 2, 1
	};
	float pointArray[2][3] = {
		2, 2, 1,
		3.5, 2, 1
	};
	float u[5];
	int index = -1;
	int sphere_offset = 0;
	for (int i = sphere_offset; i < sphereNum + sphere_offset; i++)
	{
		getSphereU(spherePara[i][0], spherePara[i][1], spherePara[i][2], spherePara[i][3], u);
		as.setSphere(u, pointArray[i][0], pointArray[i][1], pointArray[i][2], ++index);
	}

	as.calculateVolume(useNearestSphere);
	as.marchingCube();

	std::string path = "F:\\CaptainT\\Project_15\\Test\\t4_1n.obj";
	as.writeOBJ(path);
	return 0;
}
#endif