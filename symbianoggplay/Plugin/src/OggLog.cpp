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
_LIT(KLogFileName,"OggTremor.log");


void TOggLogOverflow::Overflow(TDes& /* aDes */)
{
}

COggLog* COggLog::InstanceL() 
{
	COggLog* instance = (COggLog*) Dll::Tls();

	if (!instance) 
	{
		instance = new(ELeave) COggLog;
		Dll::SetTls(instance);

		TInt ret = instance->iLog.Connect();
		if (ret == KErrNone)	
		{
		  instance->iLog.CreateLog(KLogFolder, KLogFileName,EFileLoggingModeOverwrite);
			instance->iLog.SetDateAndTime(EFalse, ETrue);
		} 
		else 
			User::Panic(_L("COggLog::Instnce"), 0);
    }

	return instance;
}

void COggLog::Exit() 
{
	COggLog* instance = (COggLog*) Dll::Tls();
	if (instance)
	{
		instance->iLog.CloseLog();
		instance->iLog.Close();
		
		Dll::SetTls(NULL);
		delete instance;
	}
}

void COggLog::FilePrint(const TDesC& msg)
{
	InstanceL()->iLog.Write(msg);
}

void COggLog::Panic(const TDesC& msg, TInt aReason)
{
	iLog.Write(_L("** Fatal: **"));
	iLog.Write(msg);

	User::Panic(_L("OggPlay"), aReason);	
}

const TDesC& COggLog::VA(TRefByValue<const TDesC> aLit, ...)
{
	VA_LIST list;
	VA_START(list, aLit);
	COggLog* instance = InstanceL();
	instance->iBuf.Zero();
	instance->iBuf.AppendFormatList(aLit, list, &instance->iOverflowHandler);

	return instance->iBuf;
}
