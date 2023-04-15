#include <stdlib.h>
#include <stdio.h>

#include "ticker.h"
#include "bitstamp.h"
#include "debug.h"

#include <unistd.h>
#include <string.h>
#include <signal.h>
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
        if(execvp("uwsc",watcher_types[BITSTAMP_WATCHER_TYPE].argv) == -1) {
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
        b->trace = 0;
        b->terminated = 0;
    }
    return b;
}

int bitstamp_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    wp->terminated = 1;
    if (kill(wp->pid, SIGTERM) == -1) {
        perror("SIGTERM send fail");
        return -1;
    }
    return 0;
}

int bitstamp_watcher_send(WATCHER *wp, void *arg) {
    // json to send to server

    return 0;
}

int bitstamp_watcher_recv(WATCHER *wp, char *txt) {
    // data from child
    return 0;
}

int bitstamp_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    return 0;
}
