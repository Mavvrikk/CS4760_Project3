#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

int shmkey = 112369;
bool signalReceived = false;
bool stillChildrenRunning = true;

typedef struct PCB
{
  int occupied;			// either true or false
  pid_t pid;			// PID of this child
  int startSeconds;		// time this was forked
  int startNano;		// time this was forked (nano seconds)
} PCB;

void helpFunction ()
{
  printf
    ("The function call should go as follows: ./oss [-h] [-n proc] [-s simu] [-t timeLimit] ");
  printf
    ("where -h displays this help function and terminates, proc is the number of processes you want to run, simu is the number of simultaneous");
  printf
    ("processes that run and timeLimit is approximately the time limit each child process runs.");
}

void incrementClock(int* shmSec, int* shmNano){
    *shmNano+=1;
    if (*shmNano >= 1000000){
    *shmSec+=1;
    *shmNano=0;
    }
}

int forker (int totaltoLaunch, int simulLimit, char* timeLimit, int* totalLaunched, PCB * processTable, int* shmSec, int*shmNano)
{
  pid_t pid;// RECREATES AN EXTRA BECAUSE TOTALLAUNCHED HAS INCREMENTED BUT FUNCTION HASNT TERMINATED 
  
    if (*totalLaunched == simulLimit || totaltoLaunch == 0)
    {
      return (totaltoLaunch);
    }
  else if (totaltoLaunch > 0)
    {
      if ((pid = fork ()) < 0) // FORK HERE
	{
	  perror ("fork");
	}
      else if (pid == 0)
	{
        char* args[]={"./worker",timeLimit,0};
        execlp(args[0],args[0],args[1],args[2]);
	}
      else if (pid > 0)
	{
  processTable[*totalLaunched].occupied = 1;
  processTable[*totalLaunched].pid = pid;
  processTable[*totalLaunched].startSeconds = *shmSec;
  processTable[*totalLaunched].startNano = *shmNano;
	    *totalLaunched += 1;
	  forker (totaltoLaunch - 1, simulLimit, timeLimit, totalLaunched,
		  processTable,shmSec,shmNano);
	}
    }
  else
    return (0);
}

bool checkifChildTerminated(int status, PCB *processTable, int size)
{
    int pid = waitpid(-1, &status, WNOHANG);
    int i = 0;
    if (pid > 0){
    for (i; i < size; i++){
        if (processTable[i].pid == pid)
            processTable[i].occupied = 0;
    }
    return true;
    }
    else if (pid == 0)
        return false;
}
void initializeStruct(struct PCB *processTable){
int i = 0;
for (i; i < 20; i++){
processTable[i].occupied = 0;
processTable[i].pid = 0;
processTable[i].startSeconds = 0;
processTable[i].startNano = 0;
}
}
void printStruct (struct PCB *processTable,int* shmSec,int* shmNano)
{
  printf("OSS PID: %d SysClock: %d SysclockNano: %d\n", getpid(),*shmSec,*shmNano);
  printf ("Process Table: \n");
  printf ("ENTRY  OCCUPIED  PID  STARTS  STARTN\n");
  int i = 0;
  for (i; i < 20; i++)
    {
      printf ("%d        %d       %d    %d        %d\n", i,
	      processTable[i].occupied, processTable[i].pid,
	      processTable[i].startSeconds, processTable[i].startNano);
    }
}

void sig_handler(int signal){
printf("\n\nSIGNAL RECEIVED, TERMINATING PROGRAM\n\n");
stillChildrenRunning = false;
signalReceived = true;
}

void sig_alarmHandler(int sigAlarm){
printf("TIMEOUT ACHIEVED. PROGRAM TERMINATING\n");
stillChildrenRunning = false;
signalReceived = true;
}

char *x = NULL;
char *y = NULL;
char *z = NULL;



int main (int argc, char **argv)
{
  int option;
  while ((option = getopt (argc, argv, "n:s:t:h")) != -1)
    {
      switch (option)
	{
	case 'h':
	  helpFunction ();
	  return 0;
	case 'n':
	  x = optarg;
	  break;
	case 's':
	  y = optarg;
	  break;
	case 't':
	  z = optarg;
	  break;
	}
    }
 
  
    //INITIALIZE ALL VARIABLES
  int totaltoLaunch = 0;	// int to hold -n arg
  int simulLimit = 0;		// int to hold -s arg
  int totalLaunched = 0;	// int to count total children launched
  totaltoLaunch = atoi (x);	// casting the argv to ints
  simulLimit = atoi (y);
  char* timeLimit = z;
  int status; 
  int exCess;
  int *shmSec;
  int *shmNano;
  PCB processTable[20];
 // bool stillChildrenRunning = true;
  bool initialLaunch = false;
 
 signal(SIGINT, sig_handler);
 signal(SIGALRM, sig_alarmHandler);
 alarm(60);

  int shmid = shmget (shmkey, 2 * sizeof (int), 0777 | IPC_CREAT);	// create shared memory
  if (shmid == -1)
    {
      printf ("ERROR IN SHMGET\n");
      exit (0);
    }
    
    //Clock
  shmSec = (int *) (shmat (shmid, 0, 0)); // create clock variables
  shmNano = shmSec + 1;
  *shmSec = 0; // initialize clock to zero
  *shmNano = 0;
    
 // initialize struct to 0
 initializeStruct(processTable);
   while(stillChildrenRunning){
  // FORK CHILDREN 
    incrementClock(shmSec,shmNano);
    if (*shmNano == 500000){
    printStruct (processTable, shmSec, shmNano);
    }
    if(initialLaunch == false){
        exCess = forker (totaltoLaunch, simulLimit, timeLimit, &totalLaunched, processTable, shmSec, shmNano);
        initialLaunch = true;
    }
    bool childHasTerminated = false;
    childHasTerminated = checkifChildTerminated(status, processTable,20);
    if(childHasTerminated == true){
        if (exCess > 0){
            forker (1, 1, timeLimit, &totalLaunched, processTable, shmSec,shmNano);
            exCess--;
        }
        totaltoLaunch--;
    }
    if (totaltoLaunch == 0)
            stillChildrenRunning = false;
    }
if (signalReceived == true){ //KILL ALL CHILDREN IF SIGNAL
int i = 0;
pid_t childPid;
for (i;i<20;i++){
	if (processTable[i].pid > 0 && processTable[i].occupied == 1){ // IF there is a process who is still runnning
		childPid = processTable[i].pid;
		kill(childPid, SIGKILL);
	}
}
}

  //DETACH SHARED MEMORY
  shmdt (shmSec);
  shmctl (shmid, IPC_RMID, NULL);
  return (0);
}