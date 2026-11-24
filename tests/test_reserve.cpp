#include "ReserveControl.hpp"
#include <cassert>
int main(){ReserveConfig c{30,60,45}; assert(reserveTarget(c,true,70,true,true,40).target==60); assert(reserveTarget(c,false,0,true,true,40).mode==ReserveMode::WiredHold); assert(reserveTarget(c,false,0,false,true,40).target==45); assert(reserveTarget(c,true,NAN,true,true,40).target==40); assert(reserveTarget(c,false,0,true,false,40).mode==ReserveMode::Fixed); assert(reserveTarget({60,30,45},false,0,false,false,0).mode==ReserveMode::Invalid);}
