#pragma once
#include <Eigen/Core>

using namespace Eigen;

class Edge
{
public:
	Edge();
	Edge(Vector2f& vs, int vsInd, Vector2f& ve, int veInd);
	bool operator==(const Edge& other) const;

	Vector2f* vs;
	Vector2f* ve;
	int vsInd;
	int veInd;
};

