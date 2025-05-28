#pragma once

#include <UGM/UGM.h>

enum DivisionType
{
	Unselect,
	Chaikin2,
	Chaikin3,
	Interpolate,
};

struct CanvasData {
	int choice = (int)Unselect;
	float alpha = 0.125;
	std::vector<Ubpa::pointf2> points;
	int divisionCount = 1;
};

#include "details/CanvasData_AutoRefl.inl"
