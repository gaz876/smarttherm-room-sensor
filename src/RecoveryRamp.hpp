#pragma once
#include "HeatingLimits.hpp"
#include <algorithm>
#include <cstdint>
class RecoveryRamp {
 float value=0; uint32_t last=0; bool active=false;
public:
 bool begin(float current,uint32_t now){active=std::isfinite(current)&&current>=HeatingLimits::floor&&current<=HeatingLimits::ceiling; value=current; last=now; return active;}
 void reset(){active=false;}
 bool step(float target,float rate,float low,float high,uint32_t now,uint32_t maxGap,float& out){
 if(!active || !std::isfinite(target)||!std::isfinite(rate)||rate<=0||!HeatingLimits{low,high}.valid()||value<low||value>high||!maxGap||maxGap>=0x80000000u){reset();return false;}
 uint32_t dt=now-last; if(dt>maxGap){reset();return false;}
 float limit=rate*(float(dt)/1000.f); if(!std::isfinite(limit)){reset();return false;}
 target=std::clamp(target,low,high); value+=std::clamp(target-value,-limit,limit); value=std::clamp(value,low,high); last=now;out=value;return true;
 }
};
