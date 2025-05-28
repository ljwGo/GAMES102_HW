#pragma once

#include <vector>
#include <unordered_map>
#include "Edge.h"

class Rectangle {
public:
	Rectangle();
	Rectangle(const Vector2f& scopeX, const Vector2f& scopeY);
	Rectangle(Vector2f& v1, Vector2f& v2, Vector2f& v3, Vector2f& v4);
	Rectangle(Vector2f&& v1, Vector2f&& v2, Vector2f&& v3, Vector2f&& v4);

	std::vector<Edge> GetEdges() const { return edges; }

	Eigen::Vector2f v1() const { return vertices[0]; }
	Eigen::Vector2f v2() const { return vertices[1]; }
	Eigen::Vector2f v3() const { return vertices[2]; }
	Eigen::Vector2f v4() const { return vertices[3]; }

	Edge e1() const { return edges[0]; }
	Edge e2() const { return edges[1]; }
	Edge e3() const { return edges[2]; }
	Edge e4() const { return edges[3]; }

	Edge l() const { return edges[edgeIx.at("left")]; }
	Edge r() const { return edges[edgeIx.at("right")]; }
	Edge t() const { return edges[edgeIx.at("top")]; }
	Edge b() const { return edges[edgeIx.at("bottom")]; }

	bool ifHasVertex(const Vector2f* vOther);

private:
	std::vector<Edge> edges;
	std::vector<Vector2f> vertices;
	std::unordered_map<std::string, size_t> edgeIx;
};