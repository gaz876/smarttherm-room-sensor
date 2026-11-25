#pragma once
#include "HeatingLimits.hpp"
struct OpenThermGuard {
 enum class State { CommunicationFault, Off, InvalidRequest, Active };
 struct Decision { State state; float target; };
 static Decision evaluate(bool linkOk, bool demand, float requested, const HeatingLimits& limits) {
  if (!linkOk) return {State::CommunicationFault, 0.f};
  if (!demand) return {State::Off, 0.f};
  float target=0.f;
  if (!limits.bound(requested,target)) return {State::InvalidRequest,0.f};
  return {State::Active,target};
 }
};
