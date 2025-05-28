#pragma once

#include <UECS/World.h>

#include "../_deps/imgui/imgui.h"
#include "../Components/CanvasData.h"
#include <vector>

std::vector<Ubpa::pointf2> DivisionOnce(const std::vector<Ubpa::pointf2>& vertices);

struct DivisionSystem {
	static void OnUpdate(Ubpa::UECS::Schedule& schedule);
};
