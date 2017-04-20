#ifndef PATCHDEALER_H
#define PATCHDEALER_H

#include "Eigen\Dense"
#include "Eigen\SVD"

#ifndef MINVALUE
#define MINVALUE 1e-4
#endif

//#define MY_DEBUG

#ifdef MY_DEBUG
	#include <iostream>
	using namespace std;
#endif

class patchDealer
{
public:
	patchDealer(double lengthPerPixel);
	~patchDealer();
	
	//0 for R, 1 for G, 2 for B
	void setColor(int index, int rgb_num, int value);
	void setDepth(int index, double value);
	void setXY(int index, double x, double y);
	void setCenter(int x, int y);
	void setNormal(int index, double x, double y, double z);

	void filterPoints(double gap);

	void calSphere();
	void renewNormals();
	void calSphereWithNormal();
	void adjustSphere();

	//result
	double getDepth(int index);
	void getXY(int index, double *x, double *y);
	int getNumOfPoints();
	double u[5];

//private:
public:
	int _color[9][3];//rgb
	double _normals[9][3];//x y z
	double _points[9][3];
	double _depth[9];
	double _xy[9][2];//the calculation using center and length per pixel is not right
	int _center[2];
	double _lengthPerPixel;
	int _NofPoints;
	
};

patchDealer::patchDealer(double lengthPerPixel = 1.0)
{
	for (size_t i = 0; i < 9; i++)
	{
		_color[i][0] = -1;
		_color[i][1] = -1;
		_color[i][2] = -1;

		_depth[i] = -1.0;

		_center[0] = -1;
		_center[1] = -1;

		_NofPoints = 0;

		u[i] = 0;

		_lengthPerPixel = lengthPerPixel;
	}
}

void patchDealer::setColor(int index, int rgb_num, int value){
	_color[index][rgb_num] = value;
}

void patchDealer::setDepth(int index, double value){
	_depth[index] = value;
}

void patchDealer::setXY(int index, double x, double y){
	_xy[index][0] = x;
	_xy[index][1] = y;
}

void patchDealer::setNormal(int index, double x, double y, double z){
	_normals[index][0] = x;
	_normals[index][1] = y;
	_normals[index][2] = z;
}

inline void patchDealer::filterPoints(double gap)
{
	_NofPoints = 0;
	for (int i = 0; i < 9; i++)
	{
		if (abs(_depth[i] - _depth[4]) > gap)
			continue;
		/*_points[_NofPoints][0] = (_center[0] + (i % 3) - 1) * _lengthPerPixel;
		_points[_NofPoints][1] = (_center[1] + (i / 3) - 1) * _lengthPerPixel;*/
		_points[_NofPoints][0] = _xy[i][0];
		_points[_NofPoints][1] = _xy[i][1];
		_points[_NofPoints][2] = _depth[i];
		_NofPoints++;
	}
}

void patchDealer::setCenter(int x, int y){
	_center[0] = x;
	_center[1] = y;
}

void normalize(double *x, double *y, double *z){
	double w = pow(*x, 2) + pow(*y, 2) + pow(*z, 2);
	w = sqrt(w);
	*x = (*x) / w;
	*y = (*y) / w;
	*z = (*z) / w;
}

void patchDealer::renewNormals(){
	//int NofPoints = 9;
	double x, y, z;
	double c[3];
	if (abs(u[4]) < MINVALUE)
	{
		//it is a plane 
		x = u[1];
		y = u[2];
		z = u[3];
		normalize(&x, &y, &z);
		if (z < 0)
		{
			x *= -1;
			y *= -1;
			z *= -1;
		}
		for (int i = 0; i < _NofPoints; i++)
		{
			setNormal(i, x, y, z);
		}
	}
	else
	{
		//it is a sphere
		c[0] = -1 * u[1] / (2 * u[4]);
		c[1] = -1 * u[2] / (2 * u[4]);
		c[2] = -1 * u[3] / (2 * u[4]);
		for (int i = 0; i < _NofPoints; i++)
		{
			x = _points[i][0];
			y = _points[i][1];
			z = _points[i][2];
			x = x - c[0];
			y = y - c[1];
			z = z - c[2];
			normalize(&x, &y, &z);
			if (z < 0)
			{
				x *= -1;
				y *= -1;
				z *= -1;
			}
			setNormal(i, x, y, z);
		}
	}
}

/*
	based on the paper 
	Hyperspheres and hyperplanes fitted seamlessly by algebraic constrained total least - squares
	Yves Nievergelt
	page 6
*/
void patchDealer::calSphere(){
	//int NofPoints = 9;//the number of points whose depth is acceptable
	if (_NofPoints<=2)
	{
		//set to be a plane across (0,0,0)
		u[0] = 0;
		u[1] = 0;
		u[2] = 0;
		u[3] = -1;
		u[4] = 0;
		return;
	}
	else if (_NofPoints <=4)
	{
		//use cross product
		double ax = _points[0][0] - _points[1][0];
		double ay = _points[0][1] - _points[1][1];
		double az = _points[0][2] - _points[1][2];

		double bx = _points[0][0] - _points[2][0];
		double by = _points[0][1] - _points[2][1];
		double bz = _points[0][2] - _points[2][2];

		u[1] = ay*bz - az*by;
		u[2] = az*bx - ax*bz;
		u[3] = ax*by - ay*bx;

		u[4] = 0;
		u[0] = -1.0 * (_points[0][0] * u[1] + _points[0][1] * u[2] + _points[0][2] * u[3]);

		if (u[0] < MINVALUE && u[1] < MINVALUE &&u[2] < MINVALUE &&u[3] < MINVALUE &&u[4] < MINVALUE){
			u[3] = -1;
		}

		return;
	}

	Eigen::MatrixXd pointSet(_NofPoints, 3);
	for (int i = 0; i < _NofPoints; i++)
	{
		pointSet(i, 0) = _points[i][0];
		pointSet(i, 1) = _points[i][1];
		pointSet(i, 2) = _points[i][2];
	}

	/*for (int i = 0; i < _NofPoints; i++)
	{
		pointSet(i, 0) = (_center[0] + (i % 3) - 1) * _lengthPerPixel;
		pointSet(i, 1) = (_center[1] + (i / 3) - 1) * _lengthPerPixel;
		pointSet(i, 2) = _depth[i];
	}*/

	// Step 1: compute the mean
	Eigen::MatrixXd weightMatrix(1, _NofPoints);
	for (int i = 0; i<_NofPoints; ++i)
	{
		weightMatrix(0, i) = 1.0 / _NofPoints;
	}
	Eigen::Matrix<double, 1, 3, Eigen::RowMajor> mean;
	mean = weightMatrix * pointSet;

	// compute the variance
	Eigen::MatrixXd varMat(1, 3* _NofPoints);
	for (int i = 0; i<_NofPoints; ++i)
	{
		varMat.block(0, i*3, 1, 3) = pointSet.row(i) - mean;
	}
	double variance = (varMat * varMat.transpose())(0,0) / _NofPoints;

	// Step 2: compute the SVD of Q"A2
	Eigen::MatrixXd matQ_A2(_NofPoints, 4);
	Eigen::Matrix<double, 1, 3, Eigen::RowMajor> oneVar;
	for (int i = 0; i<_NofPoints; ++i)
	{
		oneVar = varMat.block(0, i * 3, 1, 3);
		matQ_A2.block(i, 0, 1, 3) = oneVar;
		(matQ_A2.block(i, 3, 1, 1))(0,0) = (oneVar * oneVar.transpose())(0,0) - variance;
	}

	//S: NofPoints * 4(diagonal); 
	//V: 4*4
	Eigen::JacobiSVD<Eigen::MatrixXd> decom_svd(matQ_A2, Eigen::ComputeFullV);
	Eigen::ArrayXd svd_S = decom_svd.singularValues();
	Eigen::MatrixXd svd_V = decom_svd.matrixV();

	//result
	for (int i = 1; i < 5; ++i)
		u[i] = svd_V.col(3)(i - 1, 0);
	u[0] = -variance*u[4];

	//move back to original frame; now the frame origin is at the mean point
	Eigen::Matrix<double, 3, 1, Eigen::ColMajor> M;
	M = mean.transpose();
	Eigen::Matrix<double, 3, 1, Eigen::ColMajor> U1_3;
	U1_3(0, 0) = u[1]; 
	U1_3(1, 0) = u[2]; 
	U1_3(2, 0) = u[3];
	
	double nu0 = u[0] - (U1_3.transpose() * M)(0, 0) + u[4] * (M.transpose() * M)(0, 0);
	U1_3 = U1_3 - 2 * u[4] * M;
	u[1] = U1_3(0, 0);
	u[2] = U1_3(1, 0);
	u[3] = U1_3(2, 0);
	u[0] = nu0;

}

/*
	based on the paper
	Algebraic Point Set Surfaces
	Gael Guennebaud		Markus Gross	ETH Zurich
	page 4
*/
void patchDealer::calSphereWithNormal(){
	double beta = 1000.0;
	beta = beta * beta;

	//int NofPoints = 9;
	if (_NofPoints == 0)
	{
		return;
	}
	else if (_NofPoints == 1)
	{
		//the center point
		u[1] = _normals[4][0];
		u[2] = _normals[4][1];
		u[3] = _normals[4][2];

		int i = 4;
		double x = _points[i][0];
		double y = _points[i][1];
		double z = _points[i][2];
		//a plane
		u[0] = -1.0 * (x*u[1] + y*u[2] + z*u[3]);
		u[4] = 0.;
		return;
	}

	/** We directly compute the symetric covariance matrix taking advantage of the structure of the design matrix A:

	A'A = s(wi)     s(wi*xi)				s(wi*yi)				s(wi*zi)				s(wi*|pi|^2)
			"		s(wi*xi^2)+belta*s(wi)  s(wi*xi.yi)				s(wi*xi.zi)				s(wi*xi.|pi|^2) + 2belta*s(wi*xi)
			"        "						s(wi*yi^2)+belta*s(wi)  s(wi*yi.zi)				s(wi*yi.|pi|^2) + 2belta*s(wi*yi)
			"        "						"						s(wi*zi^2)+belta*s(wi)  s(wi*zi.|pi|^2) + 2belta*s(wi*zi)
			"        "						"						"						s(wi*|pi|^4) + 4belta*s(wi*|pi|^2)

	and A'b = 0 belta*s(wi*nxi) belta*s(wi*nyi) belta*s(wi*nzi) 2.belta*(s(wi*(xi.nxi)+(yi.nyi)+(zi.nzi)))

	where s(xi) means \sum {x_i} .

	normal_i = [nxi nyi nzi] .

	here A'A * X = A'b -> solve X; in paper D'WD * X = D'Wb -> solve X;
	*/

	Eigen::Matrix<double, 5, 1> mVecB;
	Eigen::Matrix<double, 5, 5, Eigen::RowMajor> mCovMat;
	// clear the data
	for (int i = 0; i<5; ++i)
	{
		mVecB(i,0) = 0.0;
		for (int j = 0; j<5; ++j)
			mCovMat(i,j) = 0.0;
	}

	for (int i = 0; i < _NofPoints; i++)
	{
		double wi = 1.0 / _NofPoints;
		//position
		double xi = _points[i][0];
		double yi = _points[i][1];
		double zi = _points[i][2];
		double l2 = xi*xi + yi*yi + zi*zi;
		//normal
		double nx = _normals[i][0];
		double ny = _normals[i][1];
		double nz = _normals[i][2];

		mCovMat(0,0) += wi;

		mCovMat(1, 1) += wi*xi*xi;
		mCovMat(2, 2) += wi*yi*yi;
		mCovMat(3, 3) += wi*zi*zi;
		mCovMat(4, 4) += wi*l2*l2;

		mCovMat(0, 1) += wi*xi;
		mCovMat(0, 2) += wi*yi;
		mCovMat(0, 3) += wi*zi;
		mCovMat(0, 4) += wi*l2;

		mCovMat(1, 2) += wi*xi*yi;
		mCovMat(1, 3) += wi*xi*zi;
		mCovMat(2, 3) += wi*yi*zi;

		mCovMat(1, 4) += wi*xi*l2;
		mCovMat(2, 4) += wi*yi*l2;
		mCovMat(3, 4) += wi*zi*l2;

		mVecB(1,0) += wi*nx;
		mVecB(2,0) += wi*ny;
		mVecB(3,0) += wi*nz;
		mVecB(4,0) += wi*(xi*nx + yi*ny + zi*nz);
	}

	mCovMat(1, 4) += beta*2.0*mCovMat(0, 1);
	mCovMat(2, 4) += beta*2.0*mCovMat(0, 2);
	mCovMat(3, 4) += beta*2.0*mCovMat(0, 3);
	mCovMat(4, 4) += beta*4.0*(mCovMat(1, 1) + mCovMat(2, 2) + mCovMat(3, 3));

	mCovMat(1, 1) += beta*mCovMat(0, 0);
	mCovMat(2, 2) += beta*mCovMat(0, 0);
	mCovMat(3, 3) += beta*mCovMat(0, 0);

	mVecB(1, 0) *= beta;
	mVecB(2, 0) *= beta;
	mVecB(3, 0) *= beta;
	mVecB(4, 0) *= beta*2.0;

	//test
	Eigen::Matrix<double, 5, 5, Eigen::RowMajor> covMatCopy;
	covMatCopy = mCovMat;
	Eigen::Matrix<double, 5, 1> VecBCopy;
	VecBCopy = mVecB;

#if 0
	// seems to be Eigendecomposition; only the upper triangle part of mCovMat is valid.
	// here is the problem: D is not a diagonal matrix
	Eigen::Matrix<double, 5, 5, Eigen::RowMajor> mMatU;
	int N = 5;
	for (int i = 0; i<N; ++i)
	{
		for (int j = i + 1; j<N; ++j)
		{
			mMatU(i, j) = mCovMat(i, j) / mCovMat(i, i);
			for (int k = j; k<N; ++k)
				mCovMat(j, k) -= mMatU(i, j) * mCovMat(i, k);
		}
	}

	// forward substitution : x <- inv(U') * x
	for (int i = 1; i<5; ++i)
	{
		for (int j = 0; j<i; ++j)
			mVecB(i,0) -= mMatU(j, i) * mVecB(j,0);
	}
	cout << mCovMat << endl;
	// x <- inv(D) * x  (remind that D == matcov)
	for (int i = 0; i<5; ++i)
		mVecB(i,0) /= mCovMat(i, i);

	// backsubstitution : x <- inv(U) * x
	for (int i = 5 - 2; i >= 0; --i)
	{
		for (int j = i + 1; j < 5; j++)
			mVecB(i,0) -= mMatU(i, j) * mVecB(j,0);
	}

	//cout << "left:" << endl << covMatCopy * mVecB << endl;
	//cout << "right" << endl << VecBCopy << endl;


	// copy
	for (int i = 0; i<5; ++i)
		u[i] = mVecB(i,0);
#endif
	int N = 5;
	//fill the lower part
	for (int  i = 0; i < N; i++)
	{
		for (int j = 0; j < i; j++)
		{
			mCovMat(i, j) = mCovMat(j, i);
		}
	}

	
	//add a big value to make u[4] -> 1; D1 is [0 0 0 0 big_value]; b1 = big_value
#if 0 //result not good
#define BIG_VALUE 10000;
	Eigen::MatrixXd D1TD1(5, 5);
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			D1TD1(i, j) = 0;
		}
	}
	D1TD1(4, 4) = BIG_VALUE;
	D1TD1(4, 4) *= BIG_VALUE;

	Eigen::MatrixXd D1TB1(5, 1);
	for (int i = 0; i < 4; i++)
	{
		D1TB1(i, 0) = 0;
	}
	D1TB1(4, 0) = BIG_VALUE;
	D1TB1(4, 0) *= BIG_VALUE;

	mCovMat += D1TD1;
	mVecB += D1TB1;
#endif
	//svd
	Eigen::Matrix<double, 5, 1> result;
	result = mCovMat.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(mVecB);

	// copy
	for (int i = 0; i<5; ++i)
		u[i] = result(i, 0);
}

void patchDealer::adjustSphere(){
	if (abs(u[4]) < 1e-10 )
	{
		//considered as plane, make sure f(x) increases along positive z axis
		if (u[3] < 0)
		{
			u[0] *= -1.0;
			u[1] *= -1.0;
			u[2] *= -1.0;
			u[3] *= -1.0;
			u[4] *= -1.0;
		}
	}
	else if (u[4] < 0)
	{
		/*
		if u[4] < 0, means that the derivate orientation is opposite with what we thought of normals; flip it
		flip on point X1: C1 = 2*X1 - C; r*r = CTC-u0/u4;
		new sphere: (X-C1)T(X-C1) = r*r
		*/
		double X1_X = 0;
		double X1_Y = 0;
		double X1_Z = 0;
		for (int i = 0; i < _NofPoints; i++)
		{
			X1_X += _points[i][0];
			X1_Y += _points[i][1];
			X1_Z += _points[i][2];
		}
		X1_X /= 1.0 * _NofPoints;
		X1_Y /= 1.0 * _NofPoints;
		X1_Z /= 1.0 * _NofPoints;

		//double C[3];
		//C[0] = -1.0 / (2 * u[4]) * u[1];
		//C[1] = -1.0 / (2 * u[4]) * u[2];
		//C[2] = -1.0 / (2 * u[4]) * u[3];
		//
		//// || X1-C ||
		//double dist_x1_c = X1_X - C[1]

		//flip
		double new_u[5];
		new_u[4] = -1.0 * u[4];

		new_u[1] = 4 * u[4] * X1_X + u[1];
		new_u[2] = 4 * u[4] * X1_Y + u[2];
		new_u[3] = 4 * u[4] * X1_Z + u[3];

		double X1TX1 = X1_X * X1_X + X1_Y * X1_Y + X1_Z * X1_Z;
		new_u[0] = -4.0 * u[4] * X1TX1;
		new_u[0] -= u[0];
		new_u[0] += -2.0 * (X1_X*u[1] + X1_Y*u[2] + X1_Z*u[3]);

		for (int i = 0; i < 5; i++)
		{
			u[i] = new_u[i];
		}
	}
}


double patchDealer::getDepth(int index){
	return _depth[index];
}

void patchDealer::getXY(int index, double *x, double *y){
	*x = _xy[index][0];
	*y = _xy[index][1];
}

inline int patchDealer::getNumOfPoints()
{
	return _NofPoints;
}

patchDealer::~patchDealer()
{
}

#endif