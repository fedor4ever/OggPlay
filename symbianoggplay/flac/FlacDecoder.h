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
 
#ifndef FLACDECODER_H
#define FLACDECODER_H

#include <e32base.h>
#include <f32file.h>

typedef unsigned int size_t;
#include "FLAC/stream_decoder.h"

#include "OggPlayDecoder.h"


class CDBFileAO : public CActive
	{
public:
	CDBFileAO(RFile& aFile, TUint8* aBuf, TInt aBufSize, TInt &aReadIdx, TInt& aDataSize, TInt& aFilePos);
	~CDBFileAO();

	void Read();
	TInt WaitForCompletion();

	// From CAcive
	void RunL();
	void DoCancel();

private:
	void ReadComplete(TInt aErr);

private:
	RFile& iFile;
	TBool iFileRequestPending;

	TUint8* iBuf;
	TInt iBufSize;
	TInt iHalfBufSize;
	TPtr8 iBufPtr;

	TInt& iReadIdx;
	TInt& iDataSize;
	TInt& iFilePos;
	};

const TInt KDefaultRFileDBBufSize = 131072; // 2 x 64K
class RDBFile : public RFile
	{
public:
	RDBFile(TInt aBufSize = KDefaultRFileDBBufSize);

	TInt Open(RFs& aFs, const TDesC& aFileName, TUint aMode);
	void Close();

	TInt Read(TDes8& aBuf);
	TInt Seek(TSeek aMode, TInt &aPos);

	void FileThreadTransfer();

private:
	void CopyData(TUint8* aBuf, TInt aSize);

private:
	TUint8* iBuf;
	TInt iBufSize;
	TInt iHalfBufSize;

	TInt iReadIdx;
	TInt iDataSize;

	CDBFileAO* iDBFileAO;
	TInt iFilePos;
	};

class CFlacDecoder : public CBase, public MDecoder
	{
public:
	CFlacDecoder(RFs& aFs);
	~CFlacDecoder();

	TInt Clear();
	TInt Open(const TDesC& aFilename);
	TInt OpenInfo(const TDesC& aFilename);
	TInt OpenComplete();
	
	TInt Read(TDes8& aBuffer, TInt Pos);
	void Close();

	TInt Channels();
	TInt Rate();
	TInt Bitrate();
	TInt64 TimeTotal();
	TInt FileSize();

	void Setposition(TInt64 aPosition);
	void ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber);
	void GetFrequencyBins(TInt32* aBins,TInt NumberOfBins);
	TBool RequestingFrequencyBins();

	void FileThreadTransfer();

	void GetString(TDes& aBuf, const char* aStr);

	void SetStreamInfo(const FLAC__StreamMetadata_StreamInfo* aStreamInfo); 
	void SetTitle(const TDesC& aTitle)
	{ *iTitle = aTitle; }

	void SetArtist(const TDesC& aArtist)
	{ *iArtist = aArtist; }

	void SetAlbum(const TDesC& aAlbum)
	{ *iAlbum = aAlbum; }

	void SetGenre(const TDesC& aGenre)
	{ *iGenre = aGenre; }

	void SetTrackNumber(const TDesC& aTrackNumber)
	{ *iTrackNumber = aTrackNumber; }

	void ParseBuffer(TInt aBlockSize, const FLAC__int32* const aBuffer[]);

protected:
	virtual FLAC__StreamDecoderInitStatus FLACInitStream(RDBFile* f);

protected:
	FLAC__StreamDecoder* iDecoder;

private:
	FLAC__StreamMetadata_StreamInfo iStreamInfo; 

	TDes* iTitle;
	TDes* iArtist;
	TDes* iAlbum;
	TDes* iGenre;
	TDes* iTrackNumber;

	const FLAC__int32* iOverflowBuf[2];
	TInt iOverflowBufSize;

	TUint16* iBuffer;
	TInt iLen;

	TInt iFileSize;

	RDBFile iFile;
	RFs& iFs;
	};

class CNativeFlacDecoder : public CFlacDecoder
	{
public:
	CNativeFlacDecoder(RFs& aFs);

protected:
	FLAC__StreamDecoderInitStatus FLACInitStream(RDBFile* f);
	};

#endif
