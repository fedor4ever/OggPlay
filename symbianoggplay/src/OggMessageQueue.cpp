#include <OggOs.h>
#include <e32std.h>
#include "OggMessageQueue.h"


ROggMessageQueue::ROggMessageQueue(MOggMessageObserver& aObserver)
: iNoMemoryMessage(EStreamingError, KErrNoMemory), iMessageHead(NULL), iMessageTail(NULL), iObserver(aObserver)
	{
	}

void ROggMessageQueue::Reset()
	{
	TStreamingMessage* next;
	while (iMessageHead)
		{
		next = iMessageHead->iNextMessage;

		delete iMessageHead;
		iMessageHead = next;
		}
	}

void ROggMessageQueue::Close()
	{
	Reset();
	}

TStreamingMessage* ROggMessageQueue::Message()
	{
	TStreamingMessage* message = iMessageHead;
	while (message && message->iMessageRead)
		message = message->iNextMessage;

	return message;
	}

void ROggMessageQueue::PostMessage(TStreamingMessage* aMessage)
	{
	if (!aMessage)
		aMessage = &iNoMemoryMessage;

	if (!iMessageTail)
		{
		iMessageTail = aMessage;
		iMessageHead = iMessageTail;
		}
	else
		{
		iMessageTail->iNextMessage = aMessage;
		iMessageTail = aMessage;
		}

	iObserver.MessagePosted();
	}

void ROggMessageQueue::PurgeReadMessages()
	{
	TStreamingMessage* message = iMessageHead;
	while (message && message->iMessageRead)
		{
		iMessageHead = message->iNextMessage;
		delete message;

		message = iMessageHead;
		}

	if (!iMessageHead)
		iMessageTail = NULL;
	}
