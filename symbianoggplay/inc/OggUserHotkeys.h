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

/* OggUserHotkeys.cpp - Series 60 only.
 * Contains class 'COggUserHotkeysData' and 'COggUserHotkeys'
 * Let's the user assign hotkeys (0..9,*,#) for specified actions like FWD and REW.
 */

#ifndef OGGUSERHOTKEYS_H
#define OGGUSERHOTKEYS_H

#include <coecntrl.h>
#include <bautils.h>            // BaflUtils
#include <aknquerydialog.h>

#include "OggPlay.h"
#include "OggLog.h"

class CEikTextListBox;

const TInt ENoHotkey = -1;


/// Data store for COggUserHotkeys
//
class COggUserHotkeysData : public CBase
  {
public:
  enum THotkeys {
    EFastForward,
    ERewind,
    ENofHotkeys,
    ENotAssigned = ENofHotkeys
    };
  
  static COggUserHotkeysData* NewL();

  ~COggUserHotkeysData();

  THotkeys Hotkey( TInt aScanCode ) const;

private:

  void InternalizeModelL();
  void ExternalizeModelL() const;

  void SetHotkey( TInt aRow, TInt aScanCode );

  TInt iHotkeys[ENofHotkeys];
  TFileName modelFileName;
  friend class COggUserHotkeys;
  };

/// Control hosting listbox for user hotkey assignments.
//
class COggUserHotkeys : public CCoeControl
{
public:
  COggUserHotkeys( COggUserHotkeysData& aData );
  void ConstructL(const TRect& aRect);
  ~COggUserHotkeys();

private:
	void RefreshListboxModel();
  TInt CountComponentControls() const;
  CCoeControl* ComponentControl(TInt aIndex) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType );

private:
  COggUserHotkeysData& iData;
  CEikTextListBox *iListBox;
  };
#endif
