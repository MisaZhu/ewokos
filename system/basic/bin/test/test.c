#include <stdio.h>
#include <unistd.h>

#define CLEAR() printf ("\033[2J")
#define MOVEUP(x) printf ("\033[%dA", (x))
#define MOVEDOWN(x) printf ("\033[%dB", (x))
#define MOVELEFT(y) printf ("\033[%dD", (y))
#define MOVERIGHT(y) printf ("\033[%dC", (y))
#define MOVETO(x, y) printf ("\033[%d;%dH", (x), (y))
#define RESET_CURSOR() printf ("\033[H")
#define HIDE_CURSOR() printf ("\033[?25l")
#define SHOW_CURSOR() printf ("\033[?25h")
#define HIGHT_LIGHT() printf ("\033[7m")
#define UN_HIGHT_LIGHT() printf ("\033[27m")

int main (int argc, char **argv) {
  setbuf(stdout, NULL);
  printf ("\033[2J\033[0;0H");
  printf ("\033[31mThe color,%s!\033[1m\n", "haha");
  printf ("\033[31mThe color,%s!\033[0;4m\n", "haha");
  printf ("\033[32mThe color,%s!\033[5m\n", "haha");
  printf ("\033[34mThe color,%s!\033[7m\n", "haha");
  printf ("\033[35mThe color,%s!\033[0;8m\n", "haha");
  printf ("\033[42;36mThe color,%s!\033[0m\n", "haha");
  printf ("\033[47;30mThe color,%s!\n", "haha");
  sleep (2);

  printf ("\033[30;46m\033[9B%s!", "down 9");
  sleep (2);

  printf ("\033[5A%s!", "up 5");
  sleep (2);

  printf ("\033[19C%s!", "right 24");
  sleep (2);

  printf ("\033[9B%s!", "down 9");
  sleep (2);

  printf ("\033[20D%s!", "left 20");
  sleep (2);

  printf ("\033[50;20H%s!", "move to y:50,x 20");
  sleep (2);

  printf ("\033[2J%s!\033[0m", "clear screen");
  sleep (2);

  printf ("%s!\033[?25l\n", "\nhide cursor");
  sleep (2);

  printf ("%s!\033[?25h", "show cursor");
  sleep (2);

  printf ("\033[2Jdone!\n");
  return 0;
}