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
    job *head; /* head pointer to the linked list*/
    job *fg; /* forgrounf job */
};

enum redirectType {
    READ,
    OVERWRITE,
    APPEND
};

struct redirection {
    enum redirectType type;
    int fd; /* fd being redirected */
    char *file_name; /* file to which this descriptor is redirected */
};

/* Store information about the job*/
struct job {
    int jobId; /* identifier for the job */
    int numProg; /* number of programs in job */
    int runningProgs; /* running at this moment */
    int stoppedChild; /* programs that stopped by some other process */
    childProc *child; /* array of child programs in job */
    char *jobName; /* name of the job */
    char *cmdBuf; /* buffer various argv's point to */
    pid_t pgrp; /* process group id for the job */
    job *next; /* next job to execute */
    short isBg;
};

/* Store information about child processes in a job */
struct childProc {
    pid_t pid; /* id of the running process */
    char **argv; /* name of command and argument list */
    int redirectCount; /* number of redirections */
    redirection *redirectTo; /* list to redirected numbers */
    int isStopped;
};

/* Run a particular command */
int run(job *toRun, jobSet *list, short isBg) {
    job *alias;
    int in, out, customfds;
    int fds[2], controlfds[2]; /* fd[0] for reading */
    
    
    
    if ( toRun->numProg ) {
        if(!strcmp(toRun->child[0].argv[0], "exit")) {
            exit(0); /*exit the main terminal*/
        }
        
        int i;
        in=0; /* STDIN */
        out=1; /* STDOUT */
        
        for(i=0; i<toRun->numProg; ++i) {
            /* Set the file descriptors for each process */
            if (i+1!=toRun->numProg) {
                pipe(fds);
                out=fds[1]; /* for writing */
            } else {
                out=1; /* assign to std output the last process */
            }

            pipe(controlfds);
            /* setup piping */
            if ( !(toRun->child[i].pid = fork()) ) {
                /* SIGTTOU: send if child process attempts to write to tty */
                signal(SIGTTOU, SIG_DFL);
                /* close write side of the child */
                close(controlfds[1]);
                /* Wait till the process is not moved to proper group */
                char ignore;
                read(controlfds[0], &ignore , 1);
                close(controlfds[0]);

                if(out!=1) {
                    dup2(out, 1);
                    close(out);
                }
                if(in!=0) {
                    dup2(in, 0);
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
                            mode = O_CREAT|O_WRONLY|O_TRUNC;
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
            in=fds[0];
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
   // printf("job remove called\n");
    if(job_info && toRemove)
    {
		if(job_info->fg == toRemove) {
			job_info->fg = NULL;
		}

		job* prev = job_info->head;
			// printf("head%d\n",prev);
		// head is being removed
		if(job_info->head == toRemove) {
			job_info->head = job_info->head->next;
			// printf("deleting job%d\n",toRemove);
			free(toRemove);
			return;
		} else {
			while(prev->next!=NULL && prev->next != toRemove) {
				prev=prev->next;
			}
			if(prev->next == NULL) {
				printf("The job you are trying to delete is not found");
				return;
			}
			if(prev->next == toRemove) {
				prev->next = prev->next->next;
				// printf("deleting job%d\n",toRemove);
				free(toRemove);
			}
		}
	} else {
		printf("one of the passed variable is NULL cant delete");
	}
}

void findPid(int pid, job** J, int* index, jobSet* job_info) {
	job* currJ = job_info->head;
	while (currJ != NULL) {
        int i;
		for (i=0; i < currJ->numProg; i++) {
			if (currJ->child[i].pid == pid) {
				*J = currJ;
				*index = i;
				return;
			}
		}
		currJ = currJ->next;
	}
}

void checkJobs( jobSet *list ) {
    int status, index;
    pid_t pid;
    job * task;
    /* Catch all the processes that have exited */
    while( (pid = waitpid(-1, &status, WNOHANG|WUNTRACED)>0)) {
        findPid(pid, &task, &index, list);
        if(WIFEXITED(status) && WEXITSTATUS(status)) {
			task->runningProgs--;	/*Decrement the number of running programs*/
			task->child[index].pid=0;
			if(!task->runningProgs) {
                printf("Done. Job id: %d\n", task->jobId);
                removeJob(list, task);
            }
		} else {
            task->stoppedChild++;
            task->child[index].isStopped=1;
            if(task->stoppedChild==task->numProg) {
                printf("Stopped. Job id: %d\n", task->jobId);
            }
		}
    }
}

int parse(char** args, int job_id, jobSet **job_info, job **job_elem) {
    int i=-1,x=-1,j=job_id,t;
    while(i==-1 || args[i]!=NULL) {
        
        int noOfProg=0;
        t = i+1;
        x = i+1;
        
        // Check for invalid syntax
        if (t!=0 && (*args[t-1] == '&' || *args[t-1] == ';') && *args[t] == '|') {
            printf("syntax error near '|'\n");
            break;
        }
        
        // check empty command
        if(args[t]==NULL)
            break;
        
        // dynamically creating job object and child process object
        *job_elem = (job*) malloc(sizeof(job));
        childProc *cprocess_list = (childProc *) malloc(sizeof(childProc)*MAXSIZE);
        
        // first job created, intialising head pointer
        if((*job_info)->head==NULL)
            (*job_info)->head = *job_elem;

// setting data members of job object
        (*job_elem)->jobId = j;
        (*job_elem)->runningProgs = 0;
        (*job_elem)->next = NULL;	
        (*job_elem)->child = cprocess_list;
        (*job_info)->fg = *job_elem;
        
        // maintaining job list
        if((*job_info)->head != *job_elem) {
            job* temp = (*job_info)->head;
            while(temp->next != NULL)
                temp = temp->next;
            temp->next = *job_elem;
        }
        
        int k=0,in=-1,out=-1;
        int inc_k = 1;
        short bg_exist = 0;
        
        // for loop to create all process in a given job
        for(i = t; args[i] != NULL && *args[i] != ';' ; i++) {
            if(inc_k) {
                cprocess_list[k].pid = -1;
                cprocess_list[k].argv = NULL;
                cprocess_list[k].redirectCount = 0;
                cprocess_list[k].redirectTo = (redirection *) malloc(sizeof(redirection)*2);;
                cprocess_list[k].isStopped = 0;
                inc_k = 0;
            }
            
            // background process
            if(*args[i] == '&' ) {
                    bg_exist = 1;
                    break;
            }
            
            // piping to create new process
            if(*args[i] == '|' || *args[i] =='>' || *args[i] == '<') {
				int help = i;	
				if(x!=i) {
					noOfProg++;
					int l;

					if(k<MAXSIZE) {	
						char** temp = (char**)malloc(sizeof(char*) * (i-x+1));	
						for(l = 0; l < i-x; l++) {
							temp[l] = (char*) malloc(strlen(args[x+l])+1);	
							memcpy(temp[l],args[x+l],strlen(args[x+l])+1);	
						}
						temp[i-x] = (char*) malloc(sizeof(char*));
						temp[i-x] = NULL;
						cprocess_list[k].argv = temp;
						inc_k=1;
					} else
						printf("size of child process list exceeded\n");
				}
				// indirection operator
				if(*args[i] == '<') {
					if(in!=-1) {
						printf("syntax error: more than one input indirection\n");
						continue;
					} else {
						in = i++;
						char* temp = (char*)malloc(strlen(args[i])+1);
						memcpy(temp,args[i],strlen(args[i])+1);
						cprocess_list[k].redirectCount++;
						cprocess_list[k].redirectTo[0].type = READ;
						cprocess_list[k].redirectTo[0].fd=0;
						cprocess_list[k].redirectTo[0].file_name=temp;
					}	
				} else if(*args[i]=='>') {// outdirection operator

					if(args[i+1]!=NULL && *args[i+1]=='>') {
						i++;
						cprocess_list[k].redirectTo[0].type = APPEND;
					} else {
						cprocess_list[k].redirectTo[0].type = OVERWRITE;
					}	

					out = i++;
					char* temp = (char*)malloc(strlen(args[i])+1);
					memcpy(temp,args[i],strlen(args[i])+1);

					cprocess_list[k].redirectCount++;
					cprocess_list[k].redirectTo[0].fd=1;
					cprocess_list[k].redirectTo[0].file_name=temp;

				}
				
				if(*args[help] == '|')
					k++;
                x=i+1;	
            } else if(args[i+1] == NULL || *args[i+1]==';') {	// end case
                noOfProg++;
                int l;
                if(k<MAXSIZE) {	
                    char** temp = (char**)malloc(sizeof(char*) * (i-x+2));
                    
                    for(l = 0; l < i-x+1; l++) {
                        temp[l] = (char*) malloc(strlen(args[x+l])+1);	
                        memcpy(temp[l],args[x+l],strlen(args[x+l])+1);	
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
        if(!job_info->fg) {
            args = getline_custom();
            int newjob_id = parse(args,job_id,&job_info,&job_elem);

            int job_count = newjob_id-job_id+1;
            
            if(job_count==0) {
				continue;
			}
            job_id=newjob_id++;
            
            int i;
            job* toRun = job_elem;
            for(i=0;i<job_count;i++) {
                if(toRun!=NULL) {
                    //printf("job is %d pointer %d\n",i,toRun);
                    int a = run(toRun, job_info, toRun->isBg);
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
            /* wait for some child process if it is still running */
            waitpid(job_info->fg->child[g].pid, &status, WUNTRACED);
            if( WIFSIGNALED(status) &&
                    (WTERMSIG(status) != SIGINT) ) {
                printf ("%s\n", strsignal(status));
            }
            
            if(WIFEXITED(status) ||
                    WIFSIGNALED(status)) {
                /* child exited due to some reasons */
                --(job_info->fg->runningProgs);
                job_info->fg->child[g].pid = 0;
                
                if(!job_info->fg->runningProgs) {
                    removeJob(job_info, job_info->fg);
                    job_info->fg=NULL;
				//printf("done: %d\n", getpid());
                }
            } else {
                /* if the child was stopped */
                job_info->fg->stoppedChild++;
                job_info->fg->child[g].isStopped = 1;
                if (job_info->fg->stoppedChild==job_info->fg->numProg) {
                    job_info->fg=NULL;
                }
            }
            /* move shell to foreground if all processes end */
            if(!job_info->fg) {
				signal(SIGTTOU, SIG_IGN);
                if(tcsetpgrp(0, getpid())) {
                    perror("Could not move shell to foreground\n");
                }
                signal(SIGTTOU, SIG_DFL);

            }
        }

    }
}
