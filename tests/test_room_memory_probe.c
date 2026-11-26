#include "room_memory_probe.h"
#include <assert.h>
#include <stdio.h>
static unsigned calls;
static size_t current=100000, minimum=80000, largest=40000;
static UBaseType_t stack=2048;
static void check(uint32_t c) { assert(c==(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT));calls++; }
size_t heap_caps_get_free_size(uint32_t c) { check(c);return current; }
size_t heap_caps_get_minimum_free_size(uint32_t c) { check(c);return minimum; }
size_t heap_caps_get_largest_free_block(uint32_t c) { check(c);return largest; }
UBaseType_t uxTaskGetStackHighWaterMark(TaskHandle_t t) { assert(!t);calls++;return stack; }
int main(void) {
    room_memory_snapshot s={0};
    assert(!room_memory_sample(NULL)&&calls==0);
    assert(room_memory_sample(&s)&&calls==4);
    assert(s.free_internal==100000&&s.minimum_internal==80000&&s.largest_internal==40000&&s.task_stack_min==2048);
    current=90000;minimum=70000;largest=30000;stack=1024;
    assert(room_memory_sample(&s)&&calls==8);
    assert(s.free_internal==90000&&s.minimum_internal==70000&&s.largest_internal==30000&&s.task_stack_min==1024);
    current=minimum=largest=stack=0;
    assert(room_memory_sample(&s)&&calls==12);
    assert(!s.free_internal&&!s.minimum_internal&&!s.largest_internal&&!s.task_stack_min);
    puts("memory probe mock PASS");return 0;
}
