#include "HeatingControl.hpp"
#include <cassert>
#include <iostream>
int main() {
 HeatingControl h;
 auto run=[&](uint32_t now,bool heat=true,bool pid=true,bool ot=true,bool wired=true,bool weather=false){return h.update(now,heat,pid,ot,{45,now,wired,true},{35,now,true,true},{0,100,1000},{30,60,45},weather,55,true,40);};
 auto d=run(0); assert(d.mode==SafetyControl::RESERVE && d.reserve.mode==ReserveMode::WiredHold && d.reserve.target==40 && d.wired.delta==10);
 assert(run(0,true,true,true,false).reserve.mode==ReserveMode::Fixed);
 assert(run(0,true,true,true,false,true).reserve.mode==ReserveMode::Weather);
 assert(run(0,false,true,false).mode==SafetyControl::OFF);
 assert(run(0,true,true,false).reserve.mode==ReserveMode::Invalid);
 assert(run(0,true,false,true).mode==SafetyControl::MANUAL);
 for(unsigned i=0;i<6;++i) assert(h.sample(i*10000,23,1,i,true));
 assert(run(50000).mode==SafetyControl::RECOVERY);
 assert(run(50001).mode==SafetyControl::RECOVERY);
 assert(run(170000).mode==SafetyControl::RESERVE);
 std::cout<<"PASS: integration scenarios\n";
}
