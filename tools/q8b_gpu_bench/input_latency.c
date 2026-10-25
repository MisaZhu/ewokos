/* GUI integration check: synthetic mouse IPC through the real X server. */
#include <ewoksys/kernel_tic.h>
#include <ewoksys/klog.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/proto.h>
#include <x/xcntl.h>
#include <x/xevent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint64_t now_us(void) {
    uint64_t ns=0;
    kernel_tic_nsec(&ns);
    return ns/1000u;
}

int main(int argc,char **argv) {
    unsigned seconds=argc>1?(unsigned)atoi(argv[1]):60;
    if (!seconds || seconds>3600) return 1;
    int pid=dev_get_pid("/dev/x");
    if (pid<0) return 1;
    uint64_t begin=now_us(),report=begin,total=0,max=0,period_max=0;
    unsigned count=0,over=0,errors=0;
    klog("gpu-input begin seconds=%u xserver=%d\n",seconds,pid);
    while (now_us()-begin<(uint64_t)seconds*1000000u) {
        xevent_t ev;
        memset(&ev,0,sizeof(ev));
        ev.type=XEVT_MOUSE; ev.state=MOUSE_STATE_MOVE;
        ev.value.mouse.relative=1;
        ev.value.mouse.rx=(count%80<40)?3:-3;
        proto_t in;
        PF->init(&in)->add(&in,&ev,sizeof(ev));
        uint64_t start=now_us();
        int rc=dev_cntl_by_pid(pid,X_DCNTL_INPUT,&in,NULL);
        uint64_t duration=now_us()-start;
        PF->clear(&in);
        count++; total+=duration; over+=duration>=200000u; errors+=rc!=0;
        if (duration>max) max=duration;
        if (duration>period_max) period_max=duration;
        if (now_us()-report>=5000000u) {
            klog("gpu-input progress n=%u max_interval=%llu us over200=%u errors=%u\n",
                 count,(unsigned long long)period_max,over,errors);
            period_max=0; report=now_us();
        }
        usleep(50000);
    }
    klog("gpu-input DONE n=%u mean=%llu max=%llu us over200=%u errors=%u\n",
         count,(unsigned long long)(total/(count?count:1)),
         (unsigned long long)max,over,errors);
    return over || errors?1:0;
}
