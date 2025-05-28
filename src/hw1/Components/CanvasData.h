#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<float> xs;
	std::vector<float> ys;
	std::map<std::string, bool> switchs{
		{"enablePolynomialInterpolate", false},
		{"enableGaussInterpolate", false},
		{"enablePolynomialFit", false},
		{"enableRidgeFit", false},
		{"enableCurve", false},
		{"enableLine", true},
	};

	int fitBaseCount{ 4 };
	int paramMode{ 1 };

	float delta{ 1 };
	float sigma{ 10.0 };
	float lambda{ 0.2 };

	// Draw Curve(Homework 3)
	float tDelta{ 1 };
	float tInterval{ 30 };
};

#include "details/CanvasData_AutoRefl.inl"
