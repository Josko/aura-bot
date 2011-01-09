/*****************************************************************************/
/* SListFile.cpp                          Copyright (c) Ladislav Zezula 2004 */
/*---------------------------------------------------------------------------*/
/* Description:                                                              */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 12.06.04  1.00  Lad  The first version of SListFile.cpp                   */
/*****************************************************************************/

#define __STORMLIB_SELF__
#include "StormLib.h"
#include "SCommon.h"
#include <assert.h>

//-----------------------------------------------------------------------------
// Listfile entry structure

#define LISTFILE_CACHE_SIZE 0x1000      // Size of one cache element
#define NO_MORE_CHARACTERS 256
#define HASH_TABLE_SIZE    31           // Initial hash table size (should be a prime number)

struct TListFileCache
{
    HANDLE  hFile;                      // Stormlib file handle
    char  * szMask;                     // File mask
    DWORD   dwFileSize;                 // Total size of the cached file
    DWORD   dwBuffSize;                 // File of the cache
    DWORD   dwFilePos;                  // Position of the cache in the file
    BYTE  * pBegin;                     // The begin of the listfile cache
    BYTE  * pPos;
    BYTE  * pEnd;                       // The last character in the file cache

    BYTE Buffer[1];                     // Listfile cache itself
};

//-----------------------------------------------------------------------------
// Local functions (cache)

// Reloads the cache. Returns number of characters
// that has been loaded into the cache.
static int ReloadCache(TListFileCache * pCache)
{
    // Check if there is enough characters in the cache
    // If not, we have to reload the next block
    if(pCache->pPos >= pCache->pEnd)
    {
        // If the cache is already at the end, do nothing more
        if((pCache->dwFilePos + pCache->dwBuffSize) >= pCache->dwFileSize)
            return 0;

        pCache->dwFilePos += pCache->dwBuffSize;
        SFileReadFile(pCache->hFile, pCache->Buffer, pCache->dwBuffSize, &pCache->dwBuffSize, NULL);
        if(pCache->dwBuffSize == 0)
            return 0;

        // Set the buffer pointers
        pCache->pBegin =
        pCache->pPos = &pCache->Buffer[0];
        pCache->pEnd = pCache->pBegin + pCache->dwBuffSize;
    }

    return pCache->dwBuffSize;
}

static size_t ReadLine(TListFileCache * pCache, char * szLine, int nMaxChars)
{
    char * szLineBegin = szLine;
    char * szLineEnd = szLine + nMaxChars - 1;
    
__BeginLoading:

    // Skip newlines, spaces, tabs and another non-printable stuff
    while(pCache->pPos < pCache->pEnd && *pCache->pPos <= 0x20)
        pCache->pPos++;

    // Copy the remaining characters
    while(pCache->pPos < pCache->pEnd && szLine < szLineEnd)
    {
        // If we have found a newline, stop loading
        if(*pCache->pPos == 0x0D || *pCache->pPos == 0x0A)
            break;

        *szLine++ = *pCache->pPos++;
    }

    // If we now need to reload the cache, do it
    if(pCache->pPos == pCache->pEnd)
    {
        if(ReloadCache(pCache) > 0)
            goto __BeginLoading;
    }

    *szLine = 0;
    return (szLine - szLineBegin);
}

//-----------------------------------------------------------------------------
// Local functions (listfile nodes)

// This function creates the name for the listfile.
// the file will be created under unique name in the temporary directory
static void GetListFileName(TMPQArchive * /* ha */, char * szListFile)
{
    char szTemp[MAX_PATH];

    // Create temporary file name int TEMP directory
    GetTempPath(sizeof(szTemp)-1, szTemp);
    GetTempFileName(szTemp, LISTFILE_NAME, 0, szListFile);
}

// Creates new listfile. The listfile is an array of TListFileNode
// structures. The size of the array is the same like the hash table size,
// the ordering is the same too (listfile item index is the same like
// the index in the MPQ hash table)

int SListFileCreateListFile(TMPQArchive * ha)
{
    DWORD dwItems = ha->pHeader->dwHashTableSize;

    // The listfile should be NULL now
    assert(ha->pListFile == NULL);

    ha->pListFile = ALLOCMEM(TFileNode *, dwItems);
    if(ha->pListFile == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    memset(ha->pListFile, 0xFF, dwItems * sizeof(TFileNode *));
    return ERROR_SUCCESS;
}

// Adds a name into the list of all names. For each locale in the MPQ,
// one entry will be created
// If the file name is already there, does nothing.
int SListFileCreateNodeForAllLocales(TMPQArchive * ha, const char * szFileName)
{
    TFileNode * pNode   = NULL;
    TMPQHash * pHashEnd = ha->pHashTable + ha->pHeader->dwHashTableSize;
    TMPQHash * pHash0   = GetHashEntry(ha, szFileName);
    TMPQHash * pHash    = pHash0;
    DWORD dwHashIndex = 0;
    size_t nLength;                     // File name lentgth
    DWORD dwName1;
    DWORD dwName2;

    // If the file does not exist within the MPQ, do nothing
    if(pHash == NULL)
        return ERROR_SUCCESS;

    // Remember the name
    dwName1 = pHash->dwName1;
    dwName2 = pHash->dwName2;

    // Pass all entries in the hash table
    while(pHash->dwName1 == dwName1 && pHash->dwName2 == dwName2 && pHash->dwBlockIndex != HASH_ENTRY_FREE)
    {
        // There may be an entry deleted amongst various language versions
        if(pHash->dwBlockIndex != HASH_ENTRY_DELETED)
        {
            // Create the lang version, if none
            dwHashIndex = (DWORD)(pHash - ha->pHashTable);
            if((DWORD_PTR)ha->pListFile[dwHashIndex] >= LISTFILE_ENTRY_DELETED)
            {
                // Create the listfile node and insert it into the listfile table
                // If the name node already exists there, we just increment number of references
                if(pNode == NULL)
                {
                    nLength = strlen(szFileName);
                    pNode = (TFileNode *)ALLOCMEM(char, sizeof(TFileNode) + nLength);
                    pNode->dwRefCount = 1;
                    pNode->nLength = nLength;
                    strcpy(pNode->szFileName, szFileName);
                    ha->pListFile[dwHashIndex] = pNode;
                }
                else
                {
                    pNode->dwRefCount++;
                    ha->pListFile[dwHashIndex] = pNode;
                }
            }
        }

        // Move to the next hash entry
        if(++pHash >= pHashEnd)
            pHash = ha->pHashTable;
        if(pHash == pHash0)
            break;
    }
    return ERROR_SUCCESS;
}

// Adds a filename into the listfile. If the file name is already there,
// does nothing.
int SListFileCreateNode(TMPQArchive * ha, const char * szFileName, LCID lcLocale)
{
    TFileNode * pNode = NULL;
    TMPQHash * pHash0 = GetHashEntry(ha, szFileName);
    TMPQHash * pHash1 = GetHashEntryEx(ha, szFileName, lcLocale);
    DWORD dwHashIndex0 = 0;
    DWORD dwHashIndex1 = 0;
    size_t nLength;                     // File name lentgth

    // If the file does not exist within the MPQ, do nothing
    if(pHash1 == NULL || pHash1->dwBlockIndex >= HASH_ENTRY_DELETED)
        return ERROR_SUCCESS;

    // If the locale-cpecific listfile entry already exists, do nothing
    dwHashIndex0 = (DWORD)(pHash0 - ha->pHashTable);
    dwHashIndex1 = (DWORD)(pHash1 - ha->pHashTable);
    if((DWORD_PTR)ha->pListFile[dwHashIndex1] < LISTFILE_ENTRY_DELETED)
        return ERROR_SUCCESS;

    // Does the neutral table entry exist ?
    if((DWORD_PTR)ha->pListFile[dwHashIndex0] < LISTFILE_ENTRY_DELETED)
        pNode = ha->pListFile[dwHashIndex0];

    // If no node yet, we have to create new one
    if(pNode == NULL)
    {
        nLength = strlen(szFileName);
        pNode = (TFileNode *)ALLOCMEM(char, sizeof(TFileNode) + nLength);
        pNode->dwRefCount = 1;
        pNode->nLength = nLength;
        strcpy(pNode->szFileName, szFileName);
        ha->pListFile[dwHashIndex0] = pNode;
    }

    // Also insert the node in the locale-specific entry
    if(dwHashIndex1 != dwHashIndex0)
    {
        pNode->dwRefCount++;
        ha->pListFile[dwHashIndex1] = pNode;
    }

    return ERROR_SUCCESS;
}

// Removes a filename from the listfile.
// If the name is not there, does nothing
int SListFileRemoveNode(TMPQArchive * ha, const char * szFileName, LCID lcLocale)
{
    TFileNode * pNode = NULL;
    TMPQHash * pHash = GetHashEntryEx(ha, szFileName, lcLocale);
    size_t nHashIndex = 0;

    if(pHash != NULL)
    {
        nHashIndex = pHash - ha->pHashTable;
        pNode = ha->pListFile[nHashIndex];
        ha->pListFile[nHashIndex] = (TFileNode *)LISTFILE_ENTRY_DELETED;

        // Free the node
        pNode->dwRefCount--;
        if(pNode->dwRefCount == 0)
            FREEMEM(pNode);
    }
    return ERROR_SUCCESS;
}

void SListFileFreeListFile(TMPQArchive * ha)
{
    if(ha->pListFile != NULL)
    {
        for(DWORD i = 0; i < ha->pHeader->dwHashTableSize; i++)
        {
            TFileNode * pNode = ha->pListFile[i];

            if((DWORD_PTR)pNode < LISTFILE_ENTRY_DELETED)
            {
                ha->pListFile[i] = (TFileNode *)LISTFILE_ENTRY_FREE;
                pNode->dwRefCount--;

                if(pNode->dwRefCount == 0)
                    FREEMEM(pNode);
            }
        }

        FREEMEM(ha->pListFile);
        ha->pListFile = NULL;
    }
}

// Saves the whole listfile into the MPQ.
int SListFileSaveToMpq(TMPQArchive * ha)
{
    TFileNode * pNode = NULL;
    TMPQHash * pHashEnd = NULL;
    TMPQHash * pHash0 = NULL;
    TMPQHash * pHash = NULL;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    char   szListFile[MAX_PATH];
    char   szBuffer[MAX_PATH+4];
    DWORD dwTransferred;
    size_t nLength = 0;
    DWORD dwName1 = 0;
    DWORD dwName2 = 0;
    LCID lcSave = lcLocale;
    int  nError = ERROR_SUCCESS;

    // If no listfile, do nothing
    if(ha->pListFile == NULL)
        return ERROR_SUCCESS;

    // Create the local listfile
    if(nError == ERROR_SUCCESS)
    {
        GetListFileName(ha, szListFile);
        hFile = CreateFile(szListFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if(hFile == INVALID_HANDLE_VALUE)
            nError = GetLastError();
    }

    // Find the hash entry corresponding to listfile
    pHashEnd = ha->pHashTable + ha->pHeader->dwHashTableSize;
    pHash0 = pHash = GetHashEntry(ha, 0);
    if(pHash == NULL)
        pHash0 = pHash = ha->pHashTable;

    // Save the file
    if(nError == ERROR_SUCCESS)
    {
        for(;;)
        {
            if(pHash->dwName1 != dwName1 && pHash->dwName2 != dwName2 && pHash->dwBlockIndex < HASH_ENTRY_DELETED)
            {
                dwName1 = pHash->dwName1;
                dwName2 = pHash->dwName2;
                pNode = ha->pListFile[pHash - ha->pHashTable];

                if((DWORD_PTR)pNode < LISTFILE_ENTRY_DELETED)
                {
                    memcpy(szBuffer, pNode->szFileName, pNode->nLength);
                    szBuffer[pNode->nLength + 0] = 0x0D;
                    szBuffer[pNode->nLength + 1] = 0x0A;
                    WriteFile(hFile, szBuffer, (DWORD)(pNode->nLength + 2), &dwTransferred, NULL);
                }
            }

            if(++pHash >= pHashEnd)
                pHash = ha->pHashTable;
            if(pHash == pHash0)
                break;
        }

        // Write the listfile name (if not already there)
        if(GetHashEntry(ha, LISTFILE_NAME) == NULL)
        {
            nLength = strlen(LISTFILE_NAME);
            memcpy(szBuffer, LISTFILE_NAME, nLength);
            szBuffer[nLength + 0] = 0x0D;
            szBuffer[nLength + 1] = 0x0A;
            WriteFile(hFile, szBuffer, (DWORD)(nLength + 2), &dwTransferred, NULL);
        }                     
        
        // Add the listfile into the archive.
        SFileSetLocale(LANG_NEUTRAL);
        nError = AddFileToArchive(ha,
                                  hFile,
                                  LISTFILE_NAME,
                                  MPQ_FILE_ENCRYPTED | MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING,
                                  0,
                                  SFILE_TYPE_DATA,
                                  NULL);
        lcLocale = lcSave;
    }

    // Close the temporary file and delete it.
    // There is no FILE_FLAG_DELETE_ON_CLOSE on LINUX.
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    DeleteFile(szListFile);

    return nError;
}

//-----------------------------------------------------------------------------
// File functions

// Adds a listfile into the MPQ archive.
// Note that the function does not remove the 
int WINAPI SFileAddListFile(HANDLE hMpq, const char * szListFile)
{
    TListFileCache * pCache = NULL;
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    HANDLE hListFile = NULL;
    char  szFileName[MAX_PATH + 1];
    DWORD dwSearchScope = SFILE_OPEN_LOCAL_FILE;
    DWORD dwCacheSize = 0;
    DWORD dwFileSize = 0;
    size_t nLength = 0;
    int nError = ERROR_SUCCESS;

    // If the szListFile is NULL, it means we have to open internal listfile
    if(szListFile == NULL)
    {
        szListFile = LISTFILE_NAME;
        dwSearchScope = SFILE_OPEN_FROM_MPQ;
    }

    // Open the local/internal listfile
    if(nError == ERROR_SUCCESS)
    {
        if(!SFileOpenFileEx((HANDLE)ha, szListFile, dwSearchScope, &hListFile))
            nError = GetLastError();
    }

    if(nError == ERROR_SUCCESS)
    {
        dwCacheSize = 
        dwFileSize = SFileGetFileSize(hListFile, NULL);

        // Try to allocate memory for the complete file. If it fails,
        // load the part of the file
        pCache = (TListFileCache *)ALLOCMEM(char, (sizeof(TListFileCache) + dwCacheSize));
        if(pCache == NULL)
        {
            dwCacheSize = LISTFILE_CACHE_SIZE;
            pCache = (TListFileCache *)ALLOCMEM(char, sizeof(TListFileCache) + dwCacheSize);
        }

        if(pCache == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(nError == ERROR_SUCCESS)
    {
        // Initialize the file cache
        memset(pCache, 0, sizeof(TListFileCache));
        pCache->hFile      = hListFile;
        pCache->dwFileSize = dwFileSize;
        pCache->dwBuffSize = dwCacheSize;
        pCache->dwFilePos  = 0;

        // Fill the cache
        SFileReadFile(hListFile, pCache->Buffer, pCache->dwBuffSize, &pCache->dwBuffSize, NULL);

        // Initialize the pointers
        pCache->pBegin =
        pCache->pPos = &pCache->Buffer[0];
        pCache->pEnd = pCache->pBegin + pCache->dwBuffSize;

        // Load the node list. Add the node for every locale in the archive
        while((nLength = ReadLine(pCache, szFileName, sizeof(szFileName) - 1)) > 0)
            SListFileCreateNodeForAllLocales(ha, szFileName);
    }

    // Cleanup & exit
    if(pCache != NULL)
        SListFileFindClose((HANDLE)pCache);
    return nError;
}

//-----------------------------------------------------------------------------
// Passing through the listfile

HANDLE SListFileFindFirstFile(HANDLE hMpq, const char * szListFile, const char * szMask, SFILE_FIND_DATA * lpFindFileData)
{
    TListFileCache * pCache = NULL;
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    HANDLE hListFile = NULL;
    DWORD dwSearchScope = SFILE_OPEN_LOCAL_FILE;
    DWORD dwCacheSize = 0;
    DWORD dwFileSize = 0;
    size_t nLength = 0;
    int nError = ERROR_SUCCESS;

    // Initialize the structure with zeros
    memset(lpFindFileData, 0, sizeof(SFILE_FIND_DATA));

    // If the szListFile is NULL, it means we have to open internal listfile
    if(szListFile == NULL)
    {
        szListFile = LISTFILE_NAME;
        dwSearchScope = SFILE_OPEN_FROM_MPQ;
    }

    // Open the local/internal listfile
    if(nError == ERROR_SUCCESS)
    {
        if(!SFileOpenFileEx((HANDLE)ha, szListFile, dwSearchScope, &hListFile))
            nError = GetLastError();
    }

    if(nError == ERROR_SUCCESS)
    {
        dwCacheSize = 
        dwFileSize = SFileGetFileSize(hListFile, NULL);

        // Try to allocate memory for the complete file. If it fails,
        // load the part of the file
        pCache = (TListFileCache *)ALLOCMEM(char, sizeof(TListFileCache) + dwCacheSize);
        if(pCache == NULL)
        {
            dwCacheSize = LISTFILE_CACHE_SIZE;
            pCache = (TListFileCache *)ALLOCMEM(char, sizeof(TListFileCache) + dwCacheSize);
        }

        if(pCache == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    if(nError == ERROR_SUCCESS)
    {
        // Initialize the file cache
        memset(pCache, 0, sizeof(TListFileCache));
        pCache->hFile      = hListFile;
        pCache->dwFileSize = dwFileSize;
        pCache->dwBuffSize = dwCacheSize;
        pCache->dwFilePos  = 0;
        if(szMask != NULL)
        {
            pCache->szMask = ALLOCMEM(char, strlen(szMask) + 1);
            strcpy(pCache->szMask, szMask);
        }

        // Fill the cache
        SFileReadFile(hListFile, pCache->Buffer, pCache->dwBuffSize, &pCache->dwBuffSize, NULL);

        // Initialize the pointers
        pCache->pBegin =
        pCache->pPos = &pCache->Buffer[0];
        pCache->pEnd = pCache->pBegin + pCache->dwBuffSize;

        for(;;)
        {
            // Read the (next) line
            nLength = ReadLine(pCache, lpFindFileData->cFileName, sizeof(lpFindFileData->cFileName));
            if(nLength == 0)
            {
                nError = ERROR_NO_MORE_FILES;
                break;
            }

            // If some mask entered, check it
            if(CheckWildCard(lpFindFileData->cFileName, pCache->szMask))
                break;                
        }
    }

    // Cleanup & exit
    if(nError != ERROR_SUCCESS)
    {
        memset(lpFindFileData, 0, sizeof(SFILE_FIND_DATA));
        SListFileFindClose((HANDLE)pCache);
        pCache = NULL;

        SetLastError(nError);
    }
    return (HANDLE)pCache;
}

BOOL SListFileFindNextFile(HANDLE hFind, SFILE_FIND_DATA * lpFindFileData)
{
    TListFileCache * pCache = (TListFileCache *)hFind;
    size_t nLength;
    BOOL bResult = FALSE;
    int nError = ERROR_SUCCESS;

    for(;;)
    {
        // Read the (next) line
        nLength = ReadLine(pCache, lpFindFileData->cFileName, sizeof(lpFindFileData->cFileName));
        if(nLength == 0)
        {
            nError = ERROR_NO_MORE_FILES;
            break;
        }

        // If some mask entered, check it
        if(CheckWildCard(lpFindFileData->cFileName, pCache->szMask))
        {
            bResult = TRUE;
            break;
        }
    }

    if(nError != ERROR_SUCCESS)
        SetLastError(nError);
    return bResult;
}

BOOL SListFileFindClose(HANDLE hFind)
{
    TListFileCache * pCache = (TListFileCache *)hFind;

    if(pCache != NULL)
    {
        if(pCache->hFile != NULL)
            SFileCloseFile(pCache->hFile);
        if(pCache->szMask != NULL)
            FREEMEM(pCache->szMask);

        FREEMEM(pCache);
        return TRUE;
    }

    return FALSE;
}

