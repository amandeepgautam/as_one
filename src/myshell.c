/* RCS information: $Id: myshell.c,v 1.2 2006/04/05 22:46:33 elm Exp $ */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

extern char **getline_custom();

int main() {
  int i;
  char **args; 

  while(1) {
    args = getline_custom();
    for(i = 0; args[i] != NULL; i++) {
      printf("Argument %d: %s\n", i, args[i]);
    }
  }
}
