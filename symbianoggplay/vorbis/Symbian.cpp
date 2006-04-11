/*
 *  Copyright (c) 2005 S. Fisher
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

#include <f32file.h>
#include "iVorbisFile.h"

// C interface to Symbian OS file system and User functions
extern "C"
{
char* _ogg_strstr(const char* str1, const char* str2)
{
	const char *resStr, *str2b;
	char chr1, chr2;
	if ((str1 == NULL) || (str2 == NULL))
		return NULL;

	chr2 = *str2;
	if (chr2 == 0)
		return NULL;

	chr1 = *str1;
	while (chr1)
	{
		if (chr1 == chr2)
			{
			resStr = str1;
			str2b = str2;
			do
				{
				str2b++;
				chr2 = *str2b;
				if (chr2 == 0)
					return (char *) resStr;

				str1++;
				chr1 = *str1;
				if (chr1 == 0)
					return NULL;
				} while (chr1 == chr2);
			}

		str1++;
		chr1 = *str1;
	}

	return NULL;
}

int _ogg_strncmp(const char* str1, const char* str2, size_t num)
{
	size_t i;
	char chr1 = 0, chr2 = 0;
	if ((str1 == NULL) || (str2 == NULL))
		return 0;

	for (i = 0 ; i<num ; i++)
		{
		chr1 = str1[i];
		if (chr1 == 0)
			return 0;

		chr2 = str2[i];
		if (chr2 == 0)
			return 0;

		if (chr1 != chr2)
			break;
		}

	return chr1-chr2;
}

char *_ogg_strcat(char *aDst, const char *aSrc)
{
	// Check args
	if ((aDst == NULL) || (aSrc == NULL))
		return aDst;

	// Save the return ptr
	char* retPtr = aDst;

	// Scan to the end of the dst
	while(*aDst)
		aDst++;

	// Copy the source
	char srcChar = *aSrc;
	while(srcChar)
	{
		*aDst = srcChar;
		aSrc++ ; aDst++;

		srcChar = *aSrc;
	}

	*aDst = 0;
	return retPtr;
}

int _ogg_toupper(int c)
{
	if ((c >= 'a') && ( c <= 'z'))
		return 'A' + (c - 'a');

	return  c;
}

class TQKey : public TKey
{
public:
	TQKey(int (*aCompare)(const void *, const void *), TUint8* aBase, TInt aWidth);
	TInt Compare(TInt aLeft, TInt aRight) const;

public:
	int (*iCompare)(const void *, const void *);
	TUint8* iBase;
	TInt iWidth;
};

TQKey::TQKey(int (*aCompare)(const void *, const void *), TUint8* aBase, TInt aWidth)
: iCompare(aCompare), iBase(aBase), iWidth(aWidth)
{
}

TInt TQKey::Compare(TInt aLeft, TInt aRight) const
{
	TUint8* leftPtr = iBase + aLeft*iWidth; 
	TUint8* rightPtr = iBase + aRight*iWidth; 
	return (*iCompare)(leftPtr, rightPtr);
}

class TQSwap : public TSwap
{
public:
	TQSwap(TUint8* aBase, TInt aWidth);
	void Swap(TInt aLeft, TInt aRight) const;

public:
	TUint8* iBase;
	TInt iWidth;
};

TQSwap::TQSwap(TUint8* aBase, TInt aWidth)
: iBase(aBase), iWidth(aWidth)
{
}

void TQSwap::Swap(TInt aLeft, TInt aRight) const
{
	TUint8* leftPtr = iBase + aLeft*iWidth;
	TUint8* rightPtr = iBase + aRight*iWidth;
	Mem::Swap(leftPtr, rightPtr, iWidth);
}

void _ogg_qsort(void *base, size_t num, size_t width, int (*compare)(const void *, const void *))
{ 
	TUint8* base8 = (TUint8 *) base;
	TQKey key(compare, base8, width);
	TQSwap swap(base8, width);
	User::QuickSort(num, key, swap);
}

int _ogg_memcmp(const void* aBuf1, const void* aBuf2, size_t aNumBytes)
{
	return Mem::Compare((TUint8* ) aBuf1, aNumBytes, (TUint8 *) aBuf2, aNumBytes);
}

void* _ogg_memcpy(void *aDst, const void *aSrc, size_t aNumBytes)
{
	return Mem::Copy(aDst, aSrc, aNumBytes);
}

char* _ogg_strcpy(char *aDst, const char *aSrc)
{
	if ((aDst == NULL) || (aSrc == NULL))
		return aDst;

	char* retPtr = aDst;
	char srcChar = *aSrc;
	while(srcChar)
	{
		*aDst = srcChar;
		aSrc++ ; aDst++;

		srcChar = *aSrc;
	}

	*aDst = 0;
	return retPtr;
}

size_t _ogg_strlen(const char* aStr)
{
	if (aStr == NULL)
		return 0;

	const char* strBase = aStr;
	while(*aStr)
		aStr++;

	return aStr - strBase;
}

void _ogg_exit(int aVal)
{
	User::Exit(aVal);
}

long _ogg_labs(long aVal)
{
	return (aVal<0) ? -aVal : aVal;
}

int _ogg_abs(int aVal)
{
	return (aVal<0) ? -aVal : aVal; 
}

void* _ogg_memset(void* aPtr, int aVal, size_t aNumBytes)
{
	Mem::Fill(aPtr, aNumBytes, aVal);
	return aPtr;
}

void* _ogg_memchr(const void *aBuf, int aChr, size_t aNumBytes)
{
	const TUint8* ptr = (const TUint8*) aBuf;
	TUint8 chr8 = (TUint8) aChr;
	if ((ptr == NULL) || !aNumBytes)
		return NULL;

	for (TUint i = 0 ; i<aNumBytes ; i++, ptr++)
	{
		if (*ptr == chr8)
			return (void *) ptr;
	}

	return NULL;
}

size_t _ogg_read(void* aBufPtr, size_t aReadSize, void* aFilePtr)
{
	RFile* file = (RFile *) aFilePtr;

	TPtr8 buf((TUint8 *) aBufPtr, 0, aReadSize);
	TInt err = file->Read(buf, aReadSize);

	return (err == KErrNone) ? buf.Length() : err;
}

int _ogg_seek(void* filePtr, long aOffset, int aSeekOrigin)
{
	RFile* file = (RFile *) filePtr;
	TInt offset = aOffset;

	switch (aSeekOrigin)
	{
		case SEEK_SET:
			return file->Seek(ESeekStart, offset);

		case SEEK_CUR:
			return file->Seek(ESeekCurrent, offset);

		case SEEK_END:
			return file->Seek(ESeekEnd, offset);

		default:
			return -1;
	}

}

int _ogg_tell(void* filePtr)
{
	RFile* file = (RFile *) filePtr;
	TInt pos = 0;

	TInt err = file->Seek(ESeekCurrent, pos);
	return (err == KErrNone) ? pos : -1;
}

int _ogg_close(void* filePtr)
{
	RFile* file = (RFile *) filePtr;
	
	file->Close();
	return 0;
}

void* _ogg_malloc(size_t aNumBytes)
{
	return User::Alloc(aNumBytes);
}

void* _ogg_calloc(size_t aNumItems, size_t aItemSize)
{
	TInt numBytes = aNumItems*aItemSize;
	void *ptr = User::Alloc(numBytes);
	if (ptr)
		Mem::FillZ(ptr, numBytes);

	return ptr;
}

void* _ogg_realloc(void* aPtr, size_t aNumBytes)
{
	return User::ReAlloc(aPtr, aNumBytes);
}

void _ogg_free(void* aPtr)
{
	User::Free(aPtr);
}
} // Extern "C"
