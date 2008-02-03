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

// Platform settings
#include <OggOs.h>
#include <OggPlay.rsg>

#if defined(__VC32__)
#pragma warning( disable : 4244 ) // conversion from __int64 to unsigned int: Possible loss of data
#pragma warning( disable : 4355 ) // 'this' used in base member initializer list
#endif

#include "OggLog.h"
#include "OggTremor.h"
#include "TremorDecoder.h"
#include "FlacDecoder.h"

#if defined(MP3_SUPPORT)
#include "MadDecoder.h"
#endif


COggPlayback::COggPlayback(COggMsgEnv* aEnv, MPlaybackObserver* aObserver, TInt aMachineUid)
: CAbsPlayback(aObserver), iEnv(aEnv), iSharedData(*this, iUIThread, iBufferingThread), iMachineUid(aMachineUid),
iMp3LeftDither(iMp3Random), iMp3RightDither(iMp3Random)
	{
	}

const TInt KStreamingThreadStackSize = 32768;
void COggPlayback::ConstructL()
	{
	// Set up the session with the file server
	User::LeaveIfError(iFs.Connect());

#if defined(SERIES60V3)
	User::LeaveIfError(iFs.ShareAuto());
#else
	User::LeaveIfError(iFs.Share());
#endif

	User::LeaveIfError(AttachToFs());

	// Create at least one audio buffer
	iBuffer[0] = HBufC8::NewL(KBufferSize48K);

	// Open thread handles to the UI thread and the buffering thread
	// Currently the UI and buffering threads are the same (this thread)
	TThreadId uiThreadId = iThread.Id();
	User::LeaveIfError(iUIThread.Open(uiThreadId));
	User::LeaveIfError(iBufferingThread.Open(uiThreadId));

	// Create the streaming thread
	User::LeaveIfError(iStreamingThread.Create(_L("OggPlayStream"), StreamingThread, KStreamingThreadStackSize, NULL, &iSharedData));

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

	iStartAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(StartAudioStreamingCallBack, this));
	iRestartAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(RestartAudioStreamingCallBack, this));
	iStopAudioStreamingTimer = new (ELeave) COggTimer(TCallBack(StopAudioStreamingCallBack, this));

	iOggSampleRateConverter = new (ELeave) COggSampleRateConverter;
	}

COggPlayback::~COggPlayback()
	{
	__ASSERT_ALWAYS((iState <= EStopped), User::Panic(_L("~COggPlayback"), 0));

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

	delete iDecoder; 
	delete iStartAudioStreamingTimer;
	delete iRestartAudioStreamingTimer;
	delete iStopAudioStreamingTimer;
	delete iOggSampleRateConverter;

	delete iBufferingThreadAO;
	delete iStreamingThreadListener;
	delete iStreamingThreadCommandHandler;
	delete iStreamingThreadPanicHandler;

	for (TInt i = 0 ; i<KMultiThreadBuffers ; i++)
		delete iBuffer[i];

	iFs.Close();
	}

_LIT(KOggExt, ".ogg");
_LIT(KOgaExt, ".oga");
_LIT(KFlacExt, ".flac");
_LIT(KMp3Ext, ".mp3");
MDecoder* COggPlayback::GetDecoderL(const TDesC& aFileName)
	{
	TParsePtrC p(aFileName);
	TFileName ext(p.Ext());

	MDecoder* decoder = NULL;
	if (ext.CompareF(KOggExt) == 0)
		decoder = new(ELeave) CTremorDecoder(iFs);

	if (ext.CompareF(KOgaExt) == 0)
		decoder = new(ELeave) CFlacDecoder(iFs);

	if (ext.CompareF(KFlacExt) == 0)
		decoder = new(ELeave) CNativeFlacDecoder(iFs);

#if defined(MP3_SUPPORT)
	if (ext.CompareF(KMp3Ext) == 0)
		decoder = new(ELeave) CMadDecoder(iFs, iMp3Dithering, iMp3LeftDither, iMp3RightDither);
#endif

	if (!decoder)
		User::Leave(KErrNotSupported);

	return decoder;
	}

TInt COggPlayback::Open(const TDesC& aFileName)
	{
	TRACEF(COggLog::VA(_L("OPEN") ));

	if (iDecoder)
		{	
		iDecoder->Close();

		delete iDecoder;
		iDecoder = NULL;
		}

	TRAPD(err, iDecoder = GetDecoderL(aFileName));
	if (err != KErrNone)
		return err;

	err = iDecoder->Open(aFileName);
	if (err != KErrNone)
		{
		iDecoder->Close();

		delete iDecoder;
		iDecoder = NULL;

		return err;
		}

	iDecoder->ParseTags(iFileInfo.iTitle, iFileInfo.iArtist, iFileInfo.iAlbum, iFileInfo.iGenre, iFileInfo.iTrackNumber);

	iFileInfo.iFileName = aFileName;
	iFileInfo.iBitRate = iDecoder->Bitrate();
	iFileInfo.iChannels = iDecoder->Channels();
	iFileInfo.iFileSize = iDecoder->FileSize();
	iFileInfo.iRate = iDecoder->Rate();
	iFileInfo.iTime = iDecoder->TimeTotal();
  
	err = SetAudioCaps(iDecoder->Channels(), iDecoder->Rate());
	if (err == KErrNone)
		iState = EStopped;
	else
		{
		iDecoder->Close();

		delete iDecoder;
		iDecoder = NULL;
		}

	return err;
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
	if (theRate == 8000)
		rt = TMdaAudioDataSettings::ESampleRate8000Hz;
	else if (theRate == 11025)
		rt = TMdaAudioDataSettings::ESampleRate11025Hz;
	else if (theRate == 16000)
		rt = TMdaAudioDataSettings::ESampleRate16000Hz;
	else if (theRate == 22050)
		rt = TMdaAudioDataSettings::ESampleRate22050Hz;
	else if (theRate == 32000)
		rt = TMdaAudioDataSettings::ESampleRate32000Hz;
	else if (theRate == 44100)
		rt = TMdaAudioDataSettings::ESampleRate44100Hz;
	else if (theRate == 48000)
		rt = TMdaAudioDataSettings::ESampleRate48000Hz;
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
		err = SetAudioProperties(usedRate, usedChannels);
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

	TBuf<256> buf, tbuf;
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
		err = SetAudioProperties(usedRate, usedChannels);

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

	TInt outStreamByteRate = usedChannels * usedRate * 2;
	iMaxBytesBetweenFrequencyBins = outStreamByteRate / KOggControlFreq;
	return KErrNone;
	}

void COggPlayback::SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, TBool aConvertChannel, TInt /*aNbOfChannels*/)
{
    // Print an error/info msg displaying all supported rates by this HW
    const TInt rates[] = {8000, 11025, 16000, 22050, 32000, 44100, 48000};

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
			err = SetAudioProperties(rates[i], 1);
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
	iStreamingThreadCommandHandler->Volume(); 
	return iSharedData.iVolume;
	}

void COggPlayback::SetVolume(TInt aVol)
	{
	iSharedData.iVolume = aVol;
	iStreamingThreadCommandHandler->SetVolume();
	}

void COggPlayback::SetVolumeGain(TGainType aGain)
	{
	FlushBuffers(aGain);
	}

void COggPlayback::SetMp3Dithering(TBool aDithering)
	{
	iMp3Dithering = aDithering;
	}

void COggPlayback::SetPosition(TInt64 aPos)
	{
	if (iDecoder)
		{
		// Limit FF to five seconds before the end of the track
		if (aPos>iFileInfo.iTime)
			aPos = iFileInfo.iTime - TInt64(5000);

		// And don't allow RW past the beginning either
		if (aPos<0)
			aPos = 0;

		// Pause/Restart the stream instead of just flushing buffers
	    // This works better with FF/RW because, like next/prev track, key repeats are possible (and very likely on FF/RW)
        TBool streamStopped = FlushBuffers(aPos);

		// Restart streaming
		if (streamStopped)
			iRestartAudioStreamingTimer->Wait(KStreamStartDelay);
		}
	}

TInt64 COggPlayback::Position()
	{
	// Approximate position will do here
	const TInt64 KConst500 = TInt64(500);
	TInt64 positionBytes = iSharedData.iTotalBufferBytes - iSharedData.BufferBytes();
	TInt64 positionMillisecs = (KConst500*positionBytes)/TInt64(iSharedData.iSampleRate*iSharedData.iChannels);

	return positionMillisecs;
	}

const TInt32* COggPlayback::GetFrequencyBins()
{
  // Get the precise position from the streaming thread
  iStreamingThreadCommandHandler->Position();
  const TInt64 KConst500000 = TInt64(500000);
  TInt64 streamPositionBytes = (iSharedData.iStreamingPosition.Int64()*TInt64(iSharedData.iSampleRate*iSharedData.iChannels))/KConst500000;
  TInt64 positionBytes = iLastPlayTotalBytes + streamPositionBytes;

  TInt idx = iLastFreqArrayIdx;
  for (TInt i = 0 ; i<KFreqArrayLength ; i++)
    {
      if (iFreqArray[idx].iPcmByte <= positionBytes)
          break;
      
        idx--;
        if (idx < 0)
            idx = KFreqArrayLength-1;
    }

  return iFreqArray[idx].iFreqCoefs;
}

TInt COggPlayback::Info(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver)
	{
	MDecoder* decoder = NULL;
	TRAPD(err, decoder = GetDecoderL(aFileName));
	if (err != KErrNone)
		return err;

	err = decoder->OpenInfo(aFileName);
	if (err != KErrNone)
		{
		decoder->Close();
		delete decoder;
  
		return err;
		}

	decoder->ParseTags(iInfoFileInfo.iTitle, iInfoFileInfo.iArtist, iInfoFileInfo.iAlbum, iInfoFileInfo.iGenre, iInfoFileInfo.iTrackNumber);

	iInfoFileInfo.iFileName = aFileName;

	iInfoFileInfo.iBitRate = 0;
	iInfoFileInfo.iChannels = 0;
	iInfoFileInfo.iRate = 0;
	iInfoFileInfo.iFileSize = 0;
	iInfoFileInfo.iTime = 0;

	decoder->Close(); 
	delete decoder;

	aFileInfoObserver.FileInfoCallback(KErrNone, iInfoFileInfo);
	return KErrNone;
	}

TInt COggPlayback::FullInfo(const TDesC& aFileName, MFileInfoObserver& aFileInfoObserver)
	{
	MDecoder* decoder = NULL;
	TRAPD(err, decoder = GetDecoderL(aFileName));
	if (err != KErrNone)
		return err;

	err = decoder->Open(aFileName);
	if (err != KErrNone)
		{
		decoder->Close();
		delete decoder;
 
		return err;
		}

	decoder->ParseTags(iInfoFileInfo.iTitle, iInfoFileInfo.iArtist, iInfoFileInfo.iAlbum, iInfoFileInfo.iGenre, iInfoFileInfo.iTrackNumber);

	iInfoFileInfo.iFileName = aFileName;
	iInfoFileInfo.iBitRate = decoder->Bitrate();
	iInfoFileInfo.iChannels = decoder->Channels();
	iInfoFileInfo.iRate = decoder->Rate();

	iInfoFileInfo.iFileSize = decoder->FileSize();
	iInfoFileInfo.iTime = decoder->TimeTotal();

	decoder->Close(); 
	delete decoder;

	aFileInfoObserver.FileInfoCallback(KErrNone, iInfoFileInfo);
	return KErrNone;
	}

void COggPlayback::InfoCancel()
	{
	}

void COggPlayback::Play() 
	{
	TRACEF(COggLog::VA(_L("PLAY %i "), iState ));
	switch(iState)
		{
		case EPaused:
			break;

		case EStopped:
			iEof = 0;
			break;

		default:
			iEnv->OggErrorMsgL(R_OGG_ERROR_20, R_OGG_ERROR_15);
			return;
		}

	iLastFreqArrayIdx = 0;
	iBytesSinceLastFrequencyBin = 0;

	// Clear the frequency analyser
	Mem::FillZ(&iFreqArray, sizeof(iFreqArray));

	// Clear mp3 error bits
	iMp3LeftDither.iError[0] = 0 ; iMp3LeftDither.iError[1] = 0 ; iMp3LeftDither.iError[2] = 0;
	iMp3RightDither.iError[0] = 0 ; iMp3RightDither.iError[1] = 0 ; iMp3RightDither.iError[2] = 0;

#if defined(DELAY_AUDIO_STREAMING_START)
	// Also to avoid the first buffer problem, wait a short time before streaming, 
	// so that Application drawing have been done. Processor should then
	// be fully available for doing audio thingies.
	iStartAudioStreamingTimer->Wait(KStreamStartDelay);
#else
	StartStreaming();
#endif

	iState = EPlaying;
	iObserver->ResumeUpdates();

#if defined(DELAY_AUDIO_STREAMING_START)
	iObserver->NotifyPlayStarted();
#endif
	}

void COggPlayback::Pause()
	{
	TRACEF(COggLog::VA(_L("PAUSE")));

	if (iState != EPlaying)
		return;

	iState = EPaused;
	CancelTimers();

	// Flush buffers (and pause the stream)
	FlushBuffers(EPlaybackPaused);
	}

void COggPlayback::Resume()
	{
	// Resume is equivalent of Play() 
	Play();
	}

void COggPlayback::Stop()
	{
	TRACEF(COggLog::VA(_L("STOP") ));
	if ((iState == EStopped) || (iState == EClosed))
		return;

	iState = EStreamOpen;

	CancelTimers();
	StopStreaming();

	if (iDecoder)
		{
		iDecoder->Close();

		delete iDecoder;
		iDecoder = NULL;
		}

	ClearComments();
	iFileInfo.iTime = 0;
	iEof = EFalse;

	if (iObserver)
		iObserver->NotifyUpdate();
	}

void COggPlayback::CancelTimers()
	{
	iStartAudioStreamingTimer->Cancel();
	iRestartAudioStreamingTimer->Cancel();
	iStopAudioStreamingTimer->Cancel();
	}

TInt COggPlayback::GetNewSamples(TDes8 &aBuffer)
	{
    if (iEof)
        return(KErrNotReady);

	if ((iBytesSinceLastFrequencyBin >= iMaxBytesBetweenFrequencyBins) && !iRequestingFrequencyBins)
		{
		// Make a request for frequency data
		iDecoder->GetFrequencyBins(iFreqArray[iLastFreqArrayIdx].iFreqCoefs, KNumberOfFreqBins);

		// Mark that we have issued the request
		iRequestingFrequencyBins = ETrue;
		}
    
    TInt len = aBuffer.Length();
    TInt ret = iDecoder->Read(aBuffer,len); 
    if (ret>0)
		{
        aBuffer.SetLength(len + ret);
		iBytesSinceLastFrequencyBin += ret;

		if (iRequestingFrequencyBins)
			{
			// Determine the status of the request
			TInt requestingFrequencyBins = iDecoder->RequestingFrequencyBins();
			if (!requestingFrequencyBins)
				{
				// The frequency request has completed
				iRequestingFrequencyBins = EFalse;
				iBytesSinceLastFrequencyBin = 0;

				// Time stamp the data
				iFreqArray[iLastFreqArrayIdx].iPcmByte = iSharedData.iTotalBufferBytes;

				// Move to the next array entry
				iLastFreqArrayIdx++;
				if (iLastFreqArrayIdx == KFreqArrayLength)
					iLastFreqArrayIdx = 0;

				// Mark it invalid
#if defined(SERIES60V3)
				iFreqArray[iLastFreqArrayIdx].iPcmByte = KMaxTInt64;
#else
				iFreqArray[iLastFreqArrayIdx].iPcmByte = TInt64(KMaxTInt32, KMaxTUint32);
#endif
				}
			}
		}

    if (ret == 0)
        iEof = ETrue;

	return ret;
	}

void COggPlayback::StartAudioStreamingCallBack()
	{
	StartStreaming();
	}

void COggPlayback::RestartAudioStreamingCallBack()
	{
	StartStreaming();
	}

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

TInt COggPlayback::AttachToFs()
	{
#if defined(SERIES60V3)
	return KErrNone;
#else
	return iFs.Attach();
#endif
	}

TInt COggPlayback::SetAudioProperties(TInt aSampleRate, TInt aChannels)
	{
	iSharedData.iSampleRate = aSampleRate;
	iSharedData.iChannels = aChannels;
	return iStreamingThreadCommandHandler->SetAudioProperties();
	}

void COggPlayback::StartStreaming()
	{
	// Reset the frequency analyser
	iLastFreqArrayIdx = 0;
	Mem::FillZ(&iFreqArray, sizeof(iFreqArray));

	iLastPlayTotalBytes = iSharedData.iTotalBufferBytes;

	// Start the buffering listener
	if (iSharedData.iBufferingMode == EBufferThread)
		iBufferingThreadAO->StartListening();

#if defined(SERIES60) && !defined(SERIES60V3)
	// The NGage has a poor memory and always forgets the audio settings
	if ((iMachineUid == EMachineUid_NGage) || (iMachineUid == EMachineUid_NGageQD))
		iStreamingThreadCommandHandler->SetAudioProperties();
#endif

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

	iObserver->NotifyStreamOpen(aErr);
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
