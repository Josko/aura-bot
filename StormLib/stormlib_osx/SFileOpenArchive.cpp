/*****************************************************************************/
/* SFileOpenArchive.cpp                       Copyright Ladislav Zezula 1999 */
/*                                                                           */
/* Author : Ladislav Zezula                                                  */
/* E-mail : ladik@zezula.net                                                 */
/* WWW    : www.zezula.net                                                   */
/*---------------------------------------------------------------------------*/
/*                       Archive functions of Storm.dll                      */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* xx.xx.xx  1.00  Lad  The first version of SFileOpenArchive.cpp            */
/* 19.11.03  1.01  Dan  Big endian handling                                  */
/*****************************************************************************/

#define __STORMLIB_SELF__
#include "StormLib.h"
#include "SCommon.h"

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

static BOOL IsAviFile(TMPQHeader * pHeader)
{
    DWORD * AviHdr = (DWORD *)pHeader;

    // Test for 'RIFF', 'AVI ' or 'LIST'
    return (AviHdr[0] == 'FFIR' && AviHdr[2] == ' IVA' && AviHdr[3] == 'TSIL');
}

// This function gets the right positions of the hash table and the block table.
static int RelocateMpqTablePositions(TMPQArchive * ha)
{
    TMPQHeader2 * pHeader = ha->pHeader;
    LARGE_INTEGER FileSize;
    LARGE_INTEGER TempSize;

    // Get the size of the file
    FileSize.LowPart = GetFileSize(ha->hFile, (LPDWORD)&FileSize.HighPart);

    // Set the proper hash table position
    ha->HashTablePos.HighPart = pHeader->wHashTablePosHigh;
    ha->HashTablePos.LowPart = pHeader->dwHashTablePos;
    ha->HashTablePos.QuadPart += ha->MpqPos.QuadPart;
    if(ha->HashTablePos.QuadPart > FileSize.QuadPart)
        return ERROR_BAD_FORMAT;

    // Set the proper block table position
    ha->BlockTablePos.HighPart = pHeader->wBlockTablePosHigh;
    ha->BlockTablePos.LowPart = pHeader->dwBlockTablePos;
    ha->BlockTablePos.QuadPart += ha->MpqPos.QuadPart;
    if(ha->BlockTablePos.QuadPart > FileSize.QuadPart)
        return ERROR_BAD_FORMAT;

    // Set the proper position of the extended block table
    if(pHeader->ExtBlockTablePos.QuadPart != 0)
    {
        ha->ExtBlockTablePos = pHeader->ExtBlockTablePos;
        ha->ExtBlockTablePos.QuadPart += ha->MpqPos.QuadPart;
        if(ha->ExtBlockTablePos.QuadPart > FileSize.QuadPart)
            return ERROR_BAD_FORMAT;
    }

    // Size of MPQ archive is computed as the biggest of
    // (EndOfBlockTable, EndOfHashTable, EndOfExtBlockTable)
    TempSize.QuadPart = ha->HashTablePos.QuadPart + (pHeader->dwHashTableSize * sizeof(TMPQHash));
    if(TempSize.QuadPart > ha->MpqSize.QuadPart)
        ha->MpqSize = TempSize;
    TempSize.QuadPart = ha->BlockTablePos.QuadPart + (pHeader->dwBlockTableSize * sizeof(TMPQBlock));
    if(TempSize.QuadPart > ha->MpqSize.QuadPart)
        ha->MpqSize = TempSize;
    TempSize.QuadPart = ha->ExtBlockTablePos.QuadPart + (pHeader->dwBlockTableSize * sizeof(TMPQBlockEx));
    if(TempSize.QuadPart > ha->MpqSize.QuadPart)
        ha->MpqSize = TempSize;
    
    // MPQ size does not include the bytes before MPQ header
    ha->MpqSize.QuadPart -= ha->MpqPos.QuadPart;
    return ERROR_SUCCESS;
}


/*****************************************************************************/
/* Public functions                                                          */
/*****************************************************************************/

//-----------------------------------------------------------------------------
// SFileGetLocale and SFileSetLocale
// Set the locale for all neewly opened archives and files

LCID WINAPI SFileGetLocale()
{
    return lcLocale;
}

LCID WINAPI SFileSetLocale(LCID lcNewLocale)
{
    lcLocale = lcNewLocale;
    return lcLocale;
}

//-----------------------------------------------------------------------------
// SFileOpenArchiveEx (not a public function !!!)
//
//   szFileName - MPQ archive file name to open
//   dwPriority - When SFileOpenFileEx called, this contains the search priority for searched archives
//   dwFlags    - If contains MPQ_OPEN_NO_LISTFILE, then the internal list file will not be used.
//   phMPQ      - Pointer to store open archive handle

BOOL SFileOpenArchiveEx(
    const char * szMpqName,
    DWORD dwPriority,
    DWORD dwFlags,
    HANDLE * phMPQ,
    DWORD dwAccessMode)
{
    LARGE_INTEGER TempPos;
    TMPQArchive * ha = NULL;            // Archive handle
    HANDLE hFile = INVALID_HANDLE_VALUE;// Opened archive file handle
    DWORD dwMaxBlockIndex = 0;          // Maximum value of block entry
    DWORD dwBlockTableSize = 0;         // Block table size.
    DWORD dwTransferred;                // Number of bytes read
    DWORD dwBytes = 0;                  // Number of bytes to read
    int nError = ERROR_SUCCESS;   

    // Check the right parameters
    if(nError == ERROR_SUCCESS)
    {
        if(szMpqName == NULL || *szMpqName == 0 || phMPQ == NULL)
            nError = ERROR_INVALID_PARAMETER;
    }

    // Ensure that StormBuffer is allocated
    if(nError == ERROR_SUCCESS)
        nError = PrepareStormBuffer();

    // Open the MPQ archive file
    if(nError == ERROR_SUCCESS)
    {
        hFile = CreateFile(szMpqName, dwAccessMode, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if(hFile == INVALID_HANDLE_VALUE)
            nError = GetLastError();
    }
    
    // Allocate the MPQhandle
    if(nError == ERROR_SUCCESS)
    {
        if((ha = ALLOCMEM(TMPQArchive, 1)) == NULL)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    // Initialize handle structure and allocate structure for MPQ header
    if(nError == ERROR_SUCCESS)
    {
        memset(ha, 0, sizeof(TMPQArchive));
        strncpy(ha->szFileName, szMpqName, strlen(szMpqName));
        ha->hFile      = hFile;
        ha->dwPriority = dwPriority;
        ha->pHeader    = &ha->Header;
        ha->pListFile  = NULL;
        hFile = INVALID_HANDLE_VALUE;
    }

    // Find the offset of MPQ header within the file
    if(nError == ERROR_SUCCESS)
    {
        LARGE_INTEGER SearchPos = {0};
        LARGE_INTEGER MpqPos = {0};
        DWORD dwHeaderID;

        for(;;)
        {
            // Invalidate the MPQ ID and read the eventual header
            SetFilePointer(ha->hFile, MpqPos.LowPart, &MpqPos.HighPart, FILE_BEGIN);
            ReadFile(ha->hFile, ha->pHeader, sizeof(TMPQHeader2), &dwTransferred, NULL);
            dwHeaderID = BSWAP_INT32_UNSIGNED(ha->pHeader->dwID);

            // Special check : Some MPQs are actually AVI files, only with
            // changed extension.
            if(MpqPos.QuadPart == 0 && IsAviFile(ha->pHeader))
            {
                nError = ERROR_AVI_FILE;
                break;
            }

            // If different number of bytes read, break the loop
            if(dwTransferred != sizeof(TMPQHeader2))
            {
                nError = ERROR_BAD_FORMAT;
                break;
            }

            // If there is the MPQ shunt signature, process it
            if(dwHeaderID == ID_MPQ_SHUNT && ha->pShunt == NULL)
            {
                // Ignore the MPQ shunt completely if the caller wants to open the MPQ as V1.0
                if((dwFlags & MPQ_OPEN_FORCE_MPQ_V1) == 0)
                {
                    // Fill the shunt header
                    ha->ShuntPos = MpqPos;
                    ha->pShunt = &ha->Shunt;
                    memcpy(ha->pShunt, ha->pHeader, sizeof(TMPQShunt));
                    BSWAP_TMPQSHUNT(ha->pShunt);

                    // Set the MPQ pos and repeat the search
                    MpqPos.QuadPart = SearchPos.QuadPart + ha->pShunt->dwHeaderPos;
                    continue;
                }
            }

            // There must be MPQ header signature
            if(dwHeaderID == ID_MPQ)
            {
                BSWAP_TMPQHEADER(ha->pHeader);

                // Save the position where the MPQ header has been found
                ha->MpqPos = MpqPos;

                // If valid signature has been found, break the loop
                if(ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_1)
                {
                    // W3M Map Protectors set some garbage value into the "dwHeaderSize"
                    // field of MPQ header. This value is apparently ignored by Storm.dll
                    if(ha->pHeader->dwHeaderSize != sizeof(TMPQHeader))
                    {
                        ha->dwFlags |= MPQ_FLAG_PROTECTED;
                        ha->pHeader->dwHeaderSize = sizeof(TMPQHeader);
                    }
					break;
                }

                if(ha->pHeader->wFormatVersion == MPQ_FORMAT_VERSION_2)
                {
                    // W3M Map Protectors set some garbage value into the "dwHeaderSize"
                    // field of MPQ header. This value is apparently ignored by Storm.dll
                    if(ha->pHeader->dwHeaderSize != sizeof(TMPQHeader2))
                    {
                        ha->dwFlags |= MPQ_FLAG_PROTECTED;
                        ha->pHeader->dwHeaderSize = sizeof(TMPQHeader2);
                    }
					break;
                }

				//
				// Note: the "dwArchiveSize" member in the MPQ header is ignored by Storm.dll
				// and can contain garbage value ("w3xmaster" protector)
				// 
                
                nError = ERROR_NOT_SUPPORTED;
                break;
            }

            // Move to the next possible offset
            SearchPos.QuadPart += 0x200;
            MpqPos = SearchPos;
        }
    }

    // Relocate tables position
    if(nError == ERROR_SUCCESS)
    {
        // W3x Map Protectors use the fact that War3's StormLib ignores the file shunt,
        // and probably ignores the MPQ format version as well. The trick is to
        // fake MPQ format 2, with an improper hi-word position of hash table and block table
        // We can overcome such protectors by forcing opening the archive as MPQ v 1.0
        if(dwFlags & MPQ_OPEN_FORCE_MPQ_V1)
        {
            ha->pHeader->wFormatVersion = MPQ_FORMAT_VERSION_1;
            ha->pHeader->dwHeaderSize = sizeof(TMPQHeader);
            ha->pShunt = NULL;
        }

        // Clear the fields not supported in older formats
        if(ha->pHeader->wFormatVersion < MPQ_FORMAT_VERSION_2)
        {
            ha->pHeader->ExtBlockTablePos.QuadPart = 0;
            ha->pHeader->wBlockTablePosHigh = 0;
            ha->pHeader->wHashTablePosHigh = 0;
        }

        ha->dwBlockSize = (0x200 << ha->pHeader->wBlockSize);
        nError = RelocateMpqTablePositions(ha);
    }

    // Allocate buffers
    if(nError == ERROR_SUCCESS)
    {
        //
        // Note that the block table should be as large as the hash table
        // (For later file additions).
        //
        // I have found a MPQ which has the block table larger than
        // the hash table. We should avoid buffer overruns caused by that.
        //
        
        if(ha->pHeader->dwBlockTableSize > ha->pHeader->dwHashTableSize)
            ha->pHeader->dwBlockTableSize = ha->pHeader->dwHashTableSize;
        dwBlockTableSize   = ha->pHeader->dwHashTableSize;

        ha->pHashTable     = ALLOCMEM(TMPQHash, ha->pHeader->dwHashTableSize);
        ha->pBlockTable    = ALLOCMEM(TMPQBlock, dwBlockTableSize);
        ha->pExtBlockTable = ALLOCMEM(TMPQBlockEx, dwBlockTableSize);
        ha->pbBlockBuffer  = ALLOCMEM(BYTE, ha->dwBlockSize);

        if(!ha->pHashTable || !ha->pBlockTable || !ha->pExtBlockTable || !ha->pbBlockBuffer)
            nError = ERROR_NOT_ENOUGH_MEMORY;
    }

    // Read the hash table into memory
    if(nError == ERROR_SUCCESS)
    {
        dwBytes = ha->pHeader->dwHashTableSize * sizeof(TMPQHash);
        SetFilePointer(ha->hFile, ha->HashTablePos.LowPart, &ha->HashTablePos.HighPart, FILE_BEGIN);
        ReadFile(ha->hFile, ha->pHashTable, dwBytes, &dwTransferred, NULL);

        if(dwTransferred != dwBytes)
            nError = ERROR_FILE_CORRUPT;
    }

    // Decrypt hash table and check if it is correctly decrypted
    if(nError == ERROR_SUCCESS)
    {
//      TMPQHash * pHashEnd = ha->pHashTable + ha->pHeader->dwHashTableSize;
//      TMPQHash * pHash;

        // We have to convert the hash table from LittleEndian
        BSWAP_ARRAY32_UNSIGNED((DWORD *)ha->pHashTable, (dwBytes / sizeof(DWORD)));
        DecryptHashTable((DWORD *)ha->pHashTable, (BYTE *)"(hash table)", (ha->pHeader->dwHashTableSize * 4));

        //
        // Check hash table if is correctly decrypted
        // 
        // Ladik: Some MPQ protectors corrupt the hash table by rewriting part of it.
        // To be able to open these, we will not check the entire hash table,
        // but will check it at the moment of file opening.
        // 
    }

    // Now, read the block table
    if(nError == ERROR_SUCCESS)
    {
        memset(ha->pBlockTable, 0, dwBlockTableSize * sizeof(TMPQBlock));

        // Carefully check the block table size
        dwBytes = ha->pHeader->dwBlockTableSize * sizeof(TMPQBlock);
        SetFilePointer(ha->hFile, ha->BlockTablePos.LowPart, &ha->BlockTablePos.HighPart, FILE_BEGIN);
        ReadFile(ha->hFile, ha->pBlockTable, dwBytes, &dwTransferred, NULL);

        // I have found a MPQ which claimed 0x200 entries in the block table,
        // but the file was cut and there was only 0x1A0 entries.
        // We will handle this case properly, even if that means 
        // omiting another integrity check of the MPQ
        if(dwTransferred < dwBytes)
            dwBytes = dwTransferred;
        BSWAP_ARRAY32_UNSIGNED((DWORD *)ha->pBlockTable, dwBytes / sizeof(DWORD));

        // If nothing was read, we assume the file is corrupt.
        if(dwTransferred == 0)
            nError = ERROR_FILE_CORRUPT;
    }

    // Decrypt block table.
    // Some MPQs don't have the block table decrypted, e.g. cracked Diablo version
    // We have to check if block table is really encrypted
    if(nError == ERROR_SUCCESS)
    {
        TMPQBlock * pBlockEnd = ha->pBlockTable + (dwBytes / sizeof(TMPQBlock));
        TMPQBlock * pBlock = ha->pBlockTable;
        BOOL bBlockTableEncrypted = FALSE;

        // Verify all blocks entries in the table
        // The loop usually stops at the first entry
        while(pBlock < pBlockEnd)
        {
            // The lower 8 bits of the MPQ flags are always zero.
            // Note that this may change in next MPQ versions
            if(pBlock->dwFlags & 0x000000FF)
            {
                bBlockTableEncrypted = TRUE;
                break;
            }

            // Move to the next block table entry
            pBlock++;
        }

        if(bBlockTableEncrypted)
        {
            DecryptBlockTable((DWORD *)ha->pBlockTable,
                               (BYTE *)"(block table)",
                                       (dwBytes / sizeof(DWORD)));
        }
    }

    // Now, read the extended block table.
    // For V1 archives, we still will maintain the extended block table
    // (it will be filled with zeros)
    if(nError == ERROR_SUCCESS)
    {
        memset(ha->pExtBlockTable, 0, dwBlockTableSize * sizeof(TMPQBlockEx));

        if(ha->pHeader->ExtBlockTablePos.QuadPart != 0)
        {
            dwBytes = ha->pHeader->dwBlockTableSize * sizeof(TMPQBlockEx);
            SetFilePointer(ha->hFile,
                           ha->ExtBlockTablePos.LowPart,
                          &ha->ExtBlockTablePos.HighPart,
                           FILE_BEGIN);
            ReadFile(ha->hFile, ha->pExtBlockTable, dwBytes, &dwTransferred, NULL);

            // We have to convert every DWORD in ha->block from LittleEndian
            BSWAP_ARRAY16_UNSIGNED((USHORT *)ha->pExtBlockTable, dwBytes / sizeof(USHORT));

            // The extended block table is not encrypted (so far)
            if(dwTransferred != dwBytes)
                nError = ERROR_FILE_CORRUPT;
        }
    }

    // Verify both block tables (If the MPQ file is not protected)
    if(nError == ERROR_SUCCESS && (ha->dwFlags & MPQ_FLAG_PROTECTED) == 0)
    {
        TMPQBlockEx * pBlockEx = ha->pExtBlockTable;
        TMPQBlock * pBlockEnd = ha->pBlockTable + dwMaxBlockIndex + 1;
        TMPQBlock * pBlock   = ha->pBlockTable;

        // If the MPQ file is not protected,
        // we will check if all sizes in the block table is correct.
        // Note that we will not relocate the block table (change from previous versions)
        for(; pBlock < pBlockEnd; pBlock++, pBlockEx++)
        {
            if(pBlock->dwFlags & MPQ_FILE_EXISTS)
            {
                // Get the 64-bit file position
                TempPos.HighPart = pBlockEx->wFilePosHigh;
                TempPos.LowPart = pBlock->dwFilePos;

                if(TempPos.QuadPart > ha->MpqSize.QuadPart || pBlock->dwCSize > ha->MpqSize.QuadPart)
                {
                    nError = ERROR_BAD_FORMAT;
                    break;
                }
            }
        }
    }

    // If the caller didn't specified otherwise, 
    // include the internal listfile to the TMPQArchive structure
    if(nError == ERROR_SUCCESS)
    {
        if((dwFlags & MPQ_OPEN_NO_LISTFILE) == 0)
        {
            if(nError == ERROR_SUCCESS)
                SListFileCreateListFile(ha);

            // Add the internal listfile
            if(nError == ERROR_SUCCESS)
                SFileAddListFile((HANDLE)ha, NULL);
        }
    }

    // If the caller didn't specified otherwise, 
    // load the "(attributes)" file
    if(nError == ERROR_SUCCESS && (dwFlags & MPQ_OPEN_NO_ATTRIBUTES) == 0)
    {
        // Ignore the result here. Attrobutes are not necessary,
        // if they are not there, we will just ignore them
        SAttrFileLoad(ha);
    }

    // Cleanup and exit
    if(nError != ERROR_SUCCESS)
    {
        FreeMPQArchive(ha);
        if(hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
        SetLastError(nError);
        ha = NULL;
    }
    else
    {
        if(pFirstOpen == NULL)
            pFirstOpen = ha;
    }
    *phMPQ = ha;
    return (nError == ERROR_SUCCESS);
}

BOOL WINAPI SFileOpenArchive(const char * szMpqName, DWORD dwPriority, DWORD dwFlags, HANDLE * phMPQ)
{
    return SFileOpenArchiveEx(szMpqName, dwPriority, dwFlags, phMPQ, GENERIC_READ);
}

//-----------------------------------------------------------------------------
// BOOL SFileFlushArchive(HANDLE hMpq)
//
// Saves all dirty data into MPQ archive.
// Has similar effect like SFileCLoseArchive, but the archive is not closed.
// Use on clients who keep MPQ archive open even for write operations,
// and terminating without calling SFileCloseArchive might corrupt the archive.
//

BOOL WINAPI SFileFlushArchive(HANDLE hMpq)
{
    TMPQArchive * ha = (TMPQArchive *)hMpq;
    
    // Do nothing if 'hMpq' is bad parameter
    if(!IsValidMpqHandle(ha))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // If the archive has been changed, update the changes
    // on the disk drive.
    if(ha->dwFlags & MPQ_FLAG_CHANGED)
    {
        SListFileSaveToMpq(ha);
        SAttrFileSaveToMpq(ha);
        SaveMPQTables(ha);
        ha->dwFlags &= ~MPQ_FLAG_CHANGED;
    }

    return TRUE;
}

//-----------------------------------------------------------------------------
// BOOL SFileCloseArchive(HANDLE hMPQ);
//

BOOL WINAPI SFileCloseArchive(HANDLE hMPQ)
{
    TMPQArchive * ha = (TMPQArchive *)hMPQ;
    
    // Flush all unsaved data to the storage
    if(!SFileFlushArchive(hMPQ))
        return FALSE;

    // Free all memory used by MPQ archive
    FreeMPQArchive(ha);
    return TRUE;
}

