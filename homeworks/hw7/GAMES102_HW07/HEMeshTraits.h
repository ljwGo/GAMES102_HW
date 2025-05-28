#pragma once

template<typename Traits>
class THalfEdge;
template<typename Traits>
class TVertex;
template<typename Traits>
class TPolygon;
template<typename Traits>
class THEMesh;

template <typename V_, typename HE_, typename P_>
struct HEMeshTraits{
	using V = V_;
	using HE = HE_;
	using P = P_;
	using HEM = THEMesh<HEMeshTraits>;

	static constexpr bool IsValid() noexcept {
		return std::is_base_of_v<TVertex<HEMeshTraits>, V>&&
			std::is_base_of_v<THalfEdge<HEMeshTraits>, HE>&&
			std::is_base_of_v<TPolygon<HEMeshTraits>, P>&&
			std::is_default_constructible_v<V>&&
			std::is_default_constructible_v<HE>&&
			std::is_default_constructible_v<P>;
	}
};

template <typename Traits>
using HEMeshTraits_V = typename Traits::V;

template <typename Traits>
using HEMeshTraits_HE = typename Traits::HE;

template <typename Traits>
using HEMeshTraits_P = typename Traits::P;

template<typename Traits>
using HEMeshTraits_Mesh = typename Traits::HEM;