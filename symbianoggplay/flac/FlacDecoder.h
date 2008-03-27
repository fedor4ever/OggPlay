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

#if defined(MULTITHREAD_SOURCE_READS)
#include "OggMTFile.h"
#endif


class COggHttpSource;
class CFlacDecoder : public CBase, public MDecoder
	{
public:
#if defined(MULTITHREAD_SOURCE_READS)
	CFlacDecoder(RFs& aFs, MMTSourceHandler& aSourceHandler1, MMTSourceHandler& aSourceHandler2);
#else
	CFlacDecoder(RFs& aFs);
#endif
	~CFlacDecoder();

	TInt Clear();
	TInt Open(const TDesC& aFilename, COggHttpSource* aHttpSource);
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

	void PrepareToSetPosition();
	void PrepareToPlay();
	void ThreadRelease();

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

	TInt Section();
	TBool LastBuffer();

protected:
#if defined(MULTITHREAD_SOURCE_READS)
	virtual FLAC__StreamDecoderInitStatus FLACInitStream(RMTFile* f);
#else
	virtual FLAC__StreamDecoderInitStatus FLACInitStream(RFile* f);
#endif

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
	RFs& iFs;

#if defined(MULTITHREAD_SOURCE_READS)
	RMTFile iFile;
#else
	RFile iFile;
#endif
	};

class CNativeFlacDecoder : public CFlacDecoder
	{
public:
#if defined(MULTITHREAD_SOURCE_READS)
	CNativeFlacDecoder(RFs& aFs, MMTSourceHandler& aSourceHandler1, MMTSourceHandler& aSourceHandler2);
#else
	CNativeFlacDecoder(RFs& aFs);
#endif

protected:
#if defined(MULTITHREAD_SOURCE_READS)
	FLAC__StreamDecoderInitStatus FLACInitStream(RMTFile* f);
#else
	FLAC__StreamDecoderInitStatus FLACInitStream(RFile* f);
#endif
	};

#endif
