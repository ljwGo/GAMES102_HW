#pragma once

template<typename Traits>
size_t TPolygon<Traits>::Degree() const noexcept {
	size_t degree = 0;
	auto* begin = HalfEdge();
	auto* he = begin;
	assert(he);
	do {
		++degree;
		he = he->Next();
	} while (he != begin);
	return degree;
}

template<typename Traits>
std::vector<HEMeshTraits_V<Traits>*> TPolygon<Traits>::AdjVertices() {
	std::vector<V*> vertices;
	for (auto* he : AdjHalfEdges())
		vertices.push_back(he->Origin());
	return vertices;
}

template<typename Traits>
std::vector<HEMeshTraits_P<Traits>*> TPolygon<Traits>::AdjPolygons() {
	std::vector<P*> polygons;
	for (auto* he : AdjHalfEdges())
		polygons.push_back(he->Pair()->Polygon());
	return polygons;
}