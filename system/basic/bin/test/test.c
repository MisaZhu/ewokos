#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static void* loop(void* p) {
  while(1) {
    char* x = (char*)malloc(10024);
    strcpy(x, "child");
    printf("%s\n", x);
    free(x);
  }
  return NULL;
}

int main (int argc, char **argv) {
  pthread_create(NULL, NULL, loop, NULL);
  uint32_t i=0;
  while(i++ < 1000) {
    void* x = malloc(10024);
    strcpy(x, "father");
    printf("%s\n", x);
    free(x);
  }
  return 0;
}