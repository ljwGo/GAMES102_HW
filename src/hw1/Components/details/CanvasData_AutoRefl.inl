// This file is generated by Ubpa::USRefl::AutoRefl

#pragma once

#include <USRefl/USRefl.h>

template<>
struct Ubpa::USRefl::TypeInfo<CanvasData> :
    TypeInfoBase<CanvasData>
{
#ifdef UBPA_USREFL_NOT_USE_NAMEOF
    static constexpr char name[11] = "CanvasData";
#endif
    static constexpr AttrList attrs = {};
    static constexpr FieldList fields = {
        Field {TSTR("xs"), &Type::xs},
        Field {TSTR("ys"), &Type::ys},
        Field {TSTR("switchs"), &Type::switchs, AttrList {
            Attr {TSTR(UMeta::initializer), []()->std::map<std::string, bool>{ return {
		{"enablePolynomialInterpolate", false},
		{"enableGaussInterpolate", false},
		{"enablePolynomialFit", false},
		{"enableRidgeFit", false},
		{"enableCurve", false},
		{"enableLine", true},
	}; }},
        }},
        Field {TSTR("fitBaseCount"), &Type::fitBaseCount, AttrList {
            Attr {TSTR(UMeta::initializer), []()->int{ return { 4 }; }},
        }},
        Field {TSTR("paramMode"), &Type::paramMode, AttrList {
            Attr {TSTR(UMeta::initializer), []()->int{ return { 1 }; }},
        }},
        Field {TSTR("delta"), &Type::delta, AttrList {
            Attr {TSTR(UMeta::initializer), []()->float{ return { 1 }; }},
        }},
        Field {TSTR("sigma"), &Type::sigma, AttrList {
            Attr {TSTR(UMeta::initializer), []()->float{ return { 10.0 }; }},
        }},
        Field {TSTR("lambda"), &Type::lambda, AttrList {
            Attr {TSTR(UMeta::initializer), []()->float{ return { 0.2 }; }},
        }},
        Field {TSTR("tDelta"), &Type::tDelta, AttrList {
            Attr {TSTR(UMeta::initializer), []()->float{ return { 1 }; }},
        }},
        Field {TSTR("tInterval"), &Type::tInterval, AttrList {
            Attr {TSTR(UMeta::initializer), []()->float{ return { 30 }; }},
        }},
    };
};

