#pragma once

#include <vector>
#include <set>
#include <type_traits>

#include <cassert>

#include "../HEMeshTraits.h"

template<typename V, typename HE, typename P> struct HEMeshTraits;

template<typename Traits> class THalfEdge;
template<typename Traits> class TVertex;
template<typename Traits> class TPolygon;
template<typename Traits> class THEMesh;

struct EmptyVPH_V;
struct EmptyVPH_P;
struct EmptyVPH_HE;
struct EmptyVPH_HEM;
