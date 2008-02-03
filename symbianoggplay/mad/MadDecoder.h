/*
 *  Copyright (c) 2004 P. Wolff
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

#ifndef MADDECODER_H
#define MADDECODER_H

#include <e32base.h>
#include <f32file.h>

#include "libmad\mad.h"
#include "libid3tag\id3tag.h"
#include "madplay\xltagx.h"
#include "OggPlayDecoder.h"

const TInt KInputBufferSize = 8192;
const TInt KInputBufferMaxSize = 8192+MAD_BUFFER_GUARD;

// Seek table entry
class TMadFrameTimer
	{
public:
	mad_timer_t iFrameTimer; // frame time
	TInt iFramePos; // frame file position
	};

class CMadDecoder : public CBase, public MDecoder
{
public:
	CMadDecoder(RFs& aFs, const TBool& aDitherAudio, TAudioDither& aLeftDither, TAudioDither& aRightDither);
	~CMadDecoder();

	TInt Clear();
	TInt Open(const TDesC& aFilename);
	TInt OpenInfo(const TDesC& aFilename);
	TInt OpenComplete();

	TInt Read(TDes8& aBuffer, TInt Pos);
	void Close();

	TInt Channels();
	TInt Rate();
	TInt Bitrate();
	void Setposition(TInt64 aPosition);
	TInt64 TimeTotal();
	TInt FileSize();

	void ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber);

	void GetFrequencyBins(TInt32* aBins,TInt NumberOfBins);
	TBool RequestingFrequencyBins();

private:
	// Helper functions
	static TInt16 MadFixedToTInt16Fast(mad_fixed_t aSample);
	static TInt16 MadFixedToTInt16Dithered(mad_fixed_t aSample, TAudioDither* aDither, mad_fixed_t aRandom);

	TInt MadOutputPacket(TInt16* aPtr, const TInt16* aEndptr);
	TInt MadRead(TInt16* aOutputBuffer, TInt aLength);

	TInt MadHandleError(mad_stream& aStream, TInt &aFilePos, RFile* aFile = NULL);
	void MadSeek(const mad_timer_t& aNewTimer);

	void MadTimeTotal();
	TInt MadReadData(mad_stream& aStream, RFile& aFile, TInt& aFilePos, TInt& aBufFilePos, TUint8* aBuf);

	id3_tag* GetId3(id3_length_t tagsize);
	void ProcessId3();
	void Id3GetString(TDes& aString, char const *aId);

private: 
	mad_stream iStream;
	mad_frame iFrame;
	mad_synth iSynth;

	id3_tag *iId3tag;
	id3_file *iFiletag;
	tag iXLtag; // Xing and Lame tags.

	mad_timer_t	iTimer; // current time
	mad_timer_t iTotalTime; // total time

	TUint8 iInputBuffer[KInputBufferMaxSize];
	const TUint8* iOutputBufferEnd;

	TInt iFrameCount;
	TInt iRememberPcmSamples;

	TFileName iFileName;
	TInt iFileSize;
	TInt iFilePos;
	TInt iBufFilePos;
	RFile iFile;
	RFs& iFs;

	RArray<TMadFrameTimer> iTimerArray;
	TInt iTimerIdx;

	const TBool &iDitherAudio;
	TAudioDither &iLeftDither;
	TAudioDither &iRightDither;
	};

#endif
