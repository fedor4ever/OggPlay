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
const TInt KFfRwdStep = 20000;

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

// Some global constants
const TUid KOggPlayUid1 = { 0x1000007a };
const TUid KOggPlayUid2 = { 0x100039ce };
const TUid KOggPlayUid  = { KOggPlayApplicationUidValue }; 

const TUid KOggPlayUidSplashFOView = { 1 };
const TUid KOggPlayUidSplashFCView = { 2 };
const TUid KOggPlayUidFOView = { 3 };
const TUid KOggPlayUidFCView = { 4 };
const TUid KOggPlayUidSettingsView = { 5 };
const TUid KOggPlayUidUserView = { 6 };
const TUid KOggPlayUidCodecSelectionView = { 7 };
const TUid KOggPlayUidPlaybackOptionsView = { 8 };
const TUid KOggPlayUidAlarmSettingsView = { 9 };

struct TKeyCodes
	{
	TInt code; 
	TInt mask;
	};

const TInt KMaxKeyCodes = 4;
static const TKeyCodes KHotkeyKeycodes[] = 
	{
	{ EKeyApplication1, 0 }, // camera
	{ EKeyApplication0, 0 }, // browser
	{ EKeyDevice0,      0 }, // power
	{ 0,                0 }, // flip open
	// EKeyDevice8 confirm (jog dial push)
	// EKeyDevice1/2  two way controller
	// EKeyDevice4..7 four way controller 
	// EKeyDial
	};

#if defined(__WINS__)
_LIT(KMmcSearchDir, "C:\\Ogg\\");
#else
_LIT(KMmcSearchDir, "E:\\Ogg\\");
#endif

_LIT(KFullScanString,"Full scan");

// Global settings - these are set through the settings view
#if defined(SERIES80)
const TInt KNofSoftkeys = 4;
#else
const TInt KNofSoftkeys = 1;
#endif
 
class TOggplaySettings
	{
public:
	enum TScanmode
		{
		EFullScan,
		EMmcOgg,
		EMmcFull,
		ECustomDir
		};
  
	TInt iScanmode;
  
#if defined(SERIES80)
  	TFileName iCustomScanDir;
#endif
  
	TBool iAutoplay;
	TInt iManeuvringSpeed;
	TBool iWarningsEnabled;
   
	TInt iSoftKeysPlay[KNofSoftkeys];
	TInt iSoftKeysIdle[KNofSoftkeys];
	TInt iGainType;
  
	enum THotkeys
		{  
		// COggUserHotkeys
		ENoHotkey,
		EFirstHotkeyIndex,
		EHotkeyFastForward = EFirstHotkeyIndex,
		EHotkeyRewind,
		EHotkeyPageUp,
		EHotkeyPageDown,
		EHotkeyNextSong,
		EHotkeyPreviousSong,

#if !defined(SERIES80)
		EHotkeyKeylock,
		EHotkeyPauseResume,
#endif

		EHotkeyPlay,
		EHotkeyPause,
		EHotkeyStop,
		EHotkeyVolumeBoostUp,
		EHotkeyVolumeBoostDown,
		EHotkeyExit,
		EHotkeyBack,
		EHotkeyVolumeHelp,
		EHotkeyToggleShuffle,
		EHotkeyToggleRepeat,

		ENumHotkeys
		};

	TInt iUserHotkeys[ENumHotkeys];
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

 
const TInt KHotkeysActions[]  =
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
    EUserVolumeHelp,    // EHotkeyVolumeHelp 
	EOggShuffle,        // EHotkeyToggleShuffle
	EOggRepeat          // EHotkeyToggleRepeat
	};
 
 
// Forward declarations
class COggPlayAppUi;
class COggPlayAppView;
class COggFOView;
class COggFCView;

class CEikColumnListBox;
class CEikBitmapButton;
class COggSongList;


// COggActive
// A utility class monitoring the telephone line
#if defined(MONITOR_TELEPHONE_LINE)
class COggActive : public CBase
	{
public:
	COggActive();
	~COggActive();

	void ConstructL(COggPlayAppUi* theAppUi);

	void IssueRequest();
	void CancelCallBack();

	static TInt CallBack(TAny* aPtr);
	TInt CallBack();

private:
	RTelServer* iServer;
	RPhone* iPhone;
	RLine* iLine;
	TRequestStatus iRequestStatusChange;
  
	CPeriodic* iTimer;
	TCallBack* iCallBack;

	TInt iInterrupted;
	TInt iLineCallsReportedAtIdle;
	TInt iLineToMonitor;
	COggPlayAppUi* iAppUi;
	};
#endif

class COggPlayAppUi;
class COggStartUpAO : public CActive
	{
public:
	COggStartUpAO(COggPlayAppUi& aAppUi);
	~COggStartUpAO();

	void NextStartUpState();

private:
	// From CActive
	void RunL();
	void DoCancel();

private:
	COggPlayAppUi& iAppUi;
	};

class TOggFiles;
class COggStartUpEmbeddedAO : public CActive
	{
public:
	COggStartUpEmbeddedAO(COggPlayAppUi& aAppUi, TOggFiles& aFiles);
	~COggStartUpEmbeddedAO();

	void CreateDbWithSingleFile(TFileName& aFileName);

private:
	// From CActive
	void RunL();
	void DoCancel();

private:
	COggPlayAppUi& iAppUi;
	TOggFiles& iFiles;
	};

class COggViewActivationAO : public CActive
	{
public:
	COggViewActivationAO(COggPlayAppUi& aAppUi);
	~COggViewActivationAO();

	void ActivateOggView();

private:
	// From CActive
	void RunL();
	void DoCancel();

private:
	COggPlayAppUi& iAppUi;
	};

#if defined(SERIES60)
class CAknErrorNote;
class COggPlayAppUi : public CAknAppUi, public MPlaybackObserver, public MFileInfoObserver
#elif defined(SERIES80)
class COggPlayAppUi : public CEikAppUi, public MPlaybackObserver, public MFileInfoObserver
#else
class COggPlayAppUi : public CQikAppUi, public MPlaybackObserver, public MFileInfoObserver
#endif
{
public:
	// Start up states (OggPlay startup has several stages)
	enum TStartUpState
	{ EAppUiConstruct, EActivateSplashView, EStartOggPlay, EActivateStartUpView,
	  EPostConstruct, EStartRunningEmbedded, EStartUpComplete, EStartUpFailed };

	// The views supported by the listbox
	enum TViews
	{ ETitle, EAlbum, EArtist, EGenre, ESubFolder, EFileName, EPlayList, ETop, ETrackTitle };

public:
	COggPlayAppUi();
	~COggPlayAppUi();

	void ConstructL();
	void NextStartUpState(TInt aErr);

	// from MPlaybackObserver
	void NotifyUpdate();
	void NotifyPlayComplete();
	void NotifyPlayInterrupted();
	void ResumeUpdates();

#if !defined(PLUGIN_SYSTEM)
	void NotifyStreamOpen(TInt aErr);
#endif

#if defined(DELAY_AUDIO_STREAMING_OPEN)
	void NotifyFileOpen(TInt aErr);
#endif

#if defined(DELAY_AUDIO_STREAMING_START)
	void NotifyPlayStarted();
#endif

	void NotifyFatalPlayError();

	// From MFileInfoObserver
	void FileInfoCallback(TInt aErr, const TOggFileInfo& aFileInfo);

	// High level functions
	void PlaySelect();
	void PauseResume();
	void Pause();
	void Stop();
	void NextSong();
	void PreviousSong();

	void ShowFileInfo();

	void SelectPreviousView();
	void SelectNextView();

#if defined(UIQ)
	void ActivateOggView();
#endif

	void ActivateOggViewL();
	void ActivateOggViewL(const TUid aViewId);

	void UpdateSoftkeys(TBool aForce=EFalse);
  
	// from CEikAppUi
	void HandleCommandL(TInt aCommand);
	void HandleForegroundEventL(TBool aForeground);
	void DynInitMenuPaneL(TInt aMenuId, CEikMenuPane* aMenuPane);
	TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName, const TDesC8& aTail);

	void OpenFileL(const TDesC& aFileName);
	TInt FileSize(const TDesC& aFileName);
	void WriteIniFile();
	void SetRandomL(TBool aRandom);
	void SetRepeat(TBool aRepeat);
	void ToggleRepeat();
	void ToggleRandomL();
  
#if defined(SERIES80)
	CEikButtonGroupContainer* Cba()  
	{ return CEikonEnv::Static()->AppUiFactory()->ToolBar();} 
#endif

	void SetVolumeGainL(TGainType aNewGain);

	void SetBufferingModeL(TBufferingMode aNewBufferingMode);
	void SetThreadPriority(TStreamingThreadPriority aNewThreadPriority);

	void SetAlarm(TBool aAlarmActive);
	void SetAlarmTime();

	TInt Rnd(TInt aMax);
	void FileOpenErrorL(TInt aErr);
	const TDesC& UnassignedTxt()
	{ return iUnassignedTxt; }

private:
	void StartOggPlay();
	void StartOggPlayL();

	void ActivateStartUpView();
	void ActivateStartUpViewL();

	void PostConstruct();
	void PostConstructL();

	void StartRunningEmbedded();
	void RunningEmbeddedDbReady(TInt aErr);

	void StartUpError(TInt aErr);
	void NextStartUpState();

	void ReadIniFile();
	void ReadIniFileL(TPtrC8& aIniFileData);
	void ReadSkin(TInt aSkin);

	TInt IniRawRead32(TPtrC8& aDes);
	TInt IniRead32L(TPtrC8& aDes);
	TInt IniRead32L(TPtrC8& aDes, TInt aLowerLimit, TInt aUpperLimit);
	TInt64 IniRead64L(TPtrC8& aDes);
	TInt64 IniRead64L(TPtrC8& aDes, TInt64 aLowerLimit, TInt64 aUpperLimit);
	void IniReadDesL(TPtrC8& aDes, TDes& value);

#if defined(UIQ)
	void SetHotKey();
#endif

	void FindSkins();

#if defined(SERIES60SUI)
    // From CAknAppUi
	void HandleResourceChangeL(TInt aType); 
#endif

public:
	// Global settings stored in the ini file
	TOggplaySettings iSettings;

	// backwards compatibility - deprecated
	// as long as these are not part of TOggplaySettings, they can't be set 
	// through the settings view
	TInt iHotkey;
	TInt iVolume;            // [0...100]
	TInt iAnalyzerState;     // 0= off; 1=on; 2= on with peaks
	TViews iViewBy;

	TFileName iDbFileName;
	TFileName iIniFileName;
	TFileName iFileName;

	COggPlayAppView* iAppView;
	CAbsPlayback* iOggPlayback;

	COggMsgEnv* iOggMsgEnv;
	COggSongList* iSongList;

private:
#if defined(MONITOR_TELEPHONE_LINE)
	COggActive* iActive;
#endif

	TBool iIsRunningEmbedded; // is true when application got started through the recognizer
	TInt iTryResume; // after iTryResume seconds try resuming music (after sound device was stolen)

	TInt iCapturedKeyHandle;
	TFileName iSkinFileDir;
	TInt iCurrentSkin;
	CDesCArray* iSkins;

	COggSplashView* iSplashFOView;
	COggFOView* iFOView;

	COggSplashView* iSplashFCView;
	COggFCView* iFCView;

#if defined(SERIES60)
	COggSettingsView* iSettingsView;
	COggUserHotkeysView* iUserHotkeysView;

#if defined(PLUGIN_SYSTEM)
	COggPluginSettingsView* iCodecSelectionView;
#endif

	COggPlaybackOptionsView* iPlaybackOptionsView;
	COggAlarmSettingsView* iAlarmSettingsView;
#endif

	RArray<TInt> iViewHistoryStack;
	RArray<TInt> iRestoreStack;
	TInt iRestoreCurrent;

	TBool iForeground;

	COggStartUpAO* iStartUpAO;
	COggStartUpEmbeddedAO* iStartUpEmbeddedAO;

#if defined(UIQ)
	COggViewActivationAO* iOggViewActivationAO;
#endif

	TBuf<128> iUnassignedTxt;
	TBuf<128> iStartUpErrorTxt1;
	TBuf<128> iStartUpErrorTxt2;

	TInt64 iRndSeed;
	TInt64 iRndMax;

#if defined(SERIES60)
	CAknErrorNote* iStartUpErrorDlg;
#endif

#if defined(SERIES60SUI)
	// CFbsTypefaceStore* iTypefaceStore;
	// TInt iFontId;
#endif

public:
	TStartUpState iStartUpState;

	friend class COggActive;
	friend class COggStartUpAO;
	friend class COggStartUpEmbeddedAO;
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


// COggSongList
// Base class defining the interface for managing the song list.
class TOggFile;
class COggSongList : public CBase
{
public:
    ~COggSongList();

    const TDesC& GetPlaying();
    const TOggFile* GetPlayingFile();
    TBool AnySongPlaying();
    void  SetRepeat(TBool aRepeat);
    TBool IsSelectedFromListBoxCurrentlyPlaying();

	virtual const TDesC& GetNextSong()=0;    
    virtual const TDesC& GetPreviousSong()=0;
	virtual void  SetPlayingFromListBox(TInt aPlaying);

protected:
	COggSongList(COggPlayAppView* aAppView);
    void ConstructL(COggSongList* aSongList);

	void SetPlaying(TInt aPlaying);
	void AddFile(TOggFile* aFile, TInt aPlayingIdx);

protected:
	TInt iPlayingIdx; // Index of the file currently being played (maybe a playlist)
	RPointerArray<TOggFile> iFileList;

	TInt iFullListPlayingIdx; // Index of the actual audio file being played
    RPointerArray<TOggFile> iFullFileList;

    TBool iRepeat;

private:
	COggPlayAppView* iAppView; 
};

class COggNormalPlay : public COggSongList
{
public:     
	static COggNormalPlay* NewL(COggPlayAppView* aAppView, COggSongList* aSongList = NULL);
	~COggNormalPlay();

	// From COggSongList
	const TDesC& GetNextSong();  
	const TDesC& GetPreviousSong();

private:
    COggNormalPlay(COggPlayAppView* aAppView);
};

class COggRandomPlay : public COggSongList
{
public:
	static COggRandomPlay* NewL(COggPlayAppUi* aAppUi, COggPlayAppView* aAppView, COggSongList* aSongList = NULL);
	~COggRandomPlay();

	// From COggSongList
	const TDesC& GetNextSong();
	const TDesC& GetPreviousSong();
	void SetPlayingFromListBox(TInt aPlaying);

private:
	COggRandomPlay(COggPlayAppUi* aAppUi, COggPlayAppView* aAppView);
    void ConstructL(COggSongList* aSongList);

	void GenerateRandomListL();

private:
	COggPlayAppUi* iAppUi;

    RArray<TInt> iRandomMemory;
	TInt iRandomMemoryIdx;
};


#if defined(UIQ)
class COggPlayDocument : public CQikDocument
#else
class COggPlayDocument : public CEikDocument
#endif
	{
public:
	COggPlayDocument(CEikApplication& aApp);

	CFileStore* OpenFileL(TBool aDoOpen, const TDesC& aFilename, RFs& aFs);
	CEikAppUi* CreateAppUiL();
	};

#if defined(SERIES60)
class COggPlayApplication : public CAknApplication
#elif defined(UIQ)
class COggPlayApplication : public CQikApplication
#else
class COggPlayApplication : public CEikApplication
#endif
	{
private: 
	// from CApaApplication
	CApaDocument* CreateDocumentL();
	TUid AppDllUid() const;
	};

#endif
