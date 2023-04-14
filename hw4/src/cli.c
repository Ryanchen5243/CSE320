#include <stdlib.h>

#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"

// ajsjajs
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

extern volatile sig_atomic_t user_input_detected;
extern sigset_t sigSuspendMask;
extern int quitDetected;
extern int firstHandle;
static int watchersCount = 0;
static WATCHER* watcherTable[100];
static int cmdInputLen;
extern volatile sig_atomic_t dataAvailable;
int numBytesReadGlobal;


char* bufferUserInput();

static int handleUserCommand(char*);
static void insertIntoWatcherTable(WATCHER* w);
static void displayWatchers();
static void freeAndRemoveFromWatcherTable(WATCHER* w);
static void cleanUpWatchers();


typedef struct watcher {
    int tableIndex;
    int wtype;
    int pid;
    int fdin;
    int fdout;
    char* channel;
} WATCHER;

WATCHER *cli_watcher_start(WATCHER_TYPE *type, char *args[]) {
    WATCHER* newCLI = malloc(sizeof(WATCHER)); // allocate space for a watcher
    newCLI->wtype = CLI_WATCHER_TYPE;
    newCLI->pid = -1;
    newCLI->fdin = 0;
    newCLI->fdout = 1;
    newCLI->channel = NULL;
    // initialize watcher table
    for(int i =0; i< 100;i++){
        watcherTable[i] = NULL;
    }
    insertIntoWatcherTable(newCLI);
    cli_watcher_send(newCLI,"ticker> ");
    return newCLI;
}

int cli_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    kill(wp->pid,SIGKILL);
    freeAndRemoveFromWatcherTable(wp);
    return 0;
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    if(strncmp((char*)arg, "ticker> ", strlen("ticker> ")) == 0) { // prompt user input
        write(STDOUT_FILENO, arg, strlen((char*)arg));
        fflush(stdout);
        return 0;
    }
    // else check fd for stdout to write data to
    return 0;
}

// handling user command
int cli_watcher_recv(WATCHER *wp, char *txt) {
    // debug("text is ;;;;;; %s",txt);
    // split txt into parts pass into handle in loop
    char* cur = txt;
    // debug("strlennn %zu",strlen(txt));
    int len = strlen(txt);
    for(int i = 0; i < len;i++){
        if(txt[i] == '\n'){
            txt[i] = '\0';
            debug("curr %s",cur);
            handleUserCommand(cur);
            cur = txt+1+i;
        }
    }

    if(quitDetected == 1 || numBytesReadGlobal==0){
        cleanUpWatchers();
        return 0;
    }

    // else ck fd for stdin to read from
    return 0;
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    return 0;
}

// read from buffer
// on success return ptr to buffer, caller to free
// on failure, frees ptr returns nULL
char* bufferUserInput() {
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
            fclose(stream);
            cleanUpWatchers();
            exit(1);
        }
        // debug("in loop tbd %d",totalBytesRead);
        totalBytesRead+=numBytesRead;
    }
    debug("Temp Buf is now: %s\n",tempBuf);
    if(numBytesRead == -1){
        if(errno==EWOULDBLOCK){
            dataAvailable = 0;
        }else{
            perror("laksjdfalskdfj");
            // handle error
            fclose(stream);
            cleanUpWatchers();
            exit(1);
        }
    }

    // debug("helllllllll");
    // debug("Total Bytes Read %d",totalBytesRead);
    char* buffer = (char*) malloc(totalBytesRead * sizeof(char));
    if(buffer==NULL) perror("malloc bffer error");
    // read from stream
    int c;
    int index =0;
    cmdInputLen = 0;
    while((c = fgetc(stream)) != EOF){
        buffer[index] = c;
        index++;
        cmdInputLen++;
    }
    // debug("returned buffer is %s",buffer);
    fclose(stream);
    numBytesReadGlobal = numBytesRead;
    return buffer;
}

static int handleUserCommand(char* cmd){
    // debug("handling user input");
    int numwords = 0;
    int numSpace = 0;
    debug("testing command????????? %s\n",cmd);
    int i = 0;
    for(char* p = cmd; i<cmdInputLen; p++,i++){
        // debug("Iterating ...");
        if(*p == ' '){
            numSpace++;
        }
    }
    numwords = numSpace+1;
    char* cmdPtrs[numwords];
    int ind = 0;
    cmdPtrs[ind] = cmd;
    ind++;
    i = 0;
    for(char* p = cmd; i<cmdInputLen; p++,i++){
        if(*p == ' '){
            *p = '\0';
            cmdPtrs[ind] = p+1;
            ind++;
        }
    }

    if(strncmp(cmdPtrs[0], "quit", strlen("quit")) == 0){
        debug("Invokkginn Quit Function");
        quitDetected = 1;
        cleanUpWatchers();
    } else if(strncmp(cmd,"watchers",strlen("watchers"))== 0){
        debug("Watchers Function");
        displayWatchers();
    } else if(strncmp(cmd,"start",strlen("start"))== 0){
        debug("Start Function");
        WATCHER* bsw = watcher_types[BITSTAMP_WATCHER_TYPE].start(&watcher_types[BITSTAMP_WATCHER_TYPE],
        watcher_types[BITSTAMP_WATCHER_TYPE].argv);

        if(bsw);

    } else if(strncmp(cmd,"stop",strlen("stop"))== 0){
        debug("Stop Function");
    } else if(strncmp(cmd,"trace",strlen("trace"))== 0){

    } else if(strncmp(cmd,"untrace",strlen("untrace"))== 0){

    } else if(strncmp(cmd,"show",strlen("show"))== 0){

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

static void freeAndRemoveFromWatcherTable(WATCHER* w){
    int index = w->tableIndex;
    if (index >= 0 && index < 100) {
        for (int i = index + 1; i < watchersCount; i++) {
            watcherTable[i-1] = watcherTable[i];
            watcherTable[i-1]->tableIndex = i-1;
        }
        watcherTable[watchersCount-1] = NULL;
        watchersCount--;
    }
    free(w);
}

// remove all watchers from table
static void cleanUpWatchers(){
    // debug("Now quitting program");
    // go through watcher list and free resources
    // debug("%d",watchersCount);
    for(int i = 0; i < watchersCount;i++){
        freeAndRemoveFromWatcherTable(watcherTable[i]);
    }
}