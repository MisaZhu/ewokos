#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char* test = NULL;
    test[0] = 'a'; //test core dump
    return 0;
}