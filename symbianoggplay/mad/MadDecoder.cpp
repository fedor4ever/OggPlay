/*
 *  Copyright (c) 2004 P.Wolff
 *
 * Based on :
 *     madlld.c, 
 *     (c) 2001--2004 Bertrand Petit											
 *
 *     madplay - MPEG audio decoder and player
 *     Copyright (C) 2000-2004 Robert Leslie
 *
 *     mpg321 - a fully free clone of mpg123
 *     Copyright (c) 2001 Joe Drew
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
#include <OggShared.h>

#if defined(MP3_SUPPORT)
#include <utf.h>

#include "MadDecoder.h"
#include "OggLog.h"

const mad_timer_t KMadTimerZero = { 0, 0 };
const mad_timer_t KTimeBetweenTimerEntries = { 5, 0 };


TAudioDither::TAudioDither(mad_fixed_t& aRandom)
: iRandom(aRandom)
	{
	}


CMadDecoder::CMadDecoder(RFs& aFs, const TBool& aDitherAudio, TAudioDither& aLeftDither, TAudioDither& aRightDither)
: iFs(aFs), iTimerArray(100), iTimerIdx(-1), iDitherAudio(aDitherAudio), iLeftDither(aLeftDither), iRightDither(aRightDither)
	{
	}

CMadDecoder::~CMadDecoder() 
	{
	Close();
	}

// Converts a sample from libmad's fixed point number format to a signed short (16 bits).
// Fast version (doesn't even bother rounding)
TInt16 CMadDecoder::MadFixedToTInt16Fast(mad_fixed_t aSample)
	{
	// A fixed point number is formed of the following bit pattern:

	// SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	// MSB                          LSB
	// S ==> Sign (0 is positive, 1 is negative)
	// W ==> Whole part bits
	// F ==> Fractional part bits

	// This pattern contains MAD_F_FRACBITS fractional bits, one
	// should alway use this macro when working on the bits of a fixed
	// point number. It is not guaranteed to be constant over the
	// different platforms supported by libmad.

	// The signed short value is formed, after clipping, by the least
	// significant whole part bit, followed by the 15 most significant
	// fractional part bits. Warning: this is a quick and dirty way to
	// compute the 16-bit number, madplay includes much better algorithms.

	// Clipping
	if (aSample>=MAD_F_ONE)
		return KMaxTInt16;

	if (aSample<-MAD_F_ONE)
		return KMinTInt16;

	// Conversion.
	aSample = aSample>>(MAD_F_FRACBITS-15);
	return (TInt16) aSample;
	}

// Converts a sample from libmad's fixed point number format to a signed short (16 bits).
// Dithering version (from MadPlay)
const mad_fixed_t KMask = (1L << (MAD_F_FRACBITS - 15)) - 1;
TInt16 CMadDecoder::MadFixedToTInt16Dithered(mad_fixed_t aSample, TAudioDither* aDither, mad_fixed_t aRandom)
	{
	mad_fixed_t output;

	enum
		{
		MIN = -MAD_F_ONE,
		MAX =  MAD_F_ONE - 1
		};

	// noise shape
	aSample += aDither->iError[0] - aDither->iError[1] + aDither->iError[2];

	aDither->iError[2] = aDither->iError[1];
	aDither->iError[1] = aDither->iError[0] / 2;

	// bias
	output = aSample + (1L << (MAD_F_FRACBITS - 16));

	// dither
	output += aRandom - aDither->iRandom;

	// clip
	if (output > MAX)
		{
		output = MAX;

		if (aSample > MAX)
			aSample = MAX;
		}
	else if (output < MIN)
		{
		output = MIN;

		if (aSample < MIN)
			aSample = MIN;
		}

	// quantize
	output &= ~KMask;

	// error feedback
	aDither->iError[0] = aSample - output;

	// scale
	return (TInt16) (output >> (MAD_F_FRACBITS - 15));
	}

// Helper function putting synthesized pcm samples into a buffer.
TInt CMadDecoder::MadOutputPacket(TInt16* aPtr, const TInt16* aEndPtr)
	{
	TInt maxSamples = aEndPtr-aPtr;
	TInt channels = MAD_NCHANNELS(&iFrame.header);
	if (channels == 2)
		maxSamples = maxSamples>>1;
	
	TInt availableSamples = iRememberPcmSamples ? iRememberPcmSamples : iSynth.pcm.length;
	if (maxSamples>availableSamples)
		maxSamples = availableSamples;

	TInt sampleIdx = iSynth.pcm.length - availableSamples;
	TInt maxSampleIdx = sampleIdx+maxSamples;
	if (iDitherAudio)
		{
		if (channels == 2)
			{
			for ( ; sampleIdx<maxSampleIdx ; sampleIdx++)
				{
				mad_fixed_t random  = ((mad_fixed_t) (((unsigned long) iLeftDither.iRandom) * 0x0019660DL + 0x3C6EF35FL)) & KMask;
				*(aPtr++) = MadFixedToTInt16Dithered(iSynth.pcm.samples[0][sampleIdx], &iLeftDither, random);
				*(aPtr++) = MadFixedToTInt16Dithered(iSynth.pcm.samples[1][sampleIdx], &iRightDither, random);

				iLeftDither.iRandom = random;
				}
			}
		else
			{
			for ( ; sampleIdx<maxSampleIdx ; sampleIdx++)
				{
				mad_fixed_t random  = ((mad_fixed_t) (((unsigned long) iLeftDither.iRandom) * 0x0019660DL + 0x3C6EF35FL)) & KMask;
				*(aPtr++) = MadFixedToTInt16Dithered(iSynth.pcm.samples[0][sampleIdx], &iLeftDither, random);

				iLeftDither.iRandom = random;
				}
			}
		}
	else
		{
		if (channels == 2)
			{
			for ( ; sampleIdx<maxSampleIdx ; sampleIdx++)
				{
				*(aPtr++) = MadFixedToTInt16Fast(iSynth.pcm.samples[0][sampleIdx]);
				*(aPtr++) = MadFixedToTInt16Fast(iSynth.pcm.samples[1][sampleIdx]);
				}
			}
		else
			{
			for ( ; sampleIdx<maxSampleIdx ; sampleIdx++)
				*(aPtr++) = MadFixedToTInt16Fast(iSynth.pcm.samples[0][sampleIdx]);
			}
		}

	iRememberPcmSamples = availableSamples - maxSamples;
	return maxSamples * 2 * channels; 
	}

TInt CMadDecoder::Clear()
	{
	iRememberPcmSamples = 0;
	iTotalTime = KMadTimerZero;
	iTimer = KMadTimerZero;

	iTimerArray.Reset();
	iTimerIdx = -1;

	iFrameCount = 0;
	iFilePos = 0;

	// Clear mad structures
	mad_synth_finish(&iSynth);
	mad_frame_finish(&iFrame);
	mad_stream_finish(&iStream);

	mad_stream_init(&iStream);
	mad_frame_init(&iFrame);
	mad_synth_init(&iSynth);

	// Clear Xing / Lame tag
	tag_finish(&iXLtag);
	Mem::FillZ(&iXLtag, 0);

	return KErrNone;
	}

TInt CMadDecoder::Open(const TDesC& aFileName)
	{
	return OpenInfo(aFileName);
	}

TInt CMadDecoder::OpenInfo(const TDesC& aFileName)
	{
	Close();

	iFileName = aFileName;
	TInt err = iFile.Open(iFs, iFileName, EFileShareReadersOnly);
	if (err != KErrNone)
		return err;

	err = iFile.Size(iFileSize);
	if (err != KErrNone)
		{
		iFile.Close();
		return err;
		}

	// Initialise mad structures
	mad_stream_init(&iStream);
	mad_frame_init(&iFrame);
	mad_synth_init(&iSynth);

	// Init Xing / Lame tag
	tag_init(&iXLtag);

	// Dummy read to fill header values
	err = MadRead(NULL, 0);
	if (err != KErrNone)
		{
		iFile.Close();
		return err;
		}

	if (iId3tag)
		return KErrNone; // there was an ID3v2 tag at the beginning. Good.

	// If there wasn't we open the file and look for an ID3v1 tag
	RFile* file = new(ELeave) RFile;
	err = file->Open(iFs, iFileName, EFileShareReadersOnly);
	if (err == KErrNone)
		iFiletag = id3_file_fdopen((int) file, ID3_FILE_MODE_READONLY);

	return KErrNone;
	}

TInt CMadDecoder::OpenComplete()
	{
	return KErrNone;
	}

void CMadDecoder::Close()
	{
	Clear();

	if (iFiletag)
		{
		RFile *file = (RFile*) iFiletag->iofile;
		id3_file_close(iFiletag);
		delete file;

		iFiletag = NULL;
		}

	if(iId3tag)
		{ 
		id3_tag_delete(iId3tag); 
		iId3tag = NULL;
		}

	// Clear mad structures
	mad_synth_finish(&iSynth);
	mad_frame_finish(&iFrame);
	mad_stream_finish(&iStream);

	iFile.Close();
	}

TInt CMadDecoder::Read(TDes8& aBuffer, TInt aPos) 
	{
	TInt16* outputBuffer = (TInt16 *) (aBuffer.Ptr() + aPos);
	TInt length = aBuffer.MaxLength() - aPos;

	return MadRead(outputBuffer, length>>1);
	}


// MadRead, similar to ov_read from Tremor:
// tries to put length bytes of packed PCM data into buffer

// return value:
// <0) error/hole in data 
// 0) EOF
// n) number of bytes of PCM actually returned.  
  
// We try to fill the buffer completely though.
// Passing aOutputBuffer = NULL will read the header values.
TInt CMadDecoder::MadRead(TInt16* aOutputBuffer, TInt aLength)
	{
	const TInt16 *outputBufferEnd = aOutputBuffer + aLength;
	TInt bytesWritten = 0;
	if (iRememberPcmSamples)
		{
		TInt rememberBytesWritten = MadOutputPacket(aOutputBuffer, outputBufferEnd);
		bytesWritten += rememberBytesWritten;
		aOutputBuffer += rememberBytesWritten>>1;
		}

	// It still doesn't fit - so return and try next time
	if (iRememberPcmSamples)
		return bytesWritten;

	// Set up next seek table entry
	mad_timer_t nextTime = KMadTimerZero;
	if (iTimerIdx>=0)
		{
		TInt nextTimerIdx = iTimerIdx+1;
		if (nextTimerIdx<iTimerArray.Count())
			nextTime = iTimerArray[nextTimerIdx].iFrameTimer;
		else
			{
			nextTime = iTimerArray[iTimerIdx].iFrameTimer;
			mad_timer_add(&nextTime, KTimeBetweenTimerEntries);
			}
		}

	TInt err;
	for ( ; ; )
		{
		// The input buffer must be filled if it becomes empty or if it's the first execution of the loop.
		if (!iStream.buffer || (iStream.error == MAD_ERROR_BUFLEN))
			{
			iBufFilePos = iFilePos;
			err = MadReadData(iStream, iFile, iFilePos, iBufFilePos, iInputBuffer);
			if (err == KErrEof)
				return bytesWritten;

			if (err != KErrNone)
				return err;
			}

		// Decode the next MPEG frame. The streams is read from the
		// buffer, its constituents are break down and stored the the
		// Frame structure, ready for examination/alteration or PCM
		// synthesis. Decoding options are carried in the Frame
		// structure from the Stream structure.

		// Error handling: mad_frame_decode() returns a non zero value
		// when an error occurs. The error condition can be checked in
		// the error member of the Stream structure. A mad error is
		// recoverable or fatal, the error status is checked with the
		// MAD_RECOVERABLE macro.

		// {4} When a fatal error is encountered all decoding
		// activities shall be stopped, except when a MAD_ERROR_BUFLEN
		// is signaled. This condition means that the
		// mad_frame_decode() function needs more input to complete
		// its work. One shall refill the buffer and repeat the
		// mad_frame_decode() call. Some bytes may be left unused at
		// the end of the buffer if those bytes forms an incomplete
		// frame. Before refilling, the remaining bytes must be moved
		// to the beginning of the buffer and used for input for the
		// next mad_frame_decode() invocation. (See the comments
		// marked {2} earlier for more details.)

		// Recoverable errors are caused by malformed bit-streams, in
		// this case one can call again mad_frame_decode() in order to
		// skip the faulty part and re-sync to the next frame.
		if (mad_header_decode(&iFrame.header, &iStream))
			{
			if (MAD_RECOVERABLE(iStream.error))
				{
				MadHandleError(iStream, iFilePos);
				continue;
				}
			else
				{
				if (iStream.error == MAD_ERROR_BUFLEN)
					continue;
				else
					{
					TRACEF(COggLog::VA(_L("Mad Seek fatal error: %d"), iStream.error));
					return KErrCorrupt;
					}
				}
			}

		// Accounting. Add seek table entries as we go
		if (mad_timer_compare(iTimer, nextTime)>=0)
			{
			iTimerIdx++;
			if (iTimerIdx == iTimerArray.Count())
				{
				// Try to add a new seek table entry
				TMadFrameTimer frameTimer;
				frameTimer.iFrameTimer = iTimer;
				frameTimer.iFramePos = iBufFilePos + (iStream.this_frame - iInputBuffer);
				err = iTimerArray.Append(frameTimer);
				if (err != KErrNone)
					iTimerIdx--;

				nextTime = iTimer;
				mad_timer_add(&nextTime, KTimeBetweenTimerEntries);
				}
			else
				nextTime = iTimerArray[iTimerIdx].iFrameTimer;
			}

		// More accounting, update the "current position"
		mad_timer_add(&iTimer, iFrame.header.duration);

		// Now we can decode the audio...
		if (mad_frame_decode(&iFrame, &iStream))
			{
			if (MAD_RECOVERABLE(iStream.error))
				{
				MadHandleError(iStream, iFilePos);
				continue;
				}
			else
				{
				if (iStream.error == MAD_ERROR_BUFLEN)
					continue;
				else
					{
					TRACEF(COggLog::VA(_L("MadRead fatal error: %d"), iStream.error));
					return KErrCorrupt;
					}
				}
			}

		// Xing/Lame tag (I don't know why this comes after the first frame, but it does)
		if (!iFrameCount)
			{
			// Attempt to decode the Xing tag
			if ((iXLtag.flags == 0) && tag_parse(&iXLtag, &iStream) == 0)
				{
				// We currently just use the Xing tag to get the total duration
				// In future we could also extract the TOC and add those entries into the seek table
				// With the current 20 second seeking there isn't really any need to do this ("First time FF" is fast enough on my N82 and it's "ok" on the N70)
				// However, adpative FF/RW is on the list of things to do, so this issue would be worth re-visiting then. (e.g for seeking very long mp3s / podcasts)
				// And of course the P900 has a touch screen, so the user can seek anywhere if they want to
				// It might also be good to generate some seek entries for CBR mp3s too
				// (Assuming they don't have a Xing tag that is. At the moment I'm not sure if Xing tags ever get added to CBR mp3s)

				// Some googling found me this page that describes how the TOC works (http://www.multiweb.cz/twoinches/MP3inside.htm#vbr)
				// The TOC has 100 entries, so eg. if you want to seek 25% of the way through the music you go to (TOC[25]*fileSize)/256
				// I'm not sure if that gives you an offset from the start of the file or from the start of first frame (investigation required)
				// The Xing tag also contains the length of the file in bytes, but again it's not clear if this "file length" includes the tags or not.
				if ((iXLtag.flags & TAG_XING) && (iXLtag.xing.flags & TAG_XING_FRAMES))
					{
					iTotalTime = iFrame.header.duration;
					mad_timer_multiply(&iTotalTime, iXLtag.xing.frames);
					}

				// Adjust file size
				iFileSize -= iStream.next_frame - iStream.this_frame;
				if (iFileSize<0)
					iFileSize = 0;

				continue;
				}
			}

		// Increment the frame count (this is only used for the Xing tag check)
		iFrameCount++;

		// Once decoded the frame is synthesized to PCM samples. No errors
		// are reported by mad_synth_frame();
		mad_synth_frame(&iSynth, &iFrame);

		// Synthesized samples must be converted from libmad's fixed
		// point number to the consumer format. Here we use unsigned
		// 16 bit little endian integers on two channels. Integer samples
		// are temporarily stored in a buffer that is flushed when full.
		if (aOutputBuffer)
			{
			TInt currentBytesWritten = MadOutputPacket(aOutputBuffer, outputBufferEnd);
			bytesWritten += currentBytesWritten;
			aOutputBuffer += currentBytesWritten>>1;
      
			// It's full already
			if (!currentBytesWritten || iRememberPcmSamples)
				return bytesWritten;
			}
		else
			{
			// Remember the remaining samples
			iRememberPcmSamples = iSynth.pcm.length;
			return bytesWritten;
			}
		}
	
	return KErrNone; // (Unreachable)
	}

TInt CMadDecoder::MadReadData(mad_stream& aStream, RFile& aFile, TInt& aFilePos, TInt& aBufFilePos, TUint8* aBuf)
	{
	TUint readSize;
	TUint remaining;
	TUint8 *readStart;

	// {1} When decoding from a file we need to know when the end of
	// the file is reached at the same time as the last bytes are read.
	// We thus need to keep track of the current file position by
	// updating aFilePos (see also the comment marked {3} below).

	// {2} libmad may not consume all bytes of the input
	// buffer. If the last frame in the buffer is not wholly
	// contained by it, then that frame's start is pointed by
	// the next_frame member of the Stream structure. This
	// common situation occurs when mad_frame_decode() fails,
	// sets the stream error code to MAD_ERROR_BUFLEN, and
	// sets the next_frame pointer to a non NULL value. (See
	// also the comment marked {4} bellow.)

	// When this occurs, the remaining unused bytes must be
	// put back at the beginning of the buffer and taken in
	// account before refilling the buffer. This means that
	// the input buffer must be large enough to hold a whole
	// frame at the highest observable bit-rate (currently 448
	// kb/s). XXX=XXX Is 2016 bytes the size of the largest
	// frame? (448000*(1152/32000))/8
	if (aStream.next_frame)
		{
		remaining = aStream.bufend - aStream.next_frame;
		Mem::Copy((TAny *) aBuf, aStream.next_frame, remaining);

		readStart = aBuf+remaining;
		readSize = KInputBufferSize-remaining;

		aBufFilePos -= remaining;
		}
	else
		{
		readSize = KInputBufferSize,
		readStart = aBuf;
		remaining = 0;
		}

	TPtr8 readBuf(readStart, readSize);
	TInt err = aFile.Read(readBuf);
	if (err != KErrNone)
		return err;

	readSize = readBuf.Length();
	aFilePos += readSize;

	if (!readSize) // Eof
		return KErrEof;

	// {3} When decoding the last frame of a file, it must be
	// followed by MAD_BUFFER_GUARD zero bytes if one wants to
	// decode that last frame. When the end of file is
	// detected we append that quantity of bytes at the end of
	// the available data. Note that the buffer can't overflow
	// as the guard size was allocated but not used by the
	// buffer management code. (See also the comment marked
	// {1}.)

	// In a message to the mad-dev mailing list on May 29th,
	// 2001, Rob Leslie explains the guard zone as follows:

	//  "The reason for MAD_BUFFER_GUARD has to do with the
	//  way decoding is performed. In Layer III, Huffman
	//  decoding may inadvertently read a few bytes beyond
	//  the end of the buffer in the case of certain invalid
	//  input. This is not detected until after the fact. To
	//  prevent this from causing problems, and also to
	//  ensure the next frame's main_data_begin pointer is
	//  always accessible, MAD requires MAD_BUFFER_GUARD
	//  (currently 8) bytes to be present in the buffer past
	//  the end of the current frame in order to decode the
	// frame."
	if (aFilePos == iFileSize)
		{
		// End of file reached, add the guard bytes to the end of the buffer
		Mem::FillZ(readStart+readSize, MAD_BUFFER_GUARD);
		readSize += MAD_BUFFER_GUARD;
		}

	// Pipe the new buffer content to libmad's stream decoder
	mad_stream_buffer(&aStream, aBuf, readSize+remaining);
	aStream.error = MAD_ERROR_NONE;		
	return KErrNone;
	}

TInt CMadDecoder::Channels()
	{
	return MAD_NCHANNELS(&iFrame.header);
	}

TInt CMadDecoder::Rate()
	{
	return iFrame.header.samplerate;
	}

TInt CMadDecoder::Bitrate()
	{
	// Divide the total number of bits by the length of the track
	TInt64 timeTotal = TimeTotal();
	if (timeTotal == 0)
		return 0;

	TInt64 bitTotal = TInt64(8000)*FileSize();
#if defined(SERIES60V3)
	return (TInt) (bitTotal / timeTotal);
#else
	return (bitTotal / timeTotal).GetTInt();
#endif
	}

void CMadDecoder::Setposition(TInt64 aPosition)
	{
	// Reset the mad decoder
	mad_synth_finish(&iSynth);
	mad_frame_finish(&iFrame);
	mad_stream_finish(&iStream);

	mad_stream_init(&iStream);
	mad_frame_init(&iFrame);
	mad_synth_init(&iSynth);
	iRememberPcmSamples = 0;

	// Seek to the nearest frame marker
	mad_timer_t newTimer;

#if defined(SERIES60V3)
	mad_timer_set(&newTimer, 0, (unsigned long) aPosition, 1000);
#else
	mad_timer_set(&newTimer, 0, (unsigned long) aPosition.GetTInt(), 1000);
#endif

	TInt i;
	TInt newFilePosition;
	if (iTimerIdx>=0)
		{
		mad_timer_t& currentTimer = iTimerArray[iTimerIdx].iFrameTimer;
		if (mad_timer_compare(currentTimer, newTimer)>0)
			{
			// New time is earlier, so seek backwards through the entries
			for (i = iTimerIdx-1 ; i>=0 ; i--)
				{
				if (mad_timer_compare(iTimerArray[i].iFrameTimer, newTimer)<=0)
					break;
				}

			newFilePosition = iTimerArray[i].iFramePos;
			iTimerIdx = i;
			}
		else
			{
			// New time is later, so seek forwards through the entries (if there are any ahead of us)
			TInt timerArrayCount = iTimerArray.Count();
			for (i = iTimerIdx ; i<(timerArrayCount-1) ; i++)
				{
				if (mad_timer_compare(iTimerArray[i+1].iFrameTimer, newTimer)>0)
					break;
				}

			newFilePosition = iTimerArray[i].iFramePos;
			iTimerIdx = i;
			}
		}
	else
		newFilePosition = 0;

	// Seek the file to that header
	TInt err = iFile.Seek(ESeekStart, newFilePosition);
	if (err == KErrNone)
		{
		if (iTimerIdx>=0)
			iTimer = iTimerArray[iTimerIdx].iFrameTimer;
		else
			iTimer = KMadTimerZero;

		iFilePos = newFilePosition;
		}

	// Now scan forward until we get there
	MadSeek(newTimer);
	}

void CMadDecoder::MadSeek(const mad_timer_t& aNewTimer)
	{
	// Set up next seek table entry
	// (although if there is one we will exit before we get to it)
	mad_timer_t nextTime = KMadTimerZero;
	if (iTimerIdx>=0)
		{
		TInt nextTimerIdx = iTimerIdx+1;
		if (nextTimerIdx<iTimerArray.Count())
			nextTime = iTimerArray[nextTimerIdx].iFrameTimer;
		else
			{
			nextTime = iTimerArray[iTimerIdx].iFrameTimer;
			mad_timer_add(&nextTime, KTimeBetweenTimerEntries);
			}
		}

	TInt err;
	for ( ; ; )
		{
		// The input buffer must be filled if it becomes empty or if it's the first execution of the loop.
		if (!iStream.buffer || (iStream.error == MAD_ERROR_BUFLEN))
			{
			iBufFilePos = iFilePos;
			err = MadReadData(iStream, iFile, iFilePos, iBufFilePos, iInputBuffer);
			if (err != KErrNone)
				{
				TRACEF(COggLog::VA(_L("Mad Seek fatal error: %d"), err));
				return;
				}
			}

		if (mad_header_decode(&iFrame.header, &iStream))
			{
			if (MAD_RECOVERABLE(iStream.error))
				{
				MadHandleError(iStream, iFilePos);
				continue;
				}
			else
				{
				if (iStream.error == MAD_ERROR_BUFLEN)
					continue;
				else
					{
					TRACEF(COggLog::VA(_L("Mad Seek fatal error: %d"), iStream.error));
					return;
					}
				}
			}

		// Accounting. Add seek table entries as we go
		if (mad_timer_compare(iTimer, nextTime)>=0)
			{
			iTimerIdx++;
			if (iTimerIdx == iTimerArray.Count())
				{
				// Try to add a new seek table entry
				TMadFrameTimer frameTimer;
				frameTimer.iFrameTimer = iTimer;
				frameTimer.iFramePos = iBufFilePos + (iStream.this_frame - iInputBuffer);
				err = iTimerArray.Append(frameTimer);
				if (err != KErrNone)
					iTimerIdx--;

				nextTime = iTimer;
				mad_timer_add(&nextTime, KTimeBetweenTimerEntries);
				}
			else
				nextTime = iTimerArray[iTimerIdx].iFrameTimer;
			}

		// More accounting, update the "current position"
		mad_timer_add(&iTimer, iFrame.header.duration);

		// Finally, check if we have got to where we are supposed to be going
		if (mad_timer_compare(iTimer, aNewTimer)>=0)
			break;
		}
	}

TInt64 CMadDecoder::TimeTotal()
	{
	if (mad_timer_compare(iTotalTime, KMadTimerZero) == 0)
		{
		// Not found in tags, so we have to figure it out
		MadTimeTotal();
		}

	signed long timer = mad_timer_count(iTotalTime, MAD_UNITS_MILLISECONDS);
	return TInt64((TInt) timer);
	}

void CMadDecoder::MadTimeTotal()
	{
	// In principle we should scan the file to get the total time.
	// In practice this is too slow, and most likely if there are no tags the file is CBR anyway.
#if defined(SERIES60V3)
	TInt64 time = (TInt64(8000)*iFileSize) / TInt64(iFrame.header.bitrate);
	mad_timer_set(&iTotalTime, (TInt) (time/1000), (TInt) (time%1000), 1000);
#else
	TInt64 time = (TInt64(8000)*iFileSize) / TInt64((TInt) iFrame.header.bitrate);
 	mad_timer_set(&iTotalTime, (time/1000).GetTInt(), (time%1000).GetTInt(), 1000);
#endif

	/* // Scan the whole file to determine the total duration
	mad_stream stream;
	mad_stream_init(&stream);

	mad_header header;
	mad_header_init(&header);

	RFile file;
	TInt err = file.Open(iFs, iFileName, EFileShareReadersOnly);
	if (err != KErrNone)
		return;

	HBufC8* hBuf = HBufC8::New(KInputBufferMaxSize);
	if (!hBuf)
		{
		file.Close();
		return;
		}

	TInt filePos = 0
	TInt bufFilePos = 0;
	TInt nFrames = 0;
	mad_timer_t totalTime = KMadTimerZero;
	TPtr8 buf = hBuf->Des();
	for ( ; ; )
		{
		if (!stream.buffer || (stream.error == MAD_ERROR_BUFLEN))
			{
			// bufFilePos is a dummy parameter as we don't actually need it here
			err = MadReadData(stream, file, filePos, bufFilePos, (TUint8 *) buf.Ptr());
			if (err == KErrEof)
				break;

			if (err != KErrNone)
				{
				delete hBuf;
				file.Close();

				return;
				}
			}

		if (mad_header_decode(&header, &stream))
			{
			if (MAD_RECOVERABLE(stream.error))
				{
				err = MadHandleError(stream, filePos, &file);
				if (err != KErrNone)
					{
					delete hBuf;
					file.Close();

					return;
					}

				continue;
				}
			else if (stream.error == MAD_ERROR_BUFLEN)
				continue;
			else
				{
				delete hBuf;
				file.Close();

				return;
				}
			}

		mad_timer_add(&iTotalTime, header.duration);
		nFrames++;
		}

	mad_header_finish(&header);
	mad_stream_finish(&stream);

	delete hBuf;
	file.Close(); */
	}

TInt CMadDecoder::FileSize()
	{
	return iFileSize;
	}

void CMadDecoder::ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber)
	{
	aArtist.SetLength(0);
	aTitle.SetLength(0);
	aAlbum.SetLength(0);
	aGenre.SetLength(0);
	aTrackNumber.SetLength(0);

	if (iId3tag || iFiletag)
		{
		Id3GetString(aTitle, ID3_FRAME_TITLE);
		Id3GetString(aArtist, ID3_FRAME_ARTIST);
		Id3GetString(aAlbum, ID3_FRAME_ALBUM);
		Id3GetString(aTrackNumber, ID3_FRAME_TRACK);
		Id3GetString(aGenre, ID3_FRAME_GENRE);
		}
	}

void CMadDecoder::Id3GetString(TDes& aString, char const *id)
	{
	id3_tag* tagtouse;
	if (iId3tag)
		tagtouse = iId3tag; // ID3V2
	else if (iFiletag)
		tagtouse = id3_file_tag(iFiletag); // ID3V1
	else return; 

	const id3_frame *frame = id3_tag_findframe(tagtouse, id, 0);
	if (frame) 
		{
		union id3_field const *field = id3_frame_field(frame, 1);
		const id3_ucs4_t *ucs4 = id3_field_getstrings(field, 0);
		if (strcmp(id, ID3_FRAME_GENRE) == 0)
			ucs4 = id3_genre_name(ucs4);

		if (ucs4)
			{
			id3_utf8_t* utf8 = id3_ucs4_utf8duplicate(ucs4);

			TPtrC8 p((const unsigned char *) utf8);
			CnvUtfConverter::ConvertToUnicodeFromUtf8(aString, p);
			User::Free(utf8);
			}
		}
	}

// Read and parse an ID3 tag from the stream
id3_tag* CMadDecoder::GetId3(id3_length_t tagsize)
	{
	const id3_byte_t *data;
	id3_byte_t *allocated = NULL;
	id3_length_t count = iStream.bufend - iStream.this_frame;

	if (tagsize <= count)
		{
		data = iStream.this_frame;
		mad_stream_skip(&iStream, tagsize);
		}
	else
		{
		allocated = (id3_byte_t*) User::Alloc(tagsize);
		if (!allocated)
			return NULL;

		Mem::Copy(allocated, iStream.this_frame, count);
		mad_stream_skip(&iStream, count);

		TInt extraBytesToRead = tagsize - count;
		TPtr8 bufPtr(allocated + count, extraBytesToRead);
		TInt err = iFile.Read(bufPtr);
		if ((err != KErrNone) || (bufPtr.Length() != extraBytesToRead))
			{
			User::Free(allocated);
			return NULL;
			}

		iFilePos += extraBytesToRead;
		data = allocated;
		}

	id3_tag* id3tag = id3_tag_parse(data, tagsize);
	User::Free(allocated);

	return id3tag;
	}


TInt CMadDecoder::MadHandleError(mad_stream& aStream, TInt& aFilePos, RFile* aFile)
	{
	if (aStream.error != MAD_ERROR_LOSTSYNC)
		{
		// Ignore all other errors
		return KErrNone;
		}

	// See if we've come across a tag
	long tagsize = id3_tag_query(aStream.this_frame, aStream.bufend - aStream.this_frame);
	if (tagsize>0)
		{
		if (!aFile)
			{
			// Called by the main decoder, so decode the tags
			iId3tag = GetId3(tagsize);
			if (iId3tag)
				ProcessId3();

			// Adjust file size
			iFileSize -= tagsize;
			if (iFileSize<0)
				iFileSize = 0;
			}
		else
			{
			// Called by TimeTotal(), so skip the tags
			long count = aStream.bufend - aStream.this_frame;
			if (tagsize<=count)
				mad_stream_skip(&aStream, tagsize);
			else
				{
				mad_stream_skip(&aStream, count);
				TInt seekLength = tagsize-count;
				TInt err = aFile->Seek(ESeekCurrent, seekLength);
				if (err != KErrNone)
					return err;

				aFilePos += seekLength;
				}
			}
		}

	return KErrNone;
	}

// Display and process ID3 tag information
void CMadDecoder::ProcessId3()
	{
	id3_frame const *frame;
    
	// length information
	frame = id3_tag_findframe(iId3tag, "TLEN", 0);
	if (frame)
		{
		union id3_field const *field;
		unsigned int nstrings;
    
		field = id3_frame_field(frame, 1);
		nstrings = id3_field_getnstrings(field);
    
		if (nstrings > 0)
			{
			id3_latin1_t *latin1 = id3_ucs4_latin1duplicate(id3_field_getstrings(field, 0));
			if (latin1)
				{        
				// "The 'Length' frame contains the length of the audio file
				// in milliseconds, represented as a numeric string."
				TPtrC8 latin1Ptr((TUint8 *) latin1);
				TLex8 latin1Lex(latin1Ptr);
				
				signed long ms = 0;
				latin1Lex.Val(ms);
				if (ms > 0)
					mad_timer_set(&iTotalTime, 0, ms, 1000);

				User::Free(latin1);
				}
			}
		}
  	}

void CMadDecoder::GetFrequencyBins(TInt32* /*aBins*/, TInt /*NumberOfBins*/)
	{
	}

TBool CMadDecoder::RequestingFrequencyBins()
	{
	return EFalse;
	}

void CMadDecoder::FileThreadTransfer()
	{
	}

#endif // MP3_SUPPORT
