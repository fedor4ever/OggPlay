/*
 *  Copyright (c) 2008 S. Fisher
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

#include <OggOs.h>

// Support for multi-thread source reads
#if defined(MULTITHREAD_SOURCE_READS)
#include "OggMTFile.h"
#include "OggLog.h"

RMTFile::RMTFile(MMTSourceHandler& aSourceHandler1, MMTSourceHandler& aSourceHandler2)
: iSourceHandler1(aSourceHandler1), iSourceHandler2(aSourceHandler2), iThreadId(RThread().Id())
	{
	Mem::FillZ(&iSourceData, sizeof(TMTSourceData));
	}

TInt RMTFile::Open(const TDesC& aFileName, TInt aBufSize)
	{
	__ASSERT_DEBUG(!iSourceData.iBuf, User::Panic(_L("RDBFile::Open"), 0));

	iSourceData.iBuf = (TUint8 *) User::Alloc(aBufSize);
	if (!iSourceData.iBuf)
		return KErrNoMemory;

	iSourceData.iBufSize = aBufSize;
	iSourceData.iHalfBufSize = aBufSize >> 1;
	iSourceData.iReadIdx = aBufSize;
	iSourceHandler = &iSourceHandler1;
	return iSourceHandler->OpenFile(aFileName, iSourceData);
	}

void RMTFile::Close()
	{
	// Get the source handler
	if (!iSourceHandler)
		{
		TThreadId threadId = RThread().Id();
		iSourceHandler = (threadId == iThreadId) ? &iSourceHandler1 : &iSourceHandler2;
		}

	// Close the file
	iSourceHandler->SourceClose(iSourceData);

	// Free allocated memory
	User::Free(iSourceData.iBuf);
	Mem::FillZ(&iSourceData, sizeof(TMTSourceData));
	}

TInt RMTFile::Read(TDes8& aBuf)
	{
	// Get the source handler
	if (!iSourceHandler)
		{
		TThreadId threadId = RThread().Id();
		iSourceHandler = (threadId == iThreadId) ? &iSourceHandler1 : &iSourceHandler2;
		}

	TInt dataSize = iSourceData.DataSize();
	if ((iBufferingMode == EBufferNone) && !dataSize)
		return iSourceHandler->Read(iSourceData, aBuf);

	TInt maxReadSize = aBuf.MaxSize();
	TUint8* bufPtr = (TUint8 *) aBuf.Ptr();
	TInt dataToCopy, err = KErrNone;
	if (iBufferingMode == EBufferNone)
		{
		// Consume data from the buffer
		dataToCopy = maxReadSize;
		if (dataSize>=dataToCopy)
			{
			CopyData(bufPtr, dataToCopy);
			iSourceData.iDataRead += dataToCopy;

			aBuf.SetLength(maxReadSize);
			return KErrNone;
			}

		CopyData(bufPtr, dataSize);
		iSourceData.iDataRead += dataSize;
		bufPtr += dataSize;

		TPtr8 buf(bufPtr, 0, maxReadSize-dataSize);
		err = iSourceHandler->Read(iSourceData, buf);
		if (err == KErrNone)
			aBuf.SetLength(dataSize + buf.Length());

		return err;
		}

	TInt readSize = 0;
	for ( ; ; )
		{
		// Copy the data if we have enough
		dataToCopy = maxReadSize - readSize;
		if (dataSize>=dataToCopy)
			{
			CopyData(bufPtr, dataToCopy);
			iSourceData.iDataRead += dataToCopy;

			aBuf.SetLength(maxReadSize);
			break;
			}

		// We didn't have enough, copy what we've got
		CopyData(bufPtr, dataSize);
		iSourceData.iDataRead += dataSize;

		// Advance the buffer ptr
		bufPtr += dataSize;
		readSize += dataSize;

		// Read the next block of data
		err = iSourceHandler->Read(iSourceData);

		// Get the updated data size
		dataSize = iSourceData.DataSize();
		if ((err != KErrNone) || (dataSize == 0))
			{
			aBuf.SetLength(readSize);
			break;
			}
		}

	if (err != KErrNone)
		return err;

	// TO DO: Determine if scheduling another read here is worth doing or not
	// Driving the reads from MaoscBufferCopied() and after a read completes may
	// be sufficient or it may be better to drive the asynchronous reads from here
	// For: As soon as there is space to allow another read, we make the request
	// Against: A thread context switch is required to do it.

	// Read more asynchrnously if the remaining data is less than half the buffer
	// (i.e. double-buffer the data)
	if (!iSourceData.iLastBuffer && (iSourceData.DataSize()<=iSourceData.iHalfBufSize))
		iSourceHandler->ReadRequest(iSourceData);

	return KErrNone;
	}

void RMTFile::CopyData(TUint8* aBuf, TInt aSize)
	{
	if ((iSourceData.iReadIdx+aSize)<=iSourceData.iBufSize)
		{
		Mem::Copy((TAny *) aBuf, (TAny *) (iSourceData.iBuf+iSourceData.iReadIdx), aSize);
		iSourceData.iReadIdx += aSize;
		}
	else
		{
		// Two copies required
		TInt endDataSize = iSourceData.iBufSize-iSourceData.iReadIdx;
		Mem::Copy((TAny *) aBuf, (TAny *) (iSourceData.iBuf+iSourceData.iReadIdx), endDataSize);

		iSourceData.iReadIdx = aSize - endDataSize;
		Mem::Copy((TAny *) (aBuf + endDataSize), (TAny *) iSourceData.iBuf, iSourceData.iReadIdx);
		}
	}

TInt RMTFile::Seek(TSeek aMode, TInt &aPos)
	{
	// Figure out if this is actually a seek or a current position request
	TBool readPosition = (aMode == ESeekCurrent) && (aPos == 0);
	if (readPosition)
		{
		// Return the amount of data read
		aPos = iSourceData.iDataRead;
		return KErrNone;
		}

	// Get the source handler
	if (!iSourceHandler)
		{
		TThreadId threadId = RThread().Id();
		iSourceHandler = (threadId == iThreadId) ? &iSourceHandler1 : &iSourceHandler2;
		}

	// Seek to the new position
	TInt err = iSourceHandler->FileSeek(aMode, aPos, iSourceData);
	if (err == KErrNone)
		{
		// Invalidate any existing data
		iSourceData.iDataRead = aPos;
		iSourceData.iDataWritten = aPos;
		iSourceData.iReadIdx = iSourceData.iBufSize;
		iSourceData.iLastBuffer = EFalse;
		}

	return err;
	}

TInt RMTFile::Size(TInt& aSize)
	{
	if (!iSourceHandler)
		{
		TThreadId threadId = RThread().Id();
		iSourceHandler = (threadId == iThreadId) ? &iSourceHandler1 : &iSourceHandler2;
		}

	return iSourceHandler->FileSize(aSize, iSourceData);
	}

void RMTFile::EnableDoubleBuffering()
	{
	iBufferingMode = EDoubleBuffer;

	// Get the source handler
	if (!iSourceHandler)
		{
		TThreadId threadId = RThread().Id();
		iSourceHandler = (threadId == iThreadId) ? &iSourceHandler1 : &iSourceHandler2;
		}

	// Read some data (this is to help with playback start, EnableDoubleBuffering() gets called just before Play())
	if (!iSourceData.iLastBuffer && (iSourceData.DataSize()<=iSourceData.iHalfBufSize))
		iSourceHandler->Read(iSourceData);
	}

void RMTFile::DisableDoubleBuffering()
	{
	iBufferingMode = EBufferNone;

	// Get the source handler
	if (!iSourceHandler)
		{
		TThreadId threadId = RThread().Id();
		iSourceHandler = (threadId == iThreadId) ? &iSourceHandler1 : &iSourceHandler2;
		}

	iSourceHandler->ReadCancel(iSourceData);
	}

void RMTFile::ThreadRelease()
	{
	// Clear the source handler
	iSourceHandler = NULL;
	}
#endif // MULTITHREAD_SOURCE_READS
