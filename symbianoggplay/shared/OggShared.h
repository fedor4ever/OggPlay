/*
 *  Copyright (c) 2007 S. Fisher
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef __OGGSHARED__
#define __OGGSHARED__

#include <e32def.h>

#define	SEEK_SET	0	// set file offset to offset
#define	SEEK_CUR	1	// set file offset to current plus offset
#define	SEEK_END	2	// set file offset to EOF plus offset

// j_code() function return type
// 0x0 means set code by hand
#define GB_CODE      0x0001
#define BIG5_CODE    0x0002
#define HZ_CODE      0x0004
#define UNI_CODE     0x0010
#define UTF7_CODE    0x0020
#define UTF8_CODE    0x0040
#define OTHER_CODE   0x8000

typedef unsigned int size_t;
typedef void FILE;

#ifdef __cplusplus
extern "C"
{
#endif

IMPORT_C char* _ogg_strstr(const char* str1, const char* str2);
IMPORT_C int _ogg_strcmp(const char* str1, const char* str2);
IMPORT_C int _ogg_strncmp(const char* str1, const char* str2, size_t num);
IMPORT_C char *_ogg_strcat(char *aDst, const char *aSrc);
IMPORT_C int _ogg_toupper(int c);
IMPORT_C void _ogg_qsort(void *base, size_t num, size_t width, int (*compare)(const void *, const void *));
IMPORT_C int _ogg_memcmp(const void* aBuf1, const void* aBuf2, size_t aNumBytes);
IMPORT_C void* _ogg_memcpy(void *aDst, const void *aSrc, size_t aNumBytes);
IMPORT_C void* _ogg_memmove(void *aDst, const void *aSrc, size_t aNumBytes);
IMPORT_C char* _ogg_strcpy(char *aDst, const char *aSrc);
IMPORT_C size_t _ogg_strlen(const char* aStr);
IMPORT_C long _ogg_labs(long aVal);
IMPORT_C int _ogg_abs(int aVal);
IMPORT_C void* _ogg_memset(void* aPtr, int aVal, size_t aNumBytes);
IMPORT_C void* _ogg_memchr(const void *aBuf, int aChr, size_t aNumBytes);
IMPORT_C void* _ogg_malloc(size_t aNumBytes);
IMPORT_C void* _ogg_calloc(size_t aNumItems, size_t aItemSize);
IMPORT_C void* _ogg_realloc(void* aPtr, size_t aNumBytes);
IMPORT_C void _ogg_free(void* aPtr);

IMPORT_C size_t _ogg_read(void* aBufPtr, size_t aReadSize, void* aFilePtr);
IMPORT_C int _ogg_seek(void* filePtr, long aOffset, int aSeekOrigin);
IMPORT_C int _ogg_tell(void* filePtr);
IMPORT_C int _ogg_close(void* filePtr);


IMPORT_C int jcode(const char* buff);

#ifdef __cplusplus
}
#endif

#define strstr(a, b) _ogg_strstr(a, b)
#define strcmp(a, b) _ogg_strcmp(a, b)
#define strncmp(a, b, c) _ogg_strncmp(a, b, c)

#define strcat(a, b) _ogg_strcat(a, b)
#define toupper(a) _ogg_toupper(a)
#define qsort(a, b, c, d) _ogg_qsort(a, b, c, d)

#define memcmp(a, b, c) _ogg_memcmp(a, b, c)
#define memcpy(a, b, c) _ogg_memcpy(a, b, c)
#define memmove(a, b, c) _ogg_memmove(a, b, c)
#define strcpy(a, b) _ogg_strcpy(a, b)
#define strlen(a) _ogg_strlen(a)
#define labs(a) _ogg_labs(a)
#define abs(a) _ogg_abs(a)

#define memset(a, b, c) _ogg_memset(a, b, c)
#define memchr(a, b, c) _ogg_memchr(a, b, c)

#define malloc(a) _ogg_malloc(a)
#define calloc(a, b) _ogg_calloc(a, b)
#define realloc(a, b) _ogg_realloc(a, b)
#define free(a) _ogg_free(a)

#endif // __OGGSHARED__
