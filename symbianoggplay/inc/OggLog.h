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

#define OGGLOG COggLog::InstanceL()->iLog
#define OGGPANIC(msg,reason) COggLog::InstanceL()->Panic(msg,reason)

// OggLog is a class giving access to the RFileLogger facility for all OggPlay classes.
// It's implemented closely to the singleton pattern 
// ...with a twist due to Symbian not allowing global storage.

class COggLog : CBase
{

public:
  ~COggLog();
  static COggLog* InstanceL();
  //static RFileLogger LogL();

  void Panic(const TDesC& msg,TInt aReason);
  RFileLogger iLog;
  
protected:
  COggLog();
  ConstructL();

};


#endif
