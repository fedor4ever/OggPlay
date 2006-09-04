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

_LIT(KLogFolder, "Oggplay");
_LIT(KLogFileName, "Oggplay.log");

#if defined(SERIES60V3)
#define KLoggingThreads 2
COggLog* InstanceArray[KLoggingThreads];
#endif

COggLog* COggLog::Instance()
{
#if defined(SERIES60V3)
	RThread thisThread;
	TThreadId threadId = thisThread.Id();

	COggLog* instance;
	TInt i;
	for (i = 0 ; i<KLoggingThreads ; i++)
	{
		instance = InstanceArray[i];
		if (!instance)
			continue;

		if (instance->iThreadId == threadId)
			break;
	}

	if (i == KLoggingThreads)
		instance = NULL;
#else
	COggLog* instance = (COggLog*) Dll::Tls();
#endif

	return instance;	
}

COggLog* COggLog::InstanceL() 
{
	COggLog* instance = Instance();
	if (!instance) 
	{
		instance = new(ELeave) COggLog;
		TInt ret = instance->iLog.Connect();
		if (ret == KErrNone)	
		{
			instance->iLog.CreateLog(KLogFolder, KLogFileName, EFileLoggingModeAppend);
			instance->iLog.SetDateAndTime(EFalse, ETrue);
		} 
		else 
			User::Panic(_L("COggLog::Instnce"), 0);

#if defined(SERIES60V3)
		TInt i;
		for (i = 0 ; i<KLoggingThreads; i++)
		{
			if (!InstanceArray[i])
			{
				InstanceArray[i] = instance;
				break;
			}
		}

		if (i == KLoggingThreads)
			User::Panic(_L("COggLog::Instnce"), 1);

		RThread thisThread;
		instance->iThreadId = thisThread.Id();
#else
		Dll::SetTls(instance);
#endif
	}

	return instance;
  }

void COggLog::Exit() 
{
	COggLog* instance = Instance();
	if (instance)
	{
		instance->iLog.CloseLog();
		instance->iLog.Close();

#if defined(SERIES60V3)
		TInt i;
		for (i = 0 ; i<KLoggingThreads ; i++)
		{
			if (instance == InstanceArray[i])
			{
				InstanceArray[i] = NULL;
				break;
			}
		}
#else
		Dll::SetTls(NULL);
#endif

		delete instance;
	}
  }

void COggLog::FilePrint(const TDesC& msg)
{
	InstanceL()->iLog.WriteFormat(msg);
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
	instance->iBuf.AppendFormatList(aLit,list);
  
	// There can't be at this point any % left, otherwise
	// whole thing going to panic (typically another format is called
	// right after this...).
	// Change all % into %%
	// This code stinks. Can't find a better way to do that search and replace  
	TUint16* bufStart = (TUint16*) instance->iBuf.Ptr();
	TInt len = instance->iBuf.Length();
	TPtr16 descriptor(bufStart, len, 1024);
  
	TInt nbFound = 0;
	TInt found = descriptor.Locate('%');
	while (found != KErrNotFound)
	{
		nbFound++;
		descriptor.Replace(found,1,_L("%%"));
		bufStart = bufStart + found + 2;
		len = len - (found + 2) + 1;
		descriptor.Set(bufStart, len, 1024 - nbFound);
		found = descriptor.Locate('%');
	}

	descriptor.Set((TUint16*)instance->iBuf.Ptr(), instance->iBuf.Length()+nbFound, 1024);
	instance->iBuf = descriptor;
  
	return instance->iBuf;
  }
