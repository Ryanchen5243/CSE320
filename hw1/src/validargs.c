#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

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
    printf("argc: %d\n",argc);
    // TO BE IMPLEMENTED.
    if (argc==1){ // no flags -> bin/fliki
        printf("there are no flags\n");
        printf("---------------------------------");
        return 0;
    }
    // check for flag
    char flagCk = *(*(argv+1)+0);
    if (flagCk=='-'){
        if(*(*(argv+1)+1) == '\0' || *(*(argv+1)+2)!='\0'){ // invalid flag
            printf(">>>>>>>>>>>>>>>>>invalid flag>>>>>>>>>>>>>>>>>\n");
            return -1;
        }
        if (*(*(argv+1)+1)=='h'){
            printf(">>>>>>>>>>>>>>>>>-h case\n");
            printf("global options before: %lx\n",global_options);
            global_options = global_options | 0x1;
            printf("global options after: %lx\n",global_options);
            return 0;
        } else if (*(*(argv+1)+1)=='n'){
            printf(">>>>>>>>>>>>>>>>> n case\n");

        } else if (*(*(argv+1)+1)=='q'){
            printf(">>>>>>>>>>>>>>>>> q case\n");
        } else{
            printf(">>>>>>>>>>>>>>>>> invalid flag\n");
        }
    } else { // not flag -> file name
        printf("you have included a filename\n");
    }

    for (int i=0;i<argc;i++){
        int j=0;
        while(*(*(argv+i)+j)!='\0'){
            printf("%c",*(*(argv+i)+j));
            j++;
        }
        printf("\n");
    }
    return 0;
    
    abort();
}
