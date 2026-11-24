#pragma once
#include "SafetyControl.hpp"
#include "WiredSensors.hpp"
#include "ReserveControl.hpp"
struct HeatingDecision {
 SafetyControl::Mode mode;
 WiredResult wired;
 ReserveDecision reserve;
};
class HeatingControl {
 SafetyControl safety;
public:
 bool sample(uint32_t now,float t,uint64_t session,uint64_t seq,bool fresh) {
  return safety.sample(now,t,session,seq,fresh);
 }
 HeatingDecision update(uint32_t now,bool heating,bool pid,bool ot,
 WiredReading supply,WiredReading ret,WiredLimits limits,
 ReserveConfig config,bool weatherOk,float weather,bool previousOk,float previous) {
  auto mode=safety.update(now,heating,pid,ot,false);
  auto wired=evaluateWired(now,supply,ret,limits);
  ReserveDecision reserve{ReserveMode::Invalid,NAN};
  if(mode==SafetyControl::RESERVE || mode==SafetyControl::RECOVERY)
   reserve=reserveTarget(config,weatherOk,weather,wired.deltaOk,previousOk,previous);
  return {mode,wired,reserve};
 }
};
