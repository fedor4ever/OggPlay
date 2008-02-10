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


COggSettingItemList::COggSettingItemList(COggPlayAppUi& aAppUi)
: iData(aAppUi.iSettings), iAppUi(aAppUi)
	{
	}

COggSettingItemList::~COggSettingItemList()
    {
    TRAPD(ignore, StoreSettingsL());
    }

CAknSettingItem* COggSettingItemList::CreateSettingItemL(TInt aIdentifier)
	{
	switch (aIdentifier)
		{
		case EOggSettingRepeatId:
			return new(ELeave) CRepeatSettingItem(aIdentifier, iAppUi);

		case EOggSettingRandomId:
			return new(ELeave) CRandomSettingItem(aIdentifier, iAppUi);

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

		case EOggSettingBufferingMode:
			return iBufferingModeItem = new(ELeave) CBufferingModeSettingItem(aIdentifier, iAppUi);

		case EOggSettingThreadPriority:
			return new(ELeave) CThreadPrioritySettingItem(aIdentifier, iAppUi);

		case EOggSettingVolumeBoost:
			return iGainSettingItem = new(ELeave) CGainSettingItem(aIdentifier, iAppUi);

		case EOggSettingMp3Dithering:
			return new(ELeave) CMp3DitheringSettingItem(aIdentifier, iAppUi);

		case EOggSettingAlarmActive:
			return new(ELeave) CAlarmSettingItem(aIdentifier, iAppUi);

		case EOggSettingAlarmTime:
			return new(ELeave) CAlarmTimeSettingItem(aIdentifier, iAppUi);

		case EOggSettingAlarmVolume:
			return new(ELeave) CAknVolumeSettingItem(aIdentifier, iData.iAlarmVolume);

		case EOggSettingAlarmBoost:
			return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iAlarmGain);

		case EOggSettingAlarmSnooze:
			return new(ELeave) CAknEnumeratedTextPopupSettingItem(aIdentifier, iData.iAlarmSnooze);

		default:
			break;
		}

	return NULL;
	}

void COggSettingItemList::VolumeGainChangedL()
	{
	if (iGainSettingItem)
		{
		iGainSettingItem->LoadL();
		HandleChangeInItemArrayOrVisibilityL();
		}
	}

void COggSettingItemList::BufferingModeChangedL()
	{
	if (iBufferingModeItem)
		{
		iBufferingModeItem->LoadL();
		HandleChangeInItemArrayOrVisibilityL();
		}
	}


CRepeatSettingItem::CRepeatSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknEnumeratedTextPopupSettingItem(aIdentifier, aAppUi.iSettings.iRepeat), iAppUi(aAppUi)
	{
	}
 
void CRepeatSettingItem::EditItemL(TBool aCalledFromMenu)
	{
	CAknEnumeratedTextPopupSettingItem::EditItemL(aCalledFromMenu);
	iAppUi.SetRepeat(InternalValue());
	}

CRandomSettingItem::CRandomSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknBinaryPopupSettingItem(aIdentifier, aAppUi.iSettings.iRandom), iAppUi(aAppUi)
	{
	}
 
void CRandomSettingItem::EditItemL(TBool aCalledFromMenu)
	{
	CAknBinaryPopupSettingItem::EditItemL(aCalledFromMenu);
	iAppUi.SetRandomL(InternalValue());
	}

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

void CThreadPrioritySettingItem::EditItemL(TBool aCalledFromMenu)
	{
	CAknBinaryPopupSettingItem::EditItemL(aCalledFromMenu);
	iAppUi.SetThreadPriority((TStreamingThreadPriority) InternalValue());
	}

CGainSettingItem::CGainSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknEnumeratedTextPopupSettingItem(aIdentifier, aAppUi.iSettings.iGainType), iAppUi(aAppUi)
	{
	}
 
void  CGainSettingItem::EditItemL(TBool aCalledFromMenu)
	{
	CAknEnumeratedTextPopupSettingItem::EditItemL(aCalledFromMenu);
	iAppUi.SetVolumeGainL((TGainType) InternalValue());
	}

CMp3DitheringSettingItem::CMp3DitheringSettingItem(TInt aIdentifier,  COggPlayAppUi& aAppUi)
: CAknBinaryPopupSettingItem(aIdentifier, aAppUi.iSettings.iMp3Dithering), iAppUi(aAppUi)
	{
	}
 
void CMp3DitheringSettingItem::EditItemL(TBool aCalledFromMenu)
	{
	CAknBinaryPopupSettingItem::EditItemL(aCalledFromMenu);
	iAppUi.SetMp3Dithering(InternalValue());
	}


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

#endif // SERIES60
