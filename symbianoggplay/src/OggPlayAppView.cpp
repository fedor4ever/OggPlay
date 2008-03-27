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

#include <OggOs.h>
#include "OggPlayAppView.h"
#include "OggLog.h"
#include "OggHelperFcts.h"

#include <eikclb.h>     // CEikColumnListbox
#include <eikclbd.h>    // CEikColumnListBoxData
#include <eiktxlbm.h>   // CEikTextListBox(Model)
#include <eikmenub.h>   // CEikMenuBar
#include <apgtask.h>    // TApaTask

#if defined(SERIES60)
#include <aknkeylock.h>
#include <aknquerydialog.h>
#endif

#if defined(SERIES80)
#include <eikenv.h>
#endif

#if defined(UIQ)
#include <quartzkeys.h>	// EStdQuartzKeyConfirm etc.
#endif

#include "OggAbsPlayback.h"

// Time (usecs) between canvas Refresh() for graphics updating
const TInt KCallBackPeriod = 71429; // 14Hz
const TInt KOggControlFreq = 14;

// Literal for clock update
_LIT(KDateString, "%-B%:0%J%:1%T%:3%+B");

// Array of times for snooze
const TInt KSnoozeTime[4] = { 5, 10, 20, 30 };

COggPlayAppView::COggPlayAppView()
: iPosChanged(-1), iFocusControlsPresent(EFalse), iFocusControlsHeader(_FOFF(COggControl, iDlink)),
iFocusControlsIter(iFocusControlsHeader), iCycleFrequencyCounter(KOggControlFreq)
	{
	}

COggPlayAppView::~COggPlayAppView()
	{
	delete iTextArray;

	ClearCanvases();

	if (iTimer)
		iTimer->Cancel();

	delete iTimer;
	delete iCallBack;

	if (iAlarmTimer)
		iAlarmTimer->Cancel();

	delete iAlarmTimer;
	delete iAlarmCallBack;
	delete iAlarmErrorCallBack;

	delete iCanvas[1];
	delete iCanvas[0];

	delete iOggFiles;
	}

void COggPlayAppView::ConstructL(COggPlayAppUi *aApp)
	{
	iTextArray = new(ELeave) CDesCArrayFlat(10);

	iApp = aApp;
	CreateWindowL();

	SetComponentsToInheritVisibility(EFalse);
	iOggFiles = new(ELeave) TOggFiles(aApp);

	// Construct the two canvases for flip-open and closed mode
	// Flip-open mode:
	iCanvas[0] = new(ELeave) COggCanvas;
	iCanvas[0]->SetContainerWindowL(*this);

	// Flip-closed mode
#if defined(UIQ) || defined(SERIES60SUI)
	iCanvas[1] = new(ELeave) COggCanvas;
	iCanvas[1]->SetContainerWindowL(*this);
#endif

	// Set up all the other components
	// Create the canvas refresh timer (KCallBackPeriod):
	iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
	iCallBack = new(ELeave) TCallBack(COggPlayAppView::CallBack, this);

	iAlarmCallBack = new(ELeave) TCallBack(COggPlayAppView::AlarmCallBack, this);
	iAlarmErrorCallBack = new(ELeave) TCallBack(COggPlayAppView::AlarmErrorCallBack, this);
	iAlarmTimer = new(ELeave) COggTimer(*iAlarmCallBack, *iAlarmErrorCallBack);
	}

_LIT(KBeginToken,"{");
_LIT(KEndToken,"}");
void COggPlayAppView::ReadSkin(const TFileName& aFileName)
	{
	ClearCanvases();

	iIconFileName = aFileName.Left(aFileName.Length()-4);
	iIconFileName.Append(_L(".mbm"));

	TOggParser p(iCoeEnv->FsSession(), aFileName);
	if (p.iState!=TOggParser::ESuccess)
		{
		p.ReportError();
		User::Leave(-1);
		return;
		}

	if (!p.ReadHeader())
		{
		p.ReportError();
		User::Leave(-1);
		return;
		}

	p.ReadToken();
	if (p.iToken!=_L("FlipOpen"))
		{
		p.iState= TOggParser::EFlipOpenExpected;
		p.ReportError();
		User::Leave(-1);
		return;
		}
  
	p.ReadToken();
	if (p.iToken!=KBeginToken)
		{
		p.iState= TOggParser::EBeginExpected;
		p.ReportError();
		User::Leave(-1);
		return;
		}

	ReadCanvas(0, p);
	if (p.iState!=TOggParser::ESuccess)
		{
		p.ReportError();
		User::Leave(-1);
		return;
		}

	// First canvas loaded
	iModeMax = 0;

#if defined(UIQ) || defined(SERIES60SUI)
	p.ReadToken();
	if (p.iToken==_L("FlipClosed"))
		{
		p.ReadToken();
		if (p.iToken!=KBeginToken)
			{
			p.iState= TOggParser::EBeginExpected;
			p.ReportError();
			User::Leave(-1);
			return;
			}

		ReadCanvas(1, p);
		if (p.iState!=TOggParser::ESuccess)
			{
			p.ReportError();
			User::Leave(-1);
			return;
			}

		// Second canvas loaded 
		iModeMax = 1;
		}
#endif

	iFocusControlsIter.SetToFirst();
	COggControl* c=iFocusControlsIter;
	if (c)
		c->SetFocus(ETrue);

	if (iMode>iModeMax)
		iMode = iModeMax;
	}

void COggPlayAppView::ClearCanvases()
	{
	COggControl* oggControl;
	iFocusControlsIter.SetToFirst();
	while ((oggControl = iFocusControlsIter++) != NULL)
		{
		oggControl->iDlink.Deque();
		}

	if (iCanvas[0])
		iCanvas[0]->ClearControls();

	if (iCanvas[1])
		iCanvas[1]->ClearControls();

	// Reset all our pointers
	iClock[0] = NULL ; iClock[1] = NULL;
	iAlarm[0] = NULL ; iAlarm[1] = NULL;
	iAlarmIcon[0] = NULL ; iAlarmIcon[1]= NULL;
	iPlayed[0] = NULL ; iPlayed[1] = NULL;
	iPlayedDigits[0] = NULL ; iPlayedDigits[1]= NULL;
	iTotalDigits[0]= NULL ; iTotalDigits[1]= NULL;
	iPaused[0]= NULL ; iPaused[1]= NULL;
	iPlaying[0]= NULL ; iPlaying[1]= NULL;
	iPlayButton[0]= NULL ; iPlayButton[1] = NULL;
	iPauseButton[0]= NULL ; iPauseButton[1]= NULL;
	iPlayButton2[0]= NULL ; iPlayButton2[1] = NULL;
	iPauseButton2[0] = NULL ; iPauseButton2[1] = NULL;
	iStopButton[0] = NULL ; iStopButton[1] = NULL;
	iNextSongButton[0]= NULL ; iNextSongButton[1] = NULL;
	iPrevSongButton[0]= NULL ; iPrevSongButton[1]= NULL;
	iListBox[0] = NULL ; iListBox[1] = NULL;
	iTitle[0] = NULL ; iTitle[1] = NULL;
	iAlbum[0] = NULL ; iAlbum[1] = NULL;
	iArtist[0] = NULL ; iArtist[1] = NULL;
	iGenre[0] = NULL ; iGenre[1] = NULL;
	iTrackNumber[0] = NULL ; iTrackNumber[1] = NULL;
	iFileName[0] = NULL ; iFileName[1] = NULL;
	iPosition[0] = NULL ; iPosition[1] = NULL;
	iVolume[0] = NULL ; iVolume[1] = NULL;
	iVolumeBoost[0] = NULL ; iVolumeBoost[1] = NULL;
	iAnalyzer[0] = NULL ; iAnalyzer[1] = NULL;
	iRepeatIcon[0]= NULL ; iRepeatIcon[1] = NULL;
	iRepeatButton[0] = NULL ; iRepeatButton[1] = NULL;
	iRandomIcon[0] = NULL ; iRandomIcon[1] = NULL;
	iRandomButton[0] = NULL ; iRandomButton[1] = NULL;
	iScrollBar[0] = NULL ; iScrollBar[1] = NULL;
	iAnimation[0] = NULL ; iAnimation[1] = NULL;
	iLogo[0] = NULL ; iLogo[1] = NULL;

	// Reset the max mode
	iModeMax = -1;
	}

void COggPlayAppView::ReadCanvas(TInt aCanvas, TOggParser& p)
	{
	iCanvas[aCanvas]->LoadBackgroundBitmapL(iIconFileName, aCanvas ? 18 : 19);
	iFocusControlsPresent = EFalse;

	CFont* defaultFont= const_cast<CFont*>(iCoeEnv->NormalFont());
	TBool stop(EFalse);
	while (!stop)
		{
		stop = !p.ReadToken() || p.iToken==KEndToken;
		COggControl* c = NULL;

		if (p.iToken == _L("Clock"))
			{ 
			_LIT(KAL,"Adding Clock");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iClock[aCanvas]= (COggText *) c; 
			iClock[aCanvas]->SetFont(defaultFont); 
			}
		else if (p.iToken == _L("Alarm"))
			{ 
			_LIT(KAL,"Adding Alarm");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iAlarm[aCanvas]= (COggText *) c;
			iAlarm[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("Title"))
			{ 
			_LIT(KAL,"Adding Title");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iTitle[aCanvas]= (COggText *) c;
			iTitle[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("Album"))
			{ 
			_LIT(KAL,"Adding Album");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iAlbum[aCanvas]= (COggText *) c;
			iAlbum[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("Artist"))
			{ 
			_LIT(KAL,"Adding Artist");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iArtist[aCanvas]= (COggText *) c;
			iArtist[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("Genre"))
			{ 
			_LIT(KAL,"Adding Genre");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iGenre[aCanvas]= (COggText *) c;
			iGenre[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("TrackNumber"))
			{
			_LIT(KAL,"Adding TrackNumber");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iTrackNumber[aCanvas]= (COggText *) c;
			iTrackNumber[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("FileName"))
			{
			_LIT(KAL,"Adding FileName");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iFileName[aCanvas]= (COggText *) c;
			iFileName[aCanvas]->SetFont(defaultFont);
			}
		else if (p.iToken == _L("Played"))
			{ 
			_LIT(KAL,"Adding Played");
			p.Debug(KAL);

			c = new(ELeave) COggText;
			iPlayed[aCanvas] = (COggText*) c;
			iPlayed[aCanvas]->SetFont(defaultFont);

#if defined(UIQ)
			iPlayed[aCanvas]->SetText(_L("00:00 / 00:00"));
#endif
			}
		else if (p.iToken == _L("PlayedDigits"))
			{
			c = new(ELeave) COggDigits;
			iPlayedDigits[aCanvas]= (COggDigits *) c;
			}
		else if (p.iToken == _L("TotalDigits"))
			{
			c = new(ELeave) COggDigits;
			iTotalDigits[aCanvas] = (COggDigits *) c;
			}
		else if (p.iToken == _L("StopButton"))
			{ 
			c = new(ELeave) COggButton;
			iStopButton[aCanvas] = (COggButton *) c;
			}
		else if (p.iToken == _L("PlayButton"))
			{ 
			_LIT(KAL,"Adding PlayButton");
			p.Debug(KAL);

			c = new(ELeave) COggButton;
			iPlayButton[aCanvas]= (COggButton *) c;
			}
		else if (p.iToken == _L("PauseButton"))
			{ 
			_LIT(KAL,"Adding PauseButton");
			p.Debug(KAL);

			c = new(ELeave) COggButton;
			iPauseButton[aCanvas] = (COggButton *) c;
			iPauseButton[aCanvas]->MakeVisible(EFalse);
			}
		else if (p.iToken == _L("PlayButton2"))
			{ 
			c = new(ELeave) COggButton;
			iPlayButton2[aCanvas] = (COggButton *) c;
			}
		else if (p.iToken == _L("PauseButton2"))
			{ 
			c = new(ELeave) COggButton;
			iPauseButton2[aCanvas] = (COggButton *) c;
			}
		else if (p.iToken == _L("NextSongButton"))
			{ 
			c = new(ELeave) COggButton;
			iNextSongButton[aCanvas] = (COggButton *) c;
			}
		else if (p.iToken == _L("PrevSongButton"))
			{ 
			c = new(ELeave) COggButton;
			iPrevSongButton[aCanvas] = (COggButton *) c;
			}
		else if (p.iToken == _L("PlayingIcon"))
			{ 
			_LIT(KAL,"Adding PlayingIcon");
			p.Debug(KAL);

			c = new(ELeave) COggIcon;
			iPlaying[aCanvas] = (COggIcon *) c;
			}
		else if (p.iToken == _L("PausedIcon"))
			{ 
			_LIT(KAL,"Adding PausedIcon");
			p.Debug(KAL);

			c = new(ELeave) COggBlinkIcon;
			iPaused[aCanvas] = (COggBlinkIcon *) c;
			iPaused[aCanvas]->Hide();
			}
		else if (p.iToken == _L("AlarmIcon"))
			{ 
			_LIT(KAL,"Adding AlarmIcon");
			p.Debug(KAL);

			c = new(ELeave) COggIcon;
			iAlarmIcon[aCanvas] = (COggIcon *) c;
			iAlarmIcon[aCanvas]->MakeVisible(iApp->iSettings.iAlarmActive);
			}
		else if (p.iToken == _L("RepeatIcon"))
			{
			_LIT(KAL,"Adding RepeatIcon");
			p.Debug(KAL);

			c = new(ELeave) COggMultiStateIcon;
			iRepeatIcon[aCanvas] = (COggMultiStateIcon *) c;
			iRepeatIcon[aCanvas]->SetState(iApp->iSettings.iRepeat);
			}    
		else if (p.iToken == _L("RandomIcon"))
			{
			_LIT(KAL,"Adding RandomIcon");
			p.Debug(KAL);

			c = new(ELeave) COggIcon;
			iRandomIcon[aCanvas]= (COggIcon *) c;
			iRandomIcon[aCanvas]->MakeVisible(iApp->iSettings.iRandom);
			}
		else if (p.iToken == _L("RepeatButton"))
			{ 
			_LIT(KAL,"Adding RepeatButton");
			p.Debug(KAL);

			c = new(ELeave) COggButton;
			iRepeatButton[aCanvas]= (COggButton *) c;
			iRepeatButton[aCanvas]->SetStyle(1);
			}
		else if (p.iToken == _L("RandomButton"))
			{ 
			_LIT(KAL,"Adding RandomButton");
			p.Debug(KAL);

			c = new(ELeave) COggButton;
			iRandomButton[aCanvas]= (COggButton*)c;
			iRandomButton[aCanvas]->SetStyle(1);
			}
		else if (p.iToken == _L("Analyzer"))
			{
			_LIT(KAL,"Adding Analyzer");
			p.Debug(KAL);

			c = new(ELeave) COggAnalyzer;
			iAnalyzer[aCanvas] = (COggAnalyzer *) c;
			iAnalyzer[aCanvas]->SetStyle(iApp->iAnalyzerState);
			}
		else if (p.iToken == _L("Position"))
			{
			_LIT(KAL,"Adding Position");
			p.Debug(KAL);

			c = new(ELeave) COggSlider64;
			iPosition[aCanvas] = (COggSlider64 *) c;
			}
		else if (p.iToken == _L("Volume"))
			{
			_LIT(KAL,"Adding Volume");
			p.Debug(KAL);

			c = new(ELeave) COggSlider;
			iVolume[aCanvas] = (COggSlider *) c;
			}
		else if (p.iToken == _L("VolumeBoost"))
			{
			_LIT(KAL,"Adding VolumeBoost");
			p.Debug(KAL);

			c = new(ELeave) COggSlider;
			iVolumeBoost[aCanvas] = (COggSlider *) c;
			iVolumeBoost[aCanvas]->SetMaxValue(EStatic12dB + 1);
			}
		else if (p.iToken == _L("ScrollBar"))
			{
			_LIT(KAL,"Adding ScrollBar");
			p.Debug(KAL);

			c = new(ELeave) COggScrollBar;
			iScrollBar[aCanvas] = (COggScrollBar *) c;
			if (iListBox[aCanvas])
				{
				iListBox[aCanvas]->SetVertScrollBar(iScrollBar[aCanvas]);
				iScrollBar[aCanvas]->SetAssociatedControl(iListBox[aCanvas]);
				}
			}
		else if (p.iToken == _L("ListBox"))
			{
			c = new(ELeave) COggListBox;
			iListBox[aCanvas] = (COggListBox *) c;
			SetupListBox(iListBox[aCanvas], iCanvas[aCanvas]->Size());
			if (iScrollBar[aCanvas])
				{
				iScrollBar[aCanvas]->SetAssociatedControl(iListBox[aCanvas]);
				iListBox[aCanvas]->SetVertScrollBar(iScrollBar[aCanvas]);
				}
			}
		else if (p.iToken == _L("Animation"))
			{
			c = new(ELeave) COggAnimation;
			iAnimation[aCanvas] = (COggAnimation *) c;
			}
		else if (p.iToken==_L("Logo"))
			{
			c = new(ELeave) COggAnimation;
			iLogo[aCanvas] = (COggAnimation *) c;
			}
		else if ((p.iToken == _L("HotKeys")) || (p.iToken == _L("Hotkeys")))
			{
			SetHotkeysFromSkin(p);
			}

		if (c)
			{
			c->SetObserver(this);
			c->SetBitmapFile(iIconFileName);
			c->Read(p);
			iCanvas[aCanvas]->AddControl(c);
			}
		}

	p.Debug(_L("Canvas read."));
	}

TBool COggPlayAppView::SetHotkeysFromSkin(TOggParser& p)
	{
	p.ReadToken();
	if (p.iToken != KBeginToken)
		{
		p.iState= TOggParser::EBeginExpected;
		return EFalse;
		}

	while (p.ReadToken() && (p.iToken != KEndToken) && (p.iState == TOggParser::ESuccess))
		{ 
		ReadHotkeyArgument(p); 
		}

	if ((p.iState == TOggParser::ESuccess) && (p.iToken != KEndToken)) 
		p.iState = TOggParser::EEndExpected;

	return (p.iState == TOggParser::ESuccess);
	}

TBool COggPlayAppView::ReadHotkeyArgument(TOggParser& p)
	{
	TInt numkey;
	if (p.iToken == _L("FastForward"))
		{
		p.Debug(_L("Setting FastForward."));
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyFastForward] = '0'+numkey;
		}
	else if (p.iToken == _L("Rewind"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyRewind] = '0'+numkey;
		}
	else if (p.iToken == _L("PageUp"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPageUp] = '0'+numkey;

		}
	else if (p.iToken == _L("PageDown"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPageDown] = '0'+numkey;
		}
	else if (p.iToken == _L("NextSong"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyNextSong] = '0'+numkey;
		}
	else if( p.iToken == _L("PreviousSong"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPreviousSong] = '0'+numkey;
		}

#if !defined(SERIES80) 
	else if (p.iToken == _L("PauseResume"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPauseResume] = '0'+numkey;
		}
#endif

	else if (p.iToken == _L("Play"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPlay] = '0'+numkey;
		}
	else if (p.iToken == _L("Pause"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPause] = '0'+numkey;
		}
	else if (p.iToken == _L("Stop"))
		{
		if (p.ReadToken(numkey))
			iApp->iSettings.iUserHotkeys[TOggplaySettings::EHotkeyStop] = '0'+numkey;
		}

	return ETrue;
	}

void COggPlayAppView::SetupListBox(COggListBox* aListBox, TSize aSize)
{
  // Set up the layout of the listbox
  CColumnListBoxData* cd = aListBox->GetColumnListBoxData(); 
  TInt w = aSize.iWidth;

  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());
  aListBox->SetFont(iFont);
  aListBox->SetText(GetTextArray());
  aListBox->SetObserver(this);

  CGulIcon* titleIcon = iEikonEnv->CreateIconL(iIconFileName, 0, 1);
  CGulIcon* albumIcon = iEikonEnv->CreateIconL(iIconFileName, 2, 3);
  CGulIcon* artistIcon= iEikonEnv->CreateIconL(iIconFileName, 4, 5);
  CGulIcon* genreIcon = iEikonEnv->CreateIconL(iIconFileName, 6, 7);
  CGulIcon* folderIcon= iEikonEnv->CreateIconL(iIconFileName, 8, 9);
  CGulIcon* fileIcon  = iEikonEnv->CreateIconL(iIconFileName, 10, 11);
  CGulIcon* backIcon  = iEikonEnv->CreateIconL(iIconFileName, 12, 13);
  CGulIcon* playIcon  = iEikonEnv->CreateIconL(iIconFileName, 34, 35);
  CGulIcon* pausedIcon= iEikonEnv->CreateIconL(iIconFileName, 16, 17);
  CArrayPtr<CGulIcon>* icons= cd->IconArray();
  if (!icons)
  {
    icons = new(ELeave) CArrayPtrFlat<CGulIcon>(3);
    cd->SetIconArray(icons);
  }

  icons->Reset();
  icons->AppendL(titleIcon);
  icons->AppendL(albumIcon);
  icons->AppendL(artistIcon);
  icons->AppendL(genreIcon);
  icons->AppendL(folderIcon);
  icons->AppendL(fileIcon);
  icons->AppendL(backIcon);
  icons->AppendL(playIcon);
  icons->AppendL(pausedIcon);

  TInt graphicsWidth = titleIcon->Bitmap()->SizeInPixels().iWidth;
  cd->SetColumnWidthPixelL(0, graphicsWidth);
  cd->SetGraphicsColumnL(0, ETrue);
  cd->SetColumnAlignmentL(0, CGraphicsContext::ECenter);

  cd->SetColumnWidthPixelL(1, w - graphicsWidth);
  cd->SetColumnAlignmentL(1, CGraphicsContext::ELeft);

  cd->SetColumnWidthPixelL(2, w);
  cd->SetColumnAlignmentL(2, CGraphicsContext::ELeft);
}

void COggPlayAppView::SetNextFocus()
{
  COggControl* c;
  c=iFocusControlsIter;
  c->SetFocus(EFalse);
  do {
    iFocusControlsIter++;
    if(!iFocusControlsIter) iFocusControlsIter.SetToFirst();
    c=iFocusControlsIter;
  } while (!c->IsVisible() || c->IsDimmed());

  c->SetFocus(ETrue);
  Update();
}
  
void COggPlayAppView::SetPrevFocus() {
  COggControl* c;
  c=iFocusControlsIter;
  //RDebug::Print(_L("SetNextFocus from 0x%x"),c);
  //__ASSERT_ALWAYS(c,OGGLOG.Write(_L("Assert: SetNextFocus - iterator has no control !?")));
  //__ASSERT_ALWAYS(c->Focus(),OGGLOG.Write(_L("Assert: SetNextFocus - control did not have focus")));
  c->SetFocus(EFalse);
  do {
    iFocusControlsIter--;
    if(!iFocusControlsIter) iFocusControlsIter.SetToLast();
    c=iFocusControlsIter;
  } while (!c->IsVisible() || c->IsDimmed());
  //RDebug::Print(_L("SetNextFocus to 0x%x"),c);
  //__ASSERT_DEBUG(c,OGGPANIC(_L("no control in focus iterator !"),1318));
  c->SetFocus(ETrue);
  Update();
}

void COggPlayAppView::GotoFlipClosed()
	{
	// modify controls for flip-closed mode display
	if (iListBox[1]) // Flip closed view on UIQ doesn't have a list box 
		iListBox[1]->SetCurrentItemIndex(iSelected);
	}

void COggPlayAppView::GotoFlipOpen()
	{
	// modify controls for flip-open mode display
	iListBox[0]->SetCurrentItemIndex(iSelected);
	}

TBool COggPlayAppView::IsFlipOpen()
	{
#if defined(UIQ)
	return (iCoeEnv->ScreenDevice()->CurrentScreenMode() == 0);
#elif defined(SERIES60SUI)
	TRect appRect = iApp->ApplicationRect();
	return appRect.Height()>appRect.Width();
#else
	return 1;
#endif
	}

void COggPlayAppView::HandleNewSkinLayout()
	{
	ChangeLayout(IsFlipOpen() ? 0 : 1);

	// Force a screen update
	Invalidate();
	}

void COggPlayAppView::Activated(COggViewBase* aActivatedView)
	{
	if (!iHead)
		{
		MakeVisible(ETrue);

		iHead = aActivatedView;
		iHead->iNext = NULL;
		}
	else
		{
		COggViewBase* view = iHead;
		while (view)
			{
			if (view == aActivatedView)
				break;

			view = view->iNext;
			}

		if (!view)
			{
			aActivatedView->iNext = iHead;
			iHead = aActivatedView;
			}
		}
	}

void COggPlayAppView::Deactivated(COggViewBase* aDeactivatedView)
	{
	if (iHead == aDeactivatedView)
		{
		iHead = aDeactivatedView->iNext;
		if (!iHead)
			MakeVisible(EFalse);
		}
	else
		{
		COggViewBase* view = iHead;
		while(view)
			{
			COggViewBase* nextView = view->iNext;
			if (nextView == aDeactivatedView)
				{
				view->iNext = aDeactivatedView->iNext;
				break;
				}

			view = nextView;
			}
		}
	}

void COggPlayAppView::ChangeLayout(TInt aMode)
	{
	if (aMode>iModeMax)
		aMode = iModeMax;

	iMode = aMode;
	if (iMode == 1)
		{
		GotoFlipClosed();

		Update();
		iCanvas[1]->DrawControl();

		iCanvas[1]->MakeVisible(ETrue);
		iCanvas[0]->MakeVisible(EFalse);
		}
	else if (iMode == 0)
		{
		GotoFlipOpen();

		Update();
		iCanvas[0]->DrawControl();

		iCanvas[0]->MakeVisible(ETrue);
		if (iCanvas[1])
			iCanvas[1]->MakeVisible(EFalse);
		}

#if defined(UIQ)
	// Is there a way on UIQ to tell it we don't want the title bar in FC mode?
	// As it is we just hard code a full screen client rect.
	TRect clientRect(iMode ? iCoeEnv->ScreenDevice()->SizeInPixels() : iApp->ClientRect());
#else
#if defined(SERIES60V3)
	// Support for E61, E61i and E62 (we remove the status pane if the skin is big enough)
	if ((iApp->iMachineUid == EMachineUid_E61) || (iApp->iMachineUid == EMachineUid_E61i) || (iApp->iMachineUid == EMachineUid_E62))
		{
		// If the skin is 220 pixels high we need to use the extra 20 pixels of the title bar
		// If it's not we assume it's a standard QVGA skin (i.e. iCanvas[0] height is 293, iCanvas[1] height is 200)
		if (iCanvas[0]->Size().iHeight == 220)
			COggS60Utility::RemoveStatusPane();
		else
			COggS60Utility::DisplayStatusPane(R_OGG_PLAY);
		}
#endif

	TRect clientRect(iApp->ClientRect());
#endif

	SetRect(clientRect);
	ActivateL();
	}

void COggPlayAppView::AppToForeground(const TBool aForeground) const
	{
	RWsSession& ws=iEikonEnv->WsSession();
	RWindowGroup& rw = iEikonEnv->RootWin();
	TInt winId = rw.Identifier();
	TApaTask tApatsk(ws);
	tApatsk.SetWgId(winId);
	if (aForeground)
		{
		tApatsk.BringToForeground();
		iApp->ActivateOggViewL();
		}
	else
		tApatsk.SendToBackground();
	}


_LIT(KEmpty, "");
const TInt COggPlayAppView::GetValueFromTextLine(TInt idx)
{
    // Fetch the numeric value from the line (the 2nd "argument")
    CDesCArray* arr= GetTextArray();
    
    __ASSERT_ALWAYS ( arr, 
      User::Panic(_L("COggPlayAppView::GetValue Array Not found"),0 ) );
        
    TPtrC sel,msel;
    if(idx >= 0 && idx < arr->Count()) 
    {
        sel.Set((*arr)[idx]);
        TextUtils::ColumnText(msel, 2, &sel );
    }
    TLex parse(msel);
    TInt previousViewId;
    parse.Val(previousViewId);
    return (previousViewId);
}

TBool COggPlayAppView::HasAFileName(TInt idx)
	{
	// Returns true if a filename is associated with this item
	COggListBox::TItemTypes type = GetItemType(idx);
	if ((type == COggListBox::ETitle) || (type == COggListBox::EFileName) || (type == COggListBox::EPlayList))
		return ETrue;

	return EFalse;
	}

const TDesC& COggPlayAppView::GetFileName(TInt idx)
	{
	return *(GetFile(idx)->iFileName);
	}

TOggFile* COggPlayAppView::GetFile(TInt idx)
	{
    COggListBox::TItemTypes type = GetItemType(idx);

	__ASSERT_ALWAYS(((type == COggListBox::ETitle) || (type == COggListBox::EFileName) || (type == COggListBox::EPlayList)),
    User::Panic(_L("GetFile called with wrong argument"),0 ) );

	TBuf<128> filter;
    GetFilterData(idx, filter);

    TInt fileInt(0);
    TLex parse(filter);
    parse.Val(fileInt);
    return (TOggFile*) fileInt;
	}

const COggPlayAppUi::TViews COggPlayAppView::GetViewName(TInt idx)
	{
	COggListBox::TItemTypes type = GetItemType(idx);
	__ASSERT_ALWAYS(type == COggListBox::EBack, User::Panic(_L("GetViewName called with wrong argument"), 0));

	return (COggPlayAppUi::TViews) GetValueFromTextLine(idx);
	}

void COggPlayAppView::GetFilterData(TInt idx, TDes& aData)
	{
	COggListBox::TItemTypes type = GetItemType(idx);

	__ASSERT_ALWAYS(((type == COggListBox::EAlbum) || (type == COggListBox::EArtist) || (type ==COggListBox::EGenre) || (type == COggListBox::ESubFolder) || (type == COggListBox::EPlayList) || (type == COggListBox::ETitle) || (type == COggListBox::EFileName) ), 
	User::Panic(_L("COggPlayAppView::GetFilterData called with wrong argument"),0 ) );

	CDesCArray* arr = GetTextArray();
	__ASSERT_ALWAYS(arr, User::Panic(_L("COggPlayAppView::GetFilterData Array Not found"), 0));

	TPtrC sel, msel;
	if (idx >= 0 && idx < arr->Count()) 
		{
		sel.Set((*arr)[idx]);
		TextUtils::ColumnText(msel, 2, &sel);
		}

	aData.Copy(msel);
	}

COggListBox::TItemTypes COggPlayAppView::GetItemType(TInt idx)
	{
	CDesCArray* arr= GetTextArray();
	if (!arr) 
		return COggListBox::ETitle;

	TPtrC sel, msel;
	if ((idx >= 0) && (idx < arr->Count()))
		{
		sel.Set((*arr)[idx]);
		TextUtils::ColumnText(msel, 0, &sel);

		TLex parse(msel);
		TInt type(COggListBox::ETitle);
		parse.Val(type);
		return (COggListBox::TItemTypes) type;
		}

	return COggListBox::ETitle;
	}

void COggPlayAppView::SelectItem(TInt idx)
	{
	iSelected = idx;
	if (iListBox[iMode])
		{
		iSelected = iListBox[iMode]->SetCurrentItemIndex(idx);
		iCanvas[iMode]->Refresh();
		}
	}

void COggPlayAppView::SelectFile(TOggFile* aFile)
	{
	TInt found = -1;
	for (TInt i = 0 ; i<GetTextArray()->Count() ; i++)
		{
		if (HasAFileName(i))
			{
			if (GetFile(i) == aFile)
				{
				// A match !
				found = i;
				break;
				}
			}
		}

	if (found != -1)
		{
		// If found, select it.
		iSelected = found;
		if (iListBox[iMode])
			iSelected = iListBox[iMode]->SetCurrentItemIndex(found);
		} // Otherwise, do nothing
	}

TInt COggPlayAppView::GetSelectedIndex()
	{
	return iSelected;
	}

CDesCArray* COggPlayAppView::GetTextArray()
	{
	return iTextArray;
	}

void COggPlayAppView::ListBoxPageDown() 
	{
	TInt index = iListBox[iMode]->CurrentItemIndex();
	if( index != iListBox[iMode]->CountText() - 1)
		SelectItem(index + iListBox[iMode]->NofVisibleLines() - 1);
	}
    
void COggPlayAppView::ListBoxPageUp() 
	{
	TInt index = iListBox[iMode]->CurrentItemIndex();
	if(index != 0)
		SelectItem(index - iListBox[iMode]->NofVisibleLines() + 1);
	}

void COggPlayAppView::UpdateRepeat()
	{
	if (iRepeatIcon[iMode])
		iRepeatIcon[iMode]->SetState(iApp->iSettings.iRepeat);

	if (iRepeatButton[iMode])
		iRepeatButton[iMode]->SetState(iApp->iSettings.iRepeat);
	}

void COggPlayAppView::UpdateRandom()
	{
	if (iApp->iSettings.iRandom)
		{
		if (iRandomIcon[iMode])
			iRandomIcon[iMode]->Show();

		if (iRandomButton[iMode]) iRandomButton[iMode]->SetState(1);
		}
	else
		{
		if (iRandomIcon[iMode])
			iRandomIcon[iMode]->Hide();

		if (iRandomButton[iMode])
			iRandomButton[iMode]->SetState(0);
		}
	}

TInt COggPlayAppView::CallBack(TAny* aPtr)
	{
	COggPlayAppView* self = (COggPlayAppView*) aPtr;
	self->HandleCallBack();

	return 1;
	}

void COggPlayAppView::HandleCallBack()
	{
	if (iAnalyzer[iMode] && (iAnalyzer[iMode]->Style()>0)) 
		{
		if (iApp->iOggPlayback->State() == CAbsPlayback::EPlaying)
			iAnalyzer[iMode]->RenderWaveformFromMDCT(iApp->iOggPlayback->GetFrequencyBins());
		}

	if (iCycleFrequencyCounter % 2)
		{
		if (iPosition[iMode] && (iPosChanged<0)) 
			iPosition[iMode]->SetValue(iApp->iOggPlayback->Position());

		iCanvas[iMode]->CycleLowFrequencyControls();
		}

	iCanvas[iMode]->CycleHighFrequencyControls();
	iCanvas[iMode]->Refresh();

	iCycleFrequencyCounter--;
	if (iCycleFrequencyCounter == 0)
		{
		// 1Hz controls
		iCycleFrequencyCounter = KOggControlFreq;

		// Update the clock
		UpdateClock();

		// Update the song position
		if (iApp->iOggPlayback->State() == CAbsPlayback::EPlaying)
			UpdateSongPosition();
		}
	}

void COggPlayAppView::RestartCallBack()
	{
	if (!iTimer->IsActive())
		iTimer->Start(TTimeIntervalMicroSeconds32(KCallBackPeriod), TTimeIntervalMicroSeconds32(KCallBackPeriod), *iCallBack);
	}

void COggPlayAppView::StopCallBack()
	{
	iTimer->Cancel();
	}

TInt COggPlayAppView::AlarmCallBack(TAny* aPtr)
	{
	COggPlayAppView* self = (COggPlayAppView*) aPtr;
	self->HandleAlarmCallBack();

	return 1;
	}

TInt COggPlayAppView::AlarmErrorCallBack(TAny* aPtr)
	{
	// Our alarm has been cancelled / aborted, probably because the user changed the system time
	COggPlayAppView* self = (COggPlayAppView*) aPtr;
	self->SetAlarm();

	return 1;
	}

void COggPlayAppView::HandleAlarmCallBack()
{
  // Set off an alarm when the alarm time has been reached
  if (iApp->iOggPlayback->State() != CAbsPlayback::EPlaying)
  {
	 // Set the volume and volume boost
	iApp->SetVolume((iApp->iSettings.iAlarmVolume * KMaxVolume)/10);
	iApp->SetVolumeGainL((TGainType) iApp->iSettings.iAlarmGain);

	// Play the next song or unpause the current one
	if (iApp->iOggPlayback->State() == CAbsPlayback::EPaused)
		iApp->PauseResume();
	else
		iApp->NextSong();
  }

  // Reset the alarm
#if !defined(UIQ)
#if defined(SERIES60)
  CAknQueryDialog* snoozeDlg = CAknQueryDialog::NewL();
#else
  CEikDialog* snoozeDlg = new(ELeave) CEikDialog;
#endif

  TInt snoozeCmd = snoozeDlg->ExecuteLD(R_OGGPLAY_SNOOZE_DLG);
  if (snoozeCmd == EOggButtonSnooze)
  {
    // Pause playing
    iApp->Pause();

	// Set the alarm to fire again in five minutes
	SnoozeAlarm();
  }
  else if (snoozeCmd == EOggButtonCancel)
  {
    // Stop playing
    iApp->Stop();

    // Set the alarm to fire again tomorrow 
	SetAlarm();
  }
  else
#endif
  {
    // Set the alarm to fire again tomorrow 
	SetAlarm();
  }
}

void COggPlayAppView::Invalidate()
	{
	iCanvas[iMode]->Invalidate();
	}

void COggPlayAppView::InitView()
	{
	// fill the list box with some initial content:
	if (!iOggFiles->ReadDb(iApp->iDbFileName, iCoeEnv->FsSession()))
		{
		iOggFiles->CreateDb(iCoeEnv->FsSession());
		iOggFiles->WriteDbL(iApp->iDbFileName, iCoeEnv->FsSession());
		}

	const TBuf<16> dummy;
	FillView(COggPlayAppUi::ETop, COggPlayAppUi::ETop, dummy);

	if (iAnalyzer[0]) iAnalyzer[0]->SetStyle(iApp->iAnalyzerState);
	if (iAnalyzer[1]) iAnalyzer[1]->SetStyle(iApp->iAnalyzerState);
	}

void COggPlayAppView::FillView(COggPlayAppUi::TViews theNewView, COggPlayAppUi::TViews thePreviousView, const TDesC& aSelection)
	{
	TBuf<32> buf;
	TBuf<16> dummy;
	TBuf<256> back;
	if (theNewView == COggPlayAppUi::ETop)
		{
		TInt dummy = -1;
		GetTextArray()->Reset();
		iEikonEnv->ReadResource(buf, R_OGG_STRING_6);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::ETitle, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_7);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::EAlbum, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_8);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::EArtist, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_9);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::EGenre, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_10);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::ESubFolder, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_11);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::EFileName, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_12);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::EPlayList, buf, dummy);

		iEikonEnv->ReadResource(buf, R_OGG_STRING_15);
		TOggFiles::AppendLine(*GetTextArray(), COggListBox::EPlayList, buf, dummy);
		}
	else if (thePreviousView == COggPlayAppUi::ETop)
		{
		switch (theNewView)
			{
			case COggPlayAppUi::ETitle:
				iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, dummy);
				break;

			case COggPlayAppUi::EAlbum:
				iOggFiles->FillAlbums(*GetTextArray(), dummy, dummy);
				break;

			case COggPlayAppUi::EArtist:
				iOggFiles->FillArtists(*GetTextArray(), dummy);
				break;

			case COggPlayAppUi::EGenre:
				iOggFiles->FillGenres(*GetTextArray(), dummy, dummy, dummy);
				break;

			case COggPlayAppUi::ESubFolder:
				iOggFiles->FillSubFolders(*GetTextArray());
				break;

			case COggPlayAppUi::EFileName:
				iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, dummy);
				break;

			case COggPlayAppUi::EPlayList:
				iOggFiles->FillPlayLists(*GetTextArray());
				break;

			case COggPlayAppUi::EStream:
				iOggFiles->FillStreams(*GetTextArray());
				break;

			default:
				break;
			}

		back.Num((TInt) COggListBox::EBack);
		back.Append(KColumnListSeparator);

#if defined(UIQ)
		back.Append(_L(".."));
#else
		iEikonEnv->ReadResource(buf, R_OGG_BACK_DIR);
		back.Append(buf);
#endif

		back.Append(KColumnListSeparator);
		back.AppendNum((TInt) thePreviousView);
		GetTextArray()->InsertL(0, back);
		}
	else
		{
		switch (thePreviousView)
			{
			case COggPlayAppUi::ETitle:
				iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, dummy);
				break;

			case COggPlayAppUi::EAlbum:
				iOggFiles->FillTitles(*GetTextArray(), aSelection, dummy, dummy, dummy);
				break;

			case COggPlayAppUi::EArtist:
				iOggFiles->FillTitles(*GetTextArray(), dummy, aSelection, dummy, dummy);
				break;

			case COggPlayAppUi::EGenre:
				iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, aSelection, dummy);
				break;

			case COggPlayAppUi::ESubFolder:
				iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, aSelection);
				break;

			case COggPlayAppUi::EFileName:
				iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, dummy);
				break;

			case COggPlayAppUi::EPlayList:
				iOggFiles->FillPlayList(*GetTextArray(), aSelection);
				break;

			case COggPlayAppUi::EStream:
				iOggFiles->FillPlayList(*GetTextArray(), aSelection);
				break;

			default:
				break;
			}

		back.Num((TInt) COggListBox::EBack);
		back.Append(KColumnListSeparator);

#if defined(UIQ)
		back.Append(_L(".."));
#else
		iEikonEnv->ReadResource(buf, R_OGG_BACK_DIR);
		back.Append(buf);
#endif

		back.Append(KColumnListSeparator);
		back.AppendNum((TInt) thePreviousView);
		GetTextArray()->InsertL(0, back);
		}

	iApp->iViewBy = theNewView;
	UpdateListbox();

	// Choose the selected item
	// If random is turned off:
	//   First of the list, unless it is a "back" icon, in that case 2nd
	//
	// If random is turned on: 
	//   When playing a file, do as if random wasn't turned on
	//   If not playing a file, choose a random file
	if (theNewView == COggPlayAppUi::ETop) 
		SelectItem(0);
	else
		{
		if (iApp->iSettings.iRandom)
			{
			if (iApp->iSongList->AnySongPlaying())
				{
				// When a song has been playing, give full power to the user
				// Do not select a random value.
				SelectItem(1);
				}
			else
				{
				// Select a random value for the first index
				SelectItem(iApp->Rnd(GetTextArray()->Count()-1) + 1);
				}
			}
		else
			SelectItem(1);
		}
		
	UpdateControls();
	}

void COggPlayAppView::Update()
	{
	UpdateAnalyzer();
	UpdateListbox();
	UpdateControls();
	UpdateSongPosition(ETrue);
	UpdateClock(ETrue);
	UpdateVolume();
	UpdateVolumeBoost();
	UpdatePlaying();
	}

void COggPlayAppView::UpdateAnalyzer()
	{
	if (iAnalyzer[iMode])
		{
		iAnalyzer[iMode]->Clear();
		iAnalyzer[iMode]->SetStyle(iApp->iAnalyzerState);
		}
	}

void COggPlayAppView::UpdateListbox()
	{
	if ((iApp->iViewBy != COggPlayAppUi::ETitle) && (iApp->iViewBy != COggPlayAppUi::EFileName))
		{
		if (iListBox[iMode])
			iListBox[iMode]->Redraw();

		return;
		}

	CDesCArray* txt = GetTextArray();
	TBool paused = iApp->iOggPlayback->State() == CAbsPlayback::EPaused;
	const TOggFile* currSong = iApp->iSongList->GetPlayingFile();

	for (TInt i = 0 ; i<txt->Count() ; i++)
		{
		TPtrC sel;
		sel.Set((*txt)[i]);
		TBuf<512> buf(sel);

		COggListBox::TItemTypes type = GetItemType(i);
		if (type != COggListBox::EBack)
			{
			TBuf<1> playState;
			if ((type == COggListBox::ETitle) || (type == COggListBox::EFileName) || (type == COggListBox::EPlayList))
				{
				if (GetFile(i) == currSong)
					{
					// Mark as paused or playing
					if (paused)
						playState.Num((TUint) COggListBox::EPaused);
					else
						playState.Num((TUint) COggListBox::EPlaying);
					}
				else
					playState.Num(0);

				buf.Replace(buf.Length()-1, 1, playState);
				}
			}

		txt->Delete(i);
		txt->InsertL(i, buf);
		}

	if (iListBox[iMode])
		iListBox[iMode]->Redraw();
	}

TBool COggPlayAppView::CanPlay()
	{
	CAbsPlayback::TState playState = iApp->iOggPlayback->State();
	return (playState >= CAbsPlayback::EStreamOpen) && (playState != CAbsPlayback::EPlaying);
	}

TBool COggPlayAppView::CanPause()
	{
	return iApp->iOggPlayback->State() == CAbsPlayback::EPlaying;
	}

TBool COggPlayAppView::CanStop()
	{
	return iApp->iOggPlayback->State() >= CAbsPlayback::EPaused;
	}

TBool COggPlayAppView::PlayDimmed()
	{
	return ((GetSelectedIndex()<0) ||
	(GetItemType(GetSelectedIndex()) == COggListBox::EBack) || 
	(iApp->iViewBy == COggPlayAppUi::EArtist) || (iApp->iViewBy == COggPlayAppUi::EAlbum) || 
	(iApp->iViewBy == COggPlayAppUi::EGenre) || (iApp->iViewBy == COggPlayAppUi::ESubFolder) ||
	(iApp->iViewBy == COggPlayAppUi::ETop)) 
    && !(iApp->iOggPlayback->State() == CAbsPlayback::EPaused);
}

void COggPlayAppView::UpdateControls()
	{
	if (iPlayButton[iMode] && CanPlay()) 
		iPlayButton[iMode]->SetDimmed(PlayDimmed());

	if (iPlayButton2[iMode])
		iPlayButton2[iMode]->SetDimmed(!CanPlay() || PlayDimmed());

	if (iPauseButton2[iMode])
		iPauseButton2[iMode]->SetDimmed(!CanPause());

	if (iPlayButton[iMode])
		iPlayButton[iMode]->MakeVisible(CanPlay());

	if (iPauseButton[iMode])
		iPauseButton[iMode]->MakeVisible(!CanPlay());
    
	if (iStopButton[iMode])
		iStopButton[iMode]->SetDimmed(!CanStop());

	if (iPosition[iMode])
		iPosition[iMode]->SetDimmed(!CanStop());
 
	if (iRepeatButton[iMode])
		iRepeatButton[iMode]->SetState(iApp->iSettings.iRepeat);

	if (iRandomButton[iMode])
		iRandomButton[iMode]->SetState(iApp->iSettings.iRandom);

	if (iRepeatIcon[iMode])
		iRepeatIcon[iMode]->MakeVisible(iApp->iSettings.iRepeat);

	if (iRandomIcon[iMode])
		iRandomIcon[iMode]->MakeVisible(iApp->iSettings.iRandom);
	}

_LIT(KPositionFormat, "%02d:%02d");
_LIT(KDurationFormat, "%02d:%02d");
_LIT(KPosAndDurationFormat, "%S / %S");
void COggPlayAppView::UpdateSongPosition(TBool aUpdateDuration)
	{
#if defined(SERIES60) || defined(SERIES80)
	// Only show "Played" time component when not stopped, i.e. only show when artist, title etc is displayed
	if (iPlayed[iMode])
		{
		TBool playedControlIsVisible = (iApp->iOggPlayback->State() > CAbsPlayback::EStopped);
		iPlayed[iMode]->MakeVisible(playedControlIsVisible);

		if(!playedControlIsVisible)
			return;
		}
#endif

	// Update the song position displayed in the menubar (flip open) or in the TOggCanvas (flip closed)
#if defined(SERIES60V3)
	TInt sec = iApp->iOggPlayback->Position()/1000;
#else
	TInt sec = (iApp->iOggPlayback->Position()/TInt64(1000)).GetTInt();
#endif

	TInt min = sec/60;
	sec -= min*60;

	TBuf<32> positionBuf;
	positionBuf.Format(KPositionFormat, min, sec);

	if (aUpdateDuration)
		iTimeTot = iApp->iOggPlayback->Time();

#if defined(SERIES60V3)
	TInt secTot = iTimeTot/1000;
#else
	TInt secTot = (iTimeTot/TInt64(1000)).GetTInt();
#endif

	TInt minTot = secTot/60;
	secTot -= minTot*60;

	TBuf<32> durationBuf;
	durationBuf.Format(KDurationFormat, minTot, secTot);

	TBuf<32> posAndDurationBuf;
	posAndDurationBuf.Format(KPosAndDurationFormat, &positionBuf, &durationBuf);

#if defined(UIQ)
	if (IsFlipOpen())
		{
		CEikMenuBar* mb = iEikonEnv->AppUiFactory()->MenuBar();
		if (!mb)
			return;

		// As we cannot officially get the TitleArray, we have to steal it (hack)
		CEikMenuBar::CTitleArray *ta;
		ta = *(CEikMenuBar::CTitleArray**)((char*)mb + sizeof(*mb) - 4*sizeof(void*));
		ta->At(1)->iData.iText.Copy(posAndDurationBuf);
		mb->SetSize(mb->Size());
		mb->DrawNow();
		}
	else
		{
		if (iPlayed[iMode])
			iPlayed[iMode]->SetText(posAndDurationBuf);

		if (iPlayedDigits[iMode])
			iPlayedDigits[iMode]->SetText(positionBuf);

		if (iTotalDigits[iMode] && aUpdateDuration)
			iTotalDigits[iMode]->SetText(durationBuf);
		}
#else
	if (iPlayed[iMode])
		iPlayed[iMode]->SetText(posAndDurationBuf);

	if (iPlayedDigits[iMode])
		iPlayedDigits[iMode]->SetText(positionBuf);

	if (iTotalDigits[iMode] && aUpdateDuration)
		iTotalDigits[iMode]->SetText(durationBuf);
#endif

	if (iPosition[iMode] && aUpdateDuration)
		iPosition[iMode]->SetMaxValue(iTimeTot);
	}

void COggPlayAppView::UpdateClock(TBool forceUpdate)
	{
	if (!iClock[iMode])
		return;

	// Update the clock
	TTime now;
	now.HomeTime();
	TDateTime dtNow(now.DateTime());
	TInt clockMinute = dtNow.Minute();
	TBool clockUpdateRequired = clockMinute != iCurrentClockMinute;
	if (!clockUpdateRequired && !forceUpdate)
		return;

	TBuf<32> buf;
	now.FormatL(buf, KDateString);
	iClock[iMode]->SetText(buf);
	iCurrentClockMinute = clockMinute;

	if (!forceUpdate || !iAlarm[iMode])
		return;

	// Update alarm
	iApp->iSettings.iAlarmTime.FormatL(buf, KDateString);
	iAlarm[iMode]->SetText(buf);
	}

void COggPlayAppView::UpdateVolume()
	{
	if (iVolume[iMode])
		iVolume[iMode]->SetValue(iApp->iVolume);
	}

void COggPlayAppView::UpdateVolumeBoost()
	{
	if (iVolumeBoost[iMode])
		iVolumeBoost[iMode]->SetValue(iApp->iSettings.iGainType);
	}

void COggPlayAppView::UpdatePlaying()
	{
	if (iTitle[iMode])
		{
		if (iApp->iOggPlayback->Title().Length()==0)
			{
#if defined(UIQ)
			iTitle[iMode]->SetText(iApp->iOggPlayback->FileName());
#else
			// Series 60 : Show only the filename. (space consideration)
			TParsePtrC p(iApp->iOggPlayback->FileName());
			iTitle[iMode]->SetText(p.NameAndExt());
#endif
			}
		else if(!iArtist[iMode])
			{
			_LIT(KSeparator," - ");
			TBuf<2*KMaxCommentLength + 3> text;
			text = iApp->iOggPlayback->Artist();
			text.Append(KSeparator);
			text.Append(iApp->iOggPlayback->Title());
			iTitle[iMode]->SetText(text);
			}
		else
			iTitle[iMode]->SetText(iApp->iOggPlayback->Title());
		}

	if (iAlbum[iMode])
		iAlbum[iMode]->SetText(iApp->iOggPlayback->Album());

	if (iArtist[iMode])
		iArtist[iMode]->SetText(iApp->iOggPlayback->Artist());

	if (iGenre[iMode])
		iGenre[iMode]->SetText(iApp->iOggPlayback->Genre());

	if (iTrackNumber[iMode])
		iTrackNumber[iMode]->SetText(iApp->iOggPlayback->TrackNumber());

	if (iPlaying[iMode]) 
		iPlaying[iMode]->MakeVisible(iApp->iOggPlayback->State()==CAbsPlayback::EPlaying);

	if (iPaused[iMode])
		{
		if (iApp->iOggPlayback->State()==CAbsPlayback::EPaused)
			iPaused[iMode]->Blink();
		else
			iPaused[iMode]->Hide();
		}

	if (iLogo[iMode])
		iLogo[iMode]->MakeVisible(iApp->iOggPlayback->FileName().Length()==0);
	}

void COggPlayAppView::SetAlarm()
{
  TTime now;
  now.HomeTime();

  // Sync the alarm time to the current day
  iApp->iSettings.iAlarmTime += now.DaysFrom(iApp->iSettings.iAlarmTime);
  if (now>=iApp->iSettings.iAlarmTime)
	iApp->iSettings.iAlarmTime += TTimeIntervalDays(1);

  if (iApp->iSettings.iAlarmActive)
  {
    TBuf<128> buft;
    iApp->iSettings.iAlarmTime.FormatL(buft, KDateString);
    if (iAlarm[iMode]) iAlarm[iMode]->SetText(buft);
    if (iAlarmIcon[iMode]) iAlarmIcon[iMode]->Show();

	iAlarmTimer->At(iApp->iSettings.iAlarmTime);
  } 
  else
  {
    ClearAlarm();
	iAlarmTimer->Cancel();
  }
}

void COggPlayAppView::SnoozeAlarm()
{
  TTime now;
  now.HomeTime();

  // Add the snooze time to the current time
  iApp->iSettings.iAlarmTime = now;
  iApp->iSettings.iAlarmTime += TTimeIntervalMinutes(KSnoozeTime[iApp->iSettings.iAlarmSnooze]);

  if (iApp->iSettings.iAlarmActive)
  {
    TBuf<256> buft;
    iApp->iSettings.iAlarmTime.FormatL(buft, KDateString);
    if (iAlarm[iMode]) iAlarm[iMode]->SetText(buft);
    if (iAlarm[iMode]) iAlarm[iMode]->MakeVisible(ETrue);
    if (iAlarmIcon[iMode]) iAlarmIcon[iMode]->Show();

	iAlarmTimer->At(iApp->iSettings.iAlarmTime);
  } 
  else
  {
    ClearAlarm();
	iAlarmTimer->Cancel();
  }
}
void COggPlayAppView::ClearAlarm()
	{
	if (iAlarmIcon[iMode])
		iAlarmIcon[iMode]->Hide();
	}

TInt COggPlayAppView::CountComponentControls() const
	{
	return iCanvas[1] ? 2 : 1;
	}

CCoeControl* COggPlayAppView::ComponentControl(TInt aIndex) const
	{
	return iCanvas[aIndex];
	}

TKeyResponse COggPlayAppView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
	{
#if defined(SERIES60) || defined(SERIES80)
	enum EOggKeys
		{
#if defined(SERIES60)
		EOggConfirm = EKeyDevice3
#elif defined(SERIES90)
		EOggConfirm = EKeyDevice7
#else // SERIES80
		EOggConfirm = EKeyDeviceA
#endif
		};

	enum EOggScancodes
		{
		EOggUp = EStdKeyUpArrow,
		EOggDown = EStdKeyDownArrow,
		EOggRight = EStdKeyRightArrow,
		EOggLeft = EStdKeyLeftArrow
		};

	TInt code = aKeyEvent.iCode;
	TInt scanCode = aKeyEvent.iScanCode;
	TInt index = iListBox[iMode]->CurrentItemIndex();
	CAbsPlayback::TState playbackState = iApp->iOggPlayback->State();

#if defined(SERIES80) && !defined(SERIES90)
	if ((aType == EEventKeyDown) || (aType == EEventKey))
		{
		// Conversions needed for S80 joystick
		switch (scanCode)
			{
			case EStdKeyDevice6:
				scanCode = EOggLeft;
				break;

			case EStdKeyDevice7:
				scanCode = EOggRight;
				break;

			case EStdKeyDevice8:
				scanCode = EOggUp;
				break;

			case EStdKeyDevice9:
				scanCode = EOggDown;
				break;
			}

		// It seems that on S80 a joystick "click" doesn't actually generate a key event
		// of EKeyDeviceA. Consequently I've changed this back to the way it
		// used to work: fake a key event when we get "key down EStdKeyDeviceA". 
		if ((scanCode == EStdKeyDeviceA) && (aType == EEventKeyDown))
			code = EOggConfirm;

		// Add enter key for S80 (request from Chris)
		if (code == EKeyEnter)
			code = EOggConfirm;
		}
#endif

	COggControl* c = iFocusControlsIter;
	if (aType == EEventKeyDown)
		{
		if (iFocusControlsPresent && (scanCode == EOggLeft))
			{
			SetPrevFocus();
			return EKeyWasConsumed;
			}
		else if (iFocusControlsPresent && (scanCode == EOggRight))
			{
			SetNextFocus();
			return EKeyWasConsumed;
			}
		else
			{
			if ((c == iVolume[iMode] && (scanCode==EOggUp || scanCode==EOggDown))
			|| (!iFocusControlsPresent && (scanCode==EOggLeft || scanCode==EOggRight)))
				{
				if (playbackState != CAbsPlayback::EPlaying)
					{
					// not playing, i.e. stopped or paused
					if ((scanCode == EOggDown) || (scanCode == EOggLeft))
						iApp->SelectPreviousView();
					else if ((scanCode == EOggDown) || (scanCode == EOggRight))
						{
						if (HasAFileName(index))
							iApp->HandleCommandL(EOggPlay);
						else
							iApp->SelectNextView();
						}
					}

				return EKeyWasConsumed;
				}
			}
		}

	if (code == EOggConfirm)
		{
		if (aKeyEvent.iRepeats)
			return EKeyWasConsumed;

		if(!iFocusControlsPresent)
			{
			if ((playbackState == CAbsPlayback::EPlaying) || (playbackState == CAbsPlayback::EPaused))
				{
				if (iApp->iSongList->IsSelectedFromListBoxCurrentlyPlaying()
				&& ((iApp->iViewBy == COggPlayAppUi::ETitle) || (iApp->iViewBy == COggPlayAppUi::EFileName)))
					{
					// Event on the current active file
					iApp->HandleCommandL(EOggPauseResume);
					}
				else
					iApp->HandleCommandL(EOggPlay);
				}
			else
				{
				// Event without any active files
				iApp->HandleCommandL(EOggPlay);
				}
			}
		else if (c == iPlayButton[iMode])
			{
			// manually move focus to pause button
			c->SetFocus(EFalse);

			do
				{
				iFocusControlsIter++;
				if (!iFocusControlsIter)
					iFocusControlsIter.SetToFirst();

				c = iFocusControlsIter;
				} while (c != iPauseButton[iMode]);

			c->SetFocus(ETrue);
			iApp->HandleCommandL(EOggPauseResume);
			}
		else if (c == iPauseButton[iMode])
			{
			c->SetFocus(EFalse);

			do
				{
				iFocusControlsIter++;
				if (!iFocusControlsIter)
					iFocusControlsIter.SetToFirst();

				c = iFocusControlsIter;
				} while (c != iPlayButton[iMode]);

			c->SetFocus(ETrue);
			iApp->HandleCommandL(EOggPauseResume);
			}
		else if (c == iStopButton[iMode])
			iApp->HandleCommandL(EOggStop);
		else if ((playbackState == CAbsPlayback::EPlaying) || (playbackState == CAbsPlayback::EPaused)) 
			iApp->HandleCommandL(EOggStop);
		else
			iApp->HandleCommandL(EOggPlay);

		return EKeyWasConsumed;
		}

#if defined(SERIES60)
	// S60 User Hotkeys
	switch (COggUserHotkeysControl::Hotkey(aKeyEvent, aType, &iApp->iSettings))
		{
		case TOggplaySettings::EHotkeyFastForward:
			OggSeek(KFfRwdStep);
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyRewind:
			OggSeek(-KFfRwdStep);
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyPageDown:
			ListBoxPageDown();
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyPageUp:
			ListBoxPageUp();
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyNextSong:
			iApp->NextSong();
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyPreviousSong:
			iApp->PreviousSong();
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyPauseResume:
			iApp->PauseResume();
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyPlay:
			iApp->HandleCommandL(EOggPlay);
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyPause:
			iApp->HandleCommandL(EUserPauseCBA);
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyStop:
			iApp->HandleCommandL(EOggStop);		
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyKeylock:
			if (aKeyEvent.iRepeats > 0)
				{
				RAknKeyLock keyLock;
				TInt err = keyLock.Connect();
				if (err == KErrNone)
					{
					keyLock.EnableKeyLock();
					keyLock.Close();
					}
				}

			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyVolumeBoostUp:
			{
			TGainType currentGain = (TGainType) iApp->iSettings.iGainType;
			TInt newGain = currentGain + 1;
			if (newGain<=EStatic12dB)
				{
				iApp->SetVolumeGainL((TGainType) newGain);
				UpdateVolumeBoost();
				}

			return EKeyWasConsumed;
			}

		case TOggplaySettings::EHotkeyVolumeBoostDown:
			{
			TGainType currentGain = (TGainType) iApp->iSettings.iGainType;
			TInt newGain = currentGain - 1;
			if (newGain>=EMinus24dB)
				{
				iApp->SetVolumeGainL((TGainType) newGain);
				UpdateVolumeBoost();
				}

			return EKeyWasConsumed;
			}

		case TOggplaySettings::EHotkeyToggleShuffle:
			iApp->ToggleRandomL();
			return EKeyWasConsumed;

		case TOggplaySettings::EHotkeyToggleRepeat:
			iApp->ToggleRepeat();
			return EKeyWasConsumed;

		default:
			break;
		}
#endif

	if (aType == EEventKey)
		{
		if ((c == iListBox[iMode]) || !iFocusControlsPresent)
			{
			if (scanCode == EOggDown)
				{
				SelectItem(index+1);
				return EKeyWasConsumed;
				}
			else if (scanCode == EOggUp)
				{
				SelectItem(index-1);
				return EKeyWasConsumed;
				}
			}

		if ((c == iVolume[iMode] && ((scanCode == EOggUp) || (scanCode == EOggDown)))
		|| (!iFocusControlsPresent && ((scanCode == EOggLeft) || (scanCode == EOggRight))))
			{
			if (playbackState == CAbsPlayback::EPlaying)
				{
				TInt volume = iApp->iVolume;
				if ((scanCode == EOggUp) || (scanCode == EOggRight))
					volume += KStepVolume;
				else if ((scanCode == EOggDown) || (scanCode == EOggLeft))
					volume -= KStepVolume;

				if (volume>KMaxVolume)
					volume = KMaxVolume;

				if (volume<0)
					volume = 0;

				iApp->SetVolume(volume);
				UpdateVolume();
				}
			}
		}

	return EKeyWasNotConsumed;
#else
	// UIQ keyhandling
	enum EOggKeys
		{
		EOggConfirm = EQuartzKeyConfirm,
		};

	enum EOggScancodes
		{
		EOggUp = EStdQuartzKeyTwoWayUp,
		EOggDown = EStdQuartzKeyTwoWayDown
		};

	TInt code = aKeyEvent.iCode;
	TInt modifiers = aKeyEvent.iModifiers & EAllStdModifiers;
	TInt iHotkey = ((COggPlayAppUi *) iEikonEnv->EikAppUi())->iHotkey;
	CAbsPlayback::TState playbackState = iApp->iOggPlayback->State();

	if (iHotkey && (aType == EEventKey) &&
	(code == KHotkeyKeycodes[iHotkey-1].code) && (modifiers == KHotkeyKeycodes[iHotkey-1].mask))
		{
		iApp->ActivateOggViewL();
		return EKeyWasConsumed;
		}

	if (code == EOggConfirm)
		{
		if (IsFlipOpen())
			{
			if ((playbackState == CAbsPlayback::EPlaying) || (playbackState == CAbsPlayback::EPaused))
				iApp->HandleCommandL(EOggStop);
			else
				iApp->HandleCommandL(EOggPlay);
			}
		else
			{
			if ((playbackState == CAbsPlayback::EPlaying) || (playbackState == CAbsPlayback::EPaused))
				iApp->HandleCommandL(EOggPauseResume);
			else
				iApp->HandleCommandL(EOggPlay);
			}

		return EKeyWasConsumed;
		}

	if (code>0)
		{
		if (aKeyEvent.iScanCode == EStdKeyDeviceD)
			{
			iApp->NextSong();
			return EKeyWasConsumed;
			} // jog dial up
		else if (aKeyEvent.iScanCode == EStdKeyDeviceE)
			{
			iApp->PreviousSong();
			return EKeyWasConsumed;
			}  // jog dial down
		else if (aKeyEvent.iScanCode==167)
			{
			AppToForeground(EFalse);
			return EKeyWasConsumed;
			} // back key <-
		else if (aKeyEvent.iScanCode==174)
			{
			iApp->HandleCommandL(EOggStop);
			return EKeyWasConsumed;
			} // "C" key
		else if (aKeyEvent.iScanCode==173)
			{ 
			iApp->LaunchPopupMenuL(R_POPUP_MENU,TPoint(100,100),EPopupTargetTopLeft);
			return EKeyWasConsumed; 
			}
		else if (aKeyEvent.iScanCode==133)
			{ // "*"
			if (iTitle[iMode])
				iTitle[iMode]->ScrollNow();

			if (iAlbum[iMode])
				iAlbum[iMode]->ScrollNow();

			if (iArtist[iMode])
				iArtist[iMode]->ScrollNow();

			if (iGenre[iMode])
				iGenre[iMode]->ScrollNow();

			return EKeyWasConsumed;
			}
		else if (aKeyEvent.iScanCode==127) 
			{ // "#"
			iApp->ToggleRepeat();
			return EKeyWasConsumed;
			}

		// 48..57 == "0"..."9"
		// 172 jog dial press
		// 179    == OK
		}

	if ((playbackState == CAbsPlayback::EPlaying) || !IsFlipOpen())
		{
		if (aType == EEventKeyDown)
			{
			// Volume
			if ((aKeyEvent.iScanCode == EOggUp) || (aKeyEvent.iScanCode == EOggDown))
				{
				TInt volume = iApp->iVolume;
				if ((aKeyEvent.iScanCode == EOggUp) && (volume<100))
					volume += 5;

				if ((aKeyEvent.iScanCode == EOggDown) && (volume>0))
					volume -= 5;

				iApp->SetVolume(volume);
				UpdateVolume();

				return EKeyWasConsumed;
				}
			}
		}
	else if (iListBox[iMode])
		{
		// Move current index in the list box
		if (aType == EEventKeyDown)
			{
			TInt idx = iListBox[iMode]->CurrentItemIndex();
			if ((aKeyEvent.iScanCode == EOggUp) && (idx>0))
				{
				SelectItem(idx-1);
				return EKeyWasConsumed;
				}

			if (aKeyEvent.iScanCode == EOggDown)
				{
				SelectItem(idx+1);
				return EKeyWasConsumed;
				}
			}
		}

	return EKeyWasNotConsumed;
#endif  // UIQ keyhandling
	}

void COggPlayAppView::OggSeek(TInt aSeekTime)
	{
	CAbsPlayback* playback = iApp->iOggPlayback;
	if (!playback->Seekable())
		return;

	TInt64 pos = playback->Position() + aSeekTime;
	playback->SetPosition(pos);
	iPosition[iMode]->SetValue(pos);

	UpdateSongPosition();
	iCanvas[iMode]->Refresh();
	}

void COggPlayAppView::OggControlEvent(COggControl* c, TInt aEventType, TInt /*aValue*/)
	{
	if (c == iListBox[iMode])
		{
		if (aEventType == 1)
			{
			iSelected = iListBox[iMode]->CurrentItemIndex();
			UpdateControls(); // the current item index changed
			}

		if (aEventType == 2)
			iApp->HandleCommandL(EOggPlay); // an item was selected (by tipping on it)
		}

	if (c == iPlayButton[iMode])
		{
		if (iApp->iOggPlayback->State() == CAbsPlayback::EPaused)
			iApp->HandleCommandL(EOggPauseResume);
		else
			iApp->HandleCommandL(EOggPlay);
		}

	if (c == iPauseButton[iMode])
		iApp->HandleCommandL(EOggPauseResume);

	if (c == iStopButton[iMode])
		iApp->HandleCommandL(EOggStop);

	if (c == iNextSongButton[iMode])
		iApp->NextSong();

	if (c == iPrevSongButton[iMode])
		iApp->PreviousSong();

	if (c == iRepeatButton[iMode])
		iApp->ToggleRepeat();

	if (c == iRandomButton[iMode])
		iApp->ToggleRandomL();
	}

void COggPlayAppView::OggPointerEvent(COggControl* c, const TPointerEvent& p)
	{
	if (c == iPosition[iMode])
		{
		if ((p.iType == TPointerEvent::EButton1Down) || (p.iType == TPointerEvent::EDrag))
			iPosChanged = iPosition[iMode]->CurrentValue();

		if (p.iType == TPointerEvent::EButton1Up)
			{
			iApp->iOggPlayback->SetPosition(iPosChanged);
			iPosChanged= -1;
			}
		}

	if (c == iVolume[iMode])
		iApp->SetVolume(iVolume[iMode]->CurrentValue());

	if (c == iAnalyzer[iMode])
		iApp->iAnalyzerState= iAnalyzer[iMode]->Style();
	}

void COggPlayAppView::AddControlToFocusList(COggControl* c)
	{
	COggControl* currentitem = iFocusControlsIter; 
	if (currentitem)
		{
		iFocusControlsPresent = ETrue;
		c->iDlink.Enque(&currentitem->iDlink);
		iFocusControlsIter.Set(*c);
		}
	else
		{
		iFocusControlsHeader.AddFirst(*c);
		iFocusControlsIter.SetToFirst(); 
		}
	}
