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

#ifndef __OggPlay_h
#define __OggPlay_h

#include "OggOs.h"
#include "OggPlayUid.h"
#include <e32math.h> // For Random

#if defined(SERIES60)
#include <aknappui.h>
#include <aknapp.h>
#include <eikdoc.h>
#include <stdio.h>

const TInt KFullScreenWidth = 176;
const TInt KFullScreenHeight = 188;

#elif defined(SERIES80)
#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#else
#include <qikappui.h>
#include <qikapplication.h>
#include <qikdocument.h>

const TInt KFullScreenWidth = 208;
const TInt KFullScreenHeight = 320;

#endif

const TInt ENoFileSelected = -1;

#include <coeccntx.h>  // MCoeControlContex
#include <e32base.h>
#include <etel.h>

#include <OggPlay.rsg>
#include "OggPlay.hrh"
#include "OggAbsPlayback.h"

#include "OggViews.h"
#include "OggControls.h"
#include "OggMsgEnv.h"

// some global constants:
//-----------------------
//FIXME

const TUid KOggPlayUid1 = { 0x1000007a };
const TUid KOggPlayUid2 = { 0x100039ce };
const TUid KOggPlayUid  = { KOggPlayApplicationUidValue }; 

const TUid KOggPlayUidFOView = { KOggPlayApplicationUidValue +1 };
const TUid KOggPlayUidFCView = { KOggPlayApplicationUidValue +2 };
const TUid KOggPlayUidSettingsView = { KOggPlayApplicationUidValue +3 };
const TUid KOggPlayUidUserView = { KOggPlayApplicationUidValue+4 };
const TUid KOggPlayUidSplashView = { KOggPlayApplicationUidValue+5 };
const TUid KOggPlayUidCodecSelectionView = { KOggPlayApplicationUidValue+6 };


//_LIT(KAudioResourceFile, "Z:\\System\\Apps\\Audio\\audio.mbm");
_LIT(KAudioResourceFile, "Z:\\System\\Apps\\OggPlay\\OggPlay.mbm");
_LIT(KQTimeResourceFile, "Z:\\System\\Apps\\QTime\\QTime.mbm");

const TInt KMaxFileNameLength = 0x200;


struct TKeyCodes
{
  int code; 
  int mask;
};

const TInt KMaxKeyCodes = 4;
static const TKeyCodes keycodes[] = 
{ { EKeyApplication1, 0 }, // camera
  { EKeyApplication0, 0 }, // browser
  { EKeyDevice0,      0 }, // power
  { 0,                0 }, // flip open
  // EKeyDevice8 confirm (jog dial push)
  // EKeyDevice1/2  two way controller
  // EKeyDevice4..7 four way controller 
  // EKeyDial
};

#if defined(__WINS__)
_LIT(KMmcSearchDir,"C:\\Ogg\\");
#else
_LIT(KMmcSearchDir,"E:\\Ogg\\");
#endif

// global settings - these are set through the settings view

class TOggplaySettings
	{
public:
  enum TScanmode    // COggplayDisplaySettingItemList - COggSettingsContainer
		{
      EFullScan,
      EMmcOgg,
      EMmmcFull
		};
  TInt iScanmode;
  TInt iAutoplay;
  TInt iManeuvringSpeed;
  TInt iWarningsEnabled;
  enum TRsk {
    ECbaExit,
    ECbaStop,
    ECbaPause,
    ECbaPlay,
    ECbaBack
  };
  TInt iRskIdle;
  TInt iRskPlay;
  TInt iGainType;
  
  enum THotkeys {   // COggUserHotkeys
    ENoHotkey,
    EFastForward,
    ERewind,
    EPageUp,
    EPageDown,
    ENextSong,
    EPreviousSong,
    EKeylock,
    ENofHotkeysV4=EKeylock, 
    EPauseResume,
    EPlay,
    EPause,
    EStop,
    ENofHotkeysV5,
    ENofHotkeys=ENofHotkeysV5,
    KFirstHotkeyIndex = EFastForward
    };

  TInt iUserHotkeys[ENofHotkeys];
  TBool iLockedHotkeys[ENofHotkeys];
};

// Forward declarations:
//----------------------

class COggPlayAppUi;
class COggPlayAppView;
class COggFOView;

class CEikColumnListBox;
class CEikBitmapButton;
class COggSongList;


// COggActive
// A utility class monitoring the telephone line
//----------------------------------------------

#ifdef MONITOR_TELEPHONE_LINE
class COggActive : public CBase
{
 public:
   
  COggActive();
  void ConstructL(COggPlayAppUi* theAppUi);
  ~COggActive();

  void IssueRequest();
  static TInt CallBack(TAny* aPtr);

 private:
  RTelServer*    iServer;
  RPhone*        iPhone;
  RLine*         iLine;
  TRequestStatus iRequestStatusChange;
  
  CPeriodic*     iTimer;
  TCallBack*     iCallBack;

  TInt           iInterrupted;
  TInt           iLineCallsReportedAtIdle;
  TInt           iLineToMonitor;
  COggPlayAppUi* iAppUi;
};
#endif

#ifdef SERIES60
class COggPlayAppUi : public CAknAppUi, public MPlaybackObserver
#elif defined(SERIES80)
class COggPlayAppUi : public CEikAppUi, public MPlaybackObserver
#else
class COggPlayAppUi : public CQikAppUi, public MPlaybackObserver
#endif
{
public:

  void ConstructL();
  void PostConstructL();
  ~COggPlayAppUi();

  // the views supported by the listbox:
  enum TViews { ETitle=0, EAlbum, EArtist, EGenre, ESubFolder, EFileName, ETop, ETrackTitle };

  // global settings stored in the ini file:
  TOggplaySettings iSettings;

  // backwards compatibility - deprecated
  // as long as these are not part of TOggplaySettings, they can't be set 
  // through the settings view
  int iHotkey;
  int iVolume;            // [0...100]
  int iRepeat;            // 0=off; 1=on
  TBool iRandom;          // 0=off; 1=on
  int iAnalyzerState;     // 0= off; 1=on; 2= on with peaks
  TViews iViewBy;
  TTime iAlarmTime;       // the alarm has been set to this time

  // global status:
  int iTryResume;         // after iTryResume seconds try resuming music (after sound device was stolen)
  int iAlarmTriggered;    // did the alarm clock go off?
  int iAlarmActive;       // has an alarm time been set?

  TBool iForeground;      // is the application currently in the foreground?
  TBool iIsRunningEmbedded; // is true when application got startet through the recognizer
  TBool iIsStartup;        // is true when just started, needed for autoplay
  TFileName iDbFileName;
  
  COggPlayAppView* iAppView;
  CAbsPlayback*    iOggPlayback;

#ifdef MONITOR_TELEPHONE_LINE
  COggActive*      iActive;
#endif

  COggMsgEnv*      iOggMsgEnv;

  COggSongList*    iSongList;

  // from MPlaybackObserver:
  virtual void NotifyPlayComplete();
  virtual void NotifyUpdate();
  virtual void NotifyPlayInterrupted();

  // high level functions:
  void PlaySelect();
  void PauseResume();
  void Stop();
  void NextSong();
  void PreviousSong();
  void ShowFileInfo();
  void SelectPreviousView();
  void SelectNextView();

  void ActivateOggViewL();
  void ActivateOggViewL(const TUid aViewId);

  void UpdateSeries60Softkeys(TBool aForce=EFalse);
  void SetSeries60Softkeys(TInt aSoftkey);

  // from CQikAppUi:
  void HandleCommandL(int aCommand);
  void HandleApplicationSpecificEventL(TInt aType, const TWsEvent& aEvent);
  void HandleForegroundEventL(TBool aForeground);
  void DynInitMenuPaneL(int aMenuId, CEikMenuPane* aMenuPane);
  TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName,const TDesC8& aTail);

  void OpenFileL(const TDesC& aFileName);
  void WriteIniFile();
  void WriteIniFileOnNextPause();
  void SetRepeat(TBool aRepeat);

private: 

  void ReadIniFile();
  TInt32 IniRead32( TFileText& aFile, TInt32 aDefault = 0, TInt32 aMaxValue = 0x7FFFFFFF );
  TInt64 IniRead64( TFileText& aFile, TInt64 aDefault = 0 );
  void SetHotKey();
  TBool IsAlarmTime();
  void SetProcessPriority();
  void SetThreadPriority();
  void FindSkins();
#if defined(SERIES60_SPLASH)
  void ShowSplash();
#endif

	TBool iWriteIniOnNextPause;
  int iCapturedKeyHandle;
  HBufC* iIniFileName;
  TFileName iSkinFileDir;
  TInt iCurrentSkin;
  CDesCArray* iSkins;

  COggFOView* iFOView;
  COggFCView* iFCView;
#if defined(SERIES60)
  COggSettingsView* iSettingsView;
  COggUserHotkeysView* iUserHotkeys;
#ifdef SERIES60_SPLASH_WINDOW_SERVER
  COggSplashView* iSplashView;
#endif
#ifdef PLUGIN_SYSTEM
   COggSettingsView* iCodecSelectionView;
#endif
#endif /* SERIES60 */
  RArray<TInt> iViewHistoryStack;
  RArray<TInt> iRestoreStack;
  TInt iRestoreCurrent;
};

// COggSongList:
// Base class defining the interface for managing the song list.
//------------------------------------------------------


class COggSongList : public CBase
{
    public:
     virtual void ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback);
     virtual const TDesC & GetNextSong()=0;    
     virtual const TDesC & GetPreviousSong()=0;
     void  SetPlayingFromListBox(TInt aPlaying);
     const TDesC& GetPlaying();
     const TInt GetPlayingAbsoluteIndex();
     const TBool AnySongPlaying();
     void  SetRepeat(TBool aRepeat);
     const TBool IsSelectedFromListBoxCurrentlyPlaying();
     ~COggSongList();
    protected:
        
     void  SetPlaying(TInt aPlaying);
     const TDesC & RetrieveFileName(TInt anAbsoluteIndex);
     TInt iPlayingIdx;           // index of the file which is currently being played
     RArray<TInt> iFileList;
     COggPlayAppView* iAppView; 
     CAbsPlayback*    iOggPlayback;
     TBool iRepeat;
     TBool iNewFileList;
};

class COggNormalPlay : public COggSongList
{
    public:     
        void ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback);
        const TDesC & GetNextSong();  
        const TDesC & GetPreviousSong();
        ~COggNormalPlay();
        COggNormalPlay();
    private:
};

class COggRandomPlay : public COggSongList
{
    public:     
        void ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback);
        virtual const TDesC & GetNextSong();  
        virtual const TDesC & GetPreviousSong();
        ~COggRandomPlay(); 
        COggRandomPlay();
    private:
        RArray<TInt> iRandomMemory;
        TInt64 iSeed;
};


class COggPlayDocument : public CEikDocument
{
public:
#ifdef SERIES60
  COggPlayDocument(CAknApplication& aApp) : CEikDocument(aApp) {}
#else
  COggPlayDocument(CEikApplication& aApp) : CEikDocument(aApp) {}
#endif

  CFileStore*  OpenFileL(TBool aDoOpen ,const TDesC& aFilename, RFs& aFs);
  
  CEikAppUi* CreateAppUiL() ;
};

#ifdef SERIES60
class COggPlayApplication : public CAknApplication
#elif defined(SERIES80)
class COggPlayApplication : public CEikApplication
#else
class COggPlayApplication : public CQikApplication
#endif
{
private: 
  // from CApaApplication
  CApaDocument* CreateDocumentL() { return new (ELeave)COggPlayDocument(*this); }
  TUid AppDllUid() const { TUid id = { KOggPlayApplicationUidValue }; return id; }
};


#endif
