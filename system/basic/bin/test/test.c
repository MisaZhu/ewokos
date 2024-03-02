#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static void* loop(void* p) {
  while(p)
    proc_usleep(1000);
  return NULL;
}

int main (int argc, char **argv) {
  pthread_create(NULL, NULL, loop, 1);
  pthread_create(NULL, NULL, loop, 0);
  sleep(3);
  return 0;
}