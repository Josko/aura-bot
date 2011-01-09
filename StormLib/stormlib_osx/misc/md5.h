/*****************************************************************************/
/* md5.h                                  Copyright (c) Ladislav Zezula 2007 */
/*---------------------------------------------------------------------------*/
/* Description:                                                              */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 11.06.07  1.00  Lad  The first version of md5.h                           */
/*****************************************************************************/

#ifndef __MD5_H__
#define __MD5_H__

// Functions for MD5
struct md5_context
{
    unsigned long total[2];             // number of bytes processed
    unsigned long state[4];             // intermediate digest state
    unsigned char buffer[64];           // data block being processed
};

void MD5_Init(md5_context * ctx);
void MD5_Update(md5_context * ctx, unsigned char *input, int ilen);
void MD5_Finish(md5_context * ctx, unsigned char output[16]);

#endif // __MD5_H__
