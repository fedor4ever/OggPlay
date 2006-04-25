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

// Platform settings
#include <OggOs.h>

// This file is for Series 60 only
#if defined(SERIES60)

#include "OggSettingsContainer.h"
#include <aknlists.h>
#include <aknutils.h>


void COggSettingsContainer::ConstructL(const TRect& aRect, TUid aId)
{
    CreateWindowL();
        
    if (aId ==  KOggPlayUidSettingsView)
    {
        COggS60Utility::DisplayStatusPane(R_OGG_SETTINGS);
        iListBox = new(ELeave) COggplayDisplaySettingItemList((COggPlayAppUi &)*CEikonEnv::Static()->AppUi());
        iListBox->ConstructFromResourceL(R_OGGPLAY_DISPLAY_SETTING_ITEM_LIST);
    }
#if defined(MULTI_THREAD_PLAYBACK)
	else if (aId == KOggPlayUidPlaybackOptionsView)
	{
        COggS60Utility::DisplayStatusPane(R_OGG_PLAYBACK);
        iListBox = new(ELeave) COggplayDisplaySettingItemList((COggPlayAppUi &)*CEikonEnv::Static()->AppUi());
        iListBox->ConstructFromResourceL(R_OGGPLAY_DISPLAY_PLAYBACK_OPTIONS_ITEM_LIST);
	}
#endif
	else if (aId == KOggPlayUidAlarmSettingsView)
	{
        COggS60Utility::DisplayStatusPane(R_OGG_ALARM_S60);
        iListBox = new(ELeave) COggplayDisplaySettingItemList((COggPlayAppUi &)*CEikonEnv::Static()->AppUi());
        iListBox->ConstructFromResourceL(R_OGGPLAY_ALARM_S60_SETTING_ITEM_LIST);
	}
    else
        User::Leave(KErrNotSupported);
    
    SetRect(aRect);
    ActivateL();
    iListBox->SetMopParent(this);
    iListBox->LoadSettingsL();
    
    iListBox->SetRect(aRect);
    iListBox->ActivateL();
}

COggSettingsContainer::~COggSettingsContainer()
    {
    if (iListBox)
	    {
      TRAPD(ignore, iListBox->StoreSettingsL());
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
	response = iListBox->OfferKeyEventL(aKeyEvent, aType);
  }

  return response;
}

void COggSettingsContainer::VolumeGainChangedL()
{
	if (iListBox)
		((COggplayDisplaySettingItemList*) iListBox)->VolumeGainChangedL();
}

#if defined(MULTI_THREAD_PLAYBACK)
void COggSettingsContainer::BufferingModeChangedL()
{
	if (iListBox)
		((COggplayDisplaySettingItemList*) iListBox)->BufferingModeChangedL();
}
#endif


COggplayDisplaySettingItemList::COggplayDisplaySettingItemList(COggPlayAppUi& aAppUi)
: iData(aAppUi.iSettings), iAppUi(aAppUi)
{
}

CAknSettingItem* COggplayDisplaySettingItemList::CreateSettingItemL(TInt aIdentifier)
{
  switch (aIdentifier)
  {
  case EOggRepeat:
    return new(ELeave) CRepeatSettingItem(aIdentifier, iAppUi);

  case EOggSettingScanDir:
    return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iScanmode);

  case EOggSettingManeuvringSpeed:
    return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iManeuvringSpeed);

  case EOggSettingAutoPlayId:
    return new(ELeave) CAknBinaryPopupSettingItem(aIdentifier, iData.iAutoplay);

  case EOggSettingWarningsId:
    return new(ELeave) CAknBinaryPopupSettingItem(aIdentifier, iData.iWarningsEnabled);

  case EOggSettingRskIdle:
    return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iSoftKeysIdle[0]);

  case EOggSettingRskPlay:
    return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iSoftKeysPlay[0]);

  case EOggSettingVolumeBoost:
     return iGainSettingItem = new(ELeave) CGainSettingItem(aIdentifier, iAppUi);

#if defined(MULTI_THREAD_PLAYBACK)
  case EOggSettingBufferingMode:
    return iBufferingModeItem = new(ELeave) CBufferingModeSettingItem(aIdentifier, iAppUi);

  case EOggSettingThreadPriority:
    return new(ELeave) CThreadPrioritySettingItem(aIdentifier, iAppUi);
#endif

  case EOggAlarmActive:
    return new(ELeave) CAlarmSettingItem(aIdentifier, iAppUi);

  case EOggAlarmTime:
    return new(ELeave) CAlarmTimeSettingItem(aIdentifier, iAppUi);

  case EOggAlarmVolume:
     return new(ELeave) CAknVolumeSettingItem(aIdentifier, iData.iAlarmVolume);

  case EOggAlarmBoost:
	return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iAlarmGain);

  case EOggAlarmSnooze:
	return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iAlarmSnooze);

  default:
    break;
  }

  return NULL;
}

// CRepeatItem
CRepeatSettingItem::CRepeatSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknBinaryPopupSettingItem(aIdentifier, aAppUi.iSettings.iRepeat), iAppUi(aAppUi)
{
}
 
void CRepeatSettingItem::EditItemL(TBool aCalledFromMenu)
{
  CAknBinaryPopupSettingItem::EditItemL(aCalledFromMenu);
  iAppUi.SetRepeat(InternalValue());
}


void COggplayDisplaySettingItemList::VolumeGainChangedL()
{
	if (iGainSettingItem)
		{
		iGainSettingItem->LoadL();
		HandleChangeInItemArrayOrVisibilityL();
		}
}

#if defined(MULTI_THREAD_PLAYBACK)
void COggplayDisplaySettingItemList::BufferingModeChangedL()
{
	if (iBufferingModeItem)
		{
		iBufferingModeItem->LoadL();
		HandleChangeInItemArrayOrVisibilityL();
		}
}
#endif

// Overload of the Gain setting, so that change will take place right away
CGainSettingItem::CGainSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknEnumeratedTextPopupSettingItem(aIdentifier, aAppUi.iSettings.iGainType), iAppUi(aAppUi)
{
}
 
void  CGainSettingItem::EditItemL(TBool aCalledFromMenu)
{
  CAknEnumeratedTextPopupSettingItem::EditItemL(aCalledFromMenu);
  iAppUi.SetVolumeGainL((TGainType) InternalValue());
}


#if defined(MULTI_THREAD_PLAYBACK)
CBufferingModeSettingItem::CBufferingModeSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknEnumeratedTextPopupSettingItem(aIdentifier, aAppUi.iSettings.iBufferingMode), iAppUi(aAppUi)
{
}
 
void  CBufferingModeSettingItem::EditItemL(TBool aCalledFromMenu)
{
  CAknEnumeratedTextPopupSettingItem::EditItemL(aCalledFromMenu);
  iAppUi.SetBufferingModeL((TBufferingMode) InternalValue());
}


CThreadPrioritySettingItem::CThreadPrioritySettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknBinaryPopupSettingItem(aIdentifier, aAppUi.iSettings.iThreadPriority), iAppUi(aAppUi)
{
}

void  CThreadPrioritySettingItem::EditItemL(TBool aCalledFromMenu)
{
  CAknBinaryPopupSettingItem::EditItemL(aCalledFromMenu);
  iAppUi.SetThreadPriority((TStreamingThreadPriority) InternalValue());
}
#endif

CAlarmSettingItem::CAlarmSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknBinaryPopupSettingItem(aIdentifier, aAppUi.iSettings.iAlarmActive), iAppUi(aAppUi)
{
}
 
void CAlarmSettingItem::EditItemL(TBool aCalledFromMenu)
{
  CAknBinaryPopupSettingItem::EditItemL(aCalledFromMenu);
  iAppUi.SetAlarm(InternalValue());
}

CAlarmTimeSettingItem::CAlarmTimeSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknTimeOrDateSettingItem(aIdentifier, CAknTimeOrDateSettingItem::ETime, aAppUi.iSettings.iAlarmTime), iAppUi(aAppUi)
{
}
 
void CAlarmTimeSettingItem::EditItemL(TBool aCalledFromMenu)
{
  CAknTimeOrDateSettingItem::EditItemL(aCalledFromMenu);
  StoreL();
  
  iAppUi.SetAlarmTime();
}

#endif /* SERIES60 */
