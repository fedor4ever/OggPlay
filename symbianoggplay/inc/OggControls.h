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

#ifndef OggControls_h
#define OggControls_h

#include <coecntrl.h>
#include <eikclbd.h>
#include <mdaimageconverter.h>
#include <gulicon.h>
#include <badesca.h>

#include <stdio.h>

// MOggControlObserver:
// Observe mouse pointer down/drag/up events for COggControls

class COggControl;

class MOggControlObserver {

 public:

  virtual void OggPointerEvent(COggControl* /*c*/, const TPointerEvent& /*p*/) {}
  virtual void OggControlEvent(COggControl* /*c*/, TInt /*aEventType*/, TInt /*aValue*/) {}

};


// COggControl:
// Abstract base type for objects displayed on a COggCanvas
// --------------------------------------------------------

class COggControl {

 public:

  COggControl(TInt ax, TInt ay, TInt aw, TInt ah);

  void         SetObserver(MOggControlObserver* obs);

  virtual void MakeVisible(TBool isVisible);
  TBool        IsVisible();
  void         SetDimmed(TBool aDimmed);
  TBool        IsDimmed();

  virtual void PointerEvent(const TPointerEvent& p);
  virtual void ControlEvent(TInt anEventType, TInt aValue);
  TKeyResponse KeyEvent(const TKeyEvent& aKeyEvent, TEventCode aType);

  virtual void Redraw();

  TRect        Rect();
  TSize        Size();

  TInt   ix;
  TInt   iy;
  TInt   iw;
  TInt   ih;

  virtual void  SetBitmapFile(const TFileName& aBitmapFile);

 protected:

  // these can/must be overriden to create specific behaviour:
  virtual void Cycle() {}
  virtual void Draw(CBitmapContext& aBitmapContext) = 0;

  // utility functions:
  TBool ReadEOL(FILE* tf);
  TBool ReadInt(FILE* tf, TInt& aValue);
  TBool ReadString(FILE* tf, TDes& aString);
  TBool ReadTokens(FILE* tf);
  virtual TBool ReadArguments(FILE* tf, const TDesC& token);

  TBool  iRedraw;   // if true: something has changed with the control and the control needs to be redrawn
  TInt   iCycle;    // frame nmber of an animation etc. (1 cycle = 10ms)
  TBool  iVisible;  // if false the control will not be drawn
  TBool  iDimmed;   // if false the control is disabled and rejects pointer events

  MOggControlObserver* iObserver;

  TFileName iBitmapFile;

  friend class COggCanvas;
};


// COggText:
// A line of scrollable text
//--------------------------

class COggText : public COggControl {

 public:

  COggText(TInt ax, TInt ay, TInt aw, TInt ah);
  virtual ~COggText();

  void SetText(const TDesC& aText);
  void SetFont(CFont* aFont);
  void ScrollNow();  // restart the text scrolling

 protected:

  virtual void Cycle();
  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);

  HBufC* iText;
  CFont* iFont;

  TInt   iTextWidth;
  TBool  iNeedsScrolling;
  TBool  iHasScrolled;
  TInt   iScrollDelay;
  TInt   iScrollSpeed;
};


// COggIcon:
// An icon (bitmap+mask) that can blink.
// Ownership of the CGulIcon is taken!
//--------------------------------------

class COggIcon : public COggControl {

 public:

  COggIcon(TInt ax, TInt ay, TInt aw, TInt ah);
  virtual ~COggIcon();

  void SetIcon(CGulIcon* anIcon);
  void Blink();
  void Show();
  void Hide();
  void SetBlinkFrequency(TInt aFrequency);

 protected:

  virtual TBool ReadArguments(FILE* tf, const TDesC& token);

  virtual void Cycle();
  virtual void Draw(CBitmapContext& aBitmapContext);

  CGulIcon* iIcon;
  TBool     iBlinkFrequency;
  TBool     iBlinking;
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

  COggButton(TInt ax, TInt ay, TInt aw, TInt ah);
  virtual ~COggButton();

  void SetActiveMask(const TFileName& aFileName, TInt anIdx);
  void SetNormalIcon(CGulIcon* anIcon);
  void SetPressedIcon(CGulIcon* anIcon);
  void SetDimmedIcon(CGulIcon* anIcon);

 protected:

  virtual TBool ReadArguments(FILE* tf, const TDesC& token);

  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void PointerEvent(const TPointerEvent& p);

  void DrawCenteredIcon(CBitmapContext& aBitmapContext, CGulIcon* anIcon);
  CGulIcon* CreateIcon(FILE* tf);

  CFbsBitmap* iActiveMask;
  CGulIcon*   iNormalIcon;
  CGulIcon*   iPressedIcon;
  CGulIcon*   iDimmedIcon;

  TInt        iState; // 0 = normal; 1= pressed
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

  COggSlider(TInt ax, TInt ay, TInt aw, TInt ah);

  void SetStyle(TInt aStyle);
  void SetKnobIcon(CGulIcon* anIcon);
  void SetValue(TInt aValue);  
  void SetMaxValue(TInt aMaxValue);

  TInt CurrentValue();

 protected:

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

  COggScrollBar(TInt ax, TInt ay, TInt aw, TInt ah);

  void SetStyle(TInt aStyle);
  void SetKnobIcon(CGulIcon* anIcon);
  void SetScrollerSize(TInt aSize);
  void SetPage(TInt aPage);
  void SetAssociatedControl(COggControl* aControl);
  void SetMaxValue(TInt aMaxValue);
  void SetValue(TInt aValue);

  TInt CurrentValue();

 protected:

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
  TBool        iIsMoving;
};


// COggListBox
// This is a simple list box which holds a visible text in each
// line plus an invisible ("info") string.
//------------------

class COggListBox : public COggControl 
{

 public:

  COggListBox(TInt ax, TInt ay, TInt aw, TInt ah);
  virtual ~COggListBox();

  void SetVertScrollBar(COggScrollBar* aScrollBar);
  void SetFont(CFont* aFont);

  CColumnListBoxData* GetColumnListBoxData();

  void ClearText();
  void AppendText(const TDesC& aText);
  TInt CountText();
  const TDesC& GetText(TInt idx);
  CDesCArray* GetTextArray();

  void ScrollBy(TInt nLines);
  TInt CurrentItemIndex();
  void SetCurrentItemIndex(TInt idx);
  void SetTopIndex(TInt idx);

  virtual void Redraw();

 protected:

  virtual void Draw(CBitmapContext& aBitmapContext);
  virtual void Cycle();
  virtual void PointerEvent(const TPointerEvent& p);
  virtual void ControlEvent(TInt anEventType, TInt aValue);

  TInt GetColumnFromPos(TInt aPos);
  TInt GetLineFromPos(TInt aPos);

  void UpdateScrollBar();

  CFont* iFont;
  TInt   iTop;
  TInt   iSelected;
  TInt   iLineHeight;
  TInt   iLinesVisible;
  TInt   iScroll;
  TInt   iOffset;

  CColumnListBoxData*  iData;
  CDesCArrayFlat*      iText;
  COggScrollBar*       iScrollBar;
};


// COggAnalyzer
// A frequency analyzer which does a discrete fourier transformation
// and displays the amplitude in dB of 16 frequency bands.
// --------------------

class COggAnalyzer : public COggControl {

 public:

  COggAnalyzer(TInt ax, TInt ay, TInt aw, TInt ah);
  virtual ~COggAnalyzer();

  void SetBarIcon(CGulIcon* aBarIcon);
  void SetValue(TInt i, TInt theValue);
  void SetStyle(TInt aStyle);
  void RenderFrequencies(short int data[256]);
  void RenderWaveform(short int data[2][512]);
  void Clear();
  TInt Style();

 protected:

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

class COggCanvas : public CCoeControl ,
		   public MMdaImageUtilObserver
{

 public:

  COggCanvas();
  virtual ~COggCanvas();
  void ConstructL(const TFileName& aFileName, TInt iIdx);

  void Refresh();
  void Invalidate();

  void DrawControl();  // redraw the off screen bitmap

  // Modify/add/clear controls: 
  COggControl* GetControl(TInt i);
  void AddControl(COggControl* c);
  void ClearControls();

  void RegisterCallBack(TInt (*fnc)(TAny*));

 protected:

  // from MMdaImageUtilObserver:
  virtual void MiuoOpenComplete(TInt aError);
  virtual void MiuoConvertComplete(TInt aError);
  virtual void MiuoCreateComplete(TInt aError);

  // from CCoeControl:
  virtual void HandlePointerEventL(const TPointerEvent& aPointerEvent);
  virtual TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType);

  void CycleControls();
  virtual void DrawControl(CBitmapContext& aBitmapContext, CBitmapDevice& aBitmapDevice) const;

  void SizeChanged();
  void DestroyBitmap();
  void CreateBitmapL(const TSize& aSize);
  void Draw(const TRect& aRect) const;

protected:

  CMdaImageFileToBitmapUtility* if2bm;

  CFbsBitGc*         iBitmapContext;
  CFbsBitmap*	     iBitmap;
  CFbsBitmap*        iBackground;
  CFbsBitmapDevice*  iBitmapDevice;
  
  CArrayPtrFlat<COggControl> iControls;

  COggControl*       iGrabbed; // a control gets grabbed on a pointer button down event
  COggControl*       iFocused; // control that has input focus (UIQ does not need this)

};

#endif
