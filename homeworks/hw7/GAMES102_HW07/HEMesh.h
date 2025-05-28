#pragma once

#include "details/ForwardDecl.h"

#include "THEMesh.h"
#include "TPolygon.h"
#include "TVertex.h"
#include "THEMesh.h"

struct HETriangle;
struct HEVertex;
struct HEHalfEdge;

using MyHEMeshTraits = HEMeshTraits<HEVertex, HEHalfEdge, HETriangle>;

struct HETriangle : public TPolygon<MyHEMeshTraits>
{

};

struct HEVertex : public TVertex<MyHEMeshTraits>
{
	Vector2f position{ 0, 0 };

	HEVertex() {};

	HEVertex(int x, int y) {
		position[0] = x;
		position[1] = y;
	}

	std::vector<HEHalfEdge*> BoundaryStartOutHe() {
		using Base = THalfEdge<MyHEMeshTraits>;

		if (IsIsolated()) return std::vector<HEHalfEdge*>();
		if (!IsOnBoundary()) return OutHalfEdges();
		Base* he = (Base*)GetHalfEdge();

		while (!he->IsOnBoundary())
		{
			he = (Base*)he->RotateNext();
		}

		std::vector<HEHalfEdge*> outHes;
		Base* origin = he;
		outHes.push_back((HEHalfEdge*)he);
		he = (Base*)he->RotateNext();
		while (he != origin) {
			outHes.push_back((HEHalfEdge*)he);
			he = (Base*)he->RotateNext();
		}
		return outHes;
	}
};

struct HEHalfEdge : public THalfEdge<MyHEMeshTraits>
{
	// true is right; false if right.
	bool RightOrLeft(const Vector2f& v) {
		const Vector2f& e = End()->position - Origin()->position;
		Vector3f e3(e.x(), e.y(), 0.f);
		Vector2f v2 = v - Origin()->position;
		Vector3f v3(v2.x(), v2.y(), 0.f);

		return e3.cross(v3).z() > 0;
	}
};

struct HEMesh : public THEMesh<MyHEMeshTraits>
{

};