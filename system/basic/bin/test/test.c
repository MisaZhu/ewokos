#include <stdio.h>
#include <unistd.h>

int main (int argc, char **argv) {

  for(int i=0; i<argc; i++) {
    printf("%d: 0x%x: %s\n", i, argv[i], argv[i]);
  }





  int c = 0;
  while (c != -1) {
    c = getopt (argc, argv, "abc:d");
    if(c == -1)
      break;

    switch (c) {
      case 'a':
        printf("a\n");
        break;
      case 'b':
        printf("b\n");
        break;
      case 'c':
        printf("c=%s\n", optarg);
        break;
      case 'd':
        printf("d\n");
        break;
      case '?':
        return -1;
      default:
        c = -1;
        break;
    }
  }

  for (int i=optind; i < argc; i++)
    printf ("Non-option argument %s\n", argv[i]);

  for(int i=0; i<argc; i++) {
    printf("%d: 0x%x: %s\n", i, argv[i], argv[i]);
  }
  return 0;
}