#pragma once

using namespace Eigen;

class Delaunay {
public:
	std::vector<Triangle> Watson(std::vector<Vector2f>& vertices);
};