#pragma once

#include "UECS/World.h"
#include "../_deps/imgui/imgui.h"
#include "../Components/CanvasData.h"
#include "Eigen/Core"
#include <vector>

using namespace Ubpa;

struct CubicSpline {
	static void OnUpdate(UECS::Schedule& schedule);
};