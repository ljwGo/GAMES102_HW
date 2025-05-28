#include "Circumcircle.h"
#include "Triangle.hpp"

Circumcircle::Circumcircle()
{
	radius = 0;
	center = Vector2f::Zero();
}

Circumcircle::Circumcircle(Triangle* tri)
{
	center = FindCenter(tri->v1(), tri->v2(), tri->v3());
	radius = CalcRadius(tri->v1(), center);
}

void Circumcircle::SetRadius(float radius)
{
	this->radius = radius;
}

void Circumcircle::SetCenter(Vector2f&& center)
{
	this->center = std::move(center);
}

bool Circumcircle::IfVertexInCircle(const Vector2f& v)
{
	const Vector2f& tmp = v - center;
	bool result = tmp.dot(tmp) < radius * radius;
	return result;
}

Vector2f Circumcircle::FindCenter(const Vector2f& v1, const Vector2f& v2, const Vector2f& v3) const
{
	Vector2f center;
	Vector2f b;
	Matrix2f A;

	//// center away from per vertex have same distance.
	////(Pay attention to Eigen use column major matrix)
	//b << diffDistance(v1, v2), diffDistance(v2, v3);
	//// 录入时按行为主, 内存存储以列为主
	//A << (2 * (v1[0] - v2[0])), (2 * (v1[1] - v2[1])),
	//	(2 * (v2[0] - v3[0])), (2 * (v2[1] - v3[1]));
	////center = A.inverse() * b;

	float A1 = 2 * (v2.x() - v1.x());
	float B1 = 2 * (v2.y() - v1.y());
	float C1 = v2.dot(v2) - v1.dot(v1);
	float A2 = 2 * (v3.x() - v2.x());
	float B2 = 2 * (v3.y() - v2.y());
	float C2 = v3.dot(v3) - v2.dot(v2);

	float x = ((C1 * B2) - (C2 * B1)) / ((A1 * B2) - (A2 * B1));
	float y = ((A1 * C2) - (A2 * C1)) / ((A1 * B2) - (A2 * B1));
	return Vector2f(x, y);
}

float Circumcircle::CalcRadius(const Vector2f& v, const Vector2f& center) const
{
	Vector2f radiusV = v - center;
	return sqrtf(radiusV.dot(radiusV));
}

float Circumcircle::diffDistance(const Vector2f& v, const Vector2f& other) const
{
	return v.dot(v) - other.dot(other);
}
