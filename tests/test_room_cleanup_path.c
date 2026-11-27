/* Host-only source-fragment regression. Real cleanup bodies, reduced types
 * and synthetic SDK calls. Not a full translation-unit or target build. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include "room_cleanup_budget.h"
static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static int64_t us;
static unsigned stops,detaches,destroys,deinits,security,events,backend,completions,pre_calls,post_calls;
static bool backend_ready,pre,post;
/* Inject a delay inside each otherwise successful SDK operation. */
enum { NONE,BACKEND,STOP,PRE,DETACH,DESTROY,DEINIT,POST,SECURITY,EVENTS };
static unsigned delay_at,error_at;
static void delay(unsigned where) { if (where==delay_at) us=5100000; }
static int64_t esp_timer_get_time(void) { return us; }
static uint64_t now_ms(void) { return us<0?UINT64_MAX:(uint64_t)us/1000u; }
static void enroll_wipe(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
#define ESP_OK 0
typedef struct { bool up; } esp_netif_t;
static esp_netif_t sta,ap;
static int esp_wifi_stop(void){++stops;delay(STOP);return error_at==STOP?-1:ESP_OK;}
static int esp_wifi_clear_default_wifi_driver_and_handlers(esp_netif_t *n){CHECK(n);++detaches;delay(DETACH);return error_at==DETACH?-1:ESP_OK;}
static void esp_netif_destroy(esp_netif_t *n){CHECK(n);++destroys;delay(DESTROY);}
static int esp_wifi_deinit(void){++deinits;delay(DEINIT);return error_at==DEINIT?-1:ESP_OK;}
static bool esp_netif_is_netif_up(esp_netif_t *n){return n->up;}
typedef enum {APSTA_IDLE,APSTA_RUNNING,APSTA_STOP,APSTA_DRAIN,APSTA_FINAL_DRAIN,APSTA_LOCKED} room_apsta_phase;
typedef struct { room_apsta_phase phase;esp_netif_t *sta,*ap;uint64_t window,last_window;bool attempted,uncertain,driver,sta_attached,ap_attached; } room_apsta_owner;
typedef bool (*room_apsta_barrier)(void *,uint64_t,bool);
#include "radio.inc"
typedef struct { struct {bool storage_uncertain;} enrollment;uint64_t saved_generation; } room_phone_setup;
typedef struct {uint64_t window,last_window;bool stopping;room_phone_setup phone;} room_setup_bridge;
typedef struct {room_setup_bridge *bridge;bool pending,dispatching;} room_sec2_transport;
typedef struct {room_apsta_owner radio;room_setup_bridge bridge;room_sec2_transport transport;uint64_t (*clock)(void *);void *ctx;uint32_t lease;uint64_t cleanup_deadline,cleanup_last;bool locked;} room_apsta_session;
static uint64_t session_clock(void *v){room_apsta_session *s=v;return s->clock(s->ctx);}
static uint64_t clock_ms(void *v){(void)v;return now_ms();}
static void bridge_stop(room_setup_bridge *b){b->stopping=true;}
static bool bridge_drained(room_setup_bridge *b,uint64_t now,uint64_t w){(void)now;CHECK(w==b->window);b->window=0;return true;}
static bool room_sec2_transport_destroy(room_sec2_transport *t,bool drained){CHECK(drained&&!t->pending&&!t->dispatching);++security;delay(SECURITY);t->bridge->window=0;t->bridge=NULL;return true;}
#define cleanup_live session_cleanup_live
#include "session.inc"
#undef cleanup_live
typedef struct {atomic_bool accepting;} event_state;
typedef struct {room_apsta_session session;event_state events;void *owner;unsigned pending;bool ready,failed;} room_apsta_task;
static int owner_id;
static void *xTaskGetCurrentTaskHandle(void){return &owner_id;}
static bool owned(room_apsta_task *t){return t&&t->ready&&!t->failed&&t->owner==xTaskGetCurrentTaskHandle();}
static bool task_barrier(void *v,uint64_t window,bool final){(void)v;CHECK(window==7);if(final){++post_calls;delay(POST);return post;}++pre_calls;delay(PRE);return pre;}
static void room_apsta_events_end(event_state *e){++events;atomic_store(&e->accepting,false);delay(EVENTS);}
#define barrier task_barrier
#include "task.inc"
#undef barrier
static bool room_apsta_task_sent(room_apsta_task *t,bool ok){CHECK(!ok);++completions;t->session.transport.pending=false;return true;}
typedef struct {void *ctx;bool (*open)(void);bool (*receive)(void);bool (*send)(void);bool (*close)(void *);} room_phone_io;
typedef bool (*room_phone_clock)(void *,uint64_t *);
typedef bool (*room_phone_timed_close)(void *,uint64_t,uint64_t);
typedef struct {room_phone_io io;room_apsta_task *task;uint64_t window;const uint8_t *reply;size_t length,offset;uint8_t packet[4096];bool attempted,stopping,backend_drained,timed,locked,legacy_close;uint64_t cleanup_deadline,cleanup_last;room_phone_clock clock;void *clock_ctx;room_phone_timed_close timed_close;} room_phone_channel;
#include "channel.inc"
typedef enum {ROOM_WIFI_SETUP_NO_SAVE,ROOM_WIFI_SETUP_COMMITTED,ROOM_WIFI_SETUP_UNCERTAIN} room_wifi_setup_result;
static struct {room_apsta_task task;room_phone_channel channel;room_phone_io io;bool io_bound,attempted;room_phone_timed_close timed_close;uint64_t cleanup_deadline,cleanup_last,prior_save;} a;
#define stop link_stop
#include "link.inc"
#undef stop
static bool room_wifi_idf_is_owner(void){return true;}
#include "attach.inc"
static bool outer_barrier(void){return true;}
typedef enum {CLEAN_NONE,CLEAN_STOPPED,CLEAN_DETACHED,CLEAN_DEINIT} clean_state;
enum {ROOM_WIFI_PROFILE_VALID,ROOM_WIFI_PROFILE_ERROR,ROOM_WIFI_RETRY,ROOM_WIFI_OFF};
static struct {uint64_t session,cleanup_deadline,cleanup_last;bool fatal,apsta,attempted,driver,bound;clean_state clean;esp_netif_t *netif;struct{bool pending_setup;struct{int profile,state;uint32_t retry_ms;uint64_t deadline;}policy;}owner;struct{void *ctx;bool(*stop)(void *,uint64_t,room_wifi_setup_result *);}setup_io;uint8_t config[8];} w;
#define barrier outer_barrier
#include "outer.inc"
#undef barrier
static bool backend_close(void *v,uint64_t window,uint64_t deadline){(void)v;CHECK(window==7&&deadline==5100);++backend;delay(BACKEND);return backend_ready;}
static bool legacy_backend(void *v){(void)v;return false;}
static bool no_io(void){return false;}
static void reset(bool partial){
    memset(&w,0,sizeof w);memset(&a,0,sizeof a);atomic_init(&a.task.events.accepting,true);
    stops=detaches=destroys=deinits=security=events=backend=completions=pre_calls=post_calls=0;us=100000;delay_at=error_at=NONE;backend_ready=pre=post=false;
    w.apsta=true;w.setup_io.stop=link_stop;a.attempted=a.task.ready=true;a.task.owner=&owner_id;
    room_apsta_session *s=&a.task.session;s->clock=clock_ms;s->bridge.window=7;s->bridge.last_window=7;s->transport.bridge=&s->bridge;s->transport.pending=!partial;
    s->radio=(room_apsta_owner){.phase=APSTA_RUNNING,.window=7,.last_window=7,.sta=&sta,.ap=&ap,.attempted=true,.driver=true,.sta_attached=true,.ap_attached=true};
    room_phone_io io={NULL,no_io,no_io,no_io,legacy_backend};
    /* Attach normally happens before any window exists. */
    s->bridge.last_window=0;CHECK(room_wifi_apsta_link_attach_io_until(io,backend_close));CHECK(!room_wifi_apsta_link_attach_io(io));s->bridge.last_window=7;
    a.channel=(room_phone_channel){.io=io,.task=&a.task,.window=7,.attempted=true};
}
static void normal(bool partial){
    reset(partial);CHECK(!stop(NULL));CHECK(backend==1&&w.cleanup_deadline==5100&&a.cleanup_deadline==5100&&a.channel.cleanup_deadline==5100&&!stops);
    us=2100000;backend_ready=true;CHECK(!stop(NULL));CHECK(backend==2&&stops==1&&a.task.session.cleanup_deadline==5100&&completions==!partial&&!detaches);
    us=3100000;pre=true;CHECK(!stop(NULL));CHECK(detaches==2&&destroys==2&&deinits==1&&post_calls==1&&!security);
    us=4100000;post=true;CHECK(stop(NULL));CHECK(!w.apsta&&!w.fatal&&!w.cleanup_deadline&&!a.cleanup_deadline&&!a.task.session.cleanup_deadline&&security==1&&events==1&&stops==1&&backend==2);
}
static void late(unsigned where){
    reset(false);CHECK(!stop(NULL));us=2100000;backend_ready=pre=post=true;delay_at=where;
    CHECK(!stop(NULL));CHECK(w.fatal&&w.apsta&&w.cleanup_deadline==5100);
    if(where==BACKEND)CHECK(!stops&&!completions&&a.channel.attempted);
    if(where==STOP||where==PRE)CHECK(!detaches&&!security);
    if(where==DETACH)CHECK(!destroys&&!security);
    if(where==DESTROY)CHECK(destroys==1&&detaches==1&&!security);
    if(where==DEINIT||where==POST)CHECK(!security);
    if(where==SECURITY)CHECK(!events);
    if(where==EVENTS)CHECK(events==1&&security==1);
    unsigned calls=backend+stops+detaches+destroys+deinits+security+events;
    us=3000000;delay_at=NONE;CHECK(!stop(NULL));CHECK(calls==backend+stops+detaches+destroys+deinits+security+events);
}
static void sdk_error(unsigned where){
    reset(false);backend_ready=pre=post=true;error_at=where;
    CHECK(!stop(NULL));CHECK(w.apsta&&a.task.session.locked&&a.task.session.radio.phase==APSTA_LOCKED&&!security);
    unsigned calls=stops+detaches+destroys+deinits;
    error_at=NONE;us=1100000;CHECK(!stop(NULL));CHECK(calls==stops+detaches+destroys+deinits);
    us=5100000;CHECK(!stop(NULL)&&w.fatal&&w.apsta);
}
static void station_case(unsigned where){
    reset(false);w.apsta=false;w.attempted=w.driver=w.bound=true;w.netif=&sta;delay_at=where;
    CHECK(stop(NULL)==(where==NONE));
    if(where==NONE)CHECK(!w.fatal&&!w.driver&&!w.netif&&!w.cleanup_deadline&&stops==1&&detaches==1&&destroys==1&&deinits==1);
    else{
        CHECK(w.fatal&&w.cleanup_deadline==5100);
        if(where==STOP)CHECK(!detaches&&!destroys&&!deinits);
        if(where==DETACH)CHECK(detaches==1&&!destroys&&!deinits);
        if(where==DESTROY)CHECK(destroys==1&&!deinits);
        if(where==DEINIT)CHECK(deinits==1);
        unsigned calls=stops+detaches+destroys+deinits;us=2000000;CHECK(!stop(NULL));CHECK(calls==stops+detaches+destroys+deinits);
    }
}
int main(void){
    normal(false);normal(true);
    sdk_error(STOP);sdk_error(DETACH);sdk_error(DEINIT);
    station_case(NONE);station_case(STOP);station_case(DETACH);station_case(DESTROY);station_case(DEINIT);
    for(unsigned n=BACKEND;n<=EVENTS;n++)late(n);
    reset(false);CHECK(!stop(NULL));us=99000;CHECK(!stop(NULL)&&w.fatal&&!stops);
    reset(false);CHECK(!stop(NULL));us=-1;CHECK(!stop(NULL)&&w.fatal&&!stops);
    reset(false);CHECK(!stop(NULL));CHECK(!link_stop(NULL,5200,&(room_wifi_setup_result){0}));CHECK(a.task.session.locked&&!stops);
    reset(false);CHECK(!stop(NULL));us=5100000;CHECK(!stop(NULL)&&w.fatal&&!stops);
    reset(false);room_apsta_session *s=&a.task.session;CHECK(!room_apsta_session_close(s,false,task_barrier,&a.task));CHECK(s->cleanup_deadline==5100);us=5099000;CHECK(!room_apsta_session_close(s,false,task_barrier,&a.task));CHECK(s->cleanup_deadline==5100);us=5100000;CHECK(!room_apsta_session_close(s,false,task_barrier,&a.task)&&s->locked);
    /* Legacy radio API remains available for its existing callers. */
    room_apsta_owner unused={0};CHECK(!room_apsta_stop(&unused,7,false,task_barrier,NULL));
    printf("cleanup path: %u checks PASS\n",checks);return 0;
}
