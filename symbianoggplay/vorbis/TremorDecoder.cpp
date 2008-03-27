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


#include <charconv.h>
#include <utf.h>
#include <f32file.h>

// Platform settings
#include <OggOs.h>
#include <OggShared.h>

#include "TremorDecoder.h"
#include "OggHttpSource.h"
#include "OggLog.h"

#if defined(MULTITHREAD_SOURCE_READS)
CTremorDecoder::CTremorDecoder(RFs& aFs, MMTSourceHandler& aSourceHandler1, MMTSourceHandler& aSourceHandler2)
: iFs(aFs), iFile(aSourceHandler1, aSourceHandler2)
#else
CTremorDecoder::CTremorDecoder(RFs& aFs)
: iFs(aFs)
#endif
	{
	}

CTremorDecoder::~CTremorDecoder() 
	{
	Close();
	}

TInt CTremorDecoder::Clear() 
	{
	return ov_clear(&iVf);
	}

TPtr8 CTremorDecoder::AsyncFeedGetBuffer(TAny* p, TInt bytes)
	{
	CTremorDecoder* self = (CTremorDecoder*) p;
	return TPtr8(ogg_sync_bufferin(self->iVf.oy, bytes), bytes);
	}

TInt CTremorDecoder::AsyncFeedWrote(TAny* p, TInt bytes, TBool& aOpenComplete)
	{
	CTremorDecoder* self= (CTremorDecoder*)p;
	if (!self || !self->iVf.oy)
		return OV_EINVAL;

	TInt ret= ogg_sync_wrote(self->iVf.oy, bytes);
	if (ret == OGG_SUCCESS)
		{
		while (!(self->iVf.vi && self->iVf.vi->rate && self->iVf.vc && self->iVf.vc->vendor))
			{
			// Keep processing incoming stream until info and comment header is scanned
			ret = ov_read(&(self->iVf), NULL, 0, &(self->iCurrentSection));
			if (self->iVf.vi && self->iVf.vi->rate && self->iVf.vc && self->iVf.vc->vendor)
				aOpenComplete = ETrue;

			if (ret<1)
				break;
			}

		if (!self->iVf.oy)
			{
			// recheck pointer for the case of close of decoder!?
			return OV_EINVAL;
			}

		return self->iVf.oy->fifo_fill;
		}

	return ret;
	}

TInt CTremorDecoder::FifoLength(TAny* p, TBool aMax)
	{
	CTremorDecoder* self = (CTremorDecoder*) p;
	if (aMax)
		return 256*1024; // Actually limited only by free memory

	if (!self || !self->iVf.oy)
		return 0;

	return self->iVf.oy->fifo_fill;
	}

TInt CTremorDecoder::Open(const TDesC& aFileName, COggHttpSource* aHttpSource)
	{
	Close();

	iHttpSource = aHttpSource;
	if (iHttpSource)
		{
		// Initialise the source
		iHttpSource->AsyncFeeding(TAsyncCB(&FifoLength, &AsyncFeedGetBuffer, &AsyncFeedWrote, this));

		// Open the stream
		ov_asyncin_open(&iVf);
  		}

#if defined(MULTITHREAD_SOURCE_READS)
	TInt err = iFile.Open(aFileName, iHttpSource);
#else
	TInt err = iFile.Open(iFs, aFileName, EFileShareReadersOnly);
#endif

	if (err != KErrNone)
		return err;

	if (!iHttpSource)
		{
		err = ov_open(&iFile, &iVf, NULL, 0);
		if (err>=0)
			vi = ov_info(&iVf, -1);
		else
			err = KErrCorrupt; 

		if (err != KErrNone)
			iFile.Close();
		}

	return err;
	}

TInt CTremorDecoder::OpenInfo(const TDesC& aFileName) 
	{
	Close();

#if defined(MULTITHREAD_SOURCE_READS)
	TInt err = iFile.Open(aFileName, NULL);
#else
	TInt err = iFile.Open(iFs, aFileName, EFileShareReadersOnly);
#endif

	if (err != KErrNone)
		return err;

	err = ov_test(&iFile, &iVf, NULL, 0);
	if (err>=0)
		vi = ov_info(&iVf, -1);
	else
		err = KErrCorrupt; 

	if (err != KErrNone)
		iFile.Close();

	return err;
	}

TInt CTremorDecoder::OpenComplete()
	{
	return ov_test_open(&iVf);
	}

void CTremorDecoder::Close()
	{
	iFile.Close();
	Clear();
	}

TInt CTremorDecoder::Read(TDes8& aBuffer, TInt aPos) 
	{
	if (iHttpSource)
		{
		// The http code is implemented differently.
		// Access to the buffers is NOT necessarily thread safe, so we have to use a lock
		// By comparison the file stuff is thread safe because it copies the data instead.
		// For small copies (i.e. the small reads that all the codecs use) this is potentially
		// better than locking for two reasons: 1. There are no thread context switches required.
		// 2. As long as no paging happens, the time spent blocking is a small fixed duration.
		//
		// Another problem is that we don't go through the RMTFile::Read() API so we have to
		// make a check here to see if we are running low on data

		// For future development, it would be good to make both streams and files work the same way
		// (with slightly different buffering schemes to cope with the potentially very large difference in latency)
		// See also my comments in OggMTFile.cpp
		if (iHttpSource->CheckRead())
			iFile.ReadRequest();

		iHttpSource->Lock();
		}

	TInt err = ov_read(&iVf, (char *) &(aBuffer.Ptr()[aPos]), aBuffer.MaxLength()-aPos, &iCurrentSection);
	
	if (iHttpSource)
		iHttpSource->Unlock();

	return err;
	}

void CTremorDecoder::ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber) 
	{
	char** ptr = ov_comment(&iVf, -1)->user_comments;
	aArtist.SetLength(0);
	aTitle.SetLength(0);
	aAlbum.SetLength(0);
	aGenre.SetLength(0);
	aTrackNumber.SetLength(0);

	TBuf<256> buf;
	while (*ptr)
		{
		char* s = *ptr;
		GetString(buf, s);
		buf.UpperCase();

		if (buf.Find(_L("ARTIST="))==0)
			GetString(aArtist, s+7); 
		else if (buf.Find(_L("TITLE="))==0)
			GetString(aTitle, s+6);
		else if (buf.Find(_L("ALBUM="))==0)
			GetString(aAlbum, s+6);
		else if (buf.Find(_L("GENRE="))==0)
			GetString(aGenre, s+6);
		else if (buf.Find(_L("TRACKNUMBER="))==0)		
			GetString(aTrackNumber,s+12);

		++ptr;
		}
	}

void CTremorDecoder::GetString(TDes& aBuf, const char* aStr)
	{
	// according to ogg vorbis specifications, the text should be UTF8 encoded
	TPtrC8 p((const unsigned char *)aStr);
	CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);

	// In the real world there are all kinds of coding being used, so we try to find out what it is
#if !defined(MMF_PLUGIN) // Doesn't work when used in MMF Framework. Absolutely no clue why.
	TInt i= jcode((char*) aStr);
	if (i==BIG5_CODE)
		{
		CCnvCharacterSetConverter* conv = CCnvCharacterSetConverter::NewLC();
		CCnvCharacterSetConverter::TAvailability a= conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierBig5, iFs);
		if (a==CCnvCharacterSetConverter::EAvailable)
			{
			TInt theState= CCnvCharacterSetConverter::KStateDefault;
			conv->ConvertToUnicode(aBuf, p, theState);
			}

			CleanupStack::PopAndDestroy(conv);
		}
#endif
	}

TInt CTremorDecoder::Channels()
	{
	if (!vi)
		vi = ov_info(&iVf, -1);

	return vi->channels;
	}

TInt CTremorDecoder::Rate()
	{
	if (!vi)
		vi = ov_info(&iVf, -1);

	return vi->rate;
	}

TInt CTremorDecoder::Bitrate()
	{
	if (!vi)
		vi = ov_info(&iVf, -1);

	return vi->bitrate_nominal;
	}

void CTremorDecoder::Setposition(TInt64 aPosition)
	{
#if defined(SERIES60V3)
	ov_time_seek(&iVf, aPosition);
#else
	ogg_int64_t pos;
	Mem::Copy(&pos, &aPosition, 8);

	ov_time_seek(&iVf, pos);
#endif
	}

TInt64 CTremorDecoder::TimeTotal()
	{
	ogg_int64_t oggTime;
	oggTime = ov_time_total(&iVf, -1);

	if (oggTime<0)
		oggTime = 0;

#if defined(SERIES60V3)
	return oggTime;
#else
	TInt64 time;
	Mem::Copy(&time, &oggTime, 8);

	return time;
#endif
	}

TInt CTremorDecoder::FileSize()
	{
	ogg_int64_t oggSize = ov_raw_total(&iVf, -1);
	if (oggSize<0)
		oggSize = 0;

	return (TInt) oggSize; 
	}

void CTremorDecoder::GetFrequencyBins(TInt32* aBins,TInt aNumberOfBins)
	{
	TBool active = EFalse;
	if (aNumberOfBins)
		active = ETrue;

	ov_getFreqBin(&iVf, active, aBins);  
	}

TBool CTremorDecoder::RequestingFrequencyBins()
	{
	return ov_reqFreqBin(&iVf) ? ETrue : EFalse;
	}

void CTremorDecoder::PrepareToSetPosition()
	{
#if defined(MULTITHREAD_SOURCE_READS)
	// Release the file (the streaming thread will do the setting of the position)
	iFile.ThreadRelease();

	// Disable double buffering (seeking with buffering enabled is far too slow)
	iFile.DisableDoubleBuffering();
#endif
	}

void CTremorDecoder::PrepareToPlay()
	{
#if defined(MULTITHREAD_SOURCE_READS)
	// Enable double buffering (maximise reading performance)
	iFile.EnableDoubleBuffering();

	// Release the file (the streaming thread will do the initial buffering)
	iFile.ThreadRelease();
#endif
	}

void CTremorDecoder::ThreadRelease()
	{
#if defined(MULTITHREAD_SOURCE_READS)
	iFile.ThreadRelease();
#endif
	}

TInt CTremorDecoder::Section()
	{
	return iCurrentSection;
	}

TBool CTremorDecoder::LastBuffer()
	{
	return iFile.LastBuffer();
	}
