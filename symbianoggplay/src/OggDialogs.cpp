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

#include <eiklabel.h>

#if defined(SERIES60)
#include <e32base.h>
#include <eikenv.h>
#elif defined(SERIES80)
#include <eikchlst.h>
#include <eikchkbx.h>
#include <eikenv.h> 
#else
#include <eikchlst.h>
#include <eikchkbx.h>
#include <qiktimeeditor.h>
#endif

#include "OggDialogs.h"
#include "OggPlay.hrh"
#include <OggPlay.rsg>


CHotkeyDialog::CHotkeyDialog(int *aHotKeyIndex, int* anAlarmActive, TTime* anAlarmTime)
{
  iHotKeyIndex = aHotKeyIndex;
  iAlarmActive= anAlarmActive;
  iAlarmTime= anAlarmTime;
}


TBool
CHotkeyDialog::OkToExitL(int /* aButtonId */)
{
#if !( defined(SERIES60) || defined(SERIES80) )
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
#if !( defined(SERIES60) || defined(SERIES80) )
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

void COggPlayListInfoDialog::SetFileName(const TDesC& aFileName)
{
  iFileName.Copy(aFileName);
}

void COggPlayListInfoDialog::SetFileSize(TInt aFileSize)
{
  iFileSize= aFileSize;
}

void COggPlayListInfoDialog::SetPlayListEntries(TInt aPlayListEntries)
{
  iPlayListEntries= aPlayListEntries;
}


#if !(defined(SERIES60) || defined(SERIES80))
void
COggAboutDialog::PreLayoutDynInitL()
{
  CEikLabel* cVersion= (CEikLabel*)Control(EOggLabelAboutVersion);
  cVersion->SetTextL(iVersion);
}

void
COggAboutDialog::SetVersion(const TDesC& aVersion)
{
  iVersion = _L("OggPlay ");
  iVersion.Append(aVersion);
}


#else

void
COggInfoWinDialog::PreLayoutDynInitL()
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

  // Now add the text
  // Center-align
  
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

void
COggInfoWinDialog::SetInfoWinL(const TDesC& msg1, const TDesC& msg2 )
{
    if (iMsg1)
    {
        delete iMsg1;
        iMsg1= NULL;
    }
    if (iMsg2)
    {
        delete iMsg2;
        iMsg2= NULL;
    }
    iMsg1= msg1.AllocL();
    iMsg2= msg2.AllocL();
}


COggInfoWinDialog::~COggInfoWinDialog()
{
    
    delete iRichText; // contained text object
    delete iCharFormatLayer; // character format layer
    delete iParaFormatLayer; // and para format layer
    delete iMsg1;
    delete iMsg2;

}


void
COggAboutDialog::PreLayoutDynInitL()
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
  
  iRichText->AppendParagraphL(7);

  TBuf<128> buf;

  // Now add the text
  // Center-align
  paraFormatMask.SetAttrib(EAttAlignment); // interested in alignment
  paraFormat->iHorizontalAlignment=CParaFormat::ECenterAlign; 
  pos = iRichText->CharPosOfParagraph(len,0); // get start of first para
  iRichText->ApplyParaFormatL(paraFormat,paraFormatMask,pos,1);
  iRichText->InsertL(0,iVersion);
  iRichText->InsertL(0,_L("OggPlay "));

  // One paragraph to add some space.

  pos = iRichText->CharPosOfParagraph(len,2); // get start of 2nd para
  CEikonEnv::Static()->ReadResource(buf, R_OGG_ABOUT_LINE_2);
  iRichText->InsertL(pos,buf);
  
  pos = iRichText->CharPosOfParagraph(len,3); // get start of 3rd para
  CEikonEnv::Static()->ReadResource(buf, R_OGG_ABOUT_LINE_3);
  _LIT(KAdress, "http://symbianoggplay.sourceforge.net") ;
  TInt pos2 = pos + buf.Find(KAdress); // Beginning of the www adress
  iRichText->InsertL(pos,buf);
  
  charFormatMask.SetAttrib(EAttColor);
  charFormat.iFontPresentation.iTextColor=TLogicalRgb( TRgb(0,0,255) ) ; //Blue
  charFormatMask.SetAttrib(EAttFontUnderline); // interested in underline
  charFormat.iFontPresentation.iUnderline=EUnderlineOn; // set it on
  charFormatMask.SetAttrib(EAttFontPosture);
  charFormat.iFontSpec.iFontStyle.SetPosture(EPostureItalic);
  iRichText->ApplyCharFormatL(charFormat, charFormatMask, pos2, KAdress().Length() );

  pos = iRichText->CharPosOfParagraph(len,5); // get start of 4th para
  CEikonEnv::Static()->ReadResource(buf, R_OGG_ABOUT_LINE_4);
  iRichText->InsertL(pos,buf);
  pos = iRichText->CharPosOfParagraph(len,6); 
  CEikonEnv::Static()->ReadResource(buf, R_OGG_ABOUT_LINE_5);
  iRichText->InsertL(pos,buf);
  pos = iRichText->CharPosOfParagraph(len,7); 
  CEikonEnv::Static()->ReadResource(buf, R_OGG_ABOUT_LINE_6);
  iRichText->InsertL(pos,buf);

  UpdateModelL(iRichText);
}

void
COggAboutDialog::SetVersion(const TDesC& aVersion)
{
  iVersion.Copy(aVersion);
}


COggAboutDialog::~COggAboutDialog()
{
    
    delete iRichText; // contained text object
    delete iCharFormatLayer; // character format layer
    delete iParaFormatLayer; // and para format layer

}

void
CScrollableTextDialog::UpdateModelL(CRichText * aRichText)
{
    iScrollableControl->UpdateModelL(aRichText);
}


SEikControlInfo CScrollableTextDialog::CreateCustomControlL(TInt aControlType) 
    { 
    iScrollableControl = NULL; 
    if (aControlType == EOggScrollableTextControl) 
         { 
         iScrollableControl = new(ELeave)CScrollableRichTextControl; 
         } 
    SEikControlInfo info = {iScrollableControl,0,0};
    return info;
    } 


void CScrollableRichTextControl::ConstructFromResourceL(TResourceReader& aReader)
{
    TInt width=aReader.ReadInt16();
    TInt height=aReader.ReadInt16();
    TSize containerSize (width, height);
    
    SetSize(containerSize);
    ActivateL();	
}

CScrollableRichTextControl ::~CScrollableRichTextControl()
{
    delete iTextView; // text view
    delete iLayout; // text layout
    delete iSBFrame;  // Scroll bar
}

void CScrollableRichTextControl::UpdateModelL(CRichText * aRichText)
{
    iRichText = aRichText;
    
    // Create text view and layout.
    
    
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
    
    CleanupStack::PopAndDestroy();
    
    iTextView->FormatTextL();
    UpdateScrollIndicatorL();
    DrawNow();
}

void CScrollableRichTextControl::Draw(const TRect& /*aRect*/) const
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
    TRAPD(err,iTextView->DrawL(Rect() ) );
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
        UpdateScrollIndicatorL();
    }
    return CCoeControl::OfferKeyEventL(aKeyEvent, aType);
}


void CScrollableRichTextControl::UpdateScrollIndicatorL()
    {

    if ( !iSBFrame )
        {
        iSBFrame = new( ELeave ) CEikScrollBarFrame( this, NULL, ETrue );
        iSBFrame->SetScrollBarVisibilityL( CEikScrollBarFrame::EOff,
                                           CEikScrollBarFrame::EAuto );
        iSBFrame->SetTypeOfVScrollBar( CEikScrollBarFrame::EArrowHead );
        }

    TEikScrollBarModel hSbarModel;
    TEikScrollBarModel vSbarModel;
    
    TInt aaa = iLayout->FormattedHeightInPixels() ;
    aaa = iLayout->BandHeight() ;
    aaa = iLayout->PixelsAboveBand();

    vSbarModel.iThumbPosition = iLayout->PixelsAboveBand(); 
    vSbarModel.iScrollSpan = iLayout->FormattedHeightInPixels() - iLayout->BandHeight(); 
    vSbarModel.iThumbSpan = 1; 
    
    TEikScrollBarFrameLayout layout;
    TRect rect( Rect() );
    iSBFrame->TileL( &hSbarModel, &vSbarModel, rect, rect, layout );        
#ifdef SERIES60
    iSBFrame->SetVFocusPosToThumbPos( vSbarModel.iThumbPosition );
#endif
    }

    

#endif /*SERIES60*/
    
