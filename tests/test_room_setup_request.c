#include "room_setup_request.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void rejected(const uint8_t *p,size_t n) {
    room_setup_request r={.command=ROOM_SETUP_SAVE,.trial=UINT64_MAX,
        .ssid_offset=1,.ssid_len=1,.pass_offset=1,.pass_len=1};
    assert(!room_setup_parse(p,n,&r));
    assert(!r.command && !r.trial && !r.ssid_offset && !r.ssid_len && !r.pass_offset && !r.pass_len);
}
int main(void) {
    room_setup_request r={0};
    const uint8_t status[]={1,ROOM_SETUP_STATUS};
    const uint8_t cancel[]={1,ROOM_SETUP_CANCEL};
    const uint8_t save[]={1,ROOM_SETUP_SAVE,0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef};
    const uint8_t zero[]={1,ROOM_SETUP_SAVE,0,0,0,0,0,0,0,0};
    const uint8_t max[]={1,ROOM_SETUP_SAVE,255,255,255,255,255,255,255,255};
    assert(room_setup_parse(status,sizeof(status),&r) && r.command==ROOM_SETUP_STATUS && !r.trial);
    assert(room_setup_parse(cancel,sizeof(cancel),&r) && r.command==ROOM_SETUP_CANCEL);
    assert(room_setup_parse(save,sizeof(save),&r) && r.trial==UINT64_C(0x0123456789abcdef));
    assert(room_setup_parse(max,sizeof(max),&r) && r.trial==UINT64_MAX);
    rejected(zero,sizeof(zero)); rejected(NULL,0); rejected(NULL,99);
    assert(!room_setup_parse(status,sizeof(status),NULL));
    for (size_t n=0;n<sizeof(save);n++) rejected(save,n);
    uint8_t buf[100]={1,ROOM_SETUP_STAGE,1,8};
    for (unsigned s=0;s<256;s++) for (unsigned p=0;p<256;p++) {
        buf[2]=(uint8_t)s; buf[3]=(uint8_t)p;
        size_t n=4u+s+p;
        if (n>sizeof(buf)) n=sizeof(buf);
        bool valid=s>=1 && s<=32 && p>=8 && p<=63;
        assert(room_setup_parse(buf,n,&r)==valid);
        if (valid) {
            assert(r.command==ROOM_SETUP_STAGE && !r.trial && r.ssid_offset==4 && r.ssid_len==s && r.pass_offset==4u+s && r.pass_len==p);
            rejected(buf,n-1); rejected(buf,n+1);
        }
    }
    for (size_t n=0;n<=100;n++) {
        uint8_t *v=calloc(n?n:1,1); assert(v);
        if (n) v[0]=1;
        if (n>=2) v[1]=ROOM_SETUP_STAGE;
        rejected(v,n); free(v);
    }
    for (unsigned v=0;v<256;v++) if (v!=1) {
        buf[0]=(uint8_t)v; buf[1]=ROOM_SETUP_STATUS; rejected(buf,2);
    }
    buf[0]=1;
    for (unsigned op=0;op<256;op++) if (op!=ROOM_SETUP_STATUS && op!=ROOM_SETUP_CANCEL) {
        buf[1]=(uint8_t)op; rejected(buf,2);
    }
    puts("room setup request PASS: 65536 length pairs, framing and boundary cases");
    return 0;
}
