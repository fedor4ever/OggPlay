/*
 *  Copyright (c) 2004 
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

#ifdef __VC32__
#pragma warning( disable : 4244 ) // conversion from __int64 to unsigned int: Possible loss of data
#endif

#include "AdvancedStreaming.h"
#include <e32svr.h>
#include "Ogglog.h"



#if defined(UIQ)
const TInt KAudioPriority = EMdaPriorityMax;
#else
const TInt KAudioPriority = 70; // S60 audio players typically uses 60-75.
#endif


////////////////////////////////////////////////////////////////
//
// CAdvancedStreaming
//
////////////////////////////////////////////////////////////////

CAdvancedStreaming::CAdvancedStreaming(MAdvancedStreamingObserver &anObserver) 
:iObserver(anObserver)
{
    iSettings.Query();
    
    // These basic rates should be supported by all Symbian OS
    // Higher quality might not be supported.
    iSettings.iChannels  = TMdaAudioDataSettings::EChannelsMono;
    iSettings.iSampleRate= TMdaAudioDataSettings::ESampleRate8000Hz;
    iSettings.iVolume = 0;
};

void CAdvancedStreaming::ConstructL() {
  iMaxVolume = 1; // This will be updated when stream is opened.
  iStream  = CMdaAudioOutputStream::NewL(*this);
  iStream->Open(&iSettings);

  COggAudioCapabilityPoll pollingAudio;
  iAudioCaps = pollingAudio.PollL();
  
  TDes8* buffer;

  // Reserve Buffers
  for (TInt i=0; i<KBuffers; i++) {
    buffer = new(ELeave) TBuf8<KBufferSize>;
    buffer->SetMax();
    CleanupStack::PushL(buffer);
    User::LeaveIfError(iBuffer.Append(buffer));
    CleanupStack::Pop(buffer);
  }

  iStartAudioStreamingTimer = new (ELeave) COggTimer(
      TCallBack( StartAudioStreamingCallBack,this )   );
  
  iStopAudioStreamingTimer = new (ELeave) COggTimer(
      TCallBack( StopAudioStreamingCallBack,this )   );

  iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;
}

CAdvancedStreaming::~CAdvancedStreaming() {
  iState = EDestroying;
  delete  iStartAudioStreamingTimer;
  delete  iStopAudioStreamingTimer;
  iStream->Stop();
  delete iStream;
  iBuffer.ResetAndDestroy();
  delete iOggSampleRateConverter;
}

TInt CAdvancedStreaming::Open(TInt theChannels, TInt theRate)
{
  if (iState != EIdle)
    return (KErrNotReady);

  TInt err =  SetAudioCaps( theChannels,theRate );
  return err;
} 


TInt CAdvancedStreaming::SetAudioCaps(TInt theChannels, TInt theRate)
{
  TMdaAudioDataSettings::TAudioCaps ac;
  TMdaAudioDataSettings::TAudioCaps rt;
  TBool ConvertChannel = EFalse;
  TBool ConvertRate = EFalse;

  TInt usedChannels = theChannels;
  
  if (!(iAudioCaps & TMdaAudioDataSettings::EChannelsStereo) && (theChannels==2) )
  {
      // Only Mono is supported by this phone, we need to mix the 2 channels together
      ConvertChannel = ETrue;
      usedChannels = 1; // Use 1 channel
  }

  if (usedChannels==1) ac= TMdaAudioDataSettings::EChannelsMono;
  else if (usedChannels==2) ac= TMdaAudioDataSettings::EChannelsStereo;
  else {
    //OGGLOG.Write(_L("Illegal number of channels"));
    return -100;
  }
 
  TInt usedRate = theRate;

  if (theRate==8000) rt= TMdaAudioDataSettings::ESampleRate8000Hz;
  else if (theRate==11025) rt= TMdaAudioDataSettings::ESampleRate11025Hz;
  else if (theRate==16000) rt= TMdaAudioDataSettings::ESampleRate16000Hz;
  else if (theRate==22050) rt= TMdaAudioDataSettings::ESampleRate22050Hz;
  else if (theRate==32000) rt= TMdaAudioDataSettings::ESampleRate32000Hz;
  else if (theRate==44100) rt= TMdaAudioDataSettings::ESampleRate44100Hz;
  else if (theRate==48000) rt= TMdaAudioDataSettings::ESampleRate48000Hz;
  else {
      // Rate not supported by the phone
      ConvertRate = ETrue;
      
      usedRate = 16000; // That rate should be 
                        // supported by all phones
      rt = TMdaAudioDataSettings::ESampleRate16000Hz;
  }

  if ( !(rt & iAudioCaps) )
   {
    // Rate not supported by this phone.
      ConvertRate = ETrue;
      usedRate = 16000; //That rate should be 
                        //supported by all phones
      rt = TMdaAudioDataSettings::ESampleRate16000Hz;
   }
  
#if 0
  if (ConvertChannel || ConvertRate)
  {
      SamplingRateSupportedMessage(ConvertRate, theRate, ConvertChannel, theChannels);
  }
#endif

  iOggSampleRateConverter->Init(this,KBufferSize,(TInt) (0.75*KBufferSize), theRate,usedRate, theChannels,usedChannels);
  TRAPD( error, iStream->SetAudioPropertiesL(rt, ac) );
  return error;
}

#if 0
void CAdvancedStreaming::SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, 
                                                TBool aConvertChannel, TInt /*aNbOfChannels*/)
{
    // Print an error/info msg displaying all supported rates by this HW.

    const TInt ratesMask[] = {TMdaAudioDataSettings::ESampleRate8000Hz,
    TMdaAudioDataSettings::ESampleRate11025Hz,
    TMdaAudioDataSettings::ESampleRate16000Hz,
    TMdaAudioDataSettings::ESampleRate22050Hz,
    TMdaAudioDataSettings::ESampleRate32000Hz,
    TMdaAudioDataSettings::ESampleRate44100Hz, 
    TMdaAudioDataSettings::ESampleRate48000Hz };
    const TInt rates[] = {8000,11025,16000,22050,32000,44100,48000};

    HBufC * HbufMsg = HBufC::NewL(1000);
    CleanupStack::PushL(HbufMsg);
    HBufC * HtmpBuf  = HBufC::NewL(500);
    CleanupStack::PushL(HtmpBuf);

    TPtr BufMsg = HbufMsg->Des();
    TPtr TmpBuf = HtmpBuf->Des();

    if (aConvertRate)
    {
        TBuf<128> buf;
        CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_24);
    TBool first = ETrue;
    for (TInt i=0; i<7; i++)
    {
        if (iAudioCaps & ratesMask[i])
        {
            if (!first)
            { 
                // Append a comma
                    buf.Append(_L(", "));
            }
            // Append the audio rate
                buf.AppendNum(rates[i]);
            first = EFalse;
        }
    }
        BufMsg.Format(TmpBuf, aRate, &buf);
    }
    if (aConvertChannel)
    {
        CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_25);
        BufMsg.Append(TmpBuf);
    }
    
    CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_26);
    BufMsg.Append(TmpBuf);

    iEnv->OggWarningMsgL(BufMsg);
    CleanupStack::PopAndDestroy(2);
}
#endif

TInt CAdvancedStreaming::Volume()
{
  if (!iStream) return KMaxVolume;
  TInt vol=KMaxVolume;
  TRAPD( err, vol=iStream->Volume() );
  return vol;
}


TInt CAdvancedStreaming::MaxVolume()
{
  return iMaxVolume;;
}

void CAdvancedStreaming::SetVolume(TInt aVol)
{
  if (!iStream) return;
  if (aVol>KMaxVolume) aVol= KMaxVolume;
  else if(aVol<0) aVol= 0;

  TInt volume = (TInt) (((TReal) aVol)/KMaxVolume * iMaxVolume);
  if (volume > iMaxVolume)
      volume = iMaxVolume;
  TRAPD( err, iStream->SetVolume(volume); )
}

void CAdvancedStreaming::Play() 
{
  if (iState == EStreaming)
        return; // Do nothing if already streaming

  iState = EStreaming;
  // There is something wrong how Nokia audio streaming handles the
                   // first buffers. They are somehow swallowed.
                   // To avoid that, send few (4) almost empty buffers
  iFirstBuffers = 4; 
#ifdef DELAY_AUDIO_STREAMING_START
  //Also to avoid the first buffer problem, wait a short time before streaming, 
  // so that Application drawing have been done. Processor should then
  // be fully available for doing audio thingies.
  iStartAudioStreamingTimer->Wait(0.1E6);
#else
  for (TInt i=0; i<KBuffers; i++) SendBuffer(*iBuffer[i]);
#endif
}

void CAdvancedStreaming::Pause()
{
  if (iState == EStreaming) return;
  if (iState == EStopping) return;
  // For now on, pausing is exactly the same as stop.
  // Could be improved in the future.
  TRAPD( err, iStream->Stop() );
}

void CAdvancedStreaming::Stop()
{
  if (iState == EIdle ) return;
  if (iState == EStopping) return;
  iState= EStopping;
  TRAPD( err, iStream->Stop() );
}


TInt CAdvancedStreaming::GetNewSamples(TDes8 &aBuffer)
{
    TInt len = aBuffer.Length();
    TInt ret = iObserver.GetNewSamples(aBuffer); 
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);
    }
    return (ret);
}


void CAdvancedStreaming::SendBuffer(TDes8& buf)
{
    if (iState!=EStreaming) return;
    long ret=0;
    buf.SetLength(KBufferSize);

    TInt cnt = 0;
    if (iFirstBuffers)  {
        buf.FillZ(4000);
        cnt = 4000;
        ret = 4000;
        iFirstBuffers--;
    } else {
        ret = iOggSampleRateConverter->FillBuffer( buf );
    }
    
    if (ret < 0) {
        // Error in the stream. Bad luck, we will continue anyway.
    } else if (ret == 0) {
        // End of the file
        iState = EStopping;
    } else {
        TRAPD( err, iStream->WriteL(buf) );
        if (err!=KErrNone) {
            User::Leave(err);
        }
        iSentIdx++;
        if (iSentIdx>=KBuffers) iSentIdx=0;
        iSent[iSentIdx]= &buf;
    }
}



void CAdvancedStreaming::NotifyPlayInterrupted(TInt aError)
{
    iState=EIdle;
    iObserver.NotifyPlayInterrupted(aError);
}

void CAdvancedStreaming::MaoscPlayComplete(TInt aError)
{
  // Error codes:
  // KErrCancel      -3
  // KErrUnderflow  -10
  // KErrDied       -13  (interrupted by higher priority)
  // KErrInUse      -14

  if ( (iState != EStopping) && (iState != EDestroying) ) 
  {
      NotifyPlayInterrupted(aError);
      return;
  }
  if ( aError == KErrUnderflow ) return;
  if ( aError == KErrInUse ) aError= KErrNone;
  if ( aError == KErrCancel ) return;
  if ( aError != KErrNone) {
    NotifyPlayInterrupted(aError);
  }
}

void CAdvancedStreaming::MaoscBufferCopied(TInt aError, const TDesC8& aBuffer)
{
  // Error codes:
  // KErrCancel      -3  (not sure when this happens, but ignore for now)
  // KErrUnderflow  -10  (ignore this, do as if nothing has happend, and live happily ever after)
  // KErrDied       -13  (interrupted by higher priority)
  // KErrInUse      -14  (the sound device was stolen from us)
  // KErrAbort      -39  (stream was stopped before this buffer was copied)

  if (aError == KErrCancel) aError= KErrNone;

  if (aError == KErrAbort ||
      aError == KErrInUse ||
      aError == KErrDied  ||
      aError == KErrCancel) return;

  if (iState == EIdle) return;

  TInt b;
  for (b=0; b<KBuffers; b++) if (&aBuffer == iBuffer[b]) break;

  if ( (iState == EStopping) && (iSent[iSentIdx] == &aBuffer) )
  {
      // All the buffers have been sent, stop the playback
      // We cannot rely on a MaoscPlayComplete from the CMdaAudioOutputStream
      // since not all phone supports that.
      // Thus, we wait for 0.1 sec, that everything is over.
      iStopAudioStreamingTimer->Wait(0.1E6);
  }
  if (aError == KErrUnderflow) aError= KErrNone;

  if (aError == KErrNone) SendBuffer(*iBuffer[b]); 
  else {
    NotifyPlayInterrupted(aError);
  }
}

void CAdvancedStreaming::MaoscOpenComplete(TInt aError) 
{ 
  iMaxVolume=iStream->MaxVolume();

  iStream->SetVolume(iMaxVolume);
 
  if (aError != KErrNone) {
      // TODO : Not currently handled
  } else {
    iStream->SetPriority(KAudioPriority, EMdaPriorityPreferenceTimeAndQuality);
  }
  
}


TInt CAdvancedStreaming::StartAudioStreamingCallBack(TAny* aPtr)
{
  CAdvancedStreaming* self= (CAdvancedStreaming*) aPtr;
  
  for (TInt i=0; i<KBuffers; i++) 
      self->SendBuffer(*(self->iBuffer[i]));
  return 0;
}


TInt CAdvancedStreaming::StopAudioStreamingCallBack(TAny* aPtr)
{
  CAdvancedStreaming* self= (CAdvancedStreaming*) aPtr;
  
  self->iStream->Stop();

  self->NotifyPlayInterrupted(KErrNone);
  self->iState = EIdle;
  
  return 0;
}

void CAdvancedStreaming::SetVolumeGain(TGainType aGain)
{
    iOggSampleRateConverter->SetVolumeGain(aGain);
}


TInt COggAudioCapabilityPoll::PollL()
    {
    iCaps=0;
    TInt oneSupportedRate = 0;
    for (TInt i=0; i<7; i++)
        {
        switch(i)
            {
            case 0:
                iRate=TMdaAudioDataSettings::ESampleRate8000Hz;
                break;
            case 1:
                iRate= TMdaAudioDataSettings::ESampleRate11025Hz;
                break;
            case 2:
                iRate= TMdaAudioDataSettings::ESampleRate16000Hz;
                break;
            case 3:
                iRate= TMdaAudioDataSettings::ESampleRate22050Hz;
                break;
            case 4:
                iRate= TMdaAudioDataSettings::ESampleRate32000Hz;
                break;
            case 5:
                iRate= TMdaAudioDataSettings::ESampleRate44100Hz;
                break;
            case 6:
                iRate= TMdaAudioDataSettings::ESampleRate48000Hz;
                break;
            }
        iSettings.Query();
        
        iSettings.iChannels  = TMdaAudioDataSettings::EChannelsMono;
        iSettings.iSampleRate= iRate;
        iSettings.iVolume = 0;
        
        iStream  = CMdaAudioOutputStream::NewL(*this);
        iStream->Open(&iSettings);
        CActiveScheduler::Start();

        if (iCaps & iRate){
            // We need to make sure, Nokia 6600 for example won't tell in the
            // Open() that the requested rate is not supported.
            TRAPD(err,iStream->SetAudioPropertiesL(iRate,iSettings.iChannels););
            TRACEF(COggLog::VA(_L("SampleRate Supported:%d"), err ));
            if (err == KErrNone)
            {
                // Rate is *really* supported. Remember it.
                oneSupportedRate = iRate;
            }
            else
            {
                // Rate is not supported.
                iCaps &= ~iRate;
            }
        }

        delete iStream; // This is rude...
        iStream = NULL;
        
        }
    
    // Poll for stereo support
    iSettings.iChannels  = TMdaAudioDataSettings::EChannelsStereo;
    iSettings.iSampleRate= oneSupportedRate;
    iSettings.iVolume = 0;
    
    iStream  = CMdaAudioOutputStream::NewL(*this);
    iStream->Open(&iSettings);
    iRate = TMdaAudioDataSettings::EChannelsStereo;
    CActiveScheduler::Start();
    
    if (iCaps & iRate){
       // We need to make sure, Nokia 6600 for example won't tell in the
       // Open() that the requested rate is not supported.
       TRAPD(err,iStream->SetAudioPropertiesL(iSettings.iSampleRate,iSettings.iChannels););
       TRACEF(COggLog::VA(_L("Stereo Supported:%d"), err ));
       if (err != KErrNone)
       {
          // Rate is not supported.
          iCaps &= ~iRate;
       }
    }
    delete iStream; // This is rude...
    iStream = NULL;
    return iCaps;
    }


void COggAudioCapabilityPoll::MaoscOpenComplete(TInt aError) 
{
    TRACEF(COggLog::VA(_L("SampleRa:%d"), aError ));

    if (aError==KErrNone) 
        {
        // Mode supported.
        iCaps |= iRate;
        }
    CActiveScheduler::Stop();
    
}

void COggAudioCapabilityPoll::MaoscPlayComplete(TInt /*aError*/)
    {
    }

void COggAudioCapabilityPoll::MaoscBufferCopied(TInt /*aError*/, const TDesC8& /*aBuffer*/)
    {
    }
