#include "RecoveryAdapter.hpp"
#include <cassert>
int main(){RecoveryAdapter a; float out=0; assert(!a.begin(40,0,false)); assert(!a.step(true,true,true,50,1,40,60,1000,2000,out)); assert(a.begin(40,0,true)); assert(a.step(true,true,true,50,1,40,60,1000,2000,out)&&out==41); assert(!a.step(false,true,true,50,1,40,60,2000,2000,out)); assert(!a.step(true,true,true,50,1,40,60,2000,2000,out)); assert(a.begin(40,0,true)); assert(!a.step(true,true,false,50,1,40,60,1000,2000,out)); assert(a.begin(40,0,true)); assert(!a.step(true,false,true,50,1,40,60,1000,2000,out));}
