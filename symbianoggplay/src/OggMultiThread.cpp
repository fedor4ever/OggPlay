/*
 *  Copyright (c) 2005 S. Fisher
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

// Platform settings
#include <OggOs.h>
#include <e32std.h>

#include "OggLog.h"
#include "OggTremor.h"
#include "OggMultiThread.h"


// Shared data class
TStreamingThreadData::TStreamingThreadData(COggPlayback& aOggPlayback, RThread& aUIThread, RThread& aBufferingThread)
: iOggPlayback(aOggPlayback), iUIThread(aUIThread), iBufferingThread(aBufferingThread),
iNumBuffersRead(0), iNumBuffersPlayed(0), iPrimeBufNum(0), iBufRequestInProgress(EFalse), iLastBuffer(NULL),
iBufferBytesRead(0), iBufferBytesPlayed(0), iTotalBufferBytes(0), iSampleRate(8000), iChannels(1), iBufferingMode(ENoBuffering)
	{
	}

// Buffering AO (base class for CBufferingThreadAO and CStreamingThreadAO)
CBufferingAO::CBufferingAO(TInt aPriority, TStreamingThreadData& aSharedData)
: CActive(aPriority), iSharedData(aSharedData)
	{
	}

CBufferingAO::~CBufferingAO()
	{
	iTimer.Close();
	}

void CBufferingAO::CreateTimerL()
	{
	User::LeaveIfError(iTimer.CreateLocal());
	}

void CBufferingAO::SelfComplete()
	{
#if defined(SERIES60) && !defined(PLUGIN_SYSTEM) // aka S60V1
	if (iSharedData.iMachineUid == EMachineUid_SendoX)
		{
		// The Sendo has an issue that sometimes the timers on the phone run slow
		// Consequently, we just self complete here and avoid using timers at all
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrNone);

		SetActive();
		return;
		}
#endif

	// On all other phones we manually de-schedule the thread in order to improve
	// multi-tasking performance and also to improve stability on S60V3 (I had some issues
	// that were possibly file server related, so by de-scheduling we try to avoid accessing the fileserver too much)
	iTimer.After(iStatus, 1000);
	SetActive();
	}

void CBufferingAO::SelfCompleteCancel()
	{
	iTimer.Cancel();
	}

// Decode (prime) the next audio buffer
// iSharedData.iPrimeBufNum holds the index of the next buffer
// Set the last buffer ptr if eof is encountered
void CBufferingAO::PrimeNextBuffer()
	{
	TBool lastBuffer = iSharedData.iOggPlayback.PrimeBuffer(*(iSharedData.iOggPlayback.iBuffer[iSharedData.iPrimeBufNum]));
	TInt bufferBytes = iSharedData.iOggPlayback.iBuffer[iSharedData.iPrimeBufNum]->Length();
	iSharedData.iTotalBufferBytes += bufferBytes;

	iSharedData.iBufferBytesRead += bufferBytes;
	iSharedData.iNumBuffersRead++;

	if (lastBuffer)
		iSharedData.iLastBuffer = iSharedData.iOggPlayback.iBuffer[iSharedData.iPrimeBufNum];
 
	iSharedData.iPrimeBufNum++;
	if (iSharedData.iPrimeBufNum == iSharedData.iMaxBuffers)
		iSharedData.iPrimeBufNum = 0;
	}

// Buffering thread AO (used in MultiThread mode only)
// This AO is owned by the COggPlayback object and runs in the buffering thread
CBufferingThreadAO::CBufferingThreadAO(TStreamingThreadData& aSharedData)
: CBufferingAO(EPriorityIdle, aSharedData)
	{
	}

CBufferingThreadAO::~CBufferingThreadAO()
	{
	}

void CBufferingThreadAO::StartListening()
	{
	// Set the AO active (wait for the streaming thread to make a buffering request)
	iStatus = KRequestPending;
	SetActive();
	}

// Prime the next buffer and self complete to prime more buffers if possible
void CBufferingThreadAO::RunL()
	{
	// Decode the next buffer
	PrimeNextBuffer();

	// Stop buffering if we have filled all the buffers (or if we have got to the last buffer)
	if ((iSharedData.NumBuffers() == iSharedData.iBuffersToUse) && !iSharedData.iLastBuffer)
		{
		// Listen for the next buffering request
		iStatus = KRequestPending;
		SetActive();

		iSharedData.iBufRequestInProgress = EFalse;
		}
	else if (!iSharedData.iLastBuffer)
		{
		// More buffers are required, so we self complete
		SelfComplete();
		}
	else
		iSharedData.iBufRequestInProgress = EFalse;
	}

// The buffering thread AO is owned by COggPlayback
// The COggPlayback object must ensure that the buffering AO is not cancelled if the streaming thread could be about to make a buffering request
// In other words, the streaming thread must be in a known, "inactive", state before the buffering thread AO is cancelled
void CBufferingThreadAO::DoCancel()
	{
	// If iSharedData.iBufRequestInProgress is true we will have self completed, so cancel that
	// If iSharedData.iBufRequestInProgress is false we are waiting for a request from the streaming thread so we must complete the request with KErrCancel
	if (iSharedData.iBufRequestInProgress)
		SelfCompleteCancel();
	else
		{
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrCancel);
		}

	iSharedData.iBufRequestInProgress = EFalse;
	}

// Streaming thread AO (It is used in SingleThread and MultiThread mode only)
// The streaming thread AO is owned by the playback engine and runs in the streaming thread
// aBufferFlushPending and aBufferingThreadPriority are references to the values held in the playback engine
CStreamingThreadAO::CStreamingThreadAO(TStreamingThreadData& aSharedData, TBool& aBufferFlushPending, TThreadPriority& aBufferingThreadPriority)
: CBufferingAO(EPriorityIdle, aSharedData), iBufferFlushPending(aBufferFlushPending), iBufferingThreadPriority(aBufferingThreadPriority)
	{
	}

CStreamingThreadAO::~CStreamingThreadAO()
	{
	}

void CStreamingThreadAO::StartBuffering()
	{
	// Fetch the pre buffers
	for (TInt i = 0 ; i<KPreBuffers ; i++)
		PrimeNextBuffer();

	// Start the AO
	ResumeBuffering();
	}

void CStreamingThreadAO::ResumeBuffering()
	{
	// Schedule the buffering (self complete)
	SelfComplete();
	}

// Prime the next buffer and self complete to prime more buffers if possible
// In MultiThread mode transfer the buffering to the buffering thread once there are KPrimeBuffers available
void CStreamingThreadAO::RunL()
	{
	// Decode the next buffer
	PrimeNextBuffer();

	// If we've got enough buffers and more buffering is required, transfer the buffering to the buffering thread
	if ((iSharedData.NumBuffers() > KPrimeBuffers) && !iSharedData.iLastBuffer && (iSharedData.iBufferingMode == EBufferThread))
		{
		// We can't transfer the buffering if there is a buffer flush pending, so simply return
		if (iBufferFlushPending)
			return;

		// Set the buffering request flag
		iSharedData.iBufRequestInProgress = ETrue;

		// Increase the buffering thread priority
		iBufferingThreadPriority = (iBufferingThreadPriority == EPriorityNormal) ? EPriorityMore : EPriorityAbsoluteHigh;
		iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);

		// Transfer buffering to the buffering thread
		TRequestStatus* status = &iSharedData.iBufferingThreadAO->iStatus;
		iSharedData.iBufferingThread.RequestComplete(status, KErrNone);
		}
	else if ((iSharedData.NumBuffers() < iSharedData.iBuffersToUse) && !iSharedData.iLastBuffer)
		{
		// Self complete (keep buffering)
		SelfComplete();
		}
	}

void CStreamingThreadAO::DoCancel()
	{
	// There is nothing to do here (the AO is either inactive or will have self completed)
	SelfCompleteCancel();
	}

// Streaming thread command handler AO
// The streaming thread command handler is owned by the COggPlayback object and runs in the streaming thread
// Accepts commands from the UI thread (the command thread)
// Executes commands in the streaming thread (the command handler thread)
CStreamingThreadCommandHandler::CStreamingThreadCommandHandler(RThread& aCommandThread, RThread& aStreamingThread, CThreadPanicHandler& aPanicHandler)
: CThreadCommandHandler(EPriorityHigh, aCommandThread, aStreamingThread, aPanicHandler)
{
}

CStreamingThreadCommandHandler::~CStreamingThreadCommandHandler()
{
}

void CStreamingThreadCommandHandler::Volume()
{
	SendCommand(EStreamingThreadVolume);
}

void CStreamingThreadCommandHandler::SetVolume()
{
	SendCommand(EStreamingThreadSetVolume);
}

TInt CStreamingThreadCommandHandler::SetAudioProperties()
{
	return SendCommand(EStreamingThreadSetAudioProperties);
}

void CStreamingThreadCommandHandler::StartStreaming()
{
	SendCommand(EStreamingThreadStartStreaming);
}

void CStreamingThreadCommandHandler::StopStreaming()
{
	SendCommand(EStreamingThreadStopStreaming);
}

void CStreamingThreadCommandHandler::SetBufferingMode()
{
	SendCommand(EStreamingThreadSetBufferingMode);
}

void CStreamingThreadCommandHandler::SetThreadPriority()
{
	SendCommand(EStreamingThreadSetThreadPriority);
}

void CStreamingThreadCommandHandler::Position()
{
	SendCommand(EStreamingThreadPosition);
}

TBool CStreamingThreadCommandHandler::PrepareToFlushBuffers()
{
	return SendCommand(EStreamingThreadPrepareToFlushBuffers) ? ETrue : EFalse;
}

TBool CStreamingThreadCommandHandler::FlushBuffers()
{
	return SendCommand(EStreamingThreadFlushBuffers) ? ETrue : EFalse;
}

// Wait for commands
// Commands are passed to the playback engine
void CStreamingThreadCommandHandler::ListenForCommands(CStreamingThreadPlaybackEngine& aPlaybackEngine)
{
	iPlaybackEngine = &aPlaybackEngine;
	CThreadCommandHandler::ListenForCommands();
}

// Handle commands
// The AO completion status is the command to execute
void CStreamingThreadCommandHandler::RunL()
{
	// Get the status
	TInt status = iStatus.Int();

	// Start listening again
	iStatus = KRequestPending;
	SetActive();

	// Dispatch commands to the playback engine
	TInt err = KErrNone;
	switch (status)
	{
	case EStreamingThreadSetAudioProperties:
		TRAP(err, iPlaybackEngine->SetAudioPropertiesL());
		break;

	case EStreamingThreadVolume:
		iPlaybackEngine->Volume();
		break;

	case EStreamingThreadSetVolume:
		iPlaybackEngine->SetVolume();
		break;

	case EStreamingThreadStartStreaming:
		iPlaybackEngine->StartStreaming();
		break;

	case EStreamingThreadStopStreaming:
		iPlaybackEngine->StopStreaming();
		break;

	case EStreamingThreadSetBufferingMode:
		iPlaybackEngine->SetBufferingMode();
		break;

	case EStreamingThreadSetThreadPriority:
		iPlaybackEngine->SetThreadPriority();
		break;

	case EStreamingThreadPosition:
		iPlaybackEngine->Position();
		break;

	case EStreamingThreadPrepareToFlushBuffers:
		err = iPlaybackEngine->PrepareToFlushBuffers() ? 1 : 0;
		break;

	case EStreamingThreadFlushBuffers:
		err = iPlaybackEngine->FlushBuffers() ? 1 : 0;
		break;

	case EThreadShutdown:
		iPlaybackEngine->Shutdown();
		CActiveScheduler::Stop();
		return;

	default:
		// Panic if we get an unkown command
		User::Panic(_L("STCL: RunL"), 0);
		break;
	}

	// Complete the request
	RequestComplete(err);
}

void CStreamingThreadCommandHandler::DoCancel()
{
	// Stop listening
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrCancel);
}

// Playback engine class
// Handles communication with the media server (CMdaAoudioOutputStream) and manages the audio buffering
CStreamingThreadPlaybackEngine* CStreamingThreadPlaybackEngine::NewLC(TStreamingThreadData& aSharedData)
{
	CStreamingThreadPlaybackEngine* self = new(ELeave) CStreamingThreadPlaybackEngine(aSharedData);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
}

CStreamingThreadPlaybackEngine::CStreamingThreadPlaybackEngine(TStreamingThreadData& aSharedData)
: iSharedData(aSharedData), iBufferingThreadPriority(EPriorityNormal)
{
	// Initialise buffer settings
	iSharedData.iBuffersToUse = KNoBuffers;
	iSharedData.iMaxBuffers = KNoBuffers;
	iMaxStreamBuffers = KNoBuffers;
	iBufferLowThreshold = KNoBuffers;
}

void CStreamingThreadPlaybackEngine::ConstructL()
	{
	// Open the stream
	iStream = CMdaAudioOutputStream::NewL(*this);
	iStream->Open(&iSettings);

	iStreamingThreadAO = new(ELeave) CStreamingThreadAO(iSharedData, iBufferFlushPending, iBufferingThreadPriority);
	iStreamingThreadAO->CreateTimerL();

	CActiveScheduler::Add(iStreamingThreadAO);
	}

CStreamingThreadPlaybackEngine::~CStreamingThreadPlaybackEngine()
{
	if (iStreamingThreadAO)
		iStreamingThreadAO->Cancel();

	iThread.Close();

	delete iStreamingThreadAO;
	delete iStream;
}

void CStreamingThreadPlaybackEngine::SetAudioPropertiesL()
{
	TMdaAudioDataSettings::TAudioCaps ac = TMdaAudioDataSettings::EChannelsMono;
	if (iSharedData.iChannels == 1)
		ac = TMdaAudioDataSettings::EChannelsMono;
	else
		ac = TMdaAudioDataSettings::EChannelsStereo;

	TMdaAudioDataSettings::TAudioCaps rt = TMdaAudioDataSettings::ESampleRate8000Hz;
	switch (iSharedData.iSampleRate)
	{
		case 8000:
			rt = TMdaAudioDataSettings::ESampleRate8000Hz;
			break;

		case 11025:
			rt = TMdaAudioDataSettings::ESampleRate11025Hz;
			break;

		case 16000:
			rt = TMdaAudioDataSettings::ESampleRate16000Hz;
			break;

		case 22050:
			rt = TMdaAudioDataSettings::ESampleRate22050Hz;
			break;

		case 32000:
			rt = TMdaAudioDataSettings::ESampleRate32000Hz;
			break;

		case 44100:
			rt = TMdaAudioDataSettings::ESampleRate44100Hz;
			break;

		case 48000:
			rt = TMdaAudioDataSettings::ESampleRate48000Hz;
			break;

		default:
			User::Leave(KErrNotSupported);
			break;
	}

	iStream->SetAudioPropertiesL(rt, ac);
}

void CStreamingThreadPlaybackEngine::Volume()
{
	iSharedData.iVolume = iStream->Volume();
}

void CStreamingThreadPlaybackEngine::SetVolume()
{
	TInt vol = iSharedData.iVolume;
	if (vol>KMaxVolume)
		vol = KMaxVolume;
	else if (vol<0)
		vol = 0;

	vol = (TInt) (((TReal) vol)/KMaxVolume * iMaxVolume);
	if (vol > iMaxVolume)
		vol = iMaxVolume;
	iStream->SetVolume(vol);
}

void CStreamingThreadPlaybackEngine::StartStreaming()
{
	if (iStreaming)
		User::Panic(_L("STPE: StartStream"), 0);

	// Init streaming values
	iBufNum = 0;
	iStreaming = ETrue;

	if (iSharedData.iBufferingMode == ENoBuffering)
	{
		// Fetch the first buffer
		iStreamingThreadAO->PrimeNextBuffer();

		// Stream the first buffer
		SendNextBuffer();
	}
	else
	{
		// Start the streaming thread AO
		iStreamingThreadAO->StartBuffering();

		// Stream the pre buffers
		for (TInt i = 0 ; i<KPreBuffers ; i++)
			SendNextBuffer();
	}
}

void CStreamingThreadPlaybackEngine::PauseStreaming()
{
	// Stop the stream, but don't reset the position
	StopStreaming(EFalse);
}

void CStreamingThreadPlaybackEngine::StopStreaming(TBool aResetPosition)
{
	if (iStreaming)
	{
		// Stop the stream
		iStream->Stop();

		// Stop the AOs
		iStreamingThreadAO->Cancel();

		// Reset streaming data
		iStreaming = EFalse;
		iStreamBuffers = 0;
		iBufferFlushPending = EFalse;

		// Reset shared data
		iSharedData.iNumBuffersRead = 0;
		iSharedData.iNumBuffersPlayed = 0;

		iSharedData.iPrimeBufNum = 0;
		iSharedData.iLastBuffer = NULL;

		iSharedData.iBufferBytesRead = 0;
		iSharedData.iBufferBytesPlayed = 0;

		// Reset the thread priorities
		if ((iBufferingThreadPriority != EPriorityNormal) && (iBufferingThreadPriority != EPriorityAbsoluteForeground))
		{
			iBufferingThreadPriority = (iBufferingThreadPriority == EPriorityMore) ? EPriorityNormal : EPriorityAbsoluteForeground;
			iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);
		}

		// Inform the streaming listener that the stream has stopped
		TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
		if (status->Int() == KRequestPending)
			iSharedData.iUIThread.RequestComplete(status, EPlayStopped);
	}

	if (aResetPosition)
		iSharedData.iTotalBufferBytes = 0;
}

void CStreamingThreadPlaybackEngine::SetBufferingMode()
{
	if (iStreaming)
		User::Panic(_L("STPE: SBM1"), 0);

	// Set the buffering values
	switch(iSharedData.iBufferingMode)
	{
		case ENoBuffering:
			iMaxStreamBuffers = KNoBuffers;
			iBufferLowThreshold = KNoBuffers;
			break;

		case EBufferStream:
			iMaxStreamBuffers = KSingleThreadStreamBuffers;
			iBufferLowThreshold = KSingleThreadLowThreshold;
			break;

		case EBufferThread:
			iMaxStreamBuffers = KMultiThreadStreamBuffers;
			iBufferLowThreshold = KMultiThreadLowThreshold;
			break;

		default:
			User::Panic(_L("STPE: SBM2"), 0);
			break;
	}
}

void CStreamingThreadPlaybackEngine::SetThreadPriority()
{
	// Set the thread priorities
	switch(iSharedData.iStreamingThreadPriority)
	{
		case ENormal:
			// Use normal (relative) thread priorities
			if (iBufferingThreadPriority == EPriorityAbsoluteForeground)
				iBufferingThreadPriority = EPriorityNormal;
			else if (iBufferingThreadPriority == EPriorityAbsoluteHigh)
				iBufferingThreadPriority = EPriorityMore;
			else
				break;

			iThread.SetPriority(EPriorityMore);
			iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);
			break;

		case EHigh:
			// Use high (absolute) thread priorities
			if (iBufferingThreadPriority == EPriorityNormal)
				iBufferingThreadPriority = EPriorityAbsoluteForeground;
			else if (iBufferingThreadPriority == EPriorityMore)
				iBufferingThreadPriority = EPriorityAbsoluteHigh;
			else
				break;

			iThread.SetPriority(EPriorityAbsoluteHigh);
			iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);
			break;

		default:
			User::Panic(_L("STPE: STP"), 0);
			break;
	}
}

void CStreamingThreadPlaybackEngine::Position()
{
	iSharedData.iStreamingPosition = iStream->Position();
}

TBool CStreamingThreadPlaybackEngine::PrepareToFlushBuffers()
{
	// Mark that a buffer flush is pending (this inhibits requests to the buffering thread)
	iBufferFlushPending = ETrue;

	// Return streaming status
	return iStreaming;
}

TBool CStreamingThreadPlaybackEngine::FlushBuffers()
{
	// Return the streaming status
	// (Get the value before we flush the buffers and possibly change it)
	TBool ret = iStreaming;

	// Reset the buffer flush pending flag 
	iBufferFlushPending = EFalse;

	// Reset the audio stream (move position / change volume gain)
	const TInt64 KConst500 = TInt64(500);
	switch (iSharedData.iFlushBufferEvent)
	{
		case EPlaybackPaused:
		case EBufferingModeChanged:
		{
			// Flush all of the data
			TInt bufferBytes = iSharedData.BufferBytes();
			if (bufferBytes)
			{
				iSharedData.iTotalBufferBytes-= bufferBytes;
				TInt64 newPositionMillisecs = (KConst500*iSharedData.iTotalBufferBytes)/TInt64(iSharedData.iSampleRate*iSharedData.iChannels);
				iSharedData.iOggPlayback.SetDecoderPosition(newPositionMillisecs);
			}

			// Pause the stream
			PauseStreaming();
			break;
		}

		case EVolumeGainChanged:
		{
			// Calculate how much data needs to be flushed
			// Don't flush buffers that are already in the stream and leave another KPreBuffers unflushed
			TInt availBuffers = iSharedData.NumBuffers() - iStreamBuffers;
			TBool buffersToFlush = availBuffers>KPreBuffers;
			TInt numFlushBuffers = (buffersToFlush) ? availBuffers-KPreBuffers : 0;
			iSharedData.iNumBuffersRead -= numFlushBuffers;

			TInt bytesFlushed = 0;
			if (buffersToFlush)
			{
				TInt bufNum = iBufNum + KPreBuffers;
				if (bufNum>=iSharedData.iMaxBuffers)
					bufNum -= iSharedData.iMaxBuffers;
				iSharedData.iPrimeBufNum = bufNum;

				for (TInt i = 0 ; i<numFlushBuffers ; i++)
				{
					bytesFlushed += iSharedData.iOggPlayback.iBuffer[bufNum]->Length();
					bufNum++;
					if (bufNum == iSharedData.iMaxBuffers)
						bufNum = 0;
				}
				iSharedData.iBufferBytesRead-= bytesFlushed;
			}

			// Reset the decoder position
			if (bytesFlushed)
			{
				iSharedData.iTotalBufferBytes-= bytesFlushed;
				TInt64 newPositionMillisecs = (KConst500*iSharedData.iTotalBufferBytes)/TInt64(iSharedData.iSampleRate*iSharedData.iChannels);
				iSharedData.iOggPlayback.SetDecoderPosition(newPositionMillisecs);
			}

			// Set the new volume gain (and carry on as if nothing had happened)
			iSharedData.iOggPlayback.SetSampleRateConverterVolumeGain(iSharedData.iNewGain);
			break;
		}

		case EPositionChanged:
			// Recalculate the number of buffer bytes
			iSharedData.iTotalBufferBytes = (iSharedData.iNewPosition*TInt64(iSharedData.iSampleRate*iSharedData.iChannels))/KConst500;

			// Set the new position
			iSharedData.iOggPlayback.SetDecoderPosition(iSharedData.iNewPosition);

			// Pause the stream
			PauseStreaming();
			break;

		default:
			User::Panic(_L("STPE: Flush"), 0);		
	}

	// Start the streaming thread AO
	if (iStreaming && !iStreamingThreadAO->IsActive() && (iSharedData.iBufferingMode != ENoBuffering))
	{
		// Reset the buffering thread priority
		if ((iBufferingThreadPriority != EPriorityNormal) && (iBufferingThreadPriority != EPriorityAbsoluteForeground))
		{
			iBufferingThreadPriority = (iBufferingThreadPriority == EPriorityMore) ? EPriorityNormal : EPriorityAbsoluteForeground;
			iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);
		}

		iStreamingThreadAO->ResumeBuffering();
	}

	return ret;
}

void CStreamingThreadPlaybackEngine::Shutdown()
{
	// We can't shutdown if we are streaming, so panic
	if (iStreaming)
		User::Panic(_L("STPE: Shutdown"), 0);
}

// Send the next audio buffer to the stream
// iBufNum holds the index of the next buffer
void CStreamingThreadPlaybackEngine::SendNextBuffer()
{
	TRAPD(err, iStream->WriteL(*iSharedData.iOggPlayback.iBuffer[iBufNum]));
	if (err != KErrNone)
	{
		// If an error occurs notify the UI thread
		TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
		if (status->Int() == KRequestPending)
			iSharedData.iUIThread.RequestComplete(status, err);

		return;
	}

	// Increment the number of stream buffers (we have one more now)
	iStreamBuffers++;

	// Move on to the next buffer
	iBufNum++;
	if (iBufNum == iSharedData.iMaxBuffers)
		iBufNum = 0;
}

void CStreamingThreadPlaybackEngine::MaoscOpenComplete(TInt aErr)
{ 
	TRACEF(COggLog::VA(_L("MaoscOpenComplete: %d"), aErr));
	if (aErr != KErrNone)
	{
		// Notify the UI thread
		TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
		iSharedData.iUIThread.RequestComplete(status, aErr);
		return;
	}

	// Determine the maximum volume
	iMaxVolume = iStream->MaxVolume();

	// Set our audio priority
	iStream->SetPriority(KAudioPriority, EMdaPriorityPreferenceTimeAndQuality);

	// Notify the UI thread
	TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
	iSharedData.iUIThread.RequestComplete(status, KErrNone);
}

// MaoscBufferCopied does all of the work to manage the buffering
void CStreamingThreadPlaybackEngine::MaoscBufferCopied(TInt aErr, const TDesC8& aBuffer)
	{
	// Error codes:
	// KErrCancel      -3  (not sure when this happens, but ignore for now)
	// KErrUnderflow  -10  (ignore this, do as if nothing has happend, and live happily ever after)
	// KErrDied       -13  (interrupted by higher priority)
	// KErrInUse      -14  (the sound device was stolen from us)
	// KErrAbort      -39  (stream was stopped before this buffer was copied)

	// Debug trace, if error
	if (aErr != KErrNone)
		{ TRACEF(COggLog::VA(_L("MaoscBufferCopied:%d"), aErr)); }
		
	// Decrement the stream buffers (we have one less now)
	iStreamBuffers--;

	// Adjust the number of bytes we have in buffers
	iSharedData.iBufferBytesPlayed += aBuffer.Length();

	// Increment the total number of buffers played
	iSharedData.iNumBuffersPlayed++;

	// Ignore most of the error codes
	if ((aErr == KErrAbort) || (aErr == KErrInUse) || (aErr == KErrDied)  || (aErr == KErrCancel))
		return;

	if ((aErr != KErrNone) && (aErr != KErrUnderflow))
		{
		// Notify the UI thread (unknown error)
		TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
		if (status->Int() == KRequestPending)
			iSharedData.iUIThread.RequestComplete(status, aErr);

		return;
		}

	// If we have reached the last buffer, notify the UI thread
	if (iSharedData.iLastBuffer == &aBuffer)
		{
		// Notify the UI thread that we have copied the last buffer to the stream
		TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
		iSharedData.iUIThread.RequestComplete(status, ELastBufferCopied);
		return;
		}

	// Dynamically adjust the buffering thread priority
	TBool streamingThreadActive = iStreamingThreadAO->IsActive();
	if (iSharedData.iBufferingMode == EBufferThread)
		{
		TInt numBuffers = iSharedData.NumBuffers();
		if (numBuffers < KBufferThreadLowThreshold)
			{
			if ((iBufferingThreadPriority != EPriorityMore) && (iBufferingThreadPriority != EPriorityAbsoluteHigh))
				{
				if (!streamingThreadActive && !iSharedData.iLastBuffer)
					{
					iBufferingThreadPriority = (iBufferingThreadPriority == EPriorityNormal) ? EPriorityMore : EPriorityAbsoluteHigh;
					iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);
					}
				}
			}
		else if (numBuffers > KBufferThreadHighThreshold)
			{
			if ((iBufferingThreadPriority != EPriorityNormal) && (iBufferingThreadPriority != EPriorityAbsoluteForeground))
				{
				iBufferingThreadPriority = (iBufferingThreadPriority == EPriorityMore) ? EPriorityNormal : EPriorityAbsoluteForeground;
				iSharedData.iBufferingThread.SetPriority(iBufferingThreadPriority);
				}
			}
		}
 
	// Ignore underflow if there are stream buffers left
	if ((aErr == KErrUnderflow) && iStreamBuffers)
		return;

	// Check if we have available buffers
	TInt availBuffers = iSharedData.NumBuffers() - iStreamBuffers;
	if (!availBuffers)
		{
		// If the streaming thread is doing the buffering, prime another buffer now 
		if ((streamingThreadActive || (iSharedData.iBufferingMode == ENoBuffering)) && (aErr != KErrUnderflow))
			iStreamingThreadAO->PrimeNextBuffer();
		else
			{
			// If we still have stream buffers, ignore the fact there are no buffers left
			if (iStreamBuffers)
				return;

			// Otherwise try to re-start playback
			TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
			iSharedData.iUIThread.RequestComplete(status, EPlayUnderflow);
			return;
			}
		}

	// Stream the next buffer
	SendNextBuffer();

	availBuffers = iSharedData.NumBuffers() - iStreamBuffers;
	if ((iStreamBuffers<iMaxStreamBuffers) && availBuffers)
		{
		// Try to catch up by writing another buffer
		SendNextBuffer();
		}

	// If the number of buffers has dropped below a certain threshold, request the buffering thread to start buffering
	if (iSharedData.NumBuffers()<iBufferLowThreshold) 
		{
		// Nothing to do if the streaming thread is already buffering
		// If a buffer flush is pending we don't want to start any more buffering
		if (streamingThreadActive || iBufferFlushPending)
			return;

		switch (iSharedData.iBufferingMode)
			{
			case ENoBuffering:
				break;

			case EBufferStream:
				if (!iSharedData.iLastBuffer)
					iStreamingThreadAO->ResumeBuffering();
				break;

			case EBufferThread:
				{
				// Nothing to do if the buffering thread is already buffering
				if (iSharedData.iBufRequestInProgress)
					break;

				// Nothing to do if we have reached eof
				if (iSharedData.iLastBuffer)
					break;

				// Issue a buffering request
				iSharedData.iBufRequestInProgress = ETrue;
				TRequestStatus* status = &iSharedData.iBufferingThreadAO->iStatus;
				iSharedData.iBufferingThread.RequestComplete(status, KErrNone);
				}
				break;

			default:
				User::Panic(_L("STPE: MaoscBC"), 0);
				break;
			}
		}
	}

// Handle play complete
// There are five possibilities possibilities:
// 1. Playback is stopped by the user
// In this case there is nothing to do, the UI thread will handle everything

// 2. KErrUnderflow
// MaoscBufferCopied will have scheduled the restart timer, so we ignore KErrUnderflow

// 3. KErrDied
// I'm not sure what causes this, but we simply inform the UI thread
// The UI thread will make a number of attempts to restart playback
// (This behaviour is the same as OggPlay 1.07 and earlier)

// 4. KErrInUse
// This is caused when another application takes the sound device from us (real player does this)
// OggPlay will stop playback and display a message informing the user.
// OggPlay will attempt to restart playback when the user presses "back"

// 5. Unknown error
// This is handled in the same way as KErrInUse, except that the UI thread will display the error code
void CStreamingThreadPlaybackEngine::MaoscPlayComplete(TInt aErr)
{
	// Error codes:
	// KErrNone         0 (stopped by user)
	// KErrCancel      -3 (also stopped by user)
	// KErrUnderflow  -10 (ran out of audio, EOF or lack of cpu power)
	// KErrDied       -13 (interrupted by higher priority)
	// KErrInUse      -14 (the sound device was stolen from us)

	// Trace the reason for the play complete
	TRACEF(COggLog::VA(_L("MaoscPlayComplete:%d"), aErr));
  
	// Stopped by the user
	if ((aErr == KErrNone) || (aErr == KErrCancel))
		return;

	// Ignore underflow, underflow is handled by MaoscBufferCopied
	if (aErr == KErrUnderflow)
		return;

	// Expect KErrDied if interrupted
	if (aErr == KErrDied)
	{
		TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
		if (status->Int() == KRequestPending)
			iSharedData.iUIThread.RequestComplete(status, EPlayInterrupted);

		return;
	}

	// Unknown error (or KErrInUse)
	TRequestStatus* status = &iSharedData.iStreamingThreadListener->iStatus;
	if (status->Int() == KRequestPending)
		iSharedData.iUIThread.RequestComplete(status, aErr);
}


// Streaming thread listener AO
// This AO is owned by the COggPlayback object and runs in the UI thread
// It handles events from the audio stream (CMdaAudioOutputStream events from the streaming thread)
CStreamingThreadListener::CStreamingThreadListener(COggPlayback& aOggPlayback, TStreamingThreadData& aSharedData)
: CActive(EPriorityHigh), iOggPlayback(aOggPlayback), iSharedData(aSharedData), iListeningState(EListeningForOpenComplete)
{
	CActiveScheduler::Add(this);
}

void CStreamingThreadListener::StartListening()
{
	// Start listening
	iStatus = KRequestPending;
	SetActive();
}

CStreamingThreadListener::~CStreamingThreadListener()
{
}

void CStreamingThreadListener::RunL()
{
	// Get the status
	TInt status = iStatus.Int();

	// Handle the message
	switch (iListeningState)
	{
	case EListeningForOpenComplete:
		iOggPlayback.NotifyOpenComplete(status);
		iListeningState = EListeningForStreamingStatus;
		break;

	case EListeningForStreamingStatus:
		iOggPlayback.NotifyStreamingStatus(status);
		break;
	}
}

void CStreamingThreadListener::DoCancel()
	{
	// Nothing to do
	// (the streaming thread will have completed the request)
	}
