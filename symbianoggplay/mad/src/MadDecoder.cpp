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

#pragma warning( disable : 4127 ) // while(1)

#include "MadDecoder.h"
#include "OggLog.h"
#include "mad.h"
#include "id3tag.h"
#include "gettext.h"
#include "tag.h"
#include <utf.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>

#define MadErrorString(x) mad_stream_errorstr(x)
# define N_(text)	(text)
# define _(text)	(text)

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
  int i;
  if(iRememberPcmSamples==iSynth.pcm.length) iRememberPcmSamples=0;
	for(i=iRememberPcmSamples;i<iSynth.pcm.length;i++,counter++)
		{
			signed short	Sample;
      // FIXMAD: eventually fix this warning
#pragma warning( disable : 4244 ) // conversion from int to uchar: possible loss of data

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

          
// seek to iGlobalTimer;
// IMPROVE_MAD: There are several issues here:
// o first, the buffers aren't properly cleared, which results in a delay 
//   where the old buffers are played
// o second, searching itself is just an estimate.
// To do it right, we would have to keep an array with frames and times.
// (see mpg321-0.2.10/mad.c line 222 for inspiration)
// o third, Rob Leslie says that you should call mad_synth_frame() once 
// and discard the resulting PCM.
// o Oh, and we don't update the frame counter. Should we?

void CMadDecoder::jumpseek(void) {

  int new_position;
  int gbt=mad_timer_count (iGlobalTimer, MAD_UNITS_MILLISECONDS);
  int tt=TimeTotal().Low();
  new_position = (int)((double)gbt / 
       (double)tt * iFilesize);
  new_position =new_position - (INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD);
  if(new_position<0) new_position=0;
  if (fseek (iInputFp, new_position, SEEK_SET) == -1) {
    // FIXMAD: well...
  }
  mad_frame_mute (&iFrame);
  mad_synth_mute (&iSynth);
  iStream.error = MAD_ERROR_BUFLEN;
  iStream.next_frame = NULL;
  iStream.sync = 0;

  iTimer=iGlobalTimer;
  iPlaystate=EPLAY;

}
// try to figure out the total time.
void CMadDecoder::getframeinfo(void) {

  unsigned int lbitrate;
  lbitrate = iFrame.header.bitrate / 1000;
  vbr_rate += lbitrate;
  vbr_frames++;
  vbr += (bitrate && bitrate != lbitrate) ? 128 : -1;
  if (vbr < 0)
    vbr = 0;
  else if (vbr > 512)
    vbr = 512;

  bitrate = lbitrate;

  // lucky - timer was in the ID3v2 or in the XING tag
  if(mad_timer_compare(iTotalTime,mad_timer_zero)!=0) return; 

  unsigned long rate;

  /* estimate based on size and bitrate */

  rate = vbr ? vbr_rate * 125 / vbr_frames : bitrate * 125UL;

  mad_timer_set(&iEstTotalTime, 0, iFilesize, rate);


//          if ((tag->flags & TAG_XING) &&
//	  (tag->xing.flags & TAG_XING_FRAMES)) {
//	player->stats.total_time = frame->header.duration;
//	mad_timer_multiply(&player->stats.total_time, tag->xing.frames);
//      }
//
//      /* total stream byte size adjustment */
//
//      frame_size = stream->next_frame - stream->this_frame;
//
//      if (player->stats.total_bytes == 0) {
//	if ((tag->flags & TAG_XING) && (tag->xing.flags & TAG_XING_BYTES) &&
//	    tag->xing.bytes > frame_size)
//	  player->stats.total_bytes = tag->xing.bytes - frame_size;
//      }
//      else if (player->stats.total_bytes >=
//	       stream->next_frame - stream->this_frame)
//	player->stats.total_bytes -= frame_size;

    

}
int CMadDecoder::Clear() {
  iGuardPtr=NULL;
  iStatus=0;
  iBstdFile=0;
  iRememberPcmSamples=0;
  iTotalTime=mad_timer_zero;
  if(filetag) {
    id3_file_close(filetag);
    filetag=NULL;
  }
  if(id3tag) { 
    id3_tag_delete(id3tag); 
    id3tag=NULL; 
  }

  tag_init(&xltag); //FIXFIXMAD: clear/destruct this !!!!

  /* First the structures used by libmad must be initialized. */
  mad_stream_init(&iStream);
  mad_frame_init(&iFrame);
  mad_synth_init(&iSynth);
  mad_timer_reset(&iTimer);
  mad_timer_reset(&iGlobalTimer);
  return 0;

}

int CMadDecoder::Open(FILE* f,const TDesC& aFilename) {
    //TRACEF(_L("CMadDecoder::Open"));

//  Clear();
  iBstdFile=NewBstdFile(f);
  if(iBstdFile==NULL)
  {
    return(1);
  }

  int fd   = wopen((wchar_t*)aFilename.Ptr(), O_RDONLY | O_BINARY);   
  struct stat buf;
  if (fd && (fstat (fd, &buf) != -1)) {
    iFilesize = buf.st_size;
  }

  // dummy read to fill header values
  mad_read(NULL,0);
  iInputFp=f;

  if(id3tag) return 0; // there was an ID3v2 tag at the beginning. Good.
  // else we're checking for an ID3v1 tag - slower.
  if(fd) filetag = id3_file_fdopen(fd, ID3_FILE_MODE_READONLY);
  return 0;
}

int CMadDecoder::OpenInfo(FILE* f,const TDesC& aFilename) {
  return Open(f,aFilename);
}

int CMadDecoder::Close(FILE*) {
    //TRACEF(_L("CMadDecoder::Close"));

  /* The input file was completely read; the memory allocated by our
	 * reading module must be reclaimed.
	 */
	BstdFileDestroy(iBstdFile);

  if(filetag) {
    id3_file_close(filetag);
    filetag=NULL;
  }

  if(id3tag) { 
    id3_tag_delete(id3tag); 
    id3tag=NULL; 
  }
	/* Mad is no longer used, the structures that were initialized must
     * now be cleared.
	 */
	mad_synth_finish(&iSynth);
	mad_frame_finish(&iFrame);
	mad_stream_finish(&iStream);

  //TRACEF(_L("~CMadDecoder::Clear"));

  return 0;
}


int CMadDecoder::Read(TDes8& aBuffer,int Pos) 
{
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
  const unsigned char *iOutputBufferEnd=outputbuffer+length;
  unsigned char *iOutputPtr=outputbuffer;
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
      size_t ReadSize;
      size_t Remaining;
      unsigned char *ReadStart;


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
        
        MadHandleError();
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

    getframeinfo();
//    if(iFrameCount==0) {
//        getframeinfo();
//        gettaginfo();
//    }

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

    if(iPlaystate==EREVERSE) {
      jumpseek();
      continue;
    }

    if(iPlaystate==EFASTFORWARD) {
      /* This should be the right way to do it.
         Unfortunately, it's too slow on flash-based low power devices
         like ours.

      if(mad_timer_compare(iGlobalTimer,iTimer)>0) {
        continue;
      } else {
        iPlaystate=EPLAY;
      }
      */
      jumpseek();
      continue;

    }
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
  signed long timer=mad_timer_count(iTimer,MAD_UNITS_MILLISECONDS);
  TInt64 pos;
  pos=(TInt)timer;
  return pos;
}

void CMadDecoder::Setposition(TInt64 aPosition)
{
    unsigned long ms;
    if(aPosition<0) aPosition=0;
    ms=(unsigned long) aPosition.Low();
    mad_timer_set(&iGlobalTimer,0,ms,1000);
    if(mad_timer_compare(iGlobalTimer,iTimer)>0) {
        iPlaystate=EFASTFORWARD;
    } else {
        iPlaystate=EREVERSE;
    }
    
}

TInt64 CMadDecoder::TimeTotal()
{
  signed long timer;
  if(mad_timer_compare(iTotalTime,mad_timer_zero)!=0)
    timer=mad_timer_count(iTotalTime,MAD_UNITS_MILLISECONDS);
  else if(mad_timer_compare(iEstTotalTime,mad_timer_zero)!=0)
    timer=mad_timer_count(iEstTotalTime,MAD_UNITS_MILLISECONDS);
  else
    timer=0;
  TInt64 pos;
  pos=(TInt)timer;
  return pos;
  
}

TInt CMadDecoder::FileSize()
{
  return iFilesize;
}

void CMadDecoder::ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber) {
  //TRACEF(_L("CMadDecoder::ParseTags"));
  aArtist.SetLength(0);
  aTitle.SetLength(0);
  aAlbum.SetLength(0);
  aGenre.SetLength(0);
  aTrackNumber.SetLength(0);

  if(id3tag || filetag) {
    Id3GetString(aTitle,ID3_FRAME_TITLE);
    Id3GetString(aArtist,ID3_FRAME_ARTIST);
    Id3GetString(aAlbum,ID3_FRAME_ALBUM);
    Id3GetString(aTrackNumber,ID3_FRAME_TRACK);
    Id3GetString(aGenre,ID3_FRAME_GENRE);
  } else {
    //FIXMAD: breakpoint for testing
    int i=5;
    i++;
  }
}

void CMadDecoder::Id3GetString(TDes& aString,char const *id)
{
  struct id3_frame const *frame;
  id3_ucs4_t const *ucs4;
  id3_utf8_t* utf8;
  union id3_field const *field;
  struct id3_tag* tagtouse;

  if(id3tag) { tagtouse=id3tag; } // ID3V2 tag
  else if(filetag) { tagtouse=id3_file_tag(filetag); } //ID3V1
  else return; 

  frame = id3_tag_findframe(tagtouse,id,0);
  if (frame) 
  {
    field = id3_frame_field(frame, 1);
    ucs4 = id3_field_getstrings(field, 0);
    if (strcmp(id, ID3_FRAME_GENRE) == 0) ucs4 = id3_genre_name(ucs4);
    if(ucs4)
    {
      utf8=id3_ucs4_utf8duplicate(ucs4);
      TPtrC8 p((const unsigned char *)utf8);  
      CnvUtfConverter::ConvertToUnicodeFromUtf8(aString,p);
      free(utf8);
      utf8=NULL;
    }
  }
}

void CMadDecoder::GetFrequencyBins(TInt32* /*aBins*/,TInt /*NumberOfBins*/)
{
}


/*
 * NAME:	get_id3()
 * DESCRIPTION:	read and parse an ID3 tag from a stream
 */
struct id3_tag* CMadDecoder::get_id3(struct mad_stream *stream, id3_length_t tagsize)
{
//  struct id3_tag *tag = 0;
  id3_length_t count;
  id3_byte_t const *data;
  id3_byte_t *allocated = 0;

  count = stream->bufend - stream->this_frame;

  if (tagsize <= count) {
    data = stream->this_frame;
    mad_stream_skip(stream, tagsize);
  }
  else {
    allocated = (id3_byte_t*)malloc(tagsize);
    if (allocated == 0) {
      //error("id3", _("not enough memory to allocate tag data buffer"));
      goto fail;
    }

    memcpy(allocated, stream->this_frame, count);
    mad_stream_skip(stream, count);

    while (count < tagsize) {
      int len;
      do
        len = BstdRead(allocated + count, 1,tagsize - count,iBstdFile  );
      while (len == -1 );

      if (len == -1) {
      //	error("id3", ":read");
	      goto fail;
      }

      if (len == 0) {
      //	error("id3", _("EOF while reading tag data"));
	      goto fail;
      }

      count += len;
    }

    data = allocated;
  }

  id3tag = id3_tag_parse(data, tagsize); //FXIMAD: leak?

 fail:
  if (allocated) {
    free(allocated);
    allocated=NULL;
  }

  return id3tag;
}


void CMadDecoder::MadHandleError() 
{
  signed long tagsize;

	/* Do nothing if the error is a loss of
	 * synchronization and this loss is due to the end of
	 * stream guard bytes. (See the comments marked {3}
	 * supra for more informations about guard bytes.)
	 */
	if(iStream.error!=MAD_ERROR_LOSTSYNC ||
		 iStream.this_frame!=iGuardPtr)
	{
    // recoverable frame level error:
    // ahm, do ... nothing
	}

  if(iStream.error== MAD_ERROR_LOSTSYNC) {
    tagsize = id3_tag_query(iStream.this_frame,iStream.bufend - iStream.this_frame);
    if (tagsize > 0) {
        
        id3tag = get_id3(&iStream, tagsize);
        if (id3tag) {
            process_id3(id3tag);
        }
     }
  }

}

/*
* NAME:	process_id3()
* DESCRIPTION:	display and process ID3 tag information
*/

void CMadDecoder::process_id3(struct id3_tag const *tag)
{
  struct id3_frame const *frame;
    
  /* length information */
  
  frame = id3_tag_findframe(tag, "TLEN", 0);
  if (frame) {
    union id3_field const *field;
    unsigned int nstrings;
    
    field    = id3_frame_field(frame, 1);
    nstrings = id3_field_getnstrings(field);
    
    if (nstrings > 0) {
      id3_latin1_t *latin1;
      
      latin1 = id3_ucs4_latin1duplicate(id3_field_getstrings(field, 0));
      if (latin1) {
        signed long ms;
        
        /*
        * "The 'Length' frame contains the length of the audio file
        * in milliseconds, represented as a numeric string."
        */
        
        ms = atoi((const char*)latin1);
        if (ms > 0)
          mad_timer_set(&iTotalTime, 0, ms, 1000);
        
        free(latin1);
        latin1=NULL;
      }
    }
  }
  
  /* relative volume adjustment information */
//IMPROVE_MAD: RGAIN_SUPPORT 
//  if ((player->options & PLAYER_OPTION_SHOWTAGSONLY) ||
//    !(player->options & PLAYER_OPTION_IGNOREVOLADJ)) {
  if(0) {
    frame = id3_tag_findframe(tag, "RVA2", 0);
    if (frame) {
      id3_latin1_t const *id;
      id3_byte_t const *data;
      id3_length_t length;
      
      enum {
        CHANNEL_OTHER         = 0x00,
          CHANNEL_MASTER_VOLUME = 0x01,
          CHANNEL_FRONT_RIGHT   = 0x02,
          CHANNEL_FRONT_LEFT    = 0x03,
          CHANNEL_BACK_RIGHT    = 0x04,
          CHANNEL_BACK_LEFT     = 0x05,
          CHANNEL_FRONT_CENTRE  = 0x06,
          CHANNEL_BACK_CENTRE   = 0x07,
          CHANNEL_SUBWOOFER     = 0x08
      };
      
      id   = id3_field_getlatin1(id3_frame_field(frame, 0));
      data = id3_field_getbinarydata(id3_frame_field(frame, 1), &length);
      
//      assert(id && data);
      
      /*
      * "The 'identification' string is used to identify the situation
      * and/or device where this adjustment should apply. The following is
      * then repeated for every channel
      *
      *   Type of channel         $xx
      *   Volume adjustment       $xx xx
      *   Bits representing peak  $xx
      *   Peak volume             $xx (xx ...)"
      */
      
      while (length >= 4) {
        unsigned int peak_bytes;
        
        peak_bytes = (data[3] + 7) / 8;
        if (4 + peak_bytes > length)
          break;
        
        if (data[0] == CHANNEL_MASTER_VOLUME) {
          signed int voladj_fixed;
          double voladj_float;
          
          /*
          * "The volume adjustment is encoded as a fixed point decibel
          * value, 16 bit signed integer representing (adjustment*512),
          * giving +/- 64 dB with a precision of 0.001953125 dB."
          */
          
          voladj_fixed  = (data[1] << 8) | (data[2] << 0);
          voladj_fixed |= -(voladj_fixed & 0x8000);
          
          voladj_float  = (double) voladj_fixed / 512;
          
 //         set_gain(player, GAIN_VOLADJ, voladj_float);
          
//          if (player->verbosity >= 0) {
//            detail(_("Relative Volume"),
//              _("%+.1f dB adjustment (%s)"), voladj_float, id);
//          }
          
          break;
        }
        
        data   += 4 + peak_bytes;
        length -= 4 + peak_bytes;
      }
    }
  }

//IMPROVE_MAD: RGAIN_SUPPORT 
  
  /* Replay Gain */
  
//  if ((player->options & PLAYER_OPTION_SHOWTAGSONLY) ||
//    ((player->output.replay_gain & PLAYER_RGAIN_ENABLED) &&
//    !(player->output.replay_gain & PLAYER_RGAIN_SET))) {
//    frame = id3_tag_findframe(tag, "RGAD", 0);
//    if (frame) {
//      id3_byte_t const *data;
//      id3_length_t length;
//      
//      data = id3_field_getbinarydata(id3_frame_field(frame, 0), &length);
//      assert(data);
//      
//      /*
//      * Peak Amplitude                          $xx $xx $xx $xx
//      * Radio Replay Gain Adjustment            $xx $xx
//      * Audiophile Replay Gain Adjustment       $xx $xx
//      */
//      
//      if (length >= 8) {
//        struct mad_bitptr ptr;
//        mad_fixed_t peak;
//        struct rgain rgain[2];
//        
//        mad_bit_init(&ptr, data);
//        
//        peak = mad_bit_read(&ptr, 32) << 5;
//        
//        rgain_parse(&rgain[0], &ptr);
//        rgain_parse(&rgain[1], &ptr);
//        
//        use_rgain(player, rgain);
//        
//        mad_bit_finish(&ptr);
//      }
//    }
//  }
}

void CMadDecoder::gettaginfo(void) {
        
  if (xltag.flags == 0 &&	tag_parse(&xltag, &iStream) == 0) {
    unsigned int frame_size;

//IMPROVE_MAD: RGAIN_SUPPORT 
//	if ((xltag.flags & xltag_LAME) &&
//	    (player->output.replay_gain & PLAYER_RGAIN_ENABLED) &&
//	    !(player->output.replay_gain & PLAYER_RGAIN_SET))
//	  use_rgain(player, xltag.lame.replay_gain);
//      }

    if ((xltag.flags & TAG_XING) &&
        (xltag.xing.flags & TAG_XING_FRAMES)) {
      iTotalTime = iFrame.header.duration;
      mad_timer_multiply(&iTotalTime, xltag.xing.frames);
    }

    /* total stream byte size adjustment */

    frame_size = iStream.next_frame - iStream.this_frame;

    if (iFilesize == 0) {
      if ((xltag.flags & TAG_XING) && (xltag.xing.flags & TAG_XING_BYTES) &&
        xltag.xing.bytes > frame_size)
        iFilesize = xltag.xing.bytes - frame_size;
    }
    else if (iFilesize >= iStream.next_frame - iStream.this_frame)
	      iFilesize -= frame_size;
  }

}

