#pragma once
#include "HeatingLimits.hpp"
#include <algorithm>
struct ReserveConfig { float minimum, maximum, fallback; };
enum class ReserveMode { Invalid, Weather, WiredHold, Fixed };
struct ReserveDecision { ReserveMode mode; float target; };
inline ReserveDecision reserveTarget(ReserveConfig c, bool weatherOk, float weather, bool wiredOk, bool previousOk, float previous) {
 if (!HeatingLimits{c.minimum,c.maximum}.valid()||!std::isfinite(c.fallback)||c.minimum>c.maximum||c.fallback<c.minimum||c.fallback>c.maximum) return {ReserveMode::Invalid,NAN};
 if (weatherOk&&std::isfinite(weather)) return {ReserveMode::Weather,std::clamp(weather,c.minimum,c.maximum)};
 if (wiredOk&&previousOk&&std::isfinite(previous)) return {ReserveMode::WiredHold,std::clamp(previous,c.minimum,c.maximum)};
 return {ReserveMode::Fixed,c.fallback};
}
