#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <ewoksys/vfs.h>
#include <ewoksys/basic_math.h>

#define CLEAR() printf ("\033[2J")
#define MOVEUP(x) printf ("\033[%dA", (x))
#define MOVEDOWN(x) printf ("\033[%dB", (x))
#define MOVELEFT(y) printf ("\033[%dD", (y))
#define MOVERIGHT(y) printf ("\033[%dC", (y))
#define MOVETO(x, y) printf ("\033[%d;%dH", (y), (x))
#define RESET_CURSOR() printf ("\033[H")
#define HIDE_CURSOR() printf ("\033[?25l")
#define SHOW_CURSOR() printf ("\033[?25h")
#define HIGHT_LIGHT() printf ("\033[7m")
#define UN_HIGHT_LIGHT() printf ("\033[27m")
#define FLASH() printf ("\033[5m")
#define RESET() printf ("\033[0m")

void saver() {
  const char* s = "May the Source be with you";
  CLEAR();

  FLASH();
  int x, y;
  for(int i=0; i< 10; i++) {
    x = random_to(80);
    y = random_to(30);
    MOVETO(x, y);
    printf("*");
  }
  RESET();

  x = random_to(80-strlen(s));
  y = random_to(30);
  MOVETO(x, y);
  printf("%s", s);
}

int main (int argc, char **argv) {
  setbuf(stdout, NULL);
  HIDE_CURSOR();

  int flags = vfs_get_flags(0);
  flags |= O_NONBLOCK;
  vfs_set_flags(0, flags);

  uint32_t counter = 0;
  while (1) {
    if((counter % 30) == 0) 
      saver();

  flags = vfs_get_flags(0);
    char c;
    if(read(0, &c, 1) == 1)
      break;
    usleep(100000);
    counter++;
  }

  CLEAR();
  SHOW_CURSOR();
  RESET();
  printf("\n");
  return 0;
}