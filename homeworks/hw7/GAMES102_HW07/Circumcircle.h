#pragma once

#include "Eigen/Core"


#define M_EPSILON 0.0001

using namespace Eigen;

class Triangle;
class Circumcircle
{
public:
	Circumcircle();
	Circumcircle(Triangle* tri);
	void SetRadius(float radius);
	void SetCenter(Vector2f&& center);
	float GetRadius() const { return radius; }
	Vector2f GetCenter() const { return center; }
	bool IfVertexInCircle(const Vector2f& v);

private:
	Vector2f FindCenter(const Vector2f& v1, const Vector2f& v2, const Vector2f& v3) const;
	float CalcRadius(const Vector2f& v, const Vector2f& center) const;
	float diffDistance(const Vector2f& v, const Vector2f& other) const;

	float radius;
	Vector2f center;
};

