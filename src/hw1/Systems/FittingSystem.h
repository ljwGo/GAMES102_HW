#pragma once

#include <vector>
#include "UECS/World.h"
#include "../Components/CanvasData.h"
#include "Eigen/Dense"
#include <_deps/imgui/imgui.h>
#include <iostream>

enum ParamMode {
	Uniform = 1,
	Chordal, // Radian length
	Centripetal,  // Sqrt radian length
};

struct FittingSystem
{
	static void OnUpdate(Ubpa::UECS::Schedule& schedule);
};

