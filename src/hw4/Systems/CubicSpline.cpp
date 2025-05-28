#include "CubicSpline.h"

constexpr float CONTROL_POINT_RADIUS = 8;

void DebugTrigger(CanvasData* data) {
	if (data->enableDebug) {
		std::cout << "Please set break point here!" << std::endl;
	}
}
void AddDebugSwitch(CanvasData* data) {
	ImGui::Checkbox("enable debug", &data->enableDebug);
}
void ShowDebugInfo(CanvasData* data) {
	ImGui::Text(data->debugInfo.c_str());
}

enum ParamMode {
	Uniform = 1,
	Chordal, // Radian length
	Centripetal,  // Sqrt radian length
};

ImVec2 operator+ (const ImVec2& v1, const ImVec2& v2) {
	return ImVec2(v1.x + v2.x, v1.y + v2.y);
}

ImVec2 operator- (const ImVec2& v1, const ImVec2& v2) {
	return ImVec2(v1.x - v2.x, v1.y - v2.y);
}

ImVec2 operator* (const ImVec2& v, float mul) {
	return ImVec2(v.x * mul, v.y * mul);
}

float Dot(const ImVec2& v1, const ImVec2& v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

float Len(const ImVec2& v) {
	return std::sqrt(Dot(v, v));
}

ImVec2 Normalized(const ImVec2& v) {
	float invLength = 1. / Len(v);
	return ImVec2(v.x * invLength, v.y * invLength);
}

// Calc manually 2 order
float CalcMTail(float mc, float dn, float xc, float xn, float yc, float yn) {
	float hc = xn - xc;
	float invHc = 1. / hc;
	float mn = 3 * ((yc - yn) * invHc + dn) * invHc - mc * 0.5;
	return mn;
}

float CalcM(float mn, float dc, float xc, float xn, float yc, float yn) {
	float hc = xn - xc;
	float invHc = 1. / hc;
	float mc = 1.5 * ((yc - yn) * invHc + dc) * invHc + 0.25 * mn;
	return mc;
}

// Calc 1 order
// 使用二阶公式推到一阶. 求一条线的起点.
float CalcD(float mc, float mn, float xc, float xn, float yc, float yn) {
	float hc = xn - xc;
	float constant = 1. / 6;
	float invHc = 1. / hc;
	float dConstant = yn * invHc - mn * hc * constant;
	float cConstant = mc * hc * constant - yc * invHc;
	float d = 0.5 * mc * hc + dConstant + cConstant;
	return d;
}

float CalcDTail(float mc, float mn, float xc, float xn, float yc, float yn) {
	float hc = xn - xc;
	float constant = 1. / 6;
	float invHc = 1. / hc;
	float dConstant = yn * invHc - mn * hc * constant;
	float cConstant = mc * hc * constant - yc * invHc;
	float d = 0.5 * mn * hc + dConstant + cConstant;
	return d;
}

void ClearSelectPoint(CanvasData* data) {
	for (int i = 0; i < data->controls.size(); ++i) {
		data->controls[i].isSelect = false;
	}
}

int SelectPoint(const ImVec2& canvasPos, float searchRadius, const std::vector<float>& xs, const std::vector<float>& ys) {
	for (int i = 0; i < xs.size(); ++i) {
		if (std::abs(canvasPos.x - xs[i]) < searchRadius && std::abs(canvasPos.y - ys[i]) < searchRadius) {
			return i;
		}
	}return -1;
}

int SelectPoint(const ImVec2& canvasPos, float searchRadius, const std::vector<ControlPoint>& controls, bool& outIsP1) {
	for (int i = 0; i < controls.size(); ++i) {
		const ControlPoint control = controls[i];
		if (!control.isSelect) continue;

		if (std::abs(canvasPos.x - control.p1.x) < searchRadius && std::abs(canvasPos.y - control.p1.y) < searchRadius) {
			outIsP1 = true;
			return i;
		}
		else if (std::abs(canvasPos.x - control.p2.x) < searchRadius && std::abs(canvasPos.y - control.p2.y) < searchRadius) {
			outIsP1 = false;
			return i;
		}
	}return -1;
}

std::vector<float> Parameterization2(const std::vector<float>& xs, const std::vector<float>& ys, ParamMode mode, float tInterval) {
	int n = xs.size();
	std::vector<float> t(n);
	if (n < 1) return t;

	switch (mode)
	{
	case Uniform:
		t[0] = 0;
		for (int i = 1; i < n; ++i) {
			t[i] = t[i - 1] + tInterval;
		}
		break;
	case Chordal:
		t[0] = 0;
		for (int i = 1; i < n; ++i) {
			float xDiff = xs[i] - xs[i - 1];
			float yDiff = ys[i] - ys[i - 1];
			t[i] = t[i - 1] + std::sqrt(xDiff * xDiff + yDiff * yDiff);
		}
		break;
	case Centripetal:
		t[0] = 0;
		for (int i = 1; i < n; ++i) {
			float xDiff = xs[i] - xs[i - 1];
			float yDiff = ys[i] - ys[i - 1];
			t[i] = t[i - 1] + pow(xDiff * xDiff + yDiff * yDiff, 0.25);
		}
		break;
	}
	return t;
}

void Draw(const std::vector<ImVec2>& points, const ImVec2& canvasOrigin, const ImVec2& canvasSize, ImDrawList* drawList, const ImU32& color) {
	if (points.size() < 1) return;

	ImVec2 canvasDiagonal = canvasOrigin + canvasSize;
	ImVec2 prePoint = ImVec2(canvasOrigin.x + points[0].x, canvasDiagonal.y - points[0].y);
	for (int i = 1; i < points.size(); ++i) {
		ImVec2 curPoint = ImVec2(canvasOrigin.x + points[i].x, canvasDiagonal.y - points[i].y);
		drawList->AddLine(prePoint, curPoint, color, 2.0);
		prePoint = std::move(curPoint);
	}
}

void DrawPoint(CanvasData* data, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasDiagonal) {

	std::vector<float>& xs = data->xs;
	std::vector<float>& ys = data->ys;

	//ImGui::SetItemAllowOverlap();

	for (int i = 0; i < xs.size(); ++i) {
		drawList->AddCircleFilled(ImVec2(xs[i] + canvasOrigin[0], canvasDiagonal[1] - ys[i]), CONTROL_POINT_RADIUS, IM_COL32(255, 255, 0, 255));
		//ImGui::SetCursorScreenPos(ImVec2(xs[i] + canvasOrigin.x, canvasDiagonal[1] - ys[i]));
		//ImGui::ColorButton(std::to_string(i).c_str(), ImVec4(255, 255, 0, 255), 0, ImVec2(CONTROL_POINT_SIZE, CONTROL_POINT_SIZE));
	}
}

void ShowToolBar(int ix, ControlPoint& control, const ImVec2& canvasOrigin) {
	ImGui::SetNextWindowPos(canvasOrigin);
	if (ImGui::Begin(("Control" + std::to_string(ix)).c_str())) {
		ImGui::PushItemWidth(60);
		const char* ContinuousMode[] = { "C0", "G1", "C1", "C2" };
		ImGui::Combo("ContinuousMode", (int*)(&control.mode), ContinuousMode, IM_ARRAYSIZE(ContinuousMode));
		ImGui::Text(std::to_string((int)control.mode).c_str());
	}
	ImGui::End();
}

void DrawOneGizmo(const ControlPoint& control, float x, float y, float derivativeMul, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasDiagonal) {
	// draw derivative line
	ImVec2 p(x + canvasOrigin.x, canvasDiagonal.y - y);
	ImVec2 p1(control.p1.x + canvasOrigin.x, canvasDiagonal.y - control.p1.y);
	ImVec2 p2(control.p2.x + canvasOrigin.x, canvasDiagonal.y - control.p2.y);
	drawList->AddLine(p, p1, IM_COL32(255, 255, 255, 255), 2);
	drawList->AddLine(p, p2, IM_COL32(255, 255, 255, 255), 2);
	// draw derivative control point
	ImVec2 halfDiagonal(CONTROL_POINT_RADIUS , CONTROL_POINT_RADIUS);
	drawList->AddRectFilled(p1 - halfDiagonal, p1 + halfDiagonal, IM_COL32(255, 0, 0, 255));
	drawList->AddRectFilled(p2 - halfDiagonal, p2 + halfDiagonal, IM_COL32(255, 0, 0, 255));
}

void DrawSelectGizmo(CanvasData* data, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasDiagonal) {
	std::vector<ControlPoint>& controls = data->controls;
	const std::vector<float>& xs = data->xs;
	const std::vector<float>& ys = data->ys;

	for (int i = 0; i < controls.size(); ++i) {
		if (controls[i].isSelect) {
			DrawOneGizmo(controls[i], xs[i], ys[i], data->derivativeMul, drawList, canvasOrigin, canvasDiagonal);
			ShowToolBar(i, controls[i], canvasOrigin);
		}
	}
}

void OneCubicSpline(std::vector<ImVec2>& fitPoints, float mc, float mn, float xc, float xn, float yc, float yn, float delta) {
	float constant = 1. / 6;
	float hc = xn - xc;
	float invHc = 1 / hc;
	float coefMC = mc * constant * invHc;
	float coefMN = mn * constant * invHc;
	float DIntegrateCoef = yn * invHc - mn * hc * constant;
	float CIntegrateCoef = yc * invHc - mc * hc * constant;

	float y = 0;
	float x = xc;

	int size = ceil((xn - xc) / delta);
	int startIx = fitPoints.size();
	fitPoints.resize(size + fitPoints.size());

	for (int i = startIx; i < fitPoints.size(); ++i) {
		x = std::min(xn, x);
		y = coefMC * pow(xn - x, 3) +
			coefMN * pow(x - xc, 3) +
			(x - xc) * DIntegrateCoef +
			(xn - x) * CIntegrateCoef;
		fitPoints[i][0] = x;
		fitPoints[i][1] = y;
		x += delta;
	}
}

// 对角矩阵必须是严格的对角占优矩阵(保证行列式以及它的顺序主子式行列式非0)
// |a_i_i| > |a_i_i-1| + |a_i_i+1|
Eigen::MatrixXf ConstructDiagonalMatrix(const std::vector<float>& xs) {
	// n+1 points; n cubicSpline; n-1 equation group count.
	int n = xs.size() - 1;
	Eigen::MatrixXf m = Eigen::MatrixXf::Zero(n - 1, n - 1);

	// Init diagonal element
	for (int i = 0; i < n - 1; ++i) {
		int leftIx = i - 1;
		int majorIx = i;
		int rightIx = i + 1;

		// h_i = x_i+1 - x_i
		if (leftIx >= 0) {
			m(i, leftIx) = xs[i + 1] - xs[i];
		}
		if (rightIx < n - 1) {
			m(i, rightIx) = xs[i + 2] - xs[i + 1];
		}
		// u = 2 * (h_i-1 + h_i)
		m(i, majorIx) = 2 * (xs[i + 1] - xs[i] + xs[i + 2] - xs[i + 1]);
	}
	return m;
}

Eigen::VectorXf PursueSolver(Eigen::MatrixXf& diagonalMatrix, Eigen::VectorXf& values, float m0 = 0, float mn = 0) {
	assert(diagonalMatrix.rows() == diagonalMatrix.cols());
	assert(values.size() == diagonalMatrix.rows());

	int n = diagonalMatrix.rows() + 1;
	Eigen::VectorXf b(n + 1);
	// Translate to triangle matrix.
	for (int i = 1; i < n - 1; ++i) {
		float coef = diagonalMatrix(i, i - 1) / diagonalMatrix(i - 1, i - 1);
		diagonalMatrix(i, i) -= coef * diagonalMatrix(i - 1, i);
		values(i) -= coef * values(i - 1);
		diagonalMatrix(i, i - 1) = 0;
	}

	b(n - 1) = values(n - 2) / diagonalMatrix(n - 2, n - 2);
	for (int i = n - 3; i >= 0; --i) {
		b(i + 1) = (values(i) - b(i + 2) * diagonalMatrix(i, i + 1)) / diagonalMatrix(i, i);
	}

	b(0) = m0;
	b(n) = mn;
	return b;
}

Eigen::VectorXf CubicSplineM(const std::vector<float>& xs, const std::vector<float>& ys, float delta)
{
	assert(xs.size() == ys.size());
	int n = xs.size() - 1;
	Eigen::VectorXf values(n - 1);

	// Construct values
	for (int i = 1; i < n; ++i) {
		// n represent i+1, c represent i.
		float yn = ys[i + 1] - ys[i];
		float yc = ys[i] - ys[i - 1];
		float hn = xs[i + 1] - xs[i];
		float hc = xs[i] - xs[i - 1];
		values[i - 1] = 6 * (1 / hn * yn - 1 / hc * yc);
	}

	std::vector<ImVec2> fitPoints;

	Eigen::MatrixXf diagonalMatrix = std::move(ConstructDiagonalMatrix(xs));
	Eigen::VectorXf m = std::move(PursueSolver(diagonalMatrix, values));

	return m;
}

std::vector<ImVec2> CubicSplineFn(const std::vector<float>& xs, const std::vector<float>& ys, float delta)
{
	const Eigen::VectorXf& m = CubicSplineM(xs, ys, delta);
	// Here to control m.
	std::vector<ImVec2> fitPoints;

	for (int i = 0; i < m.size() - 1; ++i) {
		OneCubicSpline(fitPoints, m(i), m(i + 1), xs[i], xs[i + 1], ys[i], ys[i + 1], delta);
	}

	return fitPoints;
}

void CubicSplineCurve(CanvasData* data, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasSize) {
	const std::vector<float>& t = data->ts;
	Eigen::VectorXf xm = std::move(CubicSplineM(t, data->xs, data->tDelta));
	Eigen::VectorXf ym = std::move(CubicSplineM(t, data->ys, data->tDelta));

	std::vector<ControlPoint>& controls = data->controls;
	for (int i = 0; i < controls.size(); ++i) {
		ControlPoint& control = controls[i];
		switch (control.mode)
		{
		case C2:
			control.xmLeft = xm(i);
			control.xmRight = xm(i);
			control.ymLeft = ym(i);
			control.ymRight = ym(i);
			if (i == 0) {
				float xd = CalcD(xm(i), xm(i + 1), t[i], t[i + 1], data->xs[i], data->xs[i + 1]);
				control.xdLeft = xd;
				control.xdRight = xd;
				float yd = CalcD(ym(i), ym(i + 1), t[i], t[i + 1], data->ys[i], data->ys[i + 1]);
				control.ydLeft = yd;
				control.ydRight = yd;
			}
			else {
				float xd = CalcDTail(xm(i - 1), xm(i), t[i - 1], t[i], data->xs[i - 1], data->xs[i]);
				control.xdLeft = xd;
				control.xdRight = xd;
				float yd = CalcDTail(ym(i - 1), ym(i), t[i - 1], t[i], data->ys[i - 1], data->ys[i]);
				control.ydLeft = yd;
				control.ydRight = yd;
			}
			control.p1.x = data->xs[i] - control.xdLeft * data->derivativeMul;
			control.p1.y = data->ys[i] - control.ydLeft * data->derivativeMul;
			control.p2.x = data->xs[i] + control.xdRight * data->derivativeMul;
			control.p2.y = data->ys[i] + control.ydRight * data->derivativeMul;
			break;
		}
	}

	std::vector<ImVec2> xs;
	std::vector<ImVec2> ys;
	for (int i = 0; i < controls.size() - 1; ++i) {
		OneCubicSpline(xs, controls[i].xmRight, controls[i + 1].xmLeft, t[i], t[i + 1], data->xs[i], data->xs[i + 1], data->tDelta);
		OneCubicSpline(ys, controls[i].ymRight, controls[i + 1].ymLeft, t[i], t[i + 1], data->ys[i], data->ys[i + 1], data->tDelta);
	}

	std::vector<ImVec2> ps(xs.size());
	for (int i = 0; i < ps.size(); ++i) {
		ps[i][0] = xs[i][1];
		ps[i][1] = ys[i][1];
	}

	Draw(ps, canvasOrigin, canvasSize, drawList, IM_COL32(0, 255, 0, 255));
}

void CubicSpline::OnUpdate(UECS::Schedule& schedule)
{
	schedule.RegisterCommand([](UECS::World* w)->void {
		CanvasData* data = w->entityMngr.GetSingleton<CanvasData>();
		if (!data) return;

		ImGui::SetNextWindowSize(ImVec2(800, 500));
		ImGuiIO& io = ImGui::GetIO();

		if (ImGui::Begin("CubicSpline", 0, ImGuiWindowFlags_NoResize)) {
			ImGui::Checkbox("enableCubicSplineFn", &data->enableCubicSplineFn);
			ImGui::Checkbox("enableCurve", &data->enableCurve);
			ImGui::RadioButton("uniform", &data->paramMode, 1);
			ImGui::SameLine(0);
			ImGui::RadioButton("chordal", &data->paramMode, 2);
			ImGui::SameLine(0);
			ImGui::RadioButton("centripetal", &data->paramMode, 3);
			ImGui::PushItemWidth(80);
			ImGui::InputFloat("showDerivativeMul", &data->derivativeMul);

			ShowDebugInfo(data);
			//AddDebugSwitch(data);

			ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
			ImVec2 canvasSize = ImGui::GetContentRegionAvail();
			ImVec2 canvasDiagonal = canvasOrigin + canvasSize;
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// button overlay and multiple interaction. 
			// https://github.com/ocornut/imgui/issues/3909
			// https://github.com/ocornut/imgui/issues/6512

			//ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
			ImGui::InvisibleButton("canvas", canvasSize);
			bool isHover = ImGui::IsItemHovered();
			bool isActive = ImGui::IsItemActive();

			// move add point
			if (data->addingPoint) {
				float xPos = io.MousePos.x - canvasOrigin.x;
				float yPos = canvasDiagonal.y - io.MousePos.y;
				data->xs.back() = xPos;
				data->ys.back() = yPos;
			}
			// move derivative point
			if (data->moveDerivative != -1) {
				int ix = data->moveDerivative;
				ControlPoint& control = data->controls[ix];
				ImVec2& p = data->isDP1 ? control.p1 : control.p2;
				ImVec2 mouseInCanvasPos(io.MousePos.x - canvasOrigin.x, canvasDiagonal.y - io.MousePos.y);
				ImVec2 controlPoint(data->xs[ix], data->ys[ix]);
				ImVec2 v1 = p - controlPoint;
				ImVec2 v2 = mouseInCanvasPos - controlPoint;
				const std::vector<float>& xs = data->xs;
				const std::vector<float>& ys = data->ys;
				const std::vector<float>& ts = data->ts;

				if (std::abs(io.MouseDelta.x) > 0
					|| std::abs(io.MouseDelta.y) > 0) {
					control.hasMove = true;
				}

				if (control.hasMove) {
					// limit move mode
					switch (control.mode) {
					case C0:
					{
						p.x = mouseInCanvasPos.x;
						p.y = mouseInCanvasPos.y;
						ImVec2 v1New = p - controlPoint;
						if (data->isDP1) {
							control.xdLeft = v1New.x / data->derivativeMul;
							control.ydLeft = v1New.y / data->derivativeMul;
						}
						else {
							control.xdRight = v1New.x / data->derivativeMul;
							control.ydRight = v1New.y / data->derivativeMul;
						}
					}
					break;
					case G1:
					{
						float ratio = Dot(v1, v2) / Dot(v1, v1);
						ImVec2 toPoint = std::move(controlPoint + v1 * ratio);
						p.x = toPoint.x;
						p.y = toPoint.y;
						ImVec2 v1New = p - controlPoint;
						if (data->isDP1) {
							control.xdLeft = -v1New.x / data->derivativeMul;
							control.ydLeft = -v1New.y / data->derivativeMul;
							const ControlPoint& preControl = data->controls[ix - 1];
							control.xmLeft = -CalcMTail(preControl.xmRight, control.xdLeft, ts[ix], ts[ix - 1], xs[ix], xs[ix - 1]);
							control.ymLeft = -CalcMTail(preControl.ymRight, control.ydLeft, ts[ix], ts[ix - 1], ys[ix], ys[ix - 1]);
						}
						else {
							control.xdRight = v1New.x / data->derivativeMul;
							control.ydRight = v1New.y / data->derivativeMul;
							/*const ControlPoint& preControl = data->controls[ix - 1];
							control.xmRight = CalcMTail(preControl.xmRight, control.xdRight, ts[ix - 1], ts[ix], xs[ix - 1], xs[ix]);
							control.ymRight = CalcMTail(preControl.ymRight, control.ydRight, ts[ix - 1], ts[ix], ys[ix - 1], ys[ix]);*/
							const ControlPoint& nextControl = data->controls[ix + 1];
							control.xmRight = -CalcM(nextControl.xmLeft, control.xdRight, ts[ix], ts[ix + 1], xs[ix], xs[ix + 1]);
							control.ymRight = -CalcM(nextControl.ymLeft, control.ydRight, ts[ix], ts[ix + 1], ys[ix], ys[ix + 1]);
						}
					}
					break;
					case C1:
					{
						float targetLength = Dot(v1, v2) / std::sqrt(Dot(v1, v1));
						ImVec2& anotherP = data->isDP1 ? control.p2 : control.p1;
						ImVec2 v3 = anotherP - controlPoint;
						ImVec2 toPoint = std::move(controlPoint + v1 * (targetLength / Len(v1)));
						ImVec2 toAnotherPoint = std::move(controlPoint + v3 * (targetLength / Len(v3)));
						p.x = toPoint.x;
						p.y = toPoint.y;
						anotherP.x = toAnotherPoint.x;
						anotherP.y = toAnotherPoint.y;
					}
					break;
					}
				}
			}
			// move exist point
			if (data->movePoint != -1) {
				float xDelta = io.MouseDelta.x;
				float yDelta = io.MouseDelta.y;
				if (std::abs(xDelta) > 0 || std::abs(yDelta) > 0) {
					data->controls[data->movePoint].hasMove = true;
				}
				data->xs[data->movePoint] += xDelta;
				data->ys[data->movePoint] -= yDelta;
			}

			if (isHover && ImGui::IsMouseDown(ImGuiMouseButton_Left) 
				&& !data->addingPoint 
				&& data->movePoint == -1 
				&& data->moveDerivative == -1) 
			{
				float xPos = io.MousePos.x - canvasOrigin.x;
				float yPos = canvasDiagonal.y - io.MousePos.y;
				bool isP1 = false;
				int ix = SelectPoint(ImVec2(xPos, yPos), CONTROL_POINT_RADIUS, data->xs, data->ys);
				int ixDPoint = SelectPoint(ImVec2(xPos, yPos), CONTROL_POINT_RADIUS, data->controls, isP1);
				// add new point
				if (-1 == ix && -1 == ixDPoint) {
					data->xs.emplace_back(xPos);
					data->ys.emplace_back(yPos);
					data->controls.emplace_back(ControlPoint());
					data->addingPoint = true;
					ClearSelectPoint(data);
				}
				// want to move control derivative point
				else if (-1 != ixDPoint) {
					ContinuousMode mode = data->controls[ixDPoint].mode;
					switch (mode) {
					case C1:
					case G1:
					case C0:
						data->moveDerivative = ixDPoint;
						data->isDP1 = isP1;
					}
				}
				// select or move exist point
				else if (-1 != ix){
					// set target point
					data->movePoint = ix;
					data->gizmoShowState = data->controls[ix].isSelect;
					ClearSelectPoint(data);
				}
			}

			bool isMouseReleased = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
			if ((!isHover || isMouseReleased) && data->addingPoint) {
				data->addingPoint = false;
			}
			if ((!isHover || isMouseReleased) && data->movePoint != -1) {
				ControlPoint& control = data->controls[data->movePoint];
				if (!control.hasMove) {
					// if you no move. you select it.
					control.isSelect = !data->gizmoShowState;
				}
				control.hasMove = false;
				data->movePoint = -1;
			}
			if ((!isHover || isMouseReleased) && data->moveDerivative != -1) {
				data->moveDerivative = -1;
				data->isDP1 = false;
			}

			//if (isActive) {
			//	data->debugInfo = std::move(std::string("true"));
			//}
			//else {
			//	data->debugInfo = std::move(std::string("false"));
			//}

			// Content Item
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && io.MouseDelta.x == 0. && io.MouseDelta.y == 0.) {
				// Allow window to popup
				ImGui::OpenPopupContextItem("content");
			}

			// Draw popup.
			if (ImGui::BeginPopup("content")) {
				if (ImGui::MenuItem("RemoveAll", NULL, false, data->xs.size() > 0 && data->ys.size() > 0)) {
					data->xs.clear();
					data->ys.clear();
					data->controls.clear();
				}
				if (ImGui::MenuItem("RemoveOne", NULL, false, data->xs.size() > 0 && data->ys.size() > 0)) {
					data->xs.resize(data->xs.size() - 1);
					data->ys.resize(data->ys.size() - 1);
					data->controls.resize(data->controls.size() - 1);
				}
				ImGui::EndPopup();
			}

			DrawPoint(data, drawList, canvasOrigin, canvasDiagonal);

			drawList->PushClipRect(canvasOrigin, canvasDiagonal, true);
			if (data->enableCubicSplineFn && data->xs.size() >= 3) {
				Draw(CubicSplineFn(data->xs, data->ys, data->delta), canvasOrigin, canvasSize, drawList, IM_COL32(255, 0, 0, 255));
			}
			else if (data->enableCurve && data->controls.size() >= 3) {
				data->ts = Parameterization2(data->xs, data->ys, (ParamMode)data->paramMode, data->tInterval);
				CubicSplineCurve(data, drawList, canvasOrigin, canvasSize);
				DrawSelectGizmo(data, drawList, canvasOrigin, canvasDiagonal);
			}
			drawList->PopClipRect();
		}
		ImGui::End();
		});
}