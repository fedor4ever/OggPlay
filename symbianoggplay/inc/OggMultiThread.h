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

	// Open a file
	EStreamingThreadFileOpen,

	// Open a stream
	EStreamingThreadStreamOpen,

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

// Streaming thread status event
enum TStreamingThreadStatus
	{
	// The last buffer has been copied to the server (played)
	ELastBufferCopied,

	// Play has been interrupted by another process
	EPlayInterrupted,

	// Playback has underflowed (aka restart request)
	EPlayUnderflow,

	// Playback has stopped
	EPlayStopped
	};

// Shared data between the UI, buffering and streaming threads
// Owned by the UI thread (part of COggPlayback)
class CBufferingThreadAO;
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

	// Pointer to the last buffer (set when eof is reached)
	TDesC8* iLastBuffer;

	// The number of audio PCM bytes currently in buffers (changes whenever iNumBuffers changes)
	TInt iBufferBytesRead;
	TInt iBufferBytesPlayed;

	// The number of audio PCM bytes currently read from the file (used to work out the position)
	TInt64 iTotalBufferBytes;

	// Machine uid (for identifying the phone model)
	TInt iMachineUid;


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
	};

// Base class for the buffering AOs
class CBufferingAO : public CActive
	{
public:
	CBufferingAO(TInt aPriority, TStreamingThreadData& aSharedData);
	~CBufferingAO();

	void CreateTimerL();
	void PrimeNextBuffer();

protected:
	void SelfComplete();
	void SelfCompleteCancel();

protected:
	TStreamingThreadData& iSharedData;

private:
	RTimer iTimer;
	};

// Buffering thread active object
// Performs buffering when requested to by the streaming thread
class CBufferingThreadAO : public CBufferingAO
	{
public:
	CBufferingThreadAO(TStreamingThreadData& aSharedData);
	~CBufferingThreadAO();

	void StartListening();

	// From CActive
	void RunL();
	void DoCancel();
	};

// Streaming thread active object
// Responsible for buffering in single thread mode
// Responsible for buffering when play starts in multi thread mode
// (Once enough buffers have been decoded, buffering is transfered to the buffering thread)
class CStreamingThreadAO : public CBufferingAO
	{
public:
	CStreamingThreadAO(TStreamingThreadData& aSharedData, TBool& aBufferFlushPending, TThreadPriority& aBufferingThreadPriority);
	~CStreamingThreadAO();

	void StartBuffering();
	void ResumeBuffering();

	// From CActive
	void RunL();
	void DoCancel();

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
	TInt OpenFile(const TDesC& aFileName, TMTSourceData& aSourceData);
	TInt OpenStream(const TDesC& aStreamName, TMTSourceData& aSourceData);
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
	CStreamingThreadSourceHandler(TStreamingThreadData& aSharedData);
	~CStreamingThreadSourceHandler();

	void SetSourceReader(CStreamingThreadSourceReader* aSourceReader);

	// From MMTSourceHandler
	TInt OpenFile(const TDesC& aFileName, TMTSourceData& aSourceData);
	TInt OpenStream(const TDesC& aStreamName, TMTSourceData& aSourceData);
	void SourceClose(TMTSourceData& aSourceData);

	TInt Read(TMTSourceData& aSourceData, TDes8& aBuf);

	TInt Read(TMTSourceData& aSourceData);
	void ReadRequest(TMTSourceData& aSourceData);
	void ReadCancel(TMTSourceData& aSourceData);

	TInt FileSeek(TSeek aMode, TInt &aPos, TMTSourceData& aSourceData);
	TInt FileSize(TInt& aSize, TMTSourceData& aSourceData);

private:
	TStreamingThreadData& iSharedData;
	CStreamingThreadSourceReader* iSourceReader;
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
	COggMTSource(TAny* aSource, TMTSourceData& aSourceData);
	~COggMTSource();

	TInt ReadBuf(TDes8& aBuf);

	void Read();
	void Read(RThread& aRequestThread, TRequestStatus& aRequestStatus);
	
	TInt WaitForCompletion();
	void WaitForCompletion(RThread& aRequestThread, TRequestStatus& aRequestStatus);

	// From CActive
	void RunL();
	void DoCancel();

private:
	void ReadComplete(TInt aErr);

private:
	TMTSourceData& iSourceData;
	TAny* iSource;
	TPtr8 iBufPtr;

	RThread* iRequestThread;
	TRequestStatus* iRequestStatus;

	TBool iFileRequestPending;
	TBool iReadCancel;

	friend class CStreamingThreadSourceReader;
	};

// Source Reader
class CStreamingThreadSourceReader : public CActive
	{
public:
	static CStreamingThreadSourceReader* NewLC(TStreamingThreadData& aThreadData);
	~CStreamingThreadSourceReader();

	TInt OpenFile();
	void SourceClose();

	TInt FileSize();
	TInt FileSeek(); 

	TInt ReadBuf();

	TInt Read();
	void Read(RThread& aRequestThread, TRequestStatus& aRequestStatus);

	void ReadRequest();
	void ScheduleRead();

	void ReadCancel();
	void ReadCancel(RThread& aRequestThread, TRequestStatus& aRequestStatus);

	// From CActive
	void RunL();
	void DoCancel();
	
private:
	CStreamingThreadSourceReader(TStreamingThreadData& aThreadData);
	void ConstructL();

private:
	TStreamingThreadData& iSharedData;
	RFs iFs;
	};

// Playback engine (not an AO, although it handles call backs from the CMdaAudioOutputStream which is an AO)
// The playback engine handles all access to the CMdaAudioOutputStream as well as managing the buffering and decoding process
class CStreamingThreadPlaybackEngine : public CBase, public MMdaAudioOutputStreamCallback
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
	enum TStreamingThreadListeningState { EListeningForOpenComplete, EListeningForStreamingStatus };

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
