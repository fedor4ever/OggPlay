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


COggLog* COggLog::iInstance=0;

COggLog* COggLog::InstanceL() {
	
	if(iInstance==0) {
		iInstance=new(ELeave) COggLog;
		iInstance->ConstructL();
		
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

COggLog::ConstructL() {
	TInt ret=iLog.Connect();
	if (ret==KErrNone)	{
		iLog.CreateLog(KLogFolder, KLogFileName,EFileLoggingModeOverwrite);
		iLog.SetDateAndTime(EFalse,ETrue);
		_LIT(KS,"OggLog started...");
		iLog.WriteFormat(KS);
	} else {
		
	}

	
}