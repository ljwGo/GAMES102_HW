#include "DivisionSystem.h"

using namespace Ubpa;

ImVec2 operator+ (const ImVec2& v1, const ImVec2& v2) {
	return ImVec2(v1.x + v2.x, v1.y + v2.y);
}

pointf2 operator+ (const pointf2& v1, const pointf2& v2) {
	return pointf2(v1[0] + v2[0], v1[1] + v2[1]);
}

pointf2 operator- (const pointf2& v1, const pointf2& v2) {
	return pointf2(v1[0] - v2[0], v1[1] - v2[1]);
}

pointf2 operator* (const pointf2& v, const float f) {
	return pointf2(v[0] * f, v[1] * f);
}


// Fit division method

// Chaikin 2 order
std::vector<Ubpa::pointf2> DivisionOnce(const std::vector<Ubpa::pointf2>& vertices){
	int degree = vertices.size();

	int extendDegree = degree * 2;
	std::vector<Ubpa::pointf2> vs(extendDegree);
	
	for (int i = 0, j = 0; i < vs.size(); i+=2) {
		// close graphics. end point use the first point.
		if (j == vertices.size() - 1) {
			vs[i] = pointf2(0.75 * vertices[j][0] + 0.25 * vertices[0][0], 0.75 * vertices[j][1] + 0.25 * vertices[0][1]);
			vs[i + 1] = pointf2(0.25 * vertices[j][0] + 0.75 * vertices[0][0], 0.25 * vertices[j][1] + 0.75 * vertices[0][1]);
		}
		else {
			vs[i] = pointf2(0.75 * vertices[j][0] + 0.25 * vertices[j + 1][0], 0.75 * vertices[j][1] + 0.25 * vertices[j + 1][1]);
			vs[i + 1] = pointf2(0.25 * vertices[j][0] + 0.75 * vertices[j + 1][0], 0.25 * vertices[j][1] + 0.75 * vertices[j + 1][1]);
			j++;
		}
	}

	return vs;
}

// Chaikin 2 order
std::vector<Ubpa::pointf2> Division(const std::vector<Ubpa::pointf2>& vertices, int iterCount = 10) {
	
	std::vector<Ubpa::pointf2> v = vertices;
	
	for (int i = 0; i < iterCount; ++i) {
		v = DivisionOnce(v);
	}

	return v;
}

// Chaikin 3 order
std::vector<Ubpa::pointf2> DivisionOnce3(const std::vector<Ubpa::pointf2>& vertices) {
	int degree = vertices.size();

	int extendDegree = degree * 2;
	std::vector<Ubpa::pointf2> vs(extendDegree);

	for (int i = 0; i < vertices.size(); ++i) {
		// move self
		const pointf2& prePoint = i <= 0 ? vertices[vertices.size() - 1] : vertices[i - 1];
		const pointf2& nextPoint = i >= vertices.size() - 1 ? vertices[0] : vertices[i + 1];
		const pointf2& curPoint = vertices[i];

		vs[i * 2] = prePoint * 0.125 + curPoint * 0.75 + nextPoint * 0.125;
		// add new point
		vs[i * 2 + 1] = curPoint * 0.5 + nextPoint * 0.5;
	}

	return vs;
}

// Chaikin 3 order
std::vector<Ubpa::pointf2> Division3(const std::vector<Ubpa::pointf2>& vertices, int iterCount = 10) {

	std::vector<Ubpa::pointf2> v = vertices;

	for (int i = 0; i < iterCount; ++i) {
		v = DivisionOnce3(v);
	}

	return v;
}

// Interpolate
std::vector<Ubpa::pointf2> InterpolateOnce(const std::vector<Ubpa::pointf2>& vertices, float alpha) {
	int degree = vertices.size();

	int extendDegree = degree * 2;
	std::vector<Ubpa::pointf2> vs(extendDegree);

	for (int i = 0; i < vertices.size(); ++i) {
		const pointf2& firstP = vertices[i];
		int secondIx = (i + 1) % vertices.size();
		const pointf2& secondP = vertices[secondIx];
		int thirdIx = (secondIx + 1) % vertices.size();
		const pointf2& thirdP = vertices[thirdIx];
		int fourIx = (thirdIx + 1) % vertices.size();
		const pointf2& fourP = vertices[fourIx];

		const pointf2& p1 = firstP * 0.5 + fourP * 0.5;
		const pointf2& p2 = secondP * 0.5 + thirdP * 0.5;
		pointf2 newP = p2 + (p2 - p1) * alpha;

		vs[i * 2] = secondP;
		vs[i * 2 + 1] = newP;
	}

	return vs;
}

std::vector<Ubpa::pointf2> InterpolateFn(const std::vector<Ubpa::pointf2>& vertices, int iterCount = 10, float alpha = 0.125) {

	std::vector<Ubpa::pointf2> v = vertices;

	for (int i = 0; i < iterCount; ++i) {
		v = InterpolateOnce(v, alpha);
	}

	return v;
}

void Draw(const std::vector<pointf2>& points, const ImVec2& canvasOrigin, const ImVec2& canvasDiagonal, ImDrawList* drawList, const ImU32& color) {
	if (points.size() < 1) return;

	pointf2 prePoint = pointf2(canvasOrigin.x + points[0][0], canvasDiagonal.y - points[0][1]);
	for (int i = 1; i < points.size(); ++i) {
		pointf2 curPoint = pointf2(canvasOrigin.x + points[i][0], canvasDiagonal.y - points[i][1]);
		drawList->AddLine(prePoint, curPoint, color);
		prePoint = curPoint;
	}

	drawList->AddLine(pointf2(canvasOrigin.x + points[0][0], canvasDiagonal.y - points[0][1]),
		pointf2(canvasOrigin.x + points[points.size() - 1][0], canvasDiagonal.y - points[points.size() - 1][1]), color);
}

void DrawPoint(const std::vector<pointf2>& points, const ImVec2& canvasOrigin, const ImVec2& canvasDiagonal, ImDrawList* drawList, const ImU32& color) {
	for (int i = 0; i < points.size(); ++i) {
		pointf2 point(canvasOrigin.x + points[i][0], canvasDiagonal.y - points[i][1]);
		drawList->AddCircleFilled(point, 6, color);
	}
}

void DrawLine(const std::vector<pointf2>& points, const ImVec2& canvasOrigin, const ImVec2& canvasDiagonal, ImDrawList* drawList, const ImU32& color) {
	if (points.size() < 1) return;

	pointf2 prePoint = pointf2(canvasOrigin.x + points[0][0], canvasDiagonal.y - points[0][1]);
	for (int i = 1; i < points.size(); ++i) {
		pointf2 curPoint = pointf2(canvasOrigin.x + points[i][0], canvasDiagonal.y - points[i][1]);
		drawList->AddLine(prePoint, curPoint, color);
		prePoint = curPoint;
	}
	drawList->AddLine(pointf2(canvasOrigin.x + points[0][0], canvasDiagonal.y - points[0][1]),
		pointf2(canvasOrigin.x + points[points.size() - 1][0], canvasDiagonal.y - points[points.size() - 1][1]), color);
}

void DivisionSystem::OnUpdate(Ubpa::UECS::Schedule& schedule)
{
	schedule.RegisterCommand([](Ubpa::UECS::World* w){
		auto data = w->entityMngr.GetSingleton<CanvasData>();
		if (!data) return;

		ImGui::SetNextWindowSize(ImVec2(800, 600));

		if (ImGui::Begin("Disvision")) {

			int* choice = &(data->choice);
			float& alpha = data->alpha;
			std::vector<pointf2>& points = data->points;

			ImGui::RadioButton("Chaikin 2 Order", choice, (int)Chaikin2);
			ImGui::RadioButton("Chaikin 3 Order", choice, (int)Chaikin3);
			ImGui::RadioButton("Interpolate", choice, (int)Interpolate);
			ImGui::InputInt("DivisionCount", &(data->divisionCount));

			if (*choice == (int)Interpolate) {
				ImGui::InputFloat("alpha", &alpha);
			}

			const ImVec2& canvasOrigin = ImGui::GetCursorScreenPos();
			const ImVec2& canvasSize = ImGui::GetContentRegionAvail();
			const ImVec2& canvasDiagonal = canvasOrigin + canvasSize;
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImGuiIO& io = ImGui::GetIO();

			ImGui::InvisibleButton("canvas", canvasSize);
			bool isHover = ImGui::IsItemHovered();
			bool isActive = ImGui::IsItemActive();

			if (isHover && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				float xInCanvas = io.MousePos.x - canvasOrigin.x;
				float yInCanvas = canvasDiagonal.y - io.MousePos.y;

				points.emplace_back(pointf2(xInCanvas, yInCanvas));
			}

			// Content Item
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && io.MouseDelta.x == 0. && io.MouseDelta.y == 0.) {
				// Allow window to popup
				ImGui::OpenPopupContextItem("content");
			}

			// Draw popup.
			if (ImGui::BeginPopup("content")) {
				if (ImGui::MenuItem("RemoveAll", NULL, false, points.size() > 0)) {
					points.clear();
				}
				if (ImGui::MenuItem("RemoveOne", NULL, false, points.size() > 0)) {
					points.resize(points.size() - 1);
				}
				ImGui::EndPopup();
			}

			DrawPoint(points, canvasOrigin, canvasDiagonal, drawList, IM_COL32(0, 255, 0, 255));
			DrawLine(points, canvasOrigin, canvasDiagonal, drawList, IM_COL32(0, 255, 0, 255));

			if (points.size() > 2 && *choice == (int)Chaikin2) {
				Draw(Division(points, data->divisionCount), canvasOrigin, canvasDiagonal,drawList, IM_COL32(255, 0, 0, 255));
			}
			else if (points.size() > 2 && *choice == (int)Chaikin3) {
				Draw(Division3(points, data->divisionCount), canvasOrigin, canvasDiagonal, drawList, IM_COL32(255, 0, 0, 255));
			}
			else if (points.size() > 2 && *choice == (int)Interpolate) {
				Draw(InterpolateFn(points, data->divisionCount, data->alpha), canvasOrigin, canvasDiagonal, drawList, IM_COL32(255, 0, 0, 255));
			}
		}
		ImGui::End();

		return;
	});
}
