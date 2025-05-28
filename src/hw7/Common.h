#pragma once

#include "Eigen/Dense"
#include <Eigen/Dense>
#include <UGM/point.h>

using namespace Eigen;

Vector4f CalcPlaneExpression(const Vector3f& p1, const Vector3f& p2, const Vector3f& p3);

Vector3f toVector3f(const Vector4f& v);

Vector4f toVector4f(const Vector3f& v);

Vector3f castVector3f(const Ubpa::pointf3& p);

Vector3f castVector3f(const Ubpa::vecf3& v);

Ubpa::pointf3 castPointf3(const Vector3f& v);

bool ClampOrNot(float val, float minVal, float maxVal, float epsilon);
