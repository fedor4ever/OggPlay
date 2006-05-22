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
#include <e32std.h>
#include <hal.h>
#ifdef SERIES60
#include <akndialog.h>   // for about
_LIT(KTsyName,"phonetsy.tsy");
#endif

#ifdef SERIES80
#include <eikEnv.h>
#include "OggDialogsS80.h"
_LIT(KTsyName,"phonetsy.tsy");
#endif

#ifdef UIQ
#include <quartzkeys.h>	// EStdQuartzKeyConfirm etc.
_LIT(KTsyName,"erigsm.tsy");
#endif

#include <eiklabel.h>   // CEikLabel
#include <gulicon.h>	// CGulIcon
#include <apgwgnam.h>	// CApaWindowGroupName
#include <eikmenup.h>	// CEikMenuPane
#include <barsread.h>

#include "OggControls.h"
#ifdef PLUGIN_SYSTEM
#include "OggPluginAdaptor.h"
#else
#include "OggTremor.h"
#endif
#include "OggPlayAppView.h"
#include "OggDialogs.h" 
#include "OggLog.h"

#include "OggFilesSearchDialogs.h"

#if defined(UIQ)
const TInt KThreadPriority = EPriorityAbsoluteHigh;
#else
const TInt KThreadPriority = EPriorityAbsoluteBackground;
#endif

// Application entry point
#if defined(SERIES60V3)
#include <eikstart.h>
static CApaApplication* NewApplication()
{
	return new COggPlayApplication;
}

GLDEF_C TInt E32Main()
{
	return EikStart::RunApplication(NewApplication);
}
#else
EXPORT_C CApaApplication* NewApplication()
{
	return new COggPlayApplication;
}

GLDEF_C TInt E32Dll(TDllReason)
{
	return KErrNone;
}
#endif

void ROggPlayListStack::Reset()
{
	iPlayListArray.Reset();
	iPlayingIdxArray.Reset();
}

TInt ROggPlayListStack::Push(const TOggPlayListStackEntry& aPlayListStackEntry)
{
	TInt err = iPlayListArray.Append(aPlayListStackEntry.iPlayList);
	if (err != KErrNone)
		return err;

	err = iPlayingIdxArray.Append(aPlayListStackEntry.iPlayingIdx);
	if (err != KErrNone)
		iPlayListArray.Remove(iPlayListArray.Count()-1);

	return err;
}

void ROggPlayListStack::Pop(TOggPlayListStackEntry& aPlayListStackEntry)
{
	aPlayListStackEntry.iPlayList = NULL;
	aPlayListStackEntry.iPlayingIdx = -1;

	TInt lastIndex = iPlayListArray.Count()-1;
	if (lastIndex>=0)
	{
		aPlayListStackEntry.iPlayList = iPlayListArray[lastIndex];
		aPlayListStackEntry.iPlayingIdx = iPlayingIdxArray[lastIndex];

		iPlayListArray.Remove(lastIndex);
		iPlayingIdxArray.Remove(lastIndex);
	}
}

void ROggPlayListStack::Close()
{
	iPlayListArray.Close();
	iPlayingIdxArray.Close();
}


// COggActive class
#ifdef MONITOR_TELEPHONE_LINE
COggActive::COggActive()
{
}

void COggActive::ConstructL(COggPlayAppUi* theAppUi)
{
	iAppUi = theAppUi;
	
#if !defined(__WINS__)  
	// The destructor panics in emulator
	iServer= new (ELeave) RTelServer;
	User::LeaveIfError(iServer->Connect());
	User::LeaveIfError(iServer->LoadPhoneModule(KTsyName));
	
	TInt nPhones;
	User::LeaveIfError(iServer->EnumeratePhones(nPhones));
	
	RTelServer::TPhoneInfo pInfo;
	User::LeaveIfError(iServer->GetPhoneInfo(0, pInfo));
	
	iPhone= new (ELeave) RPhone;
	User::LeaveIfError(iPhone->Open(*iServer, pInfo.iName));
	
	TInt nofLines;
	User::LeaveIfError(iPhone->EnumerateLines(nofLines));
	
	iLine= new (ELeave) RLine;
	
#if defined(UIQ)
	TInt linesToTest = 1;
	iLineToMonitor = 0;
	iLineCallsReportedAtIdle = 0;
#else
	// Series 60 1.X reports 3 lines 0='Fax' 1='Data' and 2='VoiceLine1' 
    // Series 60 2.0 reports 4 lines 0='Voice1' 1='Voice2' 2='Data' 3='Fax'
	// (and 'VoiceLine1' reports '1' in EnumerateCall() when in idle ?!?)

	TInt linesToTest = nofLines;
	
	// S60: Why is 'iLineCallsReportedAtIdle' not '0' at idle as 
	// should be expected ??? The voice line reports '1' ???
	RPhone::TLineInfo lInfo;
	for (TInt i=0; i<linesToTest; i++) 
	  {
		  User::LeaveIfError(iPhone->GetLineInfo(i, lInfo)); 

		  // Test code, left here because this is likely to change between
		  // phone and it's nice to keep it in traces.
		  User::LeaveIfError(iLine->Open(*iPhone, lInfo.iName));
		  TInt nCalls=-1;
		  iLine->EnumerateCall(nCalls);
		  iLine->Close();
		  TRACEF(COggLog::VA(_L("Line%d:%S=%d"), i, &lInfo.iName, nCalls ));
		  // End of test code

		  if (lInfo.iName.Match(_L("Voice*1")) != KErrNotFound)
		  {      
		    iLineToMonitor = i;
		    iLineCallsReportedAtIdle = nCalls;
		  }
	  }
#endif
#endif

#if defined(SERIES60) || defined(SERIES80)
  iTimer = CPeriodic::New(CActive::EPriorityStandard);
  iCallBack = new (ELeave) TCallBack(COggActive::CallBack, this);
#endif
}

TInt COggActive::CallBack(TAny* aPtr)
{
  COggActive* self= (COggActive*) aPtr;
  TInt callBackAgain = self->CallBack();
  if (!callBackAgain)
	  self->CancelCallBack();

  return callBackAgain;
}

TInt COggActive::CallBack()
{	
  if (iAppUi->iIsStartup)
    iAppUi->PostConstructL();

  iAppUi->NotifyUpdate();

#if !defined(__WINS__)  
  RPhone::TLineInfo lInfo;		
  iPhone->GetLineInfo(iLineToMonitor, lInfo);
  iLine->Open(*iPhone, lInfo.iName);
  
  TInt nCalls=0;
  iLine->EnumerateCall(nCalls);
  iLine->Close();
		
  TBool isRinging = nCalls > iLineCallsReportedAtIdle;
  TBool isIdle    = nCalls == iLineCallsReportedAtIdle;
  if (isRinging && !iInterrupted) 
  {
	// the phone is ringing or someone is making a call, pause the music if any
	if (iAppUi->iOggPlayback->State()==CAbsPlayback::EPlaying)
	{
		TRACELF("GSM is active");
		iInterrupted= ETrue;
		iAppUi->HandleCommandL(EOggPauseResume);

		// Continue monitoring
		return 1;
	}
  }
  else if (iInterrupted) 
  {
	// our music was interrupted by a phone call, now what
	if (isIdle)
	{
		TRACELF("GSM is idle");

		// okay, the phone is idle again, let's continue with the music
		iInterrupted= EFalse;
		if (iAppUi->iOggPlayback->State()==CAbsPlayback::EPaused)
			iAppUi->HandleCommandL(EOggPauseResume);
	}
  }
#endif

  // Continue monitoring if we have been interrupted or if we are now playing
  return iInterrupted || (iAppUi->iOggPlayback->State()==CAbsPlayback::EPlaying) || iAppUi->iTryResume;
}

void COggActive::IssueRequest()
{
#if defined(SERIES60) || defined(SERIES80)
  iTimer->Cancel();
  iTimer->Start(TTimeIntervalMicroSeconds32(1000000), TTimeIntervalMicroSeconds32(1000000), *iCallBack);
#endif
}

void COggActive::CancelCallBack()
{
	iTimer->Cancel();
}

COggActive::~COggActive()
{
  if (iLine)
  { 
	iLine->Close();
	delete iLine; 
  }

  if (iPhone)
  { 
	iPhone->Close();
	delete iPhone; 
  }

  if (iServer)
  { 
	iServer->Close();
	delete iServer; 
  }

#if defined(SERIES60) || defined(SERIES80)
  if (iTimer)
  {
	iTimer->Cancel();
	delete iTimer;
  }

  delete iCallBack;
#endif
}
#endif // MONITOR_TELEPHONE_LINE

////////////////////////////////////////////////////////////////
//
// App UI class, COggPlayAppUi
COggPlayAppUi::COggPlayAppUi()
: iViewBy(ETop), iIsStartup(ETrue)
{
}

void COggPlayAppUi::ConstructL()
{
#if defined(SERIES60_SPLASH)
  ShowSplash();

  // Otherwise BaseConstructL() clears screen when launched via recognizer (?!)
  CEikonEnv::Static()->RootWin().SetOrdinalPosition(0,ECoeWinPriorityNeverAtFront);
#endif
	
  BaseConstructL();

  TFileName fileName(Application()->AppFullName());
  TParsePtr parse(fileName);

#if defined(SERIES60V3)
  TFileName privatePath;
  User::LeaveIfError(iCoeEnv->FsSession().PrivatePath(privatePath));

  iIniFileName.Copy(parse.Drive());
  iIniFileName.Append(privatePath);
  iIniFileName.Append(_L("OggPlay.ini"));
 
  iDbFileName.Copy(parse.Drive());
  iDbFileName.Append(privatePath);
  iDbFileName.Append(_L("OggPlay.db"));

  iSkinFileDir.Copy(parse.Drive());
  iSkinFileDir.Append(privatePath);
  iSkinFileDir.Append(_L("import\\"));
#else
  iIniFileName.Copy(fileName);
  iIniFileName.SetLength(iIniFileName.Length() - 3);
  iIniFileName.Append(_L("ini"));

  iDbFileName.Copy(fileName);
  iDbFileName.SetLength(iDbFileName.Length() - 3);
  iDbFileName.Append(_L("db"));

  iSkinFileDir.Copy(parse.DriveAndPath());
#endif

#if defined(__WINS__)
  // The emulator doesn't like to write to the Z: drive, avoid that.
  iIniFileName = _L("C:\\oggplay.ini");
  iDbFileName = _L("C:\\oggplay.db");
#endif

	iSkins= new(ELeave) CDesCArrayFlat(3);
	FindSkins();
	
	iOggMsgEnv = new(ELeave) COggMsgEnv(iSettings);

#ifdef PLUGIN_SYSTEM
	iOggPlayback= new(ELeave) COggPluginAdaptor(iOggMsgEnv, this);
#else
	iOggPlayback= new(ELeave) COggPlayback(iOggMsgEnv, this);
#endif
	
	iOggPlayback->ConstructL();

	ReadIniFile();
	
	iAppView=new(ELeave) COggPlayAppView;
	iAppView->ConstructL(this, ClientRect());

	SetRandomL(iSettings.iRandom);
	SetRepeat(iSettings.iRepeat);

	HandleCommandL(EOggSkinOne+iCurrentSkin);
	
	AddToStackL(iAppView); // Receiving Keyboard Events 
	
	iAppView->InitView();
	
	SetHotKey();
	iIsRunningEmbedded = EFalse;
	
#ifdef MONITOR_TELEPHONE_LINE
	iActive= new (ELeave) COggActive();
	iActive->ConstructL(this);
	iActive->IssueRequest();
#endif
	
	if (iAppView->IsFlipOpen()) {
		// Create and activate the view
		iFOView=new(ELeave) COggFOView(*iAppView);
		RegisterViewL(*iFOView);  
		iFCView=new(ELeave) COggFCView(*iAppView);
		RegisterViewL(*iFCView);
	}
	else {
		iFCView=new(ELeave) COggFCView(*iAppView);
		RegisterViewL(*iFCView);
		// Create and activate the view
		iFOView=new(ELeave) COggFOView(*iAppView);
		RegisterViewL(*iFOView);
	}

#if defined(SERIES60)
	iSettingsView=new(ELeave) COggSettingsView(*iAppView,KOggPlayUidSettingsView);
	RegisterViewL(*iSettingsView);

	iUserHotkeysView = new(ELeave) COggUserHotkeysView(*iAppView);
	RegisterViewL(*iUserHotkeysView);

#ifdef SERIES60_SPLASH_WINDOW_SERVER
    iSplashView=new(ELeave) COggSplashView(*iAppView);
    RegisterViewL(*iSplashView);
#endif /*SERIES60_SPLASH_WINDOW_SERVER*/

#ifdef PLUGIN_SYSTEM
    iCodecSelectionView=new(ELeave) COggPluginSettingsView(*iAppView);
    RegisterViewL(*iCodecSelectionView);
#endif

#if defined(MULTI_THREAD_PLAYBACK)
	iPlaybackOptionsView = new(ELeave) COggPlaybackOptionsView(*iAppView, KOggPlayUidPlaybackOptionsView);
	RegisterViewL(*iPlaybackOptionsView);
#endif

	iAlarmSettingsView = new(ELeave) COggAlarmSettingsView(*iAppView, KOggPlayUidAlarmSettingsView);
	RegisterViewL(*iAlarmSettingsView);
#endif /* SERIES60 */
        
#if defined(MULTI_THREAD_PLAYBACK)
	// Resist any attempts by the OS to give us anything other than foreground priority
	// Disable process priority changes
	CEikonEnv::Static()->WsSession().ComputeMode(RWsSession::EPriorityControlDisabled);

	// Change the process priority to EPriorityForeground
	RProcess process;
	process.SetPriority(EPriorityForeground);

#else
	SetProcessPriority();
	SetThreadPriority();
#endif
	

#if defined(SERIES60_SPLASH_WINDOW_SERVER)
	ActivateOggViewL(KOggPlayUidSplashView);
#else
	ActivateOggViewL();
#endif

#if defined(SERIES60_SPLASH)
	iEikonEnv->RootWin().SetOrdinalPosition(0,ECoeWinPriorityNormal);
#endif
}

void COggPlayAppUi::PostConstructL()
{
  if(iSettings.iAutoplay) {
    for(TInt i=0;i<iRestoreStack.Count();i++) {
      TRACEF(COggLog::VA(_L("Setting istack[%d]:%d"), i,iRestoreStack[i]));
      iAppView->SelectItem(iRestoreStack[i]);
      HandleCommandL(EOggPlay);
    }
    iAppView->SelectItem(iRestoreCurrent);
  }

  if ( iIsRunningEmbedded || (iSettings.iAutoplay && iIsStartup) 
      && iOggPlayback->State()!=CAbsPlayback::EPlaying ) {
		iIsStartup=EFalse; // this is not redundant - needed if there's an error in NextSong()
    NextSong();
	}
  if ( iIsRunningEmbedded )
  {
      // No repeat when running in embedded mode, override the default iSettings.iRepeat
      iSongList->SetRepeat(EFalse);
  }

  iIsStartup=EFalse;
  iIsRunningEmbedded=EFalse;
}

COggPlayAppUi::~COggPlayAppUi()
{
	if(iAppView) RemoveFromStack(iAppView);
	
	delete iAppView;
	if (iFOView) {
		DeregisterView(*iFOView);
		delete iFOView;
	}
	if (iFCView) {
		DeregisterView(*iFCView);
		delete iFCView;
	}
#if defined(SERIES60)
	if (iSettingsView) {
		DeregisterView(*iSettingsView);
  	delete iSettingsView;
	}

  if( iUserHotkeysView ) {
    DeregisterView(*iUserHotkeysView);
    delete iUserHotkeysView;
    }
#ifdef SERIES60_SPLASH_WINDOW_SERVER
  if (iSplashView) {
    DeregisterView(*iSplashView);
    delete iSplashView;
  }
#endif

#ifdef PLUGIN_SYSTEM
  if (iCodecSelectionView) {
    DeregisterView(*iCodecSelectionView);
    delete iCodecSelectionView;
  }
#endif

#if defined(MULTI_THREAD_PLAYBACK)
	if (iPlaybackOptionsView)
	{
		DeregisterView(*iPlaybackOptionsView);
  		delete iPlaybackOptionsView;
	}
#endif

	if (iAlarmSettingsView)
	{
		DeregisterView(*iAlarmSettingsView);
  		delete iAlarmSettingsView;
	}
#endif /* SERIES60 */

#ifdef MONITOR_TELEPHONE_LINE
	delete iActive;
#endif

	delete iOggPlayback;
	iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);
	
	delete iSkins;
	delete iOggMsgEnv ;
    delete iSongList;
    iViewHistoryStack.Close();
    iRestoreStack.Close();
	COggLog::Exit();  
}


void COggPlayAppUi::ActivateOggViewL()
{
  TUid iViewUid;
  iViewUid = iAppView->IsFlipOpen() ? KOggPlayUidFOView : KOggPlayUidFCView;
  ActivateOggViewL(iViewUid);
}

void COggPlayAppUi::ActivateOggViewL(const TUid aViewId)
{
	TVwsViewId viewId;
	viewId.iAppUid = KOggPlayUid;
	viewId.iViewUid = aViewId;
	
	TRAPD(err, ActivateViewL(viewId));
	if (err) ActivateTopViewL();
}

void COggPlayAppUi::HandleApplicationSpecificEventL(TInt aType, const TWsEvent& aEvent)
{
	if (aType == EEventUser+1) {
		ActivateViewL(TVwsViewId(KOggPlayUid, KOggPlayUidFOView));
	}
	else 
		CEikAppUi::HandleApplicationSpecificEventL(aType, aEvent);
}

void
COggPlayAppUi::NotifyUpdate()
{
	// Try to resume after sound device was stolen
	if (iTryResume>0 && iOggPlayback->State()==CAbsPlayback::EPaused)
	{
		if (iOggPlayback->State()!=CAbsPlayback::EPaused)
			iTryResume= 0;
		else
		{
			iTryResume--;
			if (iTryResume==0 && iOggPlayback->State()==CAbsPlayback::EPaused) HandleCommandL(EOggPauseResume);
		}
	}
}

void
COggPlayAppUi::NotifyPlayInterrupted()
{
	HandleCommandL(EOggPauseResume);
	iTryResume= 2;
}

void
COggPlayAppUi::NotifyPlayComplete()
{
	if (iSongList->AnySongPlaying() <0) {
        // Does this really happens?
		iAppView->Update();
		return;
	}
	
    NextSong();
}

#if defined(DELAY_AUDIO_STREAMING_START)
void COggPlayAppUi::NotifyPlayStarted()
{
	// Update the appView and the softkeys
	iAppView->Update();
	UpdateSoftkeys();
}
#endif

void COggPlayAppUi::ResumeUpdates()
{
#if defined(MONITOR_TELEPHONE_LINE)
	if (iActive)
		iActive->IssueRequest();
#endif
}

#if defined(MULTI_THREAD_PLAYBACK)
void COggPlayAppUi::NotifyFatalPlayError()
{
	// Try to exit cleanly
	HandleCommandL(EEikCmdExit);
}
#endif

void
COggPlayAppUi::SetHotKey()
{
	if(iCapturedKeyHandle)
		iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);
	iCapturedKeyHandle = 0;
	if(!iHotkey || !keycodes[iHotkey-1].code)
		return;
#if !defined(SERIES60)
	iCapturedKeyHandle = iEikonEnv->RootWin().CaptureKey(keycodes[iHotkey-1].code,
		keycodes[iHotkey-1].mask, keycodes[iHotkey-1].mask, 2);
#else
	iCapturedKeyHandle = iEikonEnv->RootWin().CaptureKey(keycodes[iHotkey-1].code,
		keycodes[iHotkey-1].mask, keycodes[iHotkey-1].mask);
#endif
}

#if defined(SERIES60) || defined(SERIES80) 
void COggPlayAppUi::UpdateSoftkeys(TBool aForce)
#else
void COggPlayAppUi::UpdateSoftkeys(TBool /* aForce */)
#endif
{
#if defined(SERIES60) || defined(SERIES80) 
  // Return if somebody else than main view has taken focus, 
  // except if we override this check.
  // Others must control the CBA themselves
  TVwsViewId viewId;
  GetActiveViewId(viewId);
  if( !aForce && viewId.iViewUid != KOggPlayUidFOView ) 
    return;
  
#ifdef SERIES60
  if(iOggPlayback->State() == CAbsPlayback::EPlaying) {
    COggUserHotkeysS60::SetSoftkeys(ETrue);
  } else {
    COggUserHotkeysS60::SetSoftkeys(EFalse);
  }
#else
  if(iOggPlayback->State() == CAbsPlayback::EPlaying) {
    COggUserHotkeysS80::SetSoftkeys(ETrue);
  } else {
    COggUserHotkeysS80::SetSoftkeys(EFalse);
  }
#endif
  Cba()->DrawNow();
#endif //S60||S80
}


void
COggPlayAppUi::HandleCommandL(int aCommand)
{
  switch (aCommand)
  {
  case EOggAbout:
	{
		COggAboutDialog *d = new(ELeave) COggAboutDialog();
		TBuf<128> buf;
		iEikonEnv->ReadResource(buf, R_OGG_VERSION);
		d->SetVersion(buf);
		d->ExecuteLD(R_DIALOG_ABOUT);
		break;
	}

	case EOggPlay: {
    PlaySelect();
		break;
				   }
		
	case EOggStop: {
    Stop();
		break;
				   }
		
	case EOggPauseResume: {
    PauseResume();
		break;
						  }
		
	case EOggInfo: {
		ShowFileInfo();
		break;
				   }
		
	case EOggShuffle: {     
		SetRandomL(!iSettings.iRandom);
		break;
					  }
		
	case EOggRepeat: {
	    ToggleRepeat();
		break;
					 }
		
	case EOggOptions: {

#if defined(UIQ)
		CHotkeyDialog *hk = new(ELeave) CHotkeyDialog(&iHotkey, &(iSettings.iAlarmActive), &(iSettings.iAlarmTime));
		if (hk->ExecuteLD(R_DIALOG_HOTKEY)==EEikBidOk) {
			SetHotKey();
			iAppView->SetAlarm();
		}
#elif defined(SERIES80)
        CSettingsS80Dialog *hk = new(ELeave) CSettingsS80Dialog(&iSettings);
		hk->ExecuteLD(R_DIALOG_OPTIONS) ;
		UpdateSoftkeys(EFalse);
#elif defined(SERIES60)
        ActivateOggViewL(KOggPlayUidSettingsView);
#endif
		break;
					  }

#ifdef PLUGIN_SYSTEM
    case EOggCodecSelection:
        {
#if defined(SERIES80)
		CCodecsS80Dialog *cd = new(ELeave) CCodecsS80Dialog();
		cd->ExecuteLD(R_DIALOG_CODECS);
#else
         ActivateOggViewL(KOggPlayUidCodecSelectionView);
#endif
		break;
        }
#endif

#if defined(MULTI_THREAD_PLAYBACK)
    case EOggPlaybackOptions:
		ActivateOggViewL(KOggPlayUidPlaybackOptionsView);
		break;
#endif

#if defined(SERIES60)
	case EOggAlarmSettings:
		ActivateOggViewL(KOggPlayUidAlarmSettingsView);
		break;
#endif

	case EOggNextSong: {
		NextSong();
		break;
					   }
		
	case EOggPrevSong: {
		PreviousSong();
		break;
					   }
		
	case EOggViewByTitle:
	case EOggViewByAlbum:
	case EOggViewByArtist:
	case EOggViewByGenre:
	case EOggViewBySubFolder:
	case EOggViewByFileName:
	case EOggViewByPlayList:
        {
			TBuf<16> dummy;
			//iViewBy= aCommand-EOggViewByTitle;
			iAppView->FillView((TViews)(aCommand-EOggViewByTitle), ETop, dummy);
			break;
		}
		
	case EOggViewRebuild: {
		HandleCommandL(EOggStop);
		iIsRunningEmbedded = EFalse;
#ifdef SEARCH_OGGS_FROM_ROOT
        COggFilesSearchDialog *d = new(ELeave) COggFilesSearchDialog(iAppView->iOggFiles);
        if(iSettings.iScanmode==TOggplaySettings::EMmcOgg) {
          iAppView->iOggFiles->SearchSingleDrive(KMmcSearchDir,d,R_DIALOG_FILES_SEARCH,iCoeEnv->FsSession());
        } 
#ifdef SERIES80
		else if(iSettings.iScanmode==TOggplaySettings::ECustomDir) {
          iAppView->iOggFiles->SearchSingleDrive(iSettings.iCustomScanDir,d,R_DIALOG_FILES_SEARCH,iCoeEnv->FsSession());
        } 
#endif
        else {
          iAppView->iOggFiles->SearchAllDrives(d,R_DIALOG_FILES_SEARCH,iCoeEnv->FsSession());
        }

#else
		iAppView->iOggFiles->CreateDb(iCoeEnv->FsSession());
#endif
		iAppView->iOggFiles->WriteDbL(iDbFileName, iCoeEnv->FsSession());
		TBuf<16> dummy;
		iAppView->FillView(ETop, ETop, dummy);
		iAppView->Invalidate();
		break;
						  }
		
	case EOggSkinOne:
	case EOggSkinTwo:
	case EOggSkinThree:
	case EOggSkinFour:
	case EOggSkinFive:
	case EOggSkinSix:
	case EOggSkinSeven:
	case EOggSkinEight:
	case EOggSkinNine:
	case EOggSkinTen: {
		iCurrentSkin= aCommand-EOggSkinOne;
		TBuf<256> buf(iSkinFileDir);
		buf.Append((*iSkins)[iCurrentSkin]);

		iAppView->ReadSkin(buf);
		iAppView->Update();
		iAppView->Invalidate();
		break;
	  }
		
#if defined(SERIES60) || defined(SERIES80)
#if defined(SERIES60)
  case EAknSoftkeyBack:
  {
	HandleCommandL(EOggStop);
	WriteIniFile();
    Exit();
    break;
  }
 #endif

  case EUserStopPlayingCBA : {
 		HandleCommandL(EOggStop);
    break;
    }
  case EUserPauseCBA : {
 		iOggPlayback->Pause(); // does NOT call UpdateS60Softkeys
		iAppView->Update();
    UpdateSoftkeys();

    break;
    }
  case EUserPlayCBA : {
 		HandleCommandL(EOggPlay);
		iAppView->Update();
    break;
    }

  case EUserBackCBA : {
 		SelectPreviousView();
    break;
    }

  case EOggUserHotkeys :
    ActivateOggViewL(KOggPlayUidUserView);
    break;

  case EUserHotKeyCBABack: {
    ActivateOggViewL(KOggPlayUidFOView);
    }
    break;
    
  case EUserFastForward : {
    TInt64 pos= iOggPlayback->Position() + KFfRwdStep;
    iOggPlayback->SetPosition(pos);
	iAppView->UpdateSongPosition();
    }
    break;
  case EUserRewind : {
    TInt64 pos= iOggPlayback->Position() - KFfRwdStep;
    iOggPlayback->SetPosition(pos);
	iAppView->UpdateSongPosition();
    }
    break;
  case EUserListBoxPageDown :
  	iAppView->ListBoxPageDown();
    break;
  case EUserListBoxPageUp:
  	iAppView->ListBoxPageUp();
    break;
  case EUserVolumeHelp:  {
    TBuf<256> buf;
	iEikonEnv->ReadResource(buf, R_OGG_STRING_14);
	User::InfoPrint(buf);
	break;
  }

  case EVolumeBoostUp:
	  {
		TGainType currentGain = (TGainType) iSettings.iGainType;
		TInt newGain = currentGain + 1;
		if (newGain<=EStatic12dB)
			SetVolumeGainL((TGainType) newGain);
		break;
	  }

  case EVolumeBoostDown:
	  {
	  	TGainType currentGain = (TGainType) iSettings.iGainType;
		TInt newGain = currentGain - 1;
		if (newGain>=EMinus24dB)
			SetVolumeGainL((TGainType) newGain);
		break;
	  }
#endif

	case EEikCmdExit: {
		HandleCommandL(EOggStop);
		WriteIniFile();
		Exit();
		break;
    }
  }
}

void
COggPlayAppUi::PlaySelect()
{
	TInt idx = iAppView->GetSelectedIndex();

	if (iViewBy==ETop) {
		SelectNextView();
    return;
	}

	if (iAppView->GetItemType(idx)==COggListBox::EBack) {
		// the back item was selected: show the previous view
		SelectPreviousView();
		return;
	}

#ifdef PLAYLIST_SUPPORT
	if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy == EPlayList) 
#else
	if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) 
#endif
	{
		SelectNextView();
		return;
	}

  if (iViewBy==ETitle || iViewBy==EFileName)  {
        
        if ( (iOggPlayback->State()==CAbsPlayback::EPaused) &&
             (iSongList->IsSelectedFromListBoxCurrentlyPlaying() ) )
        {
            iOggPlayback->Resume();

			#if !defined(DELAY_AUDIO_STREAMING_START)
            iAppView->Update();
            UpdateSoftkeys();
			#endif
            return;
        }

        if ((iOggPlayback->State()==CAbsPlayback::EPaused) || (iOggPlayback->State()==CAbsPlayback::EPlaying))
		{
            iOggPlayback->Stop();
            UpdateSoftkeys();
        }
        
        iSongList->SetPlayingFromListBox(idx);
        if (iOggPlayback->Open(iSongList->GetPlaying())==KErrNone)
		{
            iOggPlayback->SetVolume(iVolume);
            iOggPlayback->Play();

			#if !defined(DELAY_AUDIO_STREAMING_START)
            iAppView->SetTime(iOggPlayback->Time());
            iAppView->Update();
            UpdateSoftkeys();
			#endif
        } else {
            iSongList->SetPlayingFromListBox(ENoFileSelected);
            UpdateSoftkeys();
        }
        
    }
    return;
}

void COggPlayAppUi::PauseResume()
{
    
  TRACEF(_L("PauseResume"));
  if (iOggPlayback->State()==CAbsPlayback::EPlaying)
  {
    iOggPlayback->Pause();
    iAppView->Update();
    UpdateSoftkeys();
  }
  else if  (iOggPlayback->State()==CAbsPlayback::EPaused)
  {
	  iOggPlayback->Resume();

	  #if !defined(DELAY_AUDIO_STREAMING_START)
	  iAppView->Update();
      UpdateSoftkeys();
	  #endif
  }
  else
    PlaySelect();
}

void COggPlayAppUi::Pause()
{
  if (iOggPlayback->State()==CAbsPlayback::EPlaying)
  {
    iOggPlayback->Pause();
    iAppView->Update();
    UpdateSoftkeys();
  }
}

void COggPlayAppUi::Stop()
{
  if (iOggPlayback->State()==CAbsPlayback::EPlaying ||
      iOggPlayback->State()==CAbsPlayback::EPaused) {
    iOggPlayback->Stop();
    UpdateSoftkeys();
    iSongList->SetPlayingFromListBox(ENoFileSelected); // calls iAppview->Update
  }
}


void
COggPlayAppUi::SelectPreviousView()
{
  if(iViewHistoryStack.Count()==0) return;

#if defined(SERIES60) || defined(SERIES80)
  TInt previousListboxLine = (TInt&) (iViewHistoryStack[iViewHistoryStack.Count()-1]);
#endif

  iViewHistoryStack.Remove(iViewHistoryStack.Count()-1);

  const TInt previousView= iAppView->GetViewName(0);
  if (previousView==ETop) {
	  //iViewBy= ETop;
	  TBuf<16> dummy;
	  iAppView->FillView(ETop, ETop, dummy);
  }
  else 
	  HandleCommandL(EOggViewByTitle+previousView);

  #if defined(SERIES60) || defined(SERIES80)
  // UIQ_?
  // Select the entry which were left.
  iAppView->SelectItem(previousListboxLine);
  #endif

  return;
}

void
COggPlayAppUi::SelectNextView()
{
  int idx = iAppView->GetSelectedIndex();
  if (iViewBy==ETop) {
#ifdef PLAYLIST_SUPPORT
	  if (idx>=ETitle && idx<=EPlayList){
#else
	  if (idx>=ETitle && idx<=EFileName){
#endif
			  iViewHistoryStack.Append(idx);
			  HandleCommandL(EOggViewByTitle+idx);
		  }
		  return;
	}
		
#ifdef PLAYLIST_SUPPORT
  if (!(iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==EPlayList)) return;
#else
  if (!(iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder)) return;
#endif

    if (iAppView->GetItemType(idx) == COggListBox::EBack)
     return;
    
    iViewHistoryStack.Append(idx);

    TBuf<128> filter;
    iAppView->GetFilterData(idx, filter);
    if ((iViewBy==EPlayList) || (iViewBy==ESubFolder))
		iAppView->FillView(EFileName, iViewBy, filter);
	else
		iAppView->FillView(ETitle, iViewBy, filter);
}

void
COggPlayAppUi::ShowFileInfo()
{
    TBool songPlaying = iSongList->AnySongPlaying();
	TInt selectedIndex = iAppView->GetSelectedIndex();
	if (!songPlaying && (selectedIndex<0)) return;

	COggListBox::TItemTypes itemType;
	if (!songPlaying) {
		// no song is playing, show info for selected song if possible
		itemType = iAppView->GetItemType(selectedIndex);
		if (itemType==COggListBox::EBack) return;
		if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==ETop) return;

		if (itemType != COggListBox::EPlayList)
		{
			const TDesC& fileName = iAppView->GetFileName(selectedIndex);
			if (iOggPlayback->Info(fileName)!=KErrNone)
				return;
		}
	}
	else
		itemType = (iSongList->GetPlayingFile()->FileType() == TOggFile::EPlayList) ? COggListBox::EPlayList : COggListBox::EFileName;

	if (itemType == COggListBox::EPlayList)
	{
		TOggPlayList* playList;
		if (!songPlaying)
			playList = (TOggPlayList*) iAppView->GetFile(selectedIndex);
		else
			playList = (TOggPlayList*) iSongList->GetPlayingFile();

		COggPlayListInfoDialog *d = new COggPlayListInfoDialog();
		const TDesC& fileName = *(playList->iFileName);
		d->SetFileName(fileName);
		d->SetFileSize(FileSize(fileName));
		d->SetPlayListEntries(playList->Count());

		d->ExecuteLD(R_DIALOG_PLAYLIST_INFO);
	}
	else
	{
		TInt fileSize;
		if (songPlaying)
			fileSize = iOggPlayback->FileSize();
		else
			fileSize = FileSize(iOggPlayback->FileName());

		COggInfoDialog *d = new COggInfoDialog();
		d->SetFileName(iOggPlayback->FileName());
		d->SetRate(iOggPlayback->Rate());
		d->SetChannels(iOggPlayback->Channels());
		d->SetFileSize(fileSize);

#if defined(SERIES60V3)
		d->SetTime(iOggPlayback->Time());
#else
		d->SetTime(iOggPlayback->Time().GetTInt());
#endif

		d->SetBitRate(iOggPlayback->BitRate()/1000);
		if (!songPlaying) iOggPlayback->ClearComments();

		d->ExecuteLD(R_DIALOG_INFO);
	}
}


void
COggPlayAppUi::NextSong()
{
  // This function guarantees that a next song will be played.
  // This is important since it is also being used if an alarm is triggered.
  // If neccessary the current view will be switched.	
  if ( iSongList->AnySongPlaying() ) 
    {
        // if a song is currently playing, find and play the next song
        const TDesC &songName = iSongList->GetNextSong();
#ifndef PLUGIN_SYSTEM
        iOggPlayback->Stop();
#endif
        if (songName.Length()>0 && iOggPlayback->Open(songName)==KErrNone)
		{
            iOggPlayback->Play();

			#if !defined(DELAY_AUDIO_STREAMING_START)
            iAppView->SetTime(iOggPlayback->Time());
			UpdateSoftkeys();
            iAppView->Update();
			#endif
        } else {
            iSongList->SetPlayingFromListBox(ENoFileSelected);
            HandleCommandL(EOggStop);
        }
    } else {
        // Switch view 
        if (iViewBy==ETop) {
            HandleCommandL(EOggPlay); // select the current category
            HandleCommandL(EOggPlay); // select the 1st song in that category
            HandleCommandL(EOggPlay); // play it!
        } else if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) {
            HandleCommandL(EOggPlay); // select the 1st song in the current category
            HandleCommandL(EOggPlay); // play it!
        } else {
            // if no song is playing, play the currently selected song
			if (iAppView->GetItemType(iAppView->GetSelectedIndex())==COggListBox::EBack) iAppView->SelectItem(1);
            HandleCommandL(EOggPlay); // play it!
        } 
	}
}

void
COggPlayAppUi::PreviousSong()
{
    if ( iSongList->AnySongPlaying() ) 
    {
        // if a song is currently playing, find and play the previous song
        const TDesC &songName = iSongList->GetPreviousSong();
#ifndef PLUGIN_SYSTEM
        iOggPlayback->Stop();
#endif
        if (songName.Length()>0 && iOggPlayback->Open(songName)==KErrNone)
		{
            iOggPlayback->Play();

			#if !defined(DELAY_AUDIO_STREAMING_START)
			iAppView->SetTime(iOggPlayback->Time());
            UpdateSoftkeys();
            iAppView->Update();
			#endif
        } else {
            iSongList->SetPlayingFromListBox(ENoFileSelected);
            HandleCommandL(EOggStop);
        }
    }
    else
    {
        if (iViewBy==ETop) {
            HandleCommandL(EOggPlay); // select the current category
            HandleCommandL(EOggPlay); // select the 1st song in that category
            HandleCommandL(EOggPlay); // play it!
        } else if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) {
            HandleCommandL(EOggPlay); // select the 1st song in the current category
            HandleCommandL(EOggPlay); // play it!
        } else {
            // if no song is playing, play the currently selected song
			if (iAppView->GetItemType(iAppView->GetSelectedIndex())==COggListBox::EBack) iAppView->SelectItem(1);
            HandleCommandL(EOggPlay); // play it!
        }
	}
}

void
COggPlayAppUi::DynInitMenuPaneL(int aMenuId, CEikMenuPane* aMenuPane)
{
	if (aMenuId==R_FILE_MENU) 
	{
        // "Repeat" on/off entry, UIQ uses check box, Series 60 uses variable argument string.
#if defined(UIQ)
        if (iSettings.iRepeat) 
            aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);
      // UIQ_? for the random stuff
#elif defined(SERIES60)
    // FIXIT - Should perhaps be in the options menu instead ??
    TBuf<50> buf;
	iEikonEnv->ReadResource(buf, iSettings.iRandom ? R_OGG_RANDOM_OFF : R_OGG_RANDOM_ON);
    aMenuPane->SetItemTextL(EOggShuffle, buf);
#endif
		TBool isSongList= ((iViewBy==ETitle) || (iViewBy==EFileName) || (iViewBy == EPlayList));
		aMenuPane->SetItemDimmed(EOggInfo   , (!iSongList->AnySongPlaying()) && (iAppView->GetSelectedIndex()<0 || !isSongList));
	}
	
	if (aMenuId==R_SKIN_MENU) {
		for (TInt i=0; i<iSkins->Count(); i++) {
			CEikMenuPaneItem::SData item;
			item.iText.Copy((*iSkins)[i]);
			item.iText.SetLength(item.iText.Length()-4);
			item.iCommandId= EOggSkinOne+i;
			item.iCascadeId= 0;
			item.iFlags= 0;
			aMenuPane->AddMenuItemL(item);
		}
	}

#ifdef UIQ
	if (aMenuId==R_POPUP_MENU) {
		if (iSettings.iRepeat) aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);
		aMenuPane->SetItemDimmed(EOggStop, !iAppView->CanStop());
		aMenuPane->SetItemDimmed(EOggPlay, !iAppView->CanPlay());
		aMenuPane->SetItemDimmed(EOggPauseResume, !iAppView->CanPause());
	}
#endif
}

void COggPlayAppUi::FindSkins()
{
	iSkins->Reset();
		
	CDirScan* ds = CDirScan::NewLC(iCoeEnv->FsSession());
	TRAPD(err,ds->SetScanDataL(iSkinFileDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
	if (err!=KErrNone) {
		//_LIT(KS,"Error in FindSkins-SetScanDataL");
		//OGGLOG.WriteFormat(KS);
		
		CleanupStack::PopAndDestroy(ds);
		return;
	}

	CDir* c = NULL;	
	TFileName fullname;
	for(;;) {
		ds->NextL(c);
		if (!c)
			break;
		
		for (TInt i=0; i<c->Count(); i++) 
		{
			const TEntry e= (*c)[i];
			
			fullname.Copy(ds->FullPath());
			fullname.Append(e.iName);

			TParsePtrC p(fullname);			
			if (p.Ext()==_L(".skn") || p.Ext()==_L(".SKN")) {
				iSkins->AppendL(p.NameAndExt());
				if (iSkins->Count()==10) break;
			}
		}
		delete c; c=NULL;
    }
	
	CleanupStack::PopAndDestroy(ds);
}

void
COggPlayAppUi::ReadIniFile()
{
    // Set some default values (for first time users)
#if defined(SERIES60)
	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EHotKeyExit;
    iSettings.iSoftKeysPlay[0] = TOggplaySettings::EHotKeyExit;
#else
#if defined(SERIES80)
	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EPlay;
    iSettings.iSoftKeysIdle[1] = TOggplaySettings::EHotKeyBack;
    iSettings.iSoftKeysIdle[2] = TOggplaySettings::EHotkeyVolumeHelp;
    iSettings.iSoftKeysIdle[3] = TOggplaySettings::EHotKeyExit;
    iSettings.iSoftKeysPlay[0] = TOggplaySettings::EPause;
    iSettings.iSoftKeysPlay[1] = TOggplaySettings::ENextSong;
    iSettings.iSoftKeysPlay[2] = TOggplaySettings::EFastForward;
    iSettings.iSoftKeysPlay[3] = TOggplaySettings::EHotKeyExit;
	iSettings.iCustomScanDir = KFullScanString;
#endif
#endif
 
    iSettings.iWarningsEnabled = ETrue;
	iSettings.iGainType = ENoGain;
	iVolume = KMaxVolume;

	iSettings.iAlarmTime.Set(_L("20030101:120000.000000"));
	iSettings.iAlarmVolume = 7;
	iSettings.iAlarmGain = ENoGain;
	iSettings.iAlarmSnooze = 1;

#if (defined(SERIES60 ) || defined (SERIES80) )	
    iAnalyzerState = EDecay; 
#endif

#if defined(MULTI_THREAD_PLAYBACK)
	iSettings.iBufferingMode = ENoBuffering;
	iSettings.iThreadPriority = ENormal;
#endif

    // Open the file
    RFile in;
    TInt err = in.Open(iCoeEnv->FsSession(), iIniFileName, EFileRead | EFileStreamText);    
    if (err != KErrNone)
    {
        TRACEF(COggLog::VA(_L("ReadIni:%d"), err ));
        return;
    }
	
	TFileText tf;
	tf.Set(in);

   // Read in the fields
   TInt ini_version = 0;

   TInt val = (TInt) IniRead32( tf );
   if ( val == 0xdb ) // Our magic number ! (see writeini below)
   {
      // Followed by version number
      ini_version = (TInt) IniRead32( tf );
      TRACEF(COggLog::VA(_L("Inifile version %d"), ini_version ));
   }
   else
   {
      // Not the magic number so this must be an old version
      // Seek back to beginning of the file and start parsing from there
      tf.Seek( ESeekStart );
    }
	
   iHotkey = (int)   IniRead32( tf, 0, KMaxKeyCodes );
   iSettings.iRepeat = (TBool) IniRead32( tf, 1, 1 );
   iVolume = (int)   IniRead32( tf, KMaxVolume, KMaxVolume );
	
   TInt64 tmp64 = IniRead64( tf );
   TTime t(tmp64);
   iSettings.iAlarmTime= t;

   iAnalyzerState = (int)  IniRead32( tf, 0, 3 );
   val            =        IniRead32( tf );  // For backward compatibility
   iCurrentSkin   = (TInt) IniRead32( tf, 0, (iSkins->Count() - 1) );
   
   if ( ini_version >= 2 )
   {
      TInt restore_stack_count = (TInt) IniRead32( tf );      
      for( TInt i = 0; i < restore_stack_count; i++ )
      {
         iRestoreStack.Append( (TInt) IniRead32( tf ) );
	}
	
      iRestoreCurrent            = (TInt) IniRead32( tf );
      iSettings.iScanmode        = (TInt) IniRead32( tf );
#ifdef SERIES80      
      IniReadDes( tf ,iSettings.iCustomScanDir,KMmcSearchDir);
#endif
      iSettings.iAutoplay        = (TInt) IniRead32( tf );
      iSettings.iManeuvringSpeed = (TInt) IniRead32( tf );
      
      // For backwards compatibility for number of hotkeys
      TInt num_of_hotkeys = 0;
      if (ini_version<7) 
      {
          num_of_hotkeys = ( ini_version >= 5 ) ? TOggplaySettings::ENofHotkeysV5 : TOggplaySettings::ENofHotkeysV4;
      }
      else 
      { 
      	  num_of_hotkeys = (TInt)  IniRead32( tf );
      }
 
      for ( TInt j = TOggplaySettings::KFirstHotkeyIndex; j < num_of_hotkeys; j++ ) 
      {
         iSettings.iUserHotkeys[j] = (TInt) IniRead32( tf );
      }
      
      iSettings.iWarningsEnabled      = (TInt)  IniRead32( tf );
      if (ini_version<7) 
        {
            // The way the softkeys is stored has changed, ignore the value
            // from the ini file.
            IniRead32( tf ); 
        	IniRead32( tf );
        	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EHotKeyExit;
        	iSettings.iSoftKeysPlay[0] = TOggplaySettings::EHotKeyExit;
        }
        else
        {
      		iSettings.iSoftKeysIdle[0]      = (TInt)  IniRead32( tf );
      		iSettings.iSoftKeysPlay[0]      = (TInt)  IniRead32( tf );
        }
#ifdef SERIES80
      iSettings.iSoftKeysIdle[1]      = (TInt)  IniRead32( tf );
      iSettings.iSoftKeysPlay[1]      = (TInt)  IniRead32( tf );
      iSettings.iSoftKeysIdle[2]      = (TInt)  IniRead32( tf );
      iSettings.iSoftKeysPlay[2]      = (TInt)  IniRead32( tf );
      iSettings.iSoftKeysIdle[3]      = (TInt)  IniRead32( tf );
      iSettings.iSoftKeysPlay[3]      = (TInt)  IniRead32( tf );
 #endif
      iSettings.iRandom                    = (TBool) IniRead32( tf, 0, 1 );

   } // version 2 onwards

#ifdef PLUGIN_SYSTEM
    TInt nbController = IniRead32(tf) ;
    TBuf<10> extension;
    TInt32 uid;
    for ( TInt j=0; j<nbController; j++ )
    {
        if ( tf.Read(extension) != KErrNone)
            extension = KNullDesC;
        uid = IniRead32(tf);
        TUid aUid;
        aUid.iUid = uid;
        TRAPD(newermind,
        iOggPlayback->GetPluginListL().SelectPluginL(extension, aUid); )
        
        TRACEF(COggLog::VA(_L("Looking for controller %S:%x Result:%i"), &extension, uid, newermind));
    }
#endif

	if (ini_version>=7)
	{
		iSettings.iGainType = IniRead32(tf);
		iOggPlayback->SetVolumeGain((TGainType) iSettings.iGainType);
	}

#if defined(MULTI_THREAD_PLAYBACK)
	if (ini_version>=8)
	{
		iSettings.iBufferingMode = IniRead32(tf);
		TInt err = ((COggPlayback*) iOggPlayback)->SetBufferingMode((TBufferingMode) iSettings.iBufferingMode);
		if (err != KErrNone)
			iSettings.iBufferingMode = ENoBuffering;

		iSettings.iThreadPriority = IniRead32(tf);
		((COggPlayback*) iOggPlayback)->SetThreadPriority((TStreamingThreadPriority) iSettings.iThreadPriority);
	}
#endif

	if (ini_version>=11)
		{
		iSettings.iAlarmActive = IniRead32(tf);
		iSettings.iAlarmVolume = IniRead32(tf);
		iSettings.iAlarmGain = IniRead32(tf);
		iSettings.iAlarmSnooze = IniRead32(tf);
		}

   in.Close();
	}

TInt32 
COggPlayAppUi::IniRead32( TFileText& aFile, TInt32 aDefault, TInt32 aMaxValue )
{
   TInt32   val = aDefault;
   TBuf<128> line;

   if ( aFile.Read(line) == KErrNone )
   {
		TLex parse(line);
      if ( parse.Val(val) == KErrNone )
      {
         if ( val > aMaxValue ) val = aDefault;
	  }
      else
      {
         val = aDefault;
      }
    }

   return ( val );
	  }
  
TInt64 
COggPlayAppUi::IniRead64( TFileText& aFile, TInt64 aDefault )
{
   TInt64   val = aDefault;
   TBuf<128> line;

   if ( aFile.Read(line) == KErrNone )
   {
		TLex parse(line);
      if ( parse.Val(val) != KErrNone )
    {
         val = aDefault;
      }
  	}

   return ( val );
}
#ifdef SERIES80
void COggPlayAppUi::IniReadDes( TFileText& aFile, TDes& value,const TDesC& defaultValue )
{
   TBuf<255> line;

   if ( aFile.Read(line) == KErrNone )
   {
		   value.Copy(line);
		   
    } else 
    {
    	   value.Copy(defaultValue);
    	   
    }
}
#endif

void
COggPlayAppUi::WriteIniFile()
{
    RFile out;
    
    TRACE(COggLog::VA(_L("COggPlayAppUi::WriteIniFile() %S"), &iIniFileName ));
    
    // Accessing the MMC , using the TFileText is extremely slow. 
    // We'll do everything using the C:\ drive then move the file to it's final destination
    // in MMC.
    
    TParsePtrC p(iIniFileName);
	TBool useTemporaryFile = EFalse;
	TFileName fileName = iIniFileName;
    if (p.Drive() != _L("C:"))
    {
    	useTemporaryFile = ETrue;
    	// Create the c:\tmp\ directory
        TInt errorCode = iCoeEnv->FsSession().MkDir(_L("C:\\tmp\\"));
        if ((errorCode != KErrNone) && (errorCode !=  KErrAlreadyExists) )
          return;
          
        // Create the file
    	fileName = _L("C:\\tmp\\");
    	fileName.Append(p.NameAndExt());
    }
    
    if ( out.Replace( iCoeEnv->FsSession(), fileName, EFileWrite | EFileStreamText) != KErrNone ) return;
    
    TRACEF(_L("Writing Inifile..."));
    TInt j;
    TFileText tf;
    tf.Set(out);
    
    TBuf<64> num;
    
    // this should do the trick for forward compatibility:
    TInt magic=0xdb;
    TInt iniversion=11;

    num.Num(magic);
    tf.Write(num);
    
    num.Num(iniversion);
    tf.Write(num);

	num.Num(iHotkey);
	tf.Write(num);
	
	num.Num(iSettings.iRepeat);
	tf.Write(num);

	num.Num(iVolume);
	tf.Write(num);

	num.Num(iSettings.iAlarmTime.Int64());
	tf.Write(num);
	
	num.Num(iAnalyzerState);
	tf.Write(num);
	
	// for backward compatibility:
	num.Num(iAnalyzerState);
	tf.Write(num);
	
	num.Num(iCurrentSkin);
	tf.Write(num);
	
    // from iniversion 2 onwards:
    num.Num(iViewHistoryStack.Count());
    tf.Write(num);
    
    for ( TInt i = 0; i < iViewHistoryStack.Count(); i++ )
    {
        num.Num(iViewHistoryStack[i]);
        TRACEF(COggLog::VA(_L("iViewHistoryStack[%d]= %d"),i,iViewHistoryStack[i]));
        tf.Write(num);
    }
    
    num.Num(iAppView->GetSelectedIndex());
	tf.Write(num);
	
	num.Num(iSettings.iScanmode);
	tf.Write(num);
#ifdef SERIES80	
	tf.Write(iSettings.iCustomScanDir);
#endif
	num.Num(iSettings.iAutoplay);
	tf.Write(num);

 	num.Num(iSettings.iManeuvringSpeed);
	tf.Write(num);
    
    num.Num((TUint) TOggplaySettings::ENofHotkeys);
    tf.Write(num);
    
    for( j=TOggplaySettings::KFirstHotkeyIndex; j<TOggplaySettings::ENofHotkeys; j++ ) 
    {
        num.Num(iSettings.iUserHotkeys[j]);
        tf.Write(num);
    }
    
    num.Num(iSettings.iWarningsEnabled);
    tf.Write(num);
    
    for( j=0; j<KNofSoftkeys; j++ ) 
    {
        num.Num(iSettings.iSoftKeysIdle[j]);
        tf.Write(num);
    
        num.Num(iSettings.iSoftKeysPlay[j]);
        tf.Write(num);
    }
    
    num.Num(iSettings.iRandom);
    tf.Write(num);

#ifdef PLUGIN_SYSTEM
    CDesCArrayFlat * supportedExtensionList = iOggPlayback->GetPluginListL().SupportedExtensions();
    TRAPD(err,
    {
        CleanupStack::PushL(supportedExtensionList);
        
        num.Num(supportedExtensionList->Count());
        tf.Write(num);
        for ( j=0; j<supportedExtensionList->Count(); j++ )
        {
            tf.Write( (*supportedExtensionList)[j]);
            CPluginInfo * selected = iOggPlayback->GetPluginListL().GetSelectedPluginInfo((*supportedExtensionList)[j]);
            if (selected)
            	num.Num((TInt)selected->iControllerUid.iUid) ;
            else
            	num.Num(0);
            tf.Write(num);
        }
        
        CleanupStack::PopAndDestroy(1);
    } 
    ) // End of TRAP
#endif
    
    num.Num(iSettings.iGainType);
    tf.Write(num);

#if defined(MULTI_THREAD_PLAYBACK)
    num.Num(iSettings.iBufferingMode);
    tf.Write(num);

	num.Num(iSettings.iThreadPriority);
    tf.Write(num);
#endif

	num.Num(iSettings.iAlarmActive);
	tf.Write(num);

	num.Num(iSettings.iAlarmVolume);
	tf.Write(num);

	num.Num(iSettings.iAlarmGain);
	tf.Write(num);

	num.Num(iSettings.iAlarmSnooze);
	tf.Write(num);

	// Please increase ini_version when adding stuff
	
	out.Close();
	if (useTemporaryFile) 
	{
		// Move the file to the MMC
		CFileMan* fileMan = NULL;
	    TRAPD(err2, fileMan = CFileMan::NewL(iCoeEnv->FsSession()));
		if (err2 == KErrNone)
		{
			fileMan->Move( fileName, iIniFileName,CFileMan::EOverWrite);
			delete (fileMan);
		}
	}

     TRACEF(_L("Writing Inifile 10..."));
}

void
COggPlayAppUi::HandleForegroundEventL(TBool aForeground)
{
	iForeground = aForeground;
	CEikAppUi::HandleForegroundEventL(aForeground);

	if (aForeground)
		iAppView->RestartCallBack();
	else
		iAppView->StopCallBack();
}

void COggPlayAppUi::ToggleRandom()
{
	if (iSettings.iRandom)
	      SetRandomL(EFalse);
	    else
	      SetRandomL(ETrue);
}

void
COggPlayAppUi::SetRandomL(TBool aRandom)
{
    // Toggle random
    iSettings.iRandom = aRandom;
	COggSongList* songList;
    if (iSettings.iRandom)
		songList = COggRandomPlay::NewL(iAppView, iOggPlayback, iSongList);
    else
		songList = COggNormalPlay::NewL(iAppView, iOggPlayback, iSongList);

	delete iSongList;
	iSongList = songList;

    iSongList->SetRepeat(iSettings.iRepeat);
    iAppView->UpdateRandom();
}

void 
COggPlayAppUi::ToggleRepeat()
{
	if (iSettings.iRepeat)
	      SetRepeat(EFalse);
	    else
	      SetRepeat(ETrue);
}

void
COggPlayAppUi::SetRepeat(TBool aRepeat)
{
    iSettings.iRepeat = aRepeat;

    iSongList->SetRepeat(aRepeat);
    iAppView->UpdateRepeat();
}

#if defined(SERIES60_SPLASH)
void 
COggPlayAppUi::ShowSplash()
{
  // This is a so-called pre Symbian OS v6.1 direct screen drawing routine
  // based on CFbsScreenDevice. The inherent problem with this method is that
  // the window server is unaware of the direct screen access and is likely
  // to 'refresh' whatever invalidated regions it might think it manages....
	
  // http://www.symbian.com/developer/techlib/v70docs/SDL_v7.0/doc_source/
  //   ... BasePorting/PortingTheBase/BitgdiAndGraphics/HowBitGdiWorks.guide.html
	
  User::LeaveIfError(RFbsSession::Connect());

  // Check if there is a splash mbm available, if not splash() will make a silent leave
  TFileName fileName(Application()->AppFullName());
  TParsePtr parse(fileName);

#if defined(SERIES60V3)
  TFileName privatePath;
  User::LeaveIfError(iCoeEnv->FsSession().PrivatePath(privatePath));

  fileName.Copy(parse.Drive());
  fileName.Append(privatePath);
  fileName.Append(_L("import\\s60splash.mbm"));
#else
  fileName.Copy(parse.DriveAndPath());
  fileName.Append(_L("s60splash.mbm"));
#endif
	  
  CFbsBitmap* bitmap = new (ELeave) CFbsBitmap;
  CleanupStack::PushL(bitmap);
  User::LeaveIfError(bitmap->Load(fileName, 0, EFalse));
    
  CFbsScreenDevice *device = NULL;
  TDisplayMode dispMode = CCoeEnv::Static()->ScreenDevice()->DisplayMode();
  device = CFbsScreenDevice::NewL(_L("scdv"), dispMode);
  CleanupStack::PushL(device);
	  
  CFbsBitGc *gc = NULL;
  User::LeaveIfError(device->CreateContext(gc));
  CleanupStack::PushL(gc);
  gc->BitBlt(TPoint(0, 0), bitmap);

  device->Update();
  CleanupStack::PopAndDestroy(3, bitmap);

  RFbsSession::Disconnect();
}
#endif // #if defined(SERIES60_SPLASH)


#if !defined(MULTI_THREAD_PLAYBACK)
void COggPlayAppUi::SetProcessPriority()
{
#if !defined(__WINS__) //FIXFIXME
	CEikonEnv::Static()->WsSession().ComputeMode(RWsSession::EPriorityControlDisabled);
	RProcess P;
	TFindProcess fp(_L("OggPlay*"));
	TFullName fn;
	if (fp.Next(fn)==KErrNone) {
		int err= P.Open(fn);
		if (err==KErrNone) {
			P.SetPriority(EPriorityHigh);
			P.Close();
		} else {
			TBuf<256> buf;
			iEikonEnv->ReadResource(buf, R_OGG_ERROR_3);
			buf.AppendNum(err);
			User::InfoPrint(buf);
		}
	}
	else {
		TBuf<256> buf;
		iEikonEnv->ReadResource(buf, R_OGG_ERROR_4);
		User::InfoPrint(buf);
	}
#endif
}

void COggPlayAppUi::SetThreadPriority()
{
#if !defined(__WINS__) //FIXFIXME
	//CEikonEnv::Static()->WsSession().ComputeMode(RWsSession::EPriorityControlDisabled);
	RThread T;
	TFindThread ft(_L("OggPlay*"));
	TFullName fn;
	if (ft.Next(fn)==KErrNone) {
		int err= T.Open(fn);
		if (err==KErrNone) {
#if defined(MOTOROLA)
			T.SetPriority(EPriorityAbsoluteForeground);
#else
			T.SetPriority(EPriorityAbsoluteHigh);
#endif
			T.Close();
		} else {
			TBuf<256> buf;
			iEikonEnv->ReadResource(buf, R_OGG_ERROR_5);
			buf.AppendNum(err);
			User::InfoPrint(buf);
		}
	}
	else {
		TBuf<256> buf;
		iEikonEnv->ReadResource(buf, R_OGG_ERROR_6);
		User::InfoPrint(buf);
	}
#endif
}
#endif

TBool COggPlayAppUi::ProcessCommandParametersL(TApaCommand /*aCommand*/, TFileName& /*aDocumentName*/,const TDesC8& /*aTail*/)
{
    return ETrue;
}


void COggPlayAppUi::OpenFileL(const TDesC& aFileName){
	
	if (iAppView->iOggFiles->CreateDbWithSingleFile(aFileName)) {
		iAppView->SelectItem(0);
		iIsRunningEmbedded = ETrue;
	}
	if (iDoorObserver) {
		iDoorObserver->NotifyExit(MApaEmbeddedDocObserver::ENoChanges); 
	}

}

TInt COggPlayAppUi::FileSize(const TDesC& aFileName)
{
  RFile file;
  TInt err = file.Open(iCoeEnv->FsSession(), aFileName, EFileShareReadersOnly);
  if (err != KErrNone)
	  return 0;

  TInt fileSize(0);
  file.Size(fileSize);
  file.Close();
  return fileSize; 
}

void COggPlayAppUi::SetVolumeGainL(TGainType aNewGain)
{
	iOggPlayback->SetVolumeGain(aNewGain);
	iSettings.iGainType = aNewGain;

#if defined(SERIES60)
  #if defined(MULTI_THREAD_PLAYBACK)
	iPlaybackOptionsView->VolumeGainChangedL();
  #else
	iSettingsView->VolumeGainChangedL();
  #endif
#endif
}

#if defined(MULTI_THREAD_PLAYBACK)
void COggPlayAppUi::SetBufferingModeL(TBufferingMode aNewBufferingMode)
{
	TInt err = ((COggPlayback *) iOggPlayback)->SetBufferingMode(aNewBufferingMode);
	if (err == KErrNone)
		iSettings.iBufferingMode = aNewBufferingMode;
	else
	{
		// Pop up a message (reset the value in the playback options too)
		TBuf<256> buf, tbuf;
		iEikonEnv->ReadResource(tbuf, R_OGG_ERROR_20);
		iEikonEnv->ReadResource(buf, R_OGG_ERROR_29);		
		iOggMsgEnv->OggErrorMsgL(tbuf, buf);		

		iPlaybackOptionsView->BufferingModeChangedL();
	}
}

void COggPlayAppUi::SetThreadPriority(TStreamingThreadPriority aNewThreadPriority)
{
	((COggPlayback *) iOggPlayback)->SetThreadPriority(aNewThreadPriority);
	iSettings.iThreadPriority = aNewThreadPriority;
}
#endif

void COggPlayAppUi::SetAlarm(TBool aAlarmActive)
{
	iSettings.iAlarmActive = aAlarmActive;
	iAppView->SetAlarm();
}

void COggPlayAppUi::SetAlarmTime()
{
	iAppView->SetAlarm();
}

////////////////////////////////////////////////////////////////
//
// COggPlayDocument class
//
///////////////////////////////////////////////////////////////
#if defined(SERIES60)
CFileStore* COggPlayDocument::OpenFileL(TBool /*aDoOpen*/,const TDesC& aFilename, RFs& /*aFs*/)
#else
CFileStore* COggPlayDocument::OpenFileL(TBool /*aDoOpen*/,const TDesC& /* aFilename */, RFs& /*aFs*/)
#endif
{
#if defined(SERIES60)
	iAppUi->OpenFileL(aFilename);
#endif

	return NULL;
}

CEikAppUi* COggPlayDocument::CreateAppUiL(){
    iAppUi = new (ELeave) COggPlayAppUi;
    return iAppUi;	
}


// COggSongList class
COggSongList::COggSongList(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback)
: iAppView(aAppView), iOggPlayback(aOggPlayback)
{
}

void COggSongList::ConstructL(COggSongList* aSongList)
{
	if (aSongList)
	{
		// Copy the state from the other song list
		RPointerArray<TOggFile>& fileList = aSongList->iFileList;
		TInt count = fileList.Count();
		for (TInt i = 0 ; i<count ; i++)
			iFileList.Append(fileList[i]);

		iPlayingIdx = aSongList->iPlayingIdx;
		iNewFileList = aSongList->iNewFileList;
	}
	else
	{
		iPlayingIdx = ENoFileSelected;
		iNewFileList = EFalse;
	}
}

COggSongList::~COggSongList()
{
  iFileList.Close();
  iPlayListStack.Close();
}

void
COggSongList::SetPlayingFromListBox(TInt aPlaying)
{
    // Change the currently played song
    // It is possible that the current list is not the one on the list box on screen 
    // (User has navigated while playing tune).

    // Update the file list, according to what is displayed in the list box.
    iFileList.Reset();
    iNewFileList = ETrue;
    
    iPlayingIdx = ENoFileSelected;

    for (TInt i=0; i<iAppView->GetTextArray()->Count(); i++) {
        if (iAppView->HasAFileName(i))
        {
            // We're dealing with a file, not the "back" button or something similar
			iFileList.Append(iAppView->GetFile(i));
            if (i == aPlaying)
                iPlayingIdx = iFileList.Count()-1;
        }
    }
    SetPlaying(iPlayingIdx);
}

void 
COggSongList::SetPlaying(TInt aPlaying, TBool aPreviousSong)
{   
    iPlayingIdx = aPlaying;
    if (aPlaying != ENoFileSelected)
    {
		TOggFile* file = iFileList[aPlaying];
		if (file->FileType() == TOggFile::EPlayList)
		{
			// Playing a new playlist, so reset the playlist stack and set iPlaylist, iPlayListIdx
			iPlayListStack.Reset();
			
			iPlayList = (TOggPlayList*) file;
			if (aPreviousSong)
				iPlayListIdx = iPlayList->Count()-1;
			else
				iPlayListIdx = 0;

			file = (*iPlayList)[iPlayListIdx];
			while (file->FileType() == TOggFile::EPlayList)
			{
				// Another playlist. Push the current playlist onto the stack and get the next file
				TOggPlayListStackEntry playListStackEntry(iPlayList, iPlayListIdx);
				TInt err = iPlayListStack.Push(playListStackEntry);
				if (err != KErrNone)
				{
					iPlayListStack.Reset();
					return;
				}

				iPlayList = (TOggPlayList*) file;
				if (aPreviousSong)
					iPlayListIdx = iPlayList->Count()-1;
				else
					iPlayListIdx = 0;

				file = (*iPlayList)[iPlayListIdx];
			}
		}
		else
			iPlayList = NULL;
    }

    if (iPlayingIdx != ENoFileSelected)
		{
        iAppView->SelectFile(iFileList[iPlayingIdx]);

		#if !defined(DELAY_AUDIO_STREAMING_START)
		iAppView->Update();
		#endif
		}
	else
		iAppView->Update();
}

const TOggFile*
COggSongList::GetPlayingFile()
{
    if (iPlayingIdx != ENoFileSelected)
        return ( iFileList[iPlayingIdx] );

	return(NULL);
}

const TDesC& 
COggSongList::GetPlaying()
{
    if (iPlayingIdx == ENoFileSelected)
        return KNullDesC;

	TOggFile* playingFile = iFileList[iPlayingIdx];
	if (playingFile->FileType() == TOggFile::EPlayList)
		playingFile = (*iPlayList)[iPlayListIdx];

    return(*(playingFile->iFileName));
}

const TBool 
COggSongList::AnySongPlaying()
{
    if (iPlayingIdx == ENoFileSelected)
        return EFalse;
    else
        return ETrue;
}

const TBool
COggSongList::IsSelectedFromListBoxCurrentlyPlaying()
{
    TInt idx = iAppView->GetSelectedIndex();
    if (iAppView->HasAFileName(idx))
    {
        if ( GetPlayingFile() == iAppView->GetFile(idx) )
        {
            return (ETrue);
        }
    }
    return(EFalse);
}

void  
COggSongList::SetRepeat(TBool aRepeat)
{
    iRepeat = aRepeat;
}


// COggNormalPlay class
COggNormalPlay* COggNormalPlay::NewL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback, COggSongList* aSongList)
{
	COggNormalPlay* self = new(ELeave) COggNormalPlay(aAppView, aOggPlayback);
	CleanupStack::PushL(self);
	self->ConstructL(aSongList);

	CleanupStack::Pop(self);
	return self;
}

COggNormalPlay::COggNormalPlay(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback)
: COggSongList(aAppView, aOggPlayback)
{
}

COggNormalPlay::~COggNormalPlay()
{
}
   
const TDesC& COggNormalPlay::GetNextSong()
{
    TInt nSongs= iFileList.Count();
    if ( (iPlayingIdx == ENoFileSelected) || (nSongs <=0) )
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
	}

	TOggFile* file = iFileList[iPlayingIdx];
	if (file->FileType() == TOggFile::EPlayList)
	{
		// We are playing a playlist, so get the next file from the playlist
		TInt nextFile = iPlayListIdx+1;
		if (nextFile<iPlayList->Count())
		{
			iPlayListIdx++;
			file = (*iPlayList)[iPlayListIdx];

			while (file->FileType() == TOggFile::EPlayList)
			{
				// Another playlist. Push the current playlist onto the stack and get the next file
				TOggPlayListStackEntry playListStackEntry(iPlayList, iPlayListIdx);
				TInt err = iPlayListStack.Push(playListStackEntry);
				if (err != KErrNone)
				{
					iPlayListStack.Reset();
					return KNullDesC;
				}

				iPlayList = (TOggPlayList*) file;
				iPlayListIdx = 0;

				file = (*iPlayList)[iPlayListIdx];
			}

	        // Check that the file is still there (media hasn't been removed)
		    if (iOggPlayback->Info(*(file->iFileName), ETrue) == KErrOggFileNotFound)
			    iPlayingIdx = ENoFileSelected;

			if (iPlayingIdx == ENoFileSelected)
				return (KNullDesC);

			return *(file->iFileName);
		}
		else
		{
			// End of current playlist, pop the previous playlist from the stack
			TOggPlayListStackEntry playListStackEntry;
			iPlayListStack.Pop(playListStackEntry);

			iPlayList = playListStackEntry.iPlayList;
			iPlayListIdx = playListStackEntry.iPlayingIdx;
			while (iPlayList != NULL)
			{
				// Grab the next file if possible (else pop another playlist and try again)
				TInt nextFile = iPlayListIdx+1;
				if (nextFile<iPlayList->Count())
				{
					iPlayListIdx++;
					file = (*iPlayList)[iPlayListIdx];
				
					// If we have another play list, drop into it (as above)
					while (file->FileType() == TOggFile::EPlayList)
					{
						TOggPlayListStackEntry playListStackEntry(iPlayList, iPlayListIdx);
						TInt err = iPlayListStack.Push(playListStackEntry);
						if (err != KErrNone)
						{
							iPlayListStack.Reset();
							return KNullDesC;
						}

						iPlayList = (TOggPlayList*) file;
						iPlayListIdx = 0;

						file = (*iPlayList)[iPlayListIdx];
					}

					return *(file->iFileName);
				}

				TOggPlayListStackEntry playListStackEntry;
				iPlayListStack.Pop(playListStackEntry);

				iPlayList = playListStackEntry.iPlayList;
				iPlayListIdx = playListStackEntry.iPlayingIdx;				
			}
		}
	}
	
	if (iPlayingIdx+1<nSongs)
	{
        // We are in the middle of the song list. Now play the next song.
        SetPlaying(iPlayingIdx+1);
    }
	else if (iRepeat)
    {
		// We are at the end of the song list, repeat it 
		SetPlaying(0);
    }
    else 
    {
        // We are at the end of the playlist, stop here.
        SetPlaying(ENoFileSelected);
	}

	if (iPlayingIdx == ENoFileSelected)
        return (KNullDesC);

	file = iFileList[iPlayingIdx];
	if (file->FileType() == TOggFile::EPlayList)
		file = (*iPlayList)[iPlayListIdx];

	return *(file->iFileName);
}

const TDesC& COggNormalPlay::GetPreviousSong()
{
    TInt nSongs= iFileList.Count();
    if ( (iPlayingIdx == ENoFileSelected) || (nSongs <=0) )
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }

	TOggFile* file = iFileList[iPlayingIdx];
	if (file->FileType() == TOggFile::EPlayList)
	{
		// We are playing a playlist, so get the previous file from the playlist
		TInt prevFile = iPlayListIdx-1;
		if (prevFile>=0)
		{
			iPlayListIdx--;
			file = (*iPlayList)[iPlayListIdx];

			while (file->FileType() == TOggFile::EPlayList)
			{
				// Another playlist. Push the current playlist onto the stack and get the previous file
				TOggPlayListStackEntry playListStackEntry(iPlayList, iPlayListIdx);
				TInt err = iPlayListStack.Push(playListStackEntry);
				if (err != KErrNone)
				{
					iPlayListStack.Reset();
					return KNullDesC;
				}

				iPlayList = (TOggPlayList*) file;
				iPlayListIdx = iPlayList->Count()-1;

				file = (*iPlayList)[iPlayListIdx];
			}

			return *(file->iFileName);
		}
		else
		{
			// Beginning of current playlist, pop the previous playlist from the stack
			TOggPlayListStackEntry playListStackEntry;
			iPlayListStack.Pop(playListStackEntry);

			iPlayList = playListStackEntry.iPlayList;
			iPlayListIdx = playListStackEntry.iPlayingIdx;
			while (iPlayList != NULL)
			{
				// Grab the previous file if possible (else pop another playlist and try again)
				TInt prevFile = iPlayListIdx-1;
				if (prevFile>=0)
				{
					iPlayListIdx--;
					file = (*iPlayList)[iPlayListIdx];
				
					// If we have another play list, drop into it (as above)
					while (file->FileType() == TOggFile::EPlayList)
					{
						TOggPlayListStackEntry playListStackEntry(iPlayList, iPlayListIdx);
						TInt err = iPlayListStack.Push(playListStackEntry);
						if (err != KErrNone)
						{
							iPlayListStack.Reset();
							return KNullDesC;
						}

						iPlayList = (TOggPlayList*) file;
						iPlayListIdx = iPlayList->Count()-1;

						file = (*iPlayList)[iPlayListIdx];
					}

					// Check that the file is still there (media hasn't been removed)
					if (iOggPlayback->Info(*(file->iFileName), ETrue) == KErrOggFileNotFound)
						iPlayingIdx = ENoFileSelected;

					if (iPlayingIdx == ENoFileSelected)
						return (KNullDesC);

					return *(file->iFileName);
				}

				TOggPlayListStackEntry playListStackEntry;
				iPlayListStack.Pop(playListStackEntry);

				iPlayList = playListStackEntry.iPlayList;
				iPlayListIdx = playListStackEntry.iPlayingIdx;				
			}
		}
	}

	if (iPlayingIdx-1>=0)
	{
        // We are in the middle of the playlist. Now play the previous song.
        SetPlaying(iPlayingIdx-1, ETrue);
    }
	else if (iRepeat)
    {
		// We are at the top of the playlist, repeat it 
		SetPlaying(nSongs-1, ETrue);
    }
    else 
    {
        // We are at the top of the playlist, stop here.
        SetPlaying(ENoFileSelected);
		return(KNullDesC);
	}

	file = iFileList[iPlayingIdx];
	if (file->FileType() == TOggFile::EPlayList)
		file = (*iPlayList)[iPlayListIdx];

	return *(file->iFileName);
}


// COggRandomPlay class
COggRandomPlay* COggRandomPlay::NewL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback, COggSongList* aSongList)
{
	COggRandomPlay* self = new(ELeave) COggRandomPlay(aAppView, aOggPlayback);
	CleanupStack::PushL(self);
	self->ConstructL(aSongList);

	CleanupStack::Pop(self);
	return self;
}

COggRandomPlay::COggRandomPlay(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback)
: COggSongList(aAppView, aOggPlayback)
{
}

void COggRandomPlay::ConstructL(COggSongList* aSongList)
{
	COggSongList::ConstructL(aSongList);

	// Override the default behaviour
	iNewFileList = ETrue;
}

COggRandomPlay::~COggRandomPlay()
{
    iRandomMemory.Close();
	iRandomMemoryIdx.Close();
}

const TDesC& COggRandomPlay::GetNextSong()
{
    TInt nSongs= iFileList.Count();
    if ( (iPlayingIdx == ENoFileSelected) || (nSongs <=0) )
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }
    
	TInt picked = -1;
    if (iNewFileList || ( (iRandomMemory.Count() == 0) && iRepeat) )
    {
        // Build a new list, if the playlist has changed, or if we have played all tunes and repeat is on
        // Build internal 'not already played' list, at beginning list is full
        iRandomMemory.Reset();
		iRandomMemoryIdx.Reset();

		TOggFile* playingFile = iFileList[iPlayingIdx];
		if (playingFile->FileType() == TOggFile::EPlayList)
			playingFile = (*iPlayList)[iPlayListIdx];

		for (TInt i=0; i<iFileList.Count(); i++)
        {
			TOggFile* file = iFileList[i];
			if (file->FileType() == TOggFile::EPlayList)
			{
				// Add all the files in the playlist
				ROggPlayListStack playListStack;
				TOggPlayListStackEntry playListStackEntry;
				TOggPlayList* playList = (TOggPlayList*) file;
				TInt nextFile = 0;
				while (playList)
				{
					TInt endOfPlayList = playList->Count();
					for ( ; nextFile<endOfPlayList ; nextFile++)
					{
						file = (*playList)[nextFile];
						if (file->FileType() == TOggFile::EPlayList)
						{
							playListStackEntry.iPlayingIdx = nextFile;
							playListStackEntry.iPlayList = playList;
							playListStack.Push(playListStackEntry);

							nextFile = 0;
							playList = (TOggPlayList* ) file;
							break;
						}
						else
						{
							if ((file == playingFile) && (playList == iPlayList))
								picked = iRandomMemory.Count();

							iRandomMemory.Append(file);
							iRandomMemoryIdx.Append(i);
						}
					}

					if (nextFile)
					{
						playListStack.Pop(playListStackEntry);
						playList = playListStackEntry.iPlayList;
						nextFile = playListStackEntry.iPlayingIdx + 1;
					}
				}
			}
			else
			{
				if (file == playingFile)
					picked = iRandomMemory.Count();
				
				iRandomMemory.Append(file);
				iRandomMemoryIdx.Append(i);
			}
        }
    }

    if (iNewFileList  && ((iRandomMemory.Count()>1) || !iRepeat))
    {
        // Remove the file currently played from the list
        iRandomMemory.Remove(picked);
        iRandomMemoryIdx.Remove(picked);
    }

	if (iRandomMemory.Count() == 0)
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }

	TInt64 rnd64;
	TInt64 picked64;

#if defined(SERIES60V3)
	TInt64 maxInt64 = MAKE_TINT64(1, 0);
#else
	TInt64 maxInt64 = TInt64(1, 0);
#endif

	TInt64 memCount = iRandomMemory.Count();
	TInt nextPick;
	if (iNewFileList)
	{
#if defined(SERIES60V3)
		rnd64 = Math::Random();
#else
		rnd64 = TInt64(0, Math::Random());
#endif

		picked64 = (rnd64 * memCount) / maxInt64;

#if defined(SERIES60V3)
		nextPick = I64LOW(picked64);
#else
		nextPick = picked64.Low();
#endif

		iNewFileList = EFalse;
	}
	else
	{
		// Avoid playing the same track (possible at the end of the track list when repeat is on)
        // Of course, if there's only one track in the playlist it's ok!
		do
		{
#if defined(SERIES60V3)
			rnd64 = Math::Random();
#else
			rnd64 = TInt64(0, Math::Random());
#endif

			picked64 = (rnd64 * memCount) / maxInt64;

#if defined(SERIES60V3)
		nextPick = I64LOW(picked64);
#else
		nextPick = picked64.Low();
#endif
		} while ((nextPick == picked) && (memCount>1));
	}
  
    TOggFile* file = iRandomMemory[nextPick];
    iRandomMemory.Remove(nextPick);
    
    iPlayingIdx = iRandomMemoryIdx[nextPick];
	iRandomMemoryIdx.Remove(nextPick);
    
    if (iPlayingIdx != ENoFileSelected)
		iAppView->SelectFile(iFileList[iPlayingIdx]);

	iAppView->Update();
	return *(file->iFileName);
}

const TDesC & COggRandomPlay::GetPreviousSong()
{
    // Exactly same behaviour has GetNextSong()
    return ( GetNextSong() );
}


////////////////////////////////////////////////////////////////
//
// CAbsPlayback
//
////////////////////////////////////////////////////////////////

// Just the constructor is defined in this class, everything else is 
// virtual

CAbsPlayback::CAbsPlayback(MPlaybackObserver* anObserver) :
  iState(CAbsPlayback::EClosed),
  iObserver(anObserver),
  iChannels(2),
  iRate(44100),
  iAlbum(),
  iArtist(),
  iGenre(),
  iTitle(),
  iTrackNumber()
{
}
