#include "ReserveControl.hpp"
#include "RecoveryRamp.hpp"
#include <cassert>
int main(){
 assert(reserveTarget({39,82,45},true,50,false,false,0).mode==ReserveMode::Invalid);
 assert(reserveTarget({40,83,45},true,50,false,false,0).mode==ReserveMode::Invalid);
 assert(reserveTarget({40,82,45},true,30,false,false,0).target==40);
 assert(reserveTarget({40,82,45},true,90,false,false,0).target==82);
 RecoveryRamp r; float out=0;
 assert(!r.begin(39,0)); assert(!r.begin(83,0)); assert(r.begin(40,0));
 assert(!r.step(50,1,39,82,1000,2000,out));
 assert(r.begin(40,0)); assert(r.step(30,1,40,82,1000,2000,out)&&out==40);
 assert(r.begin(82,0)); assert(r.step(90,1,40,82,1000,2000,out)&&out==82);
}
