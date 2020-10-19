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
//  rplcWin.h - header file for Windows specific functions for rplc.c
//
***************************************************************************/

#ifndef _RPLCWIN_H_
#define _RPLCWIN_H_

// Windows specific headers
#include <windows.h>
#include <direct.h>
#include <winuser.h>
#include <io.h>

// For raw (unbuffered) file I/O
#define FREQ_BINARY     _O_BINARY
#define FREQ_READ       _O_RDONLY
#define FREQ_WRITE      _O_WRONLY
#define FREQ_READWRITE  _O_RDWR
#define FREQ_CREAT      _O_CREAT
#define FREQ_TRUNC      _O_TRUNC
#define FREQ_APPEND     _O_APPEND
typedef unsigned int    FREQ_MODE_T;

// dirname delimiter - backward slash character
#define DELIM       92
//#define stat64      __stat64

/***************************************************************************
    Public Function Prototypes
***************************************************************************/
long SearchAllDirectories(const char *raw_in_file);
long SearchCurrentDirectory(const char *raw_in_file, const char *current_path);

/***************************************************************************
         Functions Prototypes for Windows specific implementation
***************************************************************************/
long WriteableReplace(char *fname, unsigned fattrib);
long PrepNewTempFileName();
void SysError( char *fmt, ... );
//char* strerror(int errNum);

#endif // _RPLCWIN_H_
