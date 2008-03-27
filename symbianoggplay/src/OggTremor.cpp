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
#include "OggHttpSource.h"
#include "OggTremor.h"
#include "TremorDecoder.h"
#include "FlacDecoder.h"

#if defined(MP3_SUPPORT)
#include "MadDecoder.h"
#endif

#if defined(SERIES60V3)
#include "S60V3ImplementationUIDs.hrh"
#else
#include "ImplementationUIDs.hrh"
#endif


// The streaming thread needs to have the same stack size as the main thread (blame the mp3 plugin for this)
const TInt KStreamingThreadStackSize = 32768;

// Literals for supported formats
_LIT(KOgg, "ogg");
_LIT(KOggExt, ".ogg");

_LIT(KOga, "oga");
_LIT(KOgaExt, ".oga");

_LIT(KFlac, "flac");
_LIT(KFlacExt, ".flac");

_LIT(KMp3, "mp3");
_LIT(KMp3Ext, ".mp3");

// Literals for our internal controller
_LIT(KOggInternalCodec, "Internal");
_LIT(KOggPlay, "OggPlay");


COggPlayback::COggPlayback(COggMsgEnv* aEnv, CPluginSupportedList& aPluginSupportedList, MPlaybackObserver* aObserver, TInt aMachineUid)
: CAbsPlayback(aPluginSupportedList, aObserver), iEnv(aEnv), iSharedData(*this, iUIThread, iBufferingThread), iMachineUid(aMachineUid),
iMp3LeftDither(iMp3Random), iMp3RightDither(iMp3Random)
	{
	}

void COggPlayback::ConstructL()
	{
	// Set up the session with the file server
	User::LeaveIfError(iFs.Connect());

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
	iStreamingThreadCommandHandler = new(ELeave) CStreamingThreadCommandHandler(iUIThread, iStreamingThread, *iStreamingThreadPanicHandler, iSharedData);
	iSharedData.iStreamingThreadCommandHandler = iStreamingThreadCommandHandler;

	// Create the streaming thread listener
	iStreamingThreadListener = new(ELeave) CStreamingThreadListener(*this, iSharedData);
	iSharedData.iStreamingThreadListener = iStreamingThreadListener;

	// Listen for the audio stream open event
	iStreamingThreadListener->StartListening();

	// Launch the streaming thread
	User::LeaveIfError(iStreamingThreadCommandHandler->ResumeCommandHandlerThread());

	// Record the fact that the streaming thread started ok
	iStreamingThreadRunning = ETrue;

	// Create the buffering active object
	iBufferingThreadAO = new(ELeave) CBufferingThreadAO(iSharedData);
	iBufferingThreadAO->ConstructL();

	// Share it with the streaming thread
	iSharedData.iBufferingThreadAO = iBufferingThreadAO;

	// Add it to the buffering threads active scheduler (this thread)
	CActiveScheduler::Add(iBufferingThreadAO);

	// Allow the access to the machine uid (for phone specific code paths)
	iSharedData.iMachineUid = iMachineUid;

	// Create the timers
	iStartAudioStreamingTimer = new(ELeave) COggTimer(TCallBack(StartAudioStreamingCallBack, this));
	iRestartAudioStreamingTimer = new(ELeave) COggTimer(TCallBack(RestartAudioStreamingCallBack, this));
	iStopAudioStreamingTimer = new(ELeave) COggTimer(TCallBack(StopAudioStreamingCallBack, this));

	// Create the sample rate converter (it also does the buffering and volume boost)
	iOggSampleRateConverter = new(ELeave) COggSampleRateConverter;

	// Initialise codecs
	TPluginImplementationInformation internalPluginInfo(KOggInternalCodec, KOggPlay, TUid::Uid((TInt) KOggTremorUidPlayFormatImplementation), 0);
	iPluginSupportedList.AddPluginL(KOgg, internalPluginInfo, KOggPlayInternalController);

	internalPluginInfo.iUid = TUid::Uid((TInt) KOggFlacUidPlayFormatImplementation);
	iPluginSupportedList.AddPluginL(KOga, internalPluginInfo, KOggPlayInternalController);

	internalPluginInfo.iUid = TUid::Uid((TInt) KNativeFlacUidPlayFormatImplementation);
	iPluginSupportedList.AddPluginL(KFlac, internalPluginInfo, KOggPlayInternalController);

	internalPluginInfo.iUid = TUid::Uid((TInt) KMp3UidPlayFormatImplementation);
	iPluginSupportedList.AddPluginL(KMp3, internalPluginInfo, KOggPlayInternalController);

	// Select codecs (will be overruled by the .ogi file, if it exists)
	iPluginSupportedList.SelectPluginL(KOgg, KOggPlayInternalController);
	iPluginSupportedList.SelectPluginL(KOga, KOggPlayInternalController);
	iPluginSupportedList.SelectPluginL(KFlac, KOggPlayInternalController);

#if !defined(PLUGIN_SYSTEM)
	// Only select the MAD mp3 codec if we don't have a plugin system (that will almost certainly have a mp3 codec)
	iPluginSupportedList.SelectPluginL(KMp3, KOggPlayInternalController);
#endif
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

	// The decoder should be NULL here, if it isn't the streaming thread has panic'd
	// There's not a lot we can do in that case, so just accept the memory leak (it would only be an issue on the emulator anyway)
	// delete iDecoder;

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

MDecoder* COggPlayback::GetDecoderL(const TDesC& aFileName, COggHttpSource* aHttpSource)
	{
	TParsePtrC p(aFileName);
	TFileName ext(p.Ext());

	MDecoder* decoder = NULL;
	if (ext.CompareF(KOggExt) == 0)
#if defined(MULTITHREAD_SOURCE_READS)
		decoder = new(ELeave) CTremorDecoder(iFs, *iStreamingThreadCommandHandler, *iSharedData.iStreamingThreadSourceHandler);
#else
		decoder = new(ELeave) CTremorDecoder(iFs);
#endif

	if (ext.CompareF(KOgaExt) == 0)
#if defined(MULTITHREAD_SOURCE_READS)
		decoder = new(ELeave) CFlacDecoder(iFs, *iStreamingThreadCommandHandler, *iSharedData.iStreamingThreadSourceHandler);
#else
		decoder = new(ELeave) CFlacDecoder(iFs);
#endif

	if (ext.CompareF(KFlacExt) == 0)
#if defined(MULTITHREAD_SOURCE_READS)
		decoder = new(ELeave) CNativeFlacDecoder(iFs, *iStreamingThreadCommandHandler, *iSharedData.iStreamingThreadSourceHandler);
#else
		decoder = new(ELeave) CNativeFlacDecoder(iFs);
#endif

#if defined(MP3_SUPPORT)
	if (ext.CompareF(KMp3Ext) == 0)
#if defined(MULTITHREAD_SOURCE_READS)
		decoder = new(ELeave) CMadDecoder(iFs, *iStreamingThreadCommandHandler, *iSharedData.iStreamingThreadSourceHandler, iMp3Dithering, iMp3LeftDither, iMp3RightDither);
#else
		decoder = new(ELeave) CMadDecoder(iFs, iMp3Dithering, iMp3LeftDither, iMp3RightDither);
#endif

	if (!decoder)
		{
		if (aHttpSource)
#if defined(MULTITHREAD_SOURCE_READS)
			decoder = new(ELeave) CMadDecoder(iFs, *iStreamingThreadCommandHandler, *iSharedData.iStreamingThreadSourceHandler, iMp3Dithering, iMp3LeftDither, iMp3RightDither);
#else
			decoder = new(ELeave) CMadDecoder(iFs, iMp3Dithering, iMp3LeftDither, iMp3RightDither);
#endif
		}
#endif

	if (!decoder)
		User::Leave(KErrNotSupported);

	return decoder;
	}

TInt COggPlayback::Open(const TOggSource& aSource, TUid /* aControllerUid */)
	{
	TRACEF(COggLog::VA(_L("OPEN %d"), iState));

	if (iDecoder)
		{	
		iDecoder->Close();
		delete iDecoder;

		iDecoder = NULL;
		iFileInfo.iFileName = KNullDesC;
		iSharedData.iDecoderSourceData = NULL;
		}

	// Create the source
	COggHttpSource* httpSource = NULL;
	if (aSource.IsStream())
		{
		httpSource = new COggHttpSource;
		if (!httpSource)
			return KErrNoMemory;
		}

	TRAPD(err, iDecoder = GetDecoderL(aSource.iSourceName, httpSource));
	if (err != KErrNone)
		return err;

	if (httpSource)
		{
		// Start the streaming listener
		iStreamingThreadListener->StartListening();
		}

	// Set the file name
	iFileInfo.iFileName = aSource.iSourceName;

	// Attempt to open the file
	err = iDecoder->Open(aSource.iSourceName, httpSource);
	if (err == KErrNone)
		{
		iState = EOpening;

		if (!httpSource)
			OpenComplete(KErrNone);
		}
	else
		{
		iStreamingThreadListener->Cancel();

		iDecoder->Close();
		delete iDecoder;

		iDecoder = NULL;
		iFileInfo.iFileName = KNullDesC;
		}

	return err;
	}

void COggPlayback::OpenComplete(TInt aErr)
	{
	TRACEF(COggLog::VA(_L("OPEN COMPLETE %d"), iState));
	if (aErr != KErrNone)
		{
		iFileInfo.iFileName = KNullDesC;
		iObserver->NotifySourceOpen(aErr);
		return;
		}

	// Set the source data
	iSharedData.iDecoderSourceData = iSharedData.iSourceData;

	// Get the tags
	iDecoder->ParseTags(iFileInfo.iTitle, iFileInfo.iArtist, iFileInfo.iAlbum, iFileInfo.iGenre, iFileInfo.iTrackNumber);

	// Get the rest of the info
	iFileInfo.iBitRate = iDecoder->Bitrate();
	iFileInfo.iChannels = iDecoder->Channels();
	iFileInfo.iFileSize = iDecoder->FileSize();
	iFileInfo.iRate = iDecoder->Rate();
	iFileInfo.iTime = iDecoder->TimeTotal();
  
	// Set audio properties
	aErr = SetAudioCaps(iDecoder->Channels(), iDecoder->Rate());
	if (aErr == KErrNone)
		iState = EStopped;

	iObserver->NotifySourceOpen(aErr);
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
	positionBytes -= iSectionStartBytes;

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

TInt COggPlayback::Info(const TDesC& aFileName, TUid /* aControllerUid */, MFileInfoObserver& aFileInfoObserver)
	{
	MDecoder* decoder = NULL;
	TRAPD(err, decoder = GetDecoderL(aFileName, NULL));
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

TInt COggPlayback::FullInfo(const TDesC& aFileName, TUid /* aControllerUid */, MFileInfoObserver& aFileInfoObserver)
	{
	MDecoder* decoder = NULL;
	TRAPD(err, decoder = GetDecoderL(aFileName, NULL));
	if (err != KErrNone)
		return err;

	err = decoder->Open(aFileName, NULL);
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
	TRACEF(COggLog::VA(_L("PLAY %i "), iState));
	switch(iState)
		{
		case EPaused:
			break;

		case EStopped:
			iEof = EFalse;
			iSection = KErrNotFound;
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

	// Also to avoid the first buffer problem, wait a short time before streaming, 
	// so that Application drawing have been done. Processor should then
	// be fully available for doing audio thingies.
	iStartAudioStreamingTimer->Wait(KStreamStartDelay);

	iObserver->ResumeUpdates();
	iState = EPlaying;
	}

void COggPlayback::Pause()
	{
	TRACEF(COggLog::VA(_L("PAUSE %d"), iState));

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
	TRACEF(COggLog::VA(_L("STOP %d"), iState));
	if ((iState == EStopped) || (iState == EStreamOpen) || (iState == EClosed))
		return;

	// Cancel any open request that is in progress 
	if (iState == EOpening)
		iDecoder->Close();

	// Reset the state
	iState = EStreamOpen;

	// Cancel timers and stop streaming
	CancelTimers();
	StopStreaming();

	// Get rid of the decoder
	if (iDecoder)
		{
		iDecoder->Close();
		delete iDecoder;

		iDecoder = NULL;
		iSharedData.iDecoderSourceData = NULL;
		}

	// Reset file info
	ClearComments();
	iFileInfo.iFileName = KNullDesC;
	iFileInfo.iTime = 0;
	iSectionStartBytes = 0;
	iEof = EFalse;
	iRestartPending = EFalse;

	// Notify the observer
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
    TInt ret = iDecoder->Read(aBuffer, len); 
    if (ret>0)
		{
        aBuffer.SetLength(len + ret);

		if (iDecoder->Section() != iSection)
			{
			// Chained streams
			// Reset stream attributes
			iSection = iDecoder->Section();

			//TODO: in theory new stream can have different attributes
			// then we should stop playback and reinit
			//ret= SetAttributes();

			iNewSection = ETrue;
			}


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

	iEof = (ret == 0) && iDecoder->LastBuffer();
	return ret;
	}

void COggPlayback::StartAudioStreamingCallBack()
	{
	iObserver->NotifyPlayStarted();
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

	// Reset the last play bytes
	iLastPlayTotalBytes = iSharedData.iTotalBufferBytes;

	// Prepare to play (enable double buffering for file reads)
	PrepareToPlay();

	// Start the buffering listener
	if (iSharedData.iBufferingMode == EBufferThread)
		iBufferingThreadAO->StartListening();

#if defined(SERIES60) && !defined(PLUGIN_SYSTEM) // aka S60V1
	// The NGage and 3650 have poor memories and always forget the audio settings
	if ((iMachineUid == EMachineUid_NGage) || (iMachineUid == EMachineUid_NGageQD) || (iMachineUid == EMachineUid_N3650))
		iStreamingThreadCommandHandler->SetAudioProperties();
#endif

	// Start the streaming listener
	iStreamingThreadListener->StartListening();

	// Start the streaming
	iStreamingThreadCommandHandler->StartStreaming();
	}

void COggPlayback::StopStreaming()
	{
	// Stop the streaming
	iStreamingThreadCommandHandler->StopStreaming();

	// Ignore any pending stream events
	iStreamingThreadListener->Cancel();
	iSharedData.iStreamingMessageQueue->Reset();

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

	// Inform the streaming thread we are going to flush buffers
	iStreamingThreadCommandHandler->PrepareToFlushBuffers();

	// Cancel the buffering
	iBufferingThreadAO->Cancel();

	// Flush the buffers
	return iStreamingThreadCommandHandler->FlushBuffers();
	}

TBool COggPlayback::FlushBuffers(TInt64 aNewPosition)
	{
	// The position has been changed so flush the buffers (and set the new position)
	iSharedData.iFlushBufferEvent = EPositionChanged;
	iSharedData.iNewPosition = aNewPosition;

	// Inform the streaming thread we are going to flush buffers
	iStreamingThreadCommandHandler->PrepareToFlushBuffers();

	// Cancel the buffering
	iBufferingThreadAO->Cancel();

	// Flush the buffers
	return iStreamingThreadCommandHandler->FlushBuffers();
	}

void COggPlayback::FlushBuffers(TGainType aNewGain)
	{
	// The volume gain has been changed so flush the buffers (and set the new gain)
	iSharedData.iFlushBufferEvent = EVolumeGainChanged;
	iSharedData.iNewGain = aNewGain;

	// Inform the streaming thread we are going to flush buffers
	TBool streaming = iStreamingThreadCommandHandler->PrepareToFlushBuffers();

	// Cancel the buffering
	// TRACEF(_L("S4"));
	iBufferingThreadAO->Cancel();

	// Start listening if we are still streaming (and in buffer thread mode) 
	// TRACEF(_L("S5"));
	if (streaming && (iSharedData.iBufferingMode == EBufferThread))
		iBufferingThreadAO->StartListening();

	// Flush the buffers
	// TRACEF(_L("S109"));
	iStreamingThreadCommandHandler->FlushBuffers();
	// TRACEF(_L("S120"));
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
	// Create a scheduler
	CActiveScheduler* scheduler = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(scheduler);

	// Install the scheduler
	CActiveScheduler::Install(scheduler);

	// Add the streaming command handler to the scheduler
	CStreamingThreadCommandHandler& streamingThreadCommandHandler = *aSharedData.iStreamingThreadCommandHandler;
	CActiveScheduler::Add(&streamingThreadCommandHandler);

	// Create the source reader
	CStreamingThreadSourceReader* sourceReader = CStreamingThreadSourceReader::NewLC(aSharedData);

	// Create the audio playback engine
	CStreamingThreadPlaybackEngine* playbackEngine = CStreamingThreadPlaybackEngine::NewLC(aSharedData, *sourceReader);

	// Create the streaming thread source handler
	CStreamingThreadSourceHandler* streamingThreadSourceHandler = new(ELeave) CStreamingThreadSourceHandler(aSharedData, *sourceReader);
	aSharedData.iStreamingThreadSourceHandler = streamingThreadSourceHandler;
	CleanupStack::PushL(streamingThreadSourceHandler);

	// Create the streaming thread message queue
	ROggMessageQueue streamingMessageQueue(*playbackEngine);
	aSharedData.iStreamingMessageQueue = &streamingMessageQueue;
	CleanupClosePushL(streamingMessageQueue);

	// Listen for commands and dispatch them to the playback engine
	streamingThreadCommandHandler.ListenForCommands(*playbackEngine, *sourceReader);

	// Inform the UI thread that the streaming thread has started
	streamingThreadCommandHandler.ResumeComplete(KErrNone);

	// Start the scheduler
	CActiveScheduler::Start();

	// Cancel the command handler
	streamingThreadCommandHandler.Cancel();

	// Uninstall the scheduler
	CActiveScheduler::Install(NULL);

	// Delete the scheduler, audio engine, source reader, source handler and message queue
	CleanupStack::PopAndDestroy(5, scheduler);
	}

TBool COggPlayback::PrimeBuffer(HBufC8& buf, TBool& aNewSection)
	{
	// Read and decode the next buffer
	TPtr8 bufPtr(buf.Des());
	iOggSampleRateConverter->FillBuffer(bufPtr);

	aNewSection = iNewSection;
	iNewSection = EFalse;
	return iEof;
	}

void COggPlayback::NotifyStreamOpen(TInt aErr)
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

TBool COggPlayback::NotifyStreamingStatus(TStreamingEvent aEvent, TInt aStatus)
	{
	// Check for an error
	if (aEvent == EStreamingError)
		{
		if (iState == EOpening)
			{
			// An error occured when opening the stream
			OpenComplete(aStatus);
			return EFalse;
			}

		// Pause the stream
		FlushBuffers(EPlaybackPaused);

		// Notify the user
		TBuf<256> buf, tbuf;
		if (aStatus == KErrInUse)
			{
			CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_18);
			CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_30);
			}
		else
			{
			CEikonEnv::Static()->ReadResource(tbuf, R_OGG_ERROR_18);
			CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_16);	
			buf.AppendNum(aStatus);
			}

		// Display the message and wait for the user
		iEnv->OggErrorMsgL(tbuf, buf);

		// Notify the UI, try to restart
		iObserver->NotifyPlayInterrupted();
		return EFalse;
		}

	// Handle stream events
	TBool continueListening = EFalse;
	switch (aEvent)
		{
		case ESourceOpenState:
			iObserver->NotifySourceOpenState(aStatus);

			// Expect more events, so start listening again
			continueListening = ETrue;
			break;

		case ESourceOpened:
			OpenComplete(aStatus);
			break;

		case EDataReceived:
			// TRACEF(_L("U1"));
			if (iSharedData.iBufferFillWaitingForData)
				{
				// TRACEF(_L("U2"));
				iBufferingThreadAO->DataReceived();
				}

			// Expect more events, so start listening again
			continueListening = ETrue;
			break;

		case ELastBufferCopied:
			// Not all phones call MaoscPlayComplete,
			// so start a timer that will stop playback afer a delay 
			iStopAudioStreamingTimer->Wait(KStreamStopDelay);
			break;

		case ENewSectionCopied:
			// New section (aka start of a new track)
			iDecoder->ParseTags(iFileInfo.iTitle, iFileInfo.iArtist, iFileInfo.iAlbum, iFileInfo.iGenre, iFileInfo.iTrackNumber);
			iObserver->NotifyNextTrack();

			// Record the start of the section
			iSectionStartBytes = iSharedData.iTotalBufferBytes - iSharedData.BufferBytes();

			// Expect more events, so start listening again
			continueListening = ETrue;
			break;

		case EPlayInterrupted:
			// Playback has been interrupted so notify the observer
			iObserver->NotifyPlayInterrupted();
			break;

		case EPlayUnderflow:
			// Flush buffers (and pause the stream)
			// TRACEF(_L("Stuart4: Underflow & Restart"));
			FlushBuffers(EPlaybackPaused);
			
			iRestartPending = ETrue;
			break;

		case EPlayStopped:
			// Playback has stopped, schedule a restart if we've stopped because of underflow
			if (iRestartPending)
				{
				iRestartPending = EFalse;
				iRestartAudioStreamingTimer->Wait(KStreamRestartDelay);
				}
			break;

		default:
			User::Panic(_L("COggPlayback:NSS"), 0);
			break;
		}

	return continueListening;
	}

void COggPlayback::HandleThreadPanic(RThread& /* aPanicThread */, TInt aErr)
	{
	// Great, the streaming thread has panic'd, now what do we do!?
	TRACEF(COggLog::VA(_L("Handle Thread Panic: %d"), aErr));

	// Reset the state
	iStreamingThreadRunning = EFalse;
	iState = EClosed;

	// Try to exit cleanly
	iObserver->NotifyFatalPlayError();
	}

void COggPlayback::PrepareToSetPosition()
	{
	// Inform the decoder that seeking may occur
	if (iDecoder)
		iDecoder->PrepareToSetPosition();
	}

void COggPlayback::PrepareToPlay()
	{
	// Inform the decoder that playback is going to start
	iDecoder->PrepareToPlay();
	}

void COggPlayback::ThreadRelease()
	{
	// Inform the decoder that access to the file will now be done from a different thread
	if (iDecoder)
		iDecoder->ThreadRelease();
	}

TBool COggPlayback::Seekable()
	{
	// Multi thread playback can seek when paused or playing
	if ((iState != EPlaying) && (iState != EPaused))
		return EFalse;

	return (iSharedData.iDecoderSourceData->iSource->Seekable());
	}
