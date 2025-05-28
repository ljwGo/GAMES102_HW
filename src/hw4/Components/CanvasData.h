#pragma once

#include <UGM/UGM.h>
#include <../_deps/imgui/imgui.h>

enum ContinuousMode {
	C0 = 0,
	G1,
	C1,
	C2,
};

struct ControlPoint {
	ContinuousMode mode = C2;
	bool isSelect = false;
	bool hasMove = false;
	// 2 order derivative
	float xmLeft;
	float xmRight;
	float ymLeft;
	float ymRight;
	// 1 order derivative
	float xdLeft;
	float xdRight;
	float ydLeft;
	float ydRight;
	ImVec2 p1;
	ImVec2 p2;
};

struct CanvasData {
	std::vector<float> xs;
	std::vector<float> ys;
	std::vector<float> ts;
	std::vector<ControlPoint> controls;
	bool enableCubicSplineFn{ false };
	bool enableCurve{ false };
	bool addingPoint{ false };
	bool isDP1 = false;

	int movePoint{ -1 };
	int moveDerivative{ -1 };
	int paramMode{ 0 };

	float delta{ 1. };
	float tDelta{ 1. };
	float tInterval{ 30. };
	float derivativeMul{ 8. };

	// Persistent date use for program logic
	bool gizmoShowState{ false };
	// Use for debug
	bool enableDebug{ false };
	std::string debugInfo;
};

#include "details/CanvasData_AutoRefl.inl"
