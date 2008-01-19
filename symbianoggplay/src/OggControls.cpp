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
#include "OggLog.h"
#include "OggControls.h"
#include "OggMsgEnv.h"

#include <OggPlay.rsg>

#include <eikenv.h>
#include <flogger.h>

#include "int_fft.c"

const TInt KLengthFFT = 512;

#if defined(SERIES60)
const TInt KListboxcycles=4;
#else
const TInt KListboxcycles=5;
#endif

TInt GetTextWidth(const TDesC& aText, CFont* aFont, TInt w)
{
  TInt pos = aFont->TextCount(aText, w);
  while (pos<aText.Length()) {
    w+= 16;
    pos = aFont->TextCount(aText, w);
  }
  return w;
}


_LIT(KBeginToken,"{");
_LIT(KEndToken,"}");
TOggParser::TOggParser(RFs& aFs, const TFileName& aFileName)
{
  iLine = 1;
  iDebug = EFalse; /* ETrue; */
  iBuffer = NULL;

  RFile file;
  TInt err = file.Open(aFs, aFileName, EFileShareReadersOnly);
  if (err == KErrNone)
	{
	TInt size;
	err = file.Size(size);
	if (err == KErrNone)
		{
		iBuffer = HBufC8::New(size);
		if (iBuffer)
			{
			TPtr8 bufDes(iBuffer->Des());
			err = file.Read(bufDes);

			iBufferPtr = iBuffer->Ptr();
			}
		else
			err = KErrNoMemory;
		}

	file.Close();
	}

  iState = (err == KErrNone) ? ESuccess : EFileNotFound;
}

TOggParser::~TOggParser()
	{
	delete iBuffer;
	}

TBool TOggParser::ReadHeader()
{
  ReadToken();
  if (iToken!=_L("OggPlay")) {
    iState= ENoOggSkin;
    return EFalse;
  }
  ReadToken(iVersion);
  if (iVersion!=1) {
    iState= EUnknownVersion;
    return EFalse;
  }
  ReadToken();
  if (iToken!=KBeginToken) {
    iState= EBeginExpected;
    return EFalse;
  }
  return ETrue;
}

TBool TOggParser::ReadToken()
{
  // Read next token. Skip any white space at the beginning.
  // Read possible EOL characters if any.
  iToken.SetLength(0);

  TInt c;
  TInt bufferLength = iBuffer->Length();
  const TUint8* bufferBase = iBuffer->Ptr();
  TBool start(ETrue), stop(EFalse), ignoreWS(EFalse);
  do {
	c = -1;
	if ((iBufferPtr - bufferBase) == bufferLength)
		break;

    c = *iBufferPtr;
	iBufferPtr++;

    // Skip initial white space (32 = Space, 9 = Tab)
    if (start && ((c==32) || (c==9)))
		continue;

	// Check for opening "
	if (start && (c==34))
	{
		ignoreWS = ETrue;
		start = EFalse;
		continue;
	}

	// Interpret the character 
    start = EFalse;
    stop = (c==13) || (c==10) || (c==12);
	if (!stop)
	{
		// Check for closing " or white space
		if (ignoreWS)
			stop = (c==34);
		else
			stop = (c==32) || (c == 9);
	}

    if (!stop)
		iToken.Append((unsigned char) c);
  } while (!stop);

  TBool eol = c != 32;
  if (c == 13)
	{
	// Skip the next character
	if ((iBufferPtr - bufferBase) != bufferLength)
		iBufferPtr++;
	}

  if (eol)
	  iLine++;

  return iToken.Length()>0;
}

TBool TOggParser::ReadToken(TInt& aValue)
{
  TBool success= ReadToken();
  if (success) {
    TLex parse(iToken);
    success= parse.Val(aValue)==KErrNone;
    if (!success) iState= EIntegerExpected;
  }
  return success;
}

void ScaleBitmapL(CFbsBitmap* aDst, CFbsBitmap* aSrc, TInt aScaleFactor)
{
  TSize origSize = aSrc->SizeInPixels();
  TDisplayMode origDM = aSrc->DisplayMode();
  TSize dblSize = TSize(aScaleFactor*origSize.iWidth, aScaleFactor*origSize.iHeight);
  User::LeaveIfError(aDst->Create(dblSize, origDM));

  CFbsBitmapDevice* device = CFbsBitmapDevice::NewL(aDst);
  CleanupStack::PushL(device);

  CFbsBitGc *gc;
  User::LeaveIfError(device->CreateContext(gc));
  CleanupStack::PushL(gc);

  gc->DrawBitmap(TRect(dblSize), aSrc, TRect(origSize));
  CleanupStack::PopAndDestroy(2, device);
}

CGulIcon* TOggParser::ReadIcon(const TFileName& aBitmapFile)
	{
	TInt bitmapIdx, maskIdx;
	if (!ReadToken(bitmapIdx))
		return NULL; // Fix me, just leave (and sort out what the AppView and Canvas do if there are skin errors!)

	if (!ReadToken(maskIdx))
		return NULL;

	CGulIcon* result = CEikonEnv::Static()->CreateIconL(aBitmapFile, bitmapIdx, maskIdx);
	if (!result) // FIXME (This won't be NULL, so all this error handling is redundant)
		iState= EBitmapNotFound;

	return result;
	}

CFbsBitmap* TOggParser::ReadBitmap(const TFileName& aBitmapFile)
	{
	TInt bitmapIdx;
	if (!ReadToken(bitmapIdx))
		return NULL;

	CFbsBitmap* bitmap = new(ELeave) CFbsBitmap;
	User::LeaveIfError(bitmap->Load(aBitmapFile, bitmapIdx));

	return bitmap;
	}

TBool TOggParser::ReadColor(TRgb& aColor)
	{
	TInt red(0), green(0), blue(0);
	TBool success = ReadToken(red) && ReadToken(green) && ReadToken(blue);
	if (success)
		{
		aColor.SetRed(red);
		aColor.SetGreen(green);
		aColor.SetBlue(blue);
		}

	return success;
	}

CFont* TOggParser::ReadFont()
	{
	if (!ReadToken())
		return 0;

	if (iToken.Length()>32)
		return 0;

	TBuf<32> name(iToken);
	TInt size(12);
	if (!ReadToken(size))
		return 0;

	TInt style(0);
	if (!ReadToken(style))
		return 0;

	CFont* result = NULL;
	TFontSpec fs(name, size);
	if (style == 1)
		fs.iFontStyle.SetStrokeWeight(EStrokeWeightBold);

#if defined(SERIES60SUI)
	fs.iFontStyle.SetBitmapType(EAntiAliasedGlyphBitmap);
#endif

	CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(result, fs);
	return result;
	}

void TOggParser::ReportError()
	{
	if (iState == ESuccess)
		return;

	TBuf<128> buf;
	buf.Append(_L("Line "));
	buf.AppendNum(iLine);
	buf.Append(_L(": "));

	switch (iState)
		{
		case EFileNotFound: buf.Append(_L("File not found.")); break;
		case ENoOggSkin: buf.Append(_L("This is not an Ogg skin file!")); break;
		case EUnknownVersion: buf.Append(_L("Unknown skin file version.")); break;
		case EFlipOpenExpected: buf.Append(_L("FlipOpen statement expected.")); break;
		case ESyntaxError: buf.Append(_L("Syntax error.")); break;
		case EBeginExpected: buf.Append(_L("{ expected.")); break;
		case EEndExpected: buf.Append(_L("} expected.")); break;
		case EBitmapNotFound: buf.Append(_L("Bitmap not found.")); break;
		case EIntegerExpected: buf.Append(_L("Integer number expected.")); break;
		case EOutOfRange: buf.Append(_L("Number is out of the valid range.")); break;
		default: buf.Append(_L("Unknown error.")); break;
		}

	buf.Append(_L("Last token: <"));
	buf.Append(iToken);
	buf.Append(_L(">"));

	TRACEF(buf);
	}

void TOggParser::Debug(const TDesC& txt)
	{
	if (!iDebug)
		return;

	TBuf<256> buf;
	buf.Append(_L("Debug ("));
	buf.AppendNum(iLine);
	buf.Append(_L("): "));
	buf.Append(txt);
	buf.Append(_L(" Token:"));
	buf.Append(iToken); 

	TRACEF(buf);
	}


COggControl::COggControl()
: iRedraw(ETrue), iVisible(ETrue)
	{
	}

COggControl::COggControl(TBool aHighFrequency)
: iRedraw(ETrue), iVisible(ETrue), iHighFrequency(aHighFrequency)
	{
	}

COggControl::~COggControl()
	{
	delete iFocusIcon;
	delete iOverlayMask;
	}

void COggControl::Cycle()
	{
	}

void COggControl::SetPosition(TInt ax, TInt ay, TInt aw, TInt ah)
	{
	ix = ax;
	iy = ay;
	iw = aw;
	ih = ah;
	
	iRedraw = ETrue;
	}

void COggControl::Redraw(TBool doRedraw)
	{
	iRedraw = doRedraw;
	}

void COggControl::MakeVisible(TBool aVisible)
	{
	if (aVisible != iVisible)
		{
		iVisible = aVisible;
		iRedraw = ETrue;
		}
	}

TBool COggControl::IsVisible()
	{
	return iVisible;
	}

void COggControl::SetDimmed(TBool aDimmed)
	{
	if (aDimmed != iDimmed)
		{
		iDimmed = aDimmed;
		iRedraw = ETrue;
		}
	}

void COggControl::SetFocus(TBool aFocus)
	{
	//__ASSERT_DEBUG(iAcceptsFocus,OGGLOG.Write(_L("Assert: OggControl::SetFocus - asked to set focus, but should never accept it")));
	if (aFocus != iFocus)
		{
		iFocus = aFocus;
		iRedraw = ETrue;
		}
	}

TBool COggControl::Focus()
	{
	return iFocus;
	}


TBool COggControl::IsDimmed()
	{
	return iDimmed;
	}

TRect COggControl::Rect()
	{
	return TRect(TPoint(ix,iy), TSize(iw,ih));
	}

TSize COggControl::Size()
	{
	return TSize(iw,ih);
	}

void COggControl::SetObserver(MOggControlObserver* obs)
	{
	iObserver = obs;
	}

void COggControl::PointerEvent(const TPointerEvent& p)
	{
	if (iObserver && !iDimmed)
		iObserver->OggPointerEvent(this, p);
	}

void COggControl::ControlEvent(TInt anEventType, TInt aValue)
	{
	if (iObserver && !iDimmed)
		iObserver->OggControlEvent(this, anEventType, aValue);
	}


void COggControl::SetBitmapFile(const TFileName& aFileName)
	{
	iBitmapFile = aFileName;
	}

void COggControl::SetFocusIcon(CGulIcon* anIcon)
	{
	iAcceptsFocus = ETrue;

	delete iFocusIcon;
	iFocusIcon= anIcon;

	iRedraw = ETrue;
	}

void COggControl::DrawFocus(CBitmapContext& aBitmapContext)
	{
	if (iFocus)
		{
		// __ASSERT_DEBUG(iAcceptsFocus,OGGPANIC(_L("Assert - Control has Focus but should never accept it !?"),3815));
		// __ASSERT_DEBUG(iVisible,OGGPANIC(_L("Assert - Control is not visible, but has focus"),3816));
		DrawCenteredIcon(aBitmapContext,iFocusIcon);
		}
	}

void COggControl::DrawCenteredIcon(CBitmapContext& aBitmapContext, CGulIcon* aIcon)
	{
	TSize s(aIcon->Bitmap()->SizeInPixels());
	TPoint p(ix+iw/2 - s.iWidth/2, iy+ih/2 - s.iHeight/2);
	aBitmapContext.BitBltMasked(p, aIcon->Bitmap(), TRect(TPoint(0, 0), s), aIcon->Mask(), ETrue);
	}

TBool COggControl::Read(TOggParser& p)
	{
	p.ReadToken();
	if (p.iToken != KBeginToken)
		{
		p.iState = TOggParser::EBeginExpected;
		return EFalse;
		}

	while (p.ReadToken() && (p.iToken != KEndToken) && (p.iState == TOggParser::ESuccess))
		ReadArguments(p); 

	if ((p.iState == TOggParser::ESuccess) && (p.iToken != KEndToken)) 
		p.iState = TOggParser::EEndExpected;

	return (p.iState == TOggParser::ESuccess);
	}

TBool COggControl::ReadArguments(TOggParser& p)
	{
	if (p.iToken == _L("Position"))
		{
		p.Debug(_L("Setting position."));
		TInt ax, ay, aw, ah;
		if (p.ReadToken(ax) && p.ReadToken(ay) && p.ReadToken(aw) && p.ReadToken(ah))
			{
			SetPosition(ax, ay, aw, ah);
			return ETrue;
			}
		else
			return EFalse;
		}
	else if (p.iToken == _L("FocusIcon"))
		{
		p.Debug(_L("Setting focused icon."));
		SetFocusIcon(p.ReadIcon(iBitmapFile));

		iObserver->AddControlToFocusList(this);
		}
	else if (p.iToken == _L("OverlayMask"))
		iOverlayMask = p.ReadBitmap(iBitmapFile);

	return ETrue;
	}

TBool COggControl::HighFrequency()
	{
	return iHighFrequency;
	}


COggText::COggText()
: iFontColor(0,0,0), iNeedsScrolling(EFalse), iHasScrolled(EFalse), iScrollDelay(30), iScrollStep(2)
	{
	}

COggText::~COggText()
	{
	delete iText;

	if (iFont && iOwnedByControl) 
		CCoeEnv::Static()->ScreenDevice()->ReleaseFont(iFont);
	}

void COggText::SetFont(CFont* aFont, TBool ownedByControl)
	{
	iFont= aFont;
	iRedraw= ETrue;
	iOwnedByControl = ownedByControl;

	iFontHeight = iFont->HeightInPixels();
	iFontAscent = iFont->AscentInPixels();

#if defined(SERIES60)
	if (iFont->TypeUid() == KCFbsFontUid)
		{
		CFbsFont* fbsFont = (CFbsFont *) iFont;
		if (fbsFont->IsOpenFont())
			{
			TOpenFontMetrics fontMetrics;
			if (fbsFont->GetFontMetrics(fontMetrics))
				{
				iFontHeight = fontMetrics.MaxHeight() + fontMetrics.MaxDepth() + 1;
				iFontAscent = fontMetrics.MaxHeight();
				}
			}
		}
#endif

	iLinePadding = (ih - iFontHeight) / 2;
	if (iLinePadding<0)
		iLinePadding = 0;
	}

void COggText::SetFontColor(TRgb aColor)
	{
	iFontColor = aColor;
	iRedraw = ETrue;
	}

void COggText::SetScrollStyle(TInt aScrollStyle)
	{
	iScrollStyle = aScrollStyle;
	}

void COggText::SetScrollDelay(TInt aScrollDelay)
	{
	iScrollDelay = aScrollDelay;
	}

void COggText::SetScrollStep(TInt aScrollStep)
	{
	iScrollStep = aScrollStep;
	}

void COggText::SetText(const TDesC& aText)
	{
    delete iText;
	iText = NULL;

	iText = HBufC::NewL(aText.Length());
	iText->Des().Copy(aText);

	iTextWidth = GetTextWidth(aText, iFont, iw);
	iNeedsScrolling = iTextWidth>iw;

	iCycle = 0;
	iDrawOffset = 0;
	iHasScrolled = EFalse;
	iRedraw = ETrue;
	}

void COggText::ScrollNow()
	{
	iHasScrolled = EFalse;
	iCycle = iScrollDelay;
	}

void COggText::Cycle()
	{
	if (!iNeedsScrolling || !iVisible)
		return;

	switch(iScrollStyle)
		{
		case EOnce:
			CycleOnce();
			break;

		case EEndless:
			CycleOff();
			break;

		case ETilBorder:
			CycleBorder();
			break;

		case EBackAndForth:
			CycleBackAndForth();
			break;

		default:
			CycleOff();
			break;
		}
	}

void COggText::CycleOnce()
	{
	if (iHasScrolled)
		return;

	CycleOff();
	}

void COggText::CycleOff()
	{
	iCycle++;

	iDrawOffset= (iCycle - iScrollDelay)*iScrollStep;
	if (iDrawOffset<0)
		iDrawOffset= 0;

	if (iCycle<iScrollDelay)
		return;

	iRedraw = ETrue;

	if ((iCycle-iScrollDelay)*iScrollStep>iTextWidth)
		{
		iHasScrolled = ETrue;
		iCycle = 0;
		iDrawOffset = 0;
		}
	}

void COggText::CycleBorder()
	{
	iCycle++;

	iDrawOffset = (iCycle - iScrollDelay)*iScrollStep;
	if (iDrawOffset<0)
		iDrawOffset = 0;

	if (iCycle<iScrollDelay)
		return;

	iRedraw = ETrue;

	if ((iw+(iCycle-iScrollDelay)*iScrollStep)>iTextWidth)
		iCycle = 0;
	}

void COggText::CycleBackAndForth()
	{
	iScrollBackward ? iCycle-- : iCycle++;

	if (iCycle < 0)
		iScrollBackward = EFalse;

	iDrawOffset = (iCycle - iScrollDelay)*iScrollStep;
	if (iDrawOffset < 0)
		iDrawOffset = 0;

	if ((iw+(iCycle-iScrollDelay)*iScrollStep)>iTextWidth)
		iDrawOffset=iTextWidth-iw;
  
	if ((iw+(iCycle-iScrollDelay)*iScrollStep)>(iTextWidth+iScrollDelay*iScrollStep))
		iScrollBackward=ETrue;

	iRedraw = ETrue;
	}

void COggText::Draw(CBitmapContext& aBitmapContext)
	{
	if (!iText)
		return;

	aBitmapContext.UseFont(iFont);
	aBitmapContext.SetPenColor(iFontColor);
	aBitmapContext.SetPenSize(TSize(2,2));

	TRect lineRect(TPoint(ix-iDrawOffset,iy+iLinePadding), TSize(iw+iDrawOffset, iFontHeight));
	TPtrC p(*iText);

	CGraphicsContext::TTextAlign a= CGraphicsContext::ECenter;
	if (iNeedsScrolling)
		a = CGraphicsContext::ELeft;

	aBitmapContext.DrawText(p, lineRect, iFontAscent, a);
	}

void COggText::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);
  if (p.iType==TPointerEvent::EButton1Down) ScrollNow();
}

TBool COggText::ReadArguments(TOggParser& p)
{
  TBool success= COggControl::ReadArguments(p);
  if (success && p.iToken==_L("Font"))
  {
    p.Debug(_L("Setting font."));
    CFont* aFont= p.ReadFont();
    if (aFont) SetFont(aFont, ETrue);
    success= aFont!=0;
  }

  if (success && p.iToken==_L("FontColor"))
  {
    p.Debug(_L("Setting font color."));
    TRgb col(0,0,0);
    success= p.ReadColor(col);
    SetFontColor(col);
  }

  if (success && p.iToken==_L("Style"))
  {
    p.Debug(_L("Setting text style."));
    TInt s;
    success= p.ReadToken(s);
    if (success) SetScrollStyle(s);
  }

  if (success && p.iToken==_L("ScrollStep"))
  {
    TInt s;
    success= p.ReadToken(s);
    if (success) SetScrollStep(s);
  }
 
  if (success && p.iToken==_L("ScrollDelay"))
  {
    TInt s;
    success= p.ReadToken(s);
    if (success) SetScrollDelay(s);
  }
  
  return success;
}


COggIcon::COggIcon()
: iIcon(0), iBlinkFrequency(5), iBlinking(EFalse)
	{
	}

COggIcon::~COggIcon()
	{
	delete iIcon;
	}

void COggIcon::SetIcon(CGulIcon* anIcon)
	{
	delete iIcon;
	iIcon = anIcon;

	iRedraw = ETrue;
	}

void COggIcon::Blink()
	{
	iBlinking = ETrue;
	iVisible = ETrue;
	iCycle = 0;
	
	iRedraw = ETrue;
	}

void COggIcon::Show()
	{
	if (!iVisible)
		iRedraw = ETrue;

	iBlinking = EFalse;
	iVisible = ETrue;
	}

void COggIcon::Hide()
	{
	if (iVisible)
		iRedraw = ETrue;

	iBlinking = EFalse;
	iVisible = EFalse;
	}

void COggIcon::SetBlinkFrequency(TInt aFrequency)
	{
	iBlinkFrequency = aFrequency;
	}

void COggIcon::Cycle()
	{
	if (!iBlinking)
		return;

	iCycle++;

	if (iCycle<iBlinkFrequency)
		return;

	iRedraw = ETrue;
	iVisible = !iVisible;
	iCycle = 0;
	}

void COggIcon::Draw(CBitmapContext& aBitmapContext)
	{
	if (!iIcon)
		return;

	aBitmapContext.BitBltMasked(TPoint(ix,iy), iIcon->Bitmap(),
	TRect(TPoint(0,0),iIcon->Bitmap()->SizeInPixels()), iIcon->Mask(), ETrue);
	}

TBool COggIcon::ReadArguments(TOggParser& p)
	{
	TBool success = COggControl::ReadArguments(p);
	if (success && p.iToken==_L("BlinkFrequency"))
		{
		p.Debug(_L("Setting blink frequency."));
		success = p.ReadToken(iBlinkFrequency);
		}

	if (success && p.iToken==_L("Icon"))
		{
		p.Debug(_L("Setting icon."));
		SetIcon(p.ReadIcon(iBitmapFile));
		success = (iIcon != NULL);
		}

	return success;
	}

COggAnimation::COggAnimation()
: iBitmaps(10), iPause(100), iFrequency(2), iFirstBitmap(0), iStyle(0)
	{
	}

COggAnimation::~COggAnimation()
	{
	ClearBitmaps();
	}

void COggAnimation::ClearBitmaps() 
	{
	iBitmaps.ResetAndDestroy();
	}

void COggAnimation::SetBitmaps(TInt aFirstBitmap, TInt aNumBitmaps)
	{
	ClearBitmaps();
	iFirstBitmap = aFirstBitmap;
  
	for (TInt i = 0 ; i<aNumBitmaps ; i++)
		{
		CFbsBitmap* ibitmap= new(ELeave) CFbsBitmap;
		ibitmap->Load(iBitmapFile,aFirstBitmap+i);
		iBitmaps.AppendL(ibitmap);
		}

	iRedraw = ETrue;
	}

void COggAnimation::Start()
	{
	iCycle = 0;
	iRedraw = ETrue;
	}

void COggAnimation::Stop()
	{
	if (iCycle!=-1)
		{
		iCycle = -1;
		iRedraw = ETrue;
		}
	}

void COggAnimation::SetPause(TInt aPause)
{
  if (iPause!=aPause)
  {
    iPause= aPause;
    iCycle= 0;
    iRedraw= ETrue;
  }
}

void COggAnimation::SetFrequency(TInt aFrequency)
{
  if (aFrequency!=iFrequency)
  {
    iFrequency= aFrequency;
    iCycle= 0;
    iRedraw= ETrue;
  }
}

void COggAnimation::SetStyle(TInt aStyle)
	{
	if (iStyle != aStyle)
		{
		iStyle = aStyle;
		iRedraw = ETrue;
		iCycle = 0;
		}
	}

void COggAnimation::Cycle()
	{
	if ((iCycle<0) || !iVisible)
		return;

#if defined(SERIES60)
	if (iBitmaps.Count() <= 1)
		return;
#endif

	iCycle++;

	if (iCycle<iPause)
		return;

	TInt style = 1;
	if (iStyle == 1)
		style = 2;

	if ((iCycle-iPause)>=(iBitmaps.Count()*iFrequency*style))
		iCycle= 0;

	if (iCycle%iFrequency == 0)
		iRedraw = ETrue;
	}

void COggAnimation::Draw(CBitmapContext& aBitmapContext)
{
  if ((iBitmaps.Count() == 0) || (iFrequency<1))
	  return;

  TInt idx= 0;
  if (iCycle>=0 && iCycle-iPause>=0)
  {
    idx= ((iCycle-iPause)/iFrequency)%iBitmaps.Count();
    if (iStyle==1 && (iCycle-iPause)/iFrequency>=iBitmaps.Count()) 
      idx= iBitmaps.Count()- idx -1;
  }

  CFbsBitmap* b= iBitmaps[idx];
  aBitmapContext.BitBlt(TPoint(ix,iy),b,TRect(TPoint(0,0),b->SizeInPixels()));
}

TBool COggAnimation::ReadArguments(TOggParser& p)
{
  TBool success= COggControl::ReadArguments(p);
  if (success && p.iToken==_L("Frequency")) {
    p.Debug(_L("Setting frequency."));
    TInt frequency;
    success= p.ReadToken(frequency);
    if (success) SetFrequency(frequency);
  }
  if (success && p.iToken==_L("Pause")) {
    p.Debug(_L("Setting pause."));
    TInt pause(100);
    success= p.ReadToken(pause);
    if (success) SetPause(pause);
  }
  if (success && p.iToken==_L("Bitmaps")) {
    p.Debug(_L("Settings bitmaps."));
    TInt first, num;
    success= p.ReadToken(first);
    if (success) 
      {
      success= p.ReadToken(num);
      if (success) 
        SetBitmaps(first,num);
      }
  }
  if (success && p.iToken==_L("Style")) {
    p.Debug(_L("Setting style."));
    TInt s;
    success= p.ReadToken(s);
    if (success) SetStyle(s);
  }
  return success;
}


COggDigits::COggDigits()
: iText(0), iBitmaps(ENumDigits), iMasks(ENumDigits)
{
}

COggDigits::~COggDigits()
{
  ClearBitmaps();
  ClearMasks();
  
  delete iText;
}

void COggDigits::ClearBitmaps() 
{
  iBitmaps.ResetAndDestroy();
}

void COggDigits::ClearMasks()
{
  iMasks.ResetAndDestroy();
}

void COggDigits::SetBitmaps(TInt aFirstBitmap)
{
  ClearBitmaps();
  for (TInt i=0; i<ENumDigits; i++)
  {    
    iBitmaps.AppendL( new (ELeave) CFbsBitmap) ;
    iBitmaps[i]->Load(iBitmapFile,aFirstBitmap+i);
  }
  iRedraw= ETrue;
}

void COggDigits::SetMasks(TInt aFirstMask)
{
  ClearMasks();
  for (TInt i=0; i<ENumDigits; i++)
  {
    iMasks.AppendL( new (ELeave) CFbsBitmap) ;
    iMasks[i]->Load(iBitmapFile,aFirstMask+i);
  }

  iRedraw= ETrue;
}

void COggDigits::SetText(const TDesC& aText)
{
  delete iText;
  iText = NULL;

  iText = HBufC::NewLC(aText.Length());
  CleanupStack::Pop();
  iText->Des().Copy(aText);

  iRedraw= ETrue;
}

void COggDigits::Draw(CBitmapContext& aBitmapContext)
{
  if ((iBitmaps.Count() == 0) || !iText)
	  return;

  TInt x=0;
  TBuf<1> zero(_L("0"));

  for (TInt i=0; i<iText->Length(); i++)
  {
    TBuf<1> c;
    c.Append((*iText)[i]);

    CFbsBitmap* b= 0;
    CFbsBitmap* m= 0;
    TInt idx=-1;

    if (c>=_L("0") && c<=_L("9")) {
      idx= c[0]-zero[0];
    } else if (c==_L(":")) {
      idx= EDigitColon;
    } else if (c==_L("-")) {
      idx= EDigitDash;
    } else if (c==_L("/")) {
      idx= EDigitSlash;
    } else if (c==_L(".")) {
      idx= EDigitDot;
    }
    if (idx>=0) b= iBitmaps[idx];
    if (iMasks.Count() >0 )
        m= iMasks[idx];
    else
        m= iBitmaps[idx];

    if (!b || !m) continue;

    aBitmapContext.BitBltMasked(TPoint(ix+x,iy),b,TRect(TPoint(0,0),b->SizeInPixels()),m,ETrue);
    x+= b->SizeInPixels().iWidth;
  }
}

TBool COggDigits::ReadArguments(TOggParser& p)
{
  TBool success= COggControl::ReadArguments(p);
  if (success && p.iToken==_L("Bitmaps")) {
    p.Debug(_L("Settings bitmaps."));
    TInt first,num=0;
    success= p.ReadToken(first) && p.ReadToken(num);
    if (success && num!=ENumDigits) {
      success= EFalse;
      p.iState= TOggParser::EOutOfRange;
    }
    if (success) SetBitmaps(first);
  }
  if (success && p.iToken==_L("Masks")) {
    p.Debug(_L("Settings masks."));
    TInt first,num=0;
    success= p.ReadToken(first) && p.ReadToken(num);
    if (success && num!=ENumDigits) {
      success= EFalse;
      p.iState= TOggParser::EOutOfRange;
    }
    if (success) SetBitmaps(first);
  }
  return success;
}


COggButton::COggButton()
: iActiveMask(0), iNormalIcon(0), iPressedIcon(0), iDimmedIcon(0), iState(0), iStyle(0)
{
}

COggButton::~COggButton()
{
  if (iActiveMask)
	  iActiveMask->Reset();

  delete iActiveMask;
  delete iNormalIcon;
  delete iPressedIcon;
  delete iDimmedIcon;
}

void COggButton::SetActiveMask(const TFileName& aFileName, TInt anIdx)
{
  iActiveMask = new(ELeave) CFbsBitmap;
  int err= iActiveMask->Load(aFileName,anIdx);
  if (err!=KErrNone)
  {
    TBuf<256> buf;
    CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_1);
    buf.AppendNum(err);
    User::InfoPrint(buf);

	delete iActiveMask;
    iActiveMask= 0;
  }
}

void COggButton::SetNormalIcon(CGulIcon* anIcon)
{
  delete iNormalIcon;
  iNormalIcon= anIcon;

  iRedraw= ETrue;
}

void COggButton::SetPressedIcon(CGulIcon* anIcon)
{
  delete iPressedIcon;
  iPressedIcon= anIcon;

  iRedraw= ETrue;
}

void COggButton::SetDimmedIcon(CGulIcon* anIcon)
{
  delete iDimmedIcon;
  iDimmedIcon= anIcon;

  iRedraw= ETrue;
}


void COggButton::SetStyle(TInt aStyle)
{
  iStyle= aStyle;
}

void COggButton::SetState(TInt aState)
{
  if (iState!=aState)
  {
    iState= aState;
    iRedraw= ETrue;
  }
}

void COggButton::Draw(CBitmapContext& aBitmapContext)
{  
  if (iDimmed) {
    if (iDimmedIcon) DrawCenteredIcon(aBitmapContext,iDimmedIcon);
    else if (iNormalIcon) DrawCenteredIcon(aBitmapContext,iNormalIcon);
    if (iFocus) DrawFocus(aBitmapContext);
  }
  else {
    if (iState==0) {
      if (iFocus) DrawFocus(aBitmapContext);
      else if (iNormalIcon) DrawCenteredIcon(aBitmapContext,iNormalIcon);
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
    if (iStyle==0) iState= 1; 
    else {
      if (iState==0) iState= 1; else iState= 0;
    }
    iRedraw= ETrue;
    if (iObserver) iObserver->OggControlEvent(this,0,0);
  }

  if (p.iType==TPointerEvent::EButton1Up) {
    if (iStyle==0) iState= 0;
    iRedraw= ETrue;
  }

  COggControl::PointerEvent(p);
}


TBool COggButton::ReadArguments(TOggParser& p)
{
  TBool success= ETrue;
  if (p.iToken==_L("NormalIcon")) {
    p.Debug(_L("Setting normal icon."));
    CGulIcon* i= p.ReadIcon(iBitmapFile);
    success= i!=0;
    if (success) SetNormalIcon(i);
  }
  else if (p.iToken==_L("PressedIcon")) {
    p.Debug(_L("Setting pressed icon."));
    CGulIcon* i= p.ReadIcon(iBitmapFile);
    success= i!=0;
    if (success) SetPressedIcon(i);
  }
  else if (p.iToken==_L("DimmedIcon")) {
    p.Debug(_L("Setting dimmed icon."));
    CGulIcon* i= p.ReadIcon(iBitmapFile);
    success= i!=0;
    if (success) SetDimmedIcon(i);
  }
  else if (p.iToken==_L("Style")) {
    p.Debug(_L("Setting style."));
    TInt s;
    success= p.ReadToken(s);
    if (success) SetStyle(s);
  }
  return success && COggControl::ReadArguments(p);
}

// There are two versions of the slider (TInt and TInt64)
// This should be done using templates but TInt64 is not a built-in type on older OS versions.
// Consequently I've just duplicated the code. Make sure any changes you make to the slider are copied to both types.
COggSliderBase::~COggSliderBase()
	{
    delete iKnobIcon;
	}

void COggSliderBase::SetKnobIcon(CGulIcon* aIcon)
	{
	delete iKnobIcon;
	iKnobIcon = aIcon;

	iRedraw = ETrue;
	}

void COggSliderBase::SetStyle(TInt aStyle)
	{
	iStyle = aStyle;
	iRedraw = ETrue; 
	}

TBool COggSliderBase::ReadArguments(TOggParser& p)
	{
	TBool success = ETrue;
	if (p.iToken == _L("KnobIcon"))
		{
		p.Debug(_L("Setting knob icon."));
		CGulIcon* i = p.ReadIcon(iBitmapFile);
		success = (i != NULL);
		if (success)
			SetKnobIcon(i);
		}
	else if (p.iToken == _L("Style"))
		{
		p.Debug(_L("Setting style."));
		TInt aStyle(0);
		success = p.ReadToken(aStyle);
		if (success)
			SetStyle(aStyle);
		}

	if (success)
		success = COggControl::ReadArguments(p);

	return success;
	}


COggSlider::COggSlider()
: iMaxValue(100)
	{
	}

void COggSlider::Draw(CBitmapContext& aBitmapContext)
	{
	if (!iKnobIcon)
		return;

	DrawFocus(aBitmapContext);
	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	TRect r(TPoint(0, 0), s);
	TPoint p(ix, iy);

	iPos = GetPosFromValue();
	switch (iStyle)
		{
		case 0:
			r.iBr.iX= iPos;
			break;

		case 1:
			r.iTl.iY= iPos;
			break;

		case 2:
			p.iX= iPos;
			break;

		case 3:
			p.iY= iPos;
			break;

		case 4:
			p.iX= ix+iPos;
			r = TRect(TPoint(iPos, 0), TSize(iw/iMaxValue, ih));
			break;
		}

	aBitmapContext.BitBltMasked(p, iKnobIcon->Bitmap(), r, iKnobIcon->Mask(), ETrue);
	}

void COggSlider::SetValue(TInt aValue)
	{
	if (aValue == iValue)
		return;

	if (aValue<0)
		aValue = 0;
	else if (aValue>iMaxValue)
		aValue = iMaxValue;

	iValue = aValue;
	iRedraw = (GetPosFromValue() != iPos); 
	}

void COggSlider::SetMaxValue(TInt aMaxValue)
	{
	iMaxValue = aMaxValue;
	if (iValue>iMaxValue)
		iValue = iMaxValue;

	iRedraw = ETrue;
	}

TInt COggSlider::CurrentValue()
	{
	return iValue;
	}

TInt COggSlider::GetPosFromValue()
	{
	if (!iKnobIcon)
		return 0;

	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	if (iMaxValue == 0)
		{
		// Implies iValue also zero, so return pos accordingly
		switch (iStyle)
			{
			case 0:
			case 1:
			case 4:
				return 0;

			case 2:
				return ix;

			case 3:
				return iy + (ih - s.iHeight);
			}

		return 0;
		}

	switch (iStyle)
		{
		case 0:
		case 4:
			return (iw*iValue)/iMaxValue;
			break;
	
		case 1:
			return (ih*iValue)/iMaxValue;
			break;

		case 2:
			return ix + ((iw-s.iWidth)*iValue)/iMaxValue;
			break;

		case 3:
			return iy+ (ih-s.iHeight) - ((ih-s.iHeight)*iValue)/iMaxValue;
			break;
		}

	return 0;
	}

TInt COggSlider::GetValueFromPos(TInt aPos)
	{
	if (!iKnobIcon)
		return 0;

	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	switch (iStyle)
		{
		case 0:
			return (aPos*iMaxValue)/iw;
			break;

		case 1:
			return (aPos*iMaxValue)/ih;
			break;

		case 2:
			return ((aPos - s.iWidth/2)*iMaxValue)/(iw - s.iWidth);
			break;

		case 3:
			return (((ih - s.iHeight)-(aPos - s.iHeight/2))*iMaxValue)/(ih - s.iHeight);
			break;
		}

	return 0;
	}

void COggSlider::PointerEvent(const TPointerEvent& p)
	{
	if (iDimmed)
		return;

	if (p.iType == TPointerEvent::EButton1Down)
		iIsMoving = ETrue;

	if (iIsMoving)
		{
		TPoint pt(p.iPosition);
		pt.iX -= ix ; pt.iY -= iy;

		switch (iStyle)
			{
			case 0:
				SetValue(GetValueFromPos(pt.iX));
				break;

			case 1:
				SetValue(GetValueFromPos(pt.iY));
				break;

			case 2:
				SetValue(GetValueFromPos(pt.iX));
				break;

			case 3:
				SetValue(GetValueFromPos(pt.iY));
				break;
			}
		}

	if (p.iType == TPointerEvent::EButton1Up)
		{
		iIsMoving = EFalse;
		iRedraw = ETrue;
		}

	COggControl::PointerEvent(p);
	}


COggSlider64::COggSlider64()
: iMaxValue(100)
	{
	}

void COggSlider64::Draw(CBitmapContext& aBitmapContext)
	{
	if (!iKnobIcon)
		return;

	DrawFocus(aBitmapContext);
	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	TRect r(TPoint(0, 0), s);
	TPoint p(ix, iy);

	iPos = GetPosFromValue();
	switch (iStyle)
		{
		case 0:
			r.iBr.iX= iPos;
			break;

		case 1:
			r.iTl.iY= iPos;
			break;

		case 2:
			p.iX= iPos;
			break;

		case 3:
			p.iY= iPos;
			break;

		case 4:
			p.iX= ix+iPos;
#if defined(SERIES60V3)
			r = TRect(TPoint(iPos, 0), TSize((TInt) (TInt64(iw)/iMaxValue), ih));
#else
			r = TRect(TPoint(iPos, 0), TSize((TInt64(iw)/iMaxValue).GetTInt(), ih));
#endif
			break;
		}

	aBitmapContext.BitBltMasked(p, iKnobIcon->Bitmap(), r, iKnobIcon->Mask(), ETrue);
	}

void COggSlider64::SetValue(TInt64 aValue)
	{
	if (aValue == iValue)
		return;

	if (aValue<0)
		aValue = 0;
	else if (aValue>iMaxValue)
		aValue = iMaxValue;

	iValue = aValue;
	iRedraw = (GetPosFromValue() != iPos); 
	}

void COggSlider64::SetMaxValue(TInt64 aMaxValue)
	{
	iMaxValue = aMaxValue;
	if (iValue>iMaxValue)
		iValue = iMaxValue;

	iRedraw = ETrue;
	}

TInt64 COggSlider64::CurrentValue()
	{
	return iValue;
	}

TInt COggSlider64::GetPosFromValue()
	{
	if (!iKnobIcon)
		return 0;

	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	if (iMaxValue == 0)
		{
		// Implies iValue also zero, so return pos accordingly
		switch (iStyle)
			{
			case 0:
			case 1:
			case 4:
				return 0;

			case 2:
				return ix;

			case 3:
				return iy + (ih - s.iHeight);
			}

		return 0;
		}

	switch (iStyle)
		{
		case 0:
		case 4:
#if defined(SERIES60V3)
			return (TInt) ((TInt64(iw)*iValue)/iMaxValue);
#else
			return ((TInt64(iw)*iValue)/iMaxValue).GetTInt();
#endif
			break;
	
		case 1:
#if defined(SERIES60V3)
			return (TInt) ((TInt64(ih)*iValue)/iMaxValue);
#else
			return ((TInt64(ih)*iValue)/iMaxValue).GetTInt();
#endif
			break;

		case 2:
#if defined(SERIES60V3)
			return ix + ((TInt) ((TInt64(iw - s.iWidth)*iValue)/iMaxValue));
#else
			return ix + ((TInt64(iw - s.iWidth)*iValue)/iMaxValue).GetTInt();
#endif
			break;

		case 3:
#if defined(SERIES60V3)
			return iy + (ih - s.iHeight) - ((TInt) ((TInt64(ih-s.iHeight)*iValue)/iMaxValue));
#else
			return iy + (ih - s.iHeight) - ((TInt64(ih-s.iHeight)*iValue)/iMaxValue).GetTInt();
#endif
			break;
		}

	return 0;
	}

TInt64 COggSlider64::GetValueFromPos(TInt aPos)
	{
	if (!iKnobIcon)
		return 0;

	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	switch (iStyle)
		{
		case 0:
			return (TInt64(aPos)*iMaxValue)/TInt64(iw);
			break;

		case 1:
			return (TInt64(aPos)*iMaxValue)/TInt64(ih);
			break;

		case 2:
			return (TInt64(aPos - s.iWidth/2)*iMaxValue)/TInt64(iw - s.iWidth);
			break;

		case 3:
			return (TInt64((ih - s.iHeight)-(aPos - s.iHeight/2))*iMaxValue)/TInt64(ih - s.iHeight);
			break;
		}

	return 0;
	}

void COggSlider64::PointerEvent(const TPointerEvent& p)
	{
	if (iDimmed)
		return;

	if (p.iType == TPointerEvent::EButton1Down)
		iIsMoving = ETrue;

	if (iIsMoving)
		{
		TPoint pt(p.iPosition);
		pt.iX -= ix ; pt.iY -= iy;

		switch (iStyle)
			{
			case 0:
				SetValue(GetValueFromPos(pt.iX));
				break;

			case 1:
				SetValue(GetValueFromPos(pt.iY));
				break;

			case 2:
				SetValue(GetValueFromPos(pt.iX));
				break;

			case 3:
				SetValue(GetValueFromPos(pt.iY));
				break;
			}
		}

	if (p.iType == TPointerEvent::EButton1Up)
		{
		iIsMoving = EFalse;
		iRedraw = ETrue;
		}

	COggControl::PointerEvent(p);
	}


COggScrollBar::COggScrollBar()
: iMaxValue(100), iScrollerSize(10), iPage(1), iStep(1)
	{
	}

COggScrollBar::~COggScrollBar()
	{
    delete iKnobIcon;
	}

void COggScrollBar::SetStyle(TInt aStyle)
	{
	iStyle = aStyle;
	iRedraw = ETrue;
	}

void COggScrollBar::SetKnobIcon(CGulIcon* anIcon)
	{
	iKnobIcon = anIcon;
	iRedraw = ETrue;
	}

void COggScrollBar::SetScrollerSize(TInt aSize)
	{
	iScrollerSize = aSize;
	iRedraw = ETrue;
	}

void COggScrollBar::SetPage(TInt aPage)
	{
	iPage = aPage;
	}

void COggScrollBar::SetStep(TInt aStep)
	{
	iStep = aStep;
	}

void COggScrollBar::SetAssociatedControl(COggControl* aControl)
	{
	iAssociated = aControl;
	}

void COggScrollBar::Draw(CBitmapContext& aBitmapContext)
	{
	if (!iKnobIcon)
		return;

	TSize s(iKnobIcon->Bitmap()->SizeInPixels());
	TRect r(TPoint(0,0), s);
	TPoint p(ix, iy);

	iPos = GetPosFromValue(iValue);

	switch (iStyle)
		{
		case 0:
			p.iX= iPos;
			break;

		case 1:
			p.iY= iPos;
			break;
		}

	aBitmapContext.BitBltMasked(p, iKnobIcon->Bitmap(), r, iKnobIcon->Mask(), ETrue);
	}

void COggScrollBar::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);

  TSize s(0, 0);
  if (iKnobIcon)
	  s= iKnobIcon->Bitmap()->SizeInPixels();

  TInt k, s1, s2;
  if (iStyle == 0)
  {
    k = s.iWidth;
    s1 = p.iPosition.iX-ix;
    s2 = p.iPosition.iX;
  }
  else
  {
    k = s.iHeight;
    s1 = p.iPosition.iY-iy;
    s2 = p.iPosition.iY;
  }

  if (p.iType==TPointerEvent::EButton1Up)
	  iIsMoving= EFalse;

  if (p.iType==TPointerEvent::EButton1Down)
  {
    if (s1<iScrollerSize)
      SetValue(iValue-iStep);
	else if (s1>ih-iScrollerSize)
      SetValue(iValue+iStep);
	else if (s2>=iPos && s2<=iPos+k)
      iIsMoving= ETrue;
	else if (s2<iPos)
      SetValue(iValue-iPage+1);
	else if (s2>iPos+k)
      SetValue(iValue+iPage-1);
  }

  if (iIsMoving)
	  SetValue(GetValueFromPos(s2));
}

void COggScrollBar::SetMaxValue(TInt aMaxValue)
{
  if (iMaxValue != aMaxValue)
  {
    iMaxValue = aMaxValue;
    iRedraw = ETrue;
  }
}

void COggScrollBar::SetValue(TInt aValue)
{
  if (aValue<0)
	  aValue=0;
  else if (aValue>iMaxValue)
	  aValue= iMaxValue;

  if (iValue!=aValue)
  {
    iValue = aValue;
    iRedraw = GetPosFromValue(aValue) != iPos;
    if (iAssociated)
		iAssociated->ControlEvent(0, aValue);
  }
}

TInt COggScrollBar::GetPosFromValue(TInt aValue)
	{
	if (!iKnobIcon)
		return 0;

	if (iMaxValue == 0)
		{
		// Implies iValue also == 0;
		switch (iStyle)
			{
			case 0:
				return ix + iScrollerSize;
				break;

			case 1:
				return iy + iScrollerSize;
				break;
			}
		}

  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  switch (iStyle)
	{
	case 0:
		return ix + iScrollerSize + ((iw - s.iWidth - 2*iScrollerSize)*aValue)/iMaxValue;
		break;

	case 1:
		return iy + iScrollerSize + ((ih-s.iHeight-2*iScrollerSize)*aValue)/iMaxValue;
		break;
  }

  return 0;
}

TInt COggScrollBar::GetValueFromPos(TInt aPos)
{
  if (!iKnobIcon)
	  return 0;

  TSize s(iKnobIcon->Bitmap()->SizeInPixels());
  switch (iStyle)
	{
	case 0:
		return ((aPos - ix - s.iWidth/2 - iScrollerSize)*iMaxValue)/(iw - s.iWidth - 2*iScrollerSize);
		break;

	case 1:
		return ((aPos - iy - iScrollerSize - s.iHeight/2)*iMaxValue)/(ih - s.iHeight - 2*iScrollerSize);
		break;
	}
 
	return 0;
}

TBool COggScrollBar::ReadArguments(TOggParser& p)
{
  TBool success= COggControl::ReadArguments(p);
  if (success && p.iToken==_L("ScrollerSize"))
  {
    p.Debug(_L("Setting scroller size."));
    TInt i;
    success= p.ReadToken(i);
    if (success) SetScrollerSize(i);
  }

  if (success && p.iToken==_L("Style"))
  {
    p.Debug(_L("Setting style."));
    TInt i;
    success= p.ReadToken(i);
    if (success) SetStyle(i);
  }
  
  if (success && p.iToken==_L("Page"))
  {
    p.Debug(_L("Setting page."));
    TInt i;
    success= p.ReadToken(i);
    if (success) SetPage(i);
  }
  
  if (success && p.iToken==_L("KnobIcon"))
  {
    p.Debug(_L("Setting knob icon."));
    CGulIcon* c= p.ReadIcon(iBitmapFile);
    success= c!=0;
    if (success) SetKnobIcon(c);
  }
 
  if (success && p.iToken==_L("Step"))
  {
    p.Debug(_L("Setting step."));
    TInt i;
    success= p.ReadToken(i);
    if (success) SetStep(i);
  }
  
  return success;
}


COggAnalyzer::COggAnalyzer()
: COggControl(ETrue), iNumValues(KAnalyzerNumValues), iStyle(1)
{
  iDx= iw/iNumValues;
  Clear();
}

COggAnalyzer::~COggAnalyzer()
{
  delete iBarIcon;
}

void COggAnalyzer::SetPosition(TInt ax, TInt ay, TInt aw, TInt ah)
{
  COggControl::SetPosition(ax,ay,aw,ah);
  iDx= iw/iNumValues;
}

void COggAnalyzer::SetBarIcon(CGulIcon* aBarIcon)
{
  delete iBarIcon;
  iBarIcon= aBarIcon;

  iRedraw= ETrue;
}

void COggAnalyzer::SetValue(TInt i, TInt theValue)
{
  if (iValues[i]!=theValue)
  {
    iValues[i]= theValue;
    if (theValue>iPeaks[i])
		iPeaks[i]= iValues[i];

	iRedraw= ETrue;
  }
}

void COggAnalyzer::SetStyle(TInt aStyle)
	{
	iStyle = aStyle;
	iRedraw = ETrue;
	}

TInt COggAnalyzer::Style()
	{
	return iStyle;
	}

void COggAnalyzer::Cycle()
	{
	if (!iVisible)
		return;

	TInt i;
	if (iStyle == EPeak)
		{
		iCycle++;
		if (iCycle%10 == 0)
			{
			iCycle = 0;
			for (i = 0 ; i<iNumValues ; i++)
				{
				if ((iPeaks[i]>0) || (iPeaks[i] != iValues[i]))
					{
					if (iPeaks[i] != iValues[i])
						{ 
						iPeaks[i] = iValues[i];
						iRedraw = ETrue;
						}
					}
				}
			}
		else
			{
			for (i = 0 ; i<iNumValues ; i++)
				{
				if (iValues[i]>iPeaks[i])
					{ 
					iPeaks[i] = iValues[i];
					iRedraw = ETrue;
					}
				}
			}
		}
	else if (iStyle == EDecay)
		{
		for (i = 0 ; i<iNumValues ; i++)
			{
			if (iValues[i] && ((iValues[i] + KDecaySteplength) > iPeaks[i]))
				{ 
				iPeaks[i] = iValues[i];
				iRedraw = ETrue;
				}
			else
				{
				if (iPeaks[i] && ((iPeaks[i] -= KDecaySteplength) < 0))
					{
					iPeaks[i] = 0;
					iRedraw = ETrue;
					}
				}
			}
		}
	}

void COggAnalyzer::Draw(CBitmapContext& aBitmapContext)
{
  // Series 60 special mode
  TInt i;
  if (iStyle==EDecay) {
    if (iBarIcon) {
      TInt x= ix;
      TSize s(iBarIcon->Bitmap()->SizeInPixels());
      for (i = 0 ; i<iNumValues ; i++) {
        TInt val = iPeaks[i];
        TRect rc(TPoint(0,ih-val), s);
        aBitmapContext.BitBltMasked(TPoint(x,iy+ih-val),iBarIcon->Bitmap(), rc, iBarIcon->Mask(), ETrue);
        x+= iDx;
      }
    }
    return;
  }

  // UIQ rendering
  if (iBarIcon) {
    TInt x= ix;
    TSize s(iBarIcon->Bitmap()->SizeInPixels());
    for (i = 0 ; i<iNumValues ; i++) {
      TRect rc(TPoint(0,ih-iValues[i]), s);
      aBitmapContext.BitBltMasked(TPoint(x,iy+ih-iValues[i]),iBarIcon->Bitmap(), rc, iBarIcon->Mask(), ETrue);
      x+= iDx;
    }
  }
  else {
    TRect r(TPoint(ix,iy),TSize(iDx-1,ih));
    aBitmapContext.SetBrushColor(KRgbBlue);
    for (i = 0 ; i<iNumValues ; i++) {
      r.iTl.iY= iy+ih-iValues[i];
      aBitmapContext.Clear(r);
      r.Move(iDx,0);
    }
  }

  if (iStyle==EPeak) {
    aBitmapContext.SetBrushColor(KRgbDarkBlue);
    TRect p(TPoint(ix,iy),TSize(iDx-1,ih));
    for (i = 0 ; i<iNumValues ; i++) {
      p.iTl.iY= iy+ih-iPeaks[i]-1;
      p.iBr.iY= p.iTl.iY+2;
      aBitmapContext.Clear(p);
      p.Move(iDx, 0);
    }
  }
}

#if!defined(SERIES60)
void COggAnalyzer::RenderWaveform(short int data[2][512])
#else
void COggAnalyzer::RenderWaveform(short int *data)
#endif
{
#if defined (SERIES60)
  if (!data)
	  return;
#endif

  if (iStyle==0) return;
  for (TInt i=0; i<512; i++) {
#if!defined(SERIES60)
    iFFTRe[i]= data[0][i];
#else
    iFFTRe[i]= *data++;
#endif
    iFFTIm[i]= 0;
  }
  window(iFFTRe, 512);
  fix_fft( iFFTRe, iFFTIm, 9, 0 );
  fix_loud( iFFTAbs, iFFTRe, iFFTIm, 256, 0);

  RenderFrequencies(iFFTAbs);
}

void COggAnalyzer::RenderWaveformFromMDCT(const TInt32* aFreqBins)
{
    // This function takes as input the maximal MDCT coefficients of 
    // each frequency bins 
    
    // Convert to log
    TInt i, j;
    for (j=0; j<16; j++)
    {
        TInt v = aFreqBins[j];
        for(i=0; i<KDCTAnalyzerDynamic; ++i)
        {
            if( loud2[i+12] <= v)
                break;
        }
        /* i is [0 43] */
        i = - i + KDCTAnalyzerDynamic ; /* [43 .. 0 ] */
        iValues[j]= (i*ih)/KDCTAnalyzerDynamic;
    }

  iRedraw= ETrue;
}

void COggAnalyzer::RenderFrequencies(short int data[256])
{
  if (iStyle==0)
	  return;

  TInt i, c, y;
  static const TInt xscale[17] = { 0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 250 };
  for (i = 0 ; i < iNumValues ; i++)
  {
    for (c = xscale[i], y = 10 ; c < xscale[i + 1] ; c++)
	{
      if (data[c] < y)
		  y = data[c];
	}

    iValues[i]= ((y+100)*ih)/110;
  }
  
  iRedraw= ETrue;
}

void COggAnalyzer::Clear()
{
  for (TInt i=0; i<iNumValues; i++)
  {
    if ( iValues[i] || iPeaks[i] )
	{
      iValues[i] = 0;
      iPeaks[i] = 0;
      iRedraw = ETrue;
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

TBool COggAnalyzer::ReadArguments(TOggParser& p)
{
  TBool success= COggControl::ReadArguments(p);
  if (success && p.iToken==_L("Style")) {
    p.Debug(_L("Setting style."));
    success= p.ReadToken(iStyle);
  }
  if (success && p.iToken==_L("BarIcon")) {
    p.Debug(_L("Setting bar icon."));
    SetBarIcon(p.ReadIcon(iBitmapFile));
    success= iBarIcon!=0;
  }
  return success;
}


COggListBox::COggListBox()
: iFontColor(0, 0, 0), iFontColorSelected(255,0, 0), iSelected(-1)
{
  iData = CColumnListBoxData::NewL();
  iLineHeight = 16;
  iLinesVisible = 1;
}

COggListBox::~COggListBox()
{
  delete iData;

  if (iFont && iOwnedByControl)
      CCoeEnv::Static()->ScreenDevice()->ReleaseFont(iFont);
}

void COggListBox::SetPosition(TInt ax, TInt ay, TInt aw, TInt ah)
{
  COggControl::SetPosition(ax, ay, aw, ah);
  iLinesVisible= ih/iLineHeight;
}

void COggListBox::SetText(CDesCArray* aText)
{
  iText= aText;
}

void COggListBox::SetFont(CFont* aFont, TBool aOwnedByControl)
{
  iFont= aFont;
  iRedraw = ETrue;
  iOwnedByControl = aOwnedByControl;

  iFontHeight = iFont->HeightInPixels();
  iFontAscent = iFont->AscentInPixels();

#if defined(SERIES60)
  if (iFont->TypeUid() == KCFbsFontUid)
  {
	  CFbsFont* fbsFont = (CFbsFont *) iFont;
	  if (fbsFont->IsOpenFont())
	  {
	    TOpenFontMetrics fontMetrics;
		if (fbsFont->GetFontMetrics(fontMetrics))
		{
		  iFontHeight = fontMetrics.MaxHeight() + fontMetrics.MaxDepth() + 1;
		  iFontAscent = fontMetrics.MaxHeight();
		}
	  }
  }
#endif

  iLinePadding = iFontHeight/6;
  iLineHeight = iFontHeight + 2*iLinePadding;

  iLinesVisible = ih/iLineHeight;
  iOffset= iScroll*iLineHeight;
  UpdateScrollBar();
}

void COggListBox::SetFontColor(TRgb aColor)
{
  iFontColor = aColor;
  iRedraw = ETrue;
}

void COggListBox::SetFontColorSelected(TRgb aColor)
{
  iFontColorSelected = aColor;
  iRedraw = ETrue;
}

void COggListBox::SetBarColorSelected(TRgb aColor)
{
  iBarColorSelected = aColor;
  iUseBarSelected = ETrue;
  iRedraw = ETrue;
}

CColumnListBoxData* COggListBox::GetColumnListBoxData()
{
  return iData;
}

void COggListBox::Redraw(TBool doRedraw)
{
  UpdateScrollBar();
  COggControl::Redraw(doRedraw);
}

void COggListBox::ClearText()
{
  iText->Reset();
  iRedraw = ETrue;
  UpdateScrollBar();
}

void COggListBox::AppendText(const TDesC& aText)
{
  iText->AppendL(aText);
  iRedraw = ETrue;
  UpdateScrollBar();
}

TInt COggListBox::CountText()
{
  if (!iText)
	  return 0;
  
  return iText->Count();
}

CDesCArray* COggListBox::GetTextArray()
{
  return iText;
}

void COggListBox::ScrollBy(TInt nLines)
{
  SetTopIndex(iTop+nLines);
}

TInt COggListBox::CurrentItemIndex()
{
  return iSelected;
}

TInt COggListBox::NofVisibleLines()
{
  return iLinesVisible;
}

TInt COggListBox::SetCurrentItemIndex(TInt idx)
{
#if defined(UIQ)
  // UIQ_?
  if (idx<0) idx = 1;
#else
  if (idx<0) idx=0; // FIXIT Quest : Newer a listbox without an valid selection
#endif
  else if (idx>=iText->Count()) idx= iText->Count()-1;
  if (idx!=iSelected) {
    iSelected= idx;
    iRedraw= ETrue;
    if (idx<iTop) SetTopIndex(idx);
    else if (idx>=iTop+iLinesVisible) SetTopIndex(idx-iLinesVisible+1);
    if (iObserver) iObserver->OggControlEvent(this,1,idx);
  }
  return idx;
}

void COggListBox::SetTopIndex(TInt idx)
{
  if (idx<0) idx=0;
  if (idx!=iTop) {
    iCycle= KListboxcycles;
    iScroll= (iTop-idx);
    iOffset= iScroll*iLineHeight;
    iTop= idx;
    iRedraw= ETrue;
    UpdateScrollBar();
    if (iObserver) iObserver->OggControlEvent(this,0,idx);
  }
}

void COggListBox::SetVertScrollBar(COggScrollBar* aScrollBar)
{
  iScrollBar= aScrollBar;
  UpdateScrollBar();
}

void COggListBox::UpdateScrollBar()
{
  if (iScrollBar && iText) {
    TInt max(iText->Count()-iLinesVisible);
    if (max<0) max=0;
    iScrollBar->SetMaxValue(max);
    iScrollBar->SetValue(iTop);
    iScrollBar->SetPage(iLinesVisible);
  }
}

TInt COggListBox::GetLineFromPos(TInt aPos)
{
  return (aPos/iLineHeight)+iTop;
}

TInt COggListBox::GetColumnFromPos(TInt aPos)
{
  for (TInt j=0; j<iData->LastColumn(); j++)
  {
    if (aPos>=0 && aPos<iData->ColumnWidthPixel(j))
      return j;

    aPos-= iData->ColumnWidthPixel(j);
  }

  return -1;
}

void COggListBox::Draw(CBitmapContext& aBitmapContext)
{
  if (!iFont || !iText)
	  return;

  DrawFocus(aBitmapContext);

  aBitmapContext.UseFont(iFont);
  aBitmapContext.SetPenSize(TSize(1,1));

  TInt y= iy - iLineHeight;
  TInt x;
  TInt len,idx;

  for (TInt i=iTop+iOffset/iLineHeight-1; i<iText->Count(); i++)
  {
    if (i<0)
	{
      y+= iLineHeight;
      continue;
    }

    TPtrC p((*iText)[i]);
    x= ix;

    if (i==iSelected) {
			if(iUseBarSelected) {
				aBitmapContext.SetBrushColor(iBarColorSelected);
				aBitmapContext.SetBrushStyle(CGraphicsContext::ESolidBrush);
				aBitmapContext.SetPenColor(iBarColorSelected);
				TRect lineRect(TPoint(x, y-iOffset%iLineHeight), TSize(iw, iLineHeight));

				aBitmapContext.DrawRect(lineRect);
				aBitmapContext.SetBrushStyle(CGraphicsContext::ENullBrush);
			}
			aBitmapContext.SetPenColor(iFontColorSelected);
    } else
      aBitmapContext.SetPenColor(iFontColor);

    for (TInt j=0; j<iData->LastColumn(); j++) {

      len= p.Locate(KColumnListSeparator);

      if (iData->ColumnIsGraphics(j) && iData->IconArray())
	  {
			TLex lex(p.Left(len));
			if (lex.Val(idx)==KErrNone)
			{
				// Patch for V1 skins (they don't have a play list icon)
				if (iData->IconArray()->Count() == idx)
					idx = EFileName; // Use the file icon instead

				if ((idx == ETitle) || (idx == EFileName) || (idx == EPlayList))
				{
					// These items can be stopped, paused or playing
					TLex lex(p.Right(1));

					TInt playState(0);
					lex.Val(playState);
					if (playState)
						idx = playState;
				}

				CGulIcon* icn= (*iData->IconArray())[idx];
				TSize s(icn->Bitmap()->SizeInPixels());
				TPoint pt(x+iData->ColumnWidthPixel(j)/2-s.iWidth/2,y+iLineHeight/2-s.iHeight/2-iOffset%iLineHeight);
				aBitmapContext.BitBltMasked(pt,icn->Bitmap(), TRect(TPoint(0,0),s), icn->Mask(), ETrue);
			}
      }
      else
	  {
			TRect lineRect(TPoint(x, y+iLinePadding-iOffset%iLineHeight), TSize(iData->ColumnWidthPixel(j), iFontHeight));
			aBitmapContext.DrawText(p.Left(len), lineRect, iFontAscent, iData->ColumnAlignment(j));
      }

      p.Set(p.Mid(len+1));
      x+= iData->ColumnWidthPixel(j);
	  
	  //FIXME: On S60, ColumnHorizontalGap returns 1080049501 !!?
#if !defined(SERIES60)
	  x+= iData->ColumnHorizontalGap(j);
#else
	  x+=2;
#endif
    }

    y+= iLineHeight;
    if (y>iy+ih) break;
  }
}

void COggListBox::Cycle()
	{
	if (!iVisible)
		return;

	if (iCycle>0)
		{
		iCycle--;
		iOffset -= (iScroll*iLineHeight)/KListboxcycles;
		iRedraw = ETrue;
		}
	else
		{
		if (iOffset != 0)
			{
			iOffset = 0;
			iRedraw = ETrue;
			}
		}
	}

void COggListBox::PointerEvent(const TPointerEvent& p)
{
  COggControl::PointerEvent(p);
  if (p.iType==TPointerEvent::EButton1Down /*|| p.iType==TPointerEvent::EDrag*/) {
    TInt idx= GetLineFromPos(p.iPosition.iY-iy);
    SetCurrentItemIndex(idx);
    if (iObserver) iObserver->OggControlEvent(this,2,idx);
    iRedraw= ETrue;
  } 
}

void COggListBox::ControlEvent(TInt anEventType, TInt aValue)
{
  COggControl::ControlEvent(anEventType, aValue);
  if (anEventType==0) SetTopIndex(aValue);
}

TBool COggListBox::ReadArguments(TOggParser& p)
{
  TBool success= COggControl::ReadArguments(p);
  if (success && p.iToken==_L("Font")) {
    p.Debug(_L("Setting font."));
    CFont* aFont= p.ReadFont();
    success= aFont!=0;
    if (success) SetFont(aFont, ETrue);
  }
  if (success && p.iToken==_L("FontColor")) {
    p.Debug(_L("Setting font color."));
    TRgb col(0,0,0);
    success= p.ReadColor(col);
    if (success) SetFontColor(col);
  }
  if (success && p.iToken==_L("FontColorSelected")) {
    p.Debug(_L("Setting selected font color."));
    TRgb col(255,0,0);
    success= p.ReadColor(col);
    if (success) SetFontColorSelected(col);
  }
  if (success && p.iToken==_L("BarColorSelected")) {
    p.Debug(_L("Setting selected bar color."));
    TRgb col(255,0,0);
    success= p.ReadColor(col);
    if (success) SetBarColorSelected(col);
  }
  return success;
}


COggCanvas::COggCanvas()
: iControls(10)
	{
	}

COggCanvas::~COggCanvas()
	{
	DestroyBitmap();

	delete iBackground;
	}

void COggCanvas::LoadBackgroundBitmapL(const TFileName& aFileName, TInt iIdx)
	{
	delete iBackground;
	iBackground = NULL;

	iBackground = new(ELeave) CFbsBitmap;
	TInt err= iBackground->Load(aFileName,iIdx);
	if (err != KErrNone)
		{
		// FIXME (Don't do the dialog display here)
		TBuf<256> buf;
		CEikonEnv::Static()->ReadResource(buf, R_OGG_ERROR_21);
		buf.AppendNum(err);
		COggMsgEnv::OggErrorMsgL(buf, _L(" "));

		delete iBackground;
		iBackground = NULL;

		User::Leave(err);
		}

	DestroyBitmap();
	CreateBitmapL(iBackground->SizeInPixels());

	SetSize(iBackground->SizeInPixels());
	EnableDragEvents();
	DrawControl();
	}

void COggCanvas::Refresh()
	{
	TBool redrawRequired = EFalse;
	TInt i;
	for (i = 0 ; i<iControls.Count() ; i++) 
		{
		if (iControls[i]->iRedraw)
			{
			redrawRequired = ETrue;
			break;
			}
		}

	if (redrawRequired)
		{
		DrawControl(*iBitmapContext, *iBitmapDevice);

		Window().Invalidate();
		ActivateGc();
		Window().BeginRedraw();
		CWindowGc& gc=SystemGc();

#if defined(SERIES90)
		// Series 90 has a problem redrawing parts of the screen, so redraw everything
		TRect rect = TRect(iBitmap->SizeInPixels());
		gc.BitBlt(rect.iTl, iBitmap, rect);
#else
		for (i = 0 ; i<iControls.Count() ; i++)
			{
			if (iControls[i]->iRedraw)
				{
				TRect rect = iControls[i]->Rect();
				gc.BitBlt(rect.iTl, iBitmap, rect);

				iControls[i]->iRedraw = EFalse;
				}
			}
#endif

		Window().EndRedraw();
		DeactivateGc();
		}
	}

void COggCanvas::Invalidate()
	{
	DrawNow();
	}

COggControl* COggCanvas::GetControl(TInt i)
	{
	return iControls[i];
	}

void COggCanvas::AddControl(COggControl* c)
	{
	iControls.AppendL(c);
	}

void COggCanvas::ClearControls()
	{
	iControls.ResetAndDestroy();
	}

void COggCanvas::DrawControl()
	{
    iBitmapContext->SetClippingRect(Rect());
    iBitmapContext->BitBlt(TPoint(0,0),iBackground);

    // DrawControl will draw relative to its Position().
    // when drawing to the bitmap gc, Position() should be (0,0)
    TPoint	position = iPosition;
    iPosition = TPoint(0,0);
	TInt i;
	TInt iMax = iControls.Count();
    for (i = 0 ; i < iMax ; i++)
		iControls[i]->iRedraw = ETrue;

	DrawControl(*iBitmapContext, *iBitmapDevice);

	for (i = 0 ; i < iMax ; i++)
		iControls[i]->iRedraw = EFalse;

	iPosition = position;
	}

void COggCanvas::DestroyBitmap()
	{
	delete iBitmapContext;
	iBitmapContext = NULL;

	delete iBitmapDevice;
	iBitmapDevice = NULL;

	delete iBitmap;
	iBitmap = NULL;

	delete iOverlayMaskContext;
	iOverlayMaskContext = NULL;

	delete iOverlayMaskDevice;
	iOverlayMaskDevice = NULL;

	delete iOverlayMaskBitmap;
	iOverlayMaskBitmap = NULL;
	}

void COggCanvas::CreateBitmapL(const TSize& aSize)
	{
	iBitmap = new(ELeave) CFbsBitmap;
	iBitmap->Create(aSize, iEikonEnv->ScreenDevice()->DisplayMode());
  
	iBitmapDevice = CFbsBitmapDevice::NewL(iBitmap);
	User::LeaveIfError(iBitmapDevice->CreateContext(iBitmapContext));

	iOverlayMaskBitmap = new(ELeave) CFbsBitmap;
	iOverlayMaskBitmap->Create(aSize, EGray2);

	iOverlayMaskDevice = CFbsBitmapDevice::NewL(iOverlayMaskBitmap);
	User::LeaveIfError(iOverlayMaskDevice->CreateContext(iOverlayMaskContext));
	
	iOverlayMaskContext->SetBrushColor(KRgbWhite);
	iOverlayMaskContext->Clear();
	}

void COggCanvas::Draw(const TRect& aRect) const
	{
	CWindowGc& gc=SystemGc();
	gc.BitBlt(aRect.iTl, iBitmap, TRect(aRect.iTl - Position(), aRect.Size()));
	}

void COggCanvas::DrawControl(CBitmapContext& aBitmapContext, CBitmapDevice& /*aBitmapDevice*/) const
	{
	// Clear backgrounds
	TInt i, iMax = iControls.Count();
	for (i = 0 ; i<iMax ; i++)
		{
		COggControl* c= iControls[i];
		if (c->iRedraw)
			{
			aBitmapContext.SetClippingRect(Rect());

			// If the control has an overlay mask we clear using the mask
			if (c->iOverlayMask)
				{
				//Set up the mask
				iOverlayMaskContext->BitBlt(TPoint(c->ix, c->iy), c->iOverlayMask, TRect(TPoint(0, 0), TSize(c->iw,c->ih)));

				// Do a mask clear
				aBitmapContext.BitBltMasked(TPoint(c->ix, c->iy), iBackground, TRect(TPoint(c->ix,c->iy), TSize(c->iw,c->ih)), iOverlayMaskBitmap, ETrue);

				// Clear the mask (ready for next time)
				iOverlayMaskContext->Clear(TRect(TPoint(c->ix, c->iy), TSize(c->iw,c->ih)));
				}
			else
				{
				// Clear the entire rectangle
				aBitmapContext.BitBlt(TPoint(c->ix, c->iy), iBackground, TRect(TPoint(c->ix,c->iy), TSize(c->iw,c->ih)));
				}
			}
		}

	// Redraw controls
	for (i = 0 ; i<iMax ; i++)
		{
		COggControl* c= iControls[i];
		if (c->iRedraw && c->IsVisible())
			{
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

void COggCanvas::CycleHighFrequencyControls()
	{
	for (TInt i = 0 ; i<iControls.Count() ; i++)
		{
		COggControl* control = iControls[i];
		if (control->HighFrequency())
			control->Cycle();
		}
	}

void COggCanvas::CycleLowFrequencyControls()
	{
	for (TInt i = 0 ; i<iControls.Count() ; i++)
		{
		COggControl* control = iControls[i];
		if (!control->HighFrequency())
			control->Cycle();
		}
	}
