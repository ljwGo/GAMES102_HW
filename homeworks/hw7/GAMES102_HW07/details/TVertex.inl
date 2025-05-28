#pragma once

#include <vector>
#include <set>

#include "ForwardDecl.h"
#include "TVertex.h"

template<typename Traits>
bool TVertex<Traits>::IsOnBoundary() const noexcept {
	if (IsIsolated())
		return true;

	auto* begin = GetHalfEdge();
	auto* he = begin;
	do {
		if (he->IsOnBoundary())
			return true;
		he = he->RotateNext();
	} while (he != begin);

	return false;
}

template<typename Traits>
size_t TVertex<Traits>::Degree() const noexcept {
	if (IsIsolated())
		return 0;

	auto* begin = GetHalfEdge();
	auto* he = begin;
	size_t degree = 0;
	do {
		++degree;
		he = he->RotateNext();
	} while (he != begin);

	return degree;
}

template<typename Traits>
std::vector<HEMeshTraits_V<Traits>*> TVertex<Traits>::AdjVertices()
{
	std::vector<V*> adjVs;
	for (auto* he : OutHalfEdges())
		adjVs.push_back(he->End());
	return adjVs;
}

template<typename Traits>
std::set<HEMeshTraits_P<Traits>*> TVertex<Traits>::AdjPolygons() {
	std::set<P*> adjPs;
	for (auto* he : OutHalfEdges())
		adjPs.insert(he->Polygon());
	return adjPs;
}

template<typename Traits>
inline std::vector<HEMeshTraits_HE<Traits>*> TVertex<Traits>::AdjHalfEdges()
{
	return OutHalfEdges();
}

// Incident »Î…‰µƒ
template<typename Traits>
HEMeshTraits_HE<Traits>* TVertex<Traits>::FindFreeIncident() noexcept {
	if (IsIsolated())
		return nullptr;

	auto* begin = GetHalfEdge()->Pair();
	auto* he = begin;
	do {
		if (he->IsFree())
			return he;
		he = he->Next()->Pair();
	} while (he != begin);

	return nullptr;
}

template<typename Traits>
HEMeshTraits_HE<Traits>* TVertex<Traits>::HalfEdgeTo(V* end) noexcept {
	if (IsIsolated())
		return nullptr;

	auto* begin = GetHalfEdge();
	auto* he = begin;
	do {
		if (he->End() == end)
			return he;
		he = he->RotateNext();
	} while (he != begin);

	return nullptr;
}
