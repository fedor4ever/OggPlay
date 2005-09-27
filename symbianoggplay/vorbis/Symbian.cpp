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
size_t OggPlayRead(void* aBufPtr, size_t aReadSize, void* aFilePtr)
{
	RFile* file = (RFile *) aFilePtr;

	TPtr8 buf((TUint8 *) aBufPtr, 0, aReadSize);
	TInt err = file->Read(buf, aReadSize);

	return (err == KErrNone) ? buf.Length() : err;
}

int OggPlaySeek(void* filePtr, long aOffset, int aSeekOrigin)
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

int OggPlayTell(void* filePtr)
{
	RFile* file = (RFile *) filePtr;
	TInt pos = 0;

	TInt err = file->Seek(ESeekCurrent, pos);
	return (err == KErrNone) ? pos : -1;
}

int OggPlayClose(void* filePtr)
{
	RFile* file = (RFile *) filePtr;
	
	file->Close();
	return 0;
}

void* OggPlayAlloc(size_t aNumBytes)
{
	return User::Alloc(aNumBytes);
}

void* OggPlayCalloc(size_t aNumItems, size_t aItemSize)
{
	TInt numBytes = aNumItems*aItemSize;
	void *ptr = User::Alloc(numBytes);
	if (ptr)
		Mem::FillZ(ptr, numBytes);

	return ptr;
}

void* OggPlayReAlloc(void* aPtr, size_t aNumBytes)
{
	return User::ReAlloc(aPtr, aNumBytes);
}

void OggPlayFree(void* aPtr)
{
	User::Free(aPtr);
}
}
