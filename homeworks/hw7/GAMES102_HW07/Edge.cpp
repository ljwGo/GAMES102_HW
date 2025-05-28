#include "Edge.h"

Edge::Edge() {
	vs = nullptr;
	ve = nullptr;
	this->vsInd = -1;
	this->veInd = -1;
}

Edge::Edge(Vector2f& vs, int vsInd, Vector2f& ve, int veInd)
{
	this->vs = &vs;
	this->ve = &ve;
	this->vsInd = vsInd;
	this->veInd = veInd;
}

bool Edge::operator==(const Edge& other) const
{
	return (vs == other.vs && ve == other.ve) || 
		(vs == other.ve && ve == other.vs);
}
