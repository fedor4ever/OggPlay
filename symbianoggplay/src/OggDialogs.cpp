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

#include "OggOs.h"
#include "OggDialogs.h"

#include "OggPlay.hrh"
#include <OggPlay.rsg>

#include <eiklabel.h>
#if defined(SERIES60)
// include 
#else
#include <eikchlst.h>
#include <eikchkbx.h>
#include <qiktimeeditor.h>
#endif

CHotkeyDialog::CHotkeyDialog(int *aHotKeyIndex, int* anAlarmActive, TTime* anAlarmTime)
{
  iHotKeyIndex = aHotKeyIndex;
  iAlarmActive= anAlarmActive;
  iAlarmTime= anAlarmTime;
}


TBool
CHotkeyDialog::OkToExitL(int /* aButtonId */)
{
#if !defined(SERIES60)
  CEikChoiceList *cl = (CEikChoiceList*)Control(EOggOptionsHotkey);
  CEikCheckBox* cb= (CEikCheckBox*)Control(EOggOptionsAlarmActive);
  CQikTimeEditor* ct= (CQikTimeEditor*)Control(EOggOptionsAlarmTime);
  *iHotKeyIndex = cl->CurrentItem();
  *iAlarmActive= cb->State();
  *iAlarmTime= ct->Time();
#endif
  return ETrue;
}


void
CHotkeyDialog::PreLayoutDynInitL()
{
#if !defined(SERIES60)
  CEikChoiceList *cl = (CEikChoiceList*)Control(EOggOptionsHotkey);
  CEikCheckBox* cb= (CEikCheckBox*)Control(EOggOptionsAlarmActive);
  CQikTimeEditor* ct= (CQikTimeEditor*)Control(EOggOptionsAlarmTime);
  cl->SetArrayL(R_HOTKEY_ARRAY);
  cl->SetCurrentItem(*iHotKeyIndex);
  cb->SetState((CEikButtonBase::TState)*iAlarmActive);
  ct->SetTimeL(*iAlarmTime);
#endif
}


void
COggInfoDialog::PreLayoutDynInitL()
{
  CEikLabel* cFileName= (CEikLabel*)Control(EOggLabelFileName);
  cFileName->SetTextL(iFileName);

  CEikLabel* cFileSize= (CEikLabel*)Control(EOggLabelFileSize);
  TBuf<256> buf;
  buf.Num(iFileSize);
  cFileSize->SetTextL(buf);

  CEikLabel* cRate= (CEikLabel*)Control(EOggLabelRate);
  buf.Num(iRate);
  cRate->SetTextL(buf);

  CEikLabel* cChannels= (CEikLabel*)Control(EOggLabelChannels);
  buf.Num(iChannels);
  cChannels->SetTextL(buf);

  CEikLabel* cBitRate= (CEikLabel*)Control(EOggLabelBitRate);
  buf.Num(iBitRate);
  cBitRate->SetTextL(buf);
}

void
COggInfoDialog::SetFileName(const TDesC& aFileName)
{
  iFileName.Copy(aFileName);
}

void
COggInfoDialog::SetFileSize(TInt aFileSize)
{
  iFileSize= aFileSize;
}

void
COggInfoDialog::SetRate(TInt aRate)
{
  iRate= aRate;
}

void
COggInfoDialog::SetChannels(TInt aChannels)
{
  iChannels= aChannels;
}

void
COggInfoDialog::SetTime(TInt aTime)
{
  iTime= aTime;
}

void
COggInfoDialog::SetBitRate(TInt aBitRate)
{
  iBitRate= aBitRate;
}


void
COggAboutDialog::PreLayoutDynInitL()
{
  CEikLabel* cVersion= (CEikLabel*)Control(EOggLabelAboutVersion);
  cVersion->SetTextL(iVersion);
}

void
COggAboutDialog::SetVersion(const TDesC& aVersion)
{
  iVersion.Copy(aVersion);
}



#if 0
// Skeleton of the Class 
// Soon to be built
SEikControlInfo COggScrollableInfoDialog::CreateCustomControlL(TInt aControlType) 
    { 
    CRichControl *control = NULL; 
    if (aControlType == EOggClTestBert) 
         { 
         control = new(ELeave)CRichControl; 
         } 
    SEikControlInfo info = {control,0,0};
    return info;
    } 



#ifdef SERIES60
void CScrollableRichTextControl::ConstructL(const TRect& aRect
                              , const CCoeControl& aParent
                              )
{
    // create window
    CreateWindowL(&aParent);
    // set rectangle to prescription
    SetRect(aRect);
    // go for it
    ActivateL();
    UpdateModelL(); // phase 0
}

void CScrollableRichTextControl::ConstructFromResourceL(TResourceReader& aReader)
{
    // Read the smiley mood from the resource file
    // Read the width of the smiley container from the resource file.
    TInt width=aReader.ReadInt16();
    TInt height=aReader.ReadInt16();
    // Set the height of the container to be half its width
    TSize containerSize (width, height);
    
    SetSize(containerSize);
    UpdateModelL();
    ActivateL();	
}


CScrollableRichTextControl ::~CScrollableRichTextControl()
{
    delete iTextView; // text view
    delete iLayout; // text layout
    delete iRichText; // contained text object
    delete iCharFormatLayer; // character format layer
    delete iParaFormatLayer; // and para format layer
}


void CScrollableRichTextControl::UpdateModelL()
{
    // Content is just an example
    // Will be defined later.
    _LIT(KTitle, "OggPlay");
    _LIT(KVersion,"Version 0XX");
    
    _LIT(KFreeware, "blabla");
    
    _LIT(KWebAdress, "please consult http://geocities.com/thing.html");
    _LIT(KEmailAdress, "Comments and improvement ideas are welcome: mail0@hotmail.com");
    
    // Create text object, text view and layout.
    
    TCharFormat charFormat;
    TCharFormatMask charFormatMask;
    charFormat.iFontSpec.iTypeface.iName = _L("LatinBold12");
    charFormatMask.SetAttrib(EAttFontTypeface); 
    
    iParaFormatLayer=CParaFormatLayer::NewL(); // required para format layer
    iCharFormatLayer=CCharFormatLayer::NewL(charFormat,charFormatMask); // required char format layer
    // Create an empty rich text object
    iRichText=CRichText::NewL(iParaFormatLayer, iCharFormatLayer);
    // prerequisites for view - viewing rectangle
    iViewRect=Rect();
    iViewRect.Shrink(3,3);
    // context and device
    CWindowGc& gc=SystemGc(); // get graphics context
    CBitmapDevice *device=(CBitmapDevice*) (gc.Device()); // device
    // Create the text layout, (required by text view),
    // with the text object and a wrap width (=width of view rect)
    iLayout=CTextLayout::NewL(iRichText,iViewRect.Width());
    // Create text view
    iTextView=CTextView::NewL(iLayout, iViewRect,
        device,
        device,
        &Window(),
        0, // no window group
        &iCoeEnv->WsSession()
        ); // new view
    
    CParaFormat *paraFormat = CParaFormat::NewL();
    CleanupStack::PushL(paraFormat);
    TParaFormatMask paraFormatMask;
    TInt len;
    TInt pos;
    
    iRichText->AppendParagraphL(5);
    
    // Center-align
    paraFormatMask.SetAttrib(EAttAlignment); // interested in alignment
    paraFormat->iHorizontalAlignment=CParaFormat::ECenterAlign; 
    
    pos = iRichText->CharPosOfParagraph(len,0); // get start of first para
    iRichText->ApplyParaFormatL(paraFormat,paraFormatMask,pos,1);
    // apply format to entire para - even length = 1 char will do
    iRichText->InsertL(0,KTitle);
    
    pos = iRichText->CharPosOfParagraph(len,1); // get start of 2nd para
    iRichText->ApplyParaFormatL(paraFormat,paraFormatMask,pos,1);
    //pos=iRichText->DocumentLength();
    iRichText->InsertL(pos,KVersion);
    
    pos = iRichText->CharPosOfParagraph(len,2); // get start of 3rd para
    //pos=iRichText->DocumentLength();
    iRichText->InsertL(pos,KFreeware);
    
    pos = iRichText->CharPosOfParagraph(len,3); // get start of 3rd para
    //pos=iRichText->DocumentLength();
    
    TBuf<100> buf1;
    buf1 = KWebAdress;
    TInt pos2 = buf1.Find(_L("http"));
    iRichText->InsertL(pos,KWebAdress);
    
    charFormatMask.SetAttrib(EAttColor);
    charFormat.iFontPresentation.iTextColor=TLogicalRgb( TRgb(0,0,255)) ; //Blue
    charFormatMask.SetAttrib(EAttFontUnderline); // interested in underline
    charFormat.iFontPresentation.iUnderline=EUnderlineOn; // set it on
    charFormatMask.SetAttrib(EAttFontPosture);
    charFormat.iFontSpec.iFontStyle.SetPosture(EPostureItalic);
    iRichText->ApplyCharFormatL(charFormat, charFormatMask,pos+pos2, buf1.Length()-pos2);
    
    pos = iRichText->CharPosOfParagraph(len,4); // get start of 3rd para
    //pos=iRichText->DocumentLength();
    iRichText->InsertL(pos,KEmailAdress);
    buf1 = KEmailAdress;
    pos2 = buf1.Find(_L("bertq"));
    iRichText->ApplyCharFormatL(charFormat, charFormatMask,pos+pos2, buf1.Length()-pos2);
    
    CleanupStack::PopAndDestroy();
    DrawNow();
}

void CScrollableRichTextControl::Draw(const TRect& aRect) const
{
    // draw surround
    CGraphicsContext& gc=SystemGc(); // context to draw into
    
    TRect rect=Rect(); // screen boundary
    gc.DrawRect(rect); // outline screen boundary
    rect.Shrink(1,1);
    gc.SetPenColor(KRgbWhite);
    gc.DrawRect(rect);
    rect.Shrink(1,1);
    gc.SetPenColor(KRgbBlack);
    gc.DrawRect(rect);
    // draw editable text - will work unless OOM
    TInt err;
    TRAP(err,iTextView->FormatTextL());
    if (err) return;
    TRAP(err,iTextView->DrawL(Rect() ) );
}


TCoeInputCapabilities CScrollableRichTextControl::InputCapabilities() const
{
    return TCoeInputCapabilities::ENone;
}


TKeyResponse CScrollableRichTextControl::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
    if (aType ==  EEventKey)
    {
        switch (aKeyEvent.iScanCode)
        {
        case EStdKeyDownArrow: 
            {
                TInt line = -1;
                iTextView->ScrollDisplayLinesL(line);
                break;
            }
            
        case EStdKeyUpArrow: 
            {
                
                TInt line = 1;
                iTextView->ScrollDisplayLinesL(line);
                break;
            }
        }
    }
    return CCoeControl::OfferKeyEventL(aKeyEvent, aType);
}

#endif /*SERIES60*/
#endif /* if 0 */
    
