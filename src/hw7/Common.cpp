#include "Common.h"
#include <assert.h>

Vector4f CalcPlaneExpression(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3) {
	Vector3f e1 = p2 - p1;
	Vector3f e2 = p3 - p1;
	assert(!ClampOrNot(e1.dot(e2), 1, 1, 1e-5) && !ClampOrNot(e1.dot(e2), -1, -1, 1e-5));
	Vector3f n = (e1.cross(e2)).normalized();
	assert(ClampOrNot(n.norm(), 1, 1, 1e-5));
	float A = n.x();
	float B = n.y();
	float C = n.z();
	float D = -(A * p1.x() + B * p1.y() + C * p1.z());
	return Vector4f(A, B, C, D);
}

Vector3f toVector3f(const Vector4f& v) {
	Vector4f tmp = v / v.w();
	return Vector3f(tmp.x(), tmp.y(), tmp.z());
}

Vector4f toVector4f(const Vector3f& v) {
	return Vector4f(v.x(), v.y(), v.z(), 1);
}

Vector3f castVector3f(const Ubpa::pointf3& p)
{
	return Vector3f(p[0], p[1], p[2]);
}

Vector3f castVector3f(const Ubpa::vecf3& v)
{
	return Vector3f(v[0], v[1], v[2]);
}

Ubpa::pointf3 castPointf3(const Vector3f& v)
{
	return Ubpa::pointf3(v[0], v[1], v[2]);
}

bool ClampOrNot(float val, float minVal, float maxVal, float epsilon = 1e-3) {
	return val > minVal - epsilon && val < maxVal + epsilon;
}