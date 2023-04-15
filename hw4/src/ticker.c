#include <stdlib.h>
#include "ticker.h"

#include <debug.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

typedef struct watcher {
    int tableIndex;
    int wtype;
    int pid;
    int fdin;
    int fdout;
    int trace;
    int terminated;
    char* channel;
} WATCHER;

volatile sig_atomic_t user_input_detected = 0;
sigset_t sigSuspendMask;
int firstHandle;
int quitDetected = 0;
char* userInputLine;
extern char* bufferUserInput();
volatile sig_atomic_t dataAvailable = 0;
extern int numBytesReadGlobal;

void sigint_handler(int s){

    exit(0);
}

void sigio_handler(int s, siginfo_t* info,void* context){
    // char* msg = "SIG IO Signal Recieved\n";
    // write(STDOUT_FILENO,msg,24);
    // debug("SIGIO signal received in handler");
    int oldErrno = errno;
    int fd = info->si_fd;
    // // criticla section - block out signal s

    if (fd == STDIN_FILENO) { // user input
        user_input_detected = 1;
        // printf("User input detected!\n");
    } else{
        printf("handling other SIGIO");
    }
    errno = oldErrno; // restore errno
}

void sigchild_handler(int s){
    // char* msg = "SIG CHILD Signal Recieved";
    // write(STDOUT_FILENO,msg,sizeof(msg));
    // waitpid() -> get watcher pid, close fd, free slots in watcher table and other memory

}

int ticker(void) {
    // killall -s SIGKILL -r tickers

    /** initialization **/
    // install handler for sigint
    struct sigaction sigintSa={0};
    sigintSa.sa_handler = sigint_handler;
    if( sigaction(SIGINT,&sigintSa,NULL) == -1){
        perror("sigaction");
        return 1;
    }

    // install handler for sigchild
    struct sigaction sigchildSa = {0};
    sigchildSa.sa_handler = sigchild_handler;
    if (sigaction(SIGCHLD,&sigchildSa,NULL) == -1){
        perror("sigchild");
        return 1;
    }

    // install handler for SIGIO
    struct sigaction sigioSa = {0};
    sigioSa.sa_sigaction = sigio_handler;
    sigioSa.sa_flags = SA_SIGINFO;
    if( sigaction(SIGIO,&sigioSa,NULL) == -1){
        perror("sigaction");
        return 1;
    }

    // Set up asynch nonblocking io for stdin
    if(fcntl(STDIN_FILENO,F_SETFL,O_ASYNC | O_NONBLOCK) == -1){
        perror("fcntl");
        return 1;
    }
    if(fcntl(STDIN_FILENO,F_SETOWN,getpid()) == -1 ){
        perror("fcntl");
        return 1;
    }
    if(fcntl(STDIN_FILENO,F_SETSIG,SIGIO) == -1){
        perror("fcntl");
        return 1;
    }

    // start cli watcher and add to watcher table
    WATCHER *cliWatcher = watcher_types[CLI_WATCHER_TYPE].start(&watcher_types[CLI_WATCHER_TYPE],NULL);
    sigemptyset(&sigSuspendMask);
    // check stdin if there exists input already? send sigio signal
    firstHandle = 1;
    char* userCmd = bufferUserInput();
    watcher_types[CLI_WATCHER_TYPE].recv(cliWatcher,userCmd);
    firstHandle = 0;
    if(quitDetected == 1 || numBytesReadGlobal==0){
        free(userCmd);
        return 0;
    }
    // debug("done handling first user input");

    while(1){ // loop until receipt of any kind of signal
        // body of main loop
        debug("In main loop");
        if (user_input_detected){
            userCmd = bufferUserInput();
            watcher_types[CLI_WATCHER_TYPE].recv(cliWatcher,userCmd);

            if(quitDetected==1){
                debug("quit detected in mainnnnnnnnnnnnn");
                free(userCmd);
                return 0;
            }
            // write(STDOUT_FILENO,"ticker> ",8);
            // debug("heheeeeeeeee");


            sigset_t mask;
            sigset_t oldMask;
            sigfillset(&mask);
            sigprocmask(SIG_BLOCK,&mask,&oldMask);
            // critical section
            user_input_detected = 0;
            sigprocmask(SIG_SETMASK,&oldMask,NULL);
        } else{
            // debug("some other signal detected");
        }

        debug("waiting for some kind signal");
        sigsuspend(&sigSuspendMask); // suspend execution until receipt of any kind of signal
        debug("received a signal");
    }

    return 0;
}

