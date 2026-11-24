#pragma once
#include <cstdint>
#include <cmath>
struct WiredReading { float celsius; uint32_t timestamp; bool crcOk; bool conversionComplete; };
struct WiredLimits { float minimum, maximum; uint32_t maxAge; };
struct WiredResult { bool supplyOk, returnOk, deltaOk; float delta; };
inline bool wiredValid(uint32_t now, WiredReading r, WiredLimits l) {
 return std::isfinite(l.minimum) && std::isfinite(l.maximum) && l.minimum <= l.maximum && l.maxAge > 0 && l.maxAge < 0x80000000u && r.crcOk && r.conversionComplete && std::isfinite(r.celsius) && r.celsius >= l.minimum && r.celsius <= l.maximum && uint32_t(now-r.timestamp) < l.maxAge;
}
inline WiredResult evaluateWired(uint32_t now, WiredReading supply, WiredReading ret, WiredLimits l) {
 bool a=wiredValid(now,supply,l), b=wiredValid(now,ret,l);
 float d=a&&b ? supply.celsius-ret.celsius : NAN;
 return {a,b,a&&b&&std::isfinite(d),d};
}
