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

#include <OggOs.h>
#include <OggPlayUid.h>
#include <e32math.h> // For Random

#if defined(SERIES60)
#include <aknappui.h>
#include <aknapp.h>
#include <eikdoc.h>

#elif defined(SERIES80)
#include <eikenv.h> 
#include <eikappui.h>
#include <eikapp.h>
#include <eikdoc.h>
#include <eikbtgpc.h> 

#else
#include <qikappui.h>
#include <qikapplication.h>
#include <qikdocument.h>

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
#include "OggMultiThread.h"

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

#if defined(MULTI_THREAD_PLAYBACK)
const TUid KOggPlayUidPlaybackOptionsView = { KOggPlayApplicationUidValue+7 };
#endif

const TUid KOggPlayUidAlarmSettingsView = { KOggPlayApplicationUidValue+8 };

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


_LIT(KFullScanString,"Full scan");

// global settings - these are set through the settings view

#ifdef SERIES80
 const TInt KNofSoftkeys = 4;
#else
 const TInt KNofSoftkeys = 1;
#endif
 
class TOggplaySettings
	{
public:
  enum TScanmode    // COggplayDisplaySettingItemList - COggSettingsContainer
		{
      EFullScan,
      EMmcOgg,
      EMmmcFull
#ifdef SERIES80
      ,ECustomDir
#endif
		};
  TInt iScanmode;
  
#ifdef SERIES80
  TBuf<255> iCustomScanDir;
#endif
  
  TInt iAutoplay;
  TInt iManeuvringSpeed;
  TInt iWarningsEnabled;
   
  TInt iSoftKeysPlay[KNofSoftkeys];
  TInt iSoftKeysIdle[KNofSoftkeys];
  TInt iGainType;
  
  enum THotkeys {   // COggUserHotkeys
    ENoHotkey,
    EFastForward,
    ERewind,
    EPageUp,
    EPageDown,
    ENextSong,
    EPreviousSong,
#if !defined(SERIES80)
    EKeylock,
    EPauseResume,
#endif
    EPlay,
    EPause,
    EStop,
	EVolumeBoostUp,
	EVolumeBoostDown,
    EHotKeyExit,
    EHotKeyBack,
    EHotkeyVolumeHelp,

    KFirstHotkeyIndex = EFastForward,
#if defined(SERIES80)
	ENofHotkeysV4=EPlay,
#else
	ENofHotkeysV4=EKeylock,
#endif
	ENofHotkeysV5=EVolumeBoostUp,
	ENofHotkeysV7=EHotKeyExit,
    ENofHotkeys=EHotkeyVolumeHelp+1
    };

  TInt iUserHotkeys[ENofHotkeys];
  TBool iLockedHotkeys[ENofHotkeys];
  TBool iRepeat;
  TBool iRandom;

  // Multi thread playback settings
  TInt iBufferingMode;
  TInt iThreadPriority;	

  // Alarm clock settings
  TBool iAlarmActive;
  TTime iAlarmTime;
  TInt iAlarmVolume;
  TInt iAlarmGain;
  TInt iAlarmSnooze;
};

 
static const TInt KHotkeysActions[]  =
  {
  	//Must be kept synchronized with the TOggplaySettings::THotkey enum !
  	EEikCmdCanceled,	// ENoHotkey,
    EUserFastForward,	// EFastForward,
    EUserRewind, 		// ERewind,
    EUserListBoxPageUp, // EPageUp,
    EUserListBoxPageDown,// EPageDown,
    EOggNextSong,		// ENextSong,
    EOggPrevSong,		// EPreviousSong,
#if !defined(SERIES80)
    EUserKeylock,		// EKeylock,
    EUserPlayPauseCBA,	// EPauseResume,
#endif
    EUserPlayCBA,   	// EPlay,
    EUserPauseCBA,  	// EPause,
    EUserStopPlayingCBA,// EStop,
	EVolumeBoostUp,		// EVolumeBoostUp,
	EVolumeBoostDown,	// EVolumeBoostDown,
    EEikCmdExit,    	// EHotKeyExit,
    EUserBackCBA,  		// EHotKeyBack,
    EUserVolumeHelp     // EHotkeyVolumeHelp 
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
  void CancelCallBack();

  static TInt CallBack(TAny* aPtr);
  TInt CallBack();

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
#ifdef PLAYLIST_SUPPORT
  enum TViews { ETitle=0, EAlbum, EArtist, EGenre, ESubFolder, EFileName, EPlayList, ETop, ETrackTitle };
#else
  enum TViews { ETitle=0, EAlbum, EArtist, EGenre, ESubFolder, EFileName, ETop, ETrackTitle };
#endif

  // global settings stored in the ini file:
  TOggplaySettings iSettings;

  // backwards compatibility - deprecated
  // as long as these are not part of TOggplaySettings, they can't be set 
  // through the settings view
  int iHotkey;
  int iVolume;            // [0...100]
  int iAnalyzerState;     // 0= off; 1=on; 2= on with peaks
  TViews iViewBy;

  // global status:
  int iTryResume;         // after iTryResume seconds try resuming music (after sound device was stolen)

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
  virtual void NotifyUpdate();
  virtual void NotifyPlayComplete();
  virtual void NotifyPlayInterrupted();
  virtual void ResumeUpdates();

#if defined(DELAY_AUDIO_STREAMING_START)
  virtual void NotifyPlayStarted();
#endif

#if defined(MULTI_THREAD_PLAYBACK)
  virtual void NotifyFatalPlayError();
#endif

  // high level functions:
  void PlaySelect();
  void PauseResume();
  void Pause();
  void Stop();
  void NextSong();
  void PreviousSong();
  void ShowFileInfo();
  void SelectPreviousView();
  void SelectNextView();

  void ActivateOggViewL();
  void ActivateOggViewL(const TUid aViewId);

  void UpdateSoftkeys(TBool aForce=EFalse);
  
  // from CQikAppUi:
  void HandleCommandL(int aCommand);
  void HandleApplicationSpecificEventL(TInt aType, const TWsEvent& aEvent);
  void HandleForegroundEventL(TBool aForeground);
  void DynInitMenuPaneL(int aMenuId, CEikMenuPane* aMenuPane);
  TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName,const TDesC8& aTail);

  void OpenFileL(const TDesC& aFileName);
  TInt FileSize(const TDesC& aFileName);
  void WriteIniFile();
  void SetRandomL(TBool aRandom);
  void SetRepeat(TBool aRepeat);
  void ToggleRepeat();
  
#ifdef SERIES80
  CEikButtonGroupContainer * Cba()  
  { return CEikonEnv::Static()->AppUiFactory()->ToolBar();} 
#endif

  void SetVolumeGainL(TGainType aNewGain);

#if defined(MULTI_THREAD_PLAYBACK)
  void SetBufferingModeL(TBufferingMode aNewBufferingMode);
  void SetThreadPriority(TStreamingThreadPriority aNewThreadPriority);
#endif

  void SetAlarm(TBool aAlarmActive);
  void SetAlarmTime();

private:
  void ReadIniFile();
  TInt32 IniRead32( TFileText& aFile, TInt32 aDefault = 0, TInt32 aMaxValue = 0x7FFFFFFF );
  TInt64 IniRead64( TFileText& aFile, TInt64 aDefault = 0 );
#ifdef SERIES80
  void IniReadDes( TFileText& aFile, TDes& value,const TDesC& defaultValue );
#endif
  void SetHotKey();

#if !defined(MULTI_THREAD_PLAYBACK)
  void SetProcessPriority();
  void SetThreadPriority();
#endif

  void FindSkins();
#if defined(SERIES60_SPLASH)
  void ShowSplash();
#endif

  int iCapturedKeyHandle;
  HBufC* iIniFileName;
  TFileName iSkinFileDir;
  TInt iCurrentSkin;
  CDesCArray* iSkins;

  COggFOView* iFOView;
  COggFCView* iFCView;
#if defined(SERIES60)
  COggSettingsView* iSettingsView;
  COggUserHotkeysView* iUserHotkeysView;

#ifdef SERIES60_SPLASH_WINDOW_SERVER
  COggSplashView* iSplashView;
#endif

#ifdef PLUGIN_SYSTEM
   COggPluginSettingsView* iCodecSelectionView;
#endif

#if defined(MULTI_THREAD_PLAYBACK)
  COggPlaybackOptionsView* iPlaybackOptionsView;
#endif

  COggAlarmSettingsView* iAlarmSettingsView;
#endif /* SERIES60 */

  RArray<TInt> iViewHistoryStack;
  RArray<TInt> iRestoreStack;
  TInt iRestoreCurrent;

  TBool iForeground;
};

class TOggPlayList;
class TOggPlayListStackEntry
{
public:
	TOggPlayListStackEntry()
	{ }

	TOggPlayListStackEntry(TOggPlayList* aPlayList, TInt aPlayingIdx)
	: iPlayList(aPlayList), iPlayingIdx(aPlayingIdx)
	{ }

public:
	TOggPlayList* iPlayList;
	TInt iPlayingIdx;
};

class ROggPlayListStack
{
public:
	TInt Push(const TOggPlayListStackEntry& aPlayListStackEntry);
	void Pop(TOggPlayListStackEntry& aPlayListStackEntry);

	void Reset();
	void Close();

private:
	RPointerArray<TOggPlayList> iPlayListArray;
	RArray<TInt> iPlayingIdxArray;
};


// COggSongList:
// Base class defining the interface for managing the song list.
//------------------------------------------------------
class TOggFile;
class COggSongList : public CBase
{
    public:
     virtual void ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback);
     virtual const TDesC & GetNextSong()=0;    
     virtual const TDesC & GetPreviousSong()=0;
     void  SetPlayingFromListBox(TInt aPlaying);
     const TDesC& GetPlaying();
     const TOggFile* GetPlayingFile();
     const TBool AnySongPlaying();
     void  SetRepeat(TBool aRepeat);
     const TBool IsSelectedFromListBoxCurrentlyPlaying();
     ~COggSongList();
    protected:
        
     void  SetPlaying(TInt aPlaying, TBool aPreviousSong = EFalse);
     TInt iPlayingIdx;           // index of the file which is currently being played
     RPointerArray<TOggFile> iFileList;
     COggPlayAppView* iAppView; 
     CAbsPlayback*    iOggPlayback;
     TBool iRepeat;
     TBool iNewFileList;

	 ROggPlayListStack iPlayListStack;
	 TOggPlayList* iPlayList;
	 TInt iPlayListIdx;
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
        RPointerArray<TOggFile> iRandomMemory;
        RArray<TInt> iRandomMemoryIdx;
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
