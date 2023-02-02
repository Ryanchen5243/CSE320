#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

int isFlag(char* str,int len);

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    // TO BE IMPLEMENTED.
    
    if (argc==1){ // no flags provided -> bin/fliki
        // printf("no flags provided\n");
        return -1;
    }
    int nActive = -1; // if n is triggered
    int qActive = -1; // if q is triggered
    for(int i = 1; i < argc; i++){
        int j = 0;
        // determine length of argument
        while (*(*(argv+i)+j) != '\0'){
            j+=1;
        }
        // check if flag
        int flag = isFlag(*(argv+i),j);

        // if h flag is not first
        if (flag=='h'&& i!=1){
            return -1;
        // if h flag
        } else if(flag=='h' && i==1){
            global_options = global_options | HELP_OPTION;
            return 0;
        // if last argument is not filename
        } else if (flag!=-1 && i==argc-1){
            return -1;
        // if file name is not last arg
        } else if (flag==-1 && i!=argc-1) {
            return -1;
        }
        
        if (flag == 'n'){ // no output option
            nActive = 1;
        } else if (flag == 'q'){ // quiet mode
            qActive = 1;
        } else { // assign filename
            diff_filename = *(argv+i);
        }

    }
    // update global options
    if (nActive == 1){
        global_options = global_options | NO_PATCH_OPTION;
    }
    if (qActive == 1){
        global_options = global_options | QUIET_OPTION;
    }
    
    return 0; // successful validation
    abort();
}

int isFlag(char* str,int len){
    if (len != 2 || *str != '-'){ // not flag
        // printf("not flag\n");
        return -1;
    }
    // printf("flag\n");
    return *(str+1);
}