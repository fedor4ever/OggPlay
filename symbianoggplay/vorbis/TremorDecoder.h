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
 
#ifndef TREMORDECODER_H
#define TREMORDECODER_H

#include <e32base.h>
#include <f32file.h>

#include "ivorbisfile.h"
#include "OggPlayDecoder.h"

#if defined(MULTITHREAD_SOURCE_READS)
#include "OggMTFile.h"
#endif

class COggHttpSource;
class CTremorDecoder: public CBase, public MDecoder
	{
public:
#if defined(MULTITHREAD_SOURCE_READS)
	CTremorDecoder(RFs& aFs, MMTSourceHandler& aSourceHandler1, MMTSourceHandler& aSourceHandler2);
#else
	CTremorDecoder(RFs& aFs);
#endif

	~CTremorDecoder();

	TInt Clear();
	TInt Open(const TDesC& aFileName, COggHttpSource* aHttpSource);
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
	void GetFrequencyBins(TInt32* aBins, TInt NumberOfBins);
	TBool RequestingFrequencyBins();

	void PrepareToSetPosition();
	void PrepareToPlay();
	void ThreadRelease();

	static TInt FifoLength(TAny* p, TBool aMax);
	static TPtr8 AsyncFeedGetBuffer(TAny* p, TInt bytes);
	static TInt AsyncFeedWrote(TAny* p, TInt bytes, TBool& aOpenComplete);

	TInt Section();
	TBool LastBuffer();

private:
	void GetString(TDes& aBuf, const char* aStr);

private:
	TInt iCurrentSection; // used in the call to ov_read
	OggVorbis_File iVf;
	vorbis_info *vi;

	RFs& iFs;

#if defined(MULTITHREAD_SOURCE_READS)
	RMTFile iFile;
#else
	RFile iFile;
#endif

	COggHttpSource* iHttpSource;
	};

#endif
