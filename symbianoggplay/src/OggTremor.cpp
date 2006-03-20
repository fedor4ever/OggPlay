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
    // These basic rates should be supported by all Symbian OS
    // Higher quality might not be supported.
    iSettings.iChannels  = TMdaAudioDataSettings::EChannelsMono;
    iSettings.iSampleRate= TMdaAudioDataSettings::ESampleRate8000Hz;
	iSettings.iFlags = TMdaAudioDataSettings::ENoNetworkRouting;
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

#if defined(MULTI_THREAD_PLAYBACK)
  // Create at least one audio buffer
  iBuffer[0] = HBufC8::NewL(KBufferSize48K);

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

  // Create the streaming thread listener
  iStreamingThreadListener = new(ELeave) CStreamingThreadListener(*this, iSharedData);
  iSharedData.iStreamingThreadListener = iStreamingThreadListener;

  // Launch the streaming thread
  User::LeaveIfError(iStreamingThreadCommandHandler->ResumeCommandHandlerThread());

  // Record the fact that the streaming thread started ok
  iStreamingThreadRunning = ETrue;

  // Create the buffering active object
  iBufferingThreadAO = new(ELeave) CBufferingThreadAO(iSharedData);
  iSharedData.iBufferingThreadAO = iBufferingThreadAO;

  // Add it to the buffering threads active scheduler (this thread)
  CActiveScheduler::Add(iBufferingThreadAO);
#else
  iStream = CMdaAudioOutputStream::NewL(*this);
  iStream->Open(&iSettings);

  for (TInt i=0; i<KBuffers; i++)
	iBuffer[i] = HBufC8::NewL(KBufferSize);
#endif

  iStartAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(StartAudioStreamingCallBack, this));
  iRestartAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(RestartAudioStreamingCallBack, this));
  iStopAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(StopAudioStreamingCallBack, this));

  iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;

  // Read the device uid
  HAL::Get(HALData::EMachineUid, iMachineUid);
  TRACEF(COggLog::VA(_L("Phone UID: %x"), iMachineUid ));
}

COggPlayback::~COggPlayback()
{
  __ASSERT_ALWAYS((iState == EClosed) || (iState == EStopped), User::Panic(_L("~COggPlayback"), 0));
	
  delete iDecoder; 
  delete iStartAudioStreamingTimer;
  delete iRestartAudioStreamingTimer;
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

	delete iDecoder;
	iDecoder = NULL;
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

  iDecoder = GetDecoderL(aFileName);
  if(iDecoder->Open(iFile, aFileName) < 0)
  {
    iDecoder->Close();
    iFile->Close();
	delete iFile;
	iFile = NULL;
    
	delete iDecoder;
	iDecoder = NULL;

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
	iState= EStopped;
  else
  {
	iDecoder->Close();
	iFile->Close();
  
	delete iFile;
	iFile = NULL;

	delete iDecoder;
	iDecoder = NULL;
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

TBool COggPlayback::GetNextLowerRate(TInt& usedRate, TMdaAudioDataSettings::TAudioCaps& rt)
{
  TBool retValue = ETrue;
  switch (usedRate)
  {
	case 48000:
		usedRate = 44100;
		rt = TMdaAudioDataSettings::ESampleRate44100Hz;
		break;

	case 44100:
		usedRate = 32000;
		rt = TMdaAudioDataSettings::ESampleRate32000Hz;
		break;

	case 32000:
		usedRate = 22050;
		rt = TMdaAudioDataSettings::ESampleRate22050Hz;
		break;

	case 22050:
		usedRate = 16000;
		rt = TMdaAudioDataSettings::ESampleRate16000Hz;
		break;

	case 16000:
		usedRate = 11025;
		rt = TMdaAudioDataSettings::ESampleRate11025Hz;
		break;

	case 11025:
		usedRate = 8000;
		rt = TMdaAudioDataSettings::ESampleRate8000Hz;
		break;

	default:
		retValue = EFalse;
		break;
  }

  return retValue;
}

TInt COggPlayback::SetAudioCaps(TInt theChannels, TInt theRate)
{
  TMdaAudioDataSettings::TAudioCaps ac;
  TMdaAudioDataSettings::TAudioCaps rt;
  TBool convertChannel = EFalse;
  TBool convertRate = EFalse;

  TInt usedRate = theRate;
  if (theRate==8000) rt= TMdaAudioDataSettings::ESampleRate8000Hz;
  else if (theRate==11025) rt= TMdaAudioDataSettings::ESampleRate11025Hz;
  else if (theRate==16000) rt= TMdaAudioDataSettings::ESampleRate16000Hz;
  else if (theRate==22050) rt= TMdaAudioDataSettings::ESampleRate22050Hz;
  else if (theRate==32000) rt= TMdaAudioDataSettings::ESampleRate32000Hz;
  else if (theRate==44100) rt= TMdaAudioDataSettings::ESampleRate44100Hz;
  else if (theRate==48000) rt= TMdaAudioDataSettings::ESampleRate48000Hz;
  else
  {
      // Rate not supported by the phone
	  TRACEF(COggLog::VA(_L("SetAudioCaps: Non standard rate: %d"), theRate));

	  // Convert to nearest rate
      convertRate = ETrue;
	  if (theRate>48000)
	  {
		  usedRate = 48000;
	      rt = TMdaAudioDataSettings::ESampleRate48000Hz;
	  }
	  else if (theRate>44100)
	  {
		  usedRate = 44100;
	      rt = TMdaAudioDataSettings::ESampleRate44100Hz;
	  }
	  else if (theRate>32000)
	  {
		  usedRate = 32000;
	      rt = TMdaAudioDataSettings::ESampleRate32000Hz;
	  }
	  else if (theRate>22050)
	  {
		  usedRate = 22050;
	      rt = TMdaAudioDataSettings::ESampleRate22050Hz;
	  }
	  else if (theRate>16000)
	  {
		  usedRate = 16000;
	      rt = TMdaAudioDataSettings::ESampleRate16000Hz;
	  }
	  else if (theRate>11025)
	  {
		  usedRate = 11025;
	      rt = TMdaAudioDataSettings::ESampleRate11025Hz;
	  }
	  else if (theRate>8000)
	  {
		  usedRate = 8000;
	      rt = TMdaAudioDataSettings::ESampleRate8000Hz;
	  }
	  else
	  {
		  // Frequencies less than 8KHz are not supported
	      iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_12);
		  return KErrNotSupported;
	  }
  }

  TInt usedChannels = theChannels;
  if (usedChannels == 1)
	  ac = TMdaAudioDataSettings::EChannelsMono;
  else if (usedChannels == 2)
	  ac = TMdaAudioDataSettings::EChannelsStereo;
  else
  {
    iEnv->OggErrorMsgL(R_OGG_ERROR_12, R_OGG_ERROR_10);
    return KErrNotSupported;
  }

  // Note current settings
  TInt bestRate = usedRate;
  TInt convertingRate = convertRate;
  TMdaAudioDataSettings::TAudioCaps bestRT = rt;

  // Try the current settings.
  // Adjust sample rate and channels if necessary
  TInt err = KErrNotSupported;
  while (err == KErrNotSupported)
	{
	#if defined(MULTI_THREAD_PLAYBACK)
	err = SetAudioProperties(usedRate, usedChannels);
	#else
	TRAP(err, iStream->SetAudioPropertiesL(rt, ac));
	#endif

	if (err == KErrNotSupported)
	{
		// Frequency is not supported
		// Try dropping the frequency
		convertRate = GetNextLowerRate(usedRate, rt);

		// If that doesn't work, try changing stereo to mono
		if (!convertRate && (usedChannels == 2))
		{
			// Reset the sample rate
			convertRate = convertingRate;
			usedRate = bestRate;
			rt = bestRT;

			// Drop channels to 1
			usedChannels = 1;
			ac = TMdaAudioDataSettings::EChannelsMono;
			convertChannel = ETrue;
		}
		else if (!convertRate)
			break; // Give up, nothing supported :-(
	}
  }

  TBuf<256> buf,tbuf;
  if (err != KErrNone)
  {
	CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
	CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_12);
	buf.AppendNum(err);
	iEnv->OggErrorMsgL(tbuf,buf);

	return err;
  }

  // Trace the settings
  TRACEF(COggLog::VA(_L("SetAudioCaps: theRate: %d, theChannels: %d, usedRate: %d, usedChannels: %d"), theRate, theChannels, usedRate, usedChannels));

  if ((convertRate || convertChannel) && iEnv->WarningsEnabled())
  {
	  // Display a warning message
      SamplingRateSupportedMessage(convertRate, theRate, convertChannel, theChannels);

	  // Put the audio properties back the way they were 
	  #if defined(MULTI_THREAD_PLAYBACK)
	  err = SetAudioProperties(usedRate, usedChannels);
	  #else
	  TRAP(err, iStream->SetAudioPropertiesL(rt, ac));
	  #endif

	  // Check that it worked (it should have)
	  if (err != KErrNone)
	  {
		CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);
		CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_12);
		buf.AppendNum(err);
		iEnv->OggErrorMsgL(tbuf,buf);

		return err;
	  }
  }

  // Determine buffer sizes so that we make approx 10-15 calls to the mediaserver per second
  // + another 10-15 calls for position if the frequency analyzer is on screen
  TInt bufferSize = 0;
  switch (usedRate)
  {
	case 48000:
	case 44100:
		bufferSize = KBufferSize48K;
		break;

	case 32000:
		bufferSize = KBufferSize32K;
		break;

	case 22050:
		bufferSize = KBufferSize22K;
		break;

	case 16000:
		bufferSize = KBufferSize16K;
		break;

	case 11025:
	case 8000:
		bufferSize = KBufferSize11K;
		break;

	default:
		User::Panic(_L("COggPlayback:SAC"), 0);
		break;
  }

  if (usedChannels == 1)
	  bufferSize /= 2;

  iOggSampleRateConverter->Init(this, bufferSize, bufferSize-1024, theRate, usedRate, theChannels, usedChannels);
  return KErrNone;
}


void COggPlayback::SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, TBool aConvertChannel, TInt /*aNbOfChannels*/)
{
    // Print an error/info msg displaying all supported rates by this HW
    const TInt rates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000};

#if !defined(MULTI_THREAD_PLAYBACK)
    const TInt ratesMask[] =
	{TMdaAudioDataSettings::ESampleRate8000Hz, TMdaAudioDataSettings::ESampleRate11025Hz,
	TMdaAudioDataSettings::ESampleRate16000Hz, TMdaAudioDataSettings::ESampleRate22050Hz,
	TMdaAudioDataSettings::ESampleRate32000Hz, TMdaAudioDataSettings::ESampleRate44100Hz, TMdaAudioDataSettings::ESampleRate48000Hz };
#endif

    HBufC * hBufMsg = HBufC::NewL(1000);
    CleanupStack::PushL(hBufMsg);

	HBufC * hTmpBuf = HBufC::NewL(500);
    CleanupStack::PushL(hTmpBuf);

    TPtr bufMsg = hBufMsg->Des();
    TPtr tmpBuf = hTmpBuf->Des();
    if (aConvertRate)
    {
        TBuf<128> buf;
        CEikonEnv::Static()->ReadResource(tmpBuf, R_OGG_ERROR_24);
		TBool first = ETrue;
		TInt err;
		for (TInt i=0; i<7; i++)
		{
			#if defined(MULTI_THREAD_PLAYBACK)
			err = SetAudioProperties(rates[i], 1);
			#else
			TRAP(err, iStream->SetAudioPropertiesL(ratesMask[i], TMdaAudioDataSettings::EChannelsMono));
			#endif

			if (err == KErrNone)
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

		bufMsg.Format(tmpBuf, aRate, &buf);
    }

	if (aConvertChannel)
    {
        CEikonEnv::Static()->ReadResource(tmpBuf, R_OGG_ERROR_25);
        bufMsg.Append(tmpBuf);
    }
    
    CEikonEnv::Static()->ReadResource(tmpBuf, R_OGG_ERROR_26);
    bufMsg.Append(tmpBuf);

    iEnv->OggWarningMsgL(bufMsg);
    CleanupStack::PopAndDestroy(2, hBufMsg);
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
  FlushBuffers(aGain);
#else
  iOggSampleRateConverter->SetVolumeGain(aGain);
#endif
}

void COggPlayback::SetPosition(TInt64 aPos)
{
  if(iDecoder)
  {
	  // Limit FF to five seconds before the end of the track
	  if (aPos>iTime)
		  aPos = iTime - TInt64(5000);

	  // And don't allow RW past the beginning either
	  if (aPos<0)
		  aPos = 0;

#if defined(MULTI_THREAD_PLAYBACK)
		// Pause/Restart the stream instead of just flushing buffers
	    // This works better with FF/RW because, like next/prev track, key repeats are possible (and very likely on FF/RW)
        TBool streamStopped = FlushBuffers(aPos);

		// Restart streaming
		if (streamStopped)
			iRestartAudioStreamingTimer->Wait(KStreamStartDelay);
#else
	  iDecoder->Setposition(aPos);
#endif
  }
}

TInt64 COggPlayback::Position()
{
#if defined(MULTI_THREAD_PLAYBACK)
  // Approximate position will do here
  const TInt64 KConst500 = TInt64(500);
  TInt64 positionBytes = iSharedData.iTotalBufferBytes - iSharedData.iBufferBytes;
  TInt64 positionMillisecs = (KConst500*positionBytes)/TInt64(iSharedData.iSampleRate*iSharedData.iChannels);

  return positionMillisecs;
#else
  // TO DO: Also take into account the buffers (see comments in GetFrequencyBins)
  if(iDecoder) return iDecoder->Position();

  return 0;
#endif
}

TInt64 COggPlayback::Time()
{
  return iTime;
}

#ifdef MDCT_FREQ_ANALYSER
const TInt32 * COggPlayback::GetFrequencyBins()
{
#if defined(MULTI_THREAD_PLAYBACK)
  // Get the precise position from the streaming thread
  iStreamingThreadCommandHandler->Position();
  const TInt64 KConst500000 = TInt64(500000);
  TInt64 streamPositionBytes = (iSharedData.iStreamingPosition.Int64()*TInt64(iSharedData.iSampleRate*iSharedData.iChannels))/KConst500000;
  TInt64 positionBytes = iLastPlayTotalBytes + streamPositionBytes;
#else
  TTimeIntervalMicroSeconds currentPos = TInt64(1000)*Position();
#endif

  TInt idx = iLastFreqArrayIdx;
  for (TInt i=0; i<KFreqArrayLength; i++)
    {
#if defined(MULTI_THREAD_PLAYBACK)
      if (iFreqArray[idx].iTime <= positionBytes)
#else
      if (iFreqArray[idx].iTime <= currentPos)
#endif
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
    switch(iState)
	{
    case EPaused:
        break;

    case EStopped:  
        iEof=0;
        break;

    default:
        iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_15);
        RDebug::Print(_L("Oggplay: Tremor - State not Open"));
        return;
  }

  // There is something wrong how Nokia audio streaming handles the
  // first buffers. They are somehow swallowed.
  // To avoid that, send few (4) almost empty buffers
  if (iMachineUid != EMachineUid_SendoX) // Sendo X doesn't need this fix. 
      iFirstBuffers = 4; 

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

  iState = EPlaying;
  iObserver->ResumeUpdates();

#if defined(DELAY_AUDIO_STREAMING_START)
#ifdef MDCT_FREQ_ANALYSER
  // Clear the frequency analyser
  Mem::FillZ(&iFreqArray, sizeof(iFreqArray));
#endif

  iObserver->NotifyPlayStarted();
#endif
}

void COggPlayback::Pause()
{
  TRACEF(COggLog::VA(_L("PAUSE")));
  if (iState != EPlaying) return;

  iState = EPaused;
  CancelTimers();

#if defined(MULTI_THREAD_PLAYBACK)
  // Flush buffers (and pause the stream)
  FlushBuffers(EPlaybackPaused);
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
  if ((iState == EStopped) || (iState == EClosed))
    return;

  iState = EStopped;
  CancelTimers();

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

void COggPlayback::CancelTimers()
{
  iStartAudioStreamingTimer->Cancel();
  iRestartAudioStreamingTimer->Cancel();
  iStopAudioStreamingTimer->Cancel();
}

TInt COggPlayback::GetNewSamples(TDes8 &aBuffer, TBool aRequestFrequencyBins)
{
    if (iEof)
        return(KErrNotReady);

#ifdef MDCT_FREQ_ANALYSER
	if (aRequestFrequencyBins && !iRequestingFrequencyBins)
	{
		// Make a request for frequency data
		iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].iFreqCoefs, KNumberOfFreqBins);

		// Mark that we have issued the request
		iRequestingFrequencyBins = ETrue;
	}
#endif
    
    TInt len = aBuffer.Length();
    TInt ret = iDecoder->Read(aBuffer,len); 
    if (ret >0)
    {
        aBuffer.SetLength(len + ret);

#ifdef MDCT_FREQ_ANALYSER
	if (iRequestingFrequencyBins)
	{
		// Determine the status of the request
		TInt requestingFrequencyBins = iDecoder->RequestingFrequencyBins();
		if (!requestingFrequencyBins)
		{
			iLastFreqArrayIdx++;
			if (iLastFreqArrayIdx >= KFreqArrayLength)
				iLastFreqArrayIdx = 0;

			// The frequency request has completed
			#if defined(MULTI_THREAD_PLAYBACK)
			iFreqArray[iLastFreqArrayIdx].iTime = iSharedData.iTotalBufferBytes;
			#else
			iFreqArray[iLastFreqArrayIdx].iTime = TInt64(iLatestPlayTime);
			#endif

			iRequestingFrequencyBins = EFalse;
		}
	}
#endif
    }

    if (ret == 0)
        iEof= ETrue;

	return ret;
}

void COggPlayback::StartAudioStreamingCallBack()
{
  #if defined(MULTI_THREAD_PLAYBACK)
  StartStreaming();
  #else
  for (TInt i=0; i<KBuffers; i++) 
	SendBuffer(*(iBuffer[i]));
  #endif
}

#if defined(MULTI_THREAD_PLAYBACK)
void COggPlayback::RestartAudioStreamingCallBack()
{
  StartStreaming();
}
#endif

void COggPlayback::StopAudioStreamingCallBack()
{
  Stop();
  iObserver->NotifyPlayComplete();
}

TInt COggPlayback::StartAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;
  self->StartAudioStreamingCallBack();

  return 0;
}

TInt COggPlayback::RestartAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;
  self->RestartAudioStreamingCallBack();

  return 0;
}

TInt COggPlayback::StopAudioStreamingCallBack(TAny* aPtr)
{
  COggPlayback* self= (COggPlayback*) aPtr;
  self->StopAudioStreamingCallBack();

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
#ifdef MDCT_FREQ_ANALYSER
  // Reset the frequency analyser
  iLastFreqArrayIdx = 0;
  Mem::FillZ(&iFreqArray, sizeof(iFreqArray));

  iLastPlayTotalBytes = iSharedData.iTotalBufferBytes;
#endif

  // Start the buffering listener
  if (iSharedData.iBufferingMode == EBufferThread)
	iBufferingThreadAO->StartListening();

  // The NGage has a poor memory and always forgets the audio settings
  if ((iMachineUid == EMachineUid_NGage) || (iMachineUid == EMachineUid_NGageQD))
	iStreamingThreadCommandHandler->SetAudioProperties();

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

  // Flush the buffers (and stop the stream)
  TBool streamStopped = FlushBuffers(EBufferingModeChanged);

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
	  if (streamStopped)
		iRestartAudioStreamingTimer->Wait(KStreamStartDelay);

	  return err;
  }

  // Set the new buffering mode values in the streaming thread
  iStreamingThreadCommandHandler->SetBufferingMode();

  // Restart the stream
  if (streamStopped)
	iRestartAudioStreamingTimer->Wait(KStreamStartDelay);

  return KErrNone;
}

void COggPlayback::SetThreadPriority(TStreamingThreadPriority aNewThreadPriority)
{
  // Set the thread priorities (Normal or High)
  iSharedData.iStreamingThreadPriority = aNewThreadPriority;
  iStreamingThreadCommandHandler->SetThreadPriority();
}

TBool COggPlayback::FlushBuffers(TFlushBufferEvent aFlushBufferEvent)
{
  __ASSERT_DEBUG((aFlushBufferEvent == EBufferingModeChanged) || (aFlushBufferEvent == EPlaybackPaused), User::Panic(_L("COggPlayback::FB"), 0));

  // Set the flush buffer event
  iSharedData.iFlushBufferEvent = aFlushBufferEvent;

  iStreamingThreadCommandHandler->PrepareToFlushBuffers();
  iBufferingThreadAO->Cancel();

  return iStreamingThreadCommandHandler->FlushBuffers();
}

TBool COggPlayback::FlushBuffers(TInt64 aNewPosition)
{
  // The position has been changed so flush the buffers (and set the new position)
  iSharedData.iFlushBufferEvent = EPositionChanged;
  iSharedData.iNewPosition = aNewPosition;

  iStreamingThreadCommandHandler->PrepareToFlushBuffers();
  iBufferingThreadAO->Cancel();

  return iStreamingThreadCommandHandler->FlushBuffers();
}

void COggPlayback::FlushBuffers(TGainType aNewGain)
{
  // The volume gain has been changed so flush the buffers (and set the new gain)
  iSharedData.iFlushBufferEvent = EVolumeGainChanged;
  iSharedData.iNewGain = aNewGain;

  TBool streaming = iStreamingThreadCommandHandler->PrepareToFlushBuffers();
  iBufferingThreadAO->Cancel();

  if (streaming && (iSharedData.iBufferingMode == EBufferThread))
	iBufferingThreadAO->StartListening();

  iStreamingThreadCommandHandler->FlushBuffers();
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
	iState = EStreamOpen;
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
				iBuffer[i] = HBufC8::New(KBufferSize48K);
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
				iBuffer[i] = HBufC8::New(KBufferSize48K);
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
				iBuffer[i] = HBufC8::New(KBufferSize48K);
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
  // Theoretically it's possible for a restart to be in progress, so make sure it doesn't complete
  // (i.e. when the streaming thread generates an event at the same time as the UI thread has started processing a RW\FW key press)
  iRestartAudioStreamingTimer->Cancel();

  // Ignore streaming status if we are already handling an error
  if (iStreamingErrorDetected)
	return;

  // Check for an error
  if (aErr < 0)
  {
	// Pause the stream
    FlushBuffers(EPlaybackPaused);
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

	case EPlayUnderflow:
		// Flush buffers (and pause the stream)
		FlushBuffers(EPlaybackPaused);
		iRestartAudioStreamingTimer->Wait(KStreamRestartDelay);
		break;

	default:
		User::Panic(_L("COggPlayback:NSS"), 0);
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
  if (iState==EPaused || iState==EStopped) return;

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
    iState = EStopped;
    iStream->SetPriority(KAudioPriority, EMdaPriorityPreferenceTimeAndQuality);
  }
}

void COggPlayback::RestartAudioStreamingCallBack()
{
  iUnderflowing = EFalse;
  TInt firstUnderflowBuffer = iFirstUnderflowBuffer;
  TInt lastUnderflowBuffer = iLastUnderflowBuffer;
  TInt i;
  if (lastUnderflowBuffer>=firstUnderflowBuffer)
  {
	for (i = firstUnderflowBuffer ; i<=lastUnderflowBuffer ; i++)
		SendBuffer(*(iBuffer[i]));
  }
  else
  {
	for (i = firstUnderflowBuffer ; i<KBuffers ; i++)
		SendBuffer(*(iBuffer[i]));

	for (i = 0 ; i<=lastUnderflowBuffer ; i++)
		SendBuffer(*(iBuffer[i]));
  }
}
#endif
