#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <fcntl.h>

#define MAXSIZE 100

extern char **getline_custom();

struct jobSet;
struct job;
struct childProc;
struct redirection;

typedef struct jobSet jobSet; 
typedef struct job job;
typedef struct childProc childProc;
typedef struct redirection redirection;

/* Structure to hold the parameters of sets of job*/
struct jobSet {
    job *head;                  /* head pointer to the linked list*/
    job *fg;                    /* forgrounf job */
};

enum redirectType {
    READ,
    OVERWRITE,
    APPEND
};

struct redirection {
    enum redirectType type;
    int fd;                    /* fd being redirected */
    char *file_name;           /* file to which this descriptor is redirected */
};

/* Store information about the job*/
struct job {
    int jobId;                  /* identifier for the job */
    int numProg;                /* number of programs in job */
    int runningProgs;           /* running at this moment */
    int stoppedChild;           /* programs that stopped by some other process */
    childProc *child;           /* array of child programs in job */
    char *jobName;              /* name of the job */
    char *cmdBuf;               /* buffer various argv's point to */
    pid_t pgrp;                 /* process group id for the job */
    job *next;                  /* next job to execute */
    short isBg;
};

/* Store information about child processes in a job */
struct childProc {
    pid_t pid;                  /* id of the running process */
    char **argv;                /* name of command and argument list */
    int redirectCount;          /* number of redirections */
    redirection *redirectTo;    /* list to redirected numbers */
    int isStopped;
};

/* Run a particular command */
int run(job *toRun, jobSet *list, short isBg) {
    job *alias;
    int in, out, customfds;
    int fds[2], controlfds[2];                 /* fd[0] for reading */
    if ( toRun->numProg ) {
        if(!strcmp(toRun->child[0].argv[0], "exit")) {
            exit(0);            /*exit the main terminal*/
        }
        
        int i;
        in=0;                   /* STDIN */
        out=1;                  /* STDOUT */
        printf ("number of programs: %d\n", toRun->numProg);
		fflush(stdout);
        for(i=0; i<toRun->numProg; ++i) {
            printf ("loop times: %d\n", toRun->numProg);
            fflush(stdout);
            /* Set the file descriptors for each process */
            if (i+1!=toRun->numProg) {
                pipe(fds);
                out=fds[1];      /* for writing */
            } else {
                out=1;           /* assign to std output the last process */
            }

            pipe(controlfds);
            /* setup piping */
            if ( !(toRun->child[i].pid = fork()) ) {
				printf("input from: %d output to %d\n", in, out);
                /* SIGTTOU: send if child process attempts to write to tty */
                signal(SIGTTOU, SIG_IGN);
                /* close write side of the child */
                close(controlfds[1]);
                /* Wait till the process is not moved to proper group */
                int ignore;
                read(controlfds[0], &ignore , 1);
                close(controlfds[0]);

                if(out!=1) {
                    dup2(out, 1);
                    close(out);
                }
                if(in!=0) {
                    dup2(in, 1);
                    close(in);
                }

                /* overwrite redirection, if there */
                int j, mode;
                for(j=0; j<toRun->child[i].redirectCount; ++j) {
                    switch (toRun->child[i].redirectTo[j].type) {
                        case READ:
                            mode = O_RDONLY;
                            break;
                        case APPEND:
                            mode = O_CREAT|O_RDWR|O_APPEND;
                            break;
                        case OVERWRITE:
                            mode = O_CREAT|O_RDWR|O_TRUNC;
                            break;
                    }
                    customfds = open(toRun->child[i].redirectTo[j].file_name, mode, 0666);
                    if(customfds<0) {
                        fprintf(stderr, "Error opening file %s: %s\n", 
                                 toRun->child[i].redirectTo[j].file_name, strerror(errno));
                    } else {
                        dup2(customfds, toRun->child[i].redirectTo[j].fd);
                        /* Close the old file descriptor */ 
                        close(customfds);
                    }
                }

                execvp(toRun->child[i].argv[0], toRun->child[i].argv);
                //printf("from child\n");
                //fflush(stdout);
                fprintf(stderr, "The call to %s failed: %s", 
                         toRun->child[i].argv[0], strerror(errno));
                exit(1);
            }
            //printf("the process id is: %d\n",toRun->child[i].pid);
            /* add process to the group */
            setpgid(toRun->child[i].pid, toRun->child[0].pid);
            close(controlfds[0]);
            close(controlfds[1]);
            
            /* Close file descriptors in parent */
            if(in!=0) {
                close(in);
            }
            if(out!=1) {
                close(out);
            }
            in=fds[1];
        }
        toRun->pgrp = toRun->child[0].pid;
        //printf("process group:%d\n", toRun->pgrp);
        toRun->runningProgs = toRun->numProg;
        //printf("running programs:%d\n", toRun->runningProgs);
        if(isBg) {
            printf("Process running as background: %d", toRun->pgrp);
        } else {
            //printf ("here is \n");
            if(tcsetpgrp(0, toRun->pgrp)!=0) {
				perror("Could not be set as foreground\n");
			}
        }
    } else {
        printf("The job submitted is empty\n");
    }
    return 0;
}

void removeJob(jobSet *job_info, job *toRemove) {
    
}

void checkJobs( jobSet *list ) {
    while (1) {
        
    }
}

int parse(char** args, int job_id, jobSet **job_info, job **job_elem) {
    
    int i=-1,x=-1,j=job_id,t;
    while(i==-1 || args[i]!=NULL) {
        
        int noOfProg=1;
        t = i+1;
        x = i+1;
        
        // Check for invalid syntax
        if (t!=0 &&  (*args[t-1] == '&' || *args[t-1] == ';') && *args[t] == '|') {
            printf("syntax error near '|'\n");
            break;
        }
        
        if(args[t]==NULL)
            break;
        
        *job_elem = (job*) malloc(sizeof(job));
        childProc *cprocess_list = (childProc *) malloc(sizeof(childProc)*MAXSIZE); 
        if(j==1)
            (*job_info)->head = *job_elem;

        (*job_elem)->jobId = j;
        (*job_elem)->runningProgs = 0;
        (*job_elem)->next = NULL;	
        (*job_elem)->child = cprocess_list;
        (*job_info)->fg = *job_elem;
        
        
        if((*job_info)->head != *job_elem) {
            //printf("setting next job elem\n");
            job* temp = (*job_info)->head;
            while(temp->next != NULL)
                temp = temp->next;
            temp->next = *job_elem;
        }
        
        int k=0,in=-1,out=-1;
        int inc_k = 1;
        short bg_exist = 0;
        for(i = t; args[i] != NULL && *args[i] != ';' ; i++) {
            if(inc_k) {
                cprocess_list[k].pid = -1;
                cprocess_list[k].argv = NULL;
                cprocess_list[k].redirectCount = 0;
                cprocess_list[k].redirectTo = (redirection *) malloc(sizeof(redirection)*2);;
                cprocess_list[k].isStopped = 0;
                inc_k = 0;
            }
            
            if(*args[i] == '<')  {
                if(in!=-1) {
                    printf("syntax error: more than one input indirection\n");
                    continue;
                }	
                else {
                    in = i++;
                    char* temp = (char*)malloc(strlen(args[i])+1);
                    memcpy(temp,&args[i],strlen(args[i])+1);
                    cprocess_list[k].redirectCount++;
                    cprocess_list[k].redirectTo[0].type = READ;
                    cprocess_list[k].redirectTo[0].fd=0;
                    cprocess_list[k].redirectTo[0].file_name=temp;
                    //cprocess_list[k].inredir=temp;
                    
                }	
            }
            
            else if(*args[i]=='>') {
                if(out!=-1) {
                    printf("syntax error: more than one output indirection\n");
                    continue;
                }	
                else {	
                    out = i++;
                    char* temp = (char*)malloc(strlen(args[i])+1);
                    memcpy(temp,&args[i],strlen(args[i])+1);
                    cprocess_list[k].redirectCount++;
                    cprocess_list[k].redirectTo[0].type = OVERWRITE;
                    cprocess_list[k].redirectTo[0].fd=1;
                    cprocess_list[k].redirectTo[0].file_name=temp;
                }
            }
            else if(*args[i] == '&' ) {
                    bg_exist = 1;
                    break;
            }
            else if(*args[i] == '|' ) {
                noOfProg++;
                int l;
                    
                if(k<MAXSIZE) {				
                    char** temp = (char**)malloc(sizeof(char*) * (i-x+1));						
                    for(l = 0; l < i-x; l++) {
                        temp[l] = (char*) malloc(strlen(args[x+l])+1);											
                        memcpy(&temp[l],&args[x+l],strlen(args[x+l])+1);						
                    }
                    temp[i-x] = (char*) malloc(sizeof(char*));
                    temp[i-x] = NULL;
                    cprocess_list[k].argv = temp;
                    k++;
                    inc_k=1;
                }
                else
                    printf("size of child process list exceeded\n");					
                x=i+1;	
            }

            if(args[i+1] == NULL || *args[i+1]==';') {			
                int l;
                if(k<MAXSIZE) {					
                    char** temp = (char**)malloc(sizeof(char*) * (i-x+2));
                    
                    for(l = 0; l < i-x+1; l++) {
                        temp[l] = (char*) malloc(strlen(args[x+l])+1);											
                        memcpy(&temp[l],&args[x+l],strlen(args[x+l])+1);						
                    }
                    temp[i-x+1] = (char*) malloc(sizeof(char*));
                    temp[i-x+1] = NULL;
                    cprocess_list[k].argv = temp;
                    k++;
                }
                else
                    printf("size of child process list exceeded");					
                x=i+1;	
            }		
        }

        if(bg_exist == -1) {
            printf("'&' allowed only at the end of job\n");
            continue;
        }
        (*job_elem)->isBg = bg_exist;
        (*job_elem)->numProg = noOfProg;
        j++;
    }
    return j-1;//job_id of final job created if single job created then same as passed
}

int main(void) {
    int status;
    char **args;
    jobSet* job_info = (jobSet*) malloc(sizeof(jobSet));
    job_info->fg = NULL;
    job_info->head = NULL;
    job* job_elem;
    int job_id = 1;
    while(1) {
        printf("start\n");
        if(!job_info->fg) {
            args = getline_custom();
            int newjob_id = parse(args,job_id,&job_info,&job_elem);
            int job_count = newjob_id-job_id+1;
            job_id=newjob_id++;
            
            int i;
            job* toRun = job_elem;
            //printf("going top execute job\n");
            for(i=0;i<job_count;i++) {
                if(toRun!=NULL) {
                    //printf("job is %d pointer %d\n",i,toRun);
                    int a = run(toRun, job_info, toRun->isBg);
                    printf("almost done\n");
					fflush(stdout);
                    //printf("a is %d",a);
                    toRun = toRun->next;
                }
                else
                    printf("dispute b\w jobcount and job to run\n");
            }            
            
        } else {
            /* Wait for process in fg */
            int g=0;
            while(!(job_info->fg->child[g].pid) ||
                    job_info->fg->child[g].isStopped) {
                ++g;
            }
            printf(">??????g is :%d\n", g);
            /* wait for some child process if it is still running */
            waitpid(job_info->fg->child[g].pid, &status, WUNTRACED);
            printf("wait over\n");
            if( WIFSIGNALED(status) && 
                    (WTERMSIG(status) != SIGINT) ) {
                printf ("%s\n", strsignal(status));
                printf("in here\n");
                fflush(stdout);
            } else {
				printf("in else\n");
                fflush(stdout);
			}
            
            if(WIFEXITED(status) || 
                    WIFSIGNALED(status)) {
                printf("in second if\n");
                fflush(stdout);
                /* child exited due to some reasons */
                --(job_info->fg->runningProgs);
                job_info->fg->child[g].pid = 0;
                
                if(!job_info->fg->runningProgs) {
                    removeJob(job_info, job_info->fg);
                    job_info->fg=NULL;
					//printf("done: %d\n", getpid());
                }
            } else {
				printf("in second else\n");
                fflush(stdout);
                /* if the child was stopped */
                job_info->fg->stoppedChild++;
                job_info->fg->child[g].isStopped = 1;
                if (job_info->fg->stoppedChild==job_info->fg->numProg) {
                    printf ("Job Stopped: %d", job_info->fg->jobId);
                    job_info->fg=NULL;
                }
            }
			printf("before giving command to shell\n" );
            /* move shell to foreground if all processes end */
            if(!job_info->fg) {
				printf("in here\n" );
                if(tcsetpgrp(0, getpid())) {
                    printf("command given to shell\n" );
                    fflush(stdout);
                    perror("Could not move shell to foreground\n");
                } else {
					printf("command not given to shell\n" );
					fflush(stdout);
				}
                
            } else {
			    printf("command not given to shell\n" );
			}
        }
    }
}

//stoppedchild ko set karana hai.
//isstopped ko set karwana hai arora se 0 par.
//fg ko null karana hai agar ye process background hai toe.
