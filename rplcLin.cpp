/****************************************************************************
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
// MESSAGE AT PROGRAM STAR_CHARTUP OR IN THE DISPLAY OF A USAGE MESSAGE,
// OR IN DOCUMENTATION (ONLINE OR TEXTUAL) PROVIDED WITH THIS PROGRAM.
//
// ALL OR PARTS OF THIS CODE WERE WRITTEN BY GERHARD W. GROSS, 1995-1999.
//
//
// Functions that are specific to Linux and other Unices are isolated in
// this file.  There is an analogous file for Windows specific code.  The
// reason that forces seperate code paths is subdirectory recursion.  Unices
// perform variable expansion from the command line by the shell.  Win32
// systems do not, but instead perform variable expansion within the
// "_wfindfirst" and "_wfindnext" functions.
// 
// In order to take advantage of wildcard expansion this program requires
// that the file name given on the command line be enclosed in single or
// double quotes in order to suppress shell wildcard expansion.
***************************************************************************/

#include "rplcLin.h"
#include "rplc.h"
#include <sys/sendfile.h>

void print_usage();

extern char F_Arr[ARSZ_MAX];
extern char F_Arr_NoWC[ARSZ_MAX];
extern bool Verbose;
extern bool Backup;
extern bool FileDirNames;
extern long g_numDirsToOmit;
extern char g_dirsToOmit[MAX_DIRS_TO_OMIT][_MAX_PATH];
extern char Temp_File[];
extern FilesDirs g_dirs[MAX_FILES_OR_DIRS_TO_CHANGE];
extern FilesDirs g_files[MAX_FILES_OR_DIRS_TO_CHANGE];
extern long g_dirRenameCnt;
extern long g_fileRenameCnt;
extern long g_numFilesChecked;

// Using extern for Raw_In_File compiles but causes segmentation faults.
// Instead declare global variable here.  Can use const even though
// it gets set in a subsequent function (probably should not work but
// it does so I'll leave it).

char g_Raw_In_File[_MAX_PATH];
struct _stat64 g_statBuf;

/***************************************************************************
    Private function prototypes
***************************************************************************/

int SelectValidDir(const struct dirent *namelist);
int SelectFileMatch(const struct dirent *namelist);
inline int strcnt(const char *str, int chr, int *idx);
bool ShouldIgnoreThisDir(const char* fname);

/***************************************************************************
 These MS functions exist for Windows but not Linux so implement here!
 Returns -1 on error, like if there is not enough room to add null char.
***************************************************************************/

int _remove(const char* fname)
{
    return remove(fname);
}

int _rename(const char* oldName, const char* newName)
{
    return rename(oldName, newName);
}

int _chmod(const char* fname, int mode)
{
    return chmod(fname, mode);
}

int _access(const char* path, int mode)
{
    return access(path, mode);
}

int _chdir(const char* dirPath)
{
    return chdir(dirPath);
}

char* _getcwd(char* dirPath, int sz)
{
    return getcwd(dirPath, sz);
}

char* strerror_s(char* buff, int buffSz, int errNum)
{
   buff[0] = 0;
   char* msg = strerror(errno);
   int msgSz = strlen(msg);
   if (msgSz < buffSz)
       strcpy(buff, msg);
   return buff;
}

int strcpy_s(char* dest, size_t destSz, const char* src)
{
    int srcLen = strlen(src);
    if ( destSz - 1 < srcLen) return -1;
    strcpy(dest, src);
    dest[srcLen] = 0;
}

int strncpy_s(char* dest, size_t destSz, const char* src, size_t numBytes)
{
    if ( destSz - 1 < numBytes) return -1;
    strncpy(dest, src, numBytes);
    dest[numBytes] = 0;
}

bool ShouldIgnoreThisDir(const char* fname)
{
    for (int i = 0; i < g_numDirsToOmit; ++i)
    {
        if (strcmp(g_dirsToOmit[i], fname) == 0)
            return true;
    }

    return false;
}

/***************************************************************************
This recursive function is called for every subdirectory in the search
path.  When it finally encounters a directory with no subdirectories,
SearchCurrentDirectory() is called to do the actual file searching
dependent on the filename, with any file cards, supplied on the command
line.
***************************************************************************/

long SearchAllDirectories(const char *raw_in_file)
{
    long retVal = 0;
    int i;
    int dirCnt;
    char *fName;
    char current_path[_MAX_PATH];
    char all_wildcard[]   = "*";    // search through for all directories
    struct dirent **namelist;

    dirCnt = 0;

    if(getcwd(current_path, _MAX_PATH) == NULL)
        RPLC_ERR_MSG(7, 0, __LINE__);

    if ((dirCnt = scandir(current_path, &namelist, SelectValidDir, alphasort)) == -1)
    {
        fprintf(stderr, "  Error scanning directory \"%s\". Skip...\n", current_path);
//        RPLC_ERR_MSG(9, 2, __LINE__);
    }

    if (dirCnt > 0)
    {
        for (i = 0; i < dirCnt; i++)
        {
            fName = namelist[i]->d_name;

            if (ShouldIgnoreThisDir(fName))
                continue;

            if (FileDirNames)
                AddToListIfMatch(current_path, fName, g_dirs, g_dirRenameCnt);
            
            if(chdir(fName) != 0)
                RPLC_ERR_MSG(8, 0, __LINE__);

            retVal = SearchAllDirectories(raw_in_file);

            if(chdir(current_path) != 0)
                RPLC_ERR_MSG(8, 1, __LINE__);
        }
    }

    retVal = SearchCurrentDirectory(raw_in_file, current_path);

LBL_END:
    return retVal;
}

/***************************************************************************
This function is called for every directory that has no subdirectories or
if the -R command line option was not supplied.  The current directory is
searched for files that match the file name, with any wildcards, given at
the command line.  For each of those files, ProcessFile() is called which
does the actual search and replace in the file contents of the command
line specified strings.
***************************************************************************/

long SearchCurrentDirectory(const char *raw_in_file, const char *current_path)
{
    long retVal = 0;
    int i;
    int fileCnt;
    char *fName;
    long match_cnt;
    struct dirent **namelist;
    char cur_path_tmp[_MAX_PATH];
    struct stat statBuf;

    fileCnt         = 0;
    cur_path_tmp[0] = 0;

    if (FileDirNames)
        sprintf(g_Raw_In_File, "%s%s%s", raw_in_file, raw_in_file[0] == 0 ? "" : "/", F_Arr);
    else
        sprintf(g_Raw_In_File, "%s", raw_in_file);

    if (current_path[0] != 0)
    {
        strcpy(cur_path_tmp, current_path);
        strcat(cur_path_tmp, "/");
    }

    if ((fileCnt = scandir(".", &namelist, SelectFileMatch, alphasort)) == -1)
    {
        fprintf(stderr, "  Error scanning directory \"%s\". Skip...\n", cur_path_tmp);
//        RPLC_ERR_MSG(9, 1, __LINE__);
    }
    else if (fileCnt == 0 && Verbose && !FileDirNames)
        fprintf(stderr, "  Did not find \"%s\" in: %s\n", raw_in_file, cur_path_tmp);
    else
    {
        for (i = 0; i < fileCnt; i++)
        {
            fName = namelist[i]->d_name;
            if (stat(fName, &statBuf) != 0)
            {
                errno = 0;
                printf("Could not run \"stat()\" on %s. Possibly dead symlink. File skipped.\n", fName);
                printf("\t(file: %s, line: %d)\n", __FILE__, __LINE__);
            }
            else if (FileDirNames)
                AddToListIfMatch(current_path, fName, g_files, g_fileRenameCnt);
            else {
                match_cnt = ProcessFile(fName, statBuf.st_mode);
                if(match_cnt > 0 || (Verbose && match_cnt > -1))
                {
                    match_cnt > 0 ? printf(" +") : printf("  ");
                    printf("%d instance(s) replaced in:  %s%s\n", match_cnt, cur_path_tmp, fName);
                }
            }
        }
    }
    return match_cnt;
}

/***************************************************************************
Get all directories in current directory except "." and ".."..
***************************************************************************/

int SelectValidDir(const struct dirent *namelist)
{
    int retMatch  = 0;
    t_BOOL skip     = FALSE;
    struct stat statBuf;
    const char *fName;

    // Include all directories except "." and ".."

    fName = namelist->d_name;
    if (stat(fName, &statBuf) != 0)
    {
        errno = 0;
        printf("Could not run \"stat()\" on %s.  Possibly ", fName);
        printf("dead symlink.  File skipped.\n");
        printf("\t(file: %s, line: %d)\n", __FILE__, __LINE__);
        skip = TRUE;
    }
    if (skip == FALSE && S_ISDIR(statBuf.st_mode))
    {
        if ((strcmp(".", fName) != 0) && (strcmp("..", fName) != 0))
            retMatch = 1;
    }

    return retMatch;
}

/***************************************************************************
Determine if the command line file name (with or without "*", "?"
wildcards) matches the passed in file name from the scandir() list.
Return 1 if a match is found and 0 otherwise.

This function takes the place of shell variable expansion of the "*" and
"?" wildcard characters.  The file name on the command line is enclosed
in quotes so the shell does not expand the wildcard characters.  This
makes it possible to search subdirectories recursively.  (When the shell
expands wildcards it uses the files in the current directory as the
selection pool.  Recursion requires that the contents of each recursive
subdirectory also be included in the file pool.)

The stat() function fails if the file name parameter is a symbolic link
that points to a non-existent file.  If stat() fails the external
variable "errno" gets set to some non-zero value and therefore causes
scandir() to fail.  Therefore, errno is explicitly set to zero if stat()
fails so scandir() does not fail needlessly.
***************************************************************************/

int SelectFileMatch(const struct dirent *namelist)
{
    int retMatch         = 0;
    int iC               = 0;
    int iA               = 0;
    int lastIdx          = 0;
    int starCntr         = 0;
    t_BOOL skip          = FALSE;
    const char *argFName = g_Raw_In_File;
    const char *curFName = namelist->d_name;
    const char *ptr;
    int numStars;
    struct stat statBuf;
    size_t argLen;
    size_t curLen;

    numStars = strcnt(argFName, STAR_CHAR, &lastIdx);
    if (stat(curFName, &statBuf) != 0)
    {
        errno = 0;
        skip = TRUE;
        printf("Could not run \"stat()\" on %s.  Possibly ", curFName);
        printf("dead symlink.  File skipped.\n");
        printf("\t(file: %s, line: %d)\n", __FILE__, __LINE__);
    }
    // Only accept "REG"ular files
    if (skip == FALSE && S_ISREG(statBuf.st_mode))
    {
        retMatch = 1;
        argLen = strlen(argFName);
        curLen = strlen(curFName);
        while (iC < curLen && iA < argLen && retMatch != 0)
        {
            if(argFName[iA] == STAR_CHAR)
            {
                if (iA + 1 < argLen)
                {
                    if (starCntr + 1 >= numStars)
                        iC = curLen - (argLen - iA - 1);
                    else {
                        ptr = strstr(curFName, F_Arr_NoWC);
                        if (ptr == 0)
                           retMatch = 0;
                        else
                            iC = ptr - curFName;
                        if (iC >= curLen)
                           retMatch = 0;
                        starCntr++;
                        iC++;
                        iA++;
                    }
                }
            }
            else if(argFName[iA] == QUES_CHAR)
                iC++;
            else if (argFName[iA] != curFName[iC++])
                retMatch = 0;
            iA++;

            if (iA >= argLen && retMatch != 0)  // end effects of argFName
            {
                if ((argFName[iA - 1] != STAR_CHAR && iC < curLen) || (argFName[iA - 1] == QUES_CHAR && curLen != iC))
                    retMatch = 0;
            }
            if (iC >= curLen && retMatch != 0)  // end effects of curFName
            {
                if (iA < argLen && argFName[iA] != STAR_CHAR) {
                    retMatch = 0;
                }
            }
        }
        g_numFilesChecked++;
    }
    return retMatch;
}

/***************************************************************************
    Count the occurrences of "chr" in "str" and put the array index number
    of the last occurrence of "chr" in "idx".  Return the number of
    occurrences.
***************************************************************************/

inline int strcnt(const char *str, int chr, int *idx)
{
    int i;
    int strLen;
    int cnt = 0;

    *idx = 0;
    strLen = strlen(str);

    for (i = strLen-1; i >= 0; i--)
    {
        if (str[i] == chr)
        {
            if (cnt == 0)
                *idx = i;
            cnt++;
        }
    }
    return cnt;
}

/***************************************************************************
    Calling link() does not work under certain conditions, like some
    links or symlinks. This approach does a file copy instead at the
    kernel level so is very fast.
***************************************************************************/

long CopyFileKernelLevel(const char* srcFl, const char* destFl)
{
    long retVal = 0;
    int source, dest;
    errno = 0;


    if ((source = open(srcFl, O_RDONLY, 0)) == -1)
        RPLC_ERR_MSG(30, 0, __LINE__);
    if ((dest = creat(destFl, O_RDWR)) == -1)
    {
        fprintf(stderr, "\n\tError opening %s, srcFl: %s\n", destFl, srcFl);
        exit(0);
        //RPLC_ERR_MSG(30, 0, __LINE__);
    }

    // struct required, rationale: function stat() exists also
    struct stat64 stat_source;
    fstat64(source, &stat_source);

    if (sendfile(dest, source, 0, stat_source.st_size) != stat_source.st_size)
        RPLC_ERR_MSG(29, 0, __LINE__);

    close(source);
    close(dest);

LBL_END:

    return retVal;
}

/***************************************************************************
The passed file is removed and the Temp file is renamed with the name of
the passed file.  The original file, "fname", is moved to the temporary
name fnameXXX.tmp and then deleted at the end of the function.  This helps
ensure that the original file is not lost if something goes wrong.
***************************************************************************/

long WriteableReplace(char *fname, unsigned fattrib)
{
    long retVal = 0;
    char bakName[_MAX_PATH];

    if (Backup)
    {
        errno = 0;
        strcpy(bakName, fname);
        strcat(bakName, ".rplc.bak");
        errno = 0;
        CopyFileKernelLevel(fname, bakName);
        fprintf(stderr, "Saved backup of original file in %s\n", bakName);
    }

    if(_remove(fname) != 0)
        RPLC_ERR_MSG(16, 0, __LINE__);
    CopyFileKernelLevel(Temp_File, fname);
    if(chmod(fname, fattrib) != 0)
        RPLC_ERR_MSG(18, 2, __LINE__);

/*
    // This block fails when trying to rename across symlinks
    if(link(fname, bakName) != 0)
        RPLC_ERR_MSG(28, 0, __LINE__);
    if(_remove(fname) != 0)
        RPLC_ERR_MSG(16, 0, __LINE__);
    if(_rename(Temp_File, fname) != 0)
        RPLC_ERR_MSG(17, 1, __LINE__);
    if(chmod(fname, fattrib) != 0)
        RPLC_ERR_MSG(18, 2, __LINE__);
*/

LBL_END:

    return retVal;
}

/***************************************************************************
    Create a new Temporary file name with path to c:\temp\.    Increnment the
    extension by one each time, resetting to 0 after 999 so the extension
    is limited to 3 characters.  The Temp Filename should look like this:

        .\rplc004.tmp

    if 4 is the current value of the file counter.
    In the Unices environments the temporary files are stored in the local
    directory.  I tried to use the /tmp directory but found it is not
    possible to rename a file crossing file system (device) boundaries.
    Some hard disks may be set up where /tmp is on a different device
    (such as mine).  This can't be done in Windows because the _wfindnext()
    functions are in the middle of scanning directories when the new file
    will be created.  Linux can use scandir() to do a complete directory
    scan before the function exits.
***************************************************************************/

void CreateSingleDirIfNotExist(const char* singleDir)
{
    long retVal = 0;
    struct stat statBuf;

    if (stat(singleDir, &statBuf) != 0)
    {
        errno = 0;
        if(mkdir(singleDir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
            RPLC_ERR_MSG(13, 0, __LINE__);
    }

LBL_END:
    if (retVal != 0)
        print_usage();
}

long VerifyTempDirPath(char* TempPath, const char* home, const char* root, const char* sub)
{
    long retVal = 0;
    char temp[_MAX_PATH];

    sprintf(temp, "%s/%s", home, root);
    CreateSingleDirIfNotExist(temp);
    CreateSingleDirIfNotExist(TempPath);

    if(access(TempPath, 0) != 0)
    {
        if(mkdir(TempPath, S_IWUSR) != 0)
            RPLC_ERR_MSG(13, 0, __LINE__);
    }

LBL_END:
    if (retVal != 0)
        print_usage();
    return retVal;
}

long PrepNewTempFileName()
{
    long retVal = 0;
    static long GlobalFileCntr = 0;
    char TempRoot[]        = "tmp";
    char TempSub[]        = "rplc";
    char TempPath[_MAX_PATH];
    char BaseFileName[]    = "/rplc000";
    char FileNameExt[]    = ".tmp";
    char FileNameCnt[10];
    size_t ExtLen, TempLen;

    char* homePath = getenv("HOME");

    sprintf(TempPath, "%s/%s/%s", homePath, TempRoot, TempSub);
    VerifyTempDirPath(TempPath, homePath, TempRoot, TempSub);

    strcpy(Temp_File, TempPath);
    strcat(Temp_File, BaseFileName);
    sprintf(FileNameCnt, "%d", GlobalFileCntr++);
    if((ExtLen = strlen(FileNameCnt)) > 3)
        RPLC_ERR_MSG(14, 0, __LINE__);

    TempLen = strlen(Temp_File);
    memcpy(&Temp_File[TempLen-ExtLen], FileNameCnt, ExtLen);
    strcat(Temp_File, FileNameExt);
    if(GlobalFileCntr == 999)
        GlobalFileCntr = 1;
LBL_END:
    return retVal;
}


