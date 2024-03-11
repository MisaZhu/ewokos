#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t mutex = {0};
static uint32_t flag  = 0;
static int err_cnt = 0;

static void delay(void){
    volatile uint64_t cnt = 10;
    while(cnt){
      cnt--;
      klog(".");
    }
    usleep(1);
}

static bool stop = false;
static void* loop(void* p) {
  int i = 0;
  int tid = pthread_self();
  while(i++ < 100) {
    pthread_mutex_lock(&mutex);
    klog("child %d ", i);
    flag = 0xaaaaaaaa;
    delay();
    if(flag != 0xaaaaaaaa){
      klog("child error %08x\n", flag);
      err_cnt++;
    }
    klog("child done %d\n", i);
    pthread_mutex_unlock(&mutex);
  }
  stop = true;
  return NULL;
}

int main (int argc, char **argv) {
  stop = false;
  pthread_mutex_init(&mutex, NULL);
  pthread_create(NULL, NULL, loop, NULL);
  int tid = pthread_self();
  while(!stop) {
    pthread_mutex_lock(&mutex);
    klog("father ");
    flag = 0x55555555;
    delay();
    if(flag != 0x55555555){
      klog("father error %08x\n", flag);
      err_cnt++;
    }
    klog("father done\n");
    pthread_mutex_unlock(&mutex);
    usleep(1);
  }
  printf("%d\n", err_cnt);
  return 0;
}