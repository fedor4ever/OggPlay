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

#include <e32std.h>
#include <e32base.h>
#include "OggOs.h"
#include "OggLog.h"
#include "OggThreadClasses.h"

// CThreadPanicHandler constructor
// TInt aPriority CActive priority
// RThread& aThread handle to the thread
// MThreadPanicObserver aPanicObserver object to notify when a panic occurs 
CThreadPanicHandler::CThreadPanicHandler(TInt aPriority, RThread& aThread, MThreadPanicObserver& aPanicObserver)
: CActive(aPriority), iThread(aThread), iPanicObserver(aPanicObserver)
{
	CActiveScheduler::Add(this);

	iThread.Logon(iStatus);
	SetActive();
}

CThreadPanicHandler::~CThreadPanicHandler()
{
}

// HandleCaughtPanic() is called by the command handler if it catches a panic
void CThreadPanicHandler::HandleCaughtPanic()
{
	// Mark that the panic has been captured
	iCaughtAPanic = ETrue;

	// Save the panic code
	TInt panicCode = iStatus.Int();

	// Cancel the request
	Cancel();

	// Inform the observer
	iPanicObserver.HandleThreadPanic(iThread, panicCode);
}

// Called if the thread exits (if it panics)
void CThreadPanicHandler::RunL()
{
	iPanicObserver.HandleThreadPanic(iThread, iStatus.Int());
}

void CThreadPanicHandler::DoCancel()
{
	if (iCaughtAPanic)
	{
		// The copmpletion event was caught by the command handler, so we must generate a cancel event.
		TRequestStatus* status = &iStatus;
		User::RequestComplete(status, KErrCancel);
	}
	else
		iThread.LogonCancel(iStatus);
}


// Thread Commandhandler constructor
// TInt aPriority CActive priority
// RThread& aCommandThread handle to the thread that issues commands
// RThread& aCommandHandlerThread handle to the thread that executes the commands (this should be a newly created thread)
// CThreadPanicHandler& aPanicHandler a reference to a panic handler object (used if a panic occurs while executing a command)
CThreadCommandHandler::CThreadCommandHandler(TInt aPriority, RThread& aCommandThread, RThread& aCommandHandlerThread, CThreadPanicHandler& aPanicHandler)
: CActive(aPriority), iCommandThread(aCommandThread), iCommandHandlerThread(aCommandHandlerThread), iPanicHandler(aPanicHandler)
{
}

CThreadCommandHandler::~CThreadCommandHandler()
{
}

void CThreadCommandHandler::ListenForCommands()
{
	iStatus = KRequestPending;
	SetActive();
}

// Start the command handler thread
// The return value is KErrNone if the thread is started successfully (or if the attempt to start it causes a panic)
// After this function has been called successfully commands may be sent to the thread
TInt CThreadCommandHandler::ResumeCommandHandlerThread()
{
	iCommandStatus = KRequestPending;
	iCommandHandlerThread.Resume();

	// Wait for the streaming thread to start (or for a panic)
	User::WaitForRequest(iCommandStatus, iPanicHandler.iStatus);

	// If the thread got started, return the result
	if (iCommandStatus != KRequestPending)
		return iCommandStatus.Int();

	// We've caught a panic, so deal with it
	iPanicHandler.HandleCaughtPanic();	
	return KErrNone;
}

// Shutdown the command handler thread
// Commands cannot be sent to the thread after it has shutdown (Shutting down the thread will cause it to stop running)
void CThreadCommandHandler::ShutdownCommandHandlerThread()
{
	iCommandStatus = KRequestPending;
	TRequestStatus* status = &iStatus;
	iCommandHandlerThread.RequestComplete(status, EThreadShutdown);

	// Wait for the streaming thread to terminate (or for a panic)
	User::WaitForRequest(iCommandStatus, iPanicHandler.iStatus);

	// If the thread terminated, return (discard the result)
	if (iCommandStatus != KRequestPending)
	{
		__ASSERT_DEBUG(iCommandStatus == KErrNone, User::Panic(_L("CTCH: Shutdown"), 0));
		return;
	}

	// We've caught a panic, so deal with it
	iPanicHandler.HandleCaughtPanic();
}

// Send a command to the command handler thread
TInt CThreadCommandHandler::SendCommand(TInt aCommand)
{
	// Mark the command status as request pending
	iCommandStatus = KRequestPending;

	// Send the command to the command handler thread
	TRequestStatus* status = &iStatus;
	iCommandHandlerThread.RequestComplete(status, aCommand);

	// Wait for an acknowlegement (or a panic)
	User::WaitForRequest(iCommandStatus, iPanicHandler.iStatus);

	// If the command completed return the result
	if (iCommandStatus != KRequestPending)
		return iCommandStatus.Int();

	// We've caught a panic, so deal with it
	iPanicHandler.HandleCaughtPanic();
	return KErrNone;
}

// The following three functions are called by the command handler thread
// to indicate the success or failure of starting the thread, shutting down the thread, and commands
void CThreadCommandHandler::ResumeComplete(TInt aErr)
{
	RequestComplete(aErr);
}

void CThreadCommandHandler::ShutdownComplete(TInt aErr)
{
	RequestComplete(aErr);
}

void CThreadCommandHandler::RequestComplete(TInt err)
{
	// Inform the command thread that the command is complete
	TRequestStatus* commandStatus = &iCommandStatus;
	iCommandThread.RequestComplete(commandStatus, err);
}

