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
//    rplcLin.h - header file for Linux specific functions for rplc.c
//
***************************************************************************/
#ifndef _RPLCLIN_H_
#define _RPLCLIN_H_

// Linux specific headers
#include<dirent.h>
#include<sys/types.h>
#include<unistd.h>
#include<errno.h>

// For raw (unbuffered) file I/O
#define FREQ_BINARY     0x00000000
#define FREQ_READ       O_RDONLY
#define FREQ_WRITE      O_WRONLY
#define FREQ_READWRITE  O_RDWR
#define FREQ_CREAT      O_CREAT
#define FREQ_TRUNC      O_TRUNC
#define FREQ_APPEND     O_APPEND

typedef mode_t          FREQ_MODE_T;
// dirname delimiter - forward slash character
#define DELIM    47
#define _stat64         stat64

/**************************************************************************
    Public Function Prototypes
***************************************************************************/
long SearchAllDirectories(const char *raw_in_file);
long SearchCurrentDirectory(
    const char *raw_in_file,
    const char *current_path);

/***************************************************************************
    Public Function Prototypes for Linux specific implementation
***************************************************************************/
long WriteableReplace(char *fname, unsigned fattrib);
long PrepNewTempFileName();
char* strerror_s(char* buff, int buffSz, int errNum);
int strcpy_s(char* dest, size_t destSz, const char* src);
int strncpy_s(char* dest, size_t destSz, const char* src, size_t numBytes);
int _access(const char* path, int mode);
int _remove(const char* fname);
int _rename(const char* oldName, const char* newName);
int _chmod(const char* fname, int mode);
int _chdir(const char* dirPath);
char* _getcwd(char* dirPath, int sz);

#endif
