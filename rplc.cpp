/****************************************************************************
// rplc -- utility program written by Gerhard Gross, 1995 - 1999.
//
// Copyright (c) 1995, 1996, 1997, 1998, 1999 Gerhard W. Gross.
//
// THIS SOFTWARE IS PROVIDED BY GERHARD W. GROSS ``AS IS'' AND ANY
// EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GERHARD W. GROSS 
// BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
// PERMISSION TO USE, COPY, MODIFY, AND DISTRIBUTE THIS SOFTWARE AND ITS
// DOCUMENTATION FOR ANY PURPOSE AND WITHOUT FEE IS HEREBY GRANTED,
// PROVIDED THAT THE ABOVE COPYRIGHT NOTICE, THE ABOVE DISCLAIMER
// NOTICE, THIS PERMISSION NOTICE, AND THE FOLLOWING ATTRIBUTION
// NOTICE APPEAR IN ALL SOURCE CODE FILES AND IN SUPPORTING
// DOCUMENTATION AND THAT GERHARD W. GROSS BE GIVEN ATTRIBUTION
// AS THE MAIN AUTHOR OF THIS PROGRAM IN THE FORM OF A TEXTUAL
// MESSAGE AT PROGRAM STARTUP OR IN THE DISPLAY OF A USAGE MESSAGE,
// OR IN DOCUMENTATION (ONLINE OR TEXTUAL) PROVIDED WITH THIS PROGRAM.
//
// ALL OR PARTS OF THIS CODE WERE WRITTEN BY GERHARD W. GROSS, 1995-1999.
//
//
// Function: Replaces one expression with another.
//
// Program: Opens fname for I/O.
//     Reads file size and creates 2 new char arrays dynamically.
//     If there is not enough room in RAM for the entire file
//     (times 2 - one for each buffer) the program automatically
//     reads in sections of the file whose size depends on the
//     amount of available RAM.  This repeats until the entire
//     file is _read and processed.  Provisions have been made
//     for the case where a matching string happens to fall across
//     section borders (with some blood, pain and tears).
//     Reads entire file contents into g_srcBuf[].
//     All characters except 'search_str' are copied into g_destBuf[].
//
// NOTE:
//
//     I changed this code in Oct. '97 to behave like normal UNIX utilities
//     by sending output to the standard output with printf commands instead
//     of writing directly to a file.  So there is no longer a destination
//     file argument on the command line.
//
// NOTE:
//
//     I changed this code in Dec. '97 back to the original way I had it,
//     where the output is written to the file named as the last command
//     line arg.  This is for PC use.  For the PC, I also had to go back
//     to buffered file I/O, since the raw I/O did not work as it was and
//     I did not have online help to fix it.
//
// NOTE:
//
//     In April, '98, I added Windows functions to this program that allow
//     the use of file wildcards so that multiple files can be processed
//     at the same time ( _findfile(), _wfindnext(), _findclose() ).  This
//     version works by replacing the original file with the contents
//     of the processed buffer.  The contents of the original file are
//     lost unless the -b option is used, which saves a copy of the
//     original file.
//
// *************************************************************************/

#if defined(_WIN32) || defined(_WIN64)
#include "rplcWin.h"
#else
#include "rplcLin.h"
#endif

//#include <share.h>
#include <sys/stat.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include "rplc.h"

using namespace std;

/***************************************************************************
         Global variables and macros
***************************************************************************/

#define SPACE_ASCII_INT           32
#define TAB_ASCII_INT             9
#define COMMA_ASCII_INT           44
#define NEWLINE_ASCII_INT         10
#define CR_ASCII_INT              13

char Raw_In_File[_MAX_PATH];
char Temp_File[_MAX_PATH];
char F_Arr[ARSZ_MAX], R_Arr[ARSZ_MAX];
char F_Arr_Case[ARSZ_MAX];
char F_Arr_NoWC[ARSZ_MAX];
char WC_Arr[ARSZ_MAX];
char Message[_MAX_PATH];
char* g_srcBuf                     = 0;
char* g_destBuf                    = 0;
long g_srchStrSz;
long g_rplcStrSz;
int Num_Backsteps;
long g_szFile;

char g_endLineMrk[] = {CR, NL, 0};

bool Integer                        = false;
bool WildCard                       = false;
bool To_Upper                       = false;
bool To_Lower                       = false;
bool Case_Insensitive               = false;
bool Verbose                        = false;
bool WholeWord                      = false;
bool Search_Subdirectories          = false;
int g_textEncoding                  = FREQ_EIGHT_BIT_ASCII;
bool g_noWrapNewLine                = false;
bool Instructions                   = false;
bool Backup                         = false;
bool Strip                          = false;
bool FileDirNames                   = false;
bool FileDirNamesExact              = false;
bool FileRenameDryRun               = false;
bool Split                          = false;
bool OverWriteAll                   = false;
bool ReadOnly                       = false;
bool Append                         = false;
bool ChngCaseOrAppend               = false;
bool First_Write                    = true;
bool First_Read                     = true;
bool SuppressErrorsPrintout         = true;
long g_filePos                      = 0;
long g_numDirsToOmit				= 0;
long g_numFilePtrnsToOmit           = 0;
long g_numLinesAtEndToRemove        = 0;
long g_numLinesAtFrontToRemove      = 0;
char g_filePtrnsToOmit[MAX_FILE_PATTERNS_TO_OMIT][_MAX_PATH];
char g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
FilesDirs g_dirs[MAX_FILES_OR_DIRS_TO_CHANGE];
FilesDirs g_files[MAX_FILES_OR_DIRS_TO_CHANGE];
long g_dirRenameCnt                   = 0;
long g_fileRenameCnt                  = 0;
char g_errBuf[_MAX_PATH];

long g_rplcCntTotal                 = 0;
long g_filesWithHitsCntr            = 0;
long g_filesSearchedCntr            = 0;
long g_numFilesChecked              = 0;
long g_numDeadSymLinks              = 0;

/***************************************************************************
    Private function prototypes
***************************************************************************/

//long wild_no_case(char *fname);
long append_only(char *fname);
long remove_last_lines_only(char *fname);
long remove_first_lines_only(char *fname);
long any_options(char *fname);
long HandleNoLineWrap( long blk, long& i, long& j, long& cnt, bool& skipLine);
void ResetVars();
int CountIntegerArgs(char *arr, char *str);
long measure_file(char *fname);
size_t read_file(const char *fname, char *s_buf, long off_st, size_t sz);
long send_to_output(char *d_buf, size_t sz);
void removeslash(char *strng);
int deal_with_options(int arrgc, char *arrgv[]);
long deal_with_fnames(int aargc, char *aargv[], char *working_dir, int working_dir_sz);
long get_mem(long blck);
long read_block(char *fname);
void fill_reverse_case_array(char *f_arr_case, const char *f_arr);
void fill_f_arr_no_wc(char* f_arr_nowc, const char* f_arr);
long count_arg_chars(char *aargv[]);
long ReplaceFilePrompt( char *fname, char& inputChar);
long NotWriteableReplace(char *fname, unsigned fattrib);
long move_temp_to_orig( char *fname, unsigned fattrib, bool& brplc);
void print_explanation();
void print_usage();
bool handle_nonsearch_requests(char* aargv[]);
long strip_chars(char* fname);
void ChangePathEndNames(FilesDirs arr[], long cnt);
void CopySearchArrIntoReplaceArr();

/***************************************************************************
    The command line options are first parsed and analyzed, then the command
    line file names and search and replace strings are parsed and put into
    local or global variables.  Dependent on whether the -R option was used,
    execution is directed to a function that recursively checks all
    subdirectories or a function that only checks the current directory.
***************************************************************************/

int main(int argc, char* argv[])
{
    char original_calling_dir[_MAX_PATH];
    char working_dir[_MAX_PATH];
    char Reset_Dir;
    long retVal, matchCnt;

    working_dir[0]  = 0;
    Reset_Dir       = 0;
    matchCnt        = 0;

    if ((argc = deal_with_options(argc, argv)) == -1)
        goto LBL_END;

    if (To_Upper || To_Lower)
        CopySearchArrIntoReplaceArr();

    if (deal_with_fnames(argc, argv, working_dir, _MAX_PATH) == -1)
        goto LBL_END;

    if (handle_nonsearch_requests(argv))
        return 0;

    if (count_arg_chars(argv) == -1)
        goto LBL_END;

    if (_getcwd(original_calling_dir, _MAX_PATH) == NULL)
        RPLC_ERR_MSG(15, 1, __LINE__);

    if (working_dir[0] != 0)
    {
        if (_chdir(working_dir) != 0)
            RPLC_ERR_MSG(24, 1, __LINE__);
        Reset_Dir = 1;
    }

    if (Search_Subdirectories)
    {
        if ((matchCnt = SearchAllDirectories(Raw_In_File)) == -1)
            goto LBL_END;
    }
    else
    {
        if ((matchCnt = SearchCurrentDirectory(Raw_In_File, working_dir)) == -1)
            goto LBL_END;
    }

    if (FileDirNames)
    {
        ChangePathEndNames(g_files, g_fileRenameCnt);
        ChangePathEndNames(g_dirs, g_dirRenameCnt);
    }

    if (Reset_Dir == 1)
    {
        if (_chdir(original_calling_dir) != 0)
            RPLC_ERR_MSG(24, 2, __LINE__);
    }

    // Print the total of all occurrences for all files searched
    printf("\n -- Found %ld matches in %ld files, %ld files searched, %ld files checked, %ld dead symlinks.\n",
        g_rplcCntTotal, g_filesWithHitsCntr, g_filesSearchedCntr, g_numFilesChecked, g_numDeadSymLinks);
LBL_END:

    return 0;
}

void CopySearchArrIntoReplaceArr()
{
    for (int i = 0; i < g_srchStrSz; ++i)
        R_Arr[i] = To_Upper ? toupper(F_Arr[i]) : tolower(F_Arr[i]);
    g_rplcStrSz = g_srchStrSz;
}

/***************************************************************************
This function is a command point that is repeated for every file.  The
buffer containing the replaced text is written to a temporary file in one
of the functions called here.  A different temporary file is used for each
call to this function.  This was required because the OS couldn't be relied
upon to delete the temp file fast enough between the processing of multiple
files by this program.  The temporary file is moved to the origianl file
specified on the command line in move_temp_to_orig(), called here.
Finally, a check is made at the end of this function to see whether the
temporary file still exists.  If it does, it is removed.
***************************************************************************/

long ProcessFile(char *fname, unsigned int fattrib)
{
    long retVal     = 0;
    long retVal2    = 0;
    bool brplc;

    ResetVars();
    g_szFile = measure_file(fname);
    
    if (g_szFile == 0)
    {
        //fprintf(stderr, "  0 bytes in file \"%s\"\n", fname);
        return 0;
    }

    if (!Append)
        if ((retVal = PrepNewTempFileName()) == -1)
            goto LBL_END;

    if (Append)
        retVal = append_only(fname);
    //else if (!ChngCaseOrAppend && !Case_Insensitive && !WholeWord)
    //    retVal = wild_no_case(fname);
    else
        retVal = any_options(fname);

    // Separate match count from possible error value
    if (retVal < 0)
        goto LBL_END;

    if (g_srcBuf != 0)
        free(g_srcBuf);
    if (g_destBuf != 0)
        free(g_destBuf);
    g_srcBuf = g_destBuf = 0;

    if (retVal > 0)
    {
        g_filesWithHitsCntr++;
        g_rplcCntTotal += retVal;
        // There has been at least one match found
        if ((retVal2 = move_temp_to_orig(fname, fattrib, brplc)) == -1)
        {
            retVal = retVal2;
            goto LBL_END;
        }
        else if (brplc == false)
            retVal = 0;
    }

    if (_access(Temp_File, 0) == 0)
    {
        if (remove(Temp_File) != 0) {
            strcpy_s(Message, _MAX_PATH, Temp_File);
            RPLC_ERR_MSG(16, 2, __LINE__);
        }
    }

    g_filesSearchedCntr++;

LBL_END:

    return retVal;
}

/***************************************************************************
    This is the workhorse for the append option. It appends the specified
    chars at the end of the file.
***************************************************************************/

long append_only(char *fname)
{
    long retVal = 0;
    errno = 0;
    ofstream outFile;

    outFile.open(fname, ios::out | ios::app | ios::binary);
    if (!outFile.good())
         RPLC_ERR_MSG(8, 12, __LINE__);

    outFile.write(F_Arr, g_srchStrSz);
    if (!outFile.good())
        RPLC_ERR_MSG(9, 10, __LINE__);

LBL_END:

    outFile.close();
    return retVal;
}

long load_file_into_mem(const char* fname, char** fileBuff)
{
    long retVal = 0;
    struct _stat64 statBuf;

    // stat file to get size
    statBuf.st_size = 0;
    if (_stat64(fname, &statBuf) != 0)
        RPLC_ERR_MSG(1, 1, __LINE__);

    // Allocate memory for file size
    if (!(*fileBuff = (char*)malloc(statBuf.st_size + 1)))
        RPLC_ERR_MSG(4, 1, __LINE__);

    // Read contents of file into buffer and close the file
    if ((retVal = read_file(fname, *fileBuff, 0, statBuf.st_size)) == -1)
        RPLC_ERR_MSG(4, 1, __LINE__);

LBL_END:

     return statBuf.st_size;
}

/***************************************************************************
 All chars between and including the startChar and endChar are stripped
 from the source file.
***************************************************************************/

long strip_chars(char* fname)
{
    long retVal = 0;
    char *srcBuff = 0, *destBuff = 0;
    long fileSz = 0;
    bool curStripEvent = false;
    long i, k;
    char startChar = 27, endChar = 109;

    fileSz = load_file_into_mem(fname, &srcBuff);

    // Allocate memory for file size
    if (!(destBuff = (char*)malloc(fileSz + 1)))
        RPLC_ERR_MSG(4, 1, __LINE__);

    for (i = 0, k = 0; i < fileSz; ++i)
    {
        if (srcBuff[i] == startChar || (curStripEvent && srcBuff[i] != endChar))
            curStripEvent = true;
        else if ((srcBuff[i] == endChar && curStripEvent))
            curStripEvent = false;
        else
            destBuff[k++] = srcBuff[i];
    }

    destBuff[k] = 0;

    // Open the same file for writing, truncating current contents
    strcpy_s(Temp_File, _MAX_PATH, fname);
    retVal = send_to_output(destBuff, (size_t)(k));

LBL_END:

    if (destBuff != 0)
        free(destBuff);
    if (srcBuff != 0)
        free(srcBuff);

    return retVal;
}

long remove_first_lines_only(char *fname)
{
    long retVal = 0;
    char *fileBuff = 0, *lastBytePtr, *cPtr;
    First_Write = true;
    long fileSz = 0;

    fileSz = load_file_into_mem(fname, &fileBuff);

    lastBytePtr = fileBuff + fileSz;
    cPtr = fileBuff;
    for (int i = 0; i < g_numLinesAtFrontToRemove; ++i)
    {
        if (cPtr < lastBytePtr)
            cPtr = strchr(cPtr, NEWLINE_ASCII_INT) + 1;
    }

    if (cPtr != 0)
    {
        // Open the same file for writing, truncating current contents
        strcpy_s(Temp_File, _MAX_PATH, fname);
        retVal = send_to_output(cPtr, (size_t)(lastBytePtr - cPtr));
    }

    if (fileBuff != 0)
        free(fileBuff);

    return retVal;
}

long remove_last_lines_only(char *fname)
{
    long retVal = 0;
    char* fileBuff, *cPtr;
    First_Write = true;
    long fileSz = 0;

    fileSz = load_file_into_mem(fname, &fileBuff);

    cPtr = fileBuff + fileSz;
    for (int i = 0; i < g_numLinesAtEndToRemove; ++i)
    {
        // Scroll backwards to get before any trailing CR or LN chars
        while (*cPtr == 0 || *cPtr == CR_ASCII_INT || *cPtr == NEWLINE_ASCII_INT)
            --cPtr;

        if (cPtr > fileBuff)
        {
            *(cPtr + 1) = 0;

            // strrchr backwards to char just before ASCII 10.
            // Back up one more byte if prev char is ASCII 13.
            cPtr = strrchr(fileBuff, NEWLINE_ASCII_INT);
            if (*(cPtr - 1) == CR_ASCII_INT)
                --cPtr;
        }
    }

    if (cPtr != 0)
    {
        *cPtr = 0;
        // Open the same file for writing, truncating current contents
        strcpy_s(Temp_File, _MAX_PATH, fname);
        retVal = send_to_output(fileBuff, (size_t)(cPtr - fileBuff));
    }

    if (fileBuff != 0)
        free(fileBuff);

    return retVal;
}

/***************************************************************************
    This is the workhorse for the WildCard option without case change option.
***************************************************************************/

long wild_no_case(char *fname)
    {
    long retVal, i, j, k, cnt, blk, match_cnt, wc_cnt;
    bool skipLine;

    retVal = i = j = k = cnt = blk = match_cnt = wc_cnt = 0;
    while(cnt < g_szFile)
    {
        k = Num_Backsteps = 0;
        if (cnt >= g_filePos)
        {
            if ((blk = read_block(fname)) == -1)
            {
                retVal = -1;
                goto LBL_END;
            }
            i = 0;
        }

        if (g_noWrapNewLine)
        {
            if ((retVal = HandleNoLineWrap(blk, i, j, cnt, skipLine)) == -1)
                goto LBL_END;
            else if (skipLine)
                continue;
        }

        if (g_srcBuf[i] == F_Arr[0] || (WildCard && F_Arr[0] == QUES_CHAR))
        {
            wc_cnt = 0;
            if (i + g_srchStrSz - 1 >= blk && Split && cnt + g_srchStrSz < g_szFile)
            {
                if ((retVal = send_to_output(g_destBuf, j)) == -1)
                    goto LBL_END;
                j = 0;
                Num_Backsteps = blk - i;
                if ((blk = read_block(fname)) == -1)
                {
                    retVal = -1;
                    goto LBL_END;
                }
                i = 0;
            }

            if (WildCard)
            {
                if (F_Arr[0] == QUES_CHAR)
                    WC_Arr[wc_cnt++] = g_srcBuf[i];
                for (k = 1; k < g_srchStrSz && ((g_srcBuf[i + k] == F_Arr[k]) || (F_Arr[k] == QUES_CHAR)); k++)
                {
                    if (F_Arr[k] == QUES_CHAR)
                        WC_Arr[wc_cnt++] = g_srcBuf[i + k];
                }
            }
            else {
                for (k = 1; (k < g_srchStrSz && (g_srcBuf[i + k] == F_Arr[k])); k++)
                { ; }
            }
        }

        if (k == g_srchStrSz)
        {
            // -------------------- Found match - replace search str to dest buf -----------------
            wc_cnt = 0;
            for (k = 0; k < g_rplcStrSz; k++)
            {
                if (WildCard && R_Arr[k] == QUES_CHAR && cnt + k <= g_szFile)    /* not EOF char */
                    g_destBuf[j++] = WC_Arr[wc_cnt++];
                else
                    g_destBuf[j++] = R_Arr[k];

                if (j == blk)
                {
                    if ((retVal = send_to_output(g_destBuf, j)) == -1)
                        goto LBL_END;
                    j = 0;
                }
            }

            i += g_srchStrSz;
            cnt += g_srchStrSz;
            match_cnt++;
        }
        else
        {
            // -------------------- Not a search str char - Copy src buf to dest buf -----------------
            cnt++;
            g_destBuf[j++] = g_srcBuf[i++];
            if (j == blk)
            {
                if ((retVal = send_to_output(g_destBuf, j)) == -1)
                    goto LBL_END;
                j = 0;
            }
        }
    }

    if ((retVal = send_to_output(g_destBuf, j)) == -1)
        goto LBL_END;
    if (retVal == 0)
        retVal = match_cnt;

LBL_END:

    return retVal;
}

/***************************************************************************
    This is the workhorse if more than one option is given. It can handle
    any or all of them but takes a little longer because of all the tests.
***************************************************************************/

long any_options(char *fname)
    {
    long retVal, i, j, k, cnt, blk, match_cnt, wc_cnt;
    bool skipLine;

    retVal = i = j = cnt = blk = wc_cnt = match_cnt = 0;
    while(cnt < g_szFile)
    {
        k = 0;
        Num_Backsteps = 0;
        if (cnt >= g_filePos)
        {
            if ((blk = read_block(fname)) == -1)
            {
                retVal = -1;
                goto LBL_END;
            }
            i = 0;
        }

        if (g_noWrapNewLine)
        {
            if ((retVal = HandleNoLineWrap(blk, i, j, cnt, skipLine)) == -1)
                goto LBL_END;
            else if (skipLine)
                continue;
        }

        if (g_srcBuf[i] == F_Arr[0] || (WildCard && F_Arr[0] == QUES_CHAR) ||
            (Case_Insensitive && (g_srcBuf[i] == F_Arr_Case[0])))
        {
            wc_cnt = 0;
            if (i + g_srchStrSz - 1 >= blk && Split && cnt + g_srchStrSz < g_szFile)
            {
                if ((retVal = send_to_output(g_destBuf, j)) == -1)
                    goto LBL_END;
                j = 0;
                Num_Backsteps = blk - i;
                if ((blk = read_block(fname)) == -1)
                {
                    retVal = -1;
                    goto LBL_END;
                }
                i = 0;
            }

            if (F_Arr[0] == QUES_CHAR)
            {
                WC_Arr[wc_cnt++] = g_srcBuf[i];
            }

            for (k = 1; k < g_srchStrSz &&
                  ( (g_srcBuf[i+k] == F_Arr[k]) ||
                    (WildCard && (F_Arr[k] == QUES_CHAR)) ||
                    (Case_Insensitive && (g_srcBuf[i+k] == F_Arr_Case[k])) ); k++)
            {
                    if (F_Arr[k] == QUES_CHAR)
                        WC_Arr[wc_cnt++] = g_srcBuf[i+k];
            }
        } // if (g_srcBuf[i] == F_Arr[0] || (WildCard && F_Arr[0] == QUES_CHAR) || ...
        if (k == g_srchStrSz && WholeWord)
        {
            if ( ((i != 0) && (isalpha(g_srcBuf[i-1]) || g_srcBuf[i-1] == '_' )) ||
                ( ((i+k) < (g_szFile)) && isalpha(g_srcBuf[i+k]) || g_srcBuf[i+k] == '_' ))
            {
                k = g_srchStrSz + 2;
            }
        }

        if (k == g_srchStrSz)
        {
            if (To_Upper || To_Lower)
            {
                for (k = 0; k < g_srchStrSz; k++)
                {
                    g_destBuf[j++] = To_Upper ? toupper(g_srcBuf[i++]) : tolower(g_srcBuf[i++]);
                    if (j == blk)
                    {
                        if ((retVal = send_to_output(g_destBuf, j)) == -1)
                            goto LBL_END;
                        j = 0;
                    }
                }
            }
            else
            {
                for (k = 0, wc_cnt = 0; k < g_rplcStrSz; k++)
                {
                    // not EOF char
                    if (WildCard && R_Arr[k] == QUES_CHAR && cnt + k <= g_szFile)
                        g_destBuf[j++] = WC_Arr[wc_cnt++];
                    else
                        g_destBuf[j++] = R_Arr[k];
                    if (j == blk)
                    {
                        if ((retVal = send_to_output(g_destBuf, j)) == -1)
                            goto LBL_END;
                        j = 0;
                    }
                }
                i += g_srchStrSz;
            }
            cnt += g_srchStrSz;
            match_cnt++;
        }
        else
        {
            g_destBuf[j++] = g_srcBuf[i++];
            cnt++;
            if (j == blk)
            {
                if ((retVal = send_to_output(g_destBuf, j)) == -1)
                    goto LBL_END;
                j = 0;
            }
        }
    }

    if ((retVal = send_to_output(g_destBuf, j)) == -1)
        goto LBL_END;
    if (retVal == 0)
        retVal = match_cnt;

LBL_END:

    return retVal;
}

/***************************************************************************
    Process commands that do more simple work on the input file instead of 
    searching and replacing.
***************************************************************************/

bool handle_nonsearch_requests(char* aargv[])
{
    if (g_numLinesAtEndToRemove > 0)
    {
        remove_last_lines_only(aargv[2]);
        return true;
    }
    else if (g_numLinesAtFrontToRemove  > 0)
    {
        remove_first_lines_only(aargv[2]);
        return true;
    }
    else if (Strip)
    {
        strip_chars(aargv[2]);
        return true;
    }

    return false;
}

/***************************************************************************
    Called from within search loop in the case where the g_noWrapNewLine
    command line option is selected.
***************************************************************************/

long HandleNoLineWrap(
    long blk,
    long& i,
    long& j,
    long& cnt,
    bool& skipLine)
{
    long retVal = 0;
    long m, lnsz, skipCnt;

    // Init out param
    skipLine = false;

    if ((lnsz = strcspn(&g_srcBuf[i], g_endLineMrk)) < g_srchStrSz)
    {
        // Search string can not span over multiple lines.
        // Jump char counter to char past the next end of line markers
        skipCnt = lnsz;
        while (g_srcBuf[i + skipCnt] == CR || g_srcBuf[i + skipCnt] == NL)
            skipCnt++;

        for (m = 0; m < skipCnt; m++)
        {
            g_destBuf[j++] = g_srcBuf[i++];
            cnt++;
            if (j == blk)
            {
                if ((retVal = send_to_output(g_destBuf, j)) == -1)
                    goto LBL_END;
                j = 0;
            }
        } // for (m = 0; m < skipCnt; m++)

        // Set return value so that the calling loop will
        // end current iteration.
        skipLine = true;

    } // if (g_noWrapNewLine && ...

LBL_END:

    return retVal;
}

/***************************************************************************
    Reset some global variables to process next file.
***************************************************************************/

void ResetVars()
{
    First_Write = 1;
    First_Read = 1;
    g_filePos = 0;
}

/***************************************************************************
    This function counts the number of integers in the command line
    arguments and puts them in an array (arr) declared in main.  The
    arguments should be separated by commas, spaces, or tabs.
***************************************************************************/

int CountIntegerArgs(char *arr, char *str)
{
    int retVal = 0;
    char *ptrs[ARSZ_MAX];
    int i = 0, j = 0, num, cnt = 0;

    // Remove leading whitespace (and commas)
    while (str[i] != 0 && (str[i] == SPACE_ASCII_INT || str[i] == TAB_ASCII_INT || str[i] == COMMA_ASCII_INT))
        { i++; }
    ptrs[j++] = &str[i];
    if (strlen(ptrs[j-1]) > 0)
        cnt = 1;
    while(str[i] != 0)
    {
        i++;
        // Get next integer when encounter delimiter (comma, space, tab)
        if (str[i] == SPACE_ASCII_INT || str[i] == TAB_ASCII_INT || str[i] == COMMA_ASCII_INT)
        {
            i++;
            while (str[i] != 0 && (str[i] == SPACE_ASCII_INT || str[i] == TAB_ASCII_INT || str[i] == COMMA_ASCII_INT))
                { i++; }
            if (str[i] != 0)
            {
                ptrs[j++] = &str[i];
                cnt++;
            }
        }
    }

    for (i = 0; i < cnt; i++)
    {
        // If first character is not a digit, 
        if (!isdigit(ptrs[i][0])) {
            cnt = 0;
            RPLC_ERR_MSG(32, 1, __LINE__);
        }
        num = atoi(ptrs[i]);
        if (num >= 0 && num < 256)
            arr[i] = (char)num;
        else
            RPLC_ERR_MSG(1, 1, __LINE__);
    }

    retVal = cnt;

LBL_END:

    return retVal;
}

char* ErrNoMsg(int errNum)
{
    strerror_s(g_errBuf, _MAX_PATH, errNum);
    return g_errBuf;
}

/***************************************************************************
    This function simply measures and returns the number of bytes in the
    file whose name is pointed to in the char pointer parameter.
***************************************************************************/

long measure_file(char *fname)
{
    errno = 0;
    struct _stat64 statBuf;

    if (_stat64(fname, &statBuf) != 0)
    {
        fprintf(stderr, "Can't stat\"%s\", errnoMsg: %s, line: %d", fname, ErrNoMsg(errno), __LINE__);
        return -1;
    }

    return statBuf.st_size;
}

/*
long measure_file(char *fname)
    {
    FILE *fp;
    long sze;
    
    if ((fp = fopen(fname, "rb")) == (FILE*)NULL)
    {
        fprintf(stderr, "Can't open \"%s\", may be busy. line:%d", fname, __LINE__);
        sze = -1;
    }
    else
    {
        fseek(fp,0L,SEEK_END);
        sze = (long)ftell(fp);
        fclose(fp);
    }
    return sze;
    }
*/


/***************************************************************************
    This function opens the specified file name, reads 'sz'
    number of bytes from this file and stores this data in the passed buffer
    s_buf, then closes the file.
***************************************************************************/

size_t read_file(const char *fname, char *s_buf, long off_st, size_t sz)
{
    long retVal = 0;
    size_t num_read;
    errno = 0;
    ifstream inFile;

    inFile.open(fname, ios::in | ios::binary);
    if (!inFile.good())
        fprintf(stderr, "line %d, Can't open %s, errnoMsg: %s", __LINE__, fname, ErrNoMsg(errno));
    else
    {
        inFile.read(s_buf, sz);
        num_read = inFile.gcount();
        if (num_read != sz)
            RPLC_ERR_MSG(10, 0, __LINE__);
    }

    s_buf[sz] = '\0';

    // No error so set return val
    retVal = (size_t)num_read;

LBL_END:

    inFile.close();
    return retVal;
}

/*
{
    size_t num_read;
    FILE *fp;
    if ((fp = fopen(fname, "rb")) == (FILE*)NULL)
        fprintf(stderr, "line %d, Can't open %s.  Is path correct?",
                     __LINE__, fname);
    fseek(fp, off_st, SEEK_SET);
    if ((num_read = fread(s_buf, sizeof(char), sz, fp)) != sz)
        RPLC_ERR_MSG(10, 0, __LINE__);
    s_buf[sz] = '\0';
    fclose(fp);
    return num_read;
}
*/

/***************************************************************************
    This function opens the file whose name is stored in Temp_File for
    writing if the 'First_Write' variable is set and for append if it is not.
    The first 'sz' bytes stored in 'd_buf' are written to the file.
    Raw (unbuffered) file I/O is used in this program as discussed at the
    top of the file.    This differs from the PC and Mac environment because
    there are file permisions that have to be dealt with.  To handle this
    the 'open' command takes a final argument that sets file permissions
    when used with the 'O_CREAT' mode bit.  Below I use the 644 permission
    for the temp file created.
    This function can be  modified so that the output is printed to
    standard output, instead of another file.  This makes it act more like
    other UNIX utilities.  The command line redirection symbol ('>') can
    be used to send the output to the desired file.
***************************************************************************/

long send_to_output(char *d_buf, size_t sz)
{
    long retVal = 0;
    //int filnum = 0;
    ofstream outFile;

    if (First_Write == 1)
    {
        //if ( (filnum = open(Temp_File, FREQ_BINARY |
        //                              FREQ_WRITE  |
        //                              FREQ_CREAT  |
        //                              FREQ_TRUNC  ,
        //                              (FREQ_MODE_T)0644)) == -1)
        errno = 0;
        outFile.open(Temp_File, ios::out | ios::trunc | ios::binary);
        if (!outFile.good())
            RPLC_ERR_MSG(8, 1, __LINE__);
    }
    else {
        //if ( (filnum = open(Temp_File, FREQ_BINARY |
        //                              FREQ_WRITE  |
        //                              FREQ_APPEND )) == -1)
        errno = 0;
        outFile.open(Temp_File, ios::out | ios::app | ios::binary);
        if (!outFile.good())
            RPLC_ERR_MSG(8, 2, __LINE__);
    }

    outFile.write(d_buf, sz);
    if (!outFile.good())
        RPLC_ERR_MSG(9, 0, __LINE__);

    outFile.close();
    First_Write = 0;

// ==> For output to standard out - UNIX
    
//   d_buf[sz] = '\0';
//   printf("%s", d_buf);

LBL_END:

    return retVal;
}

/***************************************************************************
    This function removes the second character from the passed string.  This
    is called from deal_with_options() to remove the special purpose slash
    character.
***************************************************************************/

void removeslash(char *strng)
    {
    int i, len;
    len = strlen(strng);
    for (i = 1; i < len; i++)
        strng[i] = strng[i + 1];
    }

/***************************************************************************
    This function checks all com line args to see if any begin with a
    hyphen.    If so the necessary flag is set.  argc and argv[] are adjusted
    accordingly, set back to the condition of the option not having been
    supplied at the com line (i.e. all except the first argv[] ptr are
    bumped back in the array).  This algorithm checks all arguments and
    slides the arg arrays back accordingly for each if they are option
    args.  Therefore, options can appear anywhere on the command line, in
    any order, and more than once (each option can be supplied separately).
    NOTE:
        For this program, it may be desired that the search or replace
        strings begin with a hyphen.    In this case, this function bypasses
        the option check for that arg if the second character is a slash
        ('/' ASCII 47) character.    The slash character is then removed
        from that arg with a call to the shift function.
***************************************************************************/

int deal_with_options(int arrgc, char *arrgv[])
{
    int retVal = 0;
    long i, j, num_opts;
    for (j = 1; j < arrgc; j++)
        if (*arrgv[j] == '-')
        {
            if (*(arrgv[j] + 1) == '/')
                removeslash(arrgv[j]);
            else
            {
                num_opts = strlen(arrgv[j]) - 1;
                for (i = 1; i <= num_opts; i++)
                {
                    switch(*(arrgv[j] + i))
                    {
                        case 'i':
                            Integer = true;
                            break;
                        case 'w':
                            WildCard = true;
                            break;
                        case 'u':
                            To_Upper = true;
                            ChngCaseOrAppend = true;
                            break;
                        case 'l':
                            To_Lower = true;
                            ChngCaseOrAppend = true;
                            break;
                        case 'a':
                            Append = true;
                            ChngCaseOrAppend = true;
                            break;
                        case 'c':
                             Case_Insensitive = true;
                            break;
                        case 'v':
                             Verbose = true;
                            break;
                        case 'b':
                            Backup = true;
                            break;
                        case 's':
                            Strip = true;
                            break;
                        case 'W':
                            WholeWord = true;
                            break;
                        case 'R':
                            Search_Subdirectories = true;
                            break;
                        case 'A':
                            OverWriteAll = true;
                            break;
                        case 'e':
                            SuppressErrorsPrintout = false;
                            break;
                        case 'U':
                             g_textEncoding = FREQ_UNICODE;
                             break;
                        case 'I':
                            Instructions = true;
                            break;
                        case 'n':
                            g_noWrapNewLine = true;
                            break;
                        case 'f':
                            FileDirNames = true;
                            break;
                        case 'F':
                            FileDirNames = true;
                            FileDirNamesExact = true;
                            break;
                        case 'N':
                            FileRenameDryRun = true;
                            break;
                        case 'o':
                            // This option must be succeeded with a space, then a string and then a space
                            if (g_numFilePtrnsToOmit + 1 >= MAX_FILE_PATTERNS_TO_OMIT)
                                fprintf(stderr, "Omitting too many file types - max %d", MAX_FILE_PATTERNS_TO_OMIT);
                            strcpy_s(g_filePtrnsToOmit[g_numFilePtrnsToOmit++], _MAX_PATH, (arrgv[j] + i + 2));

                            // Consume the directory name, decrement number of args, and move the loop counter
                            for (i = j + 1; i < arrgc - 1; i++)
                                arrgv[i] = arrgv[i + 1];
                            arrgc--;
                            i = num_opts;
                            break;
                        case 'O':
							// This option must be succeeded with a space, then a string and then a space
							if (g_numDirsToOmit + 1 >= MAX_DIRS_TO_OMIT)
								fprintf(stderr, "Omitting too many directories - max %d", MAX_DIRS_TO_OMIT);
							strcpy_s(g_dirsToOmit[g_numDirsToOmit++], _MAX_PATH, (arrgv[j] + i + 2));

							// Consume the directory name, decrement number of args, and move the loop counter
							for (i = j + 1; i < arrgc - 1; i++)
								arrgv[i] = arrgv[i + 1];
							arrgc--;
							i = num_opts;
							break;
                        case 'B':
                            // This option must be succeeded with a space, then an integer and then a space
                            g_numLinesAtFrontToRemove = atoi(arrgv[j] + i + 2);

                            // Consume the integer, decrement number of args, and move the loop counter
                            for (i = j + 1; i < arrgc - 1; i++)
                                arrgv[i] = arrgv[i + 1];
                            arrgc--;
                            i = num_opts;
                            break;
                        case 'E':
                            // This option must be succeeded with a space, then an integer and then a space
                            g_numLinesAtEndToRemove = atoi(arrgv[j] + i + 2);

                            // Consume the integer, decrement number of args, and move the loop counter
                            for (i = j + 1; i < arrgc - 1; i++)
                                arrgv[i] = arrgv[i + 1];
                            arrgc--;
                            i = num_opts;
                            break;
                        default:
                            RPLC_ERR_MSG(5, 1, __LINE__);
                            break;
                    } // switch(*(arrgv[j] + i))
                } // for (i = 1; i <= num_opts; i++)
                // Shift args
                for (i = j; i < arrgc - 1; i++)
                    arrgv[i] = arrgv[i + 1];
                arrgc--;
                j--;
            }
        } // if (*arrgv[j] == '-')

    // Make an option incompatibility check
    if ( (To_Upper + To_Lower + Append) > 1)
        RPLC_ERR_MSG(25, 0, __LINE__);
    if (Instructions)
        RPLC_ERR_MSG(99, 1, __LINE__);

    // No error. Set return value.
    retVal = arrgc;

LBL_END:

    return retVal;
}

/**************************************************************************************
 Seperate last item in path (file or dir name) from the path
**************************************************************************************/

void separate_filename_and_path(const char *path, char *dir, int dir_sz, char *last, int last_sz)
{
    const char *cPtr;

    dir[0] = 0;
    last[0] = 0;
    cPtr = strrchr(path, DELIM);

    if (cPtr != NULL)
    {
        strcpy_s(last, last_sz, cPtr + 1);
        strncpy_s(dir, dir_sz, path, cPtr - path);
        dir[cPtr - path] = 0;
    }
    else
        strcpy_s(last, last_sz, path);
}

/***************************************************************************
    This function was changed in Oct. 97 by removing any destination file
    artifacts.  The input file is replaced by the output.
***************************************************************************/

long deal_with_fnames(int aargc, char *aargv[], char *working_dir, int working_dir_sz)
{
    long retVal = 0;

    if (aargc == 1)
        RPLC_ERR_MSG(99, 1, __LINE__);
    if ((Strip || g_numLinesAtEndToRemove > 0 || g_numLinesAtFrontToRemove > 0) && aargc != 2)
        RPLC_ERR_MSG(3, 2, __LINE__);
    if (ChngCaseOrAppend && (aargc == 3))
        separate_filename_and_path(aargv[2], working_dir, working_dir_sz, Raw_In_File, _MAX_PATH);
    if (aargc == 4)
        separate_filename_and_path(aargv[3], working_dir, working_dir_sz, Raw_In_File, _MAX_PATH);

LBL_END:

    return retVal;
}

/***************************************************************************
    This function allocates memory for the working buffers used to hold the
    input file data and the processed output file data.  The sizes ('blck')
    are determined in another function.
***************************************************************************/

long get_mem(long blck)
{
    long retVal = 0;

    if (!(g_srcBuf = (char*)malloc(blck + 1)))
        RPLC_ERR_MSG(4, 1, __LINE__);
    if (!(g_destBuf = (char*)malloc(blck + 1)))
        RPLC_ERR_MSG(4, 2, __LINE__);

LBL_END:

    return retVal;
}

/***************************************************************************
    This function reads a block of data from the source file.
    - In Windows, virtual memory is used so there should be no problem
      requesting any size memory block.
    - a memory check is made to make sure there is enough memory for the
      two buffers as well as 51200 left over for any system stuff.
    - a check is made that there is more than 102400 free bytes in RAM.
    - if this is not the first _read and the file was split the last 100
      bytes from the last block are _read back into this block in case the
      split was made in the middle of a matching string.
    - give memory to g_srcBuf and g_destBuf.
    - _read blck bytes from In_File and store in g_srcBuf.
***************************************************************************/

long read_block(char *fname)
{
    long retVal     = 0;
    long blck       = g_szFile;
    long padding    = 512;
    long totSz      = 2 * (blck + padding);
    char* ptr;

    if (First_Read)
    {
#ifdef _WIN32

        if (!(ptr = (char*)malloc(totSz)))
            RPLC_ERR_MSG(4, 1, __LINE__);

        g_srcBuf     = ptr;
        g_destBuf    = ptr + (totSz / 2);

        if (!(g_destBuf = (char*)malloc(blck + 1)))
            RPLC_ERR_MSG(4, 2, __LINE__);
#else
        // Make memory block size originally large enough to hold
        // the whole file plus some overhead (rplc str > srch str)
        blck = g_szFile * 2 + 51200;

        // If not enough mem reduce requested size until it works.
        // The Windows OS should always grant the requested amount
        // the first time (virtually memory!)
        while(blck > 0 && !(g_srcBuf = (char*)malloc(blck)))
            blck -= 51200;

        if (g_srcBuf != 0)
            free(g_srcBuf);

        blck = (blck - 51200) / 2;
        
        if (blck < g_szFile && blck < 102400)
            RPLC_ERR_MSG(7, 1, __LINE__);

        if ((retVal = get_mem(blck + padding)) == -1)
            goto LBL_END;
#endif
    } // if (First_Read)

    if (blck < g_szFile)
    {
        Split = 1;
        if (!First_Read)
        {
            g_filePos -= Num_Backsteps;
            blck += Num_Backsteps;
        }
    }

    if ((retVal = read_file(fname, g_srcBuf, g_filePos, blck)) == -1)
        goto LBL_END;

    // Accumulate the file pos
    g_filePos += retVal;
    First_Read = 0;
    retVal = blck;

LBL_END:

    return retVal;
}

/***************************************************************************
    For cases when the Case_Insensitive option is used this function
    fills a char array with the reverse case of any alphabetic characters
    that are in the search array (f_arr).
***************************************************************************/

void fill_reverse_case_array(char *f_arr_case, const char *f_arr)
{
    long i;
    for (i = 0; i < g_srchStrSz; i++)
    {
        if (f_arr[i] >= 65 && f_arr[i] <= 90)        // uppercase
            f_arr_case[i] = f_arr[i] + 32;
        else if (f_arr[i] >= 97 && f_arr[i] <= 122)    // lowercase
            f_arr_case[i] = f_arr[i] - 32;
        else
            f_arr_case[i] = f_arr[i];                // non-alphabetic
    }
}

/***************************************************************************
    Create copy of F_Arr array but with wildcard chars removed - * and ?
***************************************************************************/

void fill_f_arr_no_wc(char* f_arr_nowc, const char* f_arr)
{
    long i, j;
    for (i = 0, j = 0; i < g_srchStrSz; i++)
    {
        if (f_arr[i] != QUES_CHAR && f_arr[i] != STAR_CHAR)
            f_arr_nowc[j++] = f_arr[i];
    }
    f_arr_nowc[j] = 0;
}

long count_arg_chars(char *aargv[])
{
    long retVal = -1;
    int i, j, len1, len2;

    if (Integer)
    {
        if ((g_srchStrSz = CountIntegerArgs(F_Arr, aargv[1])) == -1)
            goto LBL_END;

        if (!ChngCaseOrAppend)
        {
            if ((retVal = g_rplcStrSz = CountIntegerArgs(R_Arr, aargv[2])) == -1)
                goto LBL_END;
        }
    }
    else if (g_textEncoding == FREQ_EIGHT_BIT_ASCII)
    {
        // Make sure string not too large
        if (strlen(aargv[1]) >= ARSZ_MAX || strlen(aargv[2]) >= ARSZ_MAX)
            RPLC_ERR_MSG(26, 1, __LINE__);

        // Fill search array and get number of chars
        strcpy_s(F_Arr, ARSZ_MAX, aargv[1]);
        g_srchStrSz = strlen(F_Arr);

        if (!ChngCaseOrAppend)
        {
            // Fill replace array and get number of chars
            strcpy_s(R_Arr, ARSZ_MAX, aargv[2]);
            g_rplcStrSz = strlen(R_Arr);
        }
    }
    else if (g_textEncoding == FREQ_UNICODE)
    {
        // Make sure string not too large
        if ( (len1 = strlen(aargv[1])) >= ARSZ_MAX / 2 || (len2 = strlen(aargv[2])) >= ARSZ_MAX / 2)
            RPLC_ERR_MSG(26, 1, __LINE__);

        // Put a zero behind every character in search
        // string (Unicode).
        for (i = 0, j = 0; i < len1; i++, j++)
        {
            F_Arr[j++]  = aargv[1][i];
            F_Arr[j]    = 0;
        }

        // Get number of chars in search string
        g_srchStrSz = len1 * 2;

        if (!ChngCaseOrAppend)
        {
            // Put a zero behind every character in search string (Unicode)
            for (i = 0, j = 0; i < len2; i++, j++)
            {
                R_Arr[j++]  = aargv[2][i];
                R_Arr[j]    = 0;
            }

            // Get number of chars in search string
            g_rplcStrSz = len2 * 2;

        } // if (!ChngCaseOrAppend)
    } // else if (g_textEncoding == FREQ_UNICODE)

    if (Case_Insensitive == 1)
        fill_reverse_case_array(F_Arr_Case, F_Arr);

    if (FileDirNames)
        fill_f_arr_no_wc(F_Arr_NoWC, F_Arr);

    retVal = 0;

LBL_END:

    return retVal;
}

/***************************************************************************
Prompt the user to force replacing the file even though it does not have
write _access.    
***************************************************************************/

long ReplaceFilePrompt(char *fname, char& inputChar)
{
    long retVal = 0;
    char current_path[_MAX_PATH];
    
    if (_getcwd(current_path, _MAX_PATH) == NULL)
        RPLC_ERR_MSG(15, 1, __LINE__);
    do
    {
        fprintf(stderr, "  \"%s\\%s\" is not writeable!\n", current_path, fname);
        fprintf(stderr, "    Force overwrite? ('Y'es, 'N'o, 'A'll): ");
        fflush(NULL);
        fflush(stdin);
        inputChar = toupper(getchar());
    }while(inputChar != 'Y' && inputChar != 'N' && inputChar != 'A');

LBL_END:

    return retVal;
}

/***************************************************************************
The write _access of the passed in filename is enabled so the file can be
removed. The _access of the new file, of the same name, is then changed
back to _read only.
***************************************************************************/

long NotWriteableReplace(char *fname, unsigned fattrib)
{
    long retVal = 0;

    if (_chmod(fname, S_IWRITE) != 0)
        RPLC_ERR_MSG(18, 0, __LINE__);

    if ((retVal = WriteableReplace(fname, fattrib)) == -1)
        goto LBL_END;
//    if (_chmod(fname, S_IREAD) != 0)
//        RPLC_ERR_MSG(18, 1, __LINE__);

LBL_END:

    return retVal;
}

/***************************************************************************
This function is called if at least one occurrence of the search string
has been found.  A check is made to see if the file has write _access
(otherwise it can't be removed).  If it does not another function is called
that uses the __chmod() system call to change the file _access to 'writeable'.
The file is then removed and replaced with the new file.  The file _access
is then changed back. Of course, user permisions may prevent this.
***************************************************************************/

long move_temp_to_orig(char *fname, unsigned fattrib, bool& brplc)
{
    long retVal = 0;
    char chr, inputChar;

    brplc = TRUE;

    if (_access(fname, 2) == 0)
    {
        if ((retVal = WriteableReplace(fname, fattrib)) == -1)
            goto LBL_END;
    }
    else
    {
        if ((retVal = ReplaceFilePrompt(fname, inputChar)) == -1)
            goto LBL_END;
        chr = OverWriteAll == 1 ? 'Y' : inputChar;
        switch(chr)
        {
        case 'Y':
            if ((retVal = NotWriteableReplace(fname, fattrib)) == -1)
                goto LBL_END;
            break;
        case 'A':
            if ((retVal = NotWriteableReplace(fname, fattrib)) == -1)
                goto LBL_END;
            OverWriteAll = 1;
            break;
        case 'N':
            fprintf(stderr, "  ...File \"%s\" skipped!\n", fname);
            brplc = FALSE;
            if (remove(Temp_File) != 0) {
                strcpy_s(Message, _MAX_PATH, Temp_File);
                RPLC_ERR_MSG(16, 1, __LINE__);
            }
            break;
        default:
            break;
        }
    }

LBL_END:

    return retVal;
}


int AddToListIfMatch(const char* path, char* name, FilesDirs arr[], long& cnt)
{
    int retVal = -1;
    char* ptr = 0;

    if (cnt >= MAX_FILES_OR_DIRS_TO_CHANGE)
        RPLC_ERR_MSG(31, 0, __LINE__);

    if (FileDirNamesExact && (strcmp(name, F_Arr_NoWC) != 0))
        return retVal;
    else if ((ptr = strstr(name, F_Arr_NoWC)) == 0)
        return retVal;

    // The search string is contained in the file/dir name. Store dir and name.
    arr[cnt].posn = ptr - name;
    strcpy_s(arr[cnt].path, _MAX_PATH, path);
    strcpy_s(arr[cnt].name, _MAX_PATH, name);

    ++cnt;
    retVal = 0;

LBL_END:
    return retVal;
}

/***************************************************************************
 For all entries in the passed in array, replace the F_Arr portion with the
 R_Arr portion. The entries are either fie names or dir names.
***************************************************************************/

void ChangePathEndNames(FilesDirs arr[], long cnt)
{
    char part1[_MAX_PATH], part2[_MAX_PATH], src[_MAX_PATH], dest[_MAX_PATH];
    int len = strlen(F_Arr_NoWC);
    char strA[] = "Renamed";
    char strB[] = "Would rename";

    for (int i = 0; i < cnt; ++i)
    {
        part1[0] = part2[0] = 0;
        strncpy_s(part1, _MAX_PATH, arr[i].name, arr[i].posn);
        strcpy_s(part2, _MAX_PATH, arr[i].name + arr[i].posn + len);

        sprintf_s(src, _MAX_PATH, "%s/%s", arr[i].path, arr[i].name);
        sprintf_s(dest, _MAX_PATH, "%s/%s%s%s", arr[i].path, part1, R_Arr, part2);
        if (!FileRenameDryRun)
            rename(src, dest);
        printf("  !! %s %s to\n             %s\n", (FileRenameDryRun ? strB : strA), src, dest);
    }
}

/***************************************************************************
    This function reports errors and usage text to the output screen.
    If passed arg is 99, print usage and not error.
***************************************************************************/

void this_sucks(int i, int n, int line)
{
    if (SuppressErrorsPrintout)
        return;

    char buff[_MAX_PATH];
    if (Instructions)
    {
        print_explanation();
        goto LBL_END;
    }
    else if (i != 99)
        fprintf(stderr, "\n    Error %d-%d, line:%d\n    ", i, n, line);

    switch(i)
    {
        case 1:
            fprintf(stderr, "ASCII values must range from 0-255 inclusive.");
            break;
        case 3:
            fprintf(stderr, "Wrong number of arguments.");
            break;
        case 4:
            fprintf(stderr, "Not enough memory for files of this size.");
            fprintf(stderr, "\nSplit the file into one or more separate files.");
            fprintf(stderr, "\nTry again on each of the separate files.");
            break;
        case 5:
            fprintf(stderr, "That is not a valid option.");
            break;
        case 7:
            fprintf(stderr, "There ain't enough memory on this machine.");
            break;
        case 8:
            fprintf(stderr, "Can't open Temp_File: \"%s\".", Temp_File);
            break;
        case 9:
            fprintf(stderr, "File write.  Bytes: requested %d", n);
            break;
        case 10:
            fprintf(stderr, "File _read.  Bytes: requested %d", n);
            break;
        case 11:
            fprintf(stderr, "Could not perform 'del' shell command.");
            break;
        case 12:
            fprintf(stderr, "Could not perform 'move' shell command.\n");
            fprintf(stderr, "Processed file contents remain in %s.", Temp_File);
            break;
        case 13:
            fprintf(stderr, "Error with mkdir(TempPath).");
            break;
        case 14:
            fprintf(stderr, "Temporary file extension name too long (>3).");
            break;
        case 15:
            perror("Error calling \"_getcwd()\"");
            break;
        case 16:
            sprintf_s(buff, _MAX_PATH, "Error calling \"remove(%s)\"", Message);
            perror(buff);
            break;
        case 17:
            perror("Error calling \"rename()\"");
            break;
        case 18:
            perror("Error calling \"__chmod()\"");
            break;
        case 19:
            perror("Error calling \"_chdir()\"");
            break;
        case 20:
            fprintf(stderr, "Error calling \"fwprintf_s(()\".");
        case 21:
            fprintf(stderr, "Error calling \"SetFileAttributes()\".");
        case 22:
            fprintf(stderr, "Error calling \"CopyFile()\".");
        case 23:
            fprintf(stderr, "File name too long - must be less than %d.", _MAX_PATH);
            break;
        case 24:
            fprintf(stderr, "System call _chdir().");
            break;
        case 25:
            fprintf(stderr, "Incompatible options selected (-ula).");
            break;
        case 26:
            fprintf(stderr, "Search string too large.\n");
            break;
        case 27:
            fprintf(stderr, "Could not run stat()\n");
            g_numDeadSymLinks++;
            break;
        case 28:
            perror("Error calling \"link()\"");
            break;
        case 29:
            perror("Error calling \"sendfile()\"");
            break;
        case 30:
            perror("Error calling \"open()\"");
            break;
        case 31:
            fprintf(stderr, "Exceeding limit of file names to store - current: %d, max: %d", g_dirRenameCnt, MAX_FILES_OR_DIRS_TO_CHANGE);
            break;
        case 32:
            fprintf(stderr, "Integer argument is not an integer");
            break;

    } // switch(i)

LBL_END:

    fprintf(stderr, "\n");
    if (!Instructions)
        print_usage();
    exit(1);

    return;
}

/***************************************************************************
    This function prints notes to screen.
***************************************************************************/

void print_usage()
    {
    fprintf(stderr,
        " Usage:  rplc [-iwulacvbsefFNWARIOUBE] \"search_str\" \"replace_str\" \"fname\"\n\n"
        "  -i search and replace characters are ASCII integer values 0-255, space/comma delimited, quoted.\n"
        "  -w can use '?' as a character wildcard in 'search_str' and 'replace_str'. Must use quotes.\n"
        "  -u replace 'search_str' with uppercase.\n"
        "  -l replace 'search_str' with lowercase.\n"
        "  -a appends contents of 'search_str' to file. 'replace_str' not allowed with options 'u', 'l', or 'a'.\n"
        "  -c makes search case insensitive.\n"
        "  -v prints verbose output.\n"
        "  -b saves a backup copy of original file (*.rplc.bak).\n"
        "  -s strip unprintable chars generated from man/info commands 'search_str' and 'replace_str' are not allowed.\n"
        "  -f file and directory name replacement, not content replacement. Replaces first occurrence of search_str.\n"
        "  -e Print any error statements to console, suppress by default.\n"
        "  -F exact file and directory name replacement, not content replacement.\n"
        "  -N dry run for -f and -F options. Does not change names.\n"
        "  -W searches for whole word occurrences only.\n"
        "  -A Overwrite all _read only files without prompting. The Read only attribute is maintained.\n"
        "  -R recursively process 'fname' in all subdirectories.\n"
        "  -I print out verbose explanations and examples.\n"
        "  -O Dir Omit directory Dir from being searched. Can supply this option multiple times.\n"
        "  -U Unicode. File is in Unicode - Unicode replacement performed.\n"
        "  -B n  deletes the first n lines of the file. 'search_str' and 'replace_str' are not allowed.\n"
        "  -E n  deletes the last n lines of the file. 'search_str' and 'replace_str' are not allowed.\n\n"
        "     ****** Gerhard W. Gross ******\n\n");
    }

/***************************************************************************
This function prints notes to screen.
***************************************************************************/

void print_explanation()
{
    print_usage();
    printf("\n"
        " If 'search_str' or 'replace_str' include spaces or wildcard char '?', surround with double quotes.\n"
        "\n"
        " If a hyphen is used as the first char of 'search_str', proceed it with a slash character ('/') to\n"
        " differentiate it from an option. The slash is removed from the string internally.\n"
        " \n"
        " To delete 'search_str' use \"\" for 'replace_str' (empty string in quotes). For example, to delete\n"
        " every occurence of 'Horse' in the file 'fname':\n"
        "\n"
        " >   rplc Horse \"\" fname\n"
        "\n"
        " To replace every occurrence of 'Big cat' with 'Endangered cougar'\n"
        " in all files in the current directory with the extension '.doc':\n"
        "\n"
        " >   rplc \"Big cat\" \"Endangered cougar\" \"*.doc\"\n"
        "\n"
        " To replace every occurence of 'Safety' with 'Home safety' in all files\n"
        " with extension '.txt' in all subdirectories (recursively) from the\n"
        " current directory:\n"
        "\n"
        " >   rplc -R Safety \"Home safety\" \"*.txt\"\n"
        "\n"
        " To replace the bit pattern '10100100 10111011' with a '10001000 100000' in the file objects.bin\n"
        "\n"
        " >   rplc -i \"164 187\" \"136 32\"\n"
        "\n"
        " To replace every occurrence of a newline character with a carriage\n"
        " return newline pair in all files in the current directory\n"
        " (useful when moving text files from Unix to Windows):\n"
        "\n"
        " >   rplc -i 10 \"13,10\" \"*\"\n"
        "\n"
        " To replace every occurence of 'ish' and the character just before it with 'cat':\n"
        "\n"
        " >   rplc -w \"?ish\" cat fname\n"
        "\n"
        " To convert all characters in fname to lowercase and to save a backup\n"
        " copy of the original file (as \'fname.rplc.bak\'):\n"
        "\n"
        " >   rplc -lwb \"?\" fname\n"
        "\n"
        " To append a new page character (ASCII 12) to file fname followed by the string 'The End':\n"
        "\n"
        " >   rplc -ia 12 fname\n"
        " >   rplc -a \"The End\" fname\n"
        "\n"
        " To replace every occurence of 'America' with all uppercase ('AMERICA')\n"
        " automatically overwriting _read only files without prompting:\n"
        "\n"
        " >   rplc -uA America fname\n"
        "\n"
        " To replace every occurence of 'dog' with 'mutt' in Unicode file 'fname':\n"
        "\n"
        " >   rplc -U dog mutt fname\n"
        "\n"
        " To replace every occurence of \"amer???\" that occurs in any case,\n"
        " (upper or lower) where '?' represents any character, with 'United States':\n"
        "\n"
        " To recursively search and replace \"Dog\" with \"Cat\" in all files but\n"
        "  to omit DirX and DirY from the search and replace process:\n"
        "\n"
        " >   rplc -RO DirX -O DirY Dog Cat *\n"
        "\n\n"
        "     ****** Gerhard W. Gross ******\n"
    );
}
