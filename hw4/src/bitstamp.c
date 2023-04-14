#include <stdlib.h>
#include <stdio.h>

#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"

#include <unistd.h>

typedef struct watcher {
    int tableIndex;
    int wtype;
    int pid;
    int fdin;
    int fdout;
    char* channel;
} WATCHER;

WATCHER *bitstamp_watcher_start(WATCHER_TYPE *type, char *args[]) {
    // start bitstamp.net live_trades_btcusd

    WATCHER* b = malloc(sizeof(WATCHER));
    b->wtype = BITSTAMP_WATCHER_TYPE;
    debug("testing testing cmd ptr %s",*args);

    // set up 2 way pipe
    int pipe1[2],pipe2[2];
    if(pipe(pipe1) == -1){
        perror("pipe 1 error");
    }
    if(pipe(pipe2) == -1){
        perror("pipe2 error");
    }
    debug("args 0 %s",args[0]);
    debug("args 1 %s",args[1]);
    debug("args 2 %s",args[2]);
    // debug("channel %s",type->channel);
    pid_t pid = fork();

    if((pid == 0)){ // child
        debug("fork in child");
        // close ends
        close(pipe1[1]);
        close(pipe2[0]);

        // child read from read end of pipe1
        if(dup2(pipe1[0], STDIN_FILENO) == -1){
            perror("dup2 stdin error");
        }
        // child write to write end of pipe2
        if(dup2(pipe2[1], STDOUT_FILENO) == -1){
            perror("dup2 stdout error");
        }
        // start bitstamp.net live_trades_btcusd
        // close original pipe ends
        close(pipe1[0]);
        close(pipe2[1]);
        debug("in child lol");
        if(execvp(args[0],args) == -1) {
            debug("execvp error ");
        }

    } else { // parent
        close(pipe1[0]);
        close(pipe2[1]);
        b->pid = pid;

        // set up fd for watcher
        b->fdin = pipe2[0];
        b->fdout = pipe1[1];
        b->channel = *args;
        debug("tesing json");
        // send to child

    }

    return b;
}

int bitstamp_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    free(wp);
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    // TO BE IMPLEMENTED
    return 0;
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    return 0;
}
