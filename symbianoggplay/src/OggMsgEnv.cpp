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
#include <OggPlay.rsg>
#include <eikenv.h>

#include "OggMsgEnv.h"
#include "OggDialogs.h"
#include "OggPlay.h"

#if defined(SERIES60)
#include <aknmessagequerydialog.h>
#endif

COggMsgEnv::COggMsgEnv(TBool& aWarningsEnabled)
:  iWarningsEnabled(aWarningsEnabled)
	{
	}

TBool COggMsgEnv::WarningsEnabled() const
	{
	return iWarningsEnabled;
	}

COggMsgEnv::~COggMsgEnv()
	{
	}

#if defined(SERIES60)
void COggMsgEnv::OggWarningMsgL(const TDesC& aWarning) const
	{
    if (iWarningsEnabled)
		{
        TBuf<128> warningTitle;
        CEikonEnv::Static()->ReadResource(warningTitle, R_OGG_ERROR_27);

		TPtrC warningTxt(aWarning);
		CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(warningTxt);
		dlg->PrepareLC(R_DIALOG_MSG);
		dlg->QueryHeading()->SetTextL(warningTitle);
		dlg->RunLD();
		}
	}

void COggMsgEnv::OggErrorMsgL(const TDesC& aFirstLine, const TDesC& aSecondLine)
	{
	// FIXME
	// We really shouldn't need this code
	COggPlayAppUi* appUi = (COggPlayAppUi *) CEikonEnv::Static()->EikAppUi();
	if (appUi->iStartUpState != COggPlayAppUi::EStartUpComplete)
		return;

	TBuf<128> errorTitle;
	CEikonEnv::Static()->ReadResource(errorTitle, R_OGG_ERROR_33);

	TBuf<256> errorTxt;
	errorTxt.Append(aFirstLine);
	errorTxt.Append(_L("\n"));
	errorTxt.Append(aSecondLine);

	CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(errorTxt);
	dlg->PrepareLC(R_DIALOG_MSG);
	dlg->QueryHeading()->SetTextL(errorTitle);
	dlg->RunLD();
	}

void COggMsgEnv::OggErrorMsgL(TInt aFirstLineId, TInt aSecondLineId)
	{
    TBuf<256> buf1, buf2;
    CEikonEnv::Static()->ReadResource(buf1, aFirstLineId);
    CEikonEnv::Static()->ReadResource(buf2, aSecondLineId);
    OggErrorMsgL(buf1, buf2);
	}
#else
void COggMsgEnv::OggWarningMsgL(const TDesC& aWarning) const
	{
	if (iWarningsEnabled)
		{
		TBuf<64> buf(_L("Warning"));
		CEikonEnv::Static()->InfoWinL(buf, aWarning);
		}
	}

void COggMsgEnv::OggErrorMsgL(TInt aFirstLineId, TInt aSecondLineId)
	{
	CEikonEnv::Static()->InfoWinL(aFirstLineId, aSecondLineId);
	}
void COggMsgEnv::OggErrorMsgL(const TDesC& aFirstLine, const TDesC& aSecondLine)
	{
	CEikonEnv::Static()->InfoWinL(aFirstLine, aSecondLine);
	}
#endif
