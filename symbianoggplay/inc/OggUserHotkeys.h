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

// $CVSHeader$


#ifndef OGGUSERHOTKEYS_H
#define OGGUSERHOTKEYS_H

#include "OggPlay.h"
#include "OggLog.h"

#if defined(SERIES80)

class COggUserHotkeysS80 : public CBase
{
	public: 
	static void SetSoftkeys(TBool aPlaying);
	static TInt MapCommandToRssList(TInt aCommandIndex)  ;
	static TInt MapRssListToCommand(TInt aRSSIndex)  ;
};
#endif

#if defined(SERIES60)

/* OggUserHotkeys.cpp - Series 60 only.
 * Let's the user assign hotkeys (0..9,*,#,backspace,shift) for specified actions like FWD and REW.
 */
 
#include <coecntrl.h>
#include <bautils.h>            // BaflUtils
#include <aknquerydialog.h>

class CEikTextListBox;

/// Control hosting listbox for user hotkey assignments.
//

class COggUserHotkeysControl : public CCoeControl
  {
public:
  COggUserHotkeysControl( TOggplaySettings& aData );
  void ConstructL(const TRect& aRect);
  ~COggUserHotkeysControl();

  static TOggplaySettings::THotkeys Hotkey( const TKeyEvent& aKeyEvent, TEventCode aType, TOggplaySettings* aData );

private : // New
	void RefreshListboxModel();
  void SetHotkey( TInt aRow, TInt aScanCode );

private : // Framework
  TInt CountComponentControls() const;
  CCoeControl* ComponentControl(TInt aIndex) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType );
private:
  TOggplaySettings& iData;
  CEikTextListBox *iListBox;
  };
  
class COggUserHotkeysS60 : public CBase
{
	public: 
	static void SetSoftkeys(TBool aPlaying);
	
};
#endif // SERIES60

#endif
