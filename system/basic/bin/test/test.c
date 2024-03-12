#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

static pthread_mutex_t mutex = {0};
static uint32_t flag  = 0;
static int err_cnt = 0;
static int cnt = 0 ;
FILE *fp;
static int IO_DEBUG;

void * read_thread(void *){
  char buf[32];


  // while(1){
    sprintf(buf, "test %d\n", cnt++);

    pthread_mutex_lock(&mutex);
    fseek(fp, 0, SEEK_SET);
    fwrite(buf, 1, sizeof(buf), fp);
    printf("write: %s", buf);
    pthread_mutex_unlock(&mutex); 
    // usleep(1);
  // }
}

void * write_thread(void *){
  char buf[32]; 
  // while(1){
    memset(buf, 0, sizeof(buf));
    pthread_mutex_lock(&mutex);
    fseek(fp, 0, SEEK_SET);
    int len = fread(buf, 1, sizeof(buf), fp);
    if(len){
      printf("read : %s", buf);
    }else{
      printf("read error:%d\n", len); 
    }
    pthread_mutex_unlock(&mutex); 
    usleep(1);
  // }
}


int main (int argc, char **argv) {
  IO_DEBUG = 1; 
  printf("read write test\n");
  pthread_mutex_init(&mutex, NULL);
  fp = fopen("/tmp/a", "w+b");

  while(1) {
    pthread_create(NULL, NULL, read_thread, NULL);
    pthread_create(NULL, NULL, write_thread, NULL);
    usleep(10000);
  }
  return 0;
}