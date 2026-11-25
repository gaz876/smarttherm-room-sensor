#include "OpenThermGuard.hpp"
#include <cassert>
#include <limits>
int main(){
 using G=OpenThermGuard; using S=G::State;
 const HeatingLimits l{40,82};
 auto check=[](G::Decision d,S s,float t){assert(d.state==s);assert(d.target==t);};
 const float nan=std::numeric_limits<float>::quiet_NaN(), inf=std::numeric_limits<float>::infinity();
 check(G::evaluate(false,true,50,l),S::CommunicationFault,0);
 check(G::evaluate(false,false,50,l),S::CommunicationFault,0);
 check(G::evaluate(true,false,nan,l),S::Off,0);
 check(G::evaluate(true,true,30,l),S::Active,40);
 check(G::evaluate(true,true,90,l),S::Active,82);
 check(G::evaluate(true,true,50,l),S::Active,50);
 check(G::evaluate(true,true,90,{40,65}),S::Active,65);
 for(float v:{nan,inf,-inf})check(G::evaluate(true,true,v,l),S::InvalidRequest,0);
 for(auto bad:{HeatingLimits{39,82},HeatingLimits{40,83},HeatingLimits{65,40},HeatingLimits{nan,82}})check(G::evaluate(true,true,50,bad),S::InvalidRequest,0);
}
