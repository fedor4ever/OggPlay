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

#include <coeccntx.h>  // MCoeControlContex

#include "OggControls.h"
#include "OggPlay.h"
#include "OggFiles.h"
#include "OggUserHotkeys.h"

class CCoeControl;
enum COggPlayAppUi::TViews;

class COggPlayAppView : public CCoeControl,
			public MCoeControlObserver,
			public MCoeControlContext,
			public MOggControlObserver
{
public:

  COggPlayAppView();
  ~COggPlayAppView();
  void ConstructL(COggPlayAppUi *aApp, const TRect& aRect);

  // display functions:
  void InitView();
  void FillView(COggPlayAppUi::TViews theNewView, COggPlayAppUi::TViews thePreviousView, const TDesC& aSelection);
  void Invalidate();
  void Update(); // calls all of the following:
  void UpdateListbox();
  void UpdateControls();
  void UpdateSongPosition();
  void UpdateClock(TBool forceUpdate=EFalse);
  void UpdateVolume();
  void UpdatePlaying();
  void UpdateAnalyzer();

  void ToggleRepeat();
  void SetTime(TInt64 aTime);

  // access/manipulation of the playlist:
  TInt        GetNSongs();
  TDesC&      GetFileName(TInt idx);
  TInt        GetItemType(TInt idx);
  void        ShufflePlaylist();
  void        SelectSong(TInt idx);
  TInt        GetSelectedIndex();
  /// Default argument will use GetSelectedIndex() as index
  TBool       isPlayableFile( TInt aIndex = -1 );
  CDesCArray* GetTextArray();

  void  SetAlarm();
  void  ClearAlarm();

  // Handle flip-closed and flip-open modes:
  TBool IsFlipOpen();
  void  GotoFlipClosed();
  void  GotoFlipOpen();
  void  AppToForeground(const TBool aForeground) const;
  void  SwitchToStandbyView();
  void  Activated();
  void  Deactivated();
  void  ChangeLayout(TBool aSmall);

  void  ReadSkin(const TFileName& aFileName);
  void  ReadControls();

  void SetupListBox(COggListBox* aListBox);

  // keyboard focus handling:
  void SetNextFocus();
  void SetPrevFocus();

  TBool CanPlay();
  TBool CanPause();
  TBool CanStop();
  TBool PlayDimmed();

  static TInt CallBack(TAny* aPtr); // registered with iTimer (CPeriodic)
  void ListBoxNavigationTimerTick( TBool aRestart );


  // from MCoeControlObserver:
  void         HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType);

  // from CCoeControl:
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
  TInt         CountComponentControls() const;
  CCoeControl* ComponentControl(TInt aIndex) const;

  // from MOggControlObserver:
  virtual void OggPointerEvent(COggControl* c, const TPointerEvent& p);
  virtual void OggControlEvent(COggControl* c, TInt aEventType, TInt aValue);
  virtual void AddControlToFocusList(COggControl* c);

  TInt                  iPosChanged;
  TBuf<512>             iCurrentSong;
  TOggFiles*            iOggFiles;
  TCallBack*            iCallBack;

private:

  void  ReadCanvas(TInt aCanvas, TOggParser& p);
  void  ResetControls();
  void  ClearCanvases();

  /// COggCanvas[0..1], flip open and closed
  CArrayPtrFlat<CCoeControl>* iControls;
  COggPlayAppUi* iApp;

  CPeriodic* iTimer;
  TBool      iInCallBack;

  TInt       iActivateCount;
  TTime      iMinTime;
  TTime      iMaxTime;
  TFileName  iIconFileName;
  //TFileName  iBackgroundFileName;
  CDesCArray* iTextArray;
  TInt       iSelected; 
  /// Data store for GetFileName()
  TBuf<KMaxFileNameLength> iFileNameStore;
  TInt iUserInactivityTickCount;
  
  //keyboard focus handling: 
  // do we do keyboard focus handling ?
  TBool      iFocusControlsPresent;
  //maintain a doubly linked list of controls that can get focus.
  TDblQue<COggControl> iFocusControlsHeader;
  TDblQueIter<COggControl> iFocusControlsIter;

  TInt               iMode; // 0 = flip-open; 1= flip-closed
  COggCanvas*        iCanvas[2];

  // Ogg controls
  // (one for flip-closed/open each, can be the same pointer)

  COggText*          iTitle[2];
  COggText*          iArtist[2];
  COggText*          iAlbum[2];
  COggText*          iGenre[2];
  COggText*          iTrackNumber[2];
  COggText*          iFileName[2];
  COggText*          iClock[2];
  COggText*          iAlarm[2];
  COggText*          iPlayed[2];

  COggDigits*        iPlayedDigits[2];
  COggDigits*        iTotalDigits[2];

  COggIcon*          iAlarmIcon[2];
  COggIcon*          iRepeatIcon[2];
  COggIcon*          iEye[2];
  COggIcon*          iPlaying[2];
  COggIcon*          iPaused[2];

  COggSlider*        iVolume[2];
  COggSlider*        iPosition[2];

  COggButton*        iPlayButton[2];
  COggButton*        iPauseButton[2];
  COggButton*        iPlayButton2[2];
  COggButton*        iPauseButton2[2];
  COggButton*        iStopButton[2];
  COggButton*        iNextSongButton[2];
  COggButton*        iPrevSongButton[2];
  COggButton*        iRepeatButton[2];

  COggAnalyzer*      iAnalyzer[2];

  COggListBox*       iListBox[2];
  COggScrollBar*     iScrollBar[2];

  COggAnimation*     iAnimation[2];
  COggAnimation*     iLogo[2];

};

#endif
