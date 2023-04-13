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
    // args[0] = uwsc
    // args[1] = ...bitstamp.net
    // start bitstamp.net live_trades_btcusd

    WATCHER* b = malloc(sizeof(WATCHER));
    b->wtype = BITSTAMP_WATCHER_TYPE;

    // set up 2 way pipe
    int pipe1[2],pipe2[2];
    if(pipe(pipe1) == -1){
        perror("pipe 1 error");
    }
    if(pipe(pipe2) == -1){
        perror("pipe2 error");
    }

    pid_t pid = fork();
    if((pid == 0)){ // child
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

        // close original pipe ends
        close(pipe1[0]);
        close(pipe2[1]);

        if(execvp("uwsc",args) < 0){
            debug("alskdfj;laskdfja;slfkjexecvp failed");
        } else{
            debug("launched uwsc");
        }
    } else { // parent
        close(pipe1[0]);
        close(pipe2[1]);
        b->pid = pid;

        // set up fd for watcher
        b->fdin = pipe2[0];
        b->fdout = pipe1[1];
        b->channel = *args;
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
