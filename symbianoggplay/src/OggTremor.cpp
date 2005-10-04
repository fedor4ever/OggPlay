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

#ifdef __VC32__
#pragma warning( disable : 4244 ) // conversion from __int64 to unsigned int: Possible loss of data
#endif

#include "OggOs.h"
#include "OggLog.h"
#include "OggTremor.h"
#include "TremorDecoder.h"
#ifdef MP3_SUPPORT
#include "MadDecoder.h"
#endif

#include <barsread.h>
#include <eikbtpan.h>
#include <eikcmbut.h>
#include <eiklabel.h>
#include <eikmover.h>
#include <eikbtgpc.h>
#include <eikon.hrh>
#include <eikon.rsg>
#include <charconv.h>
#include <hal.h>

#include <utf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ivorbiscodec.h"
#include "ivorbisfile.h"

#if !defined(__VC32__)
#include "Utf8Fix.h"
#endif

#include <OggPlay.rsg>

#if defined(MULTI_THREAD_PLAYBACK)
#ifdef __VC32__
#pragma warning( disable : 4355 ) // 'this' used in base member initializer list
#endif
#endif


// COggPlayback
COggPlayback::COggPlayback(COggMsgEnv* anEnv, MPlaybackObserver* anObserver)
#if defined(MULTI_THREAD_PLAYBACK)
:  CAbsPlayback(anObserver), iEnv(anEnv), iSharedData(*this, iUIThread, iBufferingThread)
#else
:  CAbsPlayback(anObserver), iEnv(anEnv)
#endif
{
#if !defined(MULTI_THREAD_PLAYBACK)
    iSettings.Query();
    
    // These basic rates should be supported by all Symbian OS
    // Higher quality might not be supported.
    iSettings.iChannels  = TMdaAudioDataSettings::EChannelsMono;
    iSettings.iSampleRate= TMdaAudioDataSettings::ESampleRate8000Hz;
    iSettings.iVolume = 0;
#endif
}

void COggPlayback::ConstructL()
{
  // Set up the session with the file server
  User::LeaveIfError(iFs.Connect());

#if defined(MULTI_THREAD_PLAYBACK)
  User::LeaveIfError(iFs.Share());
  User::LeaveIfError(AttachToFs());
#endif

  // Discover audio capabilities
  COggAudioCapabilityPoll pollingAudio;
  iAudioCaps = pollingAudio.PollL();

#if defined(MULTI_THREAD_PLAYBACK)
  // Create at least one audio buffer
  iBuffer[0] = HBufC8::NewL(KBufferSize);

  // Open thread handles to the UI thread and the buffering thread
  // Currently the UI and buffering threads are the same (this thread)
  TThreadId uiThreadId = iThread.Id();
  User::LeaveIfError(iUIThread.Open(uiThreadId));
  User::LeaveIfError(iBufferingThread.Open(uiThreadId));

  // Create the streaming thread
  User::LeaveIfError(iStreamingThread.Create(_L("OggPlayStream"), StreamingThread, KDefaultStackSize, NULL, &iSharedData));

  // Create the streaming thread panic handler
  iStreamingThreadPanicHandler = new(ELeave) CThreadPanicHandler(EPriorityHigh, iStreamingThread, *this);

  // Create the streaming thread command handler
  iStreamingThreadCommandHandler = new(ELeave) CStreamingThreadCommandHandler(iUIThread, iStreamingThread, *iStreamingThreadPanicHandler);
  iSharedData.iStreamingThreadCommandHandler = iStreamingThreadCommandHandler;

  // Launch the streaming thread
  User::LeaveIfError(iStreamingThreadCommandHandler->ResumeCommandHandlerThread());

  // Record the fact that the streaming thread started ok
  iStreamingThreadRunning = ETrue;

  // Create the buffering active object
  iBufferingThreadAO = new(ELeave) CBufferingThreadAO(iSharedData);
  iSharedData.iBufferingThreadAO = iBufferingThreadAO;

  // Add it to the buffering threads active scheduler (this thread)
  CActiveScheduler::Add(iBufferingThreadAO);
 
  // Create the streaming thread listener
  iStreamingThreadListener = new(ELeave) CStreamingThreadListener(*this, iSharedData);
  iSharedData.iStreamingThreadListener = iStreamingThreadListener;
#else
  iStream = CMdaAudioOutputStream::NewL(*this);
  iStream->Open(&iSettings);

  for (TInt i=0; i<KBuffers; i++)
	iBuffer[i] = HBufC8::NewL(KBufferSize);
#endif

  iStartAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(StartAudioStreamingCallBack, this));
  iStopAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(StopAudioStreamingCallBack, this));

#if !defined(MULTI_THREAD_PLAYBACK)
  // The restart timer is in the streaming thread for multi thread playback, so only create one if MULTI_THREAD_PLAYBACK is not defined
  // This may change in the future, it might be better to do restarts from the UI thread when MULTI_THREAD_PLAYBACK is enabled
  iRestartAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(RestartAudioStreamingCallBack, this));
#endif

  iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;

  // Read the device uid
  HAL::Get(HALData::EMachineUid, iMachineUid);
}

COggPlayback::~COggPlayback()
{
  __ASSERT_ALWAYS((iState == EClosed) || (iState == EFirstOpen), User::Panic(_L("~COggPlayback"), 0));
	
  delete iDecoder; 
  delete iStartAudioStreamingTimer;
  delete iStopAudioStreamingTimer;
  delete iOggSampleRateConverter;

#if defined(MULTI_THREAD_PLAYBACK)
  if (iBufferingThreadAO)
	iBufferingThreadAO->Cancel();

  if (iStreamingThreadListener)
	iStreamingThreadListener->Cancel();

  if (iStreamingThreadRunning)
	iStreamingThreadCommandHandler->ShutdownCommandHandlerThread();

  if (iStreamingThreadCommandHandler)
	  iStreamingThreadCommandHandler->Cancel();
	
  if (iStreamingThreadPanicHandler)
	  iStreamingThreadPanicHandler->Cancel();

  iThread.Close();
  iUIThread.Close();
  iBufferingThread.Close();
  iStreamingThread.Close();

  delete iBufferingThreadAO;
  delete iStreamingThreadListener;
  delete iStreamingThreadCommandHandler;
  delete iStreamingThreadPanicHandler;

  for (TInt i = 0 ; i<KMultiThreadBuffers ; i++)
  	delete iBuffer[i];
#else
  delete iRestartAudioStreamingTimer;
  delete iStream;

  for (TInt i=0; i<KBuffers; i++)
	delete iBuffer[i];
#endif

  iFs.Close();
}

MDecoder* COggPlayback::GetDecoderL(const TDesC& aFileName)
{
  MDecoder* decoder = NULL;

  TParsePtrC p( aFileName);
  if(p.Ext().Compare( _L(".ogg"))==0 || p.Ext().Compare( _L(".OGG"))==0) {
      decoder=new(ELeave)CTremorDecoder(iFs);
  }
#if defined(MP3_SUPPORT)
  else if(p.Ext().Compare( _L(".mp3"))==0 || p.Ext().Compare( _L(".MP3"))==0) {
    decoder=new(ELeave)CMadDecoder;
  }
#endif
  else {
    _LIT(KPanic,"Panic:");
    _LIT(KNotSupported,"File type not supported");
    iEnv->OggErrorMsgL(KPanic,KNotSupported);
  }

  return decoder;
}

TInt COggPlayback::Open(const TDesC& aFileName)
{
  TRACEF(COggLog::VA(_L("OPEN") ));
  if (iFile)
  {
    iDecoder->Close();
    iFile->Close();
	
	delete iFile;
	iFile = NULL;
  }

  iTime= 0;

  if (aFileName.Length() == 0) {
    //OGGLOG.Write(_L("Oggplay: Filenamelength is 0 (Error20 Error8"));
    iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_8);
    return -100;
  }

  iFile = new(ELeave) RFile;
  if ((iFile->Open(iFs, aFileName, EFileShareReadersOnly)) != KErrNone)
  {
    delete iFile;
    iFile = NULL;

    //OGGLOG.Write(_L("Oggplay: File open returns 0 (Error20 Error14)"));
    TRACE(COggLog::VA(_L("COggPlayback::Open(%S). Failed"), &aFileName ));
    iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_14);
    return KErrOggFileNotFound;
  }

  delete iDecoder;
  iDecoder = NULL;

  iDecoder = GetDecoderL(aFileName);
  if(iDecoder->Open(iFile, aFileName) < 0)
  {
    iDecoder->Close();
    iFile->Close();
	delete iFile;
	iFile = NULL;
    
	//OGGLOG.Write(_L("Oggplay: ov_open not successful (Error20 Error9)"));
    iEnv->OggErrorMsgL(R_OGG_ERROR_20,R_OGG_ERROR_9);
    return -102;
  }

  iDecoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

  iFileName= aFileName;
  iRate= iDecoder->Rate();
  iChannels=iDecoder->Channels();
  iTime=iDecoder->TimeTotal();
  iBitRate=iDecoder->Bitrate();
  iFileSize= iDecoder->FileSize();
  
  TInt err= SetAudioCaps(iDecoder->Channels(), iDecoder->Rate());
  if (err == KErrNone)
	iState= EOpen;
  else
  {
	iDecoder->Close();
	iFile->Close();
  
	delete iFile;
	iFile = NULL;
  }

  return err;
}

void COggPlayback::GetString(TBuf<256>& aBuf, const char* aStr)
{
  // according to ogg vorbis specifications, the text should be UTF8 encoded
  TPtrC8 p((const unsigned char *)aStr);
  CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);

  // in the real world there are all kinds of coding being used!
  // so we try to find out what it is:
#if !defined(__VC32__)
  TInt i= j_code((char*)aStr,strlen(aStr));
  if (i==BIG5_CODE) {
    CCnvCharacterSetConverter* conv= CCnvCharacterSetConverter::NewL();
    CCnvCharacterSetConverter::TAvailability a= conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierBig5, iFs);
    if (a==CCnvCharacterSetConverter::EAvailable) {
      TInt theState= CCnvCharacterSetConverter::KStateDefault;
      conv->ConvertToUnicode(aBuf, p, theState);
    }
    delete conv;
  }
#endif
}

void COggPlayback::ParseComments(char** ptr)
{
  ClearComments();
  while (*ptr) {
    char* s= *ptr;
    TBuf<256> buf;
    GetString(buf,s);
    buf.UpperCase();
    if (buf.Find(_L("ARTIST="))==0) GetString(iArtist, s+7); 
    else if (buf.Find(_L("TITLE="))==0) GetString(iTitle, s+6);
    else if (buf.Find(_L("ALBUM="))==0) GetString(iAlbum, s+6);
    else if (buf.Find(_L("GENRE="))==0) GetString(iGenre, s+6);
    else if (buf.Find(_L("TRACKNUMBER="))==0) GetString(iTrackNumber,s+12);
    ++ptr;
  }
}

TInt COggPlayback::SetAudioCaps(TInt theChannels, TInt theRate)
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
    iEnv->OggErrorMsgL(R_OGG_ERROR_12,R_OGG_ERROR_10);
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
  
  if (ConvertChannel || ConvertRate)
      SamplingRateSupportedMessage(ConvertRate, theRate, ConvertChannel, theChannels);

#if defined(MULTI_THREAD_PLAYBACK)
  iOggSampleRateConverter->Init(this, KBufferSize, (TInt) (0.90*KBufferSize), theRate, usedRate, theChannels, usedChannels);
  TInt err = SetAudioProperties(usedRate, usedChannels);
#else
  iOggSampleRateConverter->Init(this, KBufferSize, (TInt) (0.75*KBufferSize), theRate, usedRate, theChannels, usedChannels);
  TRAPD(err, iStream->SetAudioPropertiesL(rt, ac));
#endif

  if (err)
  {
      TBuf<256> buf,tbuf;
      CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
      CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_20);
      buf.AppendNum(err);
      iEnv->OggErrorMsgL(tbuf,buf);
  }

#ifdef MDCT_FREQ_ANALYSER
  iTimeBetweenTwoSamples = 1.0E6/(theRate*theChannels);
  /* Make a frequency measurement only every .1 seconds, that should be enough */
  iTimeWithoutFreqCalculation = 0;
  iTimeWithoutFreqCalculationLim = (TInt)(0.1E6 / iTimeBetweenTwoSamples);
#endif

  return err;
}


void COggPlayback::SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, 
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

TInt COggPlayback::Volume()
{
#if defined (MULTI_THREAD_PLAYBACK)
  iStreamingThreadCommandHandler->Volume(); 
  return iSharedData.iVolume;
#else
  return iStream->Volume();
#endif
}

void COggPlayback::SetVolume(TInt aVol)
{
#if defined(MULTI_THREAD_PLAYBACK)
  iSharedData.iVolume = aVol;
  iStreamingThreadCommandHandler->SetVolume();
#else
  if (aVol>KMaxVolume) aVol= KMaxVolume;
  else if(aVol<0) aVol= 0;

  TInt volume = (TInt) (((TReal) aVol)/KMaxVolume * iMaxVolume);
  if (volume > iMaxVolume)
      volume = iMaxVolume;

  iStream->SetVolume(volume);
#endif
}

void COggPlayback::SetVolumeGain(TGainType aGain)
{
#if defined(MULTI_THREAD_PLAYBACK)
	TBufferingMode bufferingMode = iSharedData.iBufferingMode;
	TBool bufferFlushRequired = (iState == EPlaying) && (bufferingMode != ENoBuffering);
	if (bufferFlushRequired)
		FlushBuffers(aGain);
	else
		iOggSampleRateConverter->SetVolumeGain(aGain);
#else
  iOggSampleRateConverter->SetVolumeGain(aGain);
#endif
}

void COggPlayback::SetPosition(TInt64 aPos)
{
  if(iDecoder)
  {
	  if (aPos<0)
		  aPos = 0;

#if defined(MULTI_THREAD_PLAYBACK)
	  TBool bufferFlushRequired = (iState == EPlaying);
	  if (bufferFlushRequired)
	  {
		// Stop/Restart the stream instead of flushing buffers
	    // This works better with FF/RW because, like next/prev track, key repeats are possible (and very likely on FF/RW)
		StopStreaming();
		iDecoder->Setposition(aPos);
		iStartAudioStreamingTimer->Wait(KStreamStartDelay);
	  }
	  else
		iDecoder->Setposition(aPos);
#else
	  iDecoder->Setposition(aPos);
#endif
  }
}

TInt64 COggPlayback::Position()
{
#if defined(MULTI_THREAD_PLAYBACK)
  if(iDecoder)
  {
	  TInt bufferMillisecs = (500*iSharedData.iBufferBytes)/(iSharedData.iSampleRate*iSharedData.iChannels);
	  return iDecoder->Position() - TInt64(bufferMillisecs);
  }
#else
  // TO DO: Also take into account the buffers (see comments in GetFrequencyBins)
  if(iDecoder) return iDecoder->Position();
#endif

  return 0;
}

TInt64 COggPlayback::Time()
{
  if (iDecoder && iTime==0) {
    iTime=iDecoder->TimeTotal();
  }
  return iTime;
}

#ifdef MDCT_FREQ_ANALYSER
const TInt32 * COggPlayback::GetFrequencyBins()
{
  // Use Position() to get the position
  // TO DO: Use CMdaAudioOutputStream::Position() instead to get the stream position more accurately (we need to add a member to hold the start position, because the stream is reset after FF/RW or when the audio has to restart)
  // Better still, just change Position() to return the stream position (only if iState == EPlaying, maybe)
#if defined(MULTI_THREAD_PLAYBACK)
  TTimeIntervalMicroSeconds currentPos(TInt64(1000)*Position());
#else
  TTimeIntervalMicroSeconds currentPos = TInt64(1000)*Position();
#endif

  TInt idx = iLastFreqArrayIdx;
  for (TInt i=0; i<KFreqArrayLength; i++)
    {
      if (iFreqArray[idx].iTime < currentPos)
          break;
      
        idx--;
        if (idx < 0)
            idx = KFreqArrayLength-1;
    }
 
  return iFreqArray[idx].iFreqCoefs;
}
#else
const void* COggPlayback::GetDataChunk()
{
  TInt idx= iSentIdx;
  if( iSent[idx] ) 
    return iSent[idx]->Ptr();
  else
    return NULL;
}
#endif

TInt COggPlayback::Info(const TDesC& aFileName, TBool silent)
{
  if (aFileName.Length()==0) return -100;

  RFile* f = new RFile;
  if ((f->Open(iFs, aFileName, EFileShareReadersOnly)) != KErrNone) {
    if (!silent) {
      TBuf<128> buf;
      CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_14);
      iEnv->OggErrorMsgL(buf,aFileName);
    }
    return KErrOggFileNotFound;
  }

  MDecoder* decoder = GetDecoderL(aFileName);
  if(decoder->OpenInfo(f, aFileName) < 0) {
    decoder->Close();
	delete decoder;

	f->Close();
	delete f;

	if (!silent) {
       iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_9);
    }
    return -102;
  }
  decoder->ParseTags(iTitle, iArtist, iAlbum, iGenre, iTrackNumber);

  iFileName = aFileName;
  iRate = decoder->Rate();
  iChannels = decoder->Channels();
  iBitRate = decoder->Bitrate();

  decoder->Close(); 
  delete decoder;

  f->Close();
  delete f;

  return KErrNone;
}

void COggPlayback::Play() 
{
    TRACEF(COggLog::VA(_L("PLAY %i "), iState ));
    switch(iState) {
    case EPaused:
        break;
    case EOpen:  
        iEof=0;
        break;
    default:
    iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_15);
    RDebug::Print(_L("Oggplay: Tremor - State not Open"));
    return;
  }

  iState = EPlaying;
  // There is something wrong how Nokia audio streaming handles the
  // first buffers. They are somehow swallowed.
  // To avoid that, send few (4) almost empty buffers
  if (iMachineUid != EMachineUid_SendoX) // Sendo X doesn't need this fix. 
      iFirstBuffers = 4; 

#ifdef MDCT_FREQ_ANALYSER
  iLastFreqArrayIdx = 0;
  iLatestPlayTime = 0.0;
#endif

#if defined(DELAY_AUDIO_STREAMING_START)
  // Also to avoid the first buffer problem, wait a short time before streaming, 
  // so that Application drawing have been done. Processor should then
  // be fully available for doing audio thingies.

#if !defined(MULTI_THREAD_PLAYBACK)
  // A longer delay is necessary if MULTI_THREAD_PLAYBACK is not defined
  if (iMachineUid == EMachineUid_SendoX) // Latest 1.198.8.2 Sendo X firmware needs a little more time
	iStartAudioStreamingTimer->Wait(KSendoStreamStartDelay);
  else
#endif

	iStartAudioStreamingTimer->Wait(KStreamStartDelay);
#else
  #if defined(MULTI_THREAD_PLAYBACK)
    StartStreaming();
  #else
    for (TInt i=0; i<KBuffers; i++) SendBuffer(*iBuffer[i]);
  #endif
#endif
}

void COggPlayback::Pause()
{
  TRACEF(COggLog::VA(_L("PAUSE")));
  if (iState != EPlaying) return;

  iState = EPaused;
  iStopAudioStreamingTimer->Cancel();

#if defined(MULTI_THREAD_PLAYBACK)
  StopStreaming();
#else
  iStream->Stop();
#endif
}

void COggPlayback::Resume()
{
  //Resume is equivalent of Play() 
  Play();
}

void COggPlayback::Stop()
{
  TRACEF(COggLog::VA(_L("STOP") ));
  if (iState == EClosed)
    return;

  iState = EClosed;
  iStopAudioStreamingTimer->Cancel();

#if defined(MULTI_THREAD_PLAYBACK)
  StopStreaming();
#else
  iStream->Stop();
#endif

  if (iFile)
  {
    iDecoder->Close();
    iFile->Close();

	delete iFile;
	iFile = NULL;

	delete iDecoder;
	iDecoder = NULL;
  }

  ClearComments();
  iTime= 0;
  iEof= EFalse;

#if !defined(MULTI_THREAD_PLAYBACK)
  iUnderflowing = EFalse;
#endif

  if (iObserver) iObserver->NotifyUpdate();
}


TInt COggPlayback::GetNewSamples(TDes8 &aBuffer)
{
    if (iEof)
        return(KErrNotReady);

#ifdef MDCT_FREQ_ANALYSER
    if (iTimeWithoutFreqCalculation > iTimeWithoutFreqCalculationLim )
    {
        iTimeWithoutFreqCalculation = 0;
        iLastFreqArrayIdx++;
        if (iLastFreqArrayIdx >= KFreqArrayLength)
            iLastFreqArrayIdx = 0;
        iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].iFreqCoefs, KNumberOfFreqBins);
        iFreqArray[iLastFreqArrayIdx].iTime =  TInt64(iLatestPlayTime);
    }
    else
    {
        iDecoder->GetFrequencyBins(NULL,0);
    }
#endif
    
    TInt len = aBuffer.Length();
    TInt ret = iDecoder->Read(aBuffer,len); 
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);
#ifdef MDCT_FREQ_ANALYSER
        iLatestPlayTime += ret*iTimeBetweenTwoSamples;
        iTimeWithoutFreqCalculation += ret;
        //TRACEF(COggLog::VA(_L("U:%d %f "), ret, iLatestPlayTime ));
#endif


    }
    if (ret == 0)
    {
        iEof= ETrue;
    }
    return (ret);
}

TInt COggPlayback::StartAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;

#if defined(MULTI_THREAD_PLAYBACK)
  self->StartStreaming();
#else
  for (TInt i=0; i<KBuffers; i++) 
      self->SendBuffer(*(self->iBuffer[i]));
#endif

  return 0;
}

TInt COggPlayback::StopAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr; 
 
  self->Stop();
  self->iObserver->NotifyPlayComplete(); 
  return 0;
}

#if defined(MULTI_THREAD_PLAYBACK)
TInt COggPlayback::AttachToFs()
{
  return iFs.Attach();
}

TInt COggPlayback::SetAudioProperties(TInt aSampleRate, TInt aChannels)
{
  iSharedData.iSampleRate = aSampleRate;
  iSharedData.iChannels = aChannels;
  return iStreamingThreadCommandHandler->SetAudioProperties();
}

void COggPlayback::StartStreaming()
{
  // Start the buffering listener
  if (iSharedData.iBufferingMode == EBufferThread)
	iBufferingThreadAO->StartListening();

  // Start the streaming
  iStreamingThreadCommandHandler->StartStreaming();
}

void COggPlayback::StopStreaming()
{
  // Stop the streaming
  iStreamingThreadCommandHandler->StopStreaming();

  // Cancel the buffering AO
  iBufferingThreadAO->Cancel();
}

TInt COggPlayback::SetBufferingMode(TBufferingMode aNewBufferingMode)
{
  if (aNewBufferingMode == iSharedData.iBufferingMode)
	  return KErrNone;

  if (iState == EPlaying)
  {
	// Flush the buffers (and stop the stream)
	FlushBuffers();
  }

  // Set the new buffering mode
  TBufferingMode oldMode = iSharedData.iBufferingMode;
  iSharedData.iBufferingMode = aNewBufferingMode;
 
  // Set buffering values and allocate or free buffers
  TInt err = BufferingModeChanged();
  if (err != KErrNone)
  {
	  // Reset back to the old mode
	  iSharedData.iBufferingMode = oldMode;

	  // Free any buffers that got allocated
	  BufferingModeChanged();

	  // Restart the stream
	  if (iState == EPlaying)
		iStartAudioStreamingTimer->Wait(KStreamStartDelay);

	  return err;
  }

  // Set the new buffering mode values in the streaming thread
  iStreamingThreadCommandHandler->SetBufferingMode();

  // Restart the stream
  if (iState == EPlaying)
	iStartAudioStreamingTimer->Wait(KStreamStartDelay);

  return KErrNone;
}

void COggPlayback::SetThreadPriority(TStreamingThreadPriority aNewThreadPriority)
{
  // Set the thread priorities (Normal or High)
  iSharedData.iStreamingThreadPriority = aNewThreadPriority;
  iStreamingThreadCommandHandler->SetThreadPriority();
}

void COggPlayback::FlushBuffers()
{
  // The buffering mode has changed, so flush the buffers (and stop the stream)
  iSharedData.iFlushBufferEvent = EBufferingModeChanged;

  iStreamingThreadCommandHandler->PrepareToFlushBuffers();
  iBufferingThreadAO->Cancel();

  iStreamingThreadCommandHandler->FlushBuffers();
}

void COggPlayback::FlushBuffers(TInt64 aNewPosition)
{
  // The position has been changed so flush the buffers (and set the new position)
  iSharedData.iFlushBufferEvent = EPositionChanged;
  iSharedData.iNewPosition = aNewPosition;

  iStreamingThreadCommandHandler->PrepareToFlushBuffers();
  iBufferingThreadAO->Cancel();

  if (iSharedData.iBufferingMode == EBufferThread)
	iBufferingThreadAO->StartListening();

  iStreamingThreadCommandHandler->FlushBuffers();
}

void COggPlayback::FlushBuffers(TGainType aNewGain)
{
  // The volume gain has been changed so flush the buffers (and set the new gain)
  iSharedData.iFlushBufferEvent = EVolumeGainChanged;
  iSharedData.iNewGain = aNewGain;

  iStreamingThreadCommandHandler->PrepareToFlushBuffers();
  iBufferingThreadAO->Cancel();

  if (iSharedData.iBufferingMode == EBufferThread)
	iBufferingThreadAO->StartListening();

  iStreamingThreadCommandHandler->FlushBuffers();
}

TInt64 COggPlayback::DecoderPosition()
{
  // Read the audio position we've decoded to (return value is milliseconds) 
	return iDecoder->Position();
}

void COggPlayback::SetDecoderPosition(TInt64 aPos)
{
  // Set the deocder position (aPos is in milliseconds)
  iDecoder->Setposition(aPos);
}

void COggPlayback::SetSampleRateConverterVolumeGain(TGainType aGain)
{
  // Set the volume gain (called by the streaming thread after the buffers have been flushed)
  iOggSampleRateConverter->SetVolumeGain(aGain);
}

TInt COggPlayback::StreamingThread(TAny* aThreadData)
{
  // Access the shared data / command handler
  TStreamingThreadData& sharedData = *((TStreamingThreadData *) aThreadData);
  CStreamingThreadCommandHandler& streamingThreadCommandHandler = *sharedData.iStreamingThreadCommandHandler;

  // Create a cleanup stack
  CTrapCleanup* cleanupStack = CTrapCleanup::New();
  if (!cleanupStack)
  {
	// Inform the UI thread that starting the streaming thread failed
	streamingThreadCommandHandler.ResumeComplete(KErrNoMemory);
	return KErrNoMemory;
  }

  // Set the thread priority
  RThread streamingThread;
  streamingThread.SetPriority(EPriorityMore);
  streamingThread.Close();

  // Allocate resources and start the active scheduler
  TRAPD(err, StreamingThreadL(sharedData));

  // Cleanup
  COggLog::Exit();  
  delete cleanupStack;

  // Complete the shutdown request
  streamingThreadCommandHandler.ShutdownComplete(err);
  return err;
}

void COggPlayback::StreamingThreadL(TStreamingThreadData& aSharedData)
{
  // Attach to the file session
  User::LeaveIfError(aSharedData.iOggPlayback.AttachToFs());

  // Create a scheduler
  CActiveScheduler* scheduler = new(ELeave) CActiveScheduler;
  CleanupStack::PushL(scheduler);

  // Install the scheduler
  CActiveScheduler::Install(scheduler);

  // Add the streaming command handler to the scheduler
  CStreamingThreadCommandHandler& streamingThreadCommandHandler = *aSharedData.iStreamingThreadCommandHandler;
  CActiveScheduler::Add(&streamingThreadCommandHandler);

  // Create the audio playback engine
  CStreamingThreadPlaybackEngine* playbackEngine = CStreamingThreadPlaybackEngine::NewLC(aSharedData);

  // Listen for commands and dispatch them to the playback engine
  streamingThreadCommandHandler.ListenForCommands(*playbackEngine);

  // Inform the UI thread that the streaming thread has started
  streamingThreadCommandHandler.ResumeComplete(KErrNone);

  // Start the scheduler
  CActiveScheduler::Start();

  // Cancel the command handler
  streamingThreadCommandHandler.Cancel();

  // Uninstall the scheduler
  CActiveScheduler::Install(NULL);

  // Delete the scheduler and audio engine
  CleanupStack::PopAndDestroy(2, scheduler);
}

TBool COggPlayback::PrimeBuffer(HBufC8& buf)
{
  // Read and decode the next buffer
  TPtr8 bufPtr(buf.Des());
  iOggSampleRateConverter->FillBuffer(bufPtr);
  return iEof;
}

void COggPlayback::NotifyOpenComplete(TInt aErr)
{
  // Called by the streaming thread listener when CMdaAudioOutputStream::Open() completes
  if (aErr == KErrNone)
	iState = EFirstOpen;
  else
  {
    TBuf<32> buf;
	CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_19);
	buf.AppendNum(aErr);

	_LIT(tit, "MaoscOpenComplete");
	iEnv->OggErrorMsgL(tit, buf);
  }
}

TInt COggPlayback::BufferingModeChanged()
{
  // The buffering mode has changed so allocate or free buffers
  TInt i;
  TInt allocError = KErrNone;
  switch (iSharedData.iBufferingMode)
  {
	case ENoBuffering:
		iSharedData.iBuffersToUse = KNoBuffers;
		iSharedData.iMaxBuffers = KNoBuffers;

		// Delete all the buffers
		for (i = 1 ; i<KMultiThreadBuffers ; i++)
		{
			delete iBuffer[i];
			iBuffer[i] = NULL;
		}
		break;

	case EBufferStream:
		iSharedData.iBuffersToUse = KSingleThreadBuffersToUse;
		iSharedData.iMaxBuffers = KSingleThreadBuffers;

		if (iBuffer[KNoBuffers])
		{
			// The previous mode was EBufferThread so free some buffers
			for (i = KSingleThreadBuffers ; i<KMultiThreadBuffers ; i++)
			{
				delete iBuffer[i];
				iBuffer[i] = NULL;
			}
		}
		else
		{
			// Allocate the buffers
			for (i = 1 ; i<KSingleThreadBuffers ; i++)
			{
				iBuffer[i] = HBufC8::New(KBufferSize);
				if (!iBuffer[i])
				{
					allocError = KErrNoMemory;
					break;
				}
			}
		}
		break;

	case EBufferThread:
		iSharedData.iBuffersToUse = KMultiThreadBuffersToUse;
		iSharedData.iMaxBuffers = KMultiThreadBuffers;

		if (iBuffer[KNoBuffers])
		{
			// The previous mode was EBufferStream so allocate some more buffers
			for (i = KSingleThreadBuffers ; i<KMultiThreadBuffers ; i++)
			{
				iBuffer[i] = HBufC8::New(KBufferSize);
				if (!iBuffer[i])
				{
					allocError = KErrNoMemory;
					break;
				}
			}
		}
		else
		{
			// Allocate the buffers
			for (i = 1 ; i<KMultiThreadBuffers ; i++)
			{
				iBuffer[i] = HBufC8::New(KBufferSize);
				if (!iBuffer[i])
				{
					allocError = KErrNoMemory;
					break;
				}
			}
		}
		break;
  }

  return allocError;
}

void COggPlayback::NotifyStreamingStatus(TInt aErr)
{
  // Called by the streaming thread listener when the last buffer has been copied or play has been interrupted
  // Ignore streaming status if we are already handling an error
  if (iStreamingErrorDetected)
	return;

  // Check for an error
  if (aErr < 0)
  {
	// Stop the stream
	StopStreaming();
	iStreamingErrorDetected = ETrue;

	// Notify the user
	TBuf<256> buf, tbuf;
	if (aErr == KErrInUse)
		{
		CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_18);
		CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_30);
		}
	else
		{
		CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_18);
		CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);	
		buf.AppendNum(aErr);
		}

	// Display the message and wait for the user
	iEnv->OggErrorMsgL(tbuf, buf);

	// Notify the UI, try to restart
	iObserver->NotifyPlayInterrupted();
	iStreamingErrorDetected = EFalse;
	return;
  }

  // Handle stream events
  TStreamingThreadStatus status = (TStreamingThreadStatus) aErr;
  switch (status)
  {
	case ELastBufferCopied:
		// Not all phones call MaoscPlayComplete,
		// so start a timer that will stop playback afer a delay 
		iStopAudioStreamingTimer->Wait(KStreamStopDelay);
		break;

	case EPlayInterrupted:
		// Playback has been interrupted so notify the observer
		iObserver->NotifyPlayInterrupted();
		break;
  }
}

void COggPlayback::HandleThreadPanic(RThread& /* aPanicThread */, TInt /* aErr */)
{
  // Great, the streaming thread has panic'd, now what do we do!?
  iStreamingThreadRunning = EFalse;
  iState = EClosed;
  
  // Try to exit cleanly
  iObserver->NotifyFatalPlayError();
}

#else
void COggPlayback::SendBuffer(HBufC8& buf)
{
  if (iEof) return;
  if (iState==EPaused) return;

  long ret=0;
  TInt cnt = 0;
  TPtr8 bufPtr(buf.Des());
  if (iFirstBuffers)  {
      bufPtr.FillZ(4000);
      cnt = 4000;
      ret = 4000;
#ifdef MDCT_FREQ_ANALYSER
      iLatestPlayTime += ret*iTimeBetweenTwoSamples;
#endif
      iFirstBuffers--;
  
  } else {
    
     ret = iOggSampleRateConverter->FillBuffer(bufPtr);
  }

  if (ret < 0) {
    // Error in the stream. Bad luck, we will continue anyway.
  } else {
    TRAPD( err, iStream->WriteL(buf) );
    if (err!=KErrNone) {
      // This should never ever happen.
      TBuf<256> cbuf,tbuf;
      CEikonEnv::Static()->ReadResource(cbuf,R_OGG_ERROR_16);
      CEikonEnv::Static()->ReadResource(tbuf,R_OGG_ERROR_17);
      cbuf.AppendNum(err);
      iEnv->OggErrorMsgL(tbuf,cbuf);
      User::Leave(err);
    }
    iSentIdx++;
    if (iSentIdx>=KBuffers) iSentIdx=0;
    iSent[iSentIdx]= &buf;
  }
}

void COggPlayback::MaoscPlayComplete(TInt aErr)
{
  // Error codes:
  // KErrCancel      -3
  // KErrUnderflow  -10
  // KErrDied       -13  (interrupted by higher priority)
  // KErrInUse      -14

    
  TRACEF(COggLog::VA(_L("MaoscPlayComplete:%d"), aErr ));
  if (iState==EPaused || iState==EClosed) return;

  if (aErr == KErrUnderflow) 
      return;
  if (aErr == KErrInUse) aErr= KErrNone;

  if (aErr == KErrCancel)
      return;

  if (aErr == KErrDied) {
    iObserver->NotifyPlayInterrupted();
    return;
  }

  if (aErr != KErrNone) {
    TBuf<256> buf,tbuf;
    CEikonEnv::Static()->ReadResource(tbuf,R_OGG_ERROR_18);
    CEikonEnv::Static()->ReadResource(buf,R_OGG_ERROR_16);
    buf.AppendNum(aErr);
    iEnv->OggErrorMsgL(tbuf,buf);
    if (iObserver )
    {
        if (!iEof ) 
            iObserver->NotifyPlayInterrupted();
    }
  }

  
}

void COggPlayback::MaoscBufferCopied(TInt aErr, const TDesC8& aBuffer)
{
  // Error codes:
  // KErrCancel      -3  (not sure when this happens, but ignore for now)
  // KErrUnderflow  -10  (ignore this, do as if nothing has happend, and live happily ever after)
  // KErrDied       -13  (interrupted by higher priority)
  // KErrInUse      -14  (the sound device was stolen from us)
  // KErrAbort      -39  (stream was stopped before this buffer was copied)

    
  if (aErr != KErrNone)
    { TRACEF(COggLog::VA(_L("MaoscBufferCopied:%d"), aErr )); }
  if (aErr == KErrCancel) aErr= KErrNone;

  if (aErr == KErrAbort ||
      aErr == KErrInUse ||
      aErr == KErrDied  ||
      aErr == KErrCancel) return;

  if (iState != EPlaying) return;

  TInt b;
  for (b=0; b<KBuffers; b++) if (&aBuffer == iBuffer[b]) break;
  if ( (iEof) && (iSent[iSentIdx] == &aBuffer) )
  {
      // All the buffers have been sent, stop the playback
      // We cannot rely on a MaoscPlayComplete from the CMdaAudioOutputStream
      // since not all phone supports that.
	  iUnderflowing = EFalse;

	  iRestartAudioStreamingTimer->Cancel();
      iStopAudioStreamingTimer->Wait(KStreamStopDelay);
	  return;
  }
  else if (aErr == KErrUnderflow)
  {
#ifdef DELAY_AUDIO_STREAMING_START
      if (!iUnderflowing)
      {
          iFirstUnderflowBuffer = b;
          iLastUnderflowBuffer = b;
          iUnderflowing = ETrue;
      }
      else
          iLastUnderflowBuffer = b;

	  iRestartAudioStreamingTimer->Wait(KStreamRestartDelay);
	  return;
#else
	  aErr = KErrNone;
#endif
  }

  if (aErr == KErrNone)
  {
	  if (iUnderflowing)
	  {
		  iRestartAudioStreamingTimer->Cancel();
		  RestartAudioStreamingCallBack(this);
	  }

	  SendBuffer(*iBuffer[b]); 
  }
  else {
    // An unknown error condition. This should never ever happen.
	RDebug::Print(_L("Oggplay: MaoscBufferCopied unknown error (Error18 Error16)"));
	TBuf<256> buf,tbuf;
    CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_18);
    CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
    buf.AppendNum(aErr);
    iEnv->OggErrorMsgL(tbuf,buf);
  }
}

void COggPlayback::MaoscOpenComplete(TInt aErr) 
{ 
  TRACEF(COggLog::VA(_L("MaoscOpenComplete:%d"), aErr ));
  iMaxVolume=iStream->MaxVolume();
  if (aErr != KErrNone) {
    TBuf<32> buf;
    CEikonEnv::Static()->ReadResource(buf,R_OGG_ERROR_19);
    buf.AppendNum(aErr);
    _LIT(tit,"MaoscOpenComplete");
    iEnv->OggErrorMsgL(tit,buf);
  } else {
    iState = EFirstOpen;
    iStream->SetPriority(KAudioPriority, EMdaPriorityPreferenceTimeAndQuality);
  }
}

TInt COggPlayback::RestartAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;
  
  self->iUnderflowing = EFalse;
  TInt firstUnderflowBuffer = self->iFirstUnderflowBuffer;
  TInt lastUnderflowBuffer = self->iLastUnderflowBuffer;
  TInt i;
  if (lastUnderflowBuffer>=firstUnderflowBuffer)
  {
	for (i = firstUnderflowBuffer ; i<=lastUnderflowBuffer ; i++)
		self->SendBuffer(*(self->iBuffer[i]));
  }
  else
  {
	for (i = firstUnderflowBuffer ; i<KBuffers ; i++)
		self->SendBuffer(*(self->iBuffer[i]));

	for (i = 0 ; i<=lastUnderflowBuffer ; i++)
		self->SendBuffer(*(self->iBuffer[i]));
  }
  return 0;
}
#endif


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

void COggAudioCapabilityPoll::MaoscOpenComplete(TInt aErr) 
{
    TRACEF(COggLog::VA(_L("AudioCapPoll OpenComplete: %d"), aErr ));

    if (aErr==KErrNone) 
        {
        // Mode supported.
        iCaps |= iRate;
        }
    CActiveScheduler::Stop();
    
}

void COggAudioCapabilityPoll::MaoscPlayComplete(TInt /* aErr */)
    {
    }

void COggAudioCapabilityPoll::MaoscBufferCopied(TInt /* aErr */, const TDesC8& /* aBuffer */)
    {
    }
