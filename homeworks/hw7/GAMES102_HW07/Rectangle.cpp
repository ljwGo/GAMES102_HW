#include "Rectangle.h"

Rectangle::Rectangle()
{
}

Rectangle::Rectangle(const Vector2f& scopeX, const Vector2f& scopeY)
{
	Vector2f v1(scopeX[0], scopeY[0]);
	Vector2f v2(scopeX[0], scopeY[1]);
	Vector2f v3(scopeX[1], scopeY[1]);
	Vector2f v4(scopeX[1], scopeY[0]);

	vertices.emplace_back(std::move(v1));
	vertices.emplace_back(std::move(v2));
	vertices.emplace_back(std::move(v3));
	vertices.emplace_back(std::move(v4));

	edges.emplace_back(Edge((vertices[0]), -1, vertices[1], -1));
	edges.emplace_back(Edge((vertices[1]), -1, vertices[2], -1));
	edges.emplace_back(Edge((vertices[2]), -1, vertices[3], -1));
	edges.emplace_back(Edge((vertices[3]), -1, vertices[0], -1));

	edgeIx["left"] = 0;
	edgeIx["right"] = 0;
	edgeIx["top"] = 0;
	edgeIx["bottom"] = 0;

	float lx = std::numeric_limits<float>::min();
	float by = std::numeric_limits<float>::min();

	for (int i = 0; i < 4; ++i) {
		int iNext = (i + 1) % 4;
		if (vertices[i].x() == vertices[iNext].x()) {
			if (vertices[i].x() > lx) {
				std::swap(edgeIx["left"], edgeIx["right"]);
				edgeIx["right"] = i;
				lx = vertices[i].x();
			}
			else {
				edgeIx["left"] = i;
			}

			if (vertices[i].y() > by) {
				std::swap(edgeIx["top"], edgeIx["bottom"]);
				edgeIx["bottom"] = i;
				by = vertices[i].y();
			}
			else {
				edgeIx["top"] = i;
			}
		}
	}
}

Rectangle::Rectangle(Vector2f& v1, Vector2f& v2, Vector2f& v3, Vector2f& v4)
{
	vertices.emplace_back(v1);
	vertices.emplace_back(v2);
	vertices.emplace_back(v3);
	vertices.emplace_back(v4);

	edges.emplace_back(Edge((vertices[0]), -1, vertices[1], -1));
	edges.emplace_back(Edge((vertices[1]), -1, vertices[2], -1));
	edges.emplace_back(Edge((vertices[2]), -1, vertices[3], -1));
	edges.emplace_back(Edge((vertices[3]), -1, vertices[0], -1));

	edgeIx["left"] = 0;
	edgeIx["right"] = 0;
	edgeIx["top"] = 0;
	edgeIx["bottom"] = 0;

	float lx = std::numeric_limits<float>::min();
	float by = std::numeric_limits<float>::min();

	for (int i = 0; i < 4; ++i) {
		int iNext = (i + 1) % 4;
		if (vertices[i].x() == vertices[iNext].x()) {
			if (vertices[i].x() > lx) {
				std::swap(edgeIx["left"], edgeIx["right"]);
				edgeIx["right"] = i;
				lx = vertices[i].x();
			}
			else {
				edgeIx["left"] = i;
			}

			if (vertices[i].y() > by) {
				std::swap(edgeIx["top"], edgeIx["bottom"]);
				edgeIx["bottom"] = i;
				by = vertices[i].y();
			}
			else {
				edgeIx["top"] = i;
			}
		}
	}
}

Rectangle::Rectangle(Vector2f&& v1, Vector2f&& v2, Vector2f&& v3, Vector2f&& v4)
{
	vertices.emplace_back(std::move(v1));
	vertices.emplace_back(std::move(v2));
	vertices.emplace_back(std::move(v3));
	vertices.emplace_back(std::move(v4));

	edges.emplace_back(Edge((vertices[0]), -1, vertices[1], -1));
	edges.emplace_back(Edge((vertices[1]), -1, vertices[2], -1));
	edges.emplace_back(Edge((vertices[2]), -1, vertices[3], -1));
	edges.emplace_back(Edge((vertices[3]), -1, vertices[0], -1));

	edgeIx["left"] = 0;
	edgeIx["right"] = 0;
	edgeIx["top"] = 0;
	edgeIx["bottom"] = 0;

	float lx = std::numeric_limits<float>::min();
	float by = std::numeric_limits<float>::min();

	for (int i = 0; i < 4; ++i) {
		int iNext = (i + 1) % 4;
		if (vertices[i].x() == vertices[iNext].x()) {
			if (vertices[i].x() > lx) {
				std::swap(edgeIx["left"], edgeIx["right"]);
				edgeIx["right"] = i;
				lx = vertices[i].x();
			}
			else {
				edgeIx["left"] = i;
			}

			if (vertices[i].y() > by) {
				std::swap(edgeIx["top"], edgeIx["bottom"]);
				edgeIx["bottom"] = i;
				by = vertices[i].y();
			}
			else {
				edgeIx["top"] = i;
			}
		}
	}
}

bool Rectangle::ifHasVertex(const Vector2f* vOther)
{
	for (Vector2f& v : vertices) {
		if (vOther == &v) return true;
	}
	return false;
}
