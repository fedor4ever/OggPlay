/*
 *  Copyright (c) 2003 L. H. Wilden. All rights reserved.
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

#include "OggControls.h"

#include <OggPlay.rsg>

#include <eikenv.h>
#include <math.h>
#include "int_fft.c"

TInt GetTextWidth(const TDesC& aText, CFont* aFont, TInt w)
{
  TInt pos = aFont->TextCount(aText, w);
  while (pos<aText.Length()) {
    w+= 16;
    pos = aFont->TextCount(aText, w);
  }
  return w;
}

_LIT(KEndToken,"END");

/***********************************************************
 *
 * COggControl
 *
 ***********************************************************/

COggControl::COggControl(TInt ax, TInt ay, TInt aw, TInt ah) :
  ix(ax),
  iy(ay),
  iw(aw),
  ih(ah),
  iRedraw(ETrue),
  iCycle(0),
  iVisible(ETrue),
  iDimmed(EFalse),
  iObserver(0)
{
}

void
COggControl::Redraw()
{
  iRedraw= ETrue;
}

void
COggControl::MakeVisible(TBool isVisible)
{
  if (isVisible!=iVisible) {
    iVisible= isVisible;
    iRedraw= ETrue;
  }
}

TBool
COggControl::IsVisible()
{
  return iVisible;
}

void
COggControl::SetDimmed(TBool aDimmed)
{
  if (aDimmed!=iDimmed) {
    iDimmed= aDimmed;
    iRedraw= ETrue;
  }
}

TBool
COggControl::IsDimmed()
{
  return iDimmed;
}

TRect
COggControl::Rect()
{
  return TRect(TPoint(ix,iy),TSize(iw,ih));
}

TSize
COggControl::Size()
{
  return TSize(iw,ih);
}

void
COggControl::SetObserver(MOggControlObserver* obs)
{
  iObserver= obs;
}

void
COggControl::PointerEvent(const TPointerEvent& p)
{
  if (iObserver && !iDimmed) iObserver->OggPointerEvent(this, p);
}

void
COggControl::ControlEvent(TInt anEventType, TInt aValue)
{
  if (iObserver && !iDimmed) iObserver->OggControlEvent(this, anEventType, aValue);
}

TKeyResponse
COggControl::KeyEvent(const TKeyEvent& /*aKeyEvent*/, TEventCode /*aType*/)
{
  return EKeyWasNotConsumed;
}

void
COggControl::SetBitmapFile(const TFileName& aFileName)
{
  iBitmapFile= aFileName;
}

TBool
COggControl::ReadInt(FILE* tf, TInt& aValue)
{
  TBuf<16> buf;
  if (fscanf(tf, "%d", aValue)!=1) return EFalse;
  return ETrue;
}

TBool
COggControl::ReadEOL(FILE* tf)
{
  if (feof(tf)) return ETrue;
  int c= fgetc(tf);
  if (c==EOF) return ETrue;
  if (c==12) return ETrue;
  if (c==13) {
    c=fgetc(tf);
    if (c==10) return ETrue;
    fseek(tf,-1,SEEK_CUR);
    return EFalse;
  }
  if (c==10) return ETrue;
  fseek(tf,-1,SEEK_CUR);
  return EFalse;
}

TBool
COggControl::ReadString(FILE* tf, TDes& aString)
{
  aString.SetLength(0);
  int c;
  bool stop;
  do {
    c= fgetc(tf);
    stop= c==EOF || c==(int)" " || c==13 || c==12;
    if (!stop) aString.Append((unsigned char)c);
  } while (!stop);
  if (c==13) fseek(tf,-1,SEEK_CUR);
  return ETrue;
}

TBool
COggControl::ReadArguments(FILE* tf, const TDesC& token)
{
  if (token==_L("POSITION")) {
    return 
      ReadInt(tf, ix) &&
      ReadInt(tf, iy) &&
      ReadInt(tf, iw) &&
      ReadInt(tf, ih);
  }
  return ETrue;
}

TBool
COggControl::ReadTokens(FILE* tf)
{
  TBuf<32> token;
  do {
    if (!ReadString(tf, token)) return EFalse;
    if (token!=KEndToken) ReadArguments(tf, token);
    ReadEOL(tf);
  } while (token!=KEndToken);
  return ETrue;
}


/***********************************************************
 *
 * COggText
 *
 ***********************************************************/

COggText::COggText(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iText(0),
  iFont(0),
  iTextWidth(0),
  iNeedsScrolling(EFalse),
  iHasScrolled(EFalse),
  iScrollDelay(30),
  iScrollSpeed(2)
{
}

COggText::~COggText()
{
  if (iText) { delete iText; iText= 0; }
}

void 
COggText::SetFont(CFont* aFont)
{
  iFont= aFont;
  iRedraw= ETrue;
}

void
COggText::SetText(const TDesC& aText)
{
  if (iText) delete iText;

  iText = HBufC::NewLC(aText.Length());
  CleanupStack::Pop();
  iText->Des().Copy(aText);

  iTextWidth= GetTextWidth(aText, iFont, iw);
  iNeedsScrolling= iTextWidth>iw;

  iCycle= 0;
  iHasScrolled= EFalse;
  iRedraw= ETrue;
}

void
COggText::ScrollNow()
{
  iHasScrolled= EFalse;
  iCycle= iScrollDelay;
}

void COggText::Cycle()
{
  if (!iNeedsScrolling) return;
  if (iHasScrolled) return;

  iCycle++;

  if (iCycle<iScrollDelay) return;

  iRedraw= ETrue;
  
  if ((iCycle-iScrollDelay)*iScrollSpeed>iTextWidth) {
    iHasScrolled= ETrue;
    iCycle= 0;
  }
}

void COggText::Draw(CBitmapContext& aBitmapContext)
{
  if (!iText) return;

  aBitmapContext.UseFont(iFont);
  aBitmapContext.SetPenColor(KRgbBlack);
  aBitmapContext.SetPenSize(TSize(2,2));

  TInt offset= (iCycle - iScrollDelay)*iScrollSpeed;
  if (offset<0) offset= 0;
  TRect	lineRect(TPoint(ix-offset,iy), TSize(iw+offset,ih));
  TPtrC	p(*iText);

  CGraphicsContext::TTextAlign a= CGraphicsContext::ECenter;
  if (iNeedsScrolling) a= CGraphicsContext::ELeft;

  aBitmapContext.DrawText(p, lineRect, iFont->AscentInPixels(), a);
}

void COggText::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);
  if (p.iType==TPointerEvent::EButton1Down) ScrollNow();
}


/***********************************************************
 *
 * COggIcon
 *
 ***********************************************************/

COggIcon::COggIcon(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iIcon(0),
  iBlinkFrequency(5),
  iBlinking(EFalse)
{
}

COggIcon::~COggIcon()
{
  if (iIcon) { delete iIcon; iIcon= 0; }
}

void
COggIcon::SetIcon(CGulIcon* anIcon)
{
  if (iIcon) delete iIcon;
  iIcon= anIcon;
  iRedraw= ETrue;
}

void
COggIcon::Blink()
{
  iBlinking= ETrue;
  iVisible= ETrue;
  iCycle= 0;
  iRedraw= ETrue;
}

void
COggIcon::Show()
{
  iBlinking= EFalse;
  iVisible= ETrue;
  iRedraw= ETrue;
}

void
COggIcon::Hide()
{
  iBlinking= EFalse;
  iVisible= EFalse;
  iRedraw= ETrue;
}

void
COggIcon::SetBlinkFrequency(TInt aFrequency)
{
  iBlinkFrequency= aFrequency;
}

void COggIcon::Cycle()
{
  if (!iBlinking) return;

  iCycle++;

  if (iCycle<iBlinkFrequency) return;

  iRedraw= ETrue;
  iVisible= !iVisible;
  iCycle= 0;
}

void COggIcon::Draw(CBitmapContext& aBitmapContext)
{
  if (!iIcon) return;

  aBitmapContext.BitBltMasked
    (TPoint(ix,iy),
     iIcon->Bitmap(),
     TRect(TPoint(0,0),iIcon->Bitmap()->SizeInPixels()),
     iIcon->Mask(),
     ETrue);
}

TBool
COggIcon::ReadArguments(FILE* tf, const TDesC& token)
{
  TBool success= COggControl::ReadArguments(tf, token);
  if (success && token==_L("BlinkFrequency"))
    success= ReadInt(tf,iBlinkFrequency);
  return success;
}

/***********************************************************
 *
 * COggButton
 *
 ***********************************************************/

COggButton::COggButton(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iActiveMask(0),
  iNormalIcon(0),
  iPressedIcon(0),
  iDimmedIcon(0),
  iState(0)
{
}

COggButton::~COggButton()
{
  if (iActiveMask) { delete iActiveMask; iActiveMask= 0; }
  if (iNormalIcon) { delete iNormalIcon; iNormalIcon= 0; }
  if (iPressedIcon) { delete iPressedIcon; iPressedIcon= 0; }
  if (iDimmedIcon) { delete iDimmedIcon; iDimmedIcon= 0; }
}

void
COggButton::SetActiveMask(const TFileName& aFileName, TInt anIdx)
{
  iActiveMask = new CFbsBitmap;
  int err= iActiveMask->Load(aFileName,anIdx);
  if (err!=KErrNone) {
    TBuf<256> buf;
    CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_1);
    buf.AppendNum(err);
    User::InfoPrint(buf);
    delete iActiveMask;
    iActiveMask= 0;
  }
}

void
COggButton::SetNormalIcon(CGulIcon* anIcon)
{
  if (iNormalIcon) delete iNormalIcon;
  iNormalIcon= anIcon;
  iRedraw= ETrue;
}

void
COggButton::SetPressedIcon(CGulIcon* anIcon)
{
  if (iPressedIcon) delete iPressedIcon;
  iPressedIcon= anIcon;
  iRedraw= ETrue;
}

void
COggButton::SetDimmedIcon(CGulIcon* anIcon)
{
  if (iDimmedIcon) delete iDimmedIcon;
  iDimmedIcon= anIcon;
  iRedraw= ETrue;
}

void
COggButton::DrawCenteredIcon(CBitmapContext& aBitmapContext, CGulIcon* anIcon)
{
  TSize s(anIcon->Bitmap()->SizeInPixels());
  TPoint p(ix+iw/2-s.iWidth/2,iy+ih/2-s.iHeight/2);
  aBitmapContext.BitBltMasked(p,anIcon->Bitmap(), TRect(TPoint(0,0),s), anIcon->Mask(), ETrue);
}

void 
COggButton::Draw(CBitmapContext& aBitmapContext)
{  
  if (iDimmed) {
    if (iDimmedIcon) DrawCenteredIcon(aBitmapContext,iDimmedIcon);
    else if (iNormalIcon) DrawCenteredIcon(aBitmapContext,iNormalIcon);
  }
  else {
    if (iState==0) {
      if (iNormalIcon) DrawCenteredIcon(aBitmapContext,iNormalIcon);
    }
    if (iState==1) {
      if (iPressedIcon) DrawCenteredIcon(aBitmapContext,iPressedIcon);
      else if (iNormalIcon) DrawCenteredIcon(aBitmapContext,iNormalIcon);
    }
  }
}

void COggButton::PointerEvent(const TPointerEvent& p)
{
  if (iDimmed || !iVisible) return;

  if (p.iType==TPointerEvent::EButton1Down) {
    iState= 1;
    iRedraw= ETrue;
  }

  if (p.iType==TPointerEvent::EButton1Up) {
    iState= 0;
    iRedraw= ETrue;
  }

  COggControl::PointerEvent(p);
}

CGulIcon*
COggButton::CreateIcon(FILE* tf)
{
  TInt idxBitmap,idxMask;
  if (!ReadInt(tf,idxBitmap)) return NULL;
  if (!ReadInt(tf,idxMask)) return NULL;
  return CEikonEnv::Static()->CreateIconL(iBitmapFile, idxBitmap, idxMask);
}

TBool
COggButton::ReadArguments(FILE* tf, const TDesC& token)
{
  if (token==_L("NormalIcon")) SetNormalIcon(CreateIcon(tf));
  else if (token==_L("PressedIcon")) SetPressedIcon(CreateIcon(tf));
  else if (token==_L("DimmedIcon")) SetDimmedIcon(CreateIcon(tf));
  return COggControl::ReadArguments(tf, token);
}


/***********************************************************
 *
 * COggSlider
 *
 ***********************************************************/

COggSlider::COggSlider(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iKnobIcon(0),
  iStyle(0),
  iValue(0),
  iMaxValue(100),
  iIsMoving(EFalse),
  iPos(0)
{
}

void
COggSlider::SetKnobIcon(CGulIcon* anIcon)
{
  if (iKnobIcon) delete iKnobIcon;
  iKnobIcon= anIcon;
  iRedraw= ETrue;
}

void
COggSlider::SetStyle(TInt aStyle)
{
  iStyle= aStyle;
  iRedraw= ETrue; 
}

void
COggSlider::SetValue(TInt aValue)
{
  if (aValue==iValue) return;

  if (aValue<0) aValue=0;
  else if (aValue>iMaxValue) aValue= iMaxValue;

  iValue= aValue;
  iRedraw= (GetPosFromValue(iValue)!=iPos); 
}

void
COggSlider::SetMaxValue(TInt aMaxValue)
{
  iMaxValue= aMaxValue;
  iRedraw= ETrue;
}

TInt
COggSlider::CurrentValue()
{
  return iValue;
}

void 
COggSlider::Draw(CBitmapContext& aBitmapContext)
{
  if (!iKnobIcon) return;

  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  TRect r(TPoint(0,0),s);
  TPoint p(ix,iy);

  iPos= GetPosFromValue(iValue);

  switch (iStyle) {
  case 0: r.iBr.iX= iPos; break;
  case 1: r.iTl.iY= iPos; break;
  case 2: p.iX= iPos; break;
  case 3: p.iY= iPos; break;
  }

  aBitmapContext.BitBltMasked
    (p,
     iKnobIcon->Bitmap(),
     r,
     iKnobIcon->Mask(),
     ETrue);
}

TInt COggSlider::GetPosFromValue(TInt aValue)
{
  if (!iKnobIcon) return 0;
  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  switch (iStyle) {
  case 0: return (float)iw/iMaxValue*aValue; break;
  case 1: return (float)ih/iMaxValue*aValue; break;
  case 2: return ix+(float)(iw-s.iWidth)/iMaxValue*aValue; break;
  case 3: return iy+(ih-s.iHeight) - (float)(ih-s.iHeight)/iMaxValue*aValue; break;
  }
  return 0;
}

TInt COggSlider::GetValueFromPos(TInt aPos)
{
  if (!iKnobIcon) return 0;
  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  switch (iStyle) {
  case 0: return (float)aPos*iMaxValue/iw; break;
  case 1: return (float)aPos*iMaxValue/ih; break;
  case 2: return (float)(aPos-s.iWidth/2)*iMaxValue/(iw-s.iWidth); break;
  case 3: return (float)((ih-s.iHeight) - (aPos-s.iHeight/2))*iMaxValue/(ih-s.iHeight); break;
  }
  return 0;
}

void COggSlider::PointerEvent(const TPointerEvent& p)
{
  if (iDimmed) return;
  if (p.iType==TPointerEvent::EButton1Down) iIsMoving= ETrue;
  if (iIsMoving) {
    TPoint pt(p.iPosition);
    pt.iX-= ix;
    pt.iY-= iy;
    switch (iStyle) {
    case 0: SetValue(GetValueFromPos(pt.iX)); break;
    case 1: SetValue(GetValueFromPos(pt.iY)); break;
    case 2: SetValue(GetValueFromPos(pt.iX)); break;
    case 3: SetValue(GetValueFromPos(pt.iY)); break;
    }
  }
  if (p.iType==TPointerEvent::EButton1Up) {
    iIsMoving= EFalse;
    iRedraw= ETrue;
  }
  COggControl::PointerEvent(p);
}


/***********************************************************
 *
 * COggScrollBar
 *
 ***********************************************************/

COggScrollBar::COggScrollBar(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iStyle(0),
  iValue(0),
  iMaxValue(100),
  iAssociated(0),
  iKnobIcon(0),
  iScrollerSize(10),
  iPos(0),
  iPage(1),
  iIsMoving(EFalse)
{
}

void
COggScrollBar::SetStyle(TInt aStyle)
{
  iStyle= aStyle;
  iRedraw= ETrue;
}

void
COggScrollBar::SetKnobIcon(CGulIcon* anIcon)
{
  iKnobIcon= anIcon;
  iRedraw= ETrue;
}

void
COggScrollBar::SetScrollerSize(TInt aSize)
{
  iScrollerSize= aSize;
  iRedraw= ETrue;
}

void
COggScrollBar::SetPage(TInt aPage)
{
  iPage= aPage;
}

void
COggScrollBar::SetAssociatedControl(COggControl* aControl)
{
  iAssociated= aControl;
}

void
COggScrollBar::Draw(CBitmapContext& aBitmapContext)
{
  if (!iKnobIcon) return;

  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  TRect r(TPoint(0,0),s);
  TPoint p(ix,iy);

  iPos= GetPosFromValue(iValue);

  switch (iStyle) {
  case 0: p.iX= iPos; break;
  case 1: p.iY= iPos; break;
  }

  aBitmapContext.BitBltMasked
    (p,
     iKnobIcon->Bitmap(),
     r,
     iKnobIcon->Mask(),
     ETrue);
}

void 
COggScrollBar::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);
  if (!iKnobIcon) return;

  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  TInt k,s1,s2;
  if (iStyle==0) {
    k= s.iWidth;
    s1= p.iPosition.iX-ix;
    s2= p.iPosition.iX;
  } else {
    k= s.iHeight;
    s1= p.iPosition.iY-iy;
    s2= p.iPosition.iY;
  }

  if (p.iType==TPointerEvent::EButton1Up) iIsMoving= EFalse;

  if (p.iType==TPointerEvent::EButton1Down) {
    if (s1<iScrollerSize) {
      SetValue(iValue-1);
    } else if (s1>ih-iScrollerSize) {
      SetValue(iValue+1);
    } else if (s2>=iPos && s2<=iPos+k) {
      iIsMoving= ETrue;
    } else if (s2<iPos) {
      SetValue(iValue-iPage+1);
    } else if (s2>iPos+k) {
      SetValue(iValue+iPage-1);
    }
  }

  if (iIsMoving) SetValue(GetValueFromPos(s2));
}

void 
COggScrollBar::SetMaxValue(TInt aMaxValue)
{
  if (iMaxValue!=aMaxValue) {
    iMaxValue= aMaxValue;
    iRedraw= ETrue;
  }
}

void 
COggScrollBar::SetValue(TInt aValue)
{
  if (aValue<0) aValue=0;
  else if (aValue>iMaxValue) aValue= iMaxValue;
  if (iValue!=aValue) {
    iValue= aValue;
    iRedraw= (GetPosFromValue(aValue)!=iPos);
    if (iAssociated) iAssociated->ControlEvent(0, aValue);
  }
}

TInt 
COggScrollBar::GetPosFromValue(TInt aValue)
{
  if (!iKnobIcon || iMaxValue==0) return 0;
  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  switch (iStyle) {
  case 0: return ix+iScrollerSize+(float)(iw-s.iWidth-2*iScrollerSize)/iMaxValue*aValue; break;
  case 1: return iy+iScrollerSize+(float)(ih-s.iHeight-2*iScrollerSize)/iMaxValue*aValue; break;
  }
  return 0;
}

TInt 
COggScrollBar::GetValueFromPos(TInt aPos)
{
  if (!iKnobIcon) return 0;
  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  switch (iStyle) {
  case 0: return (float)(aPos-ix-s.iWidth/2-iScrollerSize)*iMaxValue/(iw-s.iWidth-2*iScrollerSize); break;
  case 1: return (float)(aPos-iy-iScrollerSize-s.iHeight/2)*iMaxValue/(ih-s.iHeight-2*iScrollerSize); break;
  }
  return 0;
}


/***********************************************************
 *
 * COggAnalyzer
 *
 ***********************************************************/

COggAnalyzer::COggAnalyzer(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iBarIcon(0),
  iNumValues(16),
  iStyle(1)
{
  iValues= new TInt[iNumValues];
  iPeaks= new TInt[iNumValues];
  iDx= (float)iw/iNumValues;
}

COggAnalyzer::~COggAnalyzer()
{
  if (iValues) { delete[] iValues; iValues= 0; }
  if (iPeaks) { delete[] iPeaks; iPeaks= 0; }
  if (iBarIcon) { delete iBarIcon; iBarIcon= 0; }
}

void
COggAnalyzer::SetBarIcon(CGulIcon* aBarIcon)
{
  if (iBarIcon) delete iBarIcon;
  iBarIcon= aBarIcon;
  iRedraw= ETrue;
}

void
COggAnalyzer::SetValue(TInt i, TInt theValue)
{
  if (iValues[i]!=theValue) {
    iValues[i]= theValue;
    if (theValue>iPeaks[i]) iPeaks[i]= iValues[i];
    iRedraw= ETrue;
  }
}

void
COggAnalyzer::SetStyle(TInt aStyle)
{
  iStyle= aStyle;
  iRedraw= ETrue;
}

TInt
COggAnalyzer::Style()
{
  return iStyle;
}

void
COggAnalyzer::Cycle()
{
  if (iStyle!=2) return;

  iCycle++;

  if (iCycle%10==0) {
    iCycle= 0;
    for (int i=0; i<iNumValues; i++) {
      if (iPeaks[i]>0 || iPeaks[i]!=iValues[i]) {
	if (iPeaks[i]!=iValues[i]) { 
	  iPeaks[i]= iValues[i];
	  iRedraw= ETrue;
	} 
      }
    }
  }
  else {
    for (int i=0; i<iNumValues; i++) {
      if (iValues[i]>iPeaks[i]) { 
	iPeaks[i]= iValues[i];
	iRedraw= ETrue;
      } 
    }
  }
}

void
COggAnalyzer::Draw(CBitmapContext& aBitmapContext)
{

  if (iBarIcon) {
    TInt x= ix;
    TSize s(iBarIcon->Bitmap()->SizeInPixels());
    for (int i=0; i<iNumValues; i++) {
      TRect rc(TPoint(0,ih-iValues[i]), s);
      aBitmapContext.BitBltMasked(TPoint(x,iy+ih-iValues[i]),iBarIcon->Bitmap(), rc, iBarIcon->Mask(), ETrue);
      x+= iDx;
    }
  }
  else {
    TRect r(TPoint(ix,iy),TSize(iDx-1,ih));
    aBitmapContext.SetBrushColor(KRgbBlue);
    for (int i=0; i<iNumValues; i++) {
      r.iTl.iY= iy+ih-iValues[i];
      aBitmapContext.Clear(r);
      r.Move(iDx,0);
    }
  }

  if (iStyle==2) {  
    aBitmapContext.SetBrushColor(KRgbDarkBlue);
    TRect p(TPoint(ix,iy),TSize(iDx-1,ih));
    for (int i=0; i<iNumValues; i++) {
      p.iTl.iY= iy+ih-iPeaks[i]-1;
      p.iBr.iY= p.iTl.iY+2;
      aBitmapContext.Clear(p);
      p.Move(iDx, 0);
    }
  }
}

void COggAnalyzer::RenderWaveform(short int data[2][512])
{
  if (iStyle==0) return;

  for (int i=0; i<512; i++) {
    iFFTRe[i]= data[0][i];
    iFFTIm[i]= 0;
  }
  window(iFFTRe, 512);
  fix_fft( iFFTRe, iFFTIm, 9, 0 );
  fix_loud( iFFTAbs, iFFTRe, iFFTIm, 256, 0);

  RenderFrequencies(iFFTAbs);
}

void COggAnalyzer::RenderFrequencies(short int data[256])
{
  if (iStyle==0) return;

  int i,c;
  int y;

  static const int xscale[17] = { 0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 250 };

  for(i = 0; i < iNumValues; i++) {

    for(c = xscale[i], y = 10; c < xscale[i + 1]; c++)
      if (data[c] < y) y= data[c];

    iValues[i]= (float)(y+100.)/110.*ih;
  }
  
  iRedraw= ETrue;
}

void COggAnalyzer::Clear()
{
  for (int i=0; i<iNumValues; i++) {
    if (iValues[i]!=0) {
      iValues[i]= 0;
      iRedraw= ETrue;
    }
  }
}

void COggAnalyzer::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);
  if (p.iType==TPointerEvent::EButton1Down) {
    iStyle= (iStyle+1)%3;
    iRedraw= ETrue;
    if (iStyle==0) Clear();
  }
}


/***********************************************************
 *
 * COggListBox
 *
 ***********************************************************/

COggListBox::COggListBox(TInt ax, TInt ay, TInt aw, TInt ah) :
  COggControl(ax, ay, aw, ah),
  iFont(0),
  iTop(0),
  iSelected(-1),
  iScroll(0),
  iOffset(0),
  iScrollBar(0)
{
  iText= new CDesCArrayFlat(10);
  iData= CColumnListBoxData::NewL();
  iLineHeight= 16;
  iLinesVisible= ih/iLineHeight;
}

COggListBox::~COggListBox()
{
  if (iText) {
    ClearText();
    delete iText; 
    iText= 0;
  }
  if (iData) {
    delete iData;
    iData=0;
  }
}

void
COggListBox::SetFont(CFont* aFont)
{
  iFont= aFont;
  iRedraw= ETrue;
}

CColumnListBoxData*
COggListBox::GetColumnListBoxData()
{
  return iData;
}

void
COggListBox::Redraw()
{
  UpdateScrollBar();
  COggControl::Redraw();
}

void
COggListBox::ClearText()
{
  iText->Reset();
  iRedraw= ETrue;
  UpdateScrollBar();
}

void
COggListBox::AppendText(const TDesC& aText)
{
  iText->AppendL(aText);
  iRedraw= ETrue;
  UpdateScrollBar();
}

TInt
COggListBox::CountText()
{
  if (!iText) return 0;
  else return iText->Count();
}

const TDesC&
COggListBox::GetText(TInt idx)
{
  return (*iText)[idx];
}

CDesCArray*
COggListBox::GetTextArray()
{
  return iText;
}

void
COggListBox::ScrollBy(TInt nLines)
{
  SetTopIndex(iTop+nLines);
}

TInt
COggListBox::CurrentItemIndex()
{
  return iSelected;
}

void 
COggListBox::SetCurrentItemIndex(TInt idx)
{
  if (idx<0) idx=-1;
  else if (idx>=iText->Count()) idx= iText->Count()-1;
  if (idx!=iSelected) {
    iSelected= idx;
    iRedraw= ETrue;
    if (idx<iTop) SetTopIndex(idx);
    else if (idx>=iTop+iLinesVisible) SetTopIndex(idx-iLinesVisible+1);
    if (iObserver) iObserver->OggControlEvent(this,1,idx);
  }
}

void
COggListBox::SetTopIndex(TInt idx)
{
  if (idx<0) idx=0;
  if (idx!=iTop) {
    iCycle= 5;
    iScroll= (iTop-idx);
    iOffset= iScroll*iLineHeight;
    iTop= idx;
    iRedraw= ETrue;
    UpdateScrollBar();
    if (iObserver) iObserver->OggControlEvent(this,0,idx);
  }
}

void
COggListBox::SetVertScrollBar(COggScrollBar* aScrollBar)
{
  iScrollBar= aScrollBar;
  UpdateScrollBar();
}

void
COggListBox::UpdateScrollBar()
{
  if (iScrollBar && iText) {
    TInt max(iText->Count()-iLinesVisible);
    if (max<0) max=0;
    iScrollBar->SetMaxValue(max);
    iScrollBar->SetValue(iTop);
    iScrollBar->SetPage(iLinesVisible);
  }
}

TInt
COggListBox::GetLineFromPos(TInt aPos)
{
  return (aPos/iLineHeight)+iTop;
}

TInt
COggListBox::GetColumnFromPos(TInt aPos)
{
  if (!iData) return 0;
  for (TInt j=0; j<iData->LastColumn(); j++) {
    if (aPos>=0 && aPos<iData->ColumnWidthPixel(j)) {
      return j;
    }
    aPos-= iData->ColumnWidthPixel(j);
  }
  return -1;
}

void
COggListBox::Draw(CBitmapContext& aBitmapContext)
{
  if (!iFont) return;
  if (!iData) return;
  if (!iText) return;

  aBitmapContext.UseFont(iFont);
  aBitmapContext.SetPenSize(TSize(1,1));

  TInt y= iy - iLineHeight;
  TInt x;
  TInt len,idx;

  for (TInt i=iTop+iOffset/iLineHeight-1; i<iText->Count(); i++) {

    if (i<0) {
      y+= iLineHeight;
      continue;
    }

    TPtrC p((*iText)[i]);
    x= ix;

    if (i==iSelected)
      aBitmapContext.SetPenColor(KRgbRed);
    else
      aBitmapContext.SetPenColor(KRgbBlack);

    for (TInt j=0; j<iData->LastColumn(); j++) {

      len= p.Locate(KColumnListSeparator);

      if (iData->ColumnIsGraphics(j) && iData->IconArray()) {
	TLex lex(p.Left(len));
	if (lex.Val(idx)==KErrNone) {
	  CGulIcon* icn= (*iData->IconArray())[idx];
	  TSize s(icn->Bitmap()->SizeInPixels());
	  TPoint pt(x+iData->ColumnWidthPixel(j)/2-s.iWidth/2,y+iLineHeight/2-s.iHeight/2-iOffset%iLineHeight);
	  aBitmapContext.BitBltMasked(pt,icn->Bitmap(), TRect(TPoint(0,0),s), icn->Mask(), ETrue);
	}
      }
      else {
	TRect lineRect(TPoint(x,y+iLineHeight/2-iFont->HeightInPixels()/2-iOffset%iLineHeight), 
		       TSize(iData->ColumnWidthPixel(j),iLineHeight));
	aBitmapContext.DrawText(p.Left(len), lineRect, iFont->AscentInPixels(), iData->ColumnAlignment(j));
      }

      p.Set(p.Mid(len+1));
      x+= iData->ColumnWidthPixel(j);
      x+= iData->ColumnHorizontalGap(j);

    }

    y+= iLineHeight;
    if (y>iy+ih) break;
  }
}

void
COggListBox::Cycle()
{
  if (iCycle>0) {
    iCycle--;
    iOffset -= (iScroll*iLineHeight)/5;
    iRedraw= ETrue;
  } else {
    if (iOffset!=0) {
      iOffset= 0;
      iRedraw= ETrue;
    }
  }
}

void
COggListBox::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);
  if (p.iType==TPointerEvent::EButton1Down /*|| p.iType==TPointerEvent::EDrag*/) {
    TInt idx= GetLineFromPos(p.iPosition.iY-iy);
    SetCurrentItemIndex(idx);
    if (iObserver) iObserver->OggControlEvent(this,2,idx);
    iRedraw= ETrue;
  } 
}

void
COggListBox::ControlEvent(TInt anEventType, TInt aValue)
{
  COggControl::ControlEvent(anEventType, aValue);
  if (anEventType==0) SetTopIndex(aValue);
}


/***********************************************************
 *
 * COggCanvas
 *
 ***********************************************************/

COggCanvas::COggCanvas() :
  if2bm(0),
  iBackground(0),
  iControls(10),
  iGrabbed(0),
  iFocused(0)
{
}

COggCanvas::~COggCanvas()
{
  DestroyBitmap();
}

void COggCanvas::ConstructL(const TFileName& aFileName, TInt iIdx)
{
  if2bm = CMdaImageFileToBitmapUtility::NewL(*this);
  iBackground = new CFbsBitmap;

  //if2bm->OpenL(aFileName,new TMdaBmpClipFormat());
  //CEikEnv::CreateBitmapL()

  int err= iBackground->Load(aFileName,iIdx);
  if (err!=KErrNone) {
    TBuf<256> buf;
    CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_21);
    buf.AppendNum(err);
    User::InfoPrint(buf);
    delete iBackground;
    iBackground= 0;
  }

  //SetGloballyCapturing(EFalse);
  //SetPointerCapture(EFalse);
  EnableDragEvents();

  DrawControl();
}

void COggCanvas::MiuoOpenComplete(TInt aError)
{
  if (aError==KErrNone) if2bm->ConvertL(*iBackground);
  else {
    TBuf<256> buf;
    CEikonEnv::Static()->ReadResource(buf,R_OGG_ERROR_21);
    buf.AppendNum(aError);
    User::InfoPrint(buf);
    delete iBackground;
    iBackground= 0;
  }
}

void COggCanvas::MiuoConvertComplete(TInt aError)
{
  if (aError!=KErrNone) {
    TBuf<256> buf;
    CEikonEnv::Static()->ReadResource(buf,R_OGG_ERROR_2);
    buf.AppendNum(aError);
    User::InfoPrint(buf);
    delete iBackground;
    iBackground= 0;
  }
}

void COggCanvas::MiuoCreateComplete(TInt /*aError*/)
{
}

void
COggCanvas::Refresh()
{
  CycleControls();
  for (int i=0; i<iControls.Count(); i++) 
    if (iControls[i]->iRedraw)
      Window().Invalidate(iControls[i]->Rect());
}

void
COggCanvas::Invalidate()
{
  DrawControl();
  Window().Invalidate();
}

COggControl*
COggCanvas::GetControl(TInt i)
{
  return iControls[i];
}

void
COggCanvas::AddControl(COggControl* c)
{
  iControls.AppendL(c);
}

void COggCanvas::ClearControls()
{
  for (TInt i=0; i<iControls.Count(); i++) delete iControls[i];
  iControls.Reset();
}

void COggCanvas::DrawControl()
{
  if (iBitmap) {

    iBitmapContext->SetClippingRect(Rect());
    if (iBackground) {
      iBitmapContext->BitBlt(TPoint(0,0),iBackground);
    } else {
      iBitmapContext->SetBrushColor(KRgbWhite);
      iBitmapContext->Clear(Rect());
    }

    // DrawControl will draw relative to its Position().
    // when drawing to the bitmap gc, Position() should be (0,0)
    TPoint	position = iPosition;
    iPosition = TPoint(0,0);
    for (int i=0; i<iControls.Count(); i++) iControls[i]->iRedraw= ETrue;
    DrawControl(*iBitmapContext, *iBitmapDevice);
    for (int j=0; j<iControls.Count(); j++) iControls[j]->iRedraw= EFalse;
    iPosition = position;
  }
}

void COggCanvas::SizeChanged()
{
  DestroyBitmap();
  TRAPD(err, CreateBitmapL(Size()));
  if (err)
    DestroyBitmap();
  else
    DrawControl();
}

void COggCanvas::DestroyBitmap()
{
  delete iBitmapContext;
  iBitmapContext = NULL;
  delete iBitmapDevice;
  iBitmapDevice = NULL;
  delete iBitmap;
  iBitmap = NULL;
}

void COggCanvas::CreateBitmapL(const TSize& aSize)
{
  iBitmap = new (ELeave) CFbsBitmap;
  iBitmap->Create(aSize, iEikonEnv->ScreenDevice()->DisplayMode());
  
  iBitmapDevice = CFbsBitmapDevice::NewL(iBitmap);
  iBitmapDevice->CreateContext(iBitmapContext);
}

void COggCanvas::Draw(const TRect& aRect) const
{
  CWindowGc& gc=SystemGc();
  
  if (iBitmap) {
    DrawControl(*iBitmapContext, *iBitmapDevice);
    gc.BitBlt(aRect.iTl, iBitmap, TRect(aRect.iTl - Position(), aRect.Size()));
  } else {
    CWsScreenDevice* screenDevice = iCoeEnv->ScreenDevice();
    DrawControl(gc, *screenDevice);
  }

  for (int i=0; i<iControls.Count(); i++) iControls[i]->iRedraw= EFalse;
}

void COggCanvas::DrawControl(CBitmapContext& aBitmapContext, CBitmapDevice& /*aBitmapDevice*/) const
{
  // clear backgrounds
  for (int i=0; i<iControls.Count(); i++) {
    COggControl* c= iControls[i];
    if (c->iRedraw) {
      aBitmapContext.SetClippingRect(Rect());
      if (iBackground) 
        aBitmapContext.BitBlt(TPoint(c->ix,c->iy),iBackground,TRect(TPoint(c->ix,c->iy),TSize(c->iw,c->ih)));
      else {
	aBitmapContext.SetBrushColor(KRgbWhite);
	aBitmapContext.Clear(TRect(TPoint(c->ix,c->iy),TSize(c->iw,c->ih)));
      }
    }
  }

  // redraw controls
  for (int k=0; k<iControls.Count(); k++) {
    COggControl* c= iControls[k];
    if (c->iRedraw && c->IsVisible()) {
      aBitmapContext.SetClippingRect(c->Rect());
      c->Draw(aBitmapContext);
    }
  }
}

TKeyResponse COggCanvas::OfferKeyEventL(const TKeyEvent& /*aKeyEvent*/, TEventCode /*aType*/)
{
  return EKeyWasNotConsumed;
}

void COggCanvas::HandlePointerEventL(const TPointerEvent& aPointerEvent)
{
  if (iGrabbed) iGrabbed->PointerEvent(aPointerEvent);

  if (!iGrabbed && aPointerEvent.iType==TPointerEvent::EButton1Down) {
    for (int i=0; i<iControls.Count(); i++) {
      COggControl* c= iControls[i];
      if (c->IsVisible() && !c->IsDimmed() && c->Rect().Contains(aPointerEvent.iPosition)) {
	iGrabbed= c;
	c->PointerEvent(aPointerEvent);
	return;
      }
    }
  }

  if (aPointerEvent.iType==TPointerEvent::EButton1Up) iGrabbed= 0;
}

void COggCanvas::CycleControls()
{
  for (int i=0; i<iControls.Count(); i++)
    iControls[i]->Cycle();
}
