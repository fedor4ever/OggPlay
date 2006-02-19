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

#include "OggPlayAppView.h"
#include "OggOs.h"
#include "OggLog.h"
#include "OggHelperFcts.h"

#include <eikclb.h>     // CEikColumnListbox
#include <eikclbd.h>    // CEikColumnListBoxData
#include <eiktxlbm.h>   // CEikTextListBox(Model)
#include <eikmenub.h>   // CEikMenuBar
#include <apgtask.h>    // TApaTask
#ifdef SERIES60
#include <aknkeys.h>
#include <aknkeylock.h>
#include <aknquerydialog.h>
#endif
#ifdef SERIES80
#include <eikenv.h>
#endif
#ifdef UIQ
#include <quartzkeys.h>	// EStdQuartzKeyConfirm etc.
#endif
#include <stdlib.h>
#include "OggAbsPlayback.h"

// Time (usecs) between canvas Refresh() for graphics updating
const TInt KCallBackPeriod = 71429; // 14Hz
const TInt KOggControlFreq = 14;

// Literal for clock update
_LIT(KDateString, "%-B%:0%J%:1%T%:3%+B");

// Array of times for snooze
const TInt KSnoozeTime[4] = { 5, 10, 20, 30 };

COggPlayAppView::COggPlayAppView()
: iFocusControlsPresent(EFalse),
iFocusControlsHeader(_FOFF(COggControl,iDlink)),
iFocusControlsIter(iFocusControlsHeader),
iCycleFrequencyCounter(KOggControlFreq)
{
  iControls= 0;
  iOggFiles= 0;
  iActivateCount= 0;
  iPosChanged= -1;
  iApp= 0;
  ResetControls();
}

COggPlayAppView::~COggPlayAppView()
{
  //if (iControls) { delete iControls; iControls= 0; }
  if (iControls) { iControls->Reset(); delete iControls; iControls= 0; }
  if (iTextArray) { delete iTextArray; iTextArray=0; }

  ClearCanvases();

  if(iTimer) iTimer->Cancel();
  delete iTimer;
  delete iCallBack;

  if (iAlarmTimer) iAlarmTimer->Cancel();
  delete iAlarmTimer;
  delete iAlarmCallBack;
  delete iAlarmErrorCallBack;

  if(iCanvas[1]) {
    delete iCanvas[1];
  }
  if(iCanvas[0]) {
    delete iCanvas[0];
  }

  delete iOggFiles;
}

void
COggPlayAppView::ConstructL(COggPlayAppUi *aApp, const TRect& aRect)
{
  iTextArray= new(ELeave) CDesCArrayFlat(10);

  iApp = aApp;
  CreateWindowL();
  SetRect(aRect);

  SetComponentsToInheritVisibility(EFalse);
  iOggFiles= new TOggFiles(iApp->iOggPlayback);
 
  iControls = new(ELeave)CArrayPtrFlat<CCoeControl>(10);

  iIconFileName.Copy(iApp->Application()->AppFullName());
  iIconFileName.SetLength(iIconFileName.Length() - 3);
  iIconFileName.Append(_L("mbm"));

  // construct the two canvases for flip-open and closed mode:
  //----------------------------------------------------------

  // flip-open mode:
  iCanvas[0]= new(ELeave) COggCanvas;
  iControls->AppendL(iCanvas[0]);
  iCanvas[0]->SetContainerWindowL(*this);
  iCanvas[0]->SetControlContext(this);
  iCanvas[0]->SetObserver(this);
#if defined(SERIES80)
  TPoint pBM(0,0);
  iCanvas[0]->SetExtent(TPoint(0, 0), TSize(640, 200));
#elif defined(SERIES60)
  TPoint pBM(Position());
  pBM.iY= 0;

  if (CCoeEnv::Static()->ScreenDevice()->SizeInPixels() == TSize(352, 416))
	iCanvas[0]->SetExtent(pBM, TSize(352, 376));
  else
	iCanvas[0]->SetExtent(pBM, TSize(176, 188));  
#else
  TPoint pBM(Position());
  pBM.iY= 0;

  iCanvas[0]->SetExtent(pBM, TSize(208, 320));
#endif

  // flip-closed mode:
  iCanvas[1]= new(ELeave) COggCanvas;
  iControls->AppendL(iCanvas[1]);
  iCanvas[1]->SetContainerWindowL(*this);
  iCanvas[1]->SetControlContext(this);
  iCanvas[1]->SetObserver(this);
  pBM.iY= 0;
  iCanvas[1]->SetExtent(pBM, TSize(208,144));

  // set up all the controls:
  
  //iCanvas[0]->ConstructL(iIconFileName, 19);
  //iCanvas[1]->ConstructL(iIconFileName, 18);
  //CreateDefaultSkin();

  iCanvas[0]->SetFocus(ETrue);

  // Start the canvas refresh timer (KCallBackPeriod):
  iTimer = CPeriodic::NewL(CActive::EPriorityStandard);
  iCallBack = new(ELeave) TCallBack(COggPlayAppView::CallBack, this);

  iAlarmCallBack = new(ELeave) TCallBack(COggPlayAppView::AlarmCallBack, this);
  iAlarmErrorCallBack = new(ELeave) TCallBack(COggPlayAppView::AlarmErrorCallBack, this);
  iAlarmTimer = new(ELeave) COggTimer(*iAlarmCallBack, *iAlarmErrorCallBack);

  // Only start timer once all initialization has been done
  iTimer->Start(TTimeIntervalMicroSeconds32(1000000), TTimeIntervalMicroSeconds32(KCallBackPeriod), *iCallBack);

  // Initialise alarm
  SetAlarm();

  // Activate the view
  ActivateL();
}

void 
COggPlayAppView::ReadSkin(const TFileName& aFileName)
{
  ClearCanvases();
  ResetControls();

  iIconFileName= aFileName.Left(aFileName.Length()-4);
  iIconFileName.Append(_L(".mbm"));

#if defined(SERIES60)
  TInt scaleFactor = iCanvas[0]->LoadBackgroundBitmapL(iIconFileName, 19);
  TOggParser p(aFileName, scaleFactor);
#else
  iCanvas[0]->LoadBackgroundBitmapL(iIconFileName, 19);
  iCanvas[1]->LoadBackgroundBitmapL(iIconFileName, 18);
  TOggParser p(aFileName);
#endif

  if (p.iState!=TOggParser::ESuccess) {
    p.ReportError();
    User::Leave(-1);
    return;
  }
  if (!p.ReadHeader()) {
    p.ReportError();
    User::Leave(-1);
    return;
  }
  p.ReadToken();
  if (p.iToken!=_L("FlipOpen")) {
    p.iState= TOggParser::EFlipOpenExpected;
    p.ReportError();
    User::Leave(-1);
    return;
  }
  p.ReadToken();
  if (p.iToken!=KBeginToken) {
    p.iState= TOggParser::EBeginExpected;
    p.ReportError();
    User::Leave(-1);
    return;
  }
  ReadCanvas(0,p);
  if (p.iState!=TOggParser::ESuccess) {
    p.ReportError();
    User::Leave(-1);
    return;
  }

  p.ReadToken();
  if (p.iToken==_L("FlipClosed")) {
    p.ReadToken();
    if (p.iToken!=KBeginToken) {
      p.iState= TOggParser::EBeginExpected;
      p.ReportError();
      User::Leave(-1);
      return;
    }
    ReadCanvas(1,p);
    if (p.iState!=TOggParser::ESuccess) {
      p.ReportError();
      User::Leave(-1);
      return;
    }
  }
  iFocusControlsIter.SetToFirst();
  COggControl* c=iFocusControlsIter;
  if(c) c->SetFocus(ETrue);
}

void 
COggPlayAppView::ResetControls() {
  iClock[0]= 0; iClock[1]= 0;
  iAlarm[0]= 0; iAlarm[1]= 0;
  iAlarmIcon[0]=0; iAlarmIcon[1]= 0;
  iPlayed[0]= 0; iPlayed[1]= 0;
  iPlayedDigits[0]= 0; iPlayedDigits[1]= 0;
  iTotalDigits[0]= 0; iTotalDigits[1]= 0;
  iPaused[0]= 0; iPaused[1]= 0;
  iPlaying[0]= 0; iPlaying[1]= 0;
  iPlayButton[0]= 0; iPlayButton[1]=0;
  iPauseButton[0]= 0; iPauseButton[1]= 0;
  iPlayButton2[0]= 0; iPlayButton2[1]=0;
  iPauseButton2[0]= 0; iPauseButton2[1]= 0;
  iStopButton[0]=0; iStopButton[1]= 0;
  iNextSongButton[0]= 0; iNextSongButton[1]= 0;
  iPrevSongButton[0]= 0; iPrevSongButton[1]= 0;
  iListBox[0]=0; iListBox[1]= 0;
  iTitle[0]=0; iTitle[1]= 0;
  iAlbum[0]=0; iAlbum[1]= 0;
  iArtist[0]=0; iArtist[1]= 0;
  iGenre[0]=0; iGenre[1]= 0;
  iTrackNumber[0]=0; iTrackNumber[1]= 0;
  iFileName[0]=0; iFileName[1]= 0;
  iPosition[0]= 0; iPosition[1]= 0;
  iVolume[0]= 0; iVolume[1]= 0;
  iAnalyzer[0]= 0; iAnalyzer[1]= 0;
  iRepeatIcon[0]= 0; iRepeatIcon[1]= 0;
  iRepeatButton[0]= 0; iRepeatButton[1]= 0;
  iRandomIcon[0]=0; iRandomIcon[1]=0;
  iScrollBar[0]= 0; iScrollBar[1]= 0;
  iAnimation[0]= 0; iAnimation[1]= 0;
  iLogo[0]= 0; iLogo[1]= 0;
}

void
COggPlayAppView::ClearCanvases()
  {
  COggControl* oggControl;
  iFocusControlsIter.SetToFirst();
  while ((oggControl = iFocusControlsIter++) != NULL) {
    oggControl->iDlink.Deque();
    };

  iCanvas[0]->ClearControls();
  iCanvas[1]->ClearControls();
  }


void
COggPlayAppView::ReadCanvas(TInt aCanvas, TOggParser& p)
{
  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());

  iFocusControlsPresent=EFalse;
  bool stop(EFalse);

  while (!stop) {
    stop= !p.ReadToken() || p.iToken==KEndToken;

    COggControl* c=NULL;

    if (p.iToken==_L("Clock")) { 
      _LIT(KAL,"Adding Clock");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggText();
      iClock[aCanvas]= (COggText*)c; 
      iClock[aCanvas]->SetFont(iFont); 
    }
    else if (p.iToken==_L("Alarm")) { 
      _LIT(KAL,"Adding Alarm");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggText();
      iAlarm[aCanvas]= (COggText*)c;
      iAlarm[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Title")) { 
      _LIT(KAL,"Adding Title");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggText();
      iTitle[aCanvas]= (COggText*)c;
      iTitle[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Album")) { 
      _LIT(KAL,"Adding Album");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggText();
      iAlbum[aCanvas]= (COggText*)c;
      iAlbum[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Artist")) { 
      _LIT(KAL,"Adding Artist");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggText();
      iArtist[aCanvas]= (COggText*)c;
      iArtist[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Genre")) { 
      _LIT(KAL,"Adding Genre");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggText();
      iGenre[aCanvas]= (COggText*)c;
      iGenre[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("TrackNumber")) { 
      _LIT(KAL,"Adding TrackNumber");
//      RDebug::Print(KAL);
      p.Debug(KAL);
      c= new(ELeave) COggText();
      iTrackNumber[aCanvas]= (COggText*)c;
      iTrackNumber[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("FileName")) {
      c= new COggText();
      _LIT(KAL,"Adding FileName");
//      RDebug::Print(KAL);
	  p.Debug(KAL);
      iFileName[aCanvas]= (COggText*)c;
      iFileName[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Played")) { 
      _LIT(KAL,"Adding Played");
//      RDebug::Print(KAL);
      p.Debug(KAL);
      c= new(ELeave) COggText();
      iPlayed[aCanvas]= (COggText*)c;
      iPlayed[aCanvas]->SetFont(iFont);
#if defined(UIQ)
      iPlayed[aCanvas]->SetText(_L("00:00 / 00:00"));
#endif
    }
    else if (p.iToken==_L("PlayedDigits")) {
      c= new COggDigits;
      iPlayedDigits[aCanvas]= (COggDigits*)c;
    }
    else if (p.iToken==_L("TotalDigits")) {
      c= new COggDigits;
      iTotalDigits[aCanvas]= (COggDigits*)c;
    }
    else if (p.iToken==_L("StopButton")) { 
      c= new(ELeave) COggButton();
//      _LIT(KAL,"Adding StopButton at 0x%x");
//      RDebug::Print(KAL,c);
      //OGGLOG.WriteFormat(KAL,c);
      iStopButton[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PlayButton")) { 
      _LIT(KAL,"Adding PlayButton");
//      RDebug::Print(KAL);
      p.Debug(KAL);
      c= new(ELeave) COggButton();
      iPlayButton[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PauseButton")) { 
      _LIT(KAL,"Adding PauseButton");
//      RDebug::Print(KAL);
      p.Debug(KAL);
      c= new(ELeave) COggButton();
      iPauseButton[aCanvas]= (COggButton*)c;
      iPauseButton[aCanvas]->MakeVisible(EFalse);
    }
    else if (p.iToken==_L("PlayButton2")) { 
      c= new COggButton();
      iPlayButton2[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PauseButton2")) { 
      c= new COggButton();
      iPauseButton2[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("NextSongButton")) { 
      c= new COggButton();
      iNextSongButton[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PrevSongButton")) { 
      c= new COggButton();
      iPrevSongButton[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PlayingIcon")) { 
      _LIT(KAL,"Adding PlayingIcon");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggIcon();
      iPlaying[aCanvas]= (COggIcon*)c;
    }
    else if (p.iToken==_L("PausedIcon")) { 
      _LIT(KAL,"Adding PausedIcon");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggIcon();
      iPaused[aCanvas]= (COggIcon*)c;
      iPaused[aCanvas]->Hide();
    }
    else if (p.iToken==_L("AlarmIcon")) { 
      _LIT(KAL,"Adding AlarmIcon");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggIcon();
      iAlarmIcon[aCanvas]= (COggIcon*)c;
      iAlarmIcon[aCanvas]->MakeVisible(iApp->iSettings.iAlarmActive);
    }
    else if (p.iToken==_L("RepeatIcon")) {
      _LIT(KAL,"Adding RepeadIcon");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggIcon();
      iRepeatIcon[aCanvas]= (COggIcon*)c;
      iRepeatIcon[aCanvas]->MakeVisible(iApp->iSettings.iRepeat);
    }    
    else if (p.iToken==_L("RandomIcon")) {
      _LIT(KAL,"Adding RandomIcon");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggIcon();
      iRandomIcon[aCanvas]= (COggIcon*)c;
      iRandomIcon[aCanvas]->MakeVisible(iApp->iRandom);
    }
    else if (p.iToken==_L("RepeatButton")) { 
      _LIT(KAL,"Adding RepeadButton");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new (ELeave) COggButton();
      iRepeatButton[aCanvas]= (COggButton*)c;
      iRepeatButton[aCanvas]->SetStyle(1);
    }
    else if (p.iToken==_L("Analyzer")) {
      _LIT(KAL,"Adding Analyzer");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggAnalyzer();
      iAnalyzer[aCanvas]= (COggAnalyzer*)c;
      if (iAnalyzer[aCanvas]) iAnalyzer[aCanvas]->SetStyle(iApp->iAnalyzerState);
    }
    else if (p.iToken==_L("Position")) {
      _LIT(KAL,"Adding Position");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggSlider();
      iPosition[aCanvas]= (COggSlider*)c;
    }
    else if (p.iToken==_L("Volume")) {
      _LIT(KAL,"Adding Volume");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggSlider();
      iVolume[aCanvas]= (COggSlider*)c;
    }
    else if (p.iToken==_L("ScrollBar")) {
      _LIT(KAL,"Adding ScrollBar");
//	    RDebug::Print(KAL);
	    p.Debug(KAL);
      c= new(ELeave) COggScrollBar();
      iScrollBar[aCanvas]= (COggScrollBar*)c;
      if (iListBox[aCanvas]) {
	      iListBox[aCanvas]->SetVertScrollBar(iScrollBar[aCanvas]);
	      iScrollBar[aCanvas]->SetAssociatedControl(iListBox[aCanvas]);
      }
    }
    else if (p.iToken==_L("ListBox")) {
      c= new(ELeave) COggListBox();
//      _LIT(KAL,"Adding Listbox at 0x%x");
//      RDebug::Print(KAL,c);
      //OGGLOG.WriteFormat(KAL,c);
      iListBox[aCanvas]= (COggListBox*)c;
	  SetupListBox(iListBox[aCanvas], p.iScaleFactor);
      if (iScrollBar[aCanvas]) {
	      iScrollBar[aCanvas]->SetAssociatedControl(iListBox[aCanvas]);
	      iListBox[aCanvas]->SetVertScrollBar(iScrollBar[aCanvas]);
      }
    }
    else if (p.iToken==_L("Animation")) {
      c= new(ELeave) COggAnimation();
      iAnimation[aCanvas]= (COggAnimation*)c;
    }
    else if (p.iToken==_L("Logo")) {
      c= new(ELeave) COggAnimation();
      iLogo[aCanvas]= (COggAnimation*)c;
    } 
    else if(p.iToken==_L("HotKeys")||p.iToken==_L("Hotkeys")) {
      SetHotkeysFromSkin(p);
    }

    if (c) {
      c->SetObserver(this);
      c->SetBitmapFile(iIconFileName);
      c->Read(p);
      iCanvas[aCanvas]->AddControl(c);
    }
  }

  p.Debug(_L("Canvas read."));

}

TBool COggPlayAppView::SetHotkeysFromSkin(TOggParser& p) {
  p.ReadToken();
  if (p.iToken!=KBeginToken) {
    p.iState= TOggParser::EBeginExpected;
    return EFalse;
  }
  while (p.ReadToken() && p.iToken!=KEndToken && p.iState==TOggParser::ESuccess) { 
    ReadHotkeyArgument(p); 
    }
  if (p.iState==TOggParser::ESuccess && p.iToken!=KEndToken) 
    p.iState= TOggParser::EEndExpected;
  return p.iState==TOggParser::ESuccess;


}

TBool COggPlayAppView::ReadHotkeyArgument(TOggParser& p) {
  TInt numkey;
  if (p.iToken==_L("FastForward")) {
    p.Debug(_L("Setting FastForward."));
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EFastForward]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EFastForward]=ETrue;
    }
  } else if(p.iToken==_L("Rewind")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::ERewind]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::ERewind]=ETrue;
    }
  } else if(p.iToken==_L("PageUp")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EPageUp]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EPageUp]=ETrue;
    }
  } else if(p.iToken==_L("PageDown")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EPageDown]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EPageDown]=ETrue;
    }
  } else if(p.iToken==_L("NextSong")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::ENextSong]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::ENextSong]=ETrue;
    }
  } else if(p.iToken==_L("PreviousSong")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EPreviousSong]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EPreviousSong]=ETrue;
    }
  }
#if !defined(SERIES80) 
  else if(p.iToken==_L("PauseResume"))
  {
    if (p.ReadToken(numkey))
	{
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EPauseResume]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EPauseResume]=ETrue;
    }

  }
#endif
  else if(p.iToken==_L("Play")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EPlay]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EPlay]=ETrue;
    }

  } else if(p.iToken==_L("Pause")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EPause]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EPause]=ETrue;
    }
  } else if(p.iToken==_L("Stop")) {
    if (p.ReadToken(numkey)) {
      iApp->iSettings.iUserHotkeys[TOggplaySettings::EStop]='0'+numkey;
      iApp->iSettings.iLockedHotkeys[TOggplaySettings::EStop]=ETrue;
    }

  }


  return ETrue;

}

void COggPlayAppView::SetupListBox(COggListBox* aListBox, TInt aScaleFactor)
{
  // set up the layout of the listbox:
  CColumnListBoxData* cd= aListBox->GetColumnListBoxData(); 
  TInt w = Size().iWidth;
  TInt graphicsWidth = 18;
  if ((w == 352) && (aScaleFactor == 1))
	  graphicsWidth*= 2;

  cd->SetColumnWidthPixelL(0, graphicsWidth);
  cd->SetGraphicsColumnL(0, ETrue);
  cd->SetColumnAlignmentL(0, CGraphicsContext::ECenter);
  cd->SetColumnWidthPixelL(1, w-graphicsWidth);
  cd->SetColumnAlignmentL(1, CGraphicsContext::ELeft);
  cd->SetColumnWidthPixelL(2, w);
  cd->SetColumnAlignmentL(2, CGraphicsContext::ELeft);

  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());
  aListBox->SetFont(iFont);
  aListBox->SetText(GetTextArray());
  aListBox->SetObserver(this);
  CGulIcon* titleIcon = iEikonEnv->CreateIconL(iIconFileName,0,1);
  CGulIcon* albumIcon = iEikonEnv->CreateIconL(iIconFileName,2,3);
  CGulIcon* artistIcon= iEikonEnv->CreateIconL(iIconFileName,4,5);
  CGulIcon* genreIcon = iEikonEnv->CreateIconL(iIconFileName,6,7);
  CGulIcon* folderIcon= iEikonEnv->CreateIconL(iIconFileName,8,9);
  CGulIcon* fileIcon  = iEikonEnv->CreateIconL(iIconFileName,10,11);
  CGulIcon* backIcon  = iEikonEnv->CreateIconL(iIconFileName,12,13);
  CGulIcon* playIcon  = iEikonEnv->CreateIconL(iIconFileName,34,35);
  CGulIcon* pausedIcon= iEikonEnv->CreateIconL(iIconFileName,16,17);
  //CColumnListBoxData* cd= aListBox->GetColumnListBoxData();
  CArrayPtr<CGulIcon>* icons= cd->IconArray();
  if (!icons) {
    icons = new CArrayPtrFlat<CGulIcon>(3);
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
}

void COggPlayAppView::SetNextFocus() {
  COggControl* c;
  c=iFocusControlsIter;
  //RDebug::Print(_L("SetNextFocus from 0x%x"),c);
  //__ASSERT_DEBUG(c,OGGLOG.Write(_L("Assert: SetNextFocus - iterator has no control !?")));
  //__ASSERT_DEBUG(c->Focus(),OGGLOG.Write(_L("Assert: SetNextFocus - control did not have focus")));
  c->SetFocus(EFalse);
  do {
    iFocusControlsIter++;
    if(!iFocusControlsIter) iFocusControlsIter.SetToFirst();
    c=iFocusControlsIter;
  } while (!c->IsVisible() || c->IsDimmed());
  //RDebug::Print(_L("SetNextFocus to 0x%x"),c);
  //__ASSERT_DEBUG(c,OGGPANIC(_L("no control in focus iterator !"),1318));
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

void 
COggPlayAppView::GotoFlipClosed()
{
  // modify controls for flip-closed mode display
  if (iListBox[1]) iListBox[1]->SetCurrentItemIndex(iSelected);
}

void
COggPlayAppView::GotoFlipOpen()
{
  // modify controls for flip-open mode display
  if (iListBox[0]) iListBox[0]->SetCurrentItemIndex(iSelected);
}

TBool
COggPlayAppView::IsFlipOpen()
{
#if defined(SERIES60) || defined(MOTOROLA)
	return 1;
#else
	return iCoeEnv->ScreenDevice()->CurrentScreenMode() == 0;
#endif
}

void COggPlayAppView::Activated()
{
  if (iActivateCount == 0) MakeVisible(ETrue);
  iActivateCount++;
}

void COggPlayAppView::Deactivated()
{
  iActivateCount--;
  if (iActivateCount == 0) MakeVisible(EFalse);
}

void COggPlayAppView::ChangeLayout(TBool aSmall)
{
  TRect	rect(CEikonEnv::Static()->ScreenDevice()->SizeInPixels());
  if (!aSmall)
    rect = static_cast<CEikAppUi*>(iEikonEnv->AppUi())->ClientRect();
  SetRect(rect);

  if (aSmall) {
    iMode=1;  
    GotoFlipClosed();
    Update();
    iCanvas[1]->DrawControl();
    iCanvas[1]->MakeVisible(ETrue);
    iCanvas[0]->MakeVisible(EFalse);
  } else {
    iMode= 0;
    GotoFlipOpen();
    Update();
    iCanvas[0]->DrawControl();
    iCanvas[0]->MakeVisible(ETrue);
    iCanvas[1]->MakeVisible(EFalse);
  }
}

void
COggPlayAppView::AppToForeground(const TBool aForeground) const
{
  RWsSession& ws=iEikonEnv->WsSession();
  RWindowGroup& rw = iEikonEnv->RootWin();
  TInt winId = rw.Identifier();
  TApaTask tApatsk(ws);
  tApatsk.SetWgId(winId);
  if (aForeground) {
    tApatsk.BringToForeground();
    iApp->ActivateOggViewL();
  }
  else
    tApatsk.SendToBackground();
}


_LIT(KEmpty,"");

const TInt 
COggPlayAppView::GetValueFromTextLine(TInt idx)
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

TBool
COggPlayAppView::HasAFileName(TInt idx)
{
    // Returns true if a filename is associated with this item

	COggListBox::TItemTypes type = GetItemType(idx) ;
	if ((type == COggListBox::ETitle) || (type == COggListBox::EFileName) || (type == COggListBox::EPlayList))
        return(ETrue);
    return(EFalse);
}

const TDesC &
COggPlayAppView::GetFileName(TInt idx)
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


const COggPlayAppUi::TViews
COggPlayAppView::GetViewName(TInt idx)
{
	COggListBox::TItemTypes type = GetItemType(idx);

	__ASSERT_ALWAYS ( type == COggListBox::EBack, 
      User::Panic(_L("GetViewName called with wrong argument"),0 ) );

  return (COggPlayAppUi::TViews) GetValueFromTextLine(idx);
}

void
COggPlayAppView::GetFilterData(TInt idx, TDes & aData)
{
	COggListBox::TItemTypes type = GetItemType(idx) ;

#ifdef PLAYLIST_SUPPORT
	__ASSERT_ALWAYS ( ( (type == COggListBox::EAlbum) || (type == COggListBox::EArtist) || (type ==COggListBox::EGenre) || (type == COggListBox::ESubFolder) || (type == COggListBox::EPlayList) || (type == COggListBox::ETitle) || (type == COggListBox::EFileName) ), 
      User::Panic(_L("COggPlayAppView::GetFilterData called with wrong argument"),0 ) );
#else
   __ASSERT_ALWAYS ( ( (type == COggListBox::EAlbum) || (type == COggListBox::EArtist) || (type ==COggListBox::EGenre) || (type == COggListBox::ESubFolder) ), 
      User::Panic(_L("COggPlayAppView::GetFilterData called with wrong argument"),0 ) );
#endif
    
    CDesCArray* arr= GetTextArray();

    __ASSERT_ALWAYS ( arr, 
      User::Panic(_L("COggPlayAppView::GetFilterData Array Not found"),0 ) );
    
    TPtrC sel,msel;
    if(idx >= 0 && idx < arr->Count()) 
    {
        sel.Set((*arr)[idx]);
        TextUtils::ColumnText(msel, 2, &sel );
    }
    aData.Copy(msel);
    return ;
}

COggListBox::TItemTypes
COggPlayAppView::GetItemType(TInt idx)
{
  // TODO: Panic if we can't get the item type (returning COggListBox::ETitle isn't really the right thing to do)
  CDesCArray* arr= GetTextArray();
  if (!arr) 
    return COggListBox::ETitle;

  TPtrC sel,msel;
  if(idx >= 0 && idx < arr->Count()) {
    sel.Set((*arr)[idx]);
    TextUtils::ColumnText(msel, 0, &sel);
    TLex parse(msel);
    TInt atype(COggListBox::ETitle);
    parse.Val(atype);
    return (COggListBox::TItemTypes) atype;
  }

  return COggListBox::ETitle;
}

void
COggPlayAppView::SelectItem(TInt idx)
{
  iSelected= idx;
  if (iListBox[iMode]) {
    iSelected = iListBox[iMode]->SetCurrentItemIndex(idx);
    iCanvas[iMode]->Refresh();
  }
}

void
COggPlayAppView::SelectFile(TOggFile* aFile)
{
  TInt found = -1;
  for (TInt i=0; i<GetTextArray()->Count(); i++)
  {
      if( HasAFileName(i) )
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
      iSelected= found;
      if (iListBox[iMode]) {
          iSelected = iListBox[iMode]->SetCurrentItemIndex(found);
      }
  } // Otherwise, do nothing
}


TInt
COggPlayAppView::GetSelectedIndex()
{
  return iSelected;
  /*
  if (iListBox[iMode]) return iListBox[iMode]->CurrentItemIndex();
  TInt otherMode= (iMode+1)%2;
  if (iListBox[otherMode]) return iListBox[otherMode]->CurrentItemIndex();
  return -1;
  */
}


CDesCArray*
COggPlayAppView::GetTextArray()
{
  return iTextArray;
}

void 
COggPlayAppView::ListBoxPageDown() 
{
  TInt index = iListBox[iMode]->CurrentItemIndex();
  if( index != iListBox[iMode]->CountText() - 1)
        SelectItem(index + iListBox[iMode]->NofVisibleLines() - 1);
}
    
void 
COggPlayAppView::ListBoxPageUp() 
{
  TInt index = iListBox[iMode]->CurrentItemIndex();
  if(index != 0)
        SelectItem(index - iListBox[iMode]->NofVisibleLines() + 1);
}

void
COggPlayAppView::SetTime(TInt64 aTime)
{
  if (iPosition[iMode]) iPosition[iMode]->SetMaxValue(aTime.GetTInt());
}

void
COggPlayAppView::UpdateRepeat()
{
  if (iApp->iSettings.iRepeat) {
    if (iRepeatIcon[iMode]) iRepeatIcon[iMode]->Show();
    if (iRepeatButton[iMode]) iRepeatButton[iMode]->SetState(1);
  } else {
    if (iRepeatIcon[iMode]) iRepeatIcon[iMode]->Hide();
    if (iRepeatButton[iMode]) iRepeatButton[iMode]->SetState(0);
  }
}

void
COggPlayAppView::UpdateRandom()
{
  if (iApp->iRandom) {
    if (iRandomIcon[iMode]) iRandomIcon[iMode]->Show();
  } else {
    if (iRandomIcon[iMode]) iRandomIcon[iMode]->Hide();
  }
}

TInt
COggPlayAppView::CallBack(TAny* aPtr)
{
  COggPlayAppView* self = (COggPlayAppView*) aPtr;
  self->HandleCallBack();

  return 1;
}

void COggPlayAppView::HandleCallBack()
{
  if (iAnalyzer[iMode] && iAnalyzer[iMode]->Style()>0) 
  {
    if (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying)
    {
#ifdef MDCT_FREQ_ANALYSER 
      iAnalyzer[iMode]->RenderWaveformFromMDCT(
      iApp->iOggPlayback->GetFrequencyBins() );
#else /* !MDCT_FREQ_ANALYSER */
#if defined(SERIES60)
      iAnalyzer[iMode]->RenderWaveform((short int*)iApp->iOggPlayback->GetDataChunk());
#else
      iAnalyzer[iMode]->RenderWaveform((short int[2][512])iApp->iOggPlayback->GetDataChunk());
#endif
#endif /* MDCT_FREQ_ANALYSER */
    }
  }

#if defined(UIQ)
  // Leave UIQ behaviour unchanged (for now)
  if (iPosition[iMode] && iPosChanged<0) 
	iPosition[iMode]->SetValue(iApp->iOggPlayback->Position().GetTInt());

  iCanvas[iMode]->CycleLowFrequencyControls();
  iCanvas[iMode]->CycleHighFrequencyControls();
  iCanvas[iMode]->Refresh();
#else
  if (iCycleFrequencyCounter % 2)
  {
	if (iPosition[iMode] && iPosChanged<0) 
		iPosition[iMode]->SetValue(iApp->iOggPlayback->Position().GetTInt());
    
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
#endif
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
	iApp->iVolume = (iApp->iSettings.iAlarmVolume * KMaxVolume)/10;
	iApp->SetVolumeGainL((TGainType) iApp->iSettings.iAlarmGain);

	// Play the next song or unpause the current one
	if (iApp->iOggPlayback->State() == CAbsPlayback::EPaused)
		iApp->PauseResume();
	else
		iApp->NextSong();
  }

  // Reset the alarm
  CAknQueryDialog* snoozeDlg = CAknQueryDialog::NewL();
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
  TRACELF("InitView");
  // fill the list box with some initial content:
  if (!iOggFiles->ReadDb(iApp->iDbFileName,iCoeEnv->FsSession())) {
    iOggFiles->CreateDb(iCoeEnv->FsSession());
    iOggFiles->WriteDbL(iApp->iDbFileName, iCoeEnv->FsSession());
  }

  const TBuf<16> dummy;
  //iApp->iViewBy= COggPlayAppUi::ETop;
  FillView(COggPlayAppUi::ETop, COggPlayAppUi::ETop, dummy);

  if (iAnalyzer[0]) iAnalyzer[0]->SetStyle(iApp->iAnalyzerState);
  if (iAnalyzer[1]) iAnalyzer[1]->SetStyle(iApp->iAnalyzerState);
}

void
COggPlayAppView::FillView(COggPlayAppUi::TViews theNewView, COggPlayAppUi::TViews thePreviousView, const TDesC& aSelection)
{
  TBuf<32> buf;
  TBuf<16> dummy;
  TBuf<256> back;

  if (theNewView==COggPlayAppUi::ETop) {
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

#ifdef PLAYLIST_SUPPORT
    iEikonEnv->ReadResource(buf, R_OGG_STRING_12);
	TOggFiles::AppendLine(*GetTextArray(), COggListBox::EPlayList, buf, dummy);
#endif
  }
  else if (thePreviousView==COggPlayAppUi::ETop) {
    switch (theNewView) {
    case COggPlayAppUi::ETitle: iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, dummy); break;
    case COggPlayAppUi::EAlbum: iOggFiles->FillAlbums(*GetTextArray(), dummy, dummy); break;
    case COggPlayAppUi::EArtist: iOggFiles->FillArtists(*GetTextArray(), dummy); break;
    case COggPlayAppUi::EGenre: iOggFiles->FillGenres(*GetTextArray(), dummy, dummy, dummy); break;
    case COggPlayAppUi::ESubFolder: iOggFiles->FillSubFolders(*GetTextArray()); break;
    case COggPlayAppUi::EFileName: iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, dummy); break;

#ifdef PLAYLIST_SUPPORT
	case COggPlayAppUi::EPlayList: iOggFiles->FillPlayLists(*GetTextArray()); break;
#endif
    default: break;
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
    back.AppendNum((TInt)thePreviousView);
    GetTextArray()->InsertL(0,back);
  }
  else {
    switch (thePreviousView) {
    case COggPlayAppUi::ETitle    : iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, dummy); break;
    case COggPlayAppUi::EAlbum    : iOggFiles->FillTitles(*GetTextArray(), aSelection, dummy, dummy, dummy); break;
    case COggPlayAppUi::EArtist   : iOggFiles->FillTitles(*GetTextArray(), dummy, aSelection, dummy, dummy); break;
    case COggPlayAppUi::EGenre    : iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, aSelection, dummy); break;
    case COggPlayAppUi::ESubFolder: iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, aSelection); break;
    case COggPlayAppUi::EFileName : iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, dummy); break;

#ifdef PLAYLIST_SUPPORT
    case COggPlayAppUi::EPlayList : iOggFiles->FillPlayList(*GetTextArray(), aSelection); break;
#endif
    default: break;
    }

	if (aSelection.Length()>0) {
		back.Num((TInt) COggListBox::EBack);
      back.Append(KColumnListSeparator);
#if defined(UIQ)
      back.Append(_L(".."));
#else
      iEikonEnv->ReadResource(buf, R_OGG_BACK_DIR);
      back.Append(buf);
#endif
      back.Append(KColumnListSeparator);
      back.AppendNum((TInt)thePreviousView);
      GetTextArray()->InsertL(0,back);
    }
  }

  iApp->iViewBy= theNewView;

  UpdateListbox();
  // Choose the selected item.
  // If random is turned off: First of the list, unless it is a "back" icon, in that case 2nd
  // If random is turned on: 
  //   When playing a file, do as if random wasn't turned on
  //   If not playing a file, choose a random file

  if (theNewView==COggPlayAppUi::ETop) 
  {
      SelectItem(0);
  } else
  {
      if (iApp->iRandom)
      {
          if ( iApp->iSongList->AnySongPlaying() )
          {
              // When a song has been playing, give full power to the user
              // Do not select a random value.
              SelectItem(1); 
          } else
          {
              // Select a random value for the first index
			  TInt64 rnd64 = TInt64(0, Math::Random());
			  TInt64 maxInt64 = TInt64(1, 0);
			  TInt64 nbEntries64 = GetTextArray()->Count()-1;
			  TInt64 picked64 = (rnd64 * nbEntries64) / maxInt64;
			  TInt picked = picked64.Low() + 1;

              SelectItem(picked);
          }
      } else
      {
          SelectItem(1);
      }
  }
}

void
COggPlayAppView::Update()
{
  UpdateAnalyzer();
  UpdateListbox();
  UpdateControls();
  UpdateSongPosition();
  UpdateClock(ETrue);
  UpdateVolume();
  UpdatePlaying();
}

void
COggPlayAppView::UpdateAnalyzer()
{
  if (iAnalyzer[iMode]) {
    iAnalyzer[iMode]->Clear();
    iAnalyzer[iMode]->SetStyle(iApp->iAnalyzerState);
  }
}

void
COggPlayAppView::UpdateListbox()
{
  if (iApp->iViewBy!=COggPlayAppUi::ETitle && 
      iApp->iViewBy!=COggPlayAppUi::EFileName) {
    if (iListBox[iMode]) iListBox[iMode]->Redraw();
    return;
  }

  CDesCArray* txt= GetTextArray();
  // BERT: Add a check if there is a song currently playing...
  TBool paused = iApp->iOggPlayback->State()==CAbsPlayback::EPaused;
  const TOggFile* currSong = iApp->iSongList->GetPlayingFile();

  for (TInt i=0; i<txt->Count(); i++) {
    TPtrC sel;
    sel.Set((*txt)[i]);
    TBuf<512> buf(sel);

	COggListBox::TItemTypes type = GetItemType(i);
	if (type!=COggListBox::EBack) {
		TBuf<1> playState;
		if ((type == COggListBox::ETitle) || (type == COggListBox::EFileName) || (type == COggListBox::EPlayList))
		{
			if ( GetFile(i) == currSong ) {
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
    txt->InsertL(i,buf);
  }
  if (iListBox[iMode]) iListBox[iMode]->Redraw();
}

TBool
COggPlayAppView::CanPlay()
{
  return (iApp->iOggPlayback->State()==CAbsPlayback::EPaused || 
	  iApp->iOggPlayback->State()==CAbsPlayback::EClosed ||
	  iApp->iOggPlayback->State()==CAbsPlayback::EFirstOpen);
}

TBool
COggPlayAppView::CanPause()
{
  return (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying);
}

TBool
COggPlayAppView::CanStop()
{
  return (iApp->iOggPlayback->State()==CAbsPlayback::EPaused || 
	  iApp->iOggPlayback->State()==CAbsPlayback::EPlaying);
}

TBool
COggPlayAppView::PlayDimmed()
{
  return 
	  (GetSelectedIndex()<0 || GetItemType(GetSelectedIndex())==COggListBox::EBack || 
     iApp->iViewBy==COggPlayAppUi::EArtist || iApp->iViewBy==COggPlayAppUi::EAlbum || 
     iApp->iViewBy==COggPlayAppUi::EGenre || iApp->iViewBy==COggPlayAppUi::ESubFolder ||
     iApp->iViewBy==COggPlayAppUi::ETop) 
    && !(iApp->iOggPlayback->State()==CAbsPlayback::EPaused);
}

void 
COggPlayAppView::UpdateControls()
{
  if (iPlayButton[iMode] && CanPlay()) 
    iPlayButton[iMode]->SetDimmed(PlayDimmed());

  if (iPlayButton2[iMode])
    iPlayButton2[iMode]->SetDimmed(!CanPlay() || PlayDimmed());
  if (iPauseButton2[iMode])
    iPauseButton2[iMode]->SetDimmed(!CanPause());

  if (iPlayButton[iMode]) iPlayButton[iMode]->MakeVisible(CanPlay());
  if (iPauseButton[iMode]) iPauseButton[iMode]->MakeVisible(!CanPlay());
    
  if (iStopButton[iMode]) iStopButton[iMode]->SetDimmed( !CanStop() );

  if (iPosition[iMode]) iPosition[iMode]->SetDimmed( !CanStop() );
 
  if (iRepeatButton[iMode]) iRepeatButton[iMode]->SetState(iApp->iSettings.iRepeat);
  if (iRepeatIcon[iMode]) iRepeatIcon[iMode]->MakeVisible(iApp->iSettings.iRepeat);
  if (iRandomIcon[iMode]) iRandomIcon[iMode]->MakeVisible(iApp->iRandom);
}

void
COggPlayAppView::UpdateSongPosition()
{
#if defined(SERIES60)
  // Only show "Played" time component when not stopped, i.e. only show 
  // when artist, title etc is displayed.
  if( iPlayed[iMode] )
    {
    TBool playedControlIsVisible = (iApp->iOggPlayback->State() != CAbsPlayback::EClosed) &&
                                   (iApp->iOggPlayback->State() != CAbsPlayback::EFirstOpen);
    iPlayed[iMode]->MakeVisible( playedControlIsVisible );
    if( !playedControlIsVisible )
      return;
    }
#endif


  // Update the song position displayed in the menubar (flip open) 
  // or in the TOggCanvas (flip closed):
  //------------------------------------

  TBuf<64> mbuf;
  TInt sec= iApp->iOggPlayback->Position().GetTInt()/1000;
  TInt min= sec/60;
  sec-= min*60;
  TInt sectot= iApp->iOggPlayback->Time().GetTInt()/1000;
  TInt mintot= sectot/60;
  sectot-= mintot*60;
  mbuf.Format(_L("%02d:%02d / %02d:%02d"), min, sec, mintot, sectot);
#if defined(UIQ)
  if (IsFlipOpen()) {
    CEikMenuBar* mb = iEikonEnv->AppUiFactory()->MenuBar();
    if (!mb) return;
    // As we cannot officially get the TitleArray, we have to steal it (hack)
    CEikMenuBar::CTitleArray *ta;
    ta = *(CEikMenuBar::CTitleArray**)((char*)mb + sizeof(*mb) - 4*sizeof(void*));
    ta->At(1)->iData.iText.Copy(mbuf);
    mb->SetSize(mb->Size());
    mb->DrawNow();
  } else {
    if (iPlayed[iMode]) iPlayed[iMode]->SetText(mbuf);
    if (iPlayedDigits[iMode]) iPlayedDigits[iMode]->SetText(mbuf.Left(5));
    if (iTotalDigits[iMode]) iTotalDigits[iMode]->SetText(mbuf.Right(5));
  }
#else
  if (iPlayed[iMode]) iPlayed[iMode]->SetText(mbuf);
  if (iPlayedDigits[iMode]) iPlayedDigits[iMode]->SetText(mbuf.Left(5));
  if (iTotalDigits[iMode]) iTotalDigits[iMode]->SetText(mbuf.Right(5));
#endif
  SetTime(iApp->iOggPlayback->Time());

}

void
COggPlayAppView::UpdateClock(TBool forceUpdate)
{
  if (!iClock[iMode])
	  return;

  // Update the clock
  TTime now;
  now.HomeTime();
  TDateTime dtNow(now.DateTime());
  TInt clockMinute = dtNow.Minute();
  TBool clockUpdateRequired = clockMinute != iCurrentClockMinute;
  if (!clockUpdateRequired && !forceUpdate) return;

  TBuf<256> buf;
  now.FormatL(buf, KDateString);
  iClock[iMode]->SetText(buf);
  iCurrentClockMinute = clockMinute;

  if (!forceUpdate || !iAlarm[iMode])
	  return;

  // Update alarm
  iApp->iSettings.iAlarmTime.FormatL(buf, KDateString);
  iAlarm[iMode]->SetText(buf);
}

void
COggPlayAppView::UpdateVolume()
{
  if (iVolume[iMode]) iVolume[iMode]->SetValue(iApp->iVolume);
}

void COggPlayAppView::UpdatePlaying()
{
  if (iTitle[iMode]) {
    if (iApp->iOggPlayback->Title().Length()==0)
        {
        #if defined(UIQ)
            iTitle[iMode]->SetText(iApp->iOggPlayback->FileName());
        #else
            // Series 60 : Show only the filename. (space consideration)
            TParsePtrC p(iApp->iOggPlayback->FileName());
            iTitle[iMode]->SetText( p.NameAndExt() );
        #endif
        }
      
    else if(!iArtist[iMode]) { // smelly code ahead
      _LIT(KSeparator," - ");
      TInt length=KSeparator().Length()+iApp->iOggPlayback->Title().Length()+iApp->iOggPlayback->Artist().Length();
      HBufC* iText = HBufC::NewL(length);
      iText->Des().Copy(iApp->iOggPlayback->Artist());
      iText->Des().Append(KSeparator);
      iText->Des().Append(iApp->iOggPlayback->Title());
      iTitle[iMode]->SetText(*iText);
      free(iText);
    } else {
      iTitle[iMode]->SetText(iApp->iOggPlayback->Title());
    }
  }
  if (iAlbum[iMode]) iAlbum[iMode]->SetText(iApp->iOggPlayback->Album());
  if (iArtist[iMode]) iArtist[iMode]->SetText(iApp->iOggPlayback->Artist());
  if (iGenre[iMode]) iGenre[iMode]->SetText(iApp->iOggPlayback->Genre());
  if (iTrackNumber[iMode]) iTrackNumber[iMode]->SetText(iApp->iOggPlayback->TrackNumber());

  if (iPlaying[iMode]) 
    iPlaying[iMode]->MakeVisible(iApp->iOggPlayback->State()==CAbsPlayback::EPlaying);

  if (iPaused[iMode]) {
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
    TBuf<256> buft;
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
  if (iAlarmIcon[iMode]) iAlarmIcon[iMode]->Hide();
}

TInt
COggPlayAppView::CountComponentControls() const
{
  return iControls->Count();
}

CCoeControl*
COggPlayAppView::ComponentControl(TInt aIndex) const
{
  return iControls->At(aIndex);
}

void
COggPlayAppView::HandleControlEventL(CCoeControl* /*aControl*/, TCoeEvent /*aEventType*/)
{
}


TKeyResponse
COggPlayAppView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{
#if defined(SERIES60) || defined(SERIES80)
  enum EOggKeys {
    EOggConfirm=EKeyDevice3
  };
  enum EOggScancodes {
    EOggUp=EStdKeyUpArrow,
    EOggDown=EStdKeyDownArrow,
    EOggRight=EStdKeyRightArrow,
    EOggLeft=EStdKeyLeftArrow
    };

  // Utilities
  TInt code = aKeyEvent.iCode;
  TInt index = iListBox[iMode]->CurrentItemIndex();

  COggControl* c=iFocusControlsIter;
  if (code==0 && aType==EEventKeyDown) { 
    TInt scanCode = aKeyEvent.iScanCode;
    switch (scanCode)
    {
    	case EStdKeyDevice6 : scanCode = EOggLeft; //S80 joystick
    	    break;
    	case EStdKeyDevice7 : scanCode = EOggRight;//S80 joystick
    	    break;
    	case EStdKeyDevice8 : scanCode = EOggUp;   //S80 joystick
    	    break;
    	case EStdKeyDevice9 : scanCode = EOggDown; //S80 joystick
    		break;
    	case EStdKeyDeviceA : code = EOggConfirm;//S80 joystick
    	    break;
    }
    if(iFocusControlsPresent && scanCode==EOggLeft) {
      SetPrevFocus();
      return EKeyWasConsumed;
    } else if (iFocusControlsPresent && scanCode==EOggRight) {
      SetNextFocus();
      return EKeyWasConsumed;
    } else {
      if(c==iListBox[iMode] || !iFocusControlsPresent) {
        if (scanCode==EOggDown) {
          SelectItem(index+1);
          return EKeyWasConsumed;
        } else if (scanCode==EOggUp ) {
          SelectItem(index-1);
          return EKeyWasConsumed;
        } 
      } 
      if(   (c==iVolume[iMode] && (scanCode==EOggUp || scanCode==EOggDown))
         || (!iFocusControlsPresent && (scanCode==EOggLeft || scanCode==EOggRight))
        ) {
        if(iApp->iOggPlayback->State()==CAbsPlayback::EPlaying) {
          if (scanCode==EOggUp || scanCode==EOggRight)  {
            iApp->iVolume+= KStepVolume;
          } else  if (scanCode==EOggDown || scanCode==EOggLeft) {
            iApp->iVolume-= KStepVolume;
          }
          if (iApp->iVolume>KMaxVolume) iApp->iVolume = KMaxVolume;
          if (iApp->iVolume<0) iApp->iVolume = 0;
          iApp->iOggPlayback->SetVolume(iApp->iVolume);
		      UpdateVolume();
        } else { // not playing, i.e. stopped or paused
          if (scanCode==EOggDown || scanCode==EOggLeft) {
            iApp->SelectPreviousView();
          } else if (scanCode==EOggDown || scanCode==EOggRight) {
            if( HasAFileName(index) )
              iApp->HandleCommandL(EOggPlay);
            else
              iApp->SelectNextView();
          }

        }
        return EKeyWasConsumed;
      } 
    }   
  }  

  if(code == EOggConfirm) {
    if(!iFocusControlsPresent) {

       if (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying ||
            iApp->iOggPlayback->State()==CAbsPlayback::EPaused) {
            if (iApp->iSongList->IsSelectedFromListBoxCurrentlyPlaying() && ((iApp->iViewBy==COggPlayAppUi::ETitle) || (iApp->iViewBy==COggPlayAppUi::EFileName)))
                // Event on the current active file
                iApp->HandleCommandL(EOggPauseResume);
            else
                iApp->HandleCommandL(EOggPlay);
        }
      else
        // Event without any active files
        iApp->HandleCommandL(EOggPlay);

    } else if(c==iPlayButton[iMode]) { 
      // manually move focus to pause button
      c->SetFocus(EFalse);
      do {
        iFocusControlsIter++;
        if(!iFocusControlsIter) iFocusControlsIter.SetToFirst();
        c=iFocusControlsIter;
      } while (c!=iPauseButton[iMode]);
      c->SetFocus(ETrue);
      iApp->HandleCommandL(EOggPauseResume);
    } else if (c==iPauseButton[iMode]) {
      c->SetFocus(EFalse);
      do {
        iFocusControlsIter++;
        if(!iFocusControlsIter) iFocusControlsIter.SetToFirst();
        c=iFocusControlsIter;
      } while (c!=iPlayButton[iMode]);
      c->SetFocus(ETrue);
      iApp->HandleCommandL(EOggPauseResume);
    } else if(c==iStopButton[iMode]) {
        iApp->HandleCommandL(EOggStop);
    } else if (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying ||
	             iApp->iOggPlayback->State()==CAbsPlayback::EPaused) 
           iApp->HandleCommandL(EOggStop);
      else iApp->HandleCommandL(EOggPlay);
    return EKeyWasConsumed;
  } 
  #if defined(SERIES60)
  // S60 User Hotkeys
  switch(COggUserHotkeysControl::Hotkey(aKeyEvent,aType,&iApp->iSettings)) {
    case TOggplaySettings::EFastForward : {
      TInt64 pos=iApp->iOggPlayback->Position()+=KFfRwdStep;
      iApp->iOggPlayback->SetPosition(pos);
  	  iPosition[iMode]->SetValue(pos.GetTInt());
	  UpdateSongPosition();
	  iCanvas[iMode]->Refresh();
	  return EKeyWasConsumed;
      }
    case TOggplaySettings::ERewind : {
      TInt64 pos=iApp->iOggPlayback->Position()-=KFfRwdStep;
      iApp->iOggPlayback->SetPosition(pos);
  	  iPosition[iMode]->SetValue(pos.GetTInt());
	  UpdateSongPosition();
	  iCanvas[iMode]->Refresh();
      return EKeyWasConsumed;
      }
    case TOggplaySettings::EPageDown : {
      ListBoxPageDown();
      return EKeyWasConsumed;
      }
    case TOggplaySettings::EPageUp : {
      ListBoxPageUp();
      return EKeyWasConsumed;
      }
    case TOggplaySettings::ENextSong : {
	    iApp->NextSong();
	    return EKeyWasConsumed;
	    }
    case TOggplaySettings::EPreviousSong : {
	    iApp->PreviousSong();
	    return EKeyWasConsumed;
	    }
    case TOggplaySettings::EPauseResume : {
	    iApp->PauseResume();
	    return EKeyWasConsumed;
	    }
    case TOggplaySettings::EPlay : {
	    iApp->HandleCommandL(EOggPlay);
	    return EKeyWasConsumed;
	    }
    case TOggplaySettings::EPause : {
	    iApp->HandleCommandL(EUserPauseCBA); // well..
	    return EKeyWasConsumed;
	    }
    case TOggplaySettings::EStop : {
      iApp->HandleCommandL(EOggStop);		
      return EKeyWasConsumed;
	    }
    case TOggplaySettings::EKeylock : {
      if( aKeyEvent.iRepeats > 0 )
        {
        RAknKeyLock keyLock;
        keyLock.Connect ();
        keyLock.EnableKeyLock() ;
        keyLock.Close() ;
        }
      return EKeyWasConsumed;
      }

    case TOggplaySettings::EVolumeBoostUp : {
		TGainType currentGain = (TGainType) iApp->iSettings.iGainType;
		TInt newGain = currentGain + 1;
		if (newGain<=EStatic12dB)
			iApp->SetVolumeGainL((TGainType) newGain);

		return EKeyWasConsumed;
		}

	case TOggplaySettings::EVolumeBoostDown : {
		TGainType currentGain = (TGainType) iApp->iSettings.iGainType;
		TInt newGain = currentGain - 1;
		if (newGain>=EMinus24dB)
			iApp->SetVolumeGainL((TGainType) newGain);

		return EKeyWasConsumed;
		}
	default :
      break;
    }
#endif

  if (code>0) {
    if (aKeyEvent.iScanCode==EStdKeyDeviceD) { iApp->NextSong(); return EKeyWasConsumed; }           // jog dial up
    else if (aKeyEvent.iScanCode==EStdKeyDeviceE) { iApp->PreviousSong(); return EKeyWasConsumed; }  // jog dial down
    else if (aKeyEvent.iScanCode==167) { AppToForeground(EFalse); return EKeyWasConsumed; }          // back key <-
    else if (aKeyEvent.iScanCode==174) { iApp->HandleCommandL(EOggStop); return EKeyWasConsumed; }   // "C" key
    //else if (aKeyEvent.iScanCode==173) { iApp->HandleCommandL(EOggShuffle); return EKeyWasConsumed; }// menu button
    else if (aKeyEvent.iScanCode==173) { 
    } else if (aKeyEvent.iScanCode==133) { // "*"
      if (iTitle[iMode]) iTitle[iMode]->ScrollNow();
      if (iAlbum[iMode]) iAlbum[iMode]->ScrollNow();
      if (iArtist[iMode]) iArtist[iMode]->ScrollNow();
      if (iGenre[iMode]) iGenre[iMode]->ScrollNow();
      return EKeyWasConsumed;
    }
    else if (aKeyEvent.iScanCode==127) { // "#"
      iApp->ToggleRepeat();
      return EKeyWasConsumed;
    }
    // 48..57 == "0"..."9"
    // 172 jog dial press
    // 179    == OK

    // this is what's being sent when closing through taskswitcher
    // couldn't find any documentation on this.
    if(code==EStdKeyNumLock) {
      iApp->HandleCommandL(EEikCmdExit);
      return EKeyWasConsumed;
    }
  }


  return EKeyWasNotConsumed;




#else // UIQ keyhandling

#if !defined( MOTOROLA )



  enum EOggKeys {
      EOggConfirm=EQuartzKeyConfirm,
  };
  enum EOggScancodes {
    EOggUp=EStdQuartzKeyTwoWayUp,
    EOggDown=EStdQuartzKeyTwoWayDown
  };

  TInt code     = aKeyEvent.iCode;
  TInt modifiers= aKeyEvent.iModifiers & EAllStdModifiers;
  TInt iHotkey  = ((COggPlayAppUi *)iEikonEnv->EikAppUi())->iHotkey;


  if (iHotkey && aType == EEventKey &&
     code == keycodes[iHotkey-1].code && modifiers==keycodes[iHotkey-1].mask) {
    iApp->ActivateOggViewL();
    return EKeyWasConsumed;
  }

  if(code == EOggConfirm) {
    if (IsFlipOpen()) {
      if (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying ||
	  iApp->iOggPlayback->State()==CAbsPlayback::EPaused) iApp->HandleCommandL(EOggStop);
      else iApp->HandleCommandL(EOggPlay);
    } else {
      if (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying ||
	  iApp->iOggPlayback->State()==CAbsPlayback::EPaused) iApp->HandleCommandL(EOggPauseResume);
      else iApp->HandleCommandL(EOggPlay);
    }
    return EKeyWasConsumed;
  }

  if (code>0) {
    if (aKeyEvent.iScanCode==EStdKeyDeviceD) { iApp->NextSong(); return EKeyWasConsumed; }           // jog dial up
    else if (aKeyEvent.iScanCode==EStdKeyDeviceE) { iApp->PreviousSong(); return EKeyWasConsumed; }  // jog dial down
    else if (aKeyEvent.iScanCode==167) { AppToForeground(EFalse); return EKeyWasConsumed; }          // back key <-
    else if (aKeyEvent.iScanCode==174) { iApp->HandleCommandL(EOggStop); return EKeyWasConsumed; }   // "C" key
    //else if (aKeyEvent.iScanCode==173) { iApp->HandleCommandL(EOggShuffle); return EKeyWasConsumed; }// menu button
    else if (aKeyEvent.iScanCode==173) { 
      iApp->LaunchPopupMenuL(R_POPUP_MENU,TPoint(100,100),EPopupTargetTopLeft);
      //Invalidate();
      //Update();
      return EKeyWasConsumed; 
    } else if (aKeyEvent.iScanCode==133) { // "*"
      if (iTitle[iMode]) iTitle[iMode]->ScrollNow();
      if (iAlbum[iMode]) iAlbum[iMode]->ScrollNow();
      if (iArtist[iMode]) iArtist[iMode]->ScrollNow();
      if (iGenre[iMode]) iGenre[iMode]->ScrollNow();
      return EKeyWasConsumed;
    }
    else if (aKeyEvent.iScanCode==127) { // "#"
      ToggleRepeat();
      return EKeyWasConsumed;
    }
    // 48..57 == "0"..."9"
    // 172 jog dial press
    // 179    == OK
  }

  if (iApp->iOggPlayback->State()==CAbsPlayback::EPlaying || !IsFlipOpen() ) {
    // volume
    if (code==0 && aType==EEventKeyDown) { 
      if (aKeyEvent.iScanCode==EOggUp || aKeyEvent.iScanCode==EOggDown) {
		    if (aKeyEvent.iScanCode==EOggUp && iApp->iVolume<100) iApp->iVolume+= 5; 
		    if (aKeyEvent.iScanCode==EOggDown && iApp->iVolume>0) iApp->iVolume-= 5;
		    iApp->iOggPlayback->SetVolume(iApp->iVolume);
		    UpdateVolume();
		    return EKeyWasConsumed;
      }
    }
  } else if (iListBox[iMode]) {
    // move current index in the list box
    if (code==0 && aType==EEventKeyDown) {
      TInt idx= iListBox[iMode]->CurrentItemIndex();
      if (aKeyEvent.iScanCode==EOggUp && idx>0) {
        SelectItem(idx-1);
        return EKeyWasConsumed;
      }
      if (aKeyEvent.iScanCode==EOggDown) {
        SelectItem(idx+1);
        return EKeyWasConsumed;
      }
    }
  }  

  return EKeyWasNotConsumed;

#else  // defined MOTOROLA
  
   enum EOggKeys 
   { EOggConfirm = EQuartzKeyConfirm,
     EOggUp      = EQuartzKeyFourWayUp,
     EOggDown    = EQuartzKeyFourWayDown,
     EOggLeft    = EQuartzKeyFourWayLeft,
     EOggRight   = EQuartzKeyFourWayRight,
     EOggButton1 = EKeyApplicationA,
     EOggButton2 = EKeyApplicationB
   };
  
   TKeyResponse resp = EKeyWasNotConsumed;
   TInt code         = aKeyEvent.iCode;
   TInt modifiers    = aKeyEvent.iModifiers & EAllStdModifiers;
   TInt iHotkey      = ((COggPlayAppUi *) iEikonEnv->EikAppUi())->iHotkey;

   if ( aType == EEventKey )
   {
      if ( iHotkey && 
           code      == keycodes[iHotkey-1].code && 
           modifiers == keycodes[iHotkey-1].mask )
      {
         iApp->ActivateOggViewL();
         resp = EKeyWasConsumed;
      }
      else
      {
         switch ( code )
         {
         case EOggConfirm:
            if ( iApp->iOggPlayback->State() == CAbsPlayback::EPlaying ||
                 iApp->iOggPlayback->State() == CAbsPlayback::EPaused )
            {
               iApp->HandleCommandL(EOggStop);
            }
            else
            {
               iApp->HandleCommandL(EOggPlay);
            }
            resp = EKeyWasConsumed;
            break;

         case EOggUp:
            if ( iApp->iOggPlayback->State() != CAbsPlayback::EPlaying &&
                 iApp->iOggPlayback->State() != CAbsPlayback::EPaused )
            {
               iApp->HandleCommandL(EOggPlay);
            }
            resp = EKeyWasConsumed;
            break;

         case EOggDown:
            if ( iApp->iOggPlayback->State() == CAbsPlayback::EPlaying ||
                 iApp->iOggPlayback->State() == CAbsPlayback::EPaused )
            {
               iApp->HandleCommandL(EOggStop);
            }
            resp = EKeyWasConsumed;
            break;

         case EOggLeft:
            iApp->PreviousSong();
            resp = EKeyWasConsumed;
            break;

         case EOggRight:
            iApp->NextSong();
            resp = EKeyWasConsumed;
            break;

         case EOggButton1:
            if (iTitle[iMode]) iTitle[iMode]->ScrollNow();
            if (iAlbum[iMode]) iAlbum[iMode]->ScrollNow();
            if (iArtist[iMode]) iArtist[iMode]->ScrollNow();
            if (iGenre[iMode]) iGenre[iMode]->ScrollNow();
            resp = EKeyWasConsumed;
            break;

         case EOggButton2:
            iApp->LaunchPopupMenuL(R_POPUP_MENU,TPoint(100,100),EPopupTargetTopLeft);
            resp = EKeyWasConsumed;
            break;

         default:
            break;
         }
      }
   }
   return ( resp );

#endif

#endif  // UIQ keyhandling
}


void
COggPlayAppView::OggControlEvent(COggControl* c, TInt aEventType, TInt /*aValue*/)
{
  if (c==iListBox[iMode]) {
    if (aEventType==1) {
      iSelected= iListBox[iMode]->CurrentItemIndex();
      UpdateControls(); // the current item index changed
    }
    if (aEventType==2) iApp->HandleCommandL(EOggPlay); // an item was selected (by tipping on it)
  }

  if (c==iPlayButton[iMode]) {
    if (iApp->iOggPlayback->State()==CAbsPlayback::EPaused)
      iApp->HandleCommandL(EOggPauseResume);
    else
      iApp->HandleCommandL(EOggPlay);
  }
  if (c==iPauseButton[iMode]) iApp->HandleCommandL(EOggPauseResume);
  if (c==iStopButton[iMode]) iApp->HandleCommandL(EOggStop);
  if (c==iNextSongButton[iMode]) iApp->NextSong();
  if (c==iPrevSongButton[iMode]) iApp->PreviousSong();
  if (c==iRepeatButton[iMode]) iApp->ToggleRepeat();
}

void 
COggPlayAppView::OggPointerEvent(COggControl* c, const TPointerEvent& p)
{
  if (c==iPosition[iMode]) {
    if (p.iType==TPointerEvent::EButton1Down || p.iType==TPointerEvent::EDrag) {
      iPosChanged= iPosition[iMode]->CurrentValue();
    }
    if (p.iType==TPointerEvent::EButton1Up) {
      iApp->iOggPlayback->SetPosition(TInt64(iPosChanged));
      iPosChanged= -1;
    }
  }

  if (c==iVolume[iMode]) {
    iApp->iVolume= iVolume[iMode]->CurrentValue();
    iApp->iOggPlayback->SetVolume(iApp->iVolume);
  }

  if (c==iAnalyzer[iMode]) {
    iApp->iAnalyzerState= iAnalyzer[iMode]->Style();
  }
}

void COggPlayAppView::AddControlToFocusList(COggControl* c) {
  COggControl* currentitem = iFocusControlsIter; 
  if (currentitem) {
    iFocusControlsPresent=ETrue;
    //OGGLOG.WriteFormat(_L("COggPlayAppView::AddControlToFocusList adding 0x%x"),c);
    c->iDlink.Enque(&currentitem->iDlink);
    iFocusControlsIter.Set(*c);
  } else {
    TRACEL("COggPlayAppView::AddControlToFocusList, iFocusControlsHeader.AddFirst");
    //OGGLOG.WriteFormat(_L("COggPlayAppView::AddControlToFocusList - Setting up focus list adding 0x%x"),c);
    iFocusControlsHeader.AddFirst(*c);
    iFocusControlsIter.SetToFirst(); 
  }
}
