#include "room_admission.h"
bool probe_init(room_admission *g,uint64_t w){return room_admission_init(g,w);}
bool probe_enter(room_admission *g,uint64_t w,room_lease *l){return room_admission_enter(g,w,l);}
bool probe_leave(room_lease *l){return room_admission_leave(l);}
bool probe_close(room_admission *g,uint64_t w){return room_admission_close(g,w);}
bool probe_drained(room_admission *g,uint64_t w){return room_admission_drained(g,w);}
