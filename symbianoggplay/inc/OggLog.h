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

#include "OggOs.h"
#include <flogger.h>

#define OGGLOG COggLog::InstanceL()->iLog
#define OGGPANIC(msg,reason) COggLog::InstanceL()->Panic(msg,reason)

// MACROS
//
// One reason for putting RDebug::Print in a macro is that the literals in 
// the debug messages will always be present in the binary, even on urel builds.
// You do not have to use these macros, it is always possible to access the
// OggLog static methods directly.
// 
#ifdef TRACE_ON
/// Print simple descriptor to console and file. Ex TRACEF( COggLog::VA(_L("1+1=%d"),2));
#define TRACEF(x)  RDebug::Print(x); COggLog::FilePrint(x)
/// Print simple descriptor to console only. Ex TRACE( COggLog::VA(_L("1+1=%d"),2));
#define TRACE(x)   RDebug::Print(x);
/// Print simple text to console only. Ex TRACEL("Hi there");
#define TRACEL(x)  TRACE(COggLog::VA(_L(x)))
/// Print simple text to console and file. Ex TRACELF("Hi there");
#define TRACELF(x) TRACEF(COggLog::VA(_L(x)))
#else
#define TRACEF(x)  ;
#define TRACE(x)  ;
#define TRACEL(x)  ;
#define TRACELF(x) ;
#endif

/// OggLog is a class giving access to the RFileLogger facility for all OggPlay classes.
/** It's implemented closely to the singleton pattern 
...with a twist due to Symbian not allowing global storage.<BR>
Remember to manually create "C:\Logs\OggPlay\" to get disk logging.
*/
class COggLog : public CBase
{

public:
  
  /// Prints to file. No initializations are needed beforehand.
  /**
  \sa Exit(), KLogFileName
  */
  static void FilePrint(const TDesC& msg);

  /// Not up-to-date
  void Panic(const TDesC& msg,TInt aReason);

  /// Must be called explicitly. Somebody fix this :-)
  static void Exit();

  /// Variable list argument formatting. I.e. TRACE(COggLog::VA(_L("1+1=%d"), 2 )
  static const TDesC& VA( TRefByValue<const TDesC> aLit,... );
  
protected:

  RFileLogger iLog;
  TBuf<100> iBuf;

private:

  static COggLog* InstanceL();

};


#endif
