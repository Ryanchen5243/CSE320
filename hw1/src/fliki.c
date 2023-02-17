#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

static int serialNum; // serial number for given hunk
static int diff_t; // trailing diff file ptr
static int diff_c; // current diff file ptr
static int diff_data_ptr; // data ptr
static int diff_data_trailing_ptr; // trailing data ptr

// current hunk insertion and deletion arrow counter
// to be reset after every next hunk
static int currHunkInsertArrowCount;
static int currHunkDeleteArrowCount;

typedef enum {
    NONE_OPERATION,ADDITION_MODE,DELETION_MODE
} OPERATION;

static int isDigit(char c);


/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */

int hunk_next(HUNK *hp, FILE *in) {
    while((diff_c = (fgetc(in))) != EOF) {
        // hunk begins with digit as first char or
        int h_cond = (diff_t == -1 && isDigit(diff_c))
            || (diff_t =='\n' && isDigit(diff_c));
        if (h_cond) { // found hunk
            ungetc(diff_c,in);
            // diff_c holds first digit
            // properties for hunk

            int old1=-1;
            int old2=-1;
            HUNK_TYPE h_type = -1;
            int new1=-1;
            int new2=-1;

            // parse header line for correct format

            // first section X or X,X
            diff_c = fgetc(in);
            if(!isDigit(diff_c)){ // first char
                return ERR;
            } else {
                old1 = diff_c - '0';
                diff_c = fgetc(in);
                while(isDigit(diff_c)){
                    old1 = old1 * 10;
                    old1 += (diff_c-'0');
                    diff_c = fgetc(in);
                }
                // if comma
                if (diff_c == ','){
                    diff_c = fgetc(in);
                    if(!isDigit(diff_c)){
                        return ERR;
                    } else{
                        old2 = diff_c - '0';
                        diff_c = fgetc(in);
                        while(isDigit(diff_c)){
                            old2 = old2 * 10;
                            old2 += (diff_c - '0');
                            diff_c = fgetc(in);
                        }
                        goto validate_header_cmd_type;
                    }
                } else {
                    old2 = old1;

                    validate_header_cmd_type: // goto branch
                    if(diff_c == 'a'){
                        h_type = HUNK_APPEND_TYPE;
                    } else if (diff_c == 'd'){
                        h_type = HUNK_DELETE_TYPE;
                    } else if(diff_c == 'c'){
                        h_type = HUNK_CHANGE_TYPE;
                    } else {
                        return ERR;
                    }

                    // validate new range
                    diff_c = fgetc(in);
                    if(!isDigit(diff_c)){
                        return ERR;
                    } else{
                        new1 = diff_c - '0';
                        diff_c = fgetc(in);
                        while(isDigit(diff_c)){
                            new1 = new1 * 10;
                            new1 += (diff_c - '0');
                            diff_c = fgetc(in);
                        }
                        if(diff_c == ','){
                            diff_c = fgetc(in);
                            if(!isDigit(diff_c)){
                                return ERR;
                            } else {
                                new2 = diff_c - '0';
                                diff_c = fgetc(in);
                                while(isDigit(diff_c)){
                                    new2 = new2 * 10;
                                    new2 += (diff_c - '0');
                                    diff_c = fgetc(in);
                                }
                                if(diff_c == '\n'){
                                    goto valid_hunk_header;
                                } else{
                                    return ERR;
                                }
                            }
                        } else if(diff_c == '\n'){
                            new2 = new1;
                            goto valid_hunk_header;
                        } else {
                            return ERR;
                        }
                    }
                }

            }

            valid_hunk_header:
            if(old1==-1 || old2==-1 || h_type == -1 || new1==-1 || new2==-1){
                return ERR;
            } else if(old1 > old2 || new1 > new2){
                return ERR;
            } else if(h_type == HUNK_APPEND_TYPE && old1 != old2) {
                return ERR;
            } else if(h_type == HUNK_DELETE_TYPE && new1 != new2){
                return ERR;
            }

            // initialize hunk
            (*hp).type = h_type;
            (*hp).old_start = old1;
            (*hp).old_end = old2;
            (*hp).new_start = new1;
            (*hp).new_end = new2;
            (*hp).serial = serialNum++;

            return 0; // success
        }
        diff_t = diff_c; // update trailing
    }

    return EOF; // end of file
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

int hunk_getc(HUNK *hp, FILE *in) {
    // TO BE IMPLEMENTED
    diff_data_trailing_ptr = diff_data_ptr; // update trailing ptr
    diff_data_ptr = fgetc(in); // read next char


    if (diff_data_ptr == EOF){
        return EOF;
    }
    // end of section
    if (isDigit(diff_data_ptr) && diff_data_trailing_ptr == '\n'){
        ungetc(diff_data_ptr,in); // return to stream first digit
        return EOS;
    } else if ((diff_data_ptr == '>' && diff_data_trailing_ptr == -1) ||
        (diff_data_ptr == '>' && diff_data_trailing_ptr == '\n')) {
            diff_data_trailing_ptr = diff_data_ptr;
            diff_data_ptr = fgetc(in); // read next char
            if (diff_data_ptr == ' '){
                // insertion case "> "
                currHunkInsertArrowCount++; // update arrow count
                diff_data_trailing_ptr = diff_data_ptr;
                diff_data_ptr = fgetc(in);
            } else {
                return ERR;
            }
    } else if ((diff_data_ptr == '<' && diff_data_trailing_ptr == -1) ||
        (diff_data_ptr == '<' && diff_data_trailing_ptr == '\n')){
        diff_data_trailing_ptr = diff_data_ptr;
        diff_data_ptr = fgetc(in);
        if (diff_data_ptr == ' '){
            // deletion case
            currHunkDeleteArrowCount++;
            diff_data_trailing_ptr = diff_data_ptr;
            diff_data_ptr = fgetc(in);
        } else {
            return ERR;
        }

    } else if (diff_data_ptr == '-' && diff_data_trailing_ptr == '\n'){ // validate "---\n"
        diff_data_trailing_ptr = diff_data_ptr;
        diff_data_ptr = fgetc(in);
        if(diff_data_ptr != '-'){
            return ERR;
        }
        diff_data_trailing_ptr = diff_data_ptr;
        diff_data_ptr = fgetc(in);
        if(diff_data_ptr != '-'){
            return ERR;
        }
        diff_data_trailing_ptr = diff_data_ptr;
        diff_data_ptr = fgetc(in);
        if (diff_data_ptr != '\n'){
            return ERR;
        }
        diff_data_trailing_ptr = diff_data_ptr;
        diff_data_ptr = fgetc(in);


        if (diff_data_ptr == '>'){
            ungetc(diff_data_ptr,in);
            return EOS;
        } else {
            return ERR; // hunk change type missing addition section
        }

    }

    return diff_data_ptr; // succcess
}

/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "..." followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    // TO BE IMPLEMENTED

    // line in tradtional format of hunk
    int oldStart = hp->old_start;
    int oldEnd = hp->old_end;
    int newStart = hp->new_start;
    int newEnd = hp->new_end;
    HUNK_TYPE type = hp->type;

    if(oldStart != oldEnd) {
        fprintf(out,"%d,%d",oldStart,oldEnd);
    } else {
        fprintf(out,"%d",oldStart);
    }
    if(type == HUNK_APPEND_TYPE){
        fprintf(out,"%c",'a');
    } else if(type == HUNK_DELETE_TYPE){
        fprintf(out,"%c",'d');
    } else{
        fprintf(out,"%c",'c');
    }

    if(newStart != newEnd){
        fprintf(out,"%d,%d",newStart,newEnd);
    } else {
        fprintf(out,"%d",newStart);
    }
    fprintf(out,"\n");

    // Display Buffer

    // display deletions
    char*p;

    p = hunk_deletions_buffer + 2;
    int arrowFlag = 1;

    if(hp->type == HUNK_APPEND_TYPE){
        goto display_addition_buffer;
    }

    // display deletion buffer
    do {
        if(arrowFlag){
            fprintf(stderr,"%c%c",'<',' ');
            arrowFlag = 0;
        }
        fprintf(stderr,"%c",*p);
        if(*p == '\n'){
            arrowFlag=1;
            p+=2;
            continue;
        }
        p++;
    } while(!(*p==0 && *(p+1)==0));


    if(hp->type == HUNK_DELETE_TYPE){
        return;
    }
    fprintf(stderr,"%s","---\n"); // line break for change


    display_addition_buffer:

    p = hunk_additions_buffer;
    arrowFlag = 1;

    do {
        if(arrowFlag){
            fprintf(stderr,"%c%c",'>',' ');
            arrowFlag = 0;
        }
        fprintf(stderr,"%c",*p);
        if(*p == '\n'){
            arrowFlag=1;
            p+=2;
            continue;
        }
        p++;
    } while(!(*p==0 && *(p+1)==0));



    return;
}

/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    // TO BE IMPLEMENTED
    HUNK myHunk; // Current hunk
    diff_c = -1; // for purpose of parsing diff file for hunk next
    diff_t = -1;
    serialNum = 0; // initialize serial num
    // for purpose of parsing data section of given hunk
    diff_data_ptr = -1;
    diff_data_trailing_ptr = -1;

    // bin/fliki rsrc/file1_file2.diff < rsrc/file1 > test_output/basic_test.out


    int src_file_line_ctr = 0; // num lines parsed measured by \n
    int output_line_ctr = 0; // num lines written to output by \n

    // initialize buffer pointers
    char* delBuffPtr = hunk_deletions_buffer;// deletions buffer ptr
    char* addBuffPtr = hunk_additions_buffer; // addition buffer ptr

    unsigned long globalOptionCopy = global_options >> 2; // right shift by 2
    int quietModeEnabled = 0;
    if(globalOptionCopy % 10 == 0){
        // not quite mode
    } else {
        quietModeEnabled = 1;
    }

    // while there are hunks
    int res_hunk_next = hunk_next(&myHunk,diff);
    while(res_hunk_next != EOF){
        if (res_hunk_next == ERR){
            if(!quietModeEnabled){
                fprintf(stderr,"Invalid Hunk Header\n");
            }
            return -1;
        }

        // clear buffers
        for(int i = 0; i < HUNK_MAX;i++){
            *(hunk_deletions_buffer + i) = (unsigned char) 0x0;
            *(hunk_additions_buffer + i) = (unsigned char) 0x0;
        }
        // reset buffer pointers
        delBuffPtr = hunk_deletions_buffer;
        addBuffPtr = hunk_additions_buffer;


        char * delLineContentPtr = delBuffPtr + 2; // purpose of storing contents of given line
        char* addLineContentPtr = addBuffPtr + 2;

        // Displays hunk info
        /*
        printf("%d,%d,%d,%d\n",myHunk.old_start,myHunk.old_end,myHunk.new_start,myHunk.new_end);
        printf("Serial num: %d\n",serialNum);
        if(myHunk.type == HUNK_APPEND_TYPE){
            printf("%s\n","Append");
        } else if(myHunk.type == HUNK_DELETE_TYPE){
            printf("%s\n","Delete");
        } else {
            printf("%s\n","Change");
        }*/

        // fetch hunk details
        int hunkOldStart = myHunk.old_start;
        int hunkOldEnd = myHunk.old_end;
        int hunkNewStart = myHunk.new_start;
        int hunkNewEnd = myHunk.new_end;
        int currHunkType = myHunk.type;
        /*
        printf("%d,%d",hunkOldStart,hunkOldEnd);
        if(currHunkType == HUNK_APPEND_TYPE){
            printf("%s","a");
        } else if (currHunkType == HUNK_DELETE_TYPE){
            printf("%s","d");
        } else {
            printf("%s","c");
        }
        printf("%d,%d\n",hunkNewStart,hunkNewEnd);
        */

        // in -> source file pointer
        // while source file line # isn't in consideration
        // copy contents from src to output file

        int numNewLinesTillStart = -1; // initialize num new lines tracker

        OPERATION myoperation = NONE_OPERATION;
        if(currHunkType == HUNK_APPEND_TYPE){// case addition
            myoperation = ADDITION_MODE;
            numNewLinesTillStart = hunkOldStart - src_file_line_ctr;
        } else if (currHunkType == HUNK_DELETE_TYPE){// case deletion
            myoperation = DELETION_MODE;
            numNewLinesTillStart = hunkOldStart - src_file_line_ctr - 1;
        } else {// case change
            myoperation = DELETION_MODE; // initially deletion mode
            numNewLinesTillStart = hunkOldStart - src_file_line_ctr - 1;
        }

        int sourceChar = -1;
        int new_line_count = 0; // for copy purpose


        // write contents from src to ouput until start of hunk affected line
        while(new_line_count < numNewLinesTillStart){
            sourceChar = fgetc(in); // read char
            if (sourceChar == EOF) {
                if(!quietModeEnabled) {
                    fprintf(stderr,"Invalid Start Lines Provided\n");
                }
                return -1;
            }
            fprintf(out,"%c",sourceChar); // write char to output file
            if(sourceChar == '\n'){
                src_file_line_ctr++; // increment source line count
                output_line_ctr++; // increment output line count
                new_line_count++;
            }
        }

        // in -> next source file char to handle by given hunk

        // addition -> oldstart line has already been written
        // need only to insert data section of hunk
        // validate new lines
        if(myoperation == ADDITION_MODE && hunkNewStart != (output_line_ctr + 1)){
            if(!quietModeEnabled){
                fprintf(stderr,"Invalid Addition Lines provided\n");
            }
            return -1;
        } else if (currHunkType == HUNK_CHANGE_TYPE){
            if (hunkNewStart != (output_line_ctr + 1)){
                if(!quietModeEnabled){
                    fprintf(stderr,"Invalid change new start line\n");
                }
                return -1;
            }
        }

        // initialize hunk arrow counts for given hunk
        currHunkInsertArrowCount = 0;
        currHunkDeleteArrowCount = 0;

        // read data section of given hunk
        // fgetc(diff) returns first char of curr Hunk data section

        while((diff_data_ptr = hunk_getc(&myHunk,diff)) != EOF){

            if(diff_data_ptr == EOS){// end of section
                // printf("%s\n","******************end of section >>>>>>>>>>>>>>>>>>>>>>>");
                // proceed with additions section for change type
                int temp = -1;
                if(currHunkType==HUNK_CHANGE_TYPE && ((temp = fgetc(diff)) == '>')){
                    // validate deletion lines count
                    if ((hunkOldEnd - hunkOldStart + 1) != currHunkDeleteArrowCount){
                        if(!quietModeEnabled){
                            fprintf(stderr,"%s\n","Invalid number deletion lines provided");
                            hunk_show(&myHunk,stderr);
                        }
                        return -1;
                    }
                    ungetc('>',diff);
                    diff_data_ptr = '\n';
                    myoperation = ADDITION_MODE; // update mode
                    continue;
                }
                if (temp != -1){
                    ungetc(temp,diff);
                }

                break;
            } else if (diff_data_ptr == ERR){
                // error
                if(!quietModeEnabled){
                    hunk_show(&myHunk,stderr);
                }

            } else {
                // Hunk Change Type -- validate lines (deletion portin)
                if (currHunkType == HUNK_CHANGE_TYPE && myoperation == DELETION_MODE){
                    int track_new_line;
                    track_new_line = fgetc(in);
                    *delLineContentPtr = diff_data_ptr;// add to deletion buffer
                    delLineContentPtr++; // advance ptr
                    (*delBuffPtr)+=1; // update count

                    if(diff_data_ptr != track_new_line){ // compare w source file
                        // error
                        if(!quietModeEnabled){
                            fprintf(stderr,"%s\n","Non Matching Deletion Lines");
                            hunk_show(&myHunk,stderr);
                        }
                        return -1;
                    } else if(track_new_line == '\n'){
                        src_file_line_ctr++;// increment source file count
                        // new line detected
                        delBuffPtr = delLineContentPtr;
                        delLineContentPtr += 2;
                    }
                } else if (currHunkType == HUNK_CHANGE_TYPE && myoperation == ADDITION_MODE){
                    // write data from hunk to output
                    fprintf(out,"%c",diff_data_ptr);
                    *addLineContentPtr = diff_data_ptr;
                    addLineContentPtr++;
                    (*addBuffPtr)++; // increment count

                    if(diff_data_ptr == '\n'){
                        output_line_ctr++; // increment output line ctr
                        addBuffPtr = addLineContentPtr;
                        addLineContentPtr+=2;
                    }
                } else if(currHunkType == HUNK_APPEND_TYPE && myoperation == ADDITION_MODE){
                    // write contents from diff file to output file
                    fprintf(out,"%c",diff_data_ptr);
                    *addLineContentPtr = diff_data_ptr;
                    addLineContentPtr++;
                    (*addBuffPtr)++;// increment count
                    if(diff_data_ptr == '\n'){
                        output_line_ctr++;
                        addBuffPtr = addLineContentPtr;
                        addLineContentPtr+=2;
                    }
                } else if(currHunkType == HUNK_DELETE_TYPE && myoperation == DELETION_MODE){
                    // validate deletion chars bw diff and source
                    int track_new_del_line = fgetc(in);
                    *delLineContentPtr = diff_data_ptr;
                    delLineContentPtr++;
                    (*delBuffPtr)++;
                    if(diff_data_ptr != track_new_del_line){
                        if(!quietModeEnabled){
                            fprintf(stderr,"%s\n","Non matching del lines between source and diff");
                            hunk_show(&myHunk,stderr);
                        }
                        return -1;
                    } else if (track_new_del_line == '\n'){
                        src_file_line_ctr++; // increment source file line count
                        delBuffPtr = delLineContentPtr;
                        delLineContentPtr += 2;
                    }
                }
            }
        }

        // num arrows validation
        if(currHunkType == HUNK_CHANGE_TYPE){
            // validate insertion count
            if(currHunkInsertArrowCount != (hunkNewEnd - hunkNewStart + 1)) {
                if(!quietModeEnabled){
                    fprintf(stderr,"%s","Invalid num insertion arrow\n");
                    hunk_show(&myHunk,stderr);
                }
                return -1;
            } else if ((hunkOldEnd - hunkOldStart + 1) != currHunkDeleteArrowCount){ // validate deltion count
                if(!quietModeEnabled){
                    fprintf(stderr,"%s\n","invalid num deletion arrow");
                    hunk_show(&myHunk,stderr);
                }
                return -1;
            }
        } else if (currHunkType == HUNK_APPEND_TYPE){
            // printf("%d-----%d-----------------\n",currHunkInsertArrowCount,currHunkDeleteArrowCount);
        } else { // Deletion type

        }

        // get next hunk
        // printf("getting next hunk\n");

        // display addition/deletion counts
        // printf("Addition Total Lines: %d\n",currHunkInsertArrowCount);
        // printf("Deletion Total Lines: %d\n",currHunkDeleteArrowCount);
        currHunkDeleteArrowCount = 0; // reset arrow counts
        currHunkInsertArrowCount = 0;

        diff_t = '\n';
        diff_data_ptr = -1;
        diff_data_trailing_ptr = -1;
        res_hunk_next = hunk_next(&myHunk,diff);
    }
    // what if there are more chars from source????
    // write remaining source file contents to output
    int rem_chars = fgetc(in);
    while(rem_chars != EOF){
        fprintf(out,"%c",rem_chars);
        if(rem_chars == '\n'){
            src_file_line_ctr++;
        }
        rem_chars = fgetc(in);
    }

    return 0; // success
}

static int isDigit(char c){
    return c >= '0' && c <= '9';
}