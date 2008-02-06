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

#if defined(SERIES60)
#include <e32base.h>
#include <eikenv.h>
#include <barsread.h>
#include <aknmessagequerydialog.h>
#elif defined(SERIES80)
#include <eikchlst.h>
#include <eikchkbx.h>
#include <eikenv.h> 
#include <eiklabel.h>
#else
#include <eikchlst.h>
#include <eikchkbx.h>
#include <eiklabel.h>
#include <qiktimeeditor.h>
#endif

#include "OggFileInfo.h"
#include "OggDialogs.h"
#include "OggPlay.hrh"
#include <OggPlay.rsg>

#define BETA_VERSION
#if defined(BETA_VERSION)
_LIT(KBetaTxt, " beta2");
#endif

#if defined(SERIES60)
#if !defined(SERIES60V3)
// Minor compatibility tweak
#define ReadResourceL ReadResource
#define ReadResourceAsDes8L ReadResourceAsDes8
#endif

_LIT(KSingleLF, "\n");
_LIT(KDoubleLF, "\n\n");
_LIT(KFormatTxt, "%S %d");
_LIT(KTimeFormatTxt, "%S %d:%02d");


void COggAboutDialog::ExecuteLD(TInt aResourceId)
	{
	CleanupStack::PushL(this);

	TBuf8<1024> resBuf;
	CEikonEnv::Static()->ReadResourceAsDes8L(resBuf, aResourceId); 

	TResourceReader resReader;
	resReader.SetBuffer(&resBuf);

	TBuf<512> bodyTxt;
	TPtrC resTxt = resReader.ReadTPtrC();
	bodyTxt.Append(resTxt);

#if defined(BETA_VERSION)
	bodyTxt.Append(KBetaTxt);
#endif

	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KDoubleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KDoubleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KSingleLF);

#if defined(SERIES60V3)
	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KDoubleLF);
#else
	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);
	bodyTxt.Append(KDoubleLF);

	resReader.ReadTPtrC();
#endif

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);

	CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(bodyTxt);
	dlg->ExecuteLD(R_DIALOG_ABOUT_S60);

	CleanupStack::PopAndDestroy(this);
	}


#if defined(SERIES60V3)
COggFileInfoDialog::COggFileInfoDialog(const TOggFileInfo& aInfo)
: iFileName(aInfo.iFileName), iFileSize(aInfo.iFileSize), iTime((TInt) (aInfo.iTime/1000)),
iBitRate(aInfo.iBitRate/1000), iRate(aInfo.iRate), iChannels(aInfo.iChannels)
#else
COggFileInfoDialog::COggFileInfoDialog(const TOggFileInfo& aInfo)
: iFileName(aInfo.iFileName), iFileSize(aInfo.iFileSize), iTime((aInfo.iTime/TInt64(1000)).GetTInt()),
iBitRate(aInfo.iBitRate/1000), iRate(aInfo.iRate), iChannels(aInfo.iChannels)
#endif
	{
	}

void COggFileInfoDialog::ExecuteLD(TInt aResourceId)
	{
	CleanupStack::PushL(this);

	TBuf8<1024> resBuf;
	CEikonEnv::Static()->ReadResourceAsDes8L(resBuf, aResourceId); 

	TResourceReader resReader;
	resReader.SetBuffer(&resBuf);

	TBuf<512> bodyTxt;
	TParsePtrC parse(iFileName);
	bodyTxt.Append(parse.NameAndExt());
	bodyTxt.Append(KDoubleLF);

	TPtrC resTxt = resReader.ReadTPtrC();
	bodyTxt.AppendFormat(KFormatTxt, &resTxt, iFileSize);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.AppendFormat(KTimeFormatTxt, &resTxt, iTime/60, iTime%60);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.AppendFormat(KFormatTxt, &resTxt, iBitRate);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.AppendFormat(KFormatTxt, &resTxt, iRate);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.AppendFormat(KFormatTxt, &resTxt, iChannels);
	bodyTxt.Append(KDoubleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);

	TPtrC driveAndPath(parse.DriveAndPath());
	bodyTxt.Append(driveAndPath);
	if (driveAndPath.Length()>3)
		bodyTxt.SetLength(bodyTxt.Length()-1);

	CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(bodyTxt);
	dlg->ExecuteLD(R_DIALOG_INFO_S60);

	CleanupStack::PopAndDestroy(this);
	}


COggPlayListInfoDialog::COggPlayListInfoDialog(const TDesC& aFileName, TInt aFileSize, TInt aPlayListEntries)
: iFileName(aFileName), iFileSize(aFileSize), iPlayListEntries(aPlayListEntries)
	{
	}

void COggPlayListInfoDialog::ExecuteLD(TInt aResourceId)
	{
	CleanupStack::PushL(this);

	TBuf8<512> resBuf;
	CEikonEnv::Static()->ReadResourceAsDes8L(resBuf, aResourceId); 

	TResourceReader resReader;
	resReader.SetBuffer(&resBuf);

	TBuf<256> bodyTxt;
	TParsePtrC parse(iFileName);
	bodyTxt.Append(parse.NameAndExt());
	bodyTxt.Append(KDoubleLF);

	TPtrC resTxt = resReader.ReadTPtrC();
	bodyTxt.AppendFormat(KFormatTxt, &resTxt, iFileSize);
	bodyTxt.Append(KSingleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.AppendFormat(KFormatTxt, &resTxt, iPlayListEntries);
	bodyTxt.Append(KDoubleLF);

	resTxt.Set(resReader.ReadTPtrC());
	bodyTxt.Append(resTxt);

	TPtrC driveAndPath(parse.DriveAndPath());
	bodyTxt.Append(driveAndPath);
	if (driveAndPath.Length()>3)
		bodyTxt.SetLength(bodyTxt.Length()-1);

	CAknMessageQueryDialog *dlg = CAknMessageQueryDialog::NewL(bodyTxt);
	dlg->ExecuteLD(R_DIALOG_INFO_S60);

	CleanupStack::PopAndDestroy(this);
	}
#else
COggFileInfoDialog::COggFileInfoDialog(const TOggFileInfo& aInfo)
: iFileName(aInfo.iFileName), iFileSize(aInfo.iFileSize), iTime((aInfo.iTime/TInt64(1000)).GetTInt()),
iBitRate(aInfo.iBitRate/1000), iRate(aInfo.iRate), iChannels(aInfo.iChannels)
	{
	}

_LIT(KTimeFormatTxt, "%d:%02d");
void COggFileInfoDialog::PreLayoutDynInitL()
	{
	CEikLabel* c = (CEikLabel *) Control(EOggLabelFileName);

#if defined(SERIES80)
	c->SetTextL(iFileName);
#else
	TParsePtrC parse(iFileName);
	c->SetTextL(parse.NameAndExt());
#endif

	TFileName buf;
	c = (CEikLabel*) Control(EOggLabelFileSize);
	buf.Num(iFileSize);
	c->SetTextL(buf);

	c = (CEikLabel*) Control(EOggLabelDuration);
	buf.Format(KTimeFormatTxt, iTime/60, iTime%60);
	c->SetTextL(buf);

	c = (CEikLabel*) Control(EOggLabelRate);
	buf.Num(iRate);
	c->SetTextL(buf);

	c = (CEikLabel*) Control(EOggLabelChannels);
	buf.Num(iChannels);
	c->SetTextL(buf);

	c = (CEikLabel*) Control(EOggLabelBitRate);
	buf.Num(iBitRate);
	c->SetTextL(buf);

#if !defined(SERIES80)
	TPtrC driveAndPath(parse.DriveAndPath());
	buf = driveAndPath;
	if (driveAndPath.Length()>3)
		buf.SetLength(buf.Length()-1);

	c = (CEikLabel*) Control(EOggLabelFilePath);
	c->SetTextL(buf);
#endif
	}


COggPlayListInfoDialog::COggPlayListInfoDialog(const TDesC& aFileName, TInt aFileSize, TInt aPlayListEntries)
: iFileName(aFileName), iFileSize(aFileSize), iPlayListEntries(aPlayListEntries)
	{
	}

void COggPlayListInfoDialog::PreLayoutDynInitL()
	{
	CEikLabel* cFileName= (CEikLabel*)Control(EOggLabelFileName);
	cFileName->SetTextL(iFileName);

	CEikLabel* cFileSize= (CEikLabel*)Control(EOggLabelFileSize);
	TBuf<256> buf;
	buf.Num(iFileSize);
	cFileSize->SetTextL(buf);

	CEikLabel* cPlayListEntries= (CEikLabel*)Control(EOggLabelPlayListEntries);
	buf.Num(iPlayListEntries);
	cPlayListEntries->SetTextL(buf);
	}
#endif


#if defined(UIQ)
CHotkeyDialog::CHotkeyDialog(TInt *aHotKeyIndex, TInt* anAlarmActive, TTime* anAlarmTime)
	{
	iHotKeyIndex = aHotKeyIndex;
	iAlarmActive= anAlarmActive;
	iAlarmTime= anAlarmTime;
	}

TBool CHotkeyDialog::OkToExitL(TInt /* aButtonId */)
	{
	CEikChoiceList *cl = (CEikChoiceList*)Control(EOggOptionsHotkey);
	CEikCheckBox* cb= (CEikCheckBox*)Control(EOggOptionsAlarmActive);
	CQikTimeEditor* ct= (CQikTimeEditor*)Control(EOggOptionsAlarmTime);
	*iHotKeyIndex = cl->CurrentItem();
	*iAlarmActive= cb->State();
	*iAlarmTime= ct->Time();

	return ETrue;
	}

void CHotkeyDialog::PreLayoutDynInitL()
	{
	CEikChoiceList *cl = (CEikChoiceList*)Control(EOggOptionsHotkey);
	CEikCheckBox* cb= (CEikCheckBox*)Control(EOggOptionsAlarmActive);
	CQikTimeEditor* ct= (CQikTimeEditor*)Control(EOggOptionsAlarmTime);
	cl->SetArrayL(R_HOTKEY_ARRAY);
	cl->SetCurrentItem(*iHotKeyIndex);
	cb->SetState((CEikButtonBase::TState)*iAlarmActive);
	ct->SetTimeL(*iAlarmTime);
	}
#elif defined(SERIES80)
void COggInfoWinDialog::PreLayoutDynInitL()
	{
	// Create an empty rich text object
	TCharFormat charFormat;
	TCharFormatMask charFormatMask;
	charFormat.iFontSpec.iTypeface.iName = _L("LatinBold12");
	charFormatMask.SetAttrib(EAttFontTypeface); 
  
	iParaFormatLayer=CParaFormatLayer::NewL(); // required para format layer
	iCharFormatLayer=CCharFormatLayer::NewL(charFormat,charFormatMask); // required char format layer
  
	iRichText=CRichText::NewL(iParaFormatLayer, iCharFormatLayer);
	CParaFormat *paraFormat = CParaFormat::NewL();
	CleanupStack::PushL(paraFormat);
	TParaFormatMask paraFormatMask;
	TInt len;
	TInt pos;
  
	iRichText->AppendParagraphL(2);
	paraFormatMask.SetAttrib(EAttAlignment); // interested in alignment
	paraFormat->iHorizontalAlignment=CParaFormat::ECenterAlign; 

	// Now add the text (Center-align)
	charFormatMask.SetAttrib(EAttColor);
	charFormat.iFontPresentation.iTextColor=TLogicalRgb( TRgb(0,0,255) ) ; //Blue

	pos = iRichText->CharPosOfParagraph(len,0); // get start of first para
	iRichText->SetInsertCharFormatL(charFormat,charFormatMask,0);
	iRichText->ApplyParaFormatL(paraFormat,paraFormatMask,pos,1);
	iRichText->InsertL(0,iMsg1->Des()); 

	iRichText->CancelInsertCharFormat();
	pos = iRichText->CharPosOfParagraph(len,1); // get start of 2nd para
	iRichText->InsertL(pos,iMsg2->Des());
	iRichText->ApplyParaFormatL(paraFormat,paraFormatMask,pos,2);

	UpdateModelL(iRichText);
	}

void COggInfoWinDialog::SetInfoWinL(const TDesC& msg1, const TDesC& msg2 )
	{
	delete iMsg1;
	iMsg1 = NULL;

	delete iMsg2;
	iMsg2 = NULL;

	iMsg1 = msg1.AllocL();
	iMsg2 = msg2.AllocL();
	}

COggInfoWinDialog::~COggInfoWinDialog()
	{
	delete iRichText; // contained text object
	delete iCharFormatLayer; // character format layer
	delete iParaFormatLayer; // and para format layer
	delete iMsg1;
	delete iMsg2;
	}


void COggAboutDialog::PreLayoutDynInitL()
	{
	// Create an empty rich text object
	TCharFormat charFormat;
	TCharFormatMask charFormatMask;
	charFormat.iFontSpec.iTypeface.iName = _L("LatinBold12");
	charFormatMask.SetAttrib(EAttFontTypeface); 
  
	iParaFormatLayer = CParaFormatLayer::NewL();
	iCharFormatLayer = CCharFormatLayer::NewL(charFormat,charFormatMask);

	iRichText = CRichText::NewL(iParaFormatLayer, iCharFormatLayer);
	CParaFormat *paraFormat = CParaFormat::NewL();
	CleanupStack::PushL(paraFormat);
	TParaFormatMask paraFormatMask;  
	iRichText->AppendParagraphL(9);

	// Now add the text
	paraFormatMask.SetAttrib(EAttAlignment);
	paraFormat->iHorizontalAlignment = CParaFormat::ECenterAlign; 

	TInt len;
	TInt pos = iRichText->CharPosOfParagraph(len, 0); // get start of first para
	iRichText->ApplyParaFormatL(paraFormat, paraFormatMask, pos, 1); // Center align

	TBuf8<1024> resBuf;
	CEikonEnv::Static()->ReadResourceAsDes8L(resBuf, R_DIALOG_ABOUT); 

	TResourceReader resReader;
	resReader.SetBuffer(&resBuf);

	TPtrC resTxt = resReader.ReadTPtrC();

	TBuf<256> buf;
	buf.Copy(resTxt);

	iRichText->InsertL(0, buf);

	pos = iRichText->CharPosOfParagraph(len, 1); // get start of 2nd para
	iRichText->ApplyParaFormatL(paraFormat, paraFormatMask, pos, 1); // Center align

	resTxt.Set(resReader.ReadTPtrC());
	iRichText->InsertL(pos, resTxt);
  
	pos = iRichText->CharPosOfParagraph(len, 3); // get start of 4th para
	iRichText->ApplyParaFormatL(paraFormat, paraFormatMask, pos, 1); // Center align

	resTxt.Set(resReader.ReadTPtrC());
	buf.Copy(resTxt);

	_LIT(KAdress, "http://symbianoggplay.sourceforge.net") ;
	TInt pos2 = pos + buf.Find(KAdress); // Beginning of the www adress
	iRichText->InsertL(pos, buf);
  
	charFormatMask.SetAttrib(EAttColor);
	charFormat.iFontPresentation.iTextColor = TLogicalRgb(KRgbBlue);
	charFormatMask.SetAttrib(EAttFontUnderline);
	charFormat.iFontPresentation.iUnderline = EUnderlineOn;
	charFormatMask.SetAttrib(EAttFontPosture);
	charFormat.iFontSpec.iFontStyle.SetPosture(EPostureItalic);
	iRichText->ApplyCharFormatL(charFormat, charFormatMask, pos2, KAdress().Length());

	pos = iRichText->CharPosOfParagraph(len, 5); // get start of 6th para
	iRichText->ApplyParaFormatL(paraFormat, paraFormatMask, pos, 1); // Center align

	resTxt.Set(resReader.ReadTPtrC());
	iRichText->InsertL(pos, resTxt);

	pos = iRichText->CharPosOfParagraph(len, 6); // get start of 7th para
	iRichText->ApplyParaFormatL(paraFormat, paraFormatMask, pos, 1); // Center align

	resTxt.Set(resReader.ReadTPtrC());
	iRichText->InsertL(pos, resTxt);

	resReader.ReadTPtrC();
	resTxt.Set(resReader.ReadTPtrC());

	pos = iRichText->CharPosOfParagraph(len, 8); // get start of 9th para
	iRichText->InsertL(pos, resTxt);

	UpdateModelL(iRichText);
	}

COggAboutDialog::~COggAboutDialog()
	{
    delete iRichText;
    delete iCharFormatLayer;
    delete iParaFormatLayer;
	}


void CScrollableTextDialog::UpdateModelL(CRichText * aRichText)
	{
	iScrollableControl->UpdateModelL(aRichText);
	}

SEikControlInfo CScrollableTextDialog::CreateCustomControlL(TInt aControlType) 
	{
	iScrollableControl = NULL; 
	if (aControlType == EOggScrollableTextControl) 
		iScrollableControl = new(ELeave)CScrollableRichTextControl; 

	SEikControlInfo info = {iScrollableControl, 0, 0};
	return info;
	}


void CScrollableRichTextControl::ConstructFromResourceL(TResourceReader& aReader)
	{
	TInt width = aReader.ReadInt16();
	TInt height = aReader.ReadInt16();
	TSize containerSize(width, height);
    
	SetSize(containerSize);
	ActivateL();	
	}

CScrollableRichTextControl ::~CScrollableRichTextControl()
	{
	delete iTextView;
	delete iLayout;
	delete iSBFrame;
	}

void CScrollableRichTextControl::UpdateModelL(CRichText* aRichText)
	{
	iRichText = aRichText;
    
	// Create text view and layout.
	// prerequisites for view - viewing rectangle
	iViewRect = Rect();
	iViewRect.Shrink(3, 3);

	// context and device
	CWindowGc& gc = SystemGc();
	CBitmapDevice* device = (CBitmapDevice*) gc.Device();

	// Create the text layout, (required by text view),
	// with the text object and a wrap width (=width of view rect)
	iLayout = CTextLayout::NewL(iRichText, iViewRect.Width());

	// Create text view
	iTextView = CTextView::NewL(iLayout, iViewRect, device, device, &Window(), NULL, &iCoeEnv->WsSession());
	CleanupStack::PopAndDestroy();

	iTextView->FormatTextL();
	UpdateScrollIndicatorL();
	DrawNow();
	}

void CScrollableRichTextControl::Draw(const TRect& /*aRect*/) const
	{
	// draw surround
	CGraphicsContext& gc=SystemGc(); // context to draw into

	TRect rect = Rect(); // screen boundary
	gc.DrawRect(rect); // outline screen boundary
	rect.Shrink(1, 1);

	gc.SetPenColor(KRgbWhite);
	gc.DrawRect(rect);

	rect.Shrink(1,1);
	gc.SetPenColor(KRgbBlack);
	gc.DrawRect(rect);

	// draw editable text - will work unless OOM
	TRAPD(err, iTextView->DrawL(Rect()));
	}

TKeyResponse CScrollableRichTextControl::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
	{
	if (aType == EEventKey)
		{
		switch (aKeyEvent.iScanCode)
			{
			case EStdKeyDownArrow: 
				{
				TInt line = -1;
				iTextView->ScrollDisplayLinesL(line);
				UpdateScrollIndicatorL();

				return EKeyWasConsumed;
				}

			case EStdKeyUpArrow: 
				{
				TInt line = 1;
				iTextView->ScrollDisplayLinesL(line);
				UpdateScrollIndicatorL();

				return EKeyWasConsumed;
				}
			}
		}

	return CCoeControl::OfferKeyEventL(aKeyEvent, aType);
	}

void CScrollableRichTextControl::UpdateScrollIndicatorL()
	{
	if (!iSBFrame)
		{
		iSBFrame = new(ELeave) CEikScrollBarFrame(this, NULL, ETrue);
		iSBFrame->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);
		iSBFrame->SetTypeOfVScrollBar(CEikScrollBarFrame::EArrowHead);
		}

	TEikScrollBarModel vSbarModel;
	vSbarModel.iThumbPosition = iLayout->PixelsAboveBand(); 
	vSbarModel.iScrollSpan = iLayout->FormattedHeightInPixels() - iLayout->BandHeight(); 
	vSbarModel.iThumbSpan = 1; 
    
	TRect rect(Rect());
	TEikScrollBarFrameLayout layout;
	TEikScrollBarModel hSbarModel;
	iSBFrame->TileL(&hSbarModel, &vSbarModel, rect, rect, layout);        
	}
#endif // UIQ
