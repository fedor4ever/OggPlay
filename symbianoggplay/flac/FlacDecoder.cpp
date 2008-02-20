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


#include <charconv.h>
#include <utf.h>
#include <f32file.h>

// Platform settings
#include <OggOs.h>
#include <OggShared.h>

#include "FlacDecoder.h"
#include "OggLog.h"


CDBFileAO::CDBFileAO(RFile& aFile, TUint8* aBuf, TInt aBufSize, TInt &aReadIdx, TInt& aDataSize, TInt& aFilePos)
: CActive(EPriorityIdle), iFile(aFile), iBuf(aBuf), iBufSize(aBufSize), iHalfBufSize(aBufSize/2), iBufPtr(NULL, 0 ,0),
iReadIdx(aReadIdx), iDataSize(aDataSize), iFilePos(aFilePos)
	{
	CActiveScheduler::Add(this);
	}

CDBFileAO::~CDBFileAO()
	{
	}

void CDBFileAO::RunL()
	{
	ReadComplete(iStatus.Int());
	}

void CDBFileAO::ReadComplete(TInt aErr)
	{
	iFileRequestPending = EFalse;
	if (aErr != KErrNone)
		{
		TRACEF(COggLog::VA(_L("CDBFileAO Read Complete: %d"), aErr));
		return;
		}

	// Adjust the data size and file position
	iDataSize += iBufPtr.Size();
	iFilePos += iBufPtr.Size();
	}

void CDBFileAO::DoCancel()
	{
	if (iFileRequestPending)
		{
		// There is a file request pending, so cancel it
		// (or on earlier OS versions, just wait for it to complete)
#if defined(SERIES60V3)
		iFile.ReadCancel(iStatus);
#endif
		}
	else
		{
		// We are active, but no request is pending
		// Consequently we have to generate one
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrCancel);
		}

	iFileRequestPending = EFalse;
	}

void CDBFileAO::Read()
	{
	// Check if a read request is pending
	// If so we must return as we can only handle one request at a time
	if (iFileRequestPending)
		return;

	// Figure out which half buffer to read into
	TInt writeIdx;
	if (iReadIdx == iBufSize)
		{
		// The correct buffer depends on the amount of data we currently have
		writeIdx = (iDataSize) ? iHalfBufSize : 0;
		}
	else
		{
		// The correct buffer depends on which half the read idx is in
		writeIdx = (iReadIdx>=iHalfBufSize) ? 0 : iHalfBufSize;
		}

	// Issue the read request
	iBufPtr.Set(iBuf+writeIdx, 0, iHalfBufSize);
	iFile.Read(iBufPtr, iStatus);
	iFileRequestPending = ETrue;

	if (!IsActive())
		SetActive();
	}

TInt CDBFileAO::WaitForCompletion()
	{
	TInt err;
	if (IsActive())
		{
		// If we are still active the RunL() has not been called, so we must consume the event
		User::WaitForRequest(iStatus);

		// Handle the completion
		err = iStatus.Int();
		ReadComplete(err);

		// The caller might not make another read request, so we must reset the state
		iStatus = KRequestPending;
		}
	else
		err = iStatus.Int();

	return err;
	}


RDBFile::RDBFile(TInt aBufSize)
: iBuf(NULL), iBufSize(aBufSize), iHalfBufSize(aBufSize/2), iReadIdx(aBufSize), iDataSize(0), iDBFileAO(NULL)
	{
	}

TInt RDBFile::Open(RFs& aFs, const TDesC& aFileName, TUint aMode)
	{
	__ASSERT_DEBUG(!iBuf, User::Panic(_L("RDBFile::Open"), 0));

	iBuf = (TUint8 *) User::Alloc(iBufSize);
	return RFile::Open(aFs, aFileName, aMode);
	}

void RDBFile::Close()
	{
	if (iDBFileAO)
		{
		iDBFileAO->Cancel();

		delete iDBFileAO;
		iDBFileAO = NULL;
		}

	User::Free(iBuf);
	iBuf = NULL;

	iDataSize = 0;
	iReadIdx = iBufSize;

	RFile::Close();
	}

TInt RDBFile::Read(TDes8& aBuf)
	{
	if (!iDBFileAO)
		{
		iDBFileAO = new CDBFileAO(*this, iBuf, iBufSize, iReadIdx, iDataSize, iFilePos);
		if (!iDBFileAO)
			return KErrNoMemory;
		}

	TInt err = KErrNone;
	TInt readSize = 0;
	TInt maxReadSize = aBuf.MaxSize();
	TUint8* bufPtr = (TUint8 *) aBuf.Ptr();
	for ( ; ; )
		{
		// Copy the data if we have enough
		TInt dataToCopy = maxReadSize - readSize;
		if (iDataSize>=dataToCopy)
			{
			CopyData(bufPtr, dataToCopy);
			aBuf.SetLength(maxReadSize);
			break;
			}

		// We didn't have enough, copy what we've got
		dataToCopy = iDataSize;
		CopyData(bufPtr, dataToCopy);

		// Advance the buffer ptr
		bufPtr += dataToCopy;
		readSize += dataToCopy;

		// Issue another read
		iDBFileAO->Read();

		// Wait for the read to complete
		err = iDBFileAO->WaitForCompletion();
		if ((err != KErrNone) || (iDataSize == 0))
			{
			aBuf.SetLength(readSize);
			break;
			}
		}

	if (err != KErrNone)
		return err;

	// Read more if the remaining data is less than half the buffer
	// (i.e. we double-buffer the data)
	if (iDataSize<=iHalfBufSize)
		iDBFileAO->Read();

	return KErrNone;
	}

void RDBFile::CopyData(TUint8* aBuf, TInt aSize)
	{
	if ((iReadIdx+aSize)<=iBufSize)
		{
		Mem::Copy((TAny *) aBuf, (TAny *) (iBuf+iReadIdx), aSize);
		iReadIdx += aSize;
		}
	else
		{
		// Two copies required
		TInt endDataSize = iBufSize-iReadIdx;
		Mem::Copy((TAny *) aBuf, (TAny *) (iBuf+iReadIdx), endDataSize);

		iReadIdx = aSize - endDataSize;
		Mem::Copy((TAny *) (aBuf + endDataSize), (TAny *) iBuf, iReadIdx);
		}

	iDataSize -= aSize;
	}

void RDBFile::FileThreadTransfer()
	{
	if (!iDBFileAO)
		return;

	// Wait for any outstanding read to complete
	iDBFileAO->WaitForCompletion();

	// Cancel the AO
	iDBFileAO->Cancel();

	// Get rid of it (we're gonna need a new one)
	delete iDBFileAO;
	iDBFileAO = NULL;
	}

TInt RDBFile::Seek(TSeek aMode, TInt& aPos)
	{
	// Figure out if this is actually a seek or a current position request
	TBool readPosition = (aMode == ESeekCurrent) && (aPos == 0);
	if (readPosition)
		{
		// The file position will be out by the amount that we've
		// read ahead, so adjust the position accordingly
		aPos = iFilePos - iDataSize;
		return KErrNone;
		}

	// Seek "internally", if possible
	TInt destFilePos = aPos; // Only support seek start for now (as that's all we need)
	if ((destFilePos<=iFilePos) && (destFilePos>=(iFilePos-iDataSize)))
		{
		TInt seekDistance = iDataSize - (iFilePos - destFilePos);

		iDataSize -= seekDistance;
		iReadIdx += seekDistance;
		if (iReadIdx>=iBufSize)
			iReadIdx -= iBufSize;

		return KErrNone;
		}

	// Cancel any outstanding read
	if (iDBFileAO)
		iDBFileAO->Cancel();

	// Seek to the new position
	TInt err = RFile::Seek(aMode, aPos);
	if (err == KErrNone)
		{
		// Invalidate any existing data
		iDataSize = 0;
		iReadIdx = iBufSize;

		// Update the file position
		iFilePos = aPos;
		}

	return err;
	}

static FLAC__StreamDecoderReadStatus FlacReadCallback(const FLAC__StreamDecoder* /* aDecoder */, FLAC__byte aBuffer[], size_t *aBytes, void *aFilePtr)
	{
	size_t& bytes(*aBytes);
	if (!bytes)
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;

	RDBFile* file = (RDBFile *) aFilePtr;
	TPtr8 buf((TUint8 *) aBuffer, 0, bytes);
	TInt err = file->Read(buf);
	bytes = buf.Length();

	if ((err == KErrNone) && !buf.Length())
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	else if (err == KErrNone)
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;

	return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	}

static FLAC__StreamDecoderSeekStatus FlacSeekCallback(const FLAC__StreamDecoder* /* aDecoder */, FLAC__uint64 aByteOffset, void *aFilePtr)
	{
    RDBFile* file = (RDBFile *) aFilePtr;
	TInt byteOffset = (TInt) aByteOffset;
	TInt err = file->Seek(ESeekStart, byteOffset);

	return (err == KErrNone) ? FLAC__STREAM_DECODER_SEEK_STATUS_OK : FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	}

static FLAC__StreamDecoderTellStatus FlacTellCallback(const FLAC__StreamDecoder* /*aDecoder */, FLAC__uint64* aByteOffset, void *aFilePtr) 
	{
 	RDBFile* file = (RDBFile *) aFilePtr;
	TInt pos = 0;

	TInt err = file->Seek(ESeekCurrent, pos);
	*aByteOffset = (FLAC__uint64) pos;

	return (err == KErrNone) ? FLAC__STREAM_DECODER_TELL_STATUS_OK : FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	}

static FLAC__StreamDecoderLengthStatus FlacLengthCallback(const FLAC__StreamDecoder* /* aDecoder */, FLAC__uint64* aStreamLength, void *aFilePtr)
	{
 	RDBFile* file = (RDBFile *) aFilePtr;
	TInt size;
	TInt err = file->Size(size);
	*aStreamLength = (FLAC__uint64) size;

	return (err == KErrNone) ? FLAC__STREAM_DECODER_LENGTH_STATUS_OK : FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	}

static FLAC__bool FlacEofCallback(const FLAC__StreamDecoder* /* aDecoder */, void* /* aFilePtr */) 
	{
	return false;
	}

static FLAC__StreamDecoderWriteStatus FlacWriteCallback(const FLAC__StreamDecoder* aDecoder, const FLAC__Frame* aFrame, const FLAC__int32* const aBuffer[], void* /* aFilePtr*/)
	{
	((CFlacDecoder *) aDecoder->parent)->ParseBuffer(aFrame->header.blocksize, aBuffer);
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
	}

static void FlacMetadataCallback(const FLAC__StreamDecoder* aDecoder, const FLAC__StreamMetadata* aMetadata, void* /* aFilePtr */)
	{
	if (aMetadata->type == FLAC__METADATA_TYPE_STREAMINFO)
		((CFlacDecoder *) aDecoder->parent)->SetStreamInfo(&aMetadata->data.stream_info);
	else if (aMetadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
		{
		TBuf<256> buf;
		CFlacDecoder* decoder = (CFlacDecoder *) aDecoder->parent;
		const FLAC__StreamMetadata_VorbisComment& comments(aMetadata->data.vorbis_comment);
		TInt numComments = comments.num_comments;
		for (TInt i = 0 ; i<numComments ; i++)
			{
			char* s= (char *) comments.comments[i].entry;
			decoder->GetString(buf, s);
			buf.UpperCase();

			if (buf.Find(_L("TITLE="))==0)
				{
				decoder->GetString(buf, s+6);
				decoder->SetTitle(buf);
				}
			else if (buf.Find(_L("ARTIST="))==0)
				{
				decoder->GetString(buf, s+7);
				decoder->SetArtist(buf); 
				}
			else if (buf.Find(_L("ALBUM="))==0)
				{
				decoder->GetString(buf, s+6);
				decoder->SetAlbum(buf);
				}
			else if (buf.Find(_L("GENRE="))==0)
				{
				decoder->GetString(buf, s+6);
				decoder->SetGenre(buf);
				}
			else if (buf.Find(_L("TRACKNUMBER="))==0)
				{
				decoder->GetString(buf, s+12);
				decoder->SetTrackNumber(buf);
				}
			}
		}
	}

static void FlacErrorCallback(const FLAC__StreamDecoder* /* aDecoder */, FLAC__StreamDecoderErrorStatus /* aStatus */, void* /*aFilePtr */)
	{
	}

CFlacDecoder::CFlacDecoder(RFs& aFs)
: iFs(aFs)
	{
	}

CFlacDecoder::~CFlacDecoder() 
	{
	Close();
	}

TInt CFlacDecoder::Clear() 
	{
	if (iDecoder)
		{
		FLAC__stream_decoder_finish(iDecoder);
		FLAC__stream_decoder_delete(iDecoder);
		}

	iOverflowBufSize = 0;
	iDecoder = NULL;
	return 0;
	}

FLAC__StreamDecoderInitStatus CFlacDecoder::FLACInitStream(RDBFile* f)
	{
	return FLAC__stream_decoder_init_ogg_stream(iDecoder, FlacReadCallback, FlacSeekCallback,
	FlacTellCallback, FlacLengthCallback, FlacEofCallback, FlacWriteCallback, FlacMetadataCallback, FlacErrorCallback, (void *) f);
	}

TInt CFlacDecoder::Open(const TDesC& aFilename) 
	{
	return OpenInfo(aFilename);
	}

TInt CFlacDecoder::OpenInfo(const TDesC& aFileName) 
	{
	Close();

	TInt err = iFile.Open(iFs, aFileName, EFileShareReadersOnly);
	if (err != KErrNone)
		return err;

	err = iFile.Size(iFileSize);
	if (err != KErrNone)
		{
		iFile.Close();
		return err;
		}

	iDecoder = FLAC__stream_decoder_new();
	if (!iDecoder)
		{
		iFile.Close();
		return KErrNoMemory;
		}

	FLAC__stream_decoder_set_metadata_respond(iDecoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__StreamDecoderInitStatus status = FLACInitStream(&iFile);

	switch(status)
		{
		case FLAC__STREAM_DECODER_INIT_STATUS_OK:
			iDecoder->parent = (void *) this;
			err = KErrNone;
			break;

		case FLAC__STREAM_DECODER_INIT_STATUS_MEMORY_ALLOCATION_ERROR:
			err = KErrNoMemory;
			break;

		default:
			err = KErrCorrupt;
			break;
		}

	if (err != KErrNone)
		iFile.Close();

	return err;
	}

TInt CFlacDecoder::OpenComplete()
	{
	return KErrNone;
	}

void CFlacDecoder::Close()
	{
	Clear();
	iFile.Close();
	}

TInt CFlacDecoder::Read(TDes8& aBuffer, TInt aPos) 
	{
	iBuffer = (TUint16 *) (aBuffer.Ptr() + aPos);
	iLen = aBuffer.MaxLength()-aPos;
	TInt len = iLen;
	TInt lenSamples = len>>2;

	TInt i;
	TInt iMax;
	if (lenSamples>iOverflowBufSize)
		{
		iMax = iOverflowBufSize;
		iLen -= iOverflowBufSize << 2;
		}
	else
		{
		iMax = lenSamples;
		iLen = 0;
		}

	for (i = 0 ; i<iMax ; i++)
		{
		iBuffer[0] = (TUint16) iOverflowBuf[0][i];
		iBuffer[1] = (TUint16) iOverflowBuf[1][i];

		iBuffer += 2;
		}

	if (!iLen)
		{
		iOverflowBufSize -= i;
		return len;
		}

	iOverflowBufSize = 0;
	TBool frameRead = FLAC__stream_decoder_process_single(iDecoder);
	if (!frameRead)
		return KErrGeneral;

	return len - iLen;
	}

void CFlacDecoder::ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber) 
	{
	aTitle.SetLength(0); aArtist.SetLength(0); aAlbum.SetLength(0);
	aGenre.SetLength(0); aTrackNumber.SetLength(0);

	iTitle = &aTitle; iArtist = &aArtist; iAlbum = &aAlbum;
	iGenre = &aGenre; iTrackNumber = &aTrackNumber;

	FLAC__stream_decoder_process_until_end_of_metadata(iDecoder);
	}

void CFlacDecoder::GetString(TDes& aBuf, const char* aStr)
	{
	// according to ogg vorbis specifications, the text should be UTF8 encoded
	TPtrC8 p((const unsigned char *)aStr);
	CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);

#if !defined(MMF_PLUGIN) // Doesn't work when used in MMF Framework. Absolutely no clue why.
	// In the real world there are all kinds of coding being used, so we try to find out what it is
	TInt i= jcode((char*) aStr);
	if (i==BIG5_CODE)
		{
		CCnvCharacterSetConverter* conv= CCnvCharacterSetConverter::NewL();
		CCnvCharacterSetConverter::TAvailability a= conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierBig5, iFs);
		if (a==CCnvCharacterSetConverter::EAvailable)
			{
			TInt theState= CCnvCharacterSetConverter::KStateDefault;
			conv->ConvertToUnicode(aBuf, p, theState);
			}

		delete conv;
		}
#endif
	}

TInt CFlacDecoder::Channels()
	{
	return iStreamInfo.channels;
	}

TInt CFlacDecoder::Rate()
	{
	return iStreamInfo.sample_rate;
	}

TInt CFlacDecoder::Bitrate()
	{
	TInt64 totalSamples;
	Mem::Copy(&totalSamples, &iStreamInfo.total_samples, 8);

	TInt64 totalRate(TInt64(iFileSize)*8*iStreamInfo.sample_rate);

#if defined(SERIES60V3)
	return (TInt) (totalRate/totalSamples);
#else
	return (totalRate/totalSamples).GetTInt();
#endif
	}

void CFlacDecoder::Setposition(TInt64 aPosition)
	{
	aPosition = (aPosition * iStreamInfo.sample_rate) / 1000;
	iOverflowBufSize = 0;

	FLAC__uint64 newPosition;
	Mem::Copy(&newPosition, &aPosition, 8);
	FLAC__stream_decoder_seek_absolute(iDecoder, newPosition);
	}

TInt64 CFlacDecoder::TimeTotal()
	{
	TInt64 totalSamples;
	Mem::Copy(&totalSamples, &iStreamInfo.total_samples, 8);

	return (totalSamples*1000)/iStreamInfo.sample_rate;
	}

TInt CFlacDecoder::FileSize()
	{
	return iFileSize;
	}

void CFlacDecoder::GetFrequencyBins(TInt32* /* aBins */, TInt /* aNumberOfBins */)
	{
	}

TBool CFlacDecoder::RequestingFrequencyBins()
	{
	return EFalse;
	}

void CFlacDecoder::SetStreamInfo(const FLAC__StreamMetadata_StreamInfo* aStreamInfo)
	{
	iStreamInfo = *aStreamInfo;
	}

void CFlacDecoder::ParseBuffer(TInt aBlockSize, const FLAC__int32* const aBuffer[])
	{
	if (!iBuffer)
		return;

	TInt i;
	TInt iMax;
	TInt lenSamples = iLen >> 2;
	if (lenSamples>aBlockSize)
		{
		iMax = aBlockSize;
		iLen -= aBlockSize << 2;
		}
	else
		{
		iMax = lenSamples;
		iLen = 0;
		}

	for (i = 0 ; i<iMax ; i++)
		{
		iBuffer[0] = (TUint16) aBuffer[0][i];
		iBuffer[1] = (TUint16) aBuffer[1][i];

		iBuffer += 2;
		}

	iOverflowBufSize = aBlockSize - i;
	iOverflowBuf[0] = &aBuffer[0][i];
	iOverflowBuf[1] = &aBuffer[1][i];
	iBuffer = NULL;
	}

void CFlacDecoder::FileThreadTransfer()
	{
	iFile.FileThreadTransfer();
	}


CNativeFlacDecoder::CNativeFlacDecoder(RFs& aFs)
: CFlacDecoder(aFs)
	{
	}

FLAC__StreamDecoderInitStatus CNativeFlacDecoder::FLACInitStream(RDBFile* f)
	{
	return FLAC__stream_decoder_init_stream(iDecoder, FlacReadCallback, FlacSeekCallback,
	FlacTellCallback, FlacLengthCallback, FlacEofCallback, FlacWriteCallback, FlacMetadataCallback, FlacErrorCallback, (void *) f);
	}
