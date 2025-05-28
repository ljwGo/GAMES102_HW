#pragma once
#include <Eigen/Core>
#include <random>

using namespace Eigen;

bool ClampOrNot(float val, float minVal, float maxVal, float epsilon = 1e-3) {
	return val > minVal - epsilon && val < maxVal + epsilon;
}

Vector2f solveLine(const Vector2f& v1, const Vector2f& v2) {
	Matrix2f A;
	Vector2f b, coef;
	A << v1.x(), 1,
		v2.x(), 1;

	assert(A.determinant() != 0.);

	b << v1.y(), v2.y();
	coef = A.inverse() * b;
	return coef;  // 斜率+截距
}

float solveLineC(const Vector2f& v1, float k) {
	return v1.y() - v1.x() * k;
}

Vector2f calcCenterVerticalLine(const Vector2f& v1, const Vector2f& v2) {
	Vector2f coef = solveLine(v1, v2);
	Vector2f center = (v1 + v2) * 0.5f;
	float k = -1 / coef[1];
	float c = solveLineC(center, k);
	return Vector2f(k, c);
}

Vector2f intersection(const Vector2f& line1, const Vector2f& line2) {
	Matrix2f A;
	Vector2f b;
	A << -line1[0], 1,
		 -line2[0], 1;
	b << line1[1], line2[1];
	return A.inverse() * b;
}

std::vector<Vector2f> GeneratePs(size_t count, const Vector2i& scopeX, const Vector2i& scopeY){
	std::vector<Vector2f> result;
	result.reserve(count);

	std::random_device rd;  // Generate seek;
	std::mt19937 device(rd());  // Pseudorandom generator.
	std::uniform_int_distribution<> uidx(scopeX[0], scopeX[1]);  // 将伪随机生成器返回的数转化为均匀分布
	std::uniform_int_distribution<> uidy(scopeY[0], scopeY[1]);  // 将伪随机生成器返回的数转化为均匀分布

	for (int i = 0; i < count; ++i) {
		result.push_back(Vector2f(uidx(device), uidy(device)));
	}

	return result;
}

float RightOrLeft(Vector2f es, Vector2f ee, const Vector2f& p) {
	Vector2f e = ee - es;
	Vector2f v = p - es;
	return Vector3f(e.x(), e.y(), 0).cross(Vector3f(v.x(), v.y(), 0)).z();
}

float CalcTriangleArea(const Vector2f& v1, const Vector2f& v2, const Vector2f& v3) {
	Vector2f e1 = v1 - v2;
	Vector2f e2 = v1 - v3;
	return 	Vector3f(e1.x(), e1.y(), 0.0).cross(Vector3f(e2.x(), e2.y(), 0.0)).norm() * 0.5;
}

Vector2f CalcMassOfTriangle(const Vector2f& v1, const Vector2f& v2, const Vector2f& v3) {
	Vector2f centerP1 = (v1 + v2) * 0.5f;
	Vector2f centerP2 = (v1 + v3) * 0.5f;
	Vector2f line01 = solveLine(centerP1, v3);
	Vector2f line02 = solveLine(centerP2, v2);
	//TestCenterLine(line01, centerP1, v3);
	//TestCenterLine(line02, centerP2, v2);
	Vector2f hitP = intersection(line01, line02);
	//assert(hitP.x() >= 0 && hitP.x() <= column - 1);
	//assert(hitP.y() >= 0 && hitP.y() <= row - 1);
	return hitP;
}

// Assist point should in inner shape.
// 不规则图形重心求法,将图形分割为若干个简单图形,分别求出它们的重心, 乘以基础图形面积再除以总面积(加权平均)
Vector2f CalcMassOfIrregularPolygon(const std::vector<size_t>& ixs, const std::vector<Vector2f>& ps, const Vector2f& assistP) {

	float denominator = 0;
	Vector2f numerator(0, 0);

	for (int i = 0; i < ixs.size(); ++i) {
		int ixNext = (i + 1) % ixs.size();
		const Vector2f& v1 = ps[ixs[i]];
		const Vector2f& v2 = ps[ixs[ixNext]];

		const Vector2f& massCenterP = CalcMassOfTriangle(v1, v2, assistP);
		float area = CalcTriangleArea(v1, v2, assistP);

		numerator += massCenterP * area;
		denominator += area;
	}

	return numerator / denominator;
}

