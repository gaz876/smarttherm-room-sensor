#include "WiredSensors.hpp"
#include <cassert>
#include <iostream>
int main(){ WiredLimits l{0,100,1000}; WiredReading a{45,100,true,true},b{35,100,true,true}; auto r=evaluateWired(200,a,b,l); assert(r.deltaOk&&r.delta==10); assert(evaluateWired(200,b,a,l).delta==-10); assert(!evaluateWired(1100,a,b,l).deltaOk); auto bad=a; bad.crcOk=false; assert(!evaluateWired(200,bad,b,l).supplyOk); bad=a; bad.conversionComplete=false; assert(!wiredValid(200,bad,l)); bad=a; bad.celsius=NAN; assert(!wiredValid(200,bad,l)); bad.celsius=101; assert(!wiredValid(200,bad,l)); assert(!wiredValid(200,a,{100,0,1000})); a.timestamp=UINT32_MAX-50; assert(wiredValid(49,a,l)); a.timestamp=201; assert(!wiredValid(200,a,l)); std::cout<<"PASS: 10 checks\n"; }
