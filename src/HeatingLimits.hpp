#pragma once
#include <cmath>
#include <algorithm>
struct HeatingLimits {
 static constexpr float floor=40.f, ceiling=82.f;
 float minimum, maximum;
 bool valid() const {return std::isfinite(minimum)&&std::isfinite(maximum)&&minimum>=floor&&maximum<=ceiling&&minimum<=maximum;}
 bool bound(float requested,float& output) const {if(!valid()||!std::isfinite(requested))return false;output=std::clamp(requested,minimum,maximum);return true;}
};
