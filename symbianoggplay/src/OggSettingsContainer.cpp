/*
 *  Copyright (c) 2003 P. Wolff
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

#include "OggSettingsContainer.h"

COggplayDisplaySettingItemList::COggplayDisplaySettingItemList(TOggplaySettings& aData)
: iData(aData)
{
}


CAknSettingItem* COggplayDisplaySettingItemList::CreateSettingItemL(TInt aIdentifier)
{
  switch (aIdentifier)
		{
  case EOggSettingScanDir:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iScanmode);
  case EOggSettingManeuvringSpeed:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iManeuvringSpeed);
  case EOggSettingAutoPlayId:
    return new (ELeave) CAknBinaryPopupSettingItem(aIdentifier,
      iData.iAutoplay);
  case EOggSettingWarningsId:
    return new (ELeave) CAknBinaryPopupSettingItem(aIdentifier,
      iData.iWarningsEnabled);
  case EOggSettingRskIdle:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iRskIdle);
  case EOggSettingRskPlay:
    return new (ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, 
      iData.iRskPlay);
  default:
    break;
		}
  return NULL;
}


// ---------------------------------------------------------

void COggSettingsContainer::ConstructL(const TRect& aRect)
    {
    CreateWindowL();

    SetRect(aRect);
    ActivateL();
    
    iListBox = new (ELeave) COggplayDisplaySettingItemList(((COggPlayAppUi*)CEikonEnv::Static()->AppUi())->iSettings);
    iListBox->SetMopParent(this);
    iListBox->ConstructFromResourceL(R_OGGPLAY_DISPLAY_SETTING_ITEM_LIST);

    iListBox->LoadSettingsL();

    iListBox->SetRect(aRect);
    iListBox->ActivateL();

    }

COggSettingsContainer::~COggSettingsContainer()
    {
    if (iListBox)
	    {
      TRAPD(ignore, iListBox->StoreSettingsL());
      ((COggPlayAppUi*)CEikonEnv::Static()->AppUi())->WriteIniFile();
      delete iListBox;
	    }
    }

void COggSettingsContainer::SizeChanged()
    {
    if (iListBox) iListBox->SetRect(Rect());
    }

TInt COggSettingsContainer::CountComponentControls() const
    {
    if (iListBox)
      return 1;
    else
      return 0;
    }

CCoeControl* COggSettingsContainer::ComponentControl(TInt aIndex) const
    {
    switch ( aIndex )
        {
        case 0:
            return iListBox;
        default:
            return NULL;
        }
    }

void COggSettingsContainer::Draw(const TRect& aRect) const
    {
    CWindowGc& gc = SystemGc();
    gc.SetPenStyle(CGraphicsContext::ENullPen);
    gc.SetBrushColor(KRgbGray);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(aRect);
    }

void COggSettingsContainer::HandleControlEventL(
    CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)
    {
    }

TKeyResponse COggSettingsContainer::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
  TKeyResponse response=EKeyWasNotConsumed;
  if (iListBox)
  {
    if ((aKeyEvent.iCode == EKeyUpArrow) || (aKeyEvent.iCode == EKeyDownArrow) ||
      (aKeyEvent.iCode == EKeyDevice3))
    {
      response = iListBox->OfferKeyEventL(aKeyEvent, aType);
    }
  }
  return response;
}

