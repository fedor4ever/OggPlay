/*
 *  Copyright (c) 2008 S. Fisher
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

#ifndef __OggMessage_h
#define __OggMessage_h

#include <e32std.h>

// Streaming thread status event
enum TStreamingEvent
	{
	// The audio stream is open
	EStreamOpen,

	// The stream connection is being opened (state update)
	ESourceOpenState,

	// The file (or stream) has been opened
	ESourceOpened,

	// The stream has provided more data (so buffering can now continue)
	EDataReceived,
	
	// The last buffer has been copied to the server (played)
	ELastBufferCopied,

	// A new section buffer has been copied to the server (played)
	ENewSectionCopied,

	// Play has been interrupted by another process
	EPlayInterrupted,

	// Playback has underflowed (aka restart request)
	EPlayUnderflow,

	// Playback has stopped
	EPlayStopped,

	// An unexpected error has occured
	EStreamingError
	};

class TStreamingMessage
	{
public:
	TStreamingMessage(TStreamingEvent aEvent, TInt aStatus)
	: iEvent(aEvent), iStatus(aStatus), iNextMessage(NULL), iMessageRead(EFalse)
	{ }

public:
	TStreamingEvent iEvent;
	TInt iStatus;

	TStreamingMessage* iNextMessage;
	TBool iMessageRead;
	};

class MOggMessageObserver
	{
public:
	virtual void MessagePosted() = 0;
	};

class ROggMessageQueue
	{
public:
	ROggMessageQueue(MOggMessageObserver& aObserver);
	void Reset();
	void Close();
	
	// Read access
	TStreamingMessage* Message();

	// Write access
	void PostMessage(TStreamingMessage* aMessage);

	// Purge read messages (needs to be done at a synchronisation point) 
	void PurgeReadMessages();

private:
	TStreamingMessage iNoMemoryMessage;
	TStreamingMessage* iMessageHead;
	TStreamingMessage* iMessageTail;

	MOggMessageObserver& iObserver;
	};
#endif
