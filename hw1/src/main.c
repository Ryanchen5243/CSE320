#include <stdio.h>
#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);
    // TO BE IMPLEMENTED
    // open diff file
    FILE* diffPtr = fopen(diff_filename,"r");
    // error opening file?
    if(diffPtr==NULL){
        return EXIT_FAILURE;
    }



    int patchRes = patch(stdin,stdout,diffPtr);
    fclose(diffPtr); // close diff file

    // return success or failure
    if(patchRes == -1){
        // printf("returning failure");
        return EXIT_FAILURE;
    }
    // printf("Returning exit succes");
    return EXIT_SUCCESS;
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
