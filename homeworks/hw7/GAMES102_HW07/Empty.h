#pragma once

#include "details/ForwardDecl.h"

#include "THalfEdge.h"
#include "TVertex.h"
#include "TPolygon.h"
#include "THEMesh.h"

class EmptyVPH_V;
class EmptyVPH_P;
class EmptyVPH_HE;

using EmptyTraits = HEMeshTraits<EmptyVPH_V, EmptyVPH_HE,  EmptyVPH_P>;

class EmptyVPH_V : public TVertex<EmptyTraits>{};

class EmptyVPH_P : public TPolygon<EmptyTraits>{};

class EmptyVPH_HE : public THalfEdge<EmptyTraits>{};

