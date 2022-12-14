/****************************************************************************
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
//  rplc.h - header file for system nonspecific functions for rplc.c
//
***************************************************************************/

#ifndef _RPLC_H_
#define _RPLC_H_

// Common headers
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include<stddef.h>
#include<fcntl.h>
#include<sys/stat.h>

#define ARSZ_MAX                32768  /* max length of search and replace strings */
#define MAX_FILES               3000   /* max number of files matching wildcards */
#define QUES_CHAR               63     /* '?' - char wildcard. */
#define STAR_CHAR               42     /* '*' - substring wildcard. */
#define FREQ_EIGHT_BIT_ASCII    0      // normal ASCII
#define FREQ_UNICODE            1      // Unicode search (two bytes per char)

const int MAX_DIRS_TO_OMIT      = 50;
const int MAX_FILE_PATTERNS_TO_OMIT = 50;
const int MAX_FILES_OR_DIRS_TO_CHANGE = 50000;

#define NL          10
#define CR          13

#ifndef _MAX_PATH
#define _MAX_PATH   255
#endif

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#define RPLC_ERR_MSG(a, b, c)   \
{                               \
    if (a != 99)                \
        fprintf(stderr, "\n    errnoMsg: %s\n", ErrNoMsg(errno)); \
    this_sucks(a, b, c);        \
    retVal = -1;                \
    goto LBL_END;               \
}

typedef char t_BOOL;

struct FilesDirs
{
    char path[_MAX_PATH];
    char name[_MAX_PATH];
    int posn;

    FilesDirs()
    {
        path[0] = 0;
        name[0] = 0;
        posn    = 0;
    }
};

/***************************************************************************
    Public Function Prototypes
***************************************************************************/
long ProcessFile(char *fname, unsigned fattrib);
void this_sucks(int i, int n, int line);
char* ErrNoMsg(int errNum);
void separate_filename_and_path(const char *path, char *dir, int dir_sz, char *last, int last_sz);
int AddToDirListIfNameMatch(char* current_path, char* name);
int AddToFileListIfNameMatch(const char* current_path, char* name);
int AddToListIfMatch(const char* current_path, char* name, FilesDirs arr[], long& cnt);

#endif // _RPLC_H_
