#include <stdlib.h>

#include "ticker.h"
#include "store.h"
#include "cli.h"
#include "debug.h"

// ajsjajs
#include <unistd.h>

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
    write(STDOUT_FILENO,"ticker> ",8);
    return newCLI;
}

int cli_watcher_stop(WATCHER *wp) {
    // TO BE IMPLEMENTED
    free(wp);

    return 0;
}

int cli_watcher_send(WATCHER *wp, void *arg) {
    // TO BE IMPLEMENTED
    abort();
}

int cli_watcher_recv(WATCHER *wp, char *txt) {
    // TO BE IMPLEMENTED
    abort();
}

int cli_watcher_trace(WATCHER *wp, int enable) {
    // TO BE IMPLEMENTED
    abort();
}
