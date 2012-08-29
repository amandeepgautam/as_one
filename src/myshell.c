/* RCS information: $Id: myshell.c,v 1.2 2006/04/05 22:46:33 elm Exp $ */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

extern char **getline_custom();

struct jobSet;
struct job;
struct childProc;

typedef struct jobSet jobSet; 
typedef struct job job;
typedef struct childProc childProc;

/* Structure to hold the parameters of sets of job*/
struct jobSet {
    jobSet *head;               /* head pointer to the linked list*/
    job *fg;                    /* forgrounf job */
};

/* Store information about the job*/
struct job {
    int jobId;                  /* identifier for the job */
    int numProg;                /* number of programs in job */
    int runningProgs;           /* running at this moment */
    childProc *child;           /* array of child programs in job */
    char *jobName;              /* name of the job */
    char *cmdBuf;               /* buffer various argv's point to */
    pid_t pgrp;                 /* process group id for the job */
    job *next;                  /* next job to execute */
};

/* Store information about child processes in a job */
struct childProc {
    pid_t pid;                  /* id of the running process */
    char **argv;                /* name of command and argument list */
};


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
