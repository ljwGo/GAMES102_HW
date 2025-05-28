#include "FittingSystem.h"

using namespace Ubpa;

Eigen::MatrixXf operator+ (const Eigen::MatrixXf& matrix, float v) {
	Eigen::MatrixXf m(matrix);
	for (int i = 0; i < matrix.rows(); ++i) {
		for (int j = 0; j < matrix.cols(); ++j) {
			m(i, j) += v;
		}
	}
	return m;
}

ImVec2 operator+ (const ImVec2& v1, const ImVec2& v2) {
	return ImVec2(v1.x + v2.x, v1.y + v2.y);
}

ImVec2 operator- (const ImVec2& v1, const ImVec2& v2) {
	return ImVec2(v1.x - v2.x, v1.y - v2.y);
}

float Len(const ImVec2& v2, const ImVec2& v1) {
	const ImVec2& v = v2 - v1;
	return std::sqrt(v.x * v.x + v.y * v.y);
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

float PolynomialSolver(float x, const Eigen::VectorXf coeffs) {
	int maxPow = coeffs.rows();
	float y = 0;
	for (int i = 0; i < maxPow; ++i) {
		y += coeffs(i) * pow(x, i);
	}
	return y;
}

// PI is PolynomialInterpolate abbreviation
Eigen::VectorXf PIWeight(const std::vector<float>& xs, const std::vector<float>& ys) {
	assert(xs.size() == ys.size());
	int n = xs.size();

	Eigen::MatrixXf m(n, n);
	Eigen::VectorXf y(n);

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			m(i, j) = pow(xs[i], j);
		}
		y(i) = ys[i];
	}

	return m.inverse() * y;
}

// PI is PolynomialInterpolate abbreviation
std::vector<ImVec2> PIPredict(float left, float right, float delta, const std::vector<float>& xs, const std::vector<float>& ys)
{
	const Eigen::VectorXf& w = PIWeight(xs, ys);
	int size = ceil((right - left) / delta);
	std::vector<ImVec2> fitPoints(size);

	float x = left;
	for (int i = 0; i < fitPoints.size(); ++i) {
		x = std::min(x, right);
		fitPoints[i][0] = x;
		fitPoints[i][1] = PolynomialSolver(x, w);
		x += delta;
	}

	return fitPoints;
}

// PI is PolynomialInterpolate abbreviation
void _PI(CanvasData* data, const ImVec2& canvasOrigin, const ImVec2& canvasSize, ImDrawList* drawList) {
	const std::vector<ImVec2>& ps = PIPredict(canvasOrigin.x, canvasOrigin.x + canvasSize.x, data->delta, data->xs, data->ys);
	Draw(ps, canvasOrigin, canvasSize, drawList, IM_COL32(255, 0, 0, 255));
}

float Gauss(float x, float u = 0, float invSigma = 1) {
	return exp(-0.5 * pow((x - u) * invSigma, 2));
}

float GaussSolver(const std::vector<float>& xs, float x, float sigma, const Eigen::VectorXf& coeffs)
{
	float invSigma = 1. / sigma;
	float y = 0;

	y += coeffs(0);
	for (int i = 0; i < xs.size(); ++i) {
		y += coeffs(i + 1) * Gauss(x, xs[i], invSigma);
	}
	return y;
}

// Here sigma is a manual constant.
Eigen::VectorXf GaussWeight(const std::vector<float>& xs, const std::vector<float>& ys, const std::vector<float>& us, float sigma)
{
	int n = us.size();
	// Need add a constant
	Eigen::MatrixXf m(n + 1, n + 1);
	Eigen::VectorXf y(n + 1);

	float invSigma = 1.0 / sigma;

	for (int i = 0; i < n; ++i) {
		m(i, 0) = 1;
		for (int j = 0; j < n; ++j) {
			m(i, j + 1) = Gauss(xs[i], us[j], invSigma);
		}
		y(i) = ys[i];
	}

	// Add constraint(end two points center)
	m(n, 0) = 1;
	float centerX = (xs[n - 2] + xs[n - 1]) * 0.5f;
	float centerY = (ys[n - 2] + ys[n - 1]) * 0.5f;

	for (int i = 0; i < n; ++i) {
		m(n, i + 1) = Gauss(centerX, us[i], invSigma);
	}
	y(n) = centerY;

	return m.inverse() * y;
}

// GI is GaussInterpolate abbreviation
std::vector<ImVec2> GIPredict(float left, float right, float delta, const std::vector<float>& xs, const std::vector<float>& ys, const std::vector<float>& us, float sigma)
{
	const Eigen::VectorXf& w = GaussWeight(xs, ys, us, sigma);
	int size = ceil((right - left) / delta);
	std::vector<ImVec2> fitPoints(size);

	float x = left;
	for (int i = 0; i < fitPoints.size(); ++i) {
		x = std::min(x, right);
		fitPoints[i][0] = x;
		fitPoints[i][1] = GaussSolver(xs, x, sigma, w);
		x += delta;
	}

	return fitPoints;
}

void GI(CanvasData* data, const ImVec2& canvasOrigin, const ImVec2& canvasSize, ImDrawList* drawList, float sigma) {
	const std::vector<ImVec2>& ps = GIPredict(canvasOrigin.x, canvasOrigin.x + canvasSize.x, data->delta, data->xs, data->ys, data->xs, data->sigma);
	Draw(ps, canvasOrigin, canvasSize, drawList, IM_COL32(0, 255, 0, 255));
}

// PF is PolynomialFit abbreviation
Eigen::VectorXf PFWeight(const std::vector<float>& xs, const std::vector<float>& ys, int fitBaseCount) {
	int n = xs.size();
	int equationCount = xs.size();

	if (fitBaseCount >= 2 && fitBaseCount <= n) {
		n = fitBaseCount;
	}

	Eigen::MatrixXf m(equationCount, n);
	Eigen::VectorXf y(equationCount);

	for (int i = 0; i < equationCount; ++i) {
		for (int j = 0; j < n; j++)
		{
			m(i, j) = pow(xs[i], j);
		}
		y(i) = ys[i];
	}

	return (m.transpose() * m).inverse() * m.transpose() * y;
}

std::vector<ImVec2> PFPredict(float left, float right, float delta, const std::vector<float>& xs, const std::vector<float>& ys, int fitBaseCount = 4) {
	Eigen::VectorXf w = std::move(PFWeight(xs, ys, fitBaseCount));
	int size = ceil((right - left) / delta);
	std::vector<ImVec2> fitPoints(size);

	float x = left;
	for (int i = 0; i < fitPoints.size(); ++i) {
		x = std::min(x, right);
		fitPoints[i][0] = x;
		fitPoints[i][1] = PolynomialSolver(x, w);
		x += delta;
	}

	return fitPoints;
}

void PF(CanvasData* data, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasSize) {
	const std::vector<ImVec2>& ps = PFPredict(canvasOrigin.x, canvasOrigin.x + canvasSize.x, data->delta, data->xs, data->ys, data->fitBaseCount);
	Draw(ps, canvasOrigin, canvasSize, drawList, IM_COL32(0, 0, 255, 255));
}

// FR is FittingRidge abbreviation
Eigen::VectorXf FRWeight(const std::vector<float>& xs, const std::vector<float>& ys, int fitBaseCount, float lambda)
{
	int n = xs.size();
	int equationCount = xs.size();
	if (2 <= fitBaseCount && fitBaseCount <= n) {
		n = fitBaseCount;
	}

	Eigen::MatrixXf m(equationCount, n);
	Eigen::VectorXf y(equationCount);

	for (int i = 0; i < equationCount; ++i) {
		for (int j = 0; j < n; j++)
		{
			m(i, j) = pow(xs[i], j);
		}
		y(i) = ys[i];
	}

	return ((m.transpose() * m) + lambda).inverse() * m.transpose() * y;
}

std::vector<ImVec2> FRPredict(float left, float right, float delta, const std::vector<float>& xs, const std::vector<float>& ys, int fitBaseCount = 4, float lambda = 0.1) {
	const Eigen::VectorXf& w = FRWeight(xs, ys, fitBaseCount, lambda);
	int size = ceil((right - left) / delta);
	std::vector<ImVec2> fitPoints(size);

	float x = left;
	for (int i = 0; i < fitPoints.size(); ++i) {
		x = std::min(x, right);
		fitPoints[i][0] = x;
		fitPoints[i][1] = PolynomialSolver(x, w);
		x += delta;
	}

	return fitPoints;
}

void FR(CanvasData* data, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasSize)
{
	const std::vector<ImVec2>& ps = FRPredict(canvasOrigin.x, canvasOrigin.x + canvasSize.x, data->delta, data->xs, data->ys, data->fitBaseCount, data->lambda);
	Draw(ps, canvasOrigin, canvasSize, drawList, IM_COL32(200, 180, 255, 255));
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

void CurveFit(CanvasData* data, ImDrawList* drawList, const ImVec2& canvasOrigin, const ImVec2& canvasSize) {
	const std::vector<float>& ts = Parameterization2(data->xs, data->ys, (ParamMode)(data->paramMode), data->tInterval);
	if (data->switchs["enablePolynomialInterpolate"]) {
		const std::vector<ImVec2>& xs = PIPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->xs);
		const std::vector<ImVec2>& ys = PIPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->ys);
		assert(xs.size() == ys.size());

		std::vector<ImVec2> points(xs.size());
		for (int i = 0; i < xs.size(); ++i) {
			points[i][0] = xs[i][1];
			points[i][1] = ys[i][1];
		}

		Draw(points, canvasOrigin, canvasSize, drawList, IM_COL32(255, 0, 0, 255));
	}

	if (data->switchs["enableGaussInterpolate"]) {
		const std::vector<ImVec2>& xs = GIPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->xs, ts, data->sigma);
		const std::vector<ImVec2>& ys = GIPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->ys, ts, data->sigma);
		assert(xs.size() == ys.size());

		std::vector<ImVec2> points(xs.size());
		for (int i = 0; i < xs.size(); ++i) {
			points[i][0] = xs[i][1];
			points[i][1] = ys[i][1];
		}

		Draw(points, canvasOrigin, canvasSize, drawList, IM_COL32(0, 255, 0, 255));
	}

	if (data->switchs["enablePolynomialFit"]) {
		const std::vector<ImVec2>& xs = PFPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->xs, data->fitBaseCount);
		const std::vector<ImVec2>& ys = PFPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->ys, data->fitBaseCount);
		assert(xs.size() == ys.size());

		std::vector<ImVec2> points(xs.size());
		for (int i = 0; i < xs.size(); ++i) {
			points[i][0] = xs[i][1];
			points[i][1] = ys[i][1];
		}

		Draw(points, canvasOrigin, canvasSize, drawList, IM_COL32(0, 0, 255, 255));
	}

	if (data->switchs["enableRidgeFit"]) {
		const std::vector<ImVec2>& xs = FRPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->xs, data->fitBaseCount, data->lambda);
		const std::vector<ImVec2>& ys = FRPredict(ts[0], ts[ts.size() - 1], data->tDelta, ts, data->ys, data->fitBaseCount, data->lambda);
		assert(xs.size() == ys.size());

		std::vector<ImVec2> points(xs.size());
		for (int i = 0; i < xs.size(); ++i) {
			points[i][0] = xs[i][1];
			points[i][1] = ys[i][1];
		}

		Draw(points, canvasOrigin, canvasSize, drawList, IM_COL32(200, 180, 255, 255));
	}
}

void DrawLine(CanvasData* data, ImU32 color, const ImVec2& canvasOrigin, const ImVec2& canvasSize, ImDrawList* drawList)
{
	if (data->xs.size() < 2 || data->ys.size() < 2) return;

	ImVec2 canvasDiagonal = canvasOrigin + canvasSize;
	// (Deprecate)
	//pointf2 prePoint = std::move(DomainDefinition2ScreenPos(data, points[0], canvasOrigin, canvasSize));
	ImVec2 prePoint(data->xs[0] + canvasOrigin.x, canvasDiagonal.y - data->ys[0]);
	for (int i = 1; i < data->xs.size(); ++i) {
		// (Deprecate)
		//pointf2 curPoint = std::move(DomainDefinition2ScreenPos(data, points[i], canvasOrigin, canvasSize));
		ImVec2 curPoint(data->xs[i] + canvasOrigin.x, canvasDiagonal.y - data->ys[i]);
		drawList->AddLine(prePoint, curPoint, color, 2.0f);
		prePoint = std::move(curPoint);
	}
}

void DrawCircle(CanvasData* data, ImU32 color, const ImVec2& canvasOrigin, const ImVec2& canvasSize, ImDrawList* drawList) {
	ImVec2 canvasDiagonal = canvasOrigin + canvasSize;

	for (int i = 0; i < data->xs.size(); ++i) {
		drawList->AddCircleFilled(ImVec2(data->xs[i] + canvasOrigin.x, canvasDiagonal.y - data->ys[i]), 5.0f, color);
	}
}

void FittingSystem::OnUpdate(UECS::Schedule& schedule)
{
	schedule.RegisterCommand([](UECS::World* w)->void {
		CanvasData* data = w->entityMngr.GetSingleton<CanvasData>();
		if (!data) return;

		if (ImGui::Begin("Fitting Canvas")) {
			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			// (Deprecated) Set config
			// Use domain map will cause precision problem.
			// Set item width. https://github.com/ocornut/imgui/issues/385

			ImGui::PushItemWidth(100);
			ImGui::Checkbox("enablePolynomialInterpolate", &data->switchs["enablePolynomialInterpolate"]);
			ImGui::SameLine(0);
			ImGui::Checkbox("enableGaussInterpolate", &data->switchs["enableGaussInterpolate"]);
			ImGui::SameLine(0);
			ImGui::Checkbox("enablePolynomialFit", &data->switchs["enablePolynomialFit"]);
			ImGui::SameLine(0);
			ImGui::Checkbox("enableRidgeFit", &data->switchs["enableRidgeFit"]);
			ImGui::Checkbox("enableLine", &data->switchs["enableLine"]);
			ImGui::Checkbox("enableCurve", &data->switchs["enableCurve"]);

			ImGui::PushItemWidth(200);
			ImGui::SliderFloat("sigma", &data->sigma, 1, 100, "%.2f");
			ImGui::SameLine(0);
			ImGui::SliderInt("fitBaseCount", &data->fitBaseCount, 2, 10);
			ImGui::SameLine(0);
			ImGui::SliderFloat("lambda", &data->lambda, 0.001, 0.4, "%.2f");

			ImGui::PushItemWidth(60);
			ImGui::InputFloat("delta", &data->delta);
			ImGui::SameLine(0);
			ImGui::InputFloat("tDelta", &data->tDelta);
			ImGui::SameLine(0);
			ImGui::InputFloat("tInterval", &data->tInterval);

			ImGui::RadioButton("uniform", &data->paramMode, 1);
			ImGui::SameLine(0);
			ImGui::RadioButton("chordal", &data->paramMode, 2);
			ImGui::SameLine(0);
			ImGui::RadioButton("centripetal", &data->paramMode, 3);

			ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
			ImVec2 canvasSize = ImGui::GetContentRegionAvail();
			ImVec2 canvasDiagonal = ImVec2(canvasOrigin.x + canvasSize.x, canvasSize.y + canvasOrigin.y);

			// Set interactive area by using invisibleButton. (You can use IsItemHovered and IsItemActive to control it)
			ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonLeft | ImGuiPopupFlags_MouseButtonRight);
			const bool isHover = ImGui::IsItemHovered();
			const bool isActive = ImGui::IsItemActive();

			// Get mouse down point
			if (isHover && isActive && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				// (Deprecate) Use domain map.
				//const pointf2& mousePosInCanvas = ScreenPos2DomainDefinition(data, ImVec2(io.MousePos.x, io.MousePos.y), canvasOrigin, canvasSize);
				data->xs.push_back(io.MousePos.x - canvasOrigin.x);
				data->ys.push_back(canvasDiagonal.y - io.MousePos.y);
			}

			// Add backgroud and border
			drawList->AddRectFilled(canvasOrigin, canvasDiagonal, IM_COL32(50, 50, 50, 255));
			drawList->AddRect(canvasOrigin, canvasDiagonal, IM_COL32(255, 255, 255, 255));

			// Content Item
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && io.MouseDelta.x == 0. && io.MouseDelta.y == 0.) {
				// Allow window to popup
				ImGui::OpenPopupContextItem("content");
			}

			// Draw popup.
			if (ImGui::BeginPopup("content")) {
				if (ImGui::MenuItem("RemoveAll", NULL, false, data->xs.size() > 0 || data->ys.size() > 0)) {
					data->xs.clear();
					data->ys.clear();
				}
				if (ImGui::MenuItem("RemoveOne", NULL, false, data->xs.size() > 0 || data->ys.size() > 0)) {
					data->xs.resize(data->xs.size() - 1);
					data->ys.resize(data->ys.size() - 1);
				}
				ImGui::EndPopup();
			}

			// Connect two points to create a simply line.
			if (data->xs.size() >= 1) {
				DrawCircle(data, IM_COL32(255, 255, 255, 255), canvasOrigin, canvasSize, drawList);
			}

			if (data->xs.size() >= 2 && data->switchs["enableLine"]) {
				DrawLine(data, IM_COL32(250, 250, 0, 255), canvasOrigin, canvasSize, drawList);
			}

			// Add render clip. Ignore which out of scope.
			drawList->PushClipRect(canvasOrigin, canvasDiagonal, true);
			// Draw interpolate and fitting line.
			if (data->xs.size() >= 2 && data->switchs["enableCurve"]) {
				CurveFit(data, drawList, canvasOrigin, canvasSize);
			}
			else if (data->xs.size() >= 2) {
				if (data->switchs["enablePolynomialInterpolate"]) {
					_PI(data, canvasOrigin, canvasSize, drawList);
				}
				if (data->switchs["enableGaussInterpolate"]) {
					GI(data, canvasOrigin, canvasSize, drawList, data->sigma);
				}
				if (data->switchs["enablePolynomialFit"]) {
					PF(data, drawList, canvasOrigin, canvasSize);
				}
				if (data->switchs["enableRidgeFit"]) {
					FR(data, drawList, canvasOrigin, canvasSize);
				}
			}
			drawList->PopClipRect();
		}
		ImGui::End();
	});
}

// (Deprecate)
//pointf2 FittingSystem::ScreenPos2DomainDefinition(CanvasData* data, const ImVec2& screenPos, const ImVec2& canvasOrigin, const ImVec2& canvasSize) {
//	float mapX = (screenPos.x - canvasOrigin.x) / canvasSize.x * (data->right - data->left) + data->left;
//	float mapY = (canvasSize.y - (screenPos.y - canvasOrigin.y)) / canvasSize.y * (data->maxValue - data->minValue) + data->minValue;
//	return pointf2(mapX, mapY);
//}

// (Deprecate)
//pointf2 FittingSystem::DomainDefinition2ScreenPos(CanvasData* data, const ImVec2& pos, const ImVec2& canvasOrigin, const ImVec2& canvasSize) {
//	float screenX = (pos.x - data->left) / (data->right - data->left) * canvasSize.x + canvasOrigin.x;
//	float screenY = (data->maxValue - pos.y) / (data->maxValue - data->minValue) * canvasSize.y + canvasOrigin.y;
//	return pointf2(screenX, screenY);
//}
