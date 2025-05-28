#pragma once

#include "details/ForwardDecl.h"
#include <vector>

template <typename Traits>
class TPolygon {
public:
	using V = typename HEMeshTraits_V<Traits>;
	using HE = typename  HEMeshTraits_HE<Traits>;
	using P = typename  HEMeshTraits_P<Traits>;

public:
	HE* HalfEdge() noexcept { return halfEdge; }
	const HE* HalfEdge() const noexcept { return const_cast<TPolygon*>(this)->HalfEdge(); }

	void SetHalfEdge(HE* he) noexcept { halfEdge = he; }

	// p == nullptr
	static bool IsBoundary(const P* p) noexcept { return p == nullptr; }

	// number of edges/vertices
	size_t Degree() const noexcept;

	// halfedges : [ he, he.Next(), he.Next().Next(), ..., he)
	std::vector<HE*> AdjHalfEdges() { return HalfEdge()->NextLoop(); }

	// vertices : { halfedge.Origin() for halfedge in AdjHalfEdges() }
	std::vector<V*> AdjVertices();

	// adjacent polygons : { halfedge.Pair().Polygon() for halfedge in AdjHalfEdges() }
	// maybe contains boundary polygon (nullptr)
	// use IsBoundary() to find it
	std::vector<P*> AdjPolygons();

private:
	friend THEMesh<Traits>;

	HE* halfEdge{ nullptr };
};

#include "details/TPolygon.inl"