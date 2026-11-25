#pragma once
#include "RecoveryRamp.hpp"
class RecoveryAdapter {
 RecoveryRamp ramp;
 bool prepared=false;
public:
 void cancel(){prepared=false; ramp.reset();}
 bool begin(float current,uint32_t now,bool pidPrepared){cancel(); prepared=pidPrepared && ramp.begin(current,now); return prepared;}
 bool step(bool roomValid,bool heating,bool ot,float target,float rate,float low,float high,uint32_t now,uint32_t maxGap,float& out){
  if(!prepared || !roomValid || !heating || !ot){cancel();return false;}
  if(!ramp.step(target,rate,low,high,now,maxGap,out)){cancel();return false;}
  return true;
 }
};
