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

#include "e32des8.h"
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
#include "TremorDecoder.h"
#include "OggAbsPlayback.h"

#ifdef SERIES60
const TInt KBuffers= 2;
const TInt KBufferSize = 4096;
const TInt KAudioPriority = 70; // S60 audio players typically uses 60-75.
#else
const TInt KBuffers= 12;
const TInt KBufferSize = 4096*10;
const TInt KAudioPriority = EMdaPriorityMax;
#endif

const TInt KStreamStartDelay = 100000;
const TInt KSendoStreamStartDelay = 275000;
const TInt KStreamRestartDelay = 100000;
const TInt KStreamStopDelay = 100000;

#if defined(MOTOROLA)
#include "OggTremor_Motorola.h"
#else

#include "mdaaudiooutputstream.h"
#include "mda/common/audio.h"

// COggPlayback:
// An implementation of CAbsPlayback for the Ogg/Vorbis format:
//-------------------------------------------------------------

class COggPlayback : public MMdaAudioOutputStreamCallback, 
             public MOggSampleRateFillBuffer,
		     public CAbsPlayback
{
  
 public:
  
  COggPlayback(COggMsgEnv* anEnv, MPlaybackObserver* anObserver = NULL);
  virtual ~COggPlayback();
  void ConstructL();

  virtual TInt   Info(const TDesC& aFileName, TBool silent= EFalse);
  virtual TInt   Open(const TDesC& aFileName);

  virtual void   Pause();
  virtual void   Play(); 
  virtual void   Resume();
  virtual void   Stop();

  virtual void   SetVolume(TInt aVol);
  virtual void   SetPosition(TInt64 aPos);

  virtual TInt64 Position();
  virtual TInt64 Time();
  virtual TInt   Volume();

#ifdef MDCT_FREQ_ANALYSER
  virtual const TInt32 * GetFrequencyBins();
#else
  virtual const void* GetDataChunk();
#endif

  virtual TInt GetNewSamples(TDes8 &aBuffer);
  virtual void SetVolumeGain(TGainType aGain);

 private:

  // these are abstract methods in MMdaAudioOutputStreamCallback:
  virtual void MaoscPlayComplete(TInt aError);
  virtual void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
  virtual void MaoscOpenComplete(TInt aError);

  void SendBuffer(HBufC8& buf);
  void ParseComments(char** ptr);
  TInt SetAudioCaps(TInt theChannels, TInt theRate);
  void GetString(TBuf<256>& aBuf, const char* aStr);
  void SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, TBool aConvertChannel, TInt aNbOfChannels);
  MDecoder* GetDecoderL(const TDesC& aFileName);

  COggMsgEnv*               iEnv;

  COggSampleRateConverter *iOggSampleRateConverter;
  // communication with symbian's media streaming framework:
  //--------------------------------------------------------

  CMdaAudioOutputStream*   iStream;
  TMdaAudioDataSettings    iSettings;
  HBufC8*                  iBuffer[KBuffers];
  HBufC8*                  iSent[KBuffers];
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
  static TInt StopAudioStreamingCallBack(TAny* aPtr);
  COggTimer *              iStopAudioStreamingTimer;

  static TInt RestartAudioStreamingCallBack(TAny* aPtr);
  COggTimer *              iRestartAudioStreamingTimer;
  TBool iUnderflowing;
  TInt iFirstUnderflowBuffer;
  TInt iLastUnderflowBuffer;

#ifdef MDCT_FREQ_ANALYSER
  TReal  iLatestPlayTime;
  TReal  iTimeBetweenTwoSamples;

class TFreqBins 
{
public:
    TTimeIntervalMicroSeconds iTime;
    TInt32 iFreqCoefs[KNumberOfFreqBins];
};

  TFixedArray<TFreqBins,KFreqArrayLength> iFreqArray;
  TInt iLastFreqArrayIdx;
  TInt iTimeWithoutFreqCalculation;
  TInt iTimeWithoutFreqCalculationLim;
#endif

  // Communication with the decoder
  // ------------------------------
  RFile*                   iFile;
  TBool                     iEof; // true after ov_read has encounted the eof

  MDecoder*             iDecoder;

  // Machine uid (for identifying the phone model)
  TInt iMachineUid;
  RFs iFs;
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
        TMdaAudioDataSettings  iSettings;
        TInt iCaps;
        TInt iRate;
    };
    
#endif // !MOTOROLA
#endif // _OggTremor_h
