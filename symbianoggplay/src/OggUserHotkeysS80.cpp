/*
 *  Copyright (c) 2005 OggPlay Team.
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

// This file is for Series 80 only
#if defined(SERIES80)

#include "OggPlay.h"
#include "OggUserHotkeys.h"

static const TInt KMapRssListToCommand[] =
	{
	// Maps the "RESOURCE ARRAY r_user_hotkey_items" from the .rss file
	// to a corresponding index in the table THotkeys. 
	// Must be kept synchronized with the r_user_hotkey_items!
	TOggplaySettings::ENoHotkey,
	TOggplaySettings::EHotkeyFastForward,
	TOggplaySettings::EHotkeyRewind,
	TOggplaySettings::EHotkeyPageUp,
	TOggplaySettings::EHotkeyPageDown,
	TOggplaySettings::EHotkeyNextSong,
	TOggplaySettings::EHotkeyPreviousSong,

	// EKeylock disabled, (not used in S80)
	// EPauseResume, (not used in S80)

	TOggplaySettings::EHotkeyPlay,
	TOggplaySettings::EHotkeyPause,
	TOggplaySettings::EHotkeyStop,
	TOggplaySettings::EHotkeyVolumeBoostUp,
	TOggplaySettings::EHotkeyVolumeBoostDown,
	TOggplaySettings::EHotkeyExit,
	TOggplaySettings::EHotkeyBack,
	TOggplaySettings::EHotkeyVolumeHelp,
	TOggplaySettings::EHotkeyToggleShuffle,
	TOggplaySettings::EHotkeyToggleRepeat
	};

TInt COggUserHotkeysS80::MapRssListToCommand(TInt aRSSIndex)  
	{
	return KMapRssListToCommand[aRSSIndex];
	}
  
  
TInt COggUserHotkeysS80::MapCommandToRssList(TInt aCommandIndex)  
	{
	TInt j;
	TBool found = EFalse;
	for (j = 0 ; j<TOggplaySettings::ENumHotkeys ; j++)
		{
		if (aCommandIndex == KMapRssListToCommand[j])
			{
			found = ETrue;
			break;
			}
		}

	if (found)
		return j;

	return 0; // Return NoHotkeys if not found
	}

void COggUserHotkeysS80::SetSoftkeys(TBool aPlaying)
	{
	TBuf<50> buf;
	TInt action;
	CEikonEnv* eikonEnv = CEikonEnv::Static();
	CEikButtonGroupContainer* cba = eikonEnv->AppUiFactory()->ToolBar();  
	const TOggplaySettings& settings = ((COggPlayAppUi*) eikonEnv->EikAppUi())->iSettings;
  
	CDesCArrayFlat* array = eikonEnv->ReadDesCArrayResourceL(R_USER_HOTKEY_ITEMS);
	CleanupStack::PushL(array);
 
	for (TInt i = 0 ; i<4 ; i++)
		{
		TInt hotkeyIndex = (aPlaying) ? settings.iSoftKeysPlay[i] : settings.iSoftKeysIdle[i];
		switch (hotkeyIndex)
			{
			case TOggplaySettings::ENoHotkey:
				action = EEikCmdCanceled;
				buf.Zero();
    		    break;

			default:
				action = KHotkeysActions[hotkeyIndex];
				buf.Copy(array->MdcaPoint(MapCommandToRssList(hotkeyIndex)));
				break;
			}

		cba->SetCommandL(i, action, buf); 
		}

	CleanupStack::PopAndDestroy(array);
	}

#endif /* SERIES80 */
