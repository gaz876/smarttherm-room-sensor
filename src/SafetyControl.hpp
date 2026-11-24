#pragma once
#include <cstdint>
#include <cmath>
class SafetyControl {
public:
 enum Mode { OFF, MANUAL, RESERVE, RECOVERY, NORMAL, OT_FAULT };
 static constexpr uint32_t stale=120000, minGap=8000, maxGap=15000;
 // Session must be an authenticated, monotonically increasing epoch; fresh must be verified by transport.
 bool sample(uint32_t now, float t, uint64_t session, uint64_t seq, bool fresh) {
   if(!fresh || !std::isfinite(t) || t< -10 || t>50) { count=0; return false; }
   if(seen && (session<sid || (session==sid && seq<=lastSeq))) return false;
   if(seen && session!=sid) { count=0; valid=false; mode=RESERVE; }
   if(valid && uint32_t(now-last)<minGap) return false;
   if(!valid || uint32_t(now-last)>maxGap) count=0;
   sid=session; lastSeq=seq; seen=valid=true; last=now; if(count<6) ++count; return true;
 }
 Mode update(uint32_t now,bool heating,bool pid,bool ot,bool recoveryDone=false) {
   if(!valid || uint32_t(now-last)>=stale) { valid=false; count=0; }
   if(!heating) return mode=OFF;
   if(!ot) return mode=OT_FAULT;
   if(!pid) return mode=MANUAL;
   if(!valid) return mode=RESERVE;
   if(mode==NORMAL) return mode;
   if(count<6 || uint32_t(now-last)>maxGap) return mode=RESERVE;
   if(mode==RECOVERY && recoveryDone) return mode=NORMAL;
   return mode=RECOVERY;
 }
private:
 bool seen=false,valid=false; uint64_t sid=0,lastSeq=0; uint32_t last=0; unsigned count=0; Mode mode=OFF;
};
