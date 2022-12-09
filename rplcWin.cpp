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
// Functions that are specific to Windows are isolated in this file.
// There is an analogous file for Linux/Unice  specific code.  The
// reason that forces seperate code paths is subdirectory recursion.  Unices
// perform variable expansion from the command line by the shell.  Win32
// systems do not, but instead perform variable expansion within the
// "_findfirst" and "_findnext" functions.
//
***************************************************************************/

//#include <exception>
#include <windows.h>
#include <strsafe.h>
#include "rplcWin.h"
#include "rplc.h"

extern bool Verbose;
extern bool Backup;

// Using extern for Raw_In_File compiles but causes segmentation faults.
// Instead declare global variable here.  Can use const even though
// it gets set in a subsequent function (probably should not work but
// it does so I'll leave it).

const char *g_Raw_In_File;
extern bool FileDirNames;
extern FilesDirs g_dirs[MAX_FILES_OR_DIRS_TO_CHANGE];
extern FilesDirs g_files[MAX_FILES_OR_DIRS_TO_CHANGE];
extern long g_dirRenameCnt;
extern long g_fileRenameCnt;

extern char Temp_File[];
extern long g_numDirsToOmit;
extern long g_numFilePtrnsToOmit;
extern char g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
extern char g_filePtrnsToOmit[MAX_FILE_PATTERNS_TO_OMIT][_MAX_PATH];

/***************************************************************************
    Local function prototypes
***************************************************************************/
void PrintLastError(char* lpszFunction);

//char* strerror(int errNum)
//{
//    const int errMsgSz = 2048;
//    static char errMsg[errMsgSz];
//    errMsg[0] = 0;
//    strerror_s(errMsg, errMsgSz, errNum);
//    return errMsg;
//}

bool ShouldIgnoreThisFile(const _finddata_t &c_file)
{
	bool retVal = false;

	// See if this directory should be skipped
	for (int i = 0; i < g_numDirsToOmit; ++i)
	{
		if ((c_file.attrib & _A_SUBDIR) && strcmp(g_filePtrnsToOmit[i], c_file.name) == 0)
		{
			retVal = true;
			break;
		}
	}

	return retVal;
}

bool ShouldIgnoreThisDir(WIN32_FIND_DATA ffd)
{
    for (int i = 0; i < g_numDirsToOmit; ++i)
    {
        if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && strcmp(g_dirsToOmit[i], ffd.cFileName) == 0)
            return true;
    }

    return false;
}

/***************************************************************************
This recursive function is called for every subdirectory in the search
path.  When it finally encounters a directory with no subdirectories,
SearchCurrentDirectory() is called to do the actual file searching
dependent on the filename, with any wildcards, supplied on the command
line.
***************************************************************************/

long SearchAllDirectories(const char *raw_in_file)
{
    long retVal = 0;
    char current_path[_MAX_PATH];
    char all_wildcard[]    = "*";    // search through for all directories
    long hfile, fl_cnt    = 0;
	char errStr[_MAX_PATH];
    struct _finddata_t c_file;
	bool skipThisDir = false;

    if(_getcwd(current_path, _MAX_PATH) == NULL)
        RPLC_ERR_MSG(15, 0, __LINE__);
    if ((hfile = _findfirst(all_wildcard, &c_file)) == -1L && Verbose)
        fprintf(stderr, "  No \"%s\" files in %s, line %d\n", raw_in_file, current_path, __LINE__);
    else
    {
        do
        {
//          GWG - removed check for system file/dir because it prevented
//          search into Web dirs created with MS FrontPage.
//            if(!(c_file.attrib & _A_SYSTEM) && strcmp(c_file.name, ".") != 0
			if ((skipThisDir = ShouldIgnoreThisFile(c_file)))
                continue;

			if(strcmp(c_file.name, ".") == 0 || strcmp(c_file.name, "..") == 0 || strcmp(c_file.name, "") == 0)
                continue;

            if (FileDirNames)
                AddToListIfMatch(current_path, c_file.name, g_dirs, g_dirRenameCnt);
                
            if(c_file.attrib & _A_SUBDIR)
			{
				try
				{
					if(_chdir(c_file.name) != 0)
					{
						sprintf_s(errStr, _MAX_PATH, "  Attempting to _chdir to \"%s\" - ", c_file.name);
						throw errStr;
					}
				}
				catch (char * excp)
				{
					PrintLastError(excp);
					continue;
				}

				SearchAllDirectories(raw_in_file);
				try
				{
					if(_chdir(current_path) != 0)
					{
						sprintf_s(errStr, _MAX_PATH, "  Attempting to _chdir back to \"%s\" - ", current_path);
						throw errStr;
					}
				}
				catch (char * excp)
				{
					PrintLastError(excp);
					continue;
				}
			}
        } while (_findnext(hfile, &c_file) == 0);
    }

    _findclose(hfile);

	if (!skipThisDir)
		retVal = SearchCurrentDirectory(raw_in_file, current_path);

LBL_END:

    // Return the number of matches found or error
    return retVal;
}

/***************************************************************************
This function is called for every directory that has no subdirectories or
if the -R command line option was not supplied.  The current directory is
searched for files that match the file name, with any wildcards, given at
the command line.  For each of those files, ProcessFile() is called which
does the actual search and replace of the command line specified strings.
The file attributes are saved in a local array "AllFileAttribs[]" so they
can be used to set the attributes of the new file.  This is done so that
the original file attributes are unchanged after using this utility, even
though they may be changed (e.g. from read only) temporarily during
program execution.
In order to run this utility successfully in directories with many files
that match the file being searched for, MAX_FILES is defined fairly large.
I attempted to allocate space dynamically after counting the files in the
directory but this caused crashes when doing recursive searches - somewhere
in the call to free().  So I changed back to static allocation.
***************************************************************************/

long SearchCurrentDirectory(const char *raw_in_file, const char *current_path)
{
    long retVal = 0;
    char In_File[MAX_FILES][_MAX_PATH] = { 0 };
    unsigned AllFileAttribs[MAX_FILES] = { 0 };
    long hfile;
    long i, fl_cnt = 0;
    struct _finddata_t c_file;

    if ((hfile = _findfirst(raw_in_file, &c_file)) == -1L)
    {
        if (Verbose && !FileDirNames)
            fprintf(stderr, "  Could not find \"%s\" in: %s\n", raw_in_file, current_path);
    }
    else
    {
        do
        {
            if (!(c_file.attrib & _A_SYSTEM) && !(c_file.attrib & _A_SUBDIR) &&
                strcmp(c_file.name, ".") != 0 && strcmp(c_file.name, "..") != 0 && strcmp(c_file.name, "") != 0)
            {
                {
                    AllFileAttribs[fl_cnt] = c_file.attrib;
                    if (strlen(c_file.name) >= _MAX_PATH - 1)
                        RPLC_ERR_MSG(23, 0, __LINE__);
                    strcpy_s(In_File[fl_cnt++], _MAX_PATH, c_file.name);
                }
            }
        } while (_findnext(hfile, &c_file) == 0);
    }
    _findclose(hfile);

    for (i = 0; i < fl_cnt; i++)
    {
        if (FileDirNames)
            AddToListIfMatch(current_path, In_File[i], g_files, g_fileRenameCnt);
        else if ((retVal = ProcessFile(In_File[i], AllFileAttribs[i])) == -1)
            goto LBL_END;

        if (retVal > 0 || (Verbose && retVal > -1))
        {
            retVal > 0 ? printf(" +") : printf("  ");
            printf("%d instance(s) replaced in:  \"%s\\%s\"\n", retVal, current_path, In_File[i]);
        }
    }

LBL_END:

    return retVal;
}

/***************************************************************************
     This function prints an error message and the system error message
     using perror().  exit() is not called from here so execution continues.
***************************************************************************/

//void SysError( char *fmt, ... )
//{
//    char buff[256];
//    va_list    va;
//
//    va_start(va, fmt);
//    wvsprintf( buff, fmt, va );
//    perror(buff);
//}

/***************************************************************************
The passed in file is removed and the Temp file is renamed with the name of
the passed file.
***************************************************************************/

long WriteableReplace(char *fname, unsigned fattrib)
{
    long retVal = 0;
    char backupName[_MAX_PATH];
    wchar_t *wFname = 0;
    size_t charsConverted, len;

    strcpy_s(backupName, _MAX_PATH, fname);
    strcat_s(backupName,  _MAX_PATH, ".rplc.bak");
    remove(backupName);
    if(rename(fname, backupName) != 0)
        RPLC_ERR_MSG(17, 0, __LINE__);

    if(rename(Temp_File, fname) != 0)
        RPLC_ERR_MSG(17, 1, __LINE__);
    len = strlen(fname) + 1;
    wFname = new wchar_t[len];
    mbstowcs_s(&charsConverted, wFname, len, fname, len- 1);
    if(SetFileAttributesW(wFname, (unsigned long)fattrib) == 0)
        RPLC_ERR_MSG(21, 0, __LINE__);

    if (Backup == 1)
        fprintf(stderr, "Saved backup of original file in %s\n", backupName);
    else if(remove(backupName) != 0)
        RPLC_ERR_MSG(16, 0, __LINE__);

LBL_END:

    if (wFname != 0)
        delete[] wFname;
    return retVal;
}

/***************************************************************************
    Create a new Temporary file name with path to c:\temp\.    Increnment the
    extension by one each time, resetting to 0 after 999 so the extension
    is limited to 3 characters.  The Temp Filename should look like this:
        c:\Temp\rplc004.tmp
    where 4 is replaced with whatever the current value of the file counter.
***************************************************************************/

long PrepNewTempFileName()
{
    long retVal                 = 0;
    static long GlobalFileCntr  = 0;
    const int _10_BYTES_SZ      = 10;
    char TempPath[]             = "/Temp";
    char BaseFileName[]         = "/rplc000";
    char FileNameExt[]          = ".tmp";
    char FileNameCnt[_10_BYTES_SZ];
    size_t ExtLen, TempLen;
    if(_access(TempPath, 0) != 0)
    {
        if(_mkdir(TempPath) != 0)
            RPLC_ERR_MSG(13, 0, __LINE__);
    }
    strcpy_s(Temp_File, _MAX_PATH, TempPath);
    strcat_s(Temp_File, _MAX_PATH, BaseFileName);
    _ltoa_s(GlobalFileCntr++, FileNameCnt, _10_BYTES_SZ, 10);
    if((ExtLen = strlen(FileNameCnt)) > 3)
        RPLC_ERR_MSG(14, 0, __LINE__);
    TempLen = strlen(Temp_File);
    memcpy(&Temp_File[TempLen-ExtLen], FileNameCnt, ExtLen);
    strcat_s(Temp_File, _MAX_PATH, FileNameExt);
    if(GlobalFileCntr == 999)
        GlobalFileCntr = 0;

LBL_END:

    return retVal;
}

//void PrintLastError(LPTSTR lpszFunction)
void PrintLastError(char* lpszFunction)
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(wchar_t)); 
    StringCchPrintf((LPTSTR)lpDisplayBuf, 
        LocalSize(lpDisplayBuf) / sizeof(wchar_t),
        TEXT("%s failed with error %d: %s"), 
        lpszFunction, dw, lpMsgBuf); 
    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
	fprintf(stderr, "  GetLastError: %s\n", (char*)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    //ExitProcess(dw); 
}
