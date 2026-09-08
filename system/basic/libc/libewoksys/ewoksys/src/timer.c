#include <ewoksys/timer.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/interrupt.h>
#include <ewoksys/syscall.h>
#include <ewoksys/proto.h>

#ifdef __cplusplus
extern "C" {
#endif

static void _timer_handle(uint32_t intr, ewokos_addr_t data) {
    (void)intr;
    timer_handle_t handle = (timer_handle_t)data;
    if(handle != NULL)
        handle();
    syscall0(SYS_INTR_END);
}

uint32_t timer_set(uint32_t usec, timer_handle_t handle) {
    uint32_t id = 0;
    proto_t in, out;
    PF->init(&in);
    PF->init(&out);
    PF->addi(&in, usec);
    PF->adda(&in, (ewokos_addr_t)_timer_handle);
    PF->adda(&in, (ewokos_addr_t)handle);
    if(dev_cntl("/dev/timer", TIMER_SET, &in, &out) == 0)
        id = (uint32_t)proto_read_int(&out);
    PF->clear(&in);
    PF->clear(&out);
    return id;
}

void timer_remove(uint32_t id) {
    proto_t in;
    PF->init(&in)->addi(&in, id);
    dev_cntl("/dev/timer", TIMER_REMOVE, &in, NULL);
    PF->clear(&in);
}

#ifdef __cplusplus
}
#endif
