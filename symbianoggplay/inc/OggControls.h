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

#ifndef OggControls_h
#define OggControls_h

#include "OggOs.h"

#include <coecntrl.h>
#include <eikclbd.h>
#include <mdaimageconverter.h>
#include <gulicon.h>
#include <badesca.h>
#include <flogger.h>

#include <stdio.h>

// MOggControlObserver:
// Observe mouse pointer down/drag/up events for COggControls

class COggControl;
class COggCanvas;

class MOggControlObserver {

 public:

  virtual void OggPointerEvent(COggControl* /*c*/, const TPointerEvent& /*p*/) {}
  virtual void OggControlEvent(COggControl* /*c*/, TInt /*aEventType*/, TInt /*aValue*/) {}
  virtual void AddControlToFocusList(COggControl* /*c*/) {}
};


enum TAnalyzerModes {
  EZero,
  EOne,
  EPeak,
  EDecay    /** Peak hold, constant decay for S60 DCT analyzer (masks dropouts)   */
  };

const TInt KDCTAnalyzerDynamic = 50;
const TInt KDecaySteplength = 3;


_LIT(KBeginToken,"{");
_LIT(KEndToken,"}");


// TOggParser:
// Read the skin defintion file
// ----------------------------
class TOggParser {

 public:

  enum TState { 
    ESuccess, EFileNotFound, ENoOggSkin,
    EUnknownVersion, EFlipOpenExpected,
    ESyntaxError, EBeginExpected,
    EEndExpected, EBitmapNotFound, EIntegerExpected,
    EOutOfRange};

  TOggParser(const TFileName& aFileName, TInt aScaleFactor = 1);
  ~TOggParser();

  TBool ReadSkin(COggCanvas* fo, COggCanvas* fc);
  void  ReportError();
  void  Debug(const TDesC& txt, TInt level=0);

  TBool ReadHeader();
  TBool ReadEOL();
  TBool ReadToken();
  TBool ReadToken(TInt& aValue);
  CGulIcon* ReadIcon(const TFileName& aBitmapFile);
  TBool     ReadColor(TRgb& aColor);
  CFont*    ReadFont();

  // protected:
  TBool     iDebug;
  FILE*     iFile;
  TState    iState;
  TInt      iLine;
  TBuf<128> iToken;
  TInt      iVersion;
  TInt		iScaleFactor;
};


// COggControl:
// Abstract base type for objects displayed on a COggCanvas
// --------------------------------------------------------
class COggControl : public CBase {

 public:

  COggControl();
  COggControl(TBool aHighFrequency);
  virtual ~COggControl();

  virtual void SetPosition(TInt ax, TInt ay, TInt aw, TInt ah);
  void         SetObserver(MOggControlObserver* obs);

  virtual void MakeVisible(TBool isVisible);
  TBool        IsVisible();
  void         SetDimmed(TBool aDimmed);
  TBool        IsDimmed();
  void         SetFocus(TBool aFocus);
  TBool        Focus();

  virtual void PointerEvent(const TPointerEvent& p);
  virtual void ControlEvent(TInt anEventType, TInt aValue);

  virtual void Redraw(TBool doRedraw = ETrue);
  virtual void SetFocusIcon(CGulIcon* anIcon);

  TRect        Rect();
  TSize        Size();

  TInt   ix;
  TInt   iy;
  TInt   iw;
  TInt   ih;

  CGulIcon*   iFocusIcon;
  TDblQueLink iDlink;
  
  virtual void  SetBitmapFile(const TFileName& aBitmapFile);
  virtual TBool Read(TOggParser& p);

  TBool HighFrequency();

 protected:
  // these can/must be overriden to create specific behaviour:
  virtual void Cycle() {}
  virtual void Draw(CBitmapContext& aBitmapContext) = 0;

  virtual void DrawFocus(CBitmapContext& aBitmapContext);
  void DrawCenteredIcon(CBitmapContext& aBitmapContext, CGulIcon* anIcon);

  // utility functions:
  virtual TBool ReadArguments(TOggParser& p);

  TBool  iRedraw;   // if true: something has changed with the control and the control needs to be redrawn
  TInt   iCycle;    // frame nmber of an animation etc. (1 cycle = 10ms)
  TBool  iVisible;  // if false the control will not be drawn
  TBool  iDimmed;   // if false the control is disabled and rejects pointer and keyboard focus events
  TBool  iAcceptsFocus; // if true the control can accept keyboard focus, else it's never focussed.
  TBool  iFocus;    // if true, control has focus.

  MOggControlObserver* iObserver;
  TFileName iBitmapFile;
  TBool iHighFrequency;

  friend class COggCanvas;
};


// COggText:
// A line of scrollable text
//--------------------------
class COggText : public COggControl {

 public:

  COggText();
  virtual ~COggText();

  void SetText(const TDesC& aText);
  void SetFont(CFont* aFont, TBool ownedByControl = EFalse);
  void SetFontColor(TRgb aColor);
  void ScrollNow();  // restart the text scrolling

  enum ScrollStyle {
    EOnce=0,
    EEndless,
    ETilBorder,
    EBackAndForth, 
    ERoundabout // not implemented
  };
  void SetScrollStyle(TInt aStyle);
  void SetScrollDelay(TInt aDelay);
  void SetScrollStep(TInt aStep);

 protected:

  virtual void Cycle();
  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);
  virtual TBool ReadArguments(TOggParser& p);

  HBufC* iText;
  CFont* iFont;
  TBool  iOwnedByControl;
  TRgb   iFontColor;

  TInt   iTextWidth;
  TBool  iNeedsScrolling;
  TBool  iHasScrolled;
  TInt   iScrollDelay;
  TInt   iScrollStep;
  TInt   iScrollStyle;
  TInt   iDrawOffset;
  TInt   iScrollBackward;
 
 private:
   // scroll to the left off the textarea and repeat
   void CycleOff(void);
   // ... don't repeat
   void CycleOnce(void);
   // only scroll until the right text border touches the textarea and repeat
   void CycleBorder(void);
   // ... and repeat by scrolling to the right
   void CycleBackAndForth(void);
};


// COggIcon:
// An icon (bitmap+mask) that can blink.
// Ownership of the CGulIcon is taken!
//--------------------------------------
class COggIcon : public COggControl {

 public:

  COggIcon();
  virtual ~COggIcon();

  void SetIcon(CGulIcon* anIcon);
  void Blink();
  void Show();
  void Hide();
  void SetBlinkFrequency(TInt aFrequency);

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Cycle();
  virtual void Draw(CBitmapContext& aBitmapContext);

  CGulIcon* iIcon;
  TBool     iBlinkFrequency;
  TBool     iBlinking;
};


// COggAnimation
// Display a sequence of bitmaps.
//-------------------------------
class COggAnimation : public COggControl {

 public:

  COggAnimation();
  virtual ~COggAnimation();

  void SetBitmaps(TInt aFirstBitmap, TInt aNumBitmaps);
  void ClearBitmaps();
  void Start();
  void Stop();
  void SetPause(TInt aPause);
  void SetFrequency(TInt aFrequency);
  void SetStyle(TInt aStyle);

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Cycle();
  virtual void Draw(CBitmapContext& aBitmapContext);

  //CFbsBitmap* iBitmaps;
  CArrayPtrFlat<CFbsBitmap> iBitmaps;     
  TBool       iPause;
  TInt        iFrequency;
  TInt        iFirstBitmap;
  //TInt        iNumBitmaps;
  TInt        iStyle;
};


// COggDigits
// Display numbers with custom bitmaps.
//-------------------------------------
class COggDigits : public COggControl {

 public:

  enum Digits { EDigit0=0, EDigit1, EDigit2, EDitit3, EDigit4,
		EDigit5, EDigit6, EDigit7, EDigit8, EDigit9,
		EDigitColon, EDigitDash, EDigitSlash, EDigitDot,
		ENumDigits };

  COggDigits();
  virtual ~COggDigits();

  void SetText(const TDesC& aText);
  void SetBitmaps(TInt aFirstBitmap);
  void SetMasks(TInt aFirstMask);
  void ClearBitmaps();
  void ClearMasks();

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Draw(CBitmapContext& aBitmapContext);

  HBufC*      iText;
  CArrayPtrFlat<CFbsBitmap> iBitmaps;   
  CArrayPtrFlat<CFbsBitmap> iMasks;
};


// COggButton:
// A bitmapped button. Several bitmaps can be supplied (all optional):
// - A monochrome mask defining active (black) and inactive regions (white)
//   If not suppplied, the entire control area is active.
// - the normal state (not needed if already included in the background bitmap) + mask
// - the disabled state + mask
// - the pressed state + mask
// Ownership of the bitmaps is taken!
//-------------------
class COggButton : public COggControl {

 public:

  COggButton();
  virtual ~COggButton();

  void SetActiveMask(const TFileName& aFileName, TInt anIdx);
  void SetNormalIcon(CGulIcon* anIcon);
  void SetPressedIcon(CGulIcon* anIcon);
  void SetDimmedIcon(CGulIcon* anIcon);
  void SetStyle(TInt aStyle);
  void SetState(TInt aState);

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);

  
  CFbsBitmap* iActiveMask;
  CGulIcon*   iNormalIcon;
  CGulIcon*   iPressedIcon;
  CGulIcon*   iDimmedIcon;

  TInt        iState; // 0 = normal; 1= pressed
  TInt        iStyle; // 0 = action button; 1 = two state button
};


// COggSlider:
// A slider control with different styles:
// style 0 -> part of a bitmap is shown (opened from left to right)
// style 1 -> like 0, but opening from bottom to top
// style 2 -> the bitmap is moved from left to right
// style 3 -> the bitmap is moved from bootom to top
// ----------------------------------------------------------------
class COggSlider : public COggControl {

 public:

  COggSlider();
  virtual ~COggSlider();
  void SetStyle(TInt aStyle);
  void SetKnobIcon(CGulIcon* anIcon);
  void SetValue(TInt aValue);  
  void SetMaxValue(TInt aMaxValue);

  TInt CurrentValue();

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);

  TInt GetPosFromValue(TInt aValue);
  TInt GetValueFromPos(TInt aPos);

  CGulIcon* iKnobIcon;
  TInt      iStyle;
  TInt      iValue;
  TInt      iMaxValue;
  TBool     iIsMoving;
  TInt      iPos; // current position in pixels
};


// COggScrollBar
// A scrollbar that can have an associated control (e.g. a listbox)
//---------------------------------
class COggScrollBar : public COggControl
{

 public:

  COggScrollBar();
  virtual ~COggScrollBar();

  void SetStyle(TInt aStyle);
  void SetKnobIcon(CGulIcon* anIcon);
  void SetScrollerSize(TInt aSize);
  void SetPage(TInt aPage);
  void SetStep(TInt aStep);
  void SetAssociatedControl(COggControl* aControl);
  void SetMaxValue(TInt aMaxValue);
  void SetValue(TInt aValue);

  TInt CurrentValue();

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);

  TInt GetValueFromPos(TInt aPos);
  TInt GetPosFromValue(TInt aValue);

  TBool        iStyle; // 0= horizontal; 1= vertical
  TInt         iValue;
  TInt         iMaxValue;
  COggControl* iAssociated;
  CGulIcon*    iKnobIcon;
  TInt         iScrollerSize;
  TInt         iPos;
  TInt         iPage;
  TInt         iStep;
  TBool        iIsMoving;
};


// COggListBox
// This is a simple list box which holds a visible text in each
// line plus an invisible ("info") string.
//------------------
class COggListBox : public COggControl 
{
public:
#ifdef PLAYLIST_SUPPORT
  enum TItemTypes { ETitle, EAlbum, EArtist, EGenre, ESubFolder, EFileName, EBack, EPlaying, EPaused, EPlayList};
#else
  enum TItemTypes { ETitle, EAlbum, EArtist, EGenre, ESubFolder, EFileName, EBack, EPlaying, EPaused};
#endif

 public:

  COggListBox();
  virtual ~COggListBox();

  void SetText(CDesCArray* aText);
  void SetVertScrollBar(COggScrollBar* aScrollBar);
  void SetFont(CFont* aFont, TBool ownedByControl = EFalse);
    
  void SetFontColor(TRgb aColor);
  void SetFontColorSelected(TRgb aColor);
  void SetBarColorSelected(TRgb aColor);
  virtual void SetPosition(TInt ax, TInt ay, TInt aw, TInt ah);


  CColumnListBoxData* GetColumnListBoxData();

  void ClearText();
  void AppendText(const TDesC& aText);
  TInt CountText();
  CDesCArray* GetTextArray();

  void ScrollBy(TInt nLines);
  TInt CurrentItemIndex();
  TInt NofVisibleLines();
  TInt SetCurrentItemIndex(TInt idx);
  void SetTopIndex(TInt idx);

  virtual void Redraw(TBool doRedraw = ETrue);

 protected:

  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void Cycle();
  virtual void PointerEvent(const TPointerEvent& p);
  virtual void ControlEvent(TInt anEventType, TInt aValue);
  virtual TBool ReadArguments(TOggParser& p);

  TInt GetColumnFromPos(TInt aPos);
  TInt GetLineFromPos(TInt aPos);

  void UpdateScrollBar();

  CFont* iFont;
  TBool  iOwnedByControl;
  TRgb   iFontColor;
  TRgb   iFontColorSelected;
  TBool  iUseBarSelected;
  TRgb   iBarColorSelected;
  TInt   iTop;
  TInt   iSelected;
  TInt   iLineHeight;
  TInt   iLinesVisible;
  TInt   iScroll;
  TInt   iOffset;

  CColumnListBoxData*  iData;
  CDesCArray*          iText;
  COggScrollBar*       iScrollBar;
};


// COggAnalyzer
// A frequency analyzer which does a discrete fourier transformation
// and displays the amplitude in dB of 16 frequency bands.
// --------------------
class COggAnalyzer : public COggControl {

 public:

  COggAnalyzer();
  virtual ~COggAnalyzer();

  virtual void SetPosition(TInt ax, TInt ay, TInt aw, TInt ah);
  void SetBarIcon(CGulIcon* aBarIcon);
  void SetValue(TInt i, TInt theValue);
  void SetStyle(TInt aStyle);
  void RenderFrequencies(short int data[256]);

  void RenderWaveformFromMDCT(const TInt32 * aFreqBins);
#if !defined(SERIES60)
  void RenderWaveform(short int data[2][512]);
#else
  void RenderWaveform(short int *data);
#endif
  void Clear();
  TInt Style();

 protected:

  virtual TBool ReadArguments(TOggParser& p);

  virtual void Cycle();
  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);

  CGulIcon* iBarIcon;
  TInt      iNumValues;
  //TInt      iShowPeaks; // show peak value for iShowPeaks cycles
  TInt      iStyle;     // currently only 0=off; 1=on

  TInt*     iValues;    // the current values
  TInt*     iPeaks;     // the current peak values
  short int iFFTAbs[512];
  short int iFFTIm[512];
  short int iFFTRe[512];
  int       iDx;
};


// COggCanvas
// A canvas that can display and manage several COggControls 
// and which uses an off-screen bitmap for double buffering
//----------------------------------------------------------
class COggCanvas : public CCoeControl //, public MMdaImageUtilObserver
{
 public:

  COggCanvas();
  virtual ~COggCanvas();
  TInt LoadBackgroundBitmapL(const TFileName& aFileName, TInt iIdx);

  void Refresh();
  void Invalidate();

  void DrawControl();  // redraw the off screen bitmap

  // Modify/add/clear controls: 
  COggControl* GetControl(TInt i);
  void AddControl(COggControl* c);
  void ClearControls();

  void CycleHighFrequencyControls();
  void CycleLowFrequencyControls();

  void Read(FILE* tf);

 protected:

  // from MMdaImageUtilObserver:
//  virtual void MiuoOpenComplete(TInt aError);
//  virtual void MiuoConvertComplete(TInt aError);
//  virtual void MiuoCreateComplete(TInt aError);

  // from CCoeControl:
  virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);
  virtual TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

  virtual void DrawControl(CBitmapContext& aBitmapContext, CBitmapDevice& aBitmapDevice) const;

  void SizeChanged();
  void DestroyBitmap();
  void CreateBitmapL(const TSize& aSize);
  void Draw(const TRect& aRect) const;

protected:
  CFbsBitGc*         iBitmapContext;
  CFbsBitmap*	     iBitmap;
  CFbsBitmap*        iBackground;
  CFbsBitmapDevice*  iBitmapDevice;
  
  CArrayPtrFlat<COggControl> iControls;

  COggControl*       iGrabbed; // a control gets grabbed on a pointer button down event
  COggControl*       iFocused; // control that has input focus (UIQ does not need this)
};

#endif
