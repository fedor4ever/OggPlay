/*
 *  Copyright (c) 2003 P. Wolff
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

#ifndef _OGGLOG_H
#define _OGGLOG_H

#include <flogger.h>
#include <OggOs.h>

#define OGGLOG COggLog::InstanceL()->iLog
#define OGGPANIC(msg,reason) COggLog::InstanceL()->Panic(msg,reason)

// MACROS
//
// One reason for putting RDebug::Print in a macro is that the literals in 
// the debug messages will always be present in the binary, even on urel builds.
// You do not have to use these macros, it is always possible to access the
// OggLog static methods directly.
// 
#if defined(TRACE_ON)
#if defined(_DEBUG)
// Print simple descriptor to file. Ex TRACEF( COggLog::VA(_L("1+1=%d"),2));
#define TRACEF(x)  RDebug::Print(x); COggLog::FilePrint(x);

// Print simple descriptor to console only. Ex TRACE( COggLog::VA(_L("1+1=%d"),2));
#define TRACE(x)   RDebug::Print(x);
#else
// Print simple descriptor to file. Ex TRACEF( COggLog::VA(_L("1+1=%d"),2));
#define TRACEF(x)  COggLog::FilePrint(x);

// Traces in a release build are now disabled
#define TRACE(x)   ;
#endif
#else
#define TRACEF(x)  ;
#define TRACE(x)  ;
#endif

// OggLog is a class giving access to the RFileLogger facility for all OggPlay classes.
/** It's implemented closely to the singleton pattern 
...with a twist due to Symbian not allowing global storage.<BR>
Remember to manually create "C:\Logs\OggPlay\" to get disk logging.
*/

class TOggLogOverflow : public TDesOverflow
{
	private:
		// From TDesOverflow
		void Overflow(TDes& aDes);
};

class COggLog : public CBase
{
public: 
	static void FilePrint(const TDesC& msg);

	// Not up-to-date
	void Panic(const TDesC& msg,TInt aReason);

	// Must be called explicitly. Somebody fix this :-)
	static void Exit();

	// Variable list argument formatting. I.e. TRACE(COggLog::VA(_L("1+1=%d"), 2 )
	static const TDesC& VA(TRefByValue<const TDesC> aLit, ...);

private:
	static COggLog* Instance();
	static COggLog* InstanceL();

private:
	RFileLogger iLog;
	TBuf<1024> iBuf;
	TOggLogOverflow iOverflowHandler;

#if defined(SERIES60V3)
	TThreadId iThreadId;
#endif
};


#endif
