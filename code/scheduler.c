#Natalia-Maria Ilia, 1093367 
#Theofrastos Paximadis, 1093460

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>

#define maxpathlength 100

void FCFS();
void RR();

#define NEW	0
#define STOPPED	1
#define RUNNING	2
#define EXITED	3

struct process {
	struct process *next;
	char executable[maxpathlength];
	int pid;
	int number;
	int state;
};

struct double_list {
	struct process	*first;
	struct process	*last;
	long restqueue;
};

struct double_list queue;

// Function to check if a queue is empty
bool emptyqueue(struct double_list *q) {
    return q->first == NULL;
}

void empty_doublelist(struct double_list *list){	//initialize the queue as empty
	list->first=NULL;
	list->last=NULL;
	list->restqueue=0;
}

void addnode(struct process *pr)
{
    if (emptyqueue (&queue)) queue.first = queue.last = pr;
    else {
        queue.last->next = pr;
        queue.last = pr;
        pr->next = NULL;
    }
}

struct process *dequeue ()
{
    struct process *process;

    process = queue.first;
    if (process==NULL) return NULL;

    process = queue.first;
    if (process!=NULL) {
        queue.first = process->next;
        process->next = NULL;
    }
    return process;
}

double gettime()
{
	struct timeval t;
	gettimeofday(&t, 0);
 	return (double)t.tv_sec + (double)t.tv_usec * 1.0e-6;
}

#define fcfs	0
#define rr	1 

int policy = fcfs;
int quantum = 1000;	/* ms */
struct process *currprocess;


void handler_for_child(int sign, siginfo_t *info, void *x)
{
	printf("child %d exited\n", info->si_pid);
	if (currprocess == NULL) {
		printf("Current running process==NULL\n");	
	}
	else if (currprocess->pid == info->si_pid) {
		currprocess->state = EXITED;
	}
	else {
		printf("Running %d exited %d\n", currprocess->pid, info->si_pid); 
	}
}

void FCFS(){
	struct process *process;
	int pid;
	int state;
	double t1, t2;
	t1 = gettime();

	while ((process=dequeue()) != NULL) {
		printf("Dequeue process with name %s\n", process->executable);
        	if (process->state == NEW) {
			pid = fork();
       			if (pid == -1) {
				perror("fork");
				exit(1);
        		} 
			else if (pid == 0) {
        			printf("Executing: %s\n", process->executable);
            			execl(process->executable, process->executable, NULL);
            			perror("execl");
				exit(1);
        		} 
			else {
				process->pid = pid;
				process->state = RUNNING;
				pid = waitpid(process->pid, &state, 0);
				process->state = EXITED;
				if (pid < 0) {
					printf("Waitpid is not working!");
					exit(1);
				}
       				t2 = gettime();
				printf("PID %d - CMD %s\n", pid, process->executable);
				printf("Elapsed time = %.2lf secs\n", t2-t1);
				printf("Workload time = %.2lf secs\n", t2-t1);
        		}
     		}
	}
	printf("WORKLOAD TIME: %.2lf secs\n", t2 - t1);
}

void RR()
{
	struct sigaction sa;
	struct process *process;
	int pid;
	double t1, t2;			
	t1 = gettime();
	struct timespec req, rem;

	req.tv_sec = quantum / 1000;
	req.tv_nsec = (quantum % 1000)*1000000;

	printf("tv_sec = %ld\n", req.tv_sec);
	printf("tv_nsec = %ld\n", req.tv_nsec);

	sigemptyset(&sa.sa_mask);
	sa.sa_handler = 0;
	sa.sa_flags = SA_SIGINFO | SA_NOCLDSTOP;
	sa.sa_sigaction = handler_for_child;
	sigaction (SIGCHLD,&sa,NULL);
        
	while ((process=dequeue()) != NULL) {
		printf("Dequeue process with name %s\n", process->executable);
		if (process->state == NEW) {
			pid = fork();
			if (pid == -1) {
				perror("fork");
				exit(1);
			}
			if (pid == 0) {
				printf("executing %s\n", process->executable);
				execl(process->executable, process->executable, NULL);
            			perror("execl");
				exit(1);
			}
			else {
				process->pid = pid;
				currprocess= process;
				process->state = RUNNING;
				nanosleep(&req, &rem);

				if (process->state == RUNNING) {
					kill(process->pid, SIGSTOP);
					process->state = STOPPED;
					addnode(process);
				}
				else if (process->state == EXITED) {
					t2 = gettime();
					printf("PID %d - CMD %s\n", pid, process->executable);
					printf("Elapsed time = %.2lf secs\n", t2-t1);
					printf("Workload time = %.2lf secs\n", t2-t1);
				}
			}
		}
		else if (process->state == STOPPED) {
			process->state = RUNNING;
			currprocess = process;
			kill(process->pid, SIGCONT);
			nanosleep(&req, &rem);

			if (process->state == RUNNING) {
				kill(process->pid, SIGSTOP);
				addnode(process);
				process->state = STOPPED;
			}
			else if (process->state == EXITED) {
					t2 = gettime();
					printf("PID %d - CMD %s\n", pid, process->executable);
					printf("Elapsed time = %.2lf secs\n", t2-t1);
					printf("Workload time = %.2lf secs\n", t2-t1);
			}
		}
		else if (process->state == EXITED) {
			printf("process has exited\n");
		}
		else if (process->state == RUNNING) {
			printf("This process is already running \n");
		}
		else {
			printf("Invalid process!");
			exit(1);
		}
	}
	printf("WORKLOAD TIME: %.2lf secs\n", t2 - t1);
}

int main(int argc,char **argv)
{
	FILE *file;
	char executable_two[maxpathlength];
	int pr;
	int c;
	struct process *process;
	
	if (argc == 1) {
		printf("Error in reading file!");
		exit(1);
	}
	else if (argc == 2) {
        	file = fopen(argv[1],"r");
		if (file == NULL){
			printf("Error in reading file!");
			exit(1);
		}
	}
	else if (argc > 2) {   
        	if (!strcmp(argv[1],"RR")) {
	                policy = rr;
	                quantum = atoi(argv[2]);
	                file = fopen(argv[3],"r");
			if (file == NULL){
				printf("Error in reading file!");
				exit(1);
			}
	        }      
		else if (!strcmp(argv[1],"FCFS")) {
			policy = fcfs;              
			file = fopen(argv[2],"r");
			if (file == NULL){
				printf("Error in reading file!");
				exit(1);
			}
        	}       
	       
	        else {   	
			printf("Error in reading file!");
			exit(1);
        	}
	}

	/* Read input file */
	while ((c=fscanf(file, "%s %d", executable_two, &pr))!=EOF) {		
		
		process = malloc(sizeof(struct process));
		process->next = NULL;
		strcpy(process->executable, executable_two);
		process->number = pr;
		process->pid = -1;
		process->state = NEW;
		addnode(process);
	}

	switch (policy) {
		case fcfs:
			FCFS();
			break;
		case rr:
			RR();
			break;
		default:
			printf("Error in reading file!");
			exit(1);
			break;
	}

	printf("scheduler exits\n");
	return 0;
}


