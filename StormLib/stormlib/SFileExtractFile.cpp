/*****************************************************************************/
/* SFileExtractFile.cpp                   Copyright (c) Ladislav Zezula 2003 */
/*---------------------------------------------------------------------------*/
/* Simple extracting utility                                                 */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 20.06.03  1.00  Lad  The first version of SFileExtractFile.cpp            */
/*****************************************************************************/

#define __STORMLIB_SELF__
#include "StormLib.h"
#include "SCommon.h"

BOOL WINAPI SFileExtractFile(HANDLE hMpq, const char * szToExtract, const char * szExtracted)
{
    HANDLE hLocalFile = INVALID_HANDLE_VALUE;
    HANDLE hMpqFile = NULL;
    DWORD dwSearchScope = 0;
    int nError = ERROR_SUCCESS;


    // Open the MPQ file
    if(nError == ERROR_SUCCESS)
    {
        if((DWORD_PTR)szToExtract <= 0x10000)
            dwSearchScope = SFILE_OPEN_BY_INDEX;
        if(!SFileOpenFileEx(hMpq, szToExtract, dwSearchScope, &hMpqFile))
            nError = GetLastError();
    }

    // Create the local file
    if(nError == ERROR_SUCCESS)
    {
        hLocalFile = CreateFile(szExtracted, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
        if(hLocalFile == INVALID_HANDLE_VALUE)
            nError = GetLastError();
    }

    // Copy the file's content
    if(nError == ERROR_SUCCESS)
    {
        char  szBuffer[0x1000];
        DWORD dwTransferred;

        for(;;)
        {
            // dwTransferred is only set to nonzero if something has been read.
            // nError can be ERROR_SUCCESS or ERROR_HANDLE_EOF
            if(!SFileReadFile(hMpqFile, szBuffer, sizeof(szBuffer), &dwTransferred, NULL))
                nError = GetLastError();
            if(nError == ERROR_HANDLE_EOF)
                nError = ERROR_SUCCESS;
            if(dwTransferred == 0)
                break;

            // If something has been actually read, write it
            WriteFile(hLocalFile, szBuffer, dwTransferred, &dwTransferred, NULL);
            if(dwTransferred == 0)
                nError = ERROR_DISK_FULL;
        }
    }

    // Close the files
    if(hMpqFile != NULL)
        SFileCloseFile(hMpqFile);
    if(hLocalFile != INVALID_HANDLE_VALUE)
        CloseHandle(hLocalFile);
    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return (BOOL)(nError == ERROR_SUCCESS);
}
