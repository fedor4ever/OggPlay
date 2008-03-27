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

#ifndef _OggMulti_h
#define _OggMulti_h

#include <e32base.h>
#include <e32std.h>
#include <f32file.h>
#include <mdaaudiooutputstream.h>
#include <mda\common\audio.h>
#include "OggThreadClasses.h"
#include "OggMTFile.h"
#include "OggHttpSource.h"
#include "OggMessageQueue.h"

// Thread commands for the Streaming thread
enum TStreamingThreadCommand
	{
	// Set the audio properties
	EStreamingThreadSetAudioProperties = EThreadShutdown+1,

	// Set the volume
	EStreamingThreadSetVolume,

	// Return the current volume
	EStreamingThreadVolume,

	// Start streaming (start playback)
	EStreamingThreadStartStreaming,

	// Stop streaming (stop playback)
	EStreamingThreadStopStreaming,

	// Set the buffering mode
	EStreamingThreadSetBufferingMode,

	// Return the current stream position
	EStreamingThreadStreamPosition,

	// Prepare to flush buffers
	EStreamingThreadPrepareToFlushBuffers,

	// Flush audio buffers
	EStreamingThreadFlushBuffers,

	// Set the thread priorities (normal or high)
	EStreamingThreadSetThreadPriority,

	// Get the streaming position
	EStreamingThreadPosition,

	// Open a file or stream
	EStreamingThreadSourceOpen,

	// Close a file or stream
	EStreamingThreadSourceClose,

	// Read from a file or stream
	EStreamingThreadSourceReadBuf,

	// Read from a file or stream
	EStreamingThreadSourceRead,

	// Request read from a file or stream
	EStreamingThreadSourceReadRequest,

	// Cancel reading
	EStreamingThreadSourceReadCancel,

	// Seek a file
	EStreamingThreadFileSeek,

	// Size a file
	EStreamingThreadFileSize
	};

// Buffering mode to use
enum TBufferingMode
	{
	// Multi thread playback (currently one thread buffers, the other one streams audio)
	EBufferThread,

	// Single thread playback (one thread buffers and streams audio)
	EBufferStream,

	// No buffering (one thread streams and decodes the next buffer only)
	ENoBuffering
	};

// Flush buffer event (the event that has made flushing the buffers necessary)
enum TFlushBufferEvent
	{
	// The user has changed the volume gain
	EVolumeGainChanged,

	// The user has changed the buffering mode
	EBufferingModeChanged,

	// The user has paused playback
	EPlaybackPaused,

	// The user has changed the position (ff/rw)
	EPositionChanged
	};

// Thread priorities to use (relative/normal or absolute/high)
enum TStreamingThreadPriority
	{
	// Use normal thread priorities (EPriorityNormal and EPriorityMore)
	ENormal,

	// Use high thread priorities (EPriorityAbsoluteForeground and EPriorityAbsoluteHigh)
	EHigh
	};

#if defined(BUFFER_FLOW_DEBUG)
class TBufRecord
	{
public:
	TBufRecord(TInt aBufNum, TInt aType)
	: iBufNum(aBufNum), iType(aType)
	{ }

public:
	TInt iBufNum;
	TInt iType;
	};
#endif

// Shared data between the UI, buffering and streaming threads
// Owned by the UI thread (part of COggPlayback)
class CBufferFillerAO;
class CBufferingThreadAO;
class CStreamingThreadAO;
class CStreamingThreadCommandHandler;
class CStreamingThreadSourceHandler;
class CStreamingThreadListener;
class COggPlayback;
class TStreamingThreadData
	{
public:
	TStreamingThreadData(COggPlayback& aOggPlayback, RThread& aUIThread, RThread& aBufferingThread);

	inline TInt NumBuffers()
	{ return iNumBuffersRead - iNumBuffersPlayed; }

	inline TInt BufferBytes()
	{ return iBufferBytesRead - iBufferBytesPlayed; }

public:
	// Ref to the COggPlayback object (provides access to the sample rate converter for decoding ogg data)
	COggPlayback& iOggPlayback;

	// Refs to the UI and buffering threads (used by the streaming thread)
	RThread& iUIThread;
	RThread& iBufferingThread;

	// Inter thread communication
	// The AOs are owned by the UI thread
	// The buffering thread AO runs in the buffering thread
	// The streaming thread listener runs in the UI thread
	// The streaming thread command handler runs in the streaming thread
	CBufferingThreadAO* iBufferingThreadAO;
	CStreamingThreadAO* iStreamingThreadAO;
	CStreamingThreadListener* iStreamingThreadListener;
	CStreamingThreadCommandHandler* iStreamingThreadCommandHandler;

	// Access to the source reader from the streaming thread
	// (Access from the buffering thread is via the command handler)
	CStreamingThreadSourceHandler* iStreamingThreadSourceHandler;

	// Shared data
	// Used by the buffering AOs and by the playback engine

	// The numbers of buffers to use
	// (this shouldn't be required, there is a bug in some versions of CMdaAudioOutputStream)
	TInt iBuffersToUse;

	// The max number of buffers (depends upon the buffering mode)
	TInt iMaxBuffers;

	// The number of decoded buffers (increases when a buffer is deocded, decreases when MaoscBufferCopied is called)
	TInt iNumBuffersRead;
	TInt iNumBuffersPlayed;

	// Index of the next buffer to be decoded 
	TInt iPrimeBufNum;

	// Flag used in multi thread mode to indicate that the buffering thread's AO is active
	TBool iBufRequestInProgress;

	// Flag used to determine if a buffer fill is in progress
	TBool iBufferFillInProgress;

	// Flag used to determine if the source reader is waiting for data (only applicable for streams)
	CBufferFillerAO* iBufferFillWaitingForData;

	// Pointer to the new section buffer (set when a new section is reached)
	TDesC8* iNewSection;

	// Pointer to the last buffer (set when eof is reached)
	TDesC8* iLastBuffer;

	// The number of audio PCM bytes currently in buffers (changes whenever iNumBuffers changes)
	TInt iBufferBytesRead;
	TInt iBufferBytesPlayed;

	// The number of audio PCM bytes currently read from the file (used to work out the position)
	TInt64 iTotalBufferBytes;

	// Machine uid (for identifying the phone model)
	TInt iMachineUid;

	// Message queue for streaming messages
	ROggMessageQueue* iStreamingMessageQueue;

	// Shared data for commands (These should be a union, really (TO DO))
	// Sample Rate and channels (for SetAudioProperties)
	TInt iSampleRate;
	TInt iChannels;

	// Volume (for Volume() and SetVolume())
	TInt iVolume;

	// Flush buffer data
	TFlushBufferEvent iFlushBufferEvent;
	TInt64 iNewPosition;
	TGainType iNewGain;

	// The buffering mode (used by SetBufferingMode()) 
	TBufferingMode iBufferingMode;

	// The thread priorities to use (used by SetThreadPriority())
	TStreamingThreadPriority iStreamingThreadPriority;

	// Streaming position
	TTimeIntervalMicroSeconds iStreamingPosition;

	// Source name
	const TDesC* iSourceName;

	// Source data
	TMTSourceData* iSourceData;
	TMTSourceData* iDecoderSourceData;

	// Source size
	TInt iSourceSize;

	// Source seek mode
	TSeek iSourceSeekMode;

	// Source seek pos
	TInt iSourceSeekPos;

	// File buf
	TDes8* iSourceBuf;

	// Http source (valid when opening a stream)
	COggHttpSource* iSourceHttp;

#if defined(BUFFER_FLOW_DEBUG)
	RArray<TBufRecord> iDebugBuf;
#endif
	};

class MPrimeNextBufferObserver
	{
public:
	virtual void PrimeNextBufferCallBack(TInt aNumBuffers) = 0;
	};

class CBufferFillerAO : public CActive
	{
public:
	CBufferFillerAO(TStreamingThreadData& aSharedData);
	void PrimeNextBuffer(TInt aNumBuffers, MPrimeNextBufferObserver* aObserver);
	void DataReceived();

	// Overload CActive::Cancel()
	void Cancel();

	// From CActive
	void RunL();
	void DoCancel();

private:
	void SelfComplete();

private:
	TInt iBufferBytes;
	TInt iBufferBytesRequired;

	TBool iNextBuffer;
	TInt iNumBuffersFilled;
	TInt iNumBuffersToFill;
	
	MPrimeNextBufferObserver* iObserver;
	TStreamingThreadData& iSharedData;
	};

	
// Base class for the buffering AOs
class CBufferingAO : public CActive
	{
public:
	CBufferingAO(TInt aPriority, TStreamingThreadData& aSharedData);
	~CBufferingAO();
	void ConstructL();

	void PrimeNextBuffer(TInt aNumBuffers, MPrimeNextBufferObserver* aObserver);
	void DataReceived();

	// Overload CActive IsActive() && Cancel()
	TBool IsActive();
	void Cancel();

protected:
	void SelfComplete();
	void SelfCompleteCancel();

protected:
	TStreamingThreadData& iSharedData;

private:
	RTimer iTimer;
	CBufferFillerAO* iBufferFillerAO;
	};

// Buffering thread active object
// Performs buffering when requested to by the streaming thread
class CBufferingThreadAO : public CBufferingAO, public MPrimeNextBufferObserver
	{
public:
	CBufferingThreadAO(TStreamingThreadData& aSharedData);
	~CBufferingThreadAO();

	void StartListening();

	// From CActive
	void RunL();
	void DoCancel();

	// From MPrimeNextBufferObserver
	void PrimeNextBufferCallBack(TInt aNumBuffers);
	};

// Streaming thread active object
// Responsible for buffering in single thread mode
// Responsible for buffering when play starts in multi thread mode
// (Once enough buffers have been decoded, buffering is transfered to the buffering thread)
class CStreamingThreadAO : public CBufferingAO, public MPrimeNextBufferObserver
	{
public:
	CStreamingThreadAO(TStreamingThreadData& aSharedData, TBool& aBufferFlushPending, TThreadPriority& aBufferingThreadPriority);
	~CStreamingThreadAO();

	void StartBuffering();
	void ResumeBuffering();

	// From CActive
	void RunL();
	void DoCancel();

	// From MPrimeNextBufferObserver
	void PrimeNextBufferCallBack(TInt aNumBuffers);

private:
	TBool& iBufferFlushPending;
	TThreadPriority& iBufferingThreadPriority;
	};

// Command handler for the streaming thread (Active object)
// Command functions are called by the UI thread (RunL executes them in the streaming thread)
class CStreamingThreadPlaybackEngine;
class CStreamingThreadSourceReader;
class CStreamingThreadCommandHandler : public CThreadCommandHandler, public MMTSourceHandler
	{
public:
	CStreamingThreadCommandHandler(RThread& aCommandThread, RThread& aStreamingThread, CThreadPanicHandler& aPanicHandler, TStreamingThreadData& aSharedData);
	~CStreamingThreadCommandHandler();

	void ListenForCommands(CStreamingThreadPlaybackEngine& aPlaybackEngine, CStreamingThreadSourceReader& aSourceReader);

	void Volume();
	void SetVolume();
	TInt SetAudioProperties();

	void StartStreaming();
	void StopStreaming();

	void SetBufferingMode();
	void SetThreadPriority();

	TBool PrepareToFlushBuffers();
	TBool FlushBuffers();

	void Position();

	// From MMTSourceHandler
	TInt OpenSource(const TDesC& aSourceName, COggHttpSource* aHttpSource, TMTSourceData& aSourceData);
	void SourceClose(TMTSourceData& aSourceData);

	TInt Read(TMTSourceData& aSourceData, TDes8& aBuf);

	TInt Read(TMTSourceData& aSourceData);
	void ReadRequest(TMTSourceData& aSourceData);
	void ReadCancel(TMTSourceData& aSourceData);

	TInt FileSeek(TSeek aMode, TInt &aPos, TMTSourceData& aSourceData);
	TInt FileSize(TInt& aSize, TMTSourceData& aSourceData);

protected:
	// From CActive
	void RunL();
	void DoCancel();

private:
	TStreamingThreadData& iSharedData;

	CStreamingThreadPlaybackEngine* iPlaybackEngine;
	CStreamingThreadSourceReader* iSourceReader;
	};

class CStreamingThreadSourceHandler : public CBase, public MMTSourceHandler
	{
public:
	CStreamingThreadSourceHandler(TStreamingThreadData& aSharedData, CStreamingThreadSourceReader& aSourceReader);
	~CStreamingThreadSourceHandler();

	// From MMTSourceHandler
	TInt OpenSource(const TDesC& aSourceName, COggHttpSource* aHttpSource, TMTSourceData& aSourceData);
	void SourceClose(TMTSourceData& aSourceData);

	TInt Read(TMTSourceData& aSourceData, TDes8& aBuf);

	TInt Read(TMTSourceData& aSourceData);
	void ReadRequest(TMTSourceData& aSourceData);
	void ReadCancel(TMTSourceData& aSourceData);

	TInt FileSeek(TSeek aMode, TInt &aPos, TMTSourceData& aSourceData);
	TInt FileSize(TInt& aSize, TMTSourceData& aSourceData);

private:
	TStreamingThreadData& iSharedData;
	CStreamingThreadSourceReader& iSourceReader;
	};

#if defined(PROFILE_PERF)
class CProfilePerfAO : public CActive
	{
public:
	CProfilePerfAO(CStreamingThreadPlaybackEngine& aEngine);
	~CProfilePerfAO();

	void SendNextBuffer(TDesC8* aNextBuffer);

	// From CActive
	void RunL();
	void DoCancel();

private:
	CStreamingThreadPlaybackEngine& iEngine;
	RPointerArray<TDesC8> iBuffers;
	};
#endif

// Source
class COggMTSource : public CActive
	{
public:
	enum TMTSourceType { EMTFile, EMTHttpSource };

public:
	COggMTSource(RFile* aFile, TMTSourceData& aSourceData);
	COggMTSource(COggHttpSource* aHttpSource, TMTSourceData& aSourceData);
	~COggMTSource();

	TInt ReadBuf(TDes8& aBuf);

	void Read();
	void Read(RThread& aRequestThread, TRequestStatus& aRequestStatus);

	TBool CheckRead();
	void CheckAndStartRead();
	void StartRead();
	
	TInt WaitForCompletion();
	void WaitForCompletion(RThread& aRequestThread, TRequestStatus& aRequestStatus);

	TBool Seekable()
	{ return (iSourceType == EMTFile); }	
	
	// From CActive
	void RunL();
	void DoCancel();

private:
	void ReadComplete(TInt aErr);

private:
	TMTSourceData& iSourceData;

	TAny* iSource;
	TMTSourceType iSourceType;
	TPtr8 iBufPtr;

	RThread* iRequestThread;
	TRequestStatus* iRequestStatus;

	TBool iFileRequestPending;
	TBool iReadCancel;

	friend class CStreamingThreadSourceReader;
	};

// Source Reader
class CStreamingThreadSourceReader : public CActive, public MHttpSourceObserver
	{
public:
	enum TReaderState { EInactive, EReadRequest = 1, EHttpDataReceived = 2};

public:
	static CStreamingThreadSourceReader* NewLC(TStreamingThreadData& aThreadData);
	~CStreamingThreadSourceReader();

	TInt OpenSource();
	void SourceClose();

	TInt FileSize();
	TInt FileSeek(); 

	TInt ReadBuf();

	TInt Read();
	void Read(RThread& aRequestThread, TRequestStatus& aRequestStatus);

	void ReadRequest();
	void ReadCancel();
	void ReadCancel(RThread& aRequestThread, TRequestStatus& aRequestStatus);
	
	// From MHttpSourceObserver
	void HttpStateUpdate(TInt aState);
	void HttpOpenComplete();

	void HttpDataReceived();
	void HttpError(TInt aErr);

	// From CActive
	void RunL();
	void DoCancel();

	void TimerComplete();
	void SelfComplete();

	void ScheduleRead();

private:
	CStreamingThreadSourceReader(TStreamingThreadData& aThreadData);
	void ConstructL();

private:
	TStreamingThreadData& iSharedData;
	RFs iFs;

	TReaderState iReaderState;
	RTimer iTimer;
	};

// Playback engine (not an AO, although it handles call backs from the CMdaAudioOutputStream which is an AO)
// The playback engine handles all access to the CMdaAudioOutputStream as well as managing the buffering and decoding process
class CStreamingThreadPlaybackEngine : public CBase, public MMdaAudioOutputStreamCallback, public MOggMessageObserver, public MPrimeNextBufferObserver
	{
public:
	static CStreamingThreadPlaybackEngine* NewLC(TStreamingThreadData& aThreadData, CStreamingThreadSourceReader& aSourceReader);
	~CStreamingThreadPlaybackEngine();

	void Volume();
	void SetVolume();
	void SetAudioPropertiesL();

	void StartStreaming();
	void PauseStreaming();
	void StopStreaming(TBool aResetPosition = ETrue);

	void SetBufferingMode();
	void SetThreadPriority();

	void Position();

	TBool PrepareToFlushBuffers();
	TBool FlushBuffers();

	void Shutdown();

	// From MOggMessageObserver
	void MessagePosted();

	// From MPrimeNextBufferObserver
	void PrimeNextBufferCallBack(TInt aNumBuffers);

private:
	CStreamingThreadPlaybackEngine(TStreamingThreadData& aThreadData, CStreamingThreadSourceReader& aSourceReader);
	void ConstructL();

	// From MMdaAudioOutputStreamCallback
	void MaoscPlayComplete(TInt aError);
	void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer);
	void MaoscOpenComplete(TInt aError);

	void SendNextBuffer();

private:
	CMdaAudioOutputStream* iStream;
    TMdaAudioDataSettings iSettings;
	TStreamingThreadData& iSharedData;

	// Streaming thread AO, performs initial buffering (and buffering in single thread mode) 
	CStreamingThreadAO* iStreamingThreadAO;

	// The number of buffers currently "in the stream"
	TInt iStreamBuffers;

	// The index of the next buffer to be copied
	TInt iBufNum;

	// TBool containing the status (playing or not)
	TBool iStreaming;

	// Max volume setting (read once when the stream is opened)
	TInt iMaxVolume;

	// The priority of the buffering thread (dynamically adjusted depending on the number of buffers)
	TThreadPriority iBufferingThreadPriority;
	RThread iThread;

	// Buffer threshold (restart buffering if the number of buffers falls below this threshold)
	TInt iBufferLowThreshold;

	// The maximum number of buffers to send to the stream
	TInt iMaxStreamBuffers;

	// Boolean used to inhibit buffering requests
	// (if a buffer flush is pending the streaming thread must not issue buffering requests)
	TBool iBufferFlushPending;

	// Source reader (used for scheduling async reads)
	CStreamingThreadSourceReader& iSourceReader;

#if defined(PROFILE_PERF)
	CProfilePerfAO* iProfilePerfAO;
	friend class CProfilePerfAO;
#endif
	};

// Listener active object (running in the UI thread)
// Handles stream events (stream open, last buffer copied, play complete, streaming errors)
class CStreamingThreadListener : public CActive
	{
public:
	enum TStreamingThreadListeningState { EListeningForStreamOpen, EListeningForStreamingStatus };

public:
	CStreamingThreadListener(COggPlayback& aOggPlayback, TStreamingThreadData& aSharedData);
	~CStreamingThreadListener();

	void StartListening();

	// From CActive
	void RunL();
	void DoCancel();

private:
	COggPlayback& iOggPlayback;
	TStreamingThreadData& iSharedData;

	TStreamingThreadListeningState iListeningState;
	};

#endif // _OggMulti_h
