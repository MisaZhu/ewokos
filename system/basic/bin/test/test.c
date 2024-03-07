#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static void* loop(void* p) {
  while(1) {
    void* x = malloc(10024);
    free(x);
  }
  return NULL;
}

int main (int argc, char **argv) {
  pthread_create(NULL, NULL, loop, NULL);
  uint32_t i=0;
  while(i++ < 1000) {
    void* x = malloc(10024);
    free(x);
  }
  return 0;
}