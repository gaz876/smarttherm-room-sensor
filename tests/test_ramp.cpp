#include "RecoveryRamp.hpp"
#include <cassert>
int main(){RecoveryRamp r; float out=0; assert(!r.step(50,1,40,60,0,2000,out)); assert(r.begin(40,0)); assert(r.step(50,1,40,60,1000,2000,out)&&out==41); assert(r.step(30,1,40,60,2000,2000,out)&&out==40); assert(!r.step(50,1,40,60,5001,2000,out)); assert(r.begin(40,UINT32_MAX-499)); assert(r.step(50,1,40,60,500,2000,out)&&out==41); assert(!r.step(NAN,1,40,60,600,2000,out)); assert(!r.begin(NAN,0));}
