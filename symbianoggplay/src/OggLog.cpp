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


#include "OggLog.h"

_LIT(KLogFolder,"Oggplay");
_LIT(KLogFileName,"Oggplay.log");

COggLog* COggLog::InstanceL() {
  COggLog* iInstance =(COggLog*) 0; //Dll::Tls();

	if(iInstance==0) {
		iInstance=new(ELeave) COggLog;
		iInstance->ConstructL();
		//Dll::SetTls(iInstance);
	}
	return iInstance;
}

/*
RFileLogger COggLog::LogL() {
	
	if(iInstance==0) {
		iInstance=new(ELeave) COggLog;
		iInstance->ConstructL();
		
	}
	return iLog;
}
*/

COggLog::COggLog() {
	
}

COggLog::~COggLog() {

  //Dll::SetTls(0);
	
}

void COggLog::ConstructL() {
	TInt ret=iLog.Connect();
	if (ret==KErrNone)	{
		iLog.CreateLog(KLogFolder, KLogFileName,EFileLoggingModeOverwrite);
		iLog.SetDateAndTime(EFalse,ETrue);
		_LIT(KS,"OggLog started...");
		iLog.WriteFormat(KS);
	} else {
    User::Panic(_L("OggLog"),11);	
	}

}

void COggLog::Panic(const TDesC& msg,TInt aReason) {
  iLog.Write(_L("** Fatal: **"));
  iLog.Write(msg);
  User::Panic(_L("OggPlay"),aReason);	
}


TRefByValue<const TDesC> COggLog::VA( TRefByValue<const TDesC> aLit,... )
  {
  VA_LIST list;
  VA_START(list, aLit);
  TBuf<100> buf(_L("***OggPlay*** "));
  buf.AppendFormatList(aLit,list);
  return buf;
  }


