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

#ifndef _OggPlayAppView_h
#define _OggPlayAppView_h

#include "OggControls.h"
#include "OggPlay.h"
#include "OggFiles.h"
#include "OggUserHotkeys.h"


class COggTimer;
class COggViewBase;
class COggPlayAppView : public CCoeControl, public MOggControlObserver
	{
public:
	COggPlayAppView();
	~COggPlayAppView();

	void ConstructL(COggPlayAppUi *aApp);

	// Display functions
	void InitView();
	void FillView(COggPlayAppUi::TViews theNewView, COggPlayAppUi::TViews thePreviousView, const TDesC& aSelection);
	void Invalidate();
	void Update(); // calls all of the following:
	void UpdateListbox();
	void UpdateControls();
	void UpdateSongPosition(TBool aUpdateDuration = EFalse);
	void UpdateClock(TBool aForceUpdate = EFalse);
	void UpdateVolume();
	void UpdateVolumeBoost();
	void UpdatePlaying();
	void UpdateAnalyzer();

	void UpdateRepeat(); 
	void UpdateRandom();

	// Functions to interract with the listbox
	void SelectItem(TInt idx);
	COggListBox::TItemTypes GetItemType(TInt idx);
	TInt GetSelectedIndex();
	CDesCArray* GetTextArray();
	TBool HasAFileName(TInt idx);
	const TDesC& GetFileName(TInt idx);
	TOggFile* GetFile(TInt idx);
	const COggPlayAppUi::TViews GetViewName(TInt idx);
	void GetFilterData(TInt idx, TDes &aData);
	void SelectFile(TOggFile* aFile);
	void ListBoxPageDown();
	void ListBoxPageUp();
  
	void SetAlarm();
	void SnoozeAlarm();
	void ClearAlarm();

	// Handle flip-closed and flip-open modes
	TBool IsFlipOpen();
	void GotoFlipClosed();
	void GotoFlipOpen();
	void AppToForeground(const TBool aForeground) const;

	void Activated(COggViewBase* aActivatedView);
	void Deactivated(COggViewBase* aDeactivatedView);

	void ChangeLayout(TInt aLayout);

	void ReadSkin(const TFileName& aFileName);
	void ReadControls();
	TBool SetHotkeysFromSkin(TOggParser& p);
	TBool ReadHotkeyArgument(TOggParser& p);

	void SetupListBox(COggListBox* aListBox, TSize aSize);

	// Keyboard focus handling
	void SetNextFocus();
	void SetPrevFocus();

	TBool CanPlay();
	TBool CanPause();
	TBool CanStop();
	TBool PlayDimmed();

	static TInt CallBack(TAny* aPtr); // registered with iTimer (CPeriodic)
	void HandleCallBack();

	void RestartCallBack();
	void StopCallBack();

	static TInt AlarmCallBack(TAny* aPtr);
	static TInt AlarmErrorCallBack(TAny* aPtr);
	void HandleAlarmCallBack();

	// From CCoeControl
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	TInt CountComponentControls() const;
	CCoeControl* ComponentControl(TInt aIndex) const;

	// From MOggControlObserver
	void OggPointerEvent(COggControl* c, const TPointerEvent& p);
	void OggControlEvent(COggControl* c, TInt aEventType, TInt aValue);
	void AddControlToFocusList(COggControl* c);

	void HandleNewSkinLayout();
	void OggSeek(TInt aSeekTime);

private:
	void  ReadCanvas(TInt aCanvas, TOggParser& p);
	void  ClearCanvases();

	const TInt GetValueFromTextLine(TInt idx);

public:
	TOggFiles* iOggFiles;

private:
	TInt iLayout;
	TInt64 iPosChanged;
	COggPlayAppUi* iApp;

	CPeriodic* iTimer;
	TCallBack* iCallBack;

	COggTimer* iAlarmTimer;
	TCallBack* iAlarmCallBack;
	TCallBack* iAlarmErrorCallBack;

	TFileName  iIconFileName;
	CDesCArray* iTextArray;
	TInt       iSelected; 
  
	//keyboard focus handling: 
	// Do we do keyboard focus handling
	TBool      iFocusControlsPresent;

	// Maintain a doubly linked list of controls that can get focus.
	TDblQue<COggControl> iFocusControlsHeader;
	TDblQueIter<COggControl> iFocusControlsIter;

	/// COggCanvas[0..1], flip open and closed
	TInt iMode; // 0 = flip-open ; 1= flip-closed
	TInt iModeMax;
	COggCanvas* iCanvas[2];

	// Ogg controls
	// (one for flip-closed/open each, can be the same pointer)
	COggText* iTitle[2];
	COggText* iArtist[2];
	COggText* iAlbum[2];
	COggText* iGenre[2];
	COggText* iTrackNumber[2];
	COggText* iFileName[2];
	COggText* iClock[2];
	COggText* iAlarm[2];
	COggText* iPlayed[2];

	COggDigits* iPlayedDigits[2];
	COggDigits* iTotalDigits[2];

	COggIcon* iAlarmIcon[2];
	COggIcon* iRandomIcon[2];
	COggIcon* iPlaying[2];

	COggMultiStateIcon* iRepeatIcon[2];
	COggBlinkIcon* iPaused[2];

	COggSlider* iVolume[2];
	COggSlider* iVolumeBoost[2];
	COggSlider64* iPosition[2];

	COggButton* iPlayButton[2];
	COggButton* iPauseButton[2];
	COggButton* iPlayButton2[2];
	COggButton* iPauseButton2[2];
	COggButton* iStopButton[2];
	COggButton* iNextSongButton[2];
	COggButton* iPrevSongButton[2];
	COggButton* iRepeatButton[2];
	COggButton* iRandomButton[2];

	COggAnalyzer* iAnalyzer[2];

	COggListBox*  iListBox[2];
	COggScrollBar* iScrollBar[2];

	COggAnimation* iAnimation[2];
	COggAnimation* iLogo[2];

	TInt iCycleFrequencyCounter;
	TInt iCurrentClockMinute;

	TInt64 iTimeTot;

	COggViewBase* iHead;
	};

#endif
