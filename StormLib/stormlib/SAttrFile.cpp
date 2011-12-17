/*****************************************************************************/
/* SAttrFile.cpp                          Copyright (c) Ladislav Zezula 2007 */
/*---------------------------------------------------------------------------*/
/* Description:                                                              */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 12.06.04  1.00  Lad  The first version of SAttrFile.cpp                   */
/*****************************************************************************/

#define __STORMLIB_SELF__
#include "StormLib.h"
#include "SCommon.h"
#include <assert.h>

#include "misc/crc32.h"
#include "misc/md5.h"

//-----------------------------------------------------------------------------
// Local functions

// This function creates the name for the listfile.
// the file will be created under unique name in the temporary directory
static void GetAttributesFileName(TMPQArchive * /* ha */, char * szAttrFile)
{
    char szTemp[MAX_PATH];

    // Create temporary file name int TEMP directory
    GetTempPath(sizeof(szTemp)-1, szTemp);
    GetTempFileName(szTemp, ATTRIBUTES_NAME, 0, szAttrFile);
}

//-----------------------------------------------------------------------------
// Public functions (internal use by StormLib)

int SAttrFileCreate(TMPQArchive * ha)
{
    TMPQAttr * pNewAttr;
    int nError = ERROR_SUCCESS;

    // There should NOW be any attributes
    assert(ha->pAttributes == NULL);

    pNewAttr = ALLOCMEM(TMPQAttr, 1);
    if(pNewAttr != NULL)
    {
        // Pre-set the structure
        pNewAttr->dwVersion = MPQ_ATTRIBUTES_V1;
        pNewAttr->dwFlags = 0;

        // Allocate array for CRC32
        pNewAttr->pCrc32 = ALLOCMEM(TMPQCRC32, ha->pHeader->dwHashTableSize);
        if(pNewAttr->pCrc32 != NULL)
        {
            pNewAttr->dwFlags |= MPQ_ATTRIBUTE_CRC32;
            memset(pNewAttr->pCrc32, 0, sizeof(TMPQCRC32) * ha->pHeader->dwHashTableSize);
        }
        else
            nError = ERROR_NOT_ENOUGH_MEMORY;

        // Allocate array for FILETIME
        pNewAttr->pFileTime = ALLOCMEM(TMPQFileTime, ha->pHeader->dwHashTableSize);
        if(pNewAttr->pFileTime != NULL)
        {
            pNewAttr->dwFlags |= MPQ_ATTRIBUTE_FILETIME;
            memset(pNewAttr->pFileTime, 0, sizeof(TMPQFileTime) * ha->pHeader->dwHashTableSize);
        }
        else
            nError = ERROR_NOT_ENOUGH_MEMORY;

        // Allocate array for MD5
        pNewAttr->pMd5 = ALLOCMEM(TMPQMD5, ha->pHeader->dwHashTableSize);
        if(pNewAttr->pMd5 != NULL)
        {
            pNewAttr->dwFlags |= MPQ_ATTRIBUTE_MD5;
            memset(pNewAttr->pMd5, 0, sizeof(TMPQMD5) * ha->pHeader->dwHashTableSize);
        }
        else
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    // If something failed, then free the attributes structure
    if(nError != ERROR_SUCCESS)
    {
        FreeMPQAttributes(pNewAttr);
        pNewAttr = NULL;
    }

    ha->pAttributes = pNewAttr;
    return nError;
}


int SAttrFileLoad(TMPQArchive * ha)
{
    TMPQAttr * pAttr = NULL;
    HANDLE hFile = NULL;
    DWORD dwBytesRead;
    DWORD dwToRead;
    int nError = ERROR_SUCCESS;

    // Initially, set the attrobutes to NULL
    ha->pAttributes = NULL;

    // Attempt to open the "(attributes)" file.
    // If it's not there, we don't support attributes
    if(!SFileOpenFileEx((HANDLE)ha, ATTRIBUTES_NAME, 0, &hFile))
        nError = GetLastError();

    // Allocate space for the TMPQAttributes
    if(nError == ERROR_SUCCESS)
    {
        pAttr = ALLOCMEM(TMPQAttr, 1);
        if(pAttr == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    // Load the content of the attributes file
    if(nError == ERROR_SUCCESS)
    {
        memset(pAttr, 0, sizeof(TMPQAttr));
        
        dwToRead = sizeof(DWORD) + sizeof(DWORD);
        SFileReadFile(hFile, pAttr, dwToRead, &dwBytesRead, NULL);
        if(dwBytesRead != dwToRead)
            nError = ERROR_FILE_CORRUPT;
    }

    // Verify format of the attributes
    if(nError == ERROR_SUCCESS)
    {
        if(pAttr->dwVersion > MPQ_ATTRIBUTES_V1)
            nError = ERROR_BAD_FORMAT;
    }

    // Load the CRC32 (if any)
    if(nError == ERROR_SUCCESS && (pAttr->dwFlags & MPQ_ATTRIBUTE_CRC32))
    {
        pAttr->pCrc32 = ALLOCMEM(TMPQCRC32, ha->pHeader->dwHashTableSize);
        if(pAttr->pCrc32 != NULL)
        {
            memset(pAttr->pCrc32, 0, sizeof(TMPQCRC32) * ha->pHeader->dwHashTableSize);
            dwToRead = sizeof(TMPQCRC32) * ha->pHeader->dwBlockTableSize;
            SFileReadFile(hFile, pAttr->pCrc32, dwToRead, &dwBytesRead, NULL);
            if(dwBytesRead != dwToRead)
                nError = ERROR_FILE_CORRUPT;
        }
        else
        {
            nError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Read the FILETIMEs (if any)
    if(nError == ERROR_SUCCESS && (pAttr->dwFlags & MPQ_ATTRIBUTE_FILETIME))
    {
        pAttr->pFileTime = ALLOCMEM(TMPQFileTime, ha->pHeader->dwHashTableSize);
        if(pAttr->pFileTime != NULL)
        {
            memset(pAttr->pFileTime, 0, sizeof(TMPQFileTime) * ha->pHeader->dwHashTableSize);
            dwToRead = sizeof(TMPQFileTime) * ha->pHeader->dwBlockTableSize;
            SFileReadFile(hFile, pAttr->pFileTime, dwToRead, &dwBytesRead, NULL);
            if(dwBytesRead != dwToRead)
                nError = ERROR_FILE_CORRUPT;
        }
        else
        {
            nError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Read the MD5 (if any)
    if(nError == ERROR_SUCCESS && (pAttr->dwFlags & MPQ_ATTRIBUTE_MD5))
    {
        pAttr->pMd5 = ALLOCMEM(TMPQMD5, ha->pHeader->dwHashTableSize);
        if(pAttr->pMd5 != NULL)
        {
            memset(pAttr->pMd5, 0, sizeof(TMPQMD5) * ha->pHeader->dwHashTableSize);
            dwToRead = sizeof(TMPQMD5) * ha->pHeader->dwBlockTableSize;
            SFileReadFile(hFile, pAttr->pMd5, dwToRead, &dwBytesRead, NULL);
            if(dwBytesRead != dwToRead)
                nError = ERROR_FILE_CORRUPT;
        }
        else
        {
            nError = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    // Set the attributes into the MPQ archive
    if(nError == ERROR_SUCCESS)
    {
        ha->pAttributes = pAttr;
        pAttr = NULL;
    }

    // Cleanup & exit
    FreeMPQAttributes(pAttr);
    SFileCloseFile(hFile);
    return nError;
}

int SAttrFileSaveToMpq(TMPQArchive * ha)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwToWrite;
    DWORD dwWritten;
    LCID lcSave = lcLocale;
    char szAttrFile[MAX_PATH];
    int nError = ERROR_SUCCESS;

    // If there are no attributes, do nothing
    if(ha->pAttributes == NULL)
        return ERROR_SUCCESS;

    // Create the local attributes file
    if(nError == ERROR_SUCCESS)
    {
        GetAttributesFileName(ha, szAttrFile);
        hFile = CreateFile(szAttrFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if(hFile == INVALID_HANDLE_VALUE)
            nError = GetLastError();
    }

    // Write the content of the attributes to the file
    if(nError == ERROR_SUCCESS)
    {
        // Write the header of the attributes file
        dwToWrite = sizeof(DWORD) + sizeof(DWORD);
        WriteFile(hFile, ha->pAttributes, dwToWrite, &dwWritten, NULL);
        if(dwWritten != dwToWrite)
            nError = ERROR_DISK_FULL;
    }

    // Write the array of CRC32
    if(nError == ERROR_SUCCESS && ha->pAttributes->pCrc32 != NULL)
    {
        dwToWrite = sizeof(TMPQCRC32) * ha->pHeader->dwBlockTableSize;
        WriteFile(hFile, ha->pAttributes->pCrc32, dwToWrite, &dwWritten, NULL);
        if(dwWritten != dwToWrite)
            nError = ERROR_DISK_FULL;
    }

    // Write the array of FILETIMEs
    if(nError == ERROR_SUCCESS && ha->pAttributes->pFileTime != NULL)
    {
        dwToWrite = sizeof(TMPQFileTime) * ha->pHeader->dwBlockTableSize;
        WriteFile(hFile, ha->pAttributes->pFileTime, dwToWrite, &dwWritten, NULL);
        if(dwWritten != dwToWrite)
            nError = ERROR_DISK_FULL;
    }

    // Write the array of MD5s
    if(nError == ERROR_SUCCESS && ha->pAttributes->pMd5 != NULL)
    {
        dwToWrite = sizeof(TMPQMD5) * ha->pHeader->dwBlockTableSize;
        WriteFile(hFile, ha->pAttributes->pMd5, dwToWrite, &dwWritten, NULL);
        if(dwWritten != dwToWrite)
            nError = ERROR_DISK_FULL;
    }

    // Add the attributes into MPQ
    if(nError == ERROR_SUCCESS)
    {
        SFileSetLocale(LANG_NEUTRAL);
        nError = AddFileToArchive(ha, hFile, ATTRIBUTES_NAME, MPQ_FILE_COMPRESS | MPQ_FILE_REPLACEEXISTING, 0, SFILE_TYPE_DATA, NULL);
        lcLocale = lcSave;
    }

    // Close the temporary file and delete it.
    // There is no FILE_FLAG_DELETE_ON_CLOSE on LINUX.
    if(hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);
    DeleteFile(szAttrFile);

    return nError;
}

void FreeMPQAttributes(TMPQAttr * pAttr)
{
    if(pAttr != NULL)
    {
        if(pAttr->pCrc32 != NULL)
            FREEMEM(pAttr->pCrc32);
        if(pAttr->pFileTime != NULL)
            FREEMEM(pAttr->pFileTime);
        if(pAttr->pMd5 != NULL)
            FREEMEM(pAttr->pMd5);

        FREEMEM(pAttr);
    }
}

//-----------------------------------------------------------------------------
// Public (exported) functions

BOOL WINAPI SFileVerifyFile(HANDLE hMpq, const char * szFileName, DWORD dwFlags)
{
    crc32_context crc32_ctx;
    md5_context md5_ctx;
    TMPQFile * hf;
    TMPQCRC32 Crc32;
    TMPQMD5 Md5;
    BYTE Buffer[0x1000];
    HANDLE hFile = NULL;
    DWORD dwBytesRead;
    BOOL bResult = TRUE;

    // Attempt to open the file
    if(SFileOpenFileEx(hMpq, szFileName, 0, &hFile))
    {
        // Initialize the CRC32 and MD5 counters
        CRC32_Init(&crc32_ctx);
        MD5_Init(&md5_ctx);
        hf = (TMPQFile *)hFile;

        // Go through entire file and update both CRC32 and MD5
        for(;;)
        {
            // Read data from file
            SFileReadFile(hFile, Buffer, sizeof(Buffer), &dwBytesRead, NULL);
            if(dwBytesRead == 0)
                break;

            // Update CRC32 value
            if(dwFlags & MPQ_ATTRIBUTE_CRC32)
                CRC32_Update(&crc32_ctx, Buffer, (int)dwBytesRead);
            
            // Update MD5 value
            if(dwFlags & MPQ_ATTRIBUTE_MD5)
                MD5_Update(&md5_ctx, Buffer, (int)dwBytesRead);
        }

        // Check if the CRC32 matches
        if((dwFlags & MPQ_ATTRIBUTE_CRC32) && hf->pCrc32 != NULL)
        {
            CRC32_Finish(&crc32_ctx, (unsigned long *)&Crc32.dwValue);
            if(Crc32.dwValue != hf->pCrc32->dwValue)
                bResult = FALSE;
        }

        // Check if MD5 matches
        if((dwFlags & MPQ_ATTRIBUTE_MD5) && hf->pMd5 != NULL)
        {
            MD5_Finish(&md5_ctx, Md5.Value);
            if(memcmp(Md5.Value, hf->pMd5->Value, sizeof(TMPQMD5)))
                bResult = FALSE;
        }

        SFileCloseFile(hFile);
    }

    return bResult;
}
