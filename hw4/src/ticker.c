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
    char* channel;
} WATCHER;

static int handleUserInput();
static int handleUserCommand(char*);
static void insertIntoWatcherTable(WATCHER* w);
static void displayWatchers();
volatile sig_atomic_t user_input_detected = 0;
static WATCHER* watcherTable[100];
static int watchersCount = 0;
static void removeFromWatcherTable(WATCHER* w);
static int quitDetected = 0;
static void cleanUpWatchers();
static int firstHandle;

void sigint_handler(int s){
    // char* msg = "SIGINT Signal Received\n";
    // write(STDOUT_FILENO,msg,24);
    exit(0);
}

void sigio_handler(int s, siginfo_t* info,void* context){
    // char* msg = "SIG IO Signal Recieved\n";
    // write(STDOUT_FILENO,msg,24);
    debug("SIGIO signal");
    int oldErrno = errno;
    int fd = info->si_fd;
    // criticla section - block out signal s
    sigset_t mask;
    sigset_t oldMask;
    sigemptyset(&mask);
    sigaddset(&mask,s);
    sigprocmask(SIG_BLOCK,&mask,&oldMask);
    if (fd == STDIN_FILENO) { // user input
        user_input_detected = 1;
        // printf("User input detected!\n");
    } else{
        printf("handling other SIGIO");
    }
    // unblock signals
    sigprocmask(SIG_SETMASK,&oldMask,NULL);

    errno = oldErrno; // restore errno
}

void sigchild_handler(int s){
    char* msg = "SIG CHILD Signal Recieved";
    write(STDOUT_FILENO,msg,sizeof(msg));
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
    // initialize watcher table with NULLs
    for(int i =0; i< 100;i++){
        watcherTable[i] = NULL;
    }

    // printf("\n---%p\n",cliWatcher);
    insertIntoWatcherTable(cliWatcher);

    // char* args[] = {"wss://ws.bitstamp.net",NULL};

    sigset_t sigSuspendMask;
    sigemptyset(&sigSuspendMask);

    // check stdin if there exists input already? send sigio signal
    firstHandle = 1;
    handleUserInput();
    firstHandle = 0;
    if(quitDetected){
        return 0;
    }
    debug("done handling first user input");

    while(1){ // loop until receipt of any kind of signal
        // body of main loop
        debug("In main loop");
        if (user_input_detected){
            handleUserInput();
            if(quitDetected){
                return 0;
            }
            write(STDOUT_FILENO,"ticker> ",8);

            sigset_t mask;
            sigset_t oldMask;
            sigfillset(&mask);
            sigprocmask(SIG_BLOCK,&mask,&oldMask);
            // critical section
            user_input_detected = 0;
            sigprocmask(SIG_SETMASK,&oldMask,NULL);
        }
        // else if any other signal

        debug("waiting for some kind signal");
        sigsuspend(&sigSuspendMask); // suspend execution until receipt of any kind of signal
        debug("received a signal");
    }

    return 0;
}

static int handleUserInput() {
    // stream -> openmemstream -> dynamic buffer(not used)
    char* z; // disposed buffer
    size_t bufSize; // size of buffer
    FILE* stream = open_memstream(&z,&bufSize);
    // read everything from stdin to stream
    int numBytesRead = 0;
    int totalBytesRead = 0;
    char tempBuf[100];

    while((numBytesRead = read(STDIN_FILENO,tempBuf,sizeof(tempBuf))) > 0){
        // debug("Temp Buf is now: %s\n",tempBuf);
        // write tempBuf to stream
        int bytesWritten = fwrite(tempBuf,sizeof(char),numBytesRead,stream);
        if (bytesWritten < numBytesRead) {
            perror( "Error writing to output\n");
            exit(1);
        }
        // debug("in loop tbd %d",totalBytesRead);
        totalBytesRead+=numBytesRead;
    }
    // done writing to stream
    if(numBytesRead==0){
        quitDetected = 1;
        cleanUpWatchers();
        fclose(stream);
        return 0;
    }
    debug("helllllllll");
    // debug("Total Bytes Read %d",totalBytesRead);
    char* buffer = (char*) malloc(totalBytesRead * sizeof(char));
    if(buffer==NULL) perror("malloc bffer error");
    // read from stream
    int c;
    int index =0;
    while((c = fgetc(stream)) != EOF){
        buffer[index] = c;
        index++;
    }

    // debug("Buffer looks like %s",buffer);
    handleUserCommand(buffer);
    if(quitDetected == 1){
        // debug("deteced quit -- freeing buffer, closing stream");
        free(buffer);
        fclose(stream);
        cleanUpWatchers();
        return 0;
    }
    // reset stuff
    // debug("handleuserinput -- free buffer,close stream");
    free(buffer);
    fclose(stream);
    return 0;
}

static int handleUserCommand(char* cmd){
    // debug("handling user input");
    int numwords = 0;
    int numSpace = 0;
    for(char* p = cmd; *p; p++){
        if(*p == ' '){
            numSpace++;
        }
    }
    numwords = numSpace+1;
    char* cmdPtrs[numwords];
    int ind = 0;
    cmdPtrs[ind] = cmd;
    ind++;
    for(char* p = cmd; *p; p++){
        if(*p == ' '){
            *p = '\0';
            cmdPtrs[ind] = p+1;
            ind++;
        }
    }
    // for(int i = 0; i< numwords;i++){
    //     printf("CMDPTR ::::: %s\n",cmdPtrs[i]);
    //     fflush(stdout);
    // }

    // cmdPtrs consists of pointers to cmd args
    // sizeof(cmdPtrs)/sizeof(char*)
    if(strncmp(cmdPtrs[0], "quit", strlen("quit")) == 0){
        debug("Quit Function");
        quitDetected = 1;
    } else if(strncmp(cmdPtrs[0],"watchers",strlen("watchers"))== 0){
        debug("Watchers Function");
        displayWatchers();
    } else if(strcmp(cmdPtrs[0],"start")== 0){
        debug("Start Function");
        // cmdPtrs[1] -> wtype
        // cmdPtrs[2] -> channel
        WATCHER* bsw = watcher_types[BITSTAMP_WATCHER_TYPE].start(&watcher_types[BITSTAMP_WATCHER_TYPE],
        watcher_types[BITSTAMP_WATCHER_TYPE].argv);

        if(bsw);

    } else if(strcmp(cmdPtrs[0],"stop")== 0){
        debug("Stop Function");

    } else if(strcmp(cmdPtrs[0],"trace")== 0){

    } else if(strcmp(cmdPtrs[0],"untrace")== 0){

    } else if(strcmp(cmdPtrs[0],"show")== 0){

    } else{
        // invalid cmd
        if(!firstHandle){
            printf("???\n");
            fflush(stdout);
        }
    }
    return 1;
}

static void insertIntoWatcherTable(WATCHER* w){
    watcherTable[watchersCount] = w;
    // update pos field
    w->tableIndex = watchersCount;
    watchersCount++;

}

static void displayWatchers(){
    for(int i = 0; i < watchersCount;i++){
        if(watcherTable[i]->wtype == CLI_WATCHER_TYPE){
            // char* cliDis = 
            // write(STDOUT_FILENO,"%d\t%s(%d,%d,%d)\n",watcherTable[i]->tableIndex,
            //     watcher_types[CLI_WATCHER_TYPE].name,watcherTable[i]->pid,watcherTable[i]->fdin,watcherTable[i]->fdout);
            char cliDis[100];
            sprintf(cliDis, "%d\t%s(%d,%d,%d)\n", watcherTable[i]->tableIndex, watcher_types[CLI_WATCHER_TYPE].name, watcherTable[i]->pid, watcherTable[i]->fdin, watcherTable[i]->fdout);
            debug("clidis %s",cliDis);
            write(STDOUT_FILENO, cliDis, strlen(cliDis));
        } else {
            // printf("%s\n", "bitstamp");
        }
    }
    fflush(stdout);
}

static void removeFromWatcherTable(WATCHER* w){
    debug("removing watching from table");
    int index = w->tableIndex;
    if (index >= 0 && index < 100) {
        for (int i = index + 1; i < watchersCount; i++) {
            watcherTable[i-1] = watcherTable[i];
            watcherTable[i-1]->tableIndex = i-1;
        }
        watcherTable[watchersCount-1] = NULL;
        watchersCount--;
    }
    // free resources
    free(w);
}

static void cleanUpWatchers(){
    debug("Now quitting program");
    // go through watcher list and free resources
    debug("%d",watchersCount);
    for(int i = 0; i < watchersCount;i++){
        removeFromWatcherTable(watcherTable[i]);
    }
}
