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
#include <coemain.h>

#if defined(SERIES60)
#include <aknappui.h>
#include <aknutils.h>

#if defined(SERIES60SUI)
#include <AknsDrawUtils.h>
#include <AknsBasicBackgroundControlContext.h>
#endif

#include <avkon.rsg>
#endif

#if defined(SERIES80)
#include <eikenv.h>
#include <eikappui.h>
#include <eikbtgpc.h>
#include <eikcore.rsg>
#endif

#if defined(UIQ)
#include <eikappui.h>
#endif

#include <eikapp.h>

#include <Oggplay.rsg>
#include "Oggplay.hrh"
#include "OggFilesSearchDialogs.h"
#include "OggLog.h"


COggFilesSearchDialog::COggFilesSearchDialog(MOggFilesSearchBackgroundProcess *aBackgroundProcess)
: iBackgroundProcess(aBackgroundProcess)
	{
	}

SEikControlInfo COggFilesSearchDialog::CreateCustomControlL(TInt aControlType)
	{
    if (aControlType == EOggFileSearchControl) 
        iContainer = new(ELeave) COggFilesSearchContainer(*this, iBackgroundProcess, &ButtonGroupContainer());

	SEikControlInfo info = {iContainer, 0, 0};
    return info;
	}


COggFilesSearchContainer::COggFilesSearchContainer(COggFilesSearchDialog& aDlg, MOggFilesSearchBackgroundProcess* aBackgroundProcess, CEikButtonGroupContainer* aCba)
: iParent(aDlg), iBackgroundProcess(aBackgroundProcess), iCba(aCba)
    {
    }

COggFilesSearchContainer::~COggFilesSearchContainer()
    {
    delete iFishBmp;
    delete iFishMask;
    delete iAO;

#if defined(SERIES60SUI)
	delete iBgContext;
#endif
	}

void COggFilesSearchContainer::ConstructFromResourceL(TResourceReader& aReader)
	{
	iTitleTxt = aReader.ReadTPtrC();
	iLine1Txt = aReader.ReadTPtrC();
	iLine2Txt = aReader.ReadTPtrC();
	iLine3Txt = aReader.ReadTPtrC();

#if defined(SERIES60SUI)
	iBgContext = CAknsBasicBackgroundControlContext::NewL(KAknsIIDQsnBgAreaMain, TRect(TPoint(0, 0), MinimumSize()), ETrue);
#endif

    // Load bitmaps
	TFileName fileName(iEikonEnv->EikAppUi()->Application()->AppFullName());
	TParsePtr parse(fileName);

#if defined (SERIES60V3)
	fileName.Copy(parse.Drive());
	fileName.Append(_L("\\resource\\apps\\OggPlay\\fish.mbm"));
#else
	// Copy back only drive and path
    fileName.Copy(parse.DriveAndPath());
    fileName.Append(_L("fish.mbm"));
#endif

	iFishBmp = iEikonEnv->CreateBitmapL(fileName, 0); 
    iFishMask = iEikonEnv->CreateBitmapL(fileName, 1);

	iAO = new(ELeave) COggFilesSearchAO(this);
    iAO->StartL();

    ActivateL();	
	}

#if defined(SERIES60SUI)
TTypeUid::Ptr COggFilesSearchContainer::MopSupplyObject(TTypeUid aId)
	{
	return MAknsControlContext::SupplyMopObject(aId, iBgContext);
	}
#endif

_LIT(KMaxScanTxt, " 0000");
_LIT(KFormatTxt, "%S %d");
TSize COggFilesSearchContainer::MinimumSize()
	{
#if defined(SERIES60SUI)
	const CFont* titleFont = AknLayoutUtils::FontFromId(EAknLogicalFontPrimaryFont, NULL);
	const CFont* textFont = AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont, NULL);
#elif defined(SERIES60)
	const CFont* titleFont = LatinBold12();
	const CFont* textFont = LatinPlain12();
#else
	CFont* titleFont;
	TFontSpec fs(_L("LatinPlain12"), 12);
	CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(titleFont, fs);

	CFont* textFont;
	TFontSpec fs2(_L("LatinBold12"), 12);
	CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(textFont, fs2);
#endif

	TInt titleHeight = titleFont->HeightInPixels();

	TBuf<128> txtBuf = iTitleTxt;
	TInt titleWidth = titleFont->TextWidthInPixels(txtBuf);

	txtBuf = iLine1Txt;
	txtBuf.Append(KMaxScanTxt);
	TInt textWidth = textFont->TextWidthInPixels(txtBuf);

	txtBuf = iLine2Txt;
	txtBuf.Append(KMaxScanTxt);
	TInt textWidth2 = textFont->TextWidthInPixels(txtBuf);

	txtBuf = iLine3Txt;
	txtBuf.Append(KMaxScanTxt);
	TInt textWidth3 = textFont->TextWidthInPixels(txtBuf);

	TInt totalWidth = titleWidth>textWidth ? titleWidth : textWidth;
	totalWidth = totalWidth>textWidth2 ? totalWidth : textWidth2;
	totalWidth = totalWidth>textWidth3 ? totalWidth : textWidth3;

	totalWidth = (totalWidth * 116) / 100;
	if (totalWidth & 1)
		totalWidth++;

	TInt textHeight = textFont->HeightInPixels();
	TInt totalHeight = (150 * (titleHeight + 3 * textHeight)) / 100 + 36;
	totalHeight = (totalHeight * 110) / 100;
	if (totalHeight & 1)
		totalHeight++;

	iFishPosition.iX = (totalWidth >> 1) - 21;
	iFishPosition.iY = (95 * totalHeight) / 100 - 30;
	iFishMinX = 2;
	iFishMaxX = totalWidth - 48;
	iFishStepX = (iFishMaxX - iFishMinX) / 20;

	return TSize(totalWidth, totalHeight);
	}

#if defined(SERIES60SUI)
void COggFilesSearchContainer::SizeChanged()
	{
	iBgContext->SetRect(Rect());
	iBgContext->SetParentPos(PositionRelativeToScreen());
	}
#endif

#if defined(SERIES60SUI)
void COggFilesSearchContainer::Draw(const TRect& aRect) const
    {
	CWindowGc& gc = SystemGc();

	// Draw the dialog background (with skin if available)
    MAknsSkinInstance* skin = AknsUtils::SkinInstance();
  	MAknsControlContext* cc = AknsDrawUtils::ControlContext(this);
    AknsDrawUtils::Background(skin, cc, this, gc, aRect);

	TRgb titleColor(0, 0, 0);
	AknsUtils::GetCachedColor(skin, titleColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG1);
	gc.SetPenColor(titleColor);
    gc.DrawRect(Rect());

	TRgb textColor(0, 0, 0);
	AknsUtils::GetCachedColor(skin, textColor, KAknsIIDQsnTextColors, EAknsCIQsnTextColorsCG6);
	gc.SetPenColor(textColor);

	const CFont* titleFont = AknLayoutUtils::FontFromId(EAknLogicalFontPrimaryFont, NULL);
	const CFont* textFont = AknLayoutUtils::FontFromId(EAknLogicalFontSecondaryFont, NULL);
#else
void COggFilesSearchContainer::Draw(const TRect& /* aRect */) const
    {
	CWindowGc& gc = SystemGc();

	gc.SetBrushColor(KRgbWhite);
	gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(Rect());

	TRgb textColor(0, 0, 0);

#if defined(SERIES60)
	const CFont* titleFont = LatinBold12();
	const CFont* textFont = LatinPlain12();
#else
	CFont* titleFont;
	TFontSpec fs(_L("LatinPlain12"), 12);
	CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(titleFont, fs);

	CFont* textFont;
	TFontSpec fs2(_L("LatinBold12"), 12);
	CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(textFont, fs2);
#endif
#endif

	TInt titleBaseLine = (Rect().Size().iHeight * 5) / 100;
	titleBaseLine += titleFont->AscentInPixels();

	TInt titleWidth = titleFont->TextWidthInPixels(iTitleTxt);
	if (titleWidth & 1)
		titleWidth++;

	TInt titlePos = (Rect().Size().iWidth - titleWidth) >> 1;

	gc.UseFont(titleFont);
	gc.DrawText(iTitleTxt, TPoint(titlePos, titleBaseLine));

	TInt textBaseLine = (Rect().Size().iHeight * 5) / 100;
	textBaseLine += (150 * titleFont->HeightInPixels()) / 100;

	textBaseLine += textFont->AscentInPixels();
	TInt textPos = (Rect().Size().iWidth * 10) / 100;
	TBuf<128> txtBuf;
	if (iFoldersCount)
		txtBuf.Format(KFormatTxt, &iLine1Txt, iFoldersCount);
	else
		txtBuf = iLine1Txt;

	gc.UseFont(textFont);
	gc.SetPenColor(textColor);
	gc.DrawText(txtBuf, TPoint(textPos, textBaseLine));

	if (iFilesCount)
		txtBuf.Format(KFormatTxt, &iLine2Txt, iFilesCount);
	else
		txtBuf = iLine2Txt;

	textBaseLine += (150 * textFont->HeightInPixels()) / 100;
	gc.DrawText(txtBuf, TPoint(textPos, textBaseLine));

	if (iPlayListCount)
		txtBuf.Format(KFormatTxt, &iLine3Txt, iPlayListCount);
	else
		txtBuf = iLine3Txt;

	textBaseLine += (150 * textFont->HeightInPixels()) / 100;
	gc.DrawText(txtBuf, TPoint(textPos, textBaseLine));

	gc.BitBltMasked(iFishPosition, iFishBmp, TRect(iFishBmp->SizeInPixels()), iFishMask, ETrue);
    }


#if defined(SERIES60) || defined(SERIES80)
void COggFilesSearchContainer::UpdateCba()
{
#if defined(SERIES60)
	iCba->SetCommandSetL(R_AVKON_SOFTKEYS_OK_EMPTY);
#elif defined(SERIES80)
    iCba->SetCommandSetL(R_EIK_BUTTONS_CONTINUE);
#endif

    iCba->DrawNow();
	}
#endif

void COggFilesSearchContainer::UpdateControl()
	{
	iBackgroundProcess->FileSearchGetCurrentStatus(iFoldersCount, iFilesCount, iPlayListCount);

	iFishPosition.iX -= iFishStepX;
    if (iFishPosition.iX < iFishMinX)
        iFishPosition.iX = iFishMaxX;

	DrawNow();
	}


COggFilesSearchAO::COggFilesSearchAO(COggFilesSearchContainer * aContainer)
: CActive(EPriorityIdle), iContainer(aContainer)
	{
    CActiveScheduler::Add(this);
	}

COggFilesSearchAO::~COggFilesSearchAO()
	{
	Cancel();

	if (iTimer)
		iTimer->Cancel();

	delete iCallBack;
	delete iTimer;
	}

void COggFilesSearchAO::RunL()
	{
	TInt result = iStatus.Int();
	if (result != KErrNone)
		{
		if (result == KOggFileScanStepComplete)
			{
			TRACEF(_L("File search next step"));
			iSearchStatus = (TSearchStatus) (((TInt) iSearchStatus) + 1);
			}
		else
			{
			TRACEF(COggLog::VA(_L("File search step error: %d"), result));
			}
		}

	// Run one iteration of the long process.
	MOggFilesSearchBackgroundProcess* longProcess = iContainer->iBackgroundProcess;
	switch (iSearchStatus)
		{
		case EScanningFiles:
			longProcess->FileSearchStep(iStatus);
			break;

		case EScanningPlaylists:
			longProcess->ScanNextPlayList(iStatus);
			break;

		case EScanningComplete:
			iContainer->UpdateControl();

#if defined(SERIES60) || defined(SERIES80)
			iContainer->UpdateCba();
#endif
			iTimer->Cancel();
			return;

		default:
			{
			TRACEF(_L("File Search unknown step"));
			break;
			}
		}

	SetActive();
	}

void COggFilesSearchAO::DoCancel()
	{
	iContainer->iBackgroundProcess->CancelOutstandingRequest();
	}

void COggFilesSearchAO::StartL()
	{
	iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
	iCallBack = new(ELeave) TCallBack(COggFilesSearchAO::CallBack, iContainer);
	iTimer->Start(TTimeIntervalMicroSeconds32(100000), TTimeIntervalMicroSeconds32(100000), *iCallBack);

	SelfComplete();
	}

void COggFilesSearchAO::SelfComplete()
	{
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrNone);

	SetActive();
	}

TInt COggFilesSearchAO::CallBack(TAny* aPtr)
	{
	((COggFilesSearchContainer *) aPtr)->UpdateControl();
	return 1;
	}
