/*
 *  Copyright (c) 2004 P.Wolff
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

/* 
    Based on madlld.c, 
      (c) 2001--2004 Bertrand Petit											
     
*/
 
#include <mad.h>
#include <limits.h>
#include <string.h>
#include "MadDecoder.h"
#include "OggLog.h"

#define MadErrorString(x) mad_stream_errorstr(x)

/****************************************************************************
 * Converts a sample from libmad's fixed point number format to a signed	*
 * short (16 bits).															*
 ****************************************************************************/
signed short CMadDecoder::MadFixedToSshort(mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The signed short value is formed, after clipping, by the least
	 * significant whole part bit, followed by the 15 most significant
	 * fractional part bits. Warning: this is a quick and dirty way to
	 * compute the 16-bit number, madplay includes much better
	 * algorithms.
	 */

	/* Clipping */
	if(Fixed>=MAD_F_ONE)
		return(SHRT_MAX);
	if(Fixed<=-MAD_F_ONE)
		return(-SHRT_MAX);

	/* Conversion. */
	Fixed=Fixed>>(MAD_F_FRACBITS-15);
	return((signed short)Fixed);
}


// helper function putting synthesized pcm samples into a buffer.
int CMadDecoder::mad_outputpacket(unsigned char* ptr, const unsigned char* endptr) {

  //TRACEF(_L("CMadDecoder::outputpacket"));
  int counter=0;
  if(ptr+4>=endptr) return 0;
  if(iRememberPcmSamples==iSynth.pcm.length) iRememberPcmSamples=0;
	for(int i=iRememberPcmSamples;i<iSynth.pcm.length;i++,counter++)
		{
			signed short	Sample;

			/* Left channel */
			Sample=MadFixedToSshort(iSynth.pcm.samples[0][i]);
			*(ptr++)=Sample&0xff;
			*(ptr++)=Sample>>8;

			/* Right channel. If the decoded stream is monophonic then
			 * the right output channel is the same as the left one.
			 */
			if(MAD_NCHANNELS(&iFrame.header)==2)
				Sample=MadFixedToSshort(iSynth.pcm.samples[1][i]);
			*(ptr++)=Sample&0xff;
			*(ptr++)=Sample>>8;
      if(ptr+4>endptr) { 
        break;
      }
		}
  iRememberPcmSamples=i;
  if(iRememberPcmSamples==iSynth.pcm.length) iRememberPcmSamples=0;
  //TRACEF(_L("~CMadDecoder::outputpacket"));
  return counter*4;
  
}


int CMadDecoder::Clear() {

  //TRACEF(_L("CMadDecoder::Clear"));
  iGuardPtr=NULL;
	iStatus=0;
	iBstdFile=0;
  iRememberPcmSamples=0;
	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&iStream);
	mad_frame_init(&iFrame);
	mad_synth_init(&iSynth);
	mad_timer_reset(&iTimer);
  //TRACEF(_L("~CMadDecoder::Clear"));
  return 0;

}

int CMadDecoder::Open(FILE* f) {
    //TRACEF(_L("CMadDecoder::Open"));

  Clear();
	iBstdFile=NewBstdFile(f);
	if(iBstdFile==NULL)
	{
		return(1);
	}

  // dummy read to fill header values
  mad_read(NULL,0);

  iInputFp=f;
  return 0;
}

int CMadDecoder::OpenInfo(FILE* f) {
  return Open(f);
}

int CMadDecoder::Close(FILE*) {
    //TRACEF(_L("CMadDecoder::Close"));

  /* The input file was completely read; the memory allocated by our
	 * reading module must be reclaimed.
	 */
	BstdFileDestroy(iBstdFile);

	/* Mad is no longer used, the structures that were initialized must
     * now be cleared.
	 */
	mad_synth_finish(&iSynth);
	mad_frame_finish(&iFrame);
	mad_stream_finish(&iStream);

  //TRACEF(_L("~CMadDecoder::Clear"));

  return 0;
}


int CMadDecoder::Read(TDes8& aBuffer,int Pos) {
  //TRACEF(_L("CMadDecoder::Read"));
  unsigned char* outputbuffer=(unsigned char *) &(aBuffer.Ptr()[Pos]);
  int length=aBuffer.MaxLength()-Pos;
  return mad_read(outputbuffer,length);
}

/*
mad_read, similar to ov_read from Tremor:
tries to put length bytes of packed PCM data into buffer

return value:
 <0) error/hole in data 
  0) EOF
  n) number of bytes of PCM actually returned.  
  
We try to fill the buffer completely though.

Passing outputbuffer=0 will read the header values.
*/ 
int CMadDecoder::mad_read(unsigned char* outputbuffer,int length)
{
  const unsigned char	*iOutputBufferEnd=outputbuffer+length;
  unsigned char	*iOutputPtr=outputbuffer;
  int BytesWritten=0;
	
  if(iRememberPcmSamples) {
    int CurrentBytesWritten=mad_outputpacket(iOutputPtr,iOutputBufferEnd);
    BytesWritten+=CurrentBytesWritten;
    iOutputPtr+=CurrentBytesWritten;
  }
  // it still doesn't fit - so return and try next time
  if(iRememberPcmSamples) return BytesWritten;

  while(1){	
      //TRACEF(_L("CMadDecoder::Read while"));

    /* The input bucket must be filled if it becomes empty or if
		 * it's the first execution of the loop.
		 */
    if(iStream.buffer==NULL || iStream.error==MAD_ERROR_BUFLEN)
		{
			size_t			ReadSize,
							Remaining;
			unsigned char	*ReadStart;

			/* {2} libmad may not consume all bytes of the input
			 * buffer. If the last frame in the buffer is not wholly
			 * contained by it, then that frame's start is pointed by
			 * the next_frame member of the Stream structure. This
			 * common situation occurs when mad_frame_decode() fails,
			 * sets the stream error code to MAD_ERROR_BUFLEN, and
			 * sets the next_frame pointer to a non NULL value. (See
			 * also the comment marked {4} bellow.)
			 *
			 * When this occurs, the remaining unused bytes must be
			 * put back at the beginning of the buffer and taken in
			 * account before refilling the buffer. This means that
			 * the input buffer must be large enough to hold a whole
			 * frame at the highest observable bit-rate (currently 448
			 * kb/s). XXX=XXX Is 2016 bytes the size of the largest
			 * frame? (448000*(1152/32000))/8
			 */
			if(iStream.next_frame!=NULL)
			{
				Remaining=iStream.bufend-iStream.next_frame;
				memmove(InputBuffer,iStream.next_frame,Remaining);
				ReadStart=InputBuffer+Remaining;
				ReadSize=INPUT_BUFFER_SIZE-Remaining;
			}
      else {
				ReadSize=INPUT_BUFFER_SIZE,
				ReadStart=InputBuffer,
				Remaining=0;
      }

			/* Fill-in the buffer. If an error occurs print a message
			 * and leave the decoding loop. If the end of stream is
			 * reached we also leave the loop but the return status is
			 * left untouched.
			 */
			ReadSize=BstdRead(ReadStart,1,ReadSize,iBstdFile);
			if(ReadSize<=0)
			{
				if(ferror(iInputFp))
				{
					//fprintf(stderr,"read error on bit-stream \n");
					return -1;
				}
				if(feof(iInputFp))
					//fprintf(stderr,"end of input stream\n");
  				return BytesWritten;
			}

			/* {3} When decoding the last frame of a file, it must be
			 * followed by MAD_BUFFER_GUARD zero bytes if one wants to
			 * decode that last frame. When the end of file is
			 * detected we append that quantity of bytes at the end of
			 * the available data. Note that the buffer can't overflow
			 * as the guard size was allocated but not used the the
			 * buffer management code. (See also the comment marked
			 * {1}.)
			 *
			 * In a message to the mad-dev mailing list on May 29th,
			 * 2001, Rob Leslie explains the guard zone as follows:
			 *
			 *    "The reason for MAD_BUFFER_GUARD has to do with the
			 *    way decoding is performed. In Layer III, Huffman
			 *    decoding may inadvertently read a few bytes beyond
			 *    the end of the buffer in the case of certain invalid
			 *    input. This is not detected until after the fact. To
			 *    prevent this from causing problems, and also to
			 *    ensure the next frame's main_data_begin pointer is
			 *    always accessible, MAD requires MAD_BUFFER_GUARD
			 *    (currently 8) bytes to be present in the buffer past
			 *    the end of the current frame in order to decode the
			 *    frame."
			 */
			if(BstdFileEofP(iBstdFile))
			{
				iGuardPtr=ReadStart+ReadSize;
				memset(iGuardPtr,0,MAD_BUFFER_GUARD);
				ReadSize+=MAD_BUFFER_GUARD;
			}

			/* Pipe the new buffer content to libmad's stream decoder
             * facility.
			 */
			mad_stream_buffer(&iStream,InputBuffer,ReadSize+Remaining);
			iStream.error=MAD_ERROR_NONE;
		}

		/* Decode the next MPEG frame. The streams is read from the
		 * buffer, its constituents are break down and stored the the
		 * Frame structure, ready for examination/alteration or PCM
		 * synthesis. Decoding options are carried in the Frame
		 * structure from the Stream structure.
		 *
		 * Error handling: mad_frame_decode() returns a non zero value
		 * when an error occurs. The error condition can be checked in
		 * the error member of the Stream structure. A mad error is
		 * recoverable or fatal, the error status is checked with the
		 * MAD_RECOVERABLE macro.
		 *
		 * {4} When a fatal error is encountered all decoding
		 * activities shall be stopped, except when a MAD_ERROR_BUFLEN
		 * is signaled. This condition means that the
		 * mad_frame_decode() function needs more input to complete
		 * its work. One shall refill the buffer and repeat the
		 * mad_frame_decode() call. Some bytes may be left unused at
		 * the end of the buffer if those bytes forms an incomplete
		 * frame. Before refilling, the remaining bytes must be moved
		 * to the beginning of the buffer and used for input for the
		 * next mad_frame_decode() invocation. (See the comments
		 * marked {2} earlier for more details.)
		 *
		 * Recoverable errors are caused by malformed bit-streams, in
		 * this case one can call again mad_frame_decode() in order to
		 * skip the faulty part and re-sync to the next frame.
		 */
		if(mad_frame_decode(&iFrame,&iStream))
		{
			if(MAD_RECOVERABLE(iStream.error))
			{
				/* Do not print a message if the error is a loss of
				 * synchronization and this loss is due to the end of
				 * stream guard bytes. (See the comments marked {3}
				 * supra for more informations about guard bytes.)
				 */
				if(iStream.error!=MAD_ERROR_LOSTSYNC ||
				   iStream.this_frame!=iGuardPtr)
				{
          // ahm, do ... nothing
//					fprintf(stderr,"recoverable frame level error (%s)\n",
//							MadErrorString(&iStream));
//					fflush(stderr);
				}
				continue;
			}
			else
				if(iStream.error==MAD_ERROR_BUFLEN)
					continue;
				else
				{
					return -3;
				}
		}

		/* The characteristics of the stream's first frame is printed
		 * on stderr. The first frame is representative of the entire
		 * stream.
		 */
		if(iFrameCount==0)
//			if(PrintFrameInfo(stderr,&iFrame.header))
//			{
//				iStatus=1;
//				return -2;
//			}

		/* Accounting. The computed frame duration is in the frame
		 * header structure. It is expressed as a fixed point number
		 * whole data type is mad_timer_t. It is different from the
		 * samples fixed point format and unlike it, it can't directly
		 * be added or subtracted. The timer module provides several
		 * functions to operate on such numbers. Be careful there, as
		 * some functions of libmad's timer module receive some of
		 * their mad_timer_t arguments by value!
		 */
		iFrameCount++;
		mad_timer_add(&iTimer,iFrame.header.duration);

		/* Once decoded the frame is synthesized to PCM samples. No errors
		 * are reported by mad_synth_frame();
		 */
		mad_synth_frame(&iSynth,&iFrame);

		/* Synthesized samples must be converted from libmad's fixed
		 * point number to the consumer format. Here we use unsigned
		 * 16 bit little endian integers on two channels. Integer samples
		 * are temporarily stored in a buffer that is flushed when
		 * full.
		 */
    if(outputbuffer) { 
      int CurrentBytesWritten=mad_outputpacket(iOutputPtr,iOutputBufferEnd);
      BytesWritten+=CurrentBytesWritten;
      iOutputPtr+=CurrentBytesWritten;
      // it's full already
      if(!CurrentBytesWritten||iRememberPcmSamples) return BytesWritten;
    }
    else {
          // remember the remaining samples:
          iRememberPcmSamples=iSynth.pcm.length;
          return BytesWritten;
	  }

  }

}

TInt CMadDecoder::Channels()
{
  if(iFrame.header.mode==MAD_MODE_SINGLE_CHANNEL) {
      //TRACEF(_L("CMadDecoder::Channels: 1 channel"));

    return 1;
  }
      //TRACEF(_L("CMadDecoder::Channels: 2 channels"));
  return 2;
}

TInt CMadDecoder::Rate()
{
  //TRACEF(COggLog::VA(_L("CMadDecoder::Rate:%d"), iFrame.header.samplerate ));
  return iFrame.header.samplerate;
}

TInt CMadDecoder::Bitrate()
{ 
  //TRACEF(COggLog::VA(_L("CMadDecoder::Bitrate:%d"), iFrame.header.bitrate ));
  return iFrame.header.bitrate;
}

TInt64 CMadDecoder::Position()
{ 
  TInt64 pos(mad_timer_count(iTimer,MAD_UNITS_MILLISECONDS));
  return pos;
}

void CMadDecoder::Setposition(TInt64 aPosition)
{ 
}

//FIXMAD: TimeTotal not yet implemented
TInt64 CMadDecoder::TimeTotal()
{
  TInt64 timetotal;
  return timetotal;

}

//FIXMAD: ID3 Tag parsing not yet implemented - libmad comes with a library for this (libid3tag)  
void CMadDecoder::ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber) {
  //TRACEF(_L("CMadDecoder::ParseTags"));
  
}
