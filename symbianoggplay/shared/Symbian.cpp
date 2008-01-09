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

#include <e32def.h>
#include <f32file.h>
#include <OggShared.h>

// C interface to Symbian OS file system and User functions
extern "C"
{
EXPORT_C char* _ogg_strstr(const char* str1, const char* str2)
{
	const char *resStr, *str2b;
	char chr1, chr2;
	if ((str1 == NULL) || (str2 == NULL))
		return (char *) str1;

	chr2 = *str2;
	if (chr2 == 0)
		return (char*) str1;

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
			} while (chr1 == chr2);

			chr2 = *str2;
		}
		else
		{
			str1++;
			chr1 = *str1;
		}
	}

	return NULL;
}

EXPORT_C int _ogg_strncmp(const char* str1, const char* str2, size_t num)
{
	size_t i;
	char chr1 = 0, chr2 = 0;
	if ((str1 == NULL) || (str2 == NULL))
		return 0;

	for (i = 0 ; i<num ; i++)
		{
		chr1 = str1[i];
		chr2 = str2[i];
		if ((chr1 != chr2) || !chr1)
			break;
		}

	return chr1-chr2;
}

EXPORT_C char *_ogg_strcat(char *aDst, const char *aSrc)
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
	while((*aDst++ = *aSrc++) != 0) { }

	return retPtr;
}

EXPORT_C int _ogg_toupper(int c)
{
	if ((c >= 'a') && ( c <= 'z'))
		return 'A' + (c - 'a');

	return  c;
}

static void _ogg_sort(void *base, size_t num, size_t width, int (*compare)(const void *, const void *))
{
	// Implement an efficient quick sort
	// (based upon the version in Numrical Recipies in Pascal)
	const TInt insertionNum = width * 11;
	TInt8* arrayStack[64];
	TInt stackIndex;
	TInt halfWidth = width >> 1;

	TInt8* iElement;
	TInt8* jElement;
	TInt8* kElement;
	TInt8* leftElement;
	TInt8* rightElement;
	TInt8* elementVal = (TInt8*) User::Alloc(width);

	stackIndex = 0;
	leftElement = (TInt8 *) base;
	rightElement = leftElement + width*(num-1);
	for ( ; ; )
	{
		if ((rightElement-leftElement)<insertionNum)
		{
			// Insertion sort the sub-array
			for (jElement = leftElement+width ; jElement<=rightElement ; jElement += width)
			{
				Mem::Copy(elementVal, jElement, width);
				for (iElement = leftElement ; iElement<jElement ; iElement += width)
				{
					if (compare(jElement, iElement) < 0)
					{
						Mem::Copy(iElement+width, iElement, jElement-iElement);
						Mem::Copy(iElement, elementVal, width);
						break;
					}
				}
			}

			// Pop the stack
			if (!stackIndex)
				break; // Job done

			leftElement = arrayStack[--stackIndex];
			rightElement = arrayStack[--stackIndex];
		}
		else
		{
			// Choose partitioning element
			// (median of left, middle and right elements of the current subarray)
			iElement = leftElement + width;
			jElement = rightElement;
			kElement = (TInt8 *) ((((TInt) leftElement) + ((TInt) rightElement)) >> 1);
			if (((TInt) (kElement - leftElement)) % width)
				kElement -= halfWidth;

			// Move the middle element to be after the left element
			Mem::Swap(iElement, kElement, width);

			// Arrange the three values in the correct order
			if (compare(leftElement, rightElement) > 0)
				Mem::Swap(leftElement, rightElement, width);

			if (compare(iElement, rightElement) > 0)
				Mem::Swap(iElement, rightElement, width);

			if (compare(leftElement, iElement) > 0)
				Mem::Swap(leftElement, iElement, width);

			// Do the partitioning
			kElement = iElement;
			for ( ; ; )
			{
				do
				{
					iElement += width;
				} while (compare(iElement, kElement) < 0);

				do
				{
					jElement -= width;
				} while (compare(jElement, kElement) > 0);

				if (jElement<iElement)
					break; // Partitioning done

				Mem::Swap(iElement, jElement, width);
			}

			// Move the partitioning element to it's correct place
			Mem::Swap(kElement, jElement, width);

			// Push the larger of the subarrays, process the other one
			if ((rightElement - (iElement+width)) > (jElement - leftElement))
			{
				arrayStack[stackIndex++] = rightElement;
				arrayStack[stackIndex++] = iElement;
				rightElement = jElement - width;
			}
			else
			{
				arrayStack[stackIndex++] = jElement - width;
				arrayStack[stackIndex++] = leftElement;
				leftElement = iElement;
			}
		}
	}

	User::Free(elementVal);
}

static void _ogg_aligned_sort(void *base, size_t num, int (*compare)(const void *, const void *))
{
	// Implement an efficient quick sort
	// (based upon the version in Numrical Recipies in Pascal)
	const TInt insertionNum = 11;
	TInt* arrayStack[64];
	TInt stackIndex;

	TInt* iElement;
	TInt* jElement;
	TInt* kElement;
	TInt* leftElement;
	TInt* rightElement;
	TInt elementVal;

	stackIndex = 0;
	leftElement = (TInt *) base;
	rightElement = leftElement + (num-1);
	for ( ; ; )
	{
		if ((rightElement-leftElement)<insertionNum)
		{
			// Insertion sort the sub-array
			for (jElement = leftElement+1 ; jElement<=rightElement ; jElement++)
			{
				elementVal = *jElement;
				for (iElement = leftElement ; iElement<jElement ; iElement++)
				{
					if (compare(jElement, iElement) < 0)
					{
						Mem::Move(iElement+1, iElement, 4*(jElement-iElement));
						*iElement = elementVal;
						break;
					}
				}
			}

			// Pop the stack
			if (!stackIndex)
				break; // Job done

			leftElement = arrayStack[--stackIndex];
			rightElement = arrayStack[--stackIndex];
		}
		else
		{
			// Choose partitioning element
			// (median of left, middle and right elements of the current subarray)
			iElement = leftElement + 1;
			jElement = rightElement;
			kElement = (TInt *) (((((TInt) leftElement) + ((TInt) rightElement)) >> 1) & 0xFFFFFFF0);

			// Move the middle element to be after the left element
			elementVal = *kElement;
			*kElement = *iElement;
			*iElement = elementVal;

			// Arrange the three values in the correct order
			if (compare(leftElement, rightElement) > 0)
			{
				elementVal = *leftElement;
				*leftElement = *rightElement;
				*rightElement = elementVal;
			}

			if (compare(iElement, rightElement) > 0)
			{
				elementVal = *iElement;
				*iElement = *rightElement;
				*rightElement = elementVal;
			}

			if (compare(leftElement, iElement) > 0)
			{
				elementVal = *leftElement;
				*leftElement = *iElement;
				*iElement = elementVal;
			}

			// Do the partitioning
			kElement = iElement;
			for ( ; ; )
			{
				do
				{
					iElement++;
				} while (compare(iElement, kElement) < 0);

				do
				{
					jElement--;
				} while (compare(jElement, kElement) > 0);

				if (jElement<iElement)
					break; // Partitioning done

				elementVal = *iElement;
				*iElement = *jElement;
				*jElement = elementVal;
			}

			// Move the partitioning element to it's correct place
			elementVal = *kElement;
			*kElement = *jElement;
			*jElement = elementVal;

			// Push the larger of the subarrays, process the other one
			if ((rightElement - (iElement+1)) > (jElement - leftElement))
			{
				arrayStack[stackIndex++] = rightElement;
				arrayStack[stackIndex++] = iElement;
				rightElement = jElement - 1;
			}
			else
			{
				arrayStack[stackIndex++] = jElement - 1;
				arrayStack[stackIndex++] = leftElement;
				leftElement = iElement;
			}
		}
	}
}

EXPORT_C void _ogg_qsort(void *base, size_t num, size_t width, int (*compare)(const void *, const void *))
{ 
	if (num == 0)
		return;

	if ((width == 4) && !(((TInt) base) & 0x03)) // likely
		_ogg_aligned_sort(base, num, compare);
	else
		_ogg_sort(base, num, width, compare);
}

EXPORT_C int _ogg_memcmp(const void* aBuf1, const void* aBuf2, size_t aNumBytes)
{
	return Mem::Compare((TUint8* ) aBuf1, aNumBytes, (TUint8 *) aBuf2, aNumBytes);
}

EXPORT_C void* _ogg_memcpy(void *aDst, const void *aSrc, size_t aNumBytes)
{
	Mem::Copy(aDst, aSrc, aNumBytes);
	return aDst;
}

EXPORT_C void* _ogg_memmove(void *aDst, const void *aSrc, size_t aNumBytes)
{
	Mem::Copy(aDst, aSrc, aNumBytes);
	return aDst;
}

EXPORT_C char* _ogg_strcpy(char *aDst, const char *aSrc)
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

EXPORT_C size_t _ogg_strlen(const char* aStr)
{
	if (aStr == NULL)
		return 0;

	const char* strBase = aStr;
	while(*aStr)
		aStr++;

	return aStr - strBase;
}

EXPORT_C long _ogg_labs(long aVal)
{
	return (aVal<0) ? -aVal : aVal;
}

EXPORT_C int _ogg_abs(int aVal)
{
	return (aVal<0) ? -aVal : aVal; 
}

EXPORT_C void* _ogg_memset(void* aPtr, int aVal, size_t aNumBytes)
{
	Mem::Fill(aPtr, aNumBytes, aVal);
	return aPtr;
}

EXPORT_C void* _ogg_memchr(const void *aBuf, int aChr, size_t aNumBytes)
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

EXPORT_C size_t _ogg_read(void* aBufPtr, size_t aReadSize, void* aFilePtr)
{
	RFile* file = (RFile *) aFilePtr;

	TPtr8 buf((TUint8 *) aBufPtr, 0, aReadSize);
	TInt err = file->Read(buf, aReadSize);

	return (err == KErrNone) ? buf.Length() : err;
}

EXPORT_C int _ogg_seek(void* filePtr, long aOffset, int aSeekOrigin)
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

EXPORT_C int _ogg_tell(void* filePtr)
{
	RFile* file = (RFile *) filePtr;
	TInt pos = 0;

	TInt err = file->Seek(ESeekCurrent, pos);
	return (err == KErrNone) ? pos : -1;
}

EXPORT_C int _ogg_close(void* filePtr)
{
	RFile* file = (RFile *) filePtr;
	
	file->Close();
	return 0;
}

EXPORT_C void* _ogg_malloc(size_t aNumBytes)
{
	return User::Alloc(aNumBytes);
}

EXPORT_C void* _ogg_calloc(size_t aNumItems, size_t aItemSize)
{
	TInt numBytes = aNumItems*aItemSize;
	void *ptr = User::Alloc(numBytes);
	if (ptr)
		Mem::FillZ(ptr, numBytes);

	return ptr;
}

EXPORT_C void* _ogg_realloc(void* aPtr, size_t aNumBytes)
{
	return User::ReAlloc(aPtr, aNumBytes);
}

EXPORT_C void _ogg_free(void* aPtr)
{
	User::Free(aPtr);
}
} // Extern "C"
