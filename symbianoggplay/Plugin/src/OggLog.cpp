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


COggLog* COggLog::InstanceL() 
  {
#if defined(UIQ)
    return 0;
#else
  COggLog* iInstance =(COggLog*) Dll::Tls();

  if( !iInstance ) 
    {
    iInstance=new(ELeave) COggLog;
    Dll::SetTls(iInstance);

    TInt ret=iInstance->iLog.Connect();
    if (ret==KErrNone)	
      {
		  iInstance->iLog.CreateLog(KLogFolder, KLogFileName,EFileLoggingModeOverwrite);
		  iInstance->iLog.SetDateAndTime(EFalse,ETrue);
		  _LIT(KS,"OggLog started...");
		  iInstance->iLog.WriteFormat(KS);
	    } 
    else 
      {
      User::Panic(_L("OggLog"),11);	
      }
    }
	return iInstance;
#endif
  }



void COggLog::Exit() 
  {
#if defined(UIQ)
  return;
#else
  COggLog* iInstance =(COggLog*) Dll::Tls();
  if( iInstance )
    {
	  iInstance->iLog.WriteFormat(_L("OggLog closed.."));
    iInstance->iLog.CloseLog();
    iInstance->iLog.Close();
    delete iInstance;
    Dll::SetTls(NULL);
    }
#endif
  }


void COggLog::FilePrint(const TDesC& msg) 
  {
#if (defined( SERIES60) || defined (SERIES80) )
  InstanceL()->iLog.WriteFormat(msg);
#endif
  }



void COggLog::Panic(const TDesC& msg,TInt aReason) 
  {
#if (defined( SERIES60) || defined (SERIES80) )
  iLog.Write(_L("** Fatal: **"));
  iLog.Write(msg);
#endif
  User::Panic(_L("OggPlay"),aReason);	
  }


const TDesC& COggLog::VA( TRefByValue<const TDesC> aLit,... )
  {
#if defined(UIQ)
    TBuf<16> buf;
    return buf;
#else
  VA_LIST list;
  VA_START(list, aLit);
  COggLog::InstanceL()->iBuf.Zero();
  COggLog::InstanceL()->iBuf.AppendFormatList(aLit,list);
  
  // There can't be at this point any % left, otherwise
  // whole thing going to panic (typically another format is called
  // right after this...).
  // Change all % into %%
  // This code stinks. Can't find a better way to do that search and replace
  
  TUint16* bufStart = (TUint16*)InstanceL()->iBuf.Ptr();
  TInt len = InstanceL()->iBuf.Length();
  TPtr16 descriptor( bufStart,len, 1024);
  
  TInt nbFound =0; 
  TInt found=descriptor.Locate('%');
  while ( found != KErrNotFound)
  {
      nbFound++;
      descriptor.Replace(found,1,_L("%%"));
      bufStart = bufStart + found + 2;
      len = len - (found + 2) + 1;
      descriptor.Set(bufStart, len, 1024 - nbFound);
      found=descriptor.Locate('%');
  }
  descriptor.Set((TUint16*)InstanceL()->iBuf.Ptr(),
                  InstanceL()->iBuf.Length()+nbFound,1024);
  COggLog::InstanceL()->iBuf = descriptor;
  
  return COggLog::InstanceL()->iBuf;
#endif
  }


