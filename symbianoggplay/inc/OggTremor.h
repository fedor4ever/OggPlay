/*
 *  Copyright (c) 2003 L. H. Wilden.
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

#ifndef _OggTremor_h
#define _OggTremor_h

#include "mdaaudiooutputstream.h"
#include "e32des8.h"
#include "mda/common/audio.h"
#include "eikenv.h"
#include "stdio.h"
#include "ivorbisfile.h"
#include "OggMsgEnv.h"
#include "OggHelperFcts.h"
#include "OggRateConvert.h"
// V0.2:  3 @ 4096   
// V0.3:  ?
// V0.4:  6 @ 4096*10  (not sure?!)
// V0.5:  3 @ 4096*40
// V0.6:  6 @ 4096*10
// V0.9:  12 @ 4096*10


#ifdef SERIES60
const TInt KBuffers= 2;
const TInt KBufferSize = 4096;
#else
const TInt KBuffers= 12;
const TInt KBufferSize = 4096*10;
#endif

const TInt KMaxVolume = 100;
const TInt KStepVolume = 10;

const TInt KErrOggFileNotFound = -101;


class MPlaybackObserver {
 public:
  virtual void NotifyPlayComplete() = 0;
  virtual void NotifyUpdate() = 0;
  virtual void NotifyPlayInterrupted() = 0;
};


// CAbsPlayback:
// An abstract base class defining the interface for streaming audio data,
// reading the meta data (tags) and file properties etc.
//------------------------------------------------------

class CAbsPlayback : public CBase {

 public:

  enum TState {
    EClosed,
    EFirstOpen,
    EOpen,
    EPlaying,
    EPaused
  };

  CAbsPlayback(MPlaybackObserver* anObserver=NULL);

  // Here is a bunch of abstract methods which need to implemented for
  // each audio format:

  // Open a file and retrieve information (meta data, file size etc.) without playing it:
  virtual TInt   Info(const TDesC& aFileName, TBool silent= EFalse) = 0;

  // Open a file and prepare playback:
  virtual TInt   Open(const TDesC& aFileName) = 0;

  virtual void   Pause() = 0;
  virtual void   Play() = 0;
  virtual void   Stop() = 0;

  virtual void   SetVolume(TInt aVol) = 0;
  virtual void   SetPosition(TInt64 aPos) = 0;

  virtual TInt64 Position() = 0;
  virtual TInt64 Time() = 0;
  virtual TInt   Volume() = 0;
  
#ifdef MDCT_FREQ_ANALYSER
  virtual const TInt32 * GetFrequencyBins(TTime aTime) = 0;
#else
  virtual const void* GetDataChunk() = 0;
#endif


  // The following functions have a simple default implementation:

  virtual TState State();
  virtual TInt   Rate();
  virtual TInt   Channels();
  virtual TInt   FileSize();
  virtual TInt   BitRate();
  virtual const TFileName& FileName();
  virtual const TDesC& Artist();
  virtual const TDesC& Title();
  virtual const TDesC& Album();
  virtual const TDesC& Genre();
  virtual const TDesC& TrackNumber();

  virtual void  ClearComments();
  virtual void  SetObserver(MPlaybackObserver* anObserver);

 protected:

  TState                   iState;
  MPlaybackObserver*       iObserver;

  // file properties and tag information:
  //-------------------------------------

  TBuf<256>                iTitle;
  TBuf<256>                iAlbum;
  TBuf<256>                iArtist;
  TBuf<256>                iGenre;
  TBuf<256>                iTrackNumber;
  TInt64                   iTime;
  TInt                     iRate;
  TInt                     iChannels;
  TInt                     iFileSize;
  TInt64                   iBitRate;
  TFileName                iFileName;
  
};


// COggPlayback:
// An implementation of TAbsPlayback for the Ogg/Vorbis format:
//-------------------------------------------------------------

class COggPlayback : public MMdaAudioOutputStreamCallback, 
             public MOggSampleRateFillBuffer,
		     public CAbsPlayback
{
  
 public:
  
  COggPlayback(COggMsgEnv* anEnv, MPlaybackObserver* anObserver=0);
  virtual ~COggPlayback();
  void ConstructL();

  virtual TInt   Info(const TDesC& aFileName, TBool silent= EFalse);
  virtual TInt   Open(const TDesC& aFileName);

  virtual void   Pause();
  virtual void   Play();
  virtual void   Stop();

  virtual void   SetVolume(TInt aVol);
  virtual void   SetPosition(TInt64 aPos);

  virtual TInt64 Position();
  virtual TInt64 Time();
  virtual TInt   Volume();
#ifdef MDCT_FREQ_ANALYSER
  virtual const TInt32 * GetFrequencyBins(TTime aTime);
#else
  virtual const void* GetDataChunk();
#endif
  virtual TInt GetNewSamples(TDes8 &aBuffer);

 private:

  // these are abstract methods in MMdaAudioOutputStreamCallback:
  virtual void MaoscPlayComplete(TInt aError);
  virtual void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
  virtual void MaoscOpenComplete(TInt aError);

  void SendBuffer(TDes8& buf);
  void ParseComments(char** ptr);
  TInt SetAudioCaps(TInt theChannels, TInt theRate);
  void GetString(TBuf<256>& aBuf, const char* aStr);
  void SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, TBool aConvertChannel, TInt aNbOfChannels);

  COggMsgEnv*               iEnv;

  COggSampleRateConverter *iOggSampleRateConverter;
  // communication with symbian's media streaming framework:
  //--------------------------------------------------------

  CMdaAudioOutputStream*   iStream;
  TMdaAudioDataSettings    iSettings;
  RPointerArray<TDes8>     iBuffer;
  TDes8*                   iSent[KBuffers];
  TInt                     iSentIdx;
  TInt                     iMaxVolume;
  TInt                     iAudioCaps;

   // There is something wrong with Nokia Audio Streaming (NGage)
   // First buffers are somehow swallowed.
   // To avoid that, wait a short time before streaming, so that AppView
   // draw have been done. Also send few empty (zeroed) buffers .

  TInt                     iFirstBuffers;  
  static TInt StartAudioStreamingCallBack(TAny* aPtr);
  COggTimer *              iStartAudioStreamingTimer;


#ifdef MDCT_FREQ_ANALYSER
// BL : Frequency analyser changes
  TInt32 Freq_coefs[17]; 
#endif

  // communication with the tremor/ogg codec:
  //-----------------------------------------

  OggVorbis_File           iVf;
  FILE*                    iFile;
  TInt                     iFileOpen;
  TInt                     iEof;            // true after ov_read has encounted the eof
  TInt                     iBufCount;       // counts buffers sent to the audio stream
  TInt                     iCurrentSection; // used in the call to ov_read

};



class COggAudioCapabilityPoll : public CBase, public MMdaAudioOutputStreamCallback
    {
    public:
        TInt PollL();
    private:
        // these are abstract methods in MMdaAudioOutputStreamCallback:
        void MaoscPlayComplete(TInt aError);
        void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
        void MaoscOpenComplete(TInt aError);
        
    private:
        CMdaAudioOutputStream* iStream;
        TMdaAudioDataSettings    iSettings;
        TInt iCaps;
        TInt iRate;
    };
#endif
