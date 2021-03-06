#ifndef NEW_PATCHDEALER_H
#define NEW_PATCHDEALER_H

#include <vector>

#include "Eigen\Dense"
#include "Eigen\SVD"

#ifndef MINVALUE
#define MINVALUE 1e-4
#endif

using namespace std;

class newPatchDealer{
public:
	newPatchDealer();
	~newPatchDealer();
	void setCenterPoint(double x, double y, double z);
	void addPoint(double x, double y, double z, double gap);
	void addNormal(double x, double y, double z);

	void calSphere();
	void renewNormals();
	void calSphereWithNormal();
	void adjustSphere();

	void clearData();

	int getNumOfPoints();
	void getCenter(double *x, double *y, double *z);
	double u[5];

//private:
public:
	int maxPoints;
	double CPoint[3];//should be equal to _points[0]
	std::vector<std::vector<double>> _points;
	std::vector<std::vector<double>> _normals;

};

void normalize(double *x, double *y, double *z){
	double w = pow(*x, 2) + pow(*y, 2) + pow(*z, 2);
	w = sqrt(w);
	*x = (*x) / w;
	*y = (*y) / w;
	*z = (*z) / w;
}

newPatchDealer::newPatchDealer(){
	CPoint[0] = 0;
	CPoint[1] = 0;
	CPoint[2] = 0;

	maxPoints = 9;
}

newPatchDealer::~newPatchDealer(){
	
}

void newPatchDealer::setCenterPoint(double x, double y, double z){
	CPoint[0] = x;
	CPoint[1] = y;
	CPoint[2] = z;
}

void newPatchDealer::addPoint(double x, double y, double z, double gap){
	if (abs(z - CPoint[2])>gap)
	{
		return;
	}
	if (_points.size() == maxPoints)
	{
		return;
	}
	vector<double> temp;
	temp.push_back(x);
	temp.push_back(y);
	temp.push_back(z);

	_points.push_back(temp);
}

void newPatchDealer::addNormal(double x, double y, double z){
	vector<double> temp;
	temp.push_back(x);
	temp.push_back(y);
	temp.push_back(z);

	_normals.push_back(temp);
}

int newPatchDealer::getNumOfPoints(){
	return _points.size();
}

void newPatchDealer::getCenter(double *x, double *y, double *z){
	*x = CPoint[0];
	*y = CPoint[1];
	*z = CPoint[2];
}

void newPatchDealer::clearData(){
	CPoint[0] = 0;
	CPoint[1] = 0;
	CPoint[2] = 0;

	_points.clear();
	_normals.clear();
}

void newPatchDealer::calSphere(){
	int _NofPoints = _points.size();
	if (_NofPoints <= 2)
	{
		//set to be a plane across (0,0,0)
		u[0] = 0;
		u[1] = 0;
		u[2] = 0;
		u[3] = -1;
		u[4] = 0;
		return;
	}
	else if (_NofPoints <= 4)
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
	Eigen::MatrixXd varMat(1, 3 * _NofPoints);
	for (int i = 0; i<_NofPoints; ++i)
	{
		varMat.block(0, i * 3, 1, 3) = pointSet.row(i) - mean;
	}
	double variance = (varMat * varMat.transpose())(0, 0) / _NofPoints;

	// Step 2: compute the SVD of Q"A2
	Eigen::MatrixXd matQ_A2(_NofPoints, 4);
	Eigen::Matrix<double, 1, 3, Eigen::RowMajor> oneVar;
	for (int i = 0; i<_NofPoints; ++i)
	{
		oneVar = varMat.block(0, i * 3, 1, 3);
		matQ_A2.block(i, 0, 1, 3) = oneVar;
		(matQ_A2.block(i, 3, 1, 1))(0, 0) = (oneVar * oneVar.transpose())(0, 0) - variance;
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

void newPatchDealer::renewNormals(){
	int _NofPoints = _points.size();
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
			addNormal(x, y, z);
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
			addNormal(x, y, z);
		}
	}
}

void newPatchDealer::calSphereWithNormal(){
	double beta = 1000.0;
	beta = beta * beta;

	int _NofPoints = _points.size();
	if (_NofPoints == 0)
	{
		return;
	}
	else if (_NofPoints == 1)
	{
		//the center point
		u[1] = _normals[0][0];
		u[2] = _normals[0][1];
		u[3] = _normals[0][2];

		double x = _points[0][0];
		double y = _points[0][1];
		double z = _points[0][2];
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
		mVecB(i, 0) = 0.0;
		for (int j = 0; j<5; ++j)
			mCovMat(i, j) = 0.0;
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

		mCovMat(0, 0) += wi;

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

		mVecB(1, 0) += wi*nx;
		mVecB(2, 0) += wi*ny;
		mVecB(3, 0) += wi*nz;
		mVecB(4, 0) += wi*(xi*nx + yi*ny + zi*nz);
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
			mVecB(i, 0) -= mMatU(j, i) * mVecB(j, 0);
	}
	cout << mCovMat << endl;
	// x <- inv(D) * x  (remind that D == matcov)
	for (int i = 0; i<5; ++i)
		mVecB(i, 0) /= mCovMat(i, i);

	// backsubstitution : x <- inv(U) * x
	for (int i = 5 - 2; i >= 0; --i)
	{
		for (int j = i + 1; j < 5; j++)
			mVecB(i, 0) -= mMatU(i, j) * mVecB(j, 0);
	}

	//cout << "left:" << endl << covMatCopy * mVecB << endl;
	//cout << "right" << endl << VecBCopy << endl;


	// copy
	for (int i = 0; i<5; ++i)
		u[i] = mVecB(i, 0);
#endif
	int N = 5;
	//fill the lower part
	for (int i = 0; i < N; i++)
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

void newPatchDealer::adjustSphere(){
	if (abs(u[4]) < MINVALUE)
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
		int _NofPoints = _points.size();
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


#endif