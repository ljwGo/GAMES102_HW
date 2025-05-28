#pragma once

#include "details/ForwardDecl.h"

template <typename Traits>
class TVertex {
public:
	using V = typename HEMeshTraits_V<Traits>;
	using HE = typename  HEMeshTraits_HE<Traits>;
	using P = typename  HEMeshTraits_P<Traits>;

public:
	HE* GetHalfEdge() noexcept { return halfEdge; }
	const HE* GetHalfEdge() const noexcept { return const_cast<TVertex*>(this)->GetHalfEdge(); }

	void SetHalfEdge(HE* he) noexcept { halfEdge = he; }

	bool IsIsolated() const noexcept { return !halfEdge; }

	// vertex is on boundary == any adjacent polygon is boundary (nullptr)
	bool IsOnBoundary() const noexcept;

	// number of adjacent edges
	size_t Degree() const noexcept;

	// outward halfedges : [he, he.RotateNext(), he.RotateNext().RotateNext(), ..., he)
	// HalfEdge which origin is this vertex;
	std::vector<HE*> OutHalfEdges() { return IsIsolated() ? std::vector<HE*>() : GetHalfEdge()->RotateNextLoop(); }

	// adjacent vertices : { halfedge.End() for halfedge in OutHalfEdges() }
	std::vector<V*> AdjVertices();

	// adjacent polygons : { halfedge.End() for halfedge in OutHalfEdges() }
	// [WARNING]
	// if IsOnBoundary(), result contains nullptr
	// use P::IsBoundary() to find it
	std::set<P*> AdjPolygons();

	// adjacent edges : { halfedge.Edge() for halfedge in OutHalfEdges() }
	std::vector<HE*> AdjHalfEdges();

	// find halfedge (this -> end)
	// nullptr if no exist
	HE* HalfEdgeTo(V* end) noexcept;
	const HE* HalfEdgeTo(const V* end) const noexcept
	{
		return const_cast<TVertex*>(this)->HalfEdgeTo(const_cast<V*>(end));
	}

	// find halfedge (origin -> end)
	// nullptr if no exist
	static HE* HalfEdgeAlong(V* origin, V* end) noexcept { return origin->HalfEdgeTo(end); }
	static const HE* HalfEdgeAlong(const V* origin, const V* end) noexcept
	{
		return HalfEdgeAlong(const_cast<V*>(origin), const_cast<V*>(end));
	}

private:
	friend THEMesh<Traits>;
	HE* FindFreeIncident() noexcept;
	HE* halfEdge{ nullptr };
};

#include "details/TVertex.inl"