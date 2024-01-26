#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

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

int main (int argc, char **argv) {
  setbuf(stdout, NULL);
  HIDE_CURSOR();
  const char* s = "May the Source be with you";

  while (1) {
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

    sleep(2);
  }

  CLEAR();
  SHOW_CURSOR();
  RESET();
  return 0;
}