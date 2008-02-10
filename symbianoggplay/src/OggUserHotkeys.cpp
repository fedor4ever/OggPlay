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

// Platform settings
#include <OggOs.h>

// This file is for Series 60 only
#if defined(SERIES60)

#include <w32std.h>
#include <aknlists.h> 
#include <barsread.h>  // Resource reading
#include <aknapp.h>
#include <s32file.h>

#include "OggUserHotkeys.h"
#include <OggPlay.rsg>
#include "OggPlay.hrh"
#include "OggLog.h"
#include "OggPlay.h"


COggUserHotkeysControl::COggUserHotkeysControl( TOggplaySettings& aData ) : iData(aData)
	{
	}

void COggUserHotkeysControl::ConstructL(const TRect& aRect)
	{
	CreateWindowL();

	iListBox = new(ELeave) CAknSettingStyleListBox;
	iListBox->SetMopParent(this);
	iListBox->SetContainerWindowL(*this);
	iListBox->ConstructL(this, EAknListBoxSelectionList);
	iListBox->Model()->SetOwnershipType(ELbmOwnsItemArray);

	// Scroll bar arrow indicators
	iListBox->CreateScrollBarFrameL(ETrue);
	iListBox->ScrollBarFrame()->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EAuto);

	RefreshListboxModel();
 
	iListBox->ActivateL();
	SetRect(aRect);
	ActivateL();

	iListBox->SetRect(Rect());
	}


COggUserHotkeysControl::~COggUserHotkeysControl()
	{
	delete iListBox;
	}

void COggUserHotkeysControl::SetHotkey(TInt aRow, TInt aCode)
	{
	aRow += TOggplaySettings::EFirstHotkeyIndex;
	if (aRow>TOggplaySettings::EHotkeyVolumeBoostDown)
		aRow += 3; // Skip unused entries

	for (TInt i = TOggplaySettings::EFirstHotkeyIndex ; i<TOggplaySettings::ENumHotkeys ; i++)
		{
		if( i == aRow )
			continue;
    
		// Prevent replicas
		if (iData.iUserHotkeys[i] == aCode)
			iData.iUserHotkeys[i] = TOggplaySettings::ENoHotkey;
		}
	
	iData.iUserHotkeys[aRow] = aCode;
	}

// Static method
TOggplaySettings::THotkeys COggUserHotkeysControl::Hotkey(const TKeyEvent& aKeyEvent, TEventCode aType, TOggplaySettings* aData)
	{
	for (TInt i = TOggplaySettings::EFirstHotkeyIndex ; i<TOggplaySettings::ENumHotkeys ; i++)
		{
		// Special processing for the shift key
		if ((aKeyEvent.iScanCode == EStdKeyLeftShift) || (aKeyEvent.iScanCode == EStdKeyRightShift))
			{
			if ((aData->iUserHotkeys[i] == aKeyEvent.iScanCode) && (aType == EEventKeyDown))
				return (TOggplaySettings::THotkeys) i;
			}
		else
			{
			if ((aData->iUserHotkeys[i] == aKeyEvent.iScanCode) && (aType == EEventKey))
				return (TOggplaySettings::THotkeys) i;
			}
		}

	return TOggplaySettings::ENoHotkey;
	}

void COggUserHotkeysControl::RefreshListboxModel()
	{
	TBuf<64> keyBuf, listboxBuf;
	CDesCArrayFlat* array = iCoeEnv->ReadDesCArrayResourceL(R_USER_HOTKEY_ITEMS);
	CleanupStack::PushL(array);

	CDesCArray* modelArray = (CDesCArray *) iListBox->Model()->ItemTextArray();
	for (TInt i = TOggplaySettings::EFirstHotkeyIndex, j = 0; i<=TOggplaySettings::EHotkeyToggleShuffle ; i++)
		{
		// Skip hotkeys that are not used on S60
		if ((i == TOggplaySettings::EHotkeyExit) || (i == TOggplaySettings::EHotkeyBack) || (i == TOggplaySettings::EHotkeyVolumeHelp))
			continue;

		keyBuf.Zero();
		switch (iData.iUserHotkeys[i])
			{
			case TOggplaySettings::ENoHotkey: 
				keyBuf.Copy(array->MdcaPoint(TOggplaySettings::ENoHotkey));
				break;

			case EStdKeyBackspace:
				iEikonEnv->ReadResource(keyBuf, R_OGG_BACKSPACE);
				break;

			case EStdKeyLeftShift: 
			case EStdKeyRightShift:
				iEikonEnv->ReadResource(keyBuf, R_OGG_SHIFT);
				break;

			case EStdKeyNkpAsterisk: 
				keyBuf.Append(_L("*"));
				break;

			case EStdKeyHash: 
				keyBuf.Append(_L("#"));
				break;

			default:
				keyBuf.Append(iData.iUserHotkeys[i]);
				break;
			}

		TBuf<64> title;
		title.Copy(array->MdcaPoint(j + TOggplaySettings::EFirstHotkeyIndex));
		listboxBuf.Format(_L("\t%S\t\t%S"), &title, &keyBuf);

		modelArray->InsertL(j, listboxBuf);
		if ((modelArray->MdcaCount()-1) > j)
			modelArray->Delete(j+1);

		j++;
		}

	iListBox->HandleItemAdditionL();
	DrawDeferred();

	CleanupStack::PopAndDestroy(array);
	}

TInt COggUserHotkeysControl::CountComponentControls() const
  {
  return 1;
  }

CCoeControl* COggUserHotkeysControl::ComponentControl(TInt /*aIndex*/) const
  {
  return iListBox;
  }

TKeyResponse COggUserHotkeysControl::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
	TInt validKey = TOggplaySettings::ENoHotkey;

	if ((Rng((TInt) '0', aKeyEvent.iScanCode, (TInt) '9') || 
	aKeyEvent.iScanCode == EStdKeyHash || aKeyEvent.iScanCode == EStdKeyNkpAsterisk ||
	aKeyEvent.iScanCode == '*' || aKeyEvent.iScanCode == EStdKeyBackspace ||
	aKeyEvent.iScanCode == EStdKeyLeftShift || aKeyEvent.iScanCode == EStdKeyRightShift) && aType == EEventKeyDown)
		validKey = aKeyEvent.iScanCode;
 
	// Save if a valid key or save a ENoHotkey to clear current entry if LSK (Clear) were pressed
	if (validKey != TOggplaySettings::ENoHotkey || aKeyEvent.iScanCode == EStdKeyDevice0)
		{
		TInt row = iListBox->CurrentItemIndex();
		SetHotkey(row, validKey);
		RefreshListboxModel();
		return EKeyWasConsumed;
		}
	else
		return iListBox->OfferKeyEventL( aKeyEvent, aType );
	}

void COggUserHotkeysS60::SetSoftkeys(TBool aPlaying)
	{
	CEikonEnv * eikonEnv = CEikonEnv::Static();
	COggPlayAppUi * appUi = static_cast <COggPlayAppUi *> ( eikonEnv->AppUi() ) ;
	CEikButtonGroupContainer * Cba = appUi->Cba();  
	TOggplaySettings settings = appUi->iSettings;

	TInt softkey = aPlaying ? settings.iSoftKeysPlay[0] : settings.iSoftKeysIdle[0];
	switch(softkey)
		{
		case TOggplaySettings::EHotkeyStop:
			Cba->AddCommandSetToStackL(R_OPTION_STOP_CBA);
			break;

		case TOggplaySettings::EHotkeyExit:
			Cba->AddCommandSetToStackL(R_OPTION_EXIT_CBA);
			break;

		case TOggplaySettings::EHotkeyPause:
			Cba->AddCommandSetToStackL(R_OPTION_PAUSE_CBA);
			break;

		case TOggplaySettings::EHotkeyPlay:
			Cba->AddCommandSetToStackL(R_OPTION_PLAY_CBA);
			break;

		case TOggplaySettings::EHotkeyBack:
			Cba->AddCommandSetToStackL(R_OPTION_BACK_CBA);
			break;

		default:
			break;
		}
	}

#endif /* SERIES60 */

