#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
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
#define REVERSE() printf ("\033[7m")
#define RESET() printf ("\033[0m")
#define COLOR(c) printf ("\033[3%dm", (c))
#define BGCOLOR(c) printf ("\033[4%dm", (c))

enum {
  BLACK = 0,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  PURPLE,
  CYAN,
  WHITE
};

void saver() {
  const char* s[] = {
      "                               ",
      "          EWOKOS v1.0          ",
      "  May the Source be with you!  ",
      "     (press ENTER to login)    ",
      "                               ",
      NULL
    };

  RESET();
  CLEAR();

  FLASH();
  int x, y, c;
  for(int i=0; i< 20; i++) {
    x = random_to(80);
    y = random_to(30);
    c = random_to(8);
    MOVETO(x, y);
    COLOR(c);
    printf("*");
  }
  RESET();

  //BGCOLOR(YELLOW);
  //COLOR(BLACK);
  REVERSE();
  x = random_to(80-strlen(s[0]));
  y = random_to(22);
  int i = 0;
  while(true) {
    if(s[i] == NULL) 
      break;
    MOVETO(x, y+i);
    printf("%s", s[i]);
    i++;
  }
  RESET();
}

int main (int argc, char **argv) {
  setbuf(stdout, NULL);
  HIDE_CURSOR();

  int flags = fcntl(0, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(0, F_SETFL, flags);

  uint32_t counter = 0;
  while (1) {
    if((counter % 30) == 0) 
      saver();

    char c;
    if(read(0, &c, 1) == 1 && (c == '\r' || c == '\n'))
      break;
    proc_usleep(100000);
    counter++;
  }

  CLEAR();
  MOVETO(0, 0);
  SHOW_CURSOR();
  return 0;
}