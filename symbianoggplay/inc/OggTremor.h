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

#include <e32des8.h>
#include <eikenv.h>

#include "libmad\mad.h"

#include "OggMsgEnv.h"
#include "OggHelperFcts.h"
#include "OggRateConvert.h"
#include "TremorDecoder.h"
#include "OggAbsPlayback.h"
#include "OggMessageQueue.h"
#include "OggMultiThread.h"

// No buffering buffers
const TInt KNoBuffers = 1;

#if defined(SERIES60V3)
// Total number of buffers
const TInt KSingleThreadBuffers = 16;
const TInt KMultiThreadBuffers = 64;

// The actual number of buffers to use
const TInt KSingleThreadBuffersToUse = 16;
const TInt KMultiThreadBuffersToUse = 64;
#else
// Total number of buffers
const TInt KSingleThreadBuffers = 16;
const TInt KMultiThreadBuffers = 64;

// The actual number of buffers to use
// Set to one less than the total
// This is a workaround for a bug in Symbian's buffering code
// MaoscBufferCopied gets called before it has actually copied the buffer
const TInt KSingleThreadBuffersToUse = 15;
const TInt KMultiThreadBuffersToUse = 63;
#endif

// Number of buffers to fetch before starting playback (one seems to be enough)
const TInt KPreBuffers = 1;

// Number of buffers to keep in the audio stream (provides a small amount of underflow protection)
const TInt KSingleThreadStreamBuffers = 3;
const TInt KMultiThreadStreamBuffers = 5;

// Number of buffers to fetch before transferring the buffering to the buffering thread
const TInt KPrimeBuffers = 12;

// Number of buffers required to cause the thread priority of the buffering thread to be reduced
// The buffering thread priority will remain at a reduced level as long as the number of buffers remains above KBufferLowThreshold
const TInt KBufferThreadHighThreshold = 32;

// Number of buffers required to cause the thread priority of the buffering thread to be increased
// The buffering thread priority will remain at an increased level until the number of buffers reaches KBufferHighThreshold
const TInt KBufferThreadLowThreshold = 12;

// The number of buffers at which to resume buffering
// #define LAZY_BUFFERING
#if defined(LAZY_BUFFERING)
const TInt KSingleThreadLowThreshold = 5;
const TInt KMultiThreadLowThreshold = 20;
#else
const TInt KSingleThreadLowThreshold = KSingleThreadBuffersToUse;
const TInt KMultiThreadLowThreshold = KMultiThreadBuffersToUse;
#endif

#if defined(SERIES60)
  const TInt KAudioPriority = 70; // S60 audio players typically uses 60-75.
#else
  const TInt KAudioPriority = EMdaPriorityMax;
#endif

// Buffer sizes to use
// (approx. 0.1s of audio per buffer)
const TInt KBufferSize48K = 16384;
const TInt KBufferSize32K = 12288;
const TInt KBufferSize22K = 8192;
const TInt KBufferSize16K = 6144;
const TInt KBufferSize11K = 4096;

// frequency of canvas Refresh() for graphics updating - need to get FreqBin
const TInt KOggControlFreq = 14;

// Stream start delays
// Avoid buffer underflows at stream startup and stop multiple starts when the user moves quickly between tracks
const TInt KStreamStartDelay = 100000;

// Stream stop delay
// OggPlay doesn't wait for a play complete message so wait a little while before "stopping"
const TInt KStreamStopDelay = 100000;

// Stream restart delay (the time to wait after running out of buffers before attempting to play again)
const TInt KStreamRestartDelay = 100000;

// Uid for "tremor controller" (the MTP engine)
const TUid KOggPlayInternalController = { 0x10202030 };


#include "mdaaudiooutputstream.h"
#include "mda/common/audio.h"
#include "OggThreadClasses.h"

class TStreamingThreadData;
class CStreamingThreadCommandHandler;
class CBufferingThreadAO;
class CStreamingThreadListener;
class COggPlayback : public CAbsPlayback, public MOggSampleRateFillBuffer, public MThreadPanicObserver
	{
public: 
	COggPlayback(COggMsgEnv* aEnv, CPluginSupportedList& aPluginSupportedList, MPlaybackObserver* aObserver, TInt aMachineUid);
	~COggPlayback();

	// From CAbsPlayback
	void ConstructL();

	TInt Info(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver);
	TInt FullInfo(const TDesC& aFileName, TUid aControllerUid, MFileInfoObserver& aFileInfoObserver);
	void InfoCancel();

	TInt Open(const TOggSource& aSource, TUid aControllerUid);
	void OpenComplete(TInt aErr);

	void Pause();
	void Play(); 
	void Resume();
	void Stop();

	void SetVolume(TInt aVol);
	void SetPosition(TInt64 aPos);
	TBool Seekable();

	TInt64 Position();
	TInt Volume();

	const TInt32* GetFrequencyBins();


	// From MOggSampleRateFillBuffer
	TInt GetNewSamples(TDes8 &aBuffer);

	// New functions (Multi thread playback fns.)
	TBool PrimeBuffer(HBufC8& buf, TBool& aNewSection);

	TInt SetBufferingMode(TBufferingMode aNewBufferingMode);
	void SetThreadPriority(TStreamingThreadPriority aNewThreadPriority);
	void SetVolumeGain(TGainType aGain);
	void SetMp3Dithering(TBool aDithering);

	TBool FlushBuffers(TFlushBufferEvent aFlushBufferEvent);
	TBool FlushBuffers(TInt64 aNewPosition);
	void FlushBuffers(TGainType aNewGain);

	void SetDecoderPosition(TInt64 aPos);
	void SetSampleRateConverterVolumeGain(TGainType aGain);

	void NotifyStreamOpen(TInt aErr);
	TBool NotifyStreamingStatus(TStreamingEvent aEvent, TInt aErr);

	void PrepareToSetPosition();
	void PrepareToPlay();
	void ThreadRelease();

private:
	TBool GetNextLowerRate(TInt& usedRate, TMdaAudioDataSettings::TAudioCaps& rt);
	TInt SetAudioCaps(TInt theChannels, TInt theRate);

	void SamplingRateSupportedMessage(TBool aConvertRate, TInt aRate, TBool aConvertChannel, TInt aNbOfChannels);
	MDecoder* GetDecoderL(const TDesC& aFileName, COggHttpSource* aHttpSource);

	void StartAudioStreamingCallBack();
	static TInt StartAudioStreamingCallBack(TAny* aPtr);

	void RestartAudioStreamingCallBack();
	static TInt RestartAudioStreamingCallBack(TAny* aPtr);

	void StopAudioStreamingCallBack();
	static TInt StopAudioStreamingCallBack(TAny* aPtr);

	void CancelTimers();

	TInt SetAudioProperties(TInt aSampleRate, TInt aChannels);
	void StartStreaming();
	void StopStreaming();

	TInt BufferingModeChanged();

	static TInt StreamingThread(TAny* aThreadData);
	static void StreamingThreadL(TStreamingThreadData& aSharedData);

	// From MThreadPanicHandler 
	void HandleThreadPanic(RThread& aPanicThread, TInt aErr);

public:
	HBufC8* iBuffer[KMultiThreadBuffers];

private:
	class TFreqBins 
		{
	public:
		TInt64 iPcmByte;
		TInt32 iFreqCoefs[KNumberOfFreqBins];
		};

	COggTimer* iStartAudioStreamingTimer;
	COggTimer* iRestartAudioStreamingTimer;
	COggTimer* iStopAudioStreamingTimer;

	COggMsgEnv* iEnv;
	COggSampleRateConverter *iOggSampleRateConverter;

	TFreqBins iFreqArray[KFreqArrayLength];
	TInt iLastFreqArrayIdx;

	TBool iRequestingFrequencyBins;
	TInt iBytesSinceLastFrequencyBin;
	TInt iMaxBytesBetweenFrequencyBins;
	TInt64 iLastPlayTotalBytes;
	TInt64 iSectionStartBytes;

	// Communication with the decoder
	MDecoder* iDecoder;
	TBool iEof;
	RFs iFs;

	RThread iThread;
	RThread iUIThread;
	RThread iBufferingThread;
	RThread iStreamingThread;

	CThreadPanicHandler* iStreamingThreadPanicHandler;
	CStreamingThreadCommandHandler* iStreamingThreadCommandHandler;
	CBufferingThreadAO* iBufferingThreadAO;
	CStreamingThreadListener* iStreamingThreadListener;

	TBool iStreamingThreadRunning;
	TStreamingThreadData iSharedData;

	TOggFileInfo iInfoFileInfo;

	// Machine uid (for identifying the phone model)
	TInt iMachineUid;

	TBool iMp3Dithering;
	TAudioDither iMp3LeftDither;
	TAudioDither iMp3RightDither;
	mad_fixed_t iMp3Random;

	TInt iSection;
	TBool iNewSection;
	TBool iRestartPending;
	};
    
#endif // _OggTremor_h
