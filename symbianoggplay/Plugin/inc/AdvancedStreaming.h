/*
 *  Copyright (c) 2004 L. H. Wilden.
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

#ifndef _Streaming_h
#define _Streaming_h

#include "mdaaudiooutputstream.h"
#include "mda/common/audio.h"
#include "OggRateConvert.h"
#include "OggHelperFcts.h"
#include <OggOs.h> // For SERIES60 definition

#ifdef SERIES60
const TInt KBuffers= 2;
const TInt KBufferSize = 4096;
#else
const TInt KBuffers= 12;
const TInt KBufferSize = 4096*10;
#endif

const TInt KMaxVolume = 100;


class MAdvancedStreamingObserver
{
public:
   virtual TInt GetNewSamples(TDes8 &aBuffer) = 0; 
   virtual void NotifyPlayInterrupted(TInt aError) = 0;
};


class CAdvancedStreaming : public CBase,
                           public MMdaAudioOutputStreamCallback, 
                           public MOggSampleRateFillBuffer
{
  
 public:
  
  CAdvancedStreaming(MAdvancedStreamingObserver &anObserver);
  ~CAdvancedStreaming();
  TInt   Open(TInt theChannels, TInt theRate);
  void   ConstructL();
  void   Pause();
  void   Play();
  void   Stop();

  TInt GetNewSamples(TDes8 &aBuffer);
  void SetVolumeGain(TGainType aGain);
  void SetVolume(TInt aVol);
  TInt Volume();
  TInt MaxVolume();

  enum TState {
    EIdle,
    EStopping,
    EStreaming,
    EDestroying
  };

 private:

  // these are abstract methods in MMdaAudioOutputStreamCallback:
  virtual void MaoscPlayComplete(TInt aError);
  virtual void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
  virtual void MaoscOpenComplete(TInt aError);

  void SendBuffer(TDes8& buf);
  TInt SetAudioCaps(TInt theChannels, TInt theRate);
 // void SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, TBool aConvertChannel, TInt aNbOfChannels);
  void NotifyPlayInterrupted(TInt aError);

  MAdvancedStreamingObserver &iObserver;
  COggSampleRateConverter *iOggSampleRateConverter;
  // communication with symbian's media streaming framework:
  //--------------------------------------------------------

  CMdaAudioOutputStream*   iStream;
  TMdaAudioDataSettings    iSettings;
  RPointerArray<TDes8>     iBuffer;
  TDes8*                   iSent[KBuffers];
  TInt                     iSentIdx;

  TInt                     iAudioCaps;
  TInt                     iMaxVolume;
  TState                   iState;

   // There is something wrong with Nokia Audio Streaming (NGage)
   // First buffers are somehow swallowed.
   // To avoid that, wait a short time before streaming, so that AppView
   // draw have been done. Also send few empty (zeroed) buffers .

  TInt                     iFirstBuffers;  
  static TInt StartAudioStreamingCallBack(TAny* aPtr);
  COggTimer *              iStartAudioStreamingTimer;
  static TInt StopAudioStreamingCallBack(TAny* aPtr);
  COggTimer *              iStopAudioStreamingTimer;
  TBool                    iStoppedFromEof;
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
