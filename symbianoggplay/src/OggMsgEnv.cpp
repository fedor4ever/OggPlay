/*
 *  Copyright (c) 2003 L. H. Wilden.
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

#include <OggOs.h>
#include "OggMsgEnv.h"
#include "OggDialogs.h"
#include "OggPlay.h"
#include <OggPlay.rsg>
#include <eikenv.h>

COggMsgEnv::COggMsgEnv( TOggplaySettings &aSettings) :  iSettings (aSettings)
{
}

TBool COggMsgEnv::WarningsEnabled()
{
	return iSettings.iWarningsEnabled;
}

COggMsgEnv::~COggMsgEnv()
{
}

#ifdef SERIES60
void COggMsgEnv::OggWarningMsgL(const TDesC& aWarning)// const
{
    if (iSettings.iWarningsEnabled)
    {
        TBuf<128> TmpBuf;
        CEikonEnv::Static()->ReadResource(TmpBuf,R_OGG_ERROR_27);
        COggInfoWinDialog *d = new COggInfoWinDialog();
        d->SetInfoWinL(aWarning,TmpBuf);
        d->ExecuteLD(R_DIALOG_INFOWIN);
    }
}

void COggMsgEnv::OggErrorMsgL(const TDesC& aFirstLine,const TDesC& aSecondLine)// const
{
    COggInfoWinDialog *d = new COggInfoWinDialog();
    d->SetInfoWinL(aFirstLine,aSecondLine);
    d->ExecuteLD(R_DIALOG_INFOWIN);
}

void COggMsgEnv::OggErrorMsgL(TInt aFirstLineId,TInt aSecondLineId)// const
{
    TBuf<256> buf1,buf2;
    CEikonEnv::Static()->ReadResource(buf1, aFirstLineId);
    CEikonEnv::Static()->ReadResource(buf2, aSecondLineId);
    OggErrorMsgL(buf1,buf2);
}
#else
void COggMsgEnv::OggWarningMsgL(const TDesC& aWarning)// const
{
    if (iSettings.iWarningsEnabled)
    {
      TBuf<256> buf(_L("Warning"));
      CEikonEnv::Static()->InfoWinL(buf, aWarning);
    }
}

void COggMsgEnv::OggErrorMsgL(TInt aFirstLineId,TInt aSecondLineId)// const
{
    CEikonEnv::Static()->InfoWinL(aFirstLineId, aSecondLineId);
}
void COggMsgEnv::OggErrorMsgL(const TDesC& aFirstLine,const TDesC& aSecondLine)// const
{
    CEikonEnv::Static()->InfoWinL(aFirstLine, aSecondLine);
}
#endif
