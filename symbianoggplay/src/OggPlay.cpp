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

#include "OggOs.h"
#include <e32std.h>
#include <hal.h>
#ifdef SERIES60
#include <aknkeys.h>	// EStdQuartzKeyConfirm etc.
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
#include <reent.h>

#include "OggControls.h"
#ifdef PLUGIN_SYSTEM
#include "OggPluginAdaptor.h"
#else
#include "OggTremor.h"
#endif
#include "OggPlayAppView.h"
#include "OggDialogs.h" 
#include "OggLog.h"

#include <OggPlay.rsg>
#include "OggFilesSearchDialogs.h"

#if defined(UIQ)
const TInt KThreadPriority = EPriorityAbsoluteHigh;
#else
const TInt KThreadPriority = EPriorityAbsoluteBackground;
#endif

//
// EXPORTed functions
//
EXPORT_C CApaApplication*
NewApplication()
{
	return new COggPlayApplication;
}

GLDEF_C int
E32Dll(TDllReason)
{
	return KErrNone;
}

////////////////////////////////////////////////////////////////
//
// COggActive class
//
///////////////////////////////////////////////////////////////

#ifdef MONITOR_TELEPHONE_LINE
COggActive::COggActive():iServer(0),iPhone(0),iLine(0),iRequestStatusChange(),iTimer(0),iCallBack(0),  iInterrupted(0)
{
}


void COggActive::ConstructL(COggPlayAppUi* theAppUi)
{
	iAppUi = theAppUi;
	
#if !defined(__WINS__)  
	// The destructor panics in emulator
	iServer= new (ELeave) RTelServer;
	User::LeaveIfError( iServer->Connect() );
	User::LeaveIfError( iServer->LoadPhoneModule(KTsyName) );
	
	TInt nPhones;
	User::LeaveIfError( iServer->EnumeratePhones(nPhones) );
	
	RTelServer::TPhoneInfo PInfo;
	User::LeaveIfError( iServer->GetPhoneInfo(0,PInfo) );
	
	iPhone= new (ELeave) RPhone;
	User::LeaveIfError( iPhone->Open(*iServer,PInfo.iName) );
	
	TInt nofLines;
	User::LeaveIfError( iPhone->EnumerateLines(nofLines) );
	
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
	RPhone::TLineInfo LInfo;
	for (TInt i=0; i<linesToTest; i++) 
	  {
	    TInt ret= iPhone->GetLineInfo(i,LInfo); 
	    if (ret!=KErrNone) 
	      {
		//TRACEF(COggLog::VA(_L("Line%d:Err%d"), i, ret ));
		User::Leave(ret);
	      }
	    else
	      {
		// Test code, left here because this is likely to change between
		// phone and it's nice to keep it in traces.
		User::LeaveIfError( iLine->Open(*iPhone,LInfo.iName) );
		TInt nCalls=-1;
		iLine->EnumerateCall(nCalls);
		iLine->Close();
		TRACEF(COggLog::VA(_L("Line%d:%S=%d"), i, &LInfo.iName, nCalls ));
		// End of test code

		if (LInfo.iName.Match(_L("Voice*1")) != KErrNotFound)
		  {
                
		    iLineToMonitor = i;
		    iLineCallsReportedAtIdle = nCalls;
		  }
	      }
	  }
#endif
#endif
}

TInt
COggActive::CallBack(TAny* aPtr)
{
	COggActive* self= (COggActive*)aPtr;
	
  if(self->iAppUi->iIsStartup) {
    self->iAppUi->PostConstructL();
	}
	
	self->iAppUi->NotifyUpdate();
	
	if( self->iLine )
    {
		RPhone::TLineInfo LInfo;
		
		self->iPhone->GetLineInfo(self->iLineToMonitor,LInfo);
		self->iLine->Open(*self->iPhone,LInfo.iName);
		TInt nCalls=0;
		self->iLine->EnumerateCall(nCalls);
		self->iLine->Close();
		
		TBool isRinging = nCalls > self->iLineCallsReportedAtIdle;
		TBool isIdle    = nCalls == self->iLineCallsReportedAtIdle;
		
		if (isRinging && !self->iInterrupted) 
		{
			// the phone is ringing or someone is making a call, pause the music if any
			if (self->iAppUi->iOggPlayback->State()==CAbsPlayback::EPlaying) {
				TRACELF("GSM is active");
				self->iInterrupted= ETrue;
				self->iAppUi->HandleCommandL(EOggPauseResume);
			}
		}
		else if (self->iInterrupted) 
		{
			// our music was interrupted by a phone call, now what
			if (isIdle) {
				TRACELF("GSM is idle");
				// okay, the phone is idle again, let's continue with the music
				self->iInterrupted= EFalse;
				if (self->iAppUi->iOggPlayback->State()==CAbsPlayback::EPaused)
					self->iAppUi->HandleCommandL(EOggPauseResume);
			}
		}
	}
	return 1;
}


void
COggActive::IssueRequest()
{
#if defined(SERIES60) || defined(SERIES80)
	iTimer= CPeriodic::New(CActive::EPriorityStandard);
	iCallBack= new (ELeave) TCallBack(COggActive::CallBack,this);
	iTimer->Start(TTimeIntervalMicroSeconds32(1000000),TTimeIntervalMicroSeconds32(1000000),*iCallBack);
#endif
}

COggActive::~COggActive()
{
	if (iLine) { 
		iLine->Close();
		delete iLine; 
		iLine= NULL;
	}
	if (iPhone) { 
		iPhone->Close();
		delete iPhone; 
		iPhone= NULL; 
	}
	if (iServer) { 
		iServer->Close();
		delete iServer; 
		iServer= NULL;
	}
#if defined(SERIES60) || defined(SERIES80)
	// UIQ_?
	if (iTimer) {
		iTimer->Cancel();
		delete iTimer;
		iTimer = NULL;
	}
	if (iCallBack) {
		delete iCallBack;
		iCallBack = NULL;
	}
#endif
}
#endif // MONITOR_TELEPHONE_LINE

////////////////////////////////////////////////////////////////
//
// App UI class, COggPlayAppUi
//
////////////////////////////////////////////////////////////////

void
COggPlayAppUi::ConstructL()
{
#if defined(SERIES60_SPLASH)
	ShowSplash();
  // Otherwise BaseConstructL() clears screen when launched via recognizer (?!)
  CEikonEnv::Static()->RootWin().SetOrdinalPosition(0,ECoeWinPriorityNeverAtFront);
#endif
	
	BaseConstructL();
	
	TParsePtrC aP(Application()->AppFullName());
  _LIT(KiIniFileNameExtension,".ini");
	const TUint KExtLength=4;
	iIniFileName=HBufC::NewL(aP.Drive().Length()+aP.Path().Length()+aP.Name().Length()+KExtLength);
  iIniFileName->Des().Copy(Application()->AppFullName());
  iIniFileName->Des().SetLength(iIniFileName->Length() - KExtLength);
  iIniFileName->Des().Append( KiIniFileNameExtension );

  iDbFileName.Copy(Application()->AppFullName());
  iDbFileName.SetLength(iDbFileName.Length() - 3);
  iDbFileName.Append(_L("db"));

	iSkinFileDir.Copy(Application()->AppFullName());
	iSkinFileDir.SetLength(iSkinFileDir.Length() - 11);

#if defined(__WINS__)
  // The emulator doesn't like to write to the Z: drive, avoid that.
  *iIniFileName = _L("C:\\oggplay.ini");
  iDbFileName = _L("C:\\oggplay.db");
#endif

	
	iSkins= new CDesCArrayFlat(3);
	FindSkins();
	iCurrentSkin= 0;
	
	iHotkey= 0;
	iVolume= KMaxVolume;
	iTryResume= 0;
	iAlarmTriggered= 0;
	iAlarmActive= 0;
	iAlarmTime.Set(_L("20030101:120000.000000"));
	iViewBy= ETop;
	iAnalyzerState= 0;
    iIsStartup=ETrue;
    iSettings.iWarningsEnabled = ETrue;
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

	SetRandomL(iRandom);
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
    iUserHotkeysView =new(ELeave) COggUserHotkeysView(*iAppView);
	RegisterViewL(*iUserHotkeysView);
#ifdef SERIES60_SPLASH_WINDOW_SERVER
    iSplashView=new(ELeave) COggSplashView(*iAppView);
    RegisterViewL(*iSplashView);
#endif /*SERIES60_SPLASH_WINDOW_SERVER*/
#ifdef PLUGIN_SYSTEM
    iCodecSelectionView=new(ELeave) COggPluginSettingsView(*iAppView);
    RegisterViewL(*iCodecSelectionView);
#endif
#endif /* SERIES60 */
        
	SetProcessPriority();
	SetThreadPriority();
	

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
#endif /* SERIES60 */

#ifdef MONITOR_TELEPHONE_LINE
	if (iActive) { delete iActive; iActive=0; }
#endif

	if (iOggPlayback) { delete iOggPlayback; iOggPlayback=0; }
	iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);
	
	delete iIniFileName;
	delete iSkins;
	delete iOggMsgEnv ;
    delete iSongList;
    iViewHistoryStack.Close();
    iRestoreStack.Close();
	COggLog::Exit();  
	
	CloseSTDLIB();
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

TBool
COggPlayAppUi::IsAlarmTime()
{
	TInt err;
	TTimeIntervalSeconds deltaT;
	TTime now;
	now.HomeTime();
	err= iAlarmTime.SecondsFrom(now, deltaT);
	return (err==KErrNone && deltaT<TTimeIntervalSeconds(10));
}

void
COggPlayAppUi::NotifyUpdate()
{
	// Set off an alarm if the alarm time has been reached
	if (iAlarmActive && !iAlarmTriggered && IsAlarmTime()) {
		iAlarmTriggered= 1;
		iAlarmActive= 0;
		TBuf<256> buf;
		iEikonEnv->ReadResource(buf, R_OGG_STRING_3);
		User::InfoPrint(buf);
		iAppView->ClearAlarm();
		NextSong();
	}
	
	// Try to resume after sound device was stolen
	if (iTryResume>0 && iOggPlayback->State()==CAbsPlayback::EPaused) {
		if (iOggPlayback->State()!=CAbsPlayback::EPaused) iTryResume= 0;
		else {
			iTryResume--;
			if (iTryResume==0 && iOggPlayback->State()==CAbsPlayback::EPaused) HandleCommandL(EOggPauseResume);
		}
	}
	
	iAppView->UpdateClock();
	
	if (iOggPlayback->State()!=CAbsPlayback::EPlaying &&
		iOggPlayback->State()!=CAbsPlayback::EPaused) return; 
	
	iAppView->UpdateSongPosition();
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

void
COggPlayAppUi::UpdateSoftkeys(TBool aForce)
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
	

  switch (aCommand) {

  case EOggAbout: {
		COggAboutDialog *d = new COggAboutDialog();
		TBuf<128> buf;
		iEikonEnv->ReadResource(buf, R_OGG_VERSION);
		d->SetVersion(buf);
#ifndef SERIES80 // To be removed when functionality has been implemented
		d->ExecuteLD(R_DIALOG_ABOUT);
#endif		
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
     
		HandleCommandL(EOggStop);
        if (iRandom)
        {
            SetRandomL(EFalse);
        }
        else
        {
            SetRandomL(ETrue);
        }
        iSongList->SetRepeat(iSettings.iRepeat);
		break;
					  }
		
	case EOggRepeat: {
	    ToggleRepeat();
		break;
					 }
		
	case EOggOptions: {

#ifdef UIQ
		CHotkeyDialog *hk = new CHotkeyDialog(&iHotkey, &iAlarmActive, &iAlarmTime);
		if (hk->ExecuteLD(R_DIALOG_HOTKEY)==EEikBidOk) {
			SetHotKey();
			iAppView->SetAlarm();
		}
#elif defined(SERIES80)
        CSettingsS80Dialog *hk = new CSettingsS80Dialog(&iSettings);
		hk->ExecuteLD(R_DIALOG_OPTIONS) ;
		UpdateSoftkeys(EFalse);
#elif defined(SERIES60)
        ActivateOggViewL(KOggPlayUidSettingsView);
        SetRepeat(iSettings.iRepeat);
#endif
		break;
					  }

#ifdef PLUGIN_SYSTEM
    case EOggCodecSelection:
        {
#if defined(SERIES80)
		CCodecsS80Dialog *cd = new CCodecsS80Dialog();
		cd->ExecuteLD(R_DIALOG_CODECS);
#else
         ActivateOggViewL(KOggPlayUidCodecSelectionView);
#endif
		break;
        }
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
        COggFilesSearchDialog *d = new (ELeave) COggFilesSearchDialog(iAppView->iOggFiles);
        if(iSettings.iScanmode==TOggplaySettings::EMmcOgg) {
          iAppView->iOggFiles->SearchSingleDrive(KMmcSearchDir,d,R_DIALOG_FILES_SEARCH,iCoeEnv->FsSession());
        } else {
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
#if defined(SERIES60)
		// UIQ_?
		if( iAppView->CanStop() )
			HandleCommandL(EOggStop);
#endif
		iAppView->ReadSkin(buf); //FIXME
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
    TInt64 pos= iOggPlayback->Position()+=KFfRwdStep;
    iOggPlayback->SetPosition(pos);
	iAppView->UpdateSongPosition();
    }
    break;
  case EUserRewind : {
    TInt64 pos= iOggPlayback->Position()-=KFfRwdStep;
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
    iSongList->SetRepeat(iSettings.iRepeat); // Make sure that the repeat setting in AppUi has same
                                   // value in the songList
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
            iAppView->Update();
            UpdateSoftkeys();
            return;
        }

        if (iOggPlayback->State()==CAbsPlayback::EPlaying) {
            iOggPlayback->Pause();
            UpdateSoftkeys();
        }
        
        iSongList->SetPlayingFromListBox(idx);
        if (iOggPlayback->Open(iSongList->GetPlaying())==KErrNone) {
            iOggPlayback->SetVolume(iVolume);
            if (iOggPlayback->State()!=CAbsPlayback::EPlaying) {
                iAppView->SetTime(iOggPlayback->Time());
                iOggPlayback->Play();
                iAppView->Update();
            }
            UpdateSoftkeys();
        } else {
            iSongList->SetPlayingFromListBox(ENoFileSelected);
            UpdateSoftkeys();
        }
        
    }
    return;
}

void
COggPlayAppUi::PauseResume()
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
	  iAppView->Update();
      UpdateSoftkeys();
  }
  else
    PlaySelect();
}

void
COggPlayAppUi::Stop()
{
  if (iOggPlayback->State()==CAbsPlayback::EPlaying ||
      iOggPlayback->State()==CAbsPlayback::EPaused) {
    iOggPlayback->Stop();
    UpdateSoftkeys();
    iSongList->SetPlayingFromListBox(ENoFileSelected); // calls iAppview->Update
  }
  return;

}


void
COggPlayAppUi::SelectPreviousView()
{
  if(iViewHistoryStack.Count()==0) return;

#if defined(SERIES60)
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

  #if defined(SERIES60)
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

    HBufC * filter = HBufC::NewL(128);
    CleanupStack::PushL(filter);
    TPtr tmp = filter->Des();
    iAppView->GetFilterData(idx, tmp);
    iAppView->FillView(ETitle, iViewBy, filter->Des());
    CleanupStack::PopAndDestroy();
}

void
COggPlayAppUi::ShowFileInfo()
{
    TBool songPlaying = iSongList->AnySongPlaying();
	if ((!songPlaying) && iAppView->GetSelectedIndex()<0) return;
	if (!songPlaying) {
		// no song is playing, show info for selected song if possible
		if (iAppView->GetItemType(iAppView->GetSelectedIndex())==COggListBox::EBack) return;
		if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==ETop) return;
		if (iOggPlayback->Info( iAppView->GetFileName(iAppView->GetSelectedIndex()))!=KErrNone)
            return;
	}
	COggInfoDialog *d = new COggInfoDialog();
	d->SetFileName(iOggPlayback->FileName());
	d->SetRate(iOggPlayback->Rate());
	d->SetChannels(iOggPlayback->Channels());
	d->SetFileSize(iOggPlayback->FileSize());
	d->SetTime(iOggPlayback->Time().GetTInt());
	d->SetBitRate(iOggPlayback->BitRate()/1000);
	if ( ! iSongList->AnySongPlaying() ) iOggPlayback->ClearComments();
#ifndef SERIES80 // To be removed when functionality has been implemented
	d->ExecuteLD(R_DIALOG_INFO);
#endif
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
        iOggPlayback->Pause();
#endif
        if (songName.Length()>0 && iOggPlayback->Open(songName)==KErrNone) {
            iOggPlayback->Play();
            iAppView->SetTime(iOggPlayback->Time());
            UpdateSoftkeys();
            iAppView->Update();
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
        iOggPlayback->Pause();
#endif
        if (songName.Length()>0 && iOggPlayback->Open(songName)==KErrNone) {
            iOggPlayback->Play();
            iAppView->SetTime(iOggPlayback->Time());
            UpdateSoftkeys();
            iAppView->Update();
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
    iEikonEnv->ReadResource(buf, (iRandom) ? R_OGG_RANDOM_ON : R_OGG_RANDOM_OFF );
    aMenuPane->SetItemTextL( EOggShuffle, buf );
#endif
		TBool isSongList= ((iViewBy==ETitle) || (iViewBy==EFileName));
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

void
COggPlayAppUi::FindSkins()
{
	iSkins->Reset();
	
	RFs session;
	User::LeaveIfError(session.Connect());
	
	CDirScan* ds = CDirScan::NewLC(iCoeEnv->FsSession());
	TRAPD(err,ds->SetScanDataL(iSkinFileDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
	if (err!=KErrNone) {
		//_LIT(KS,"Error in FindSkins-SetScanDataL");
		//OGGLOG.WriteFormat(KS);
		
		delete ds;
		return;
	}
	CleanupStack::Pop(ds);
	CDir* c=0;
	
	HBufC* fullname = HBufC::NewLC(512);
	
	for(;;) {
		ds->NextL(c);
		if (c==0) break;
		
		for (TInt i=0; i<c->Count(); i++) 
		{
			const TEntry e= (*c)[i];
			
			fullname->Des().Copy(ds->FullPath());
			fullname->Des().Append(e.iName);
			
			TParsePtrC p(fullname->Des());
			
			if (p.Ext()==_L(".skn") || p.Ext()==_L(".SKN")) {
				iSkins->AppendL(p.NameAndExt());
				if (iSkins->Count()==10) break;
			}
		}
		delete c; c=NULL;
    }
	
	CleanupStack::PopAndDestroy(); // fullname
	delete ds;
	session.Close();
	
	//_LIT(KS,"Found %d skin(s)");
	//OGGLOG.WriteFormat(KS,iSkins->Count());
	
}

void
COggPlayAppUi::ReadIniFile()
{
    RFile in;
    TInt err;
    
#ifdef SERIES80
      iSettings.iSoftKeysIdle[0] = TOggplaySettings::EPlay;
      iSettings.iSoftKeysIdle[1] = TOggplaySettings::EStop ;
      iSettings.iSoftKeysIdle[2] = TOggplaySettings::ENoHotkey;
      iSettings.iSoftKeysIdle[3] = TOggplaySettings::EHotKeyExit ;
      iSettings.iSoftKeysPlay[0] = TOggplaySettings::EPause;
      iSettings.iSoftKeysPlay[1] = TOggplaySettings::ENextSong ;
      iSettings.iSoftKeysPlay[2] = TOggplaySettings::EFastForward;
      iSettings.iSoftKeysPlay[3] = TOggplaySettings::EHotKeyExit ;
#endif
 
    // Default value
	iSettings.iGainType = ENoGain;

    // Open the file
    if ( (err = in.Open(iCoeEnv->FsSession(), iIniFileName->Des(), EFileRead | EFileStreamText)) != KErrNone )
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
   if ( val != 0 )
   {
      TTime t(tmp64);
			iAlarmTime= t;
		}

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
      iRandom                    = (TBool) IniRead32( tf, 0, 1 );

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

	if ( ini_version >=7)
	{
		iSettings.iGainType = IniRead32(tf);
		iOggPlayback->SetVolumeGain((TGainType) iSettings.iGainType);
	}

   in.Close();
	
   IFDEF_S60( iAnalyzerState = EDecay; )
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

void
COggPlayAppUi::WriteIniFile()
{
    RFile out;
    
    TRACE(COggLog::VA(_L("COggPlayAppUi::WriteIniFile() %S"), iIniFileName ));
    
    // Accessing the MMC , using the TFileText is extremely slow. 
    // We'll do everything using the C:\ drive then move the file to it's final destination
    // in MMC.
    
    TParsePtrC p(iIniFileName->Des());
	TBool useTemporaryFile = EFalse;
	TFileName fileName = iIniFileName->Des();
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
    TInt iniversion=7;
    
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

	num.Num(iAlarmTime.Int64());
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

	num.Num(iSettings.iAutoplay);
	tf.Write(num);

 	num.Num(iSettings.iManeuvringSpeed);
	tf.Write(num);
    
    num.Num(TOggplaySettings::ENofHotkeys);
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
    
    num.Num(iRandom);
    tf.Write(num);

#ifdef PLUGIN_SYSTEM
    CDesCArrayFlat * supportedExtensionList = iOggPlayback->GetPluginListL().SupportedExtensions();
    TRAPD(Err,
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

	// Please increase ini_version when adding stuff
	
	out.Close();
	if (useTemporaryFile) 
	{
		// Move the file to the MMC
		CFileMan* fileMan = NULL;
	    TRAPD(Err2, fileMan = CFileMan::NewL(iCoeEnv->FsSession()));
		if (Err2 == KErrNone)
		{
			fileMan->Move( fileName, *iIniFileName,CFileMan::EOverWrite);
			delete (fileMan);
		}
	}
	
	
     TRACEF(_L("Writing Inifile 10..."));
}

void 
COggPlayAppUi::HandleForegroundEventL(TBool aForeground)
{
	CEikAppUi::HandleForegroundEventL(aForeground);
	iForeground= aForeground;
}

void
COggPlayAppUi::SetRandomL(TBool aRandom)
{

   // Toggle random
    if (iSongList)
     delete(iSongList);
    iSongList = NULL;
        
    iRandom = aRandom;
    if (iRandom)
        iSongList = new(ELeave) COggRandomPlay();
    else
    	iSongList = new(ELeave) COggNormalPlay();
    
    iSongList->ConstructL(iAppView, iOggPlayback);
}

void 
COggPlayAppUi::ToggleRepeat()
{
	if (iSettings.iRepeat)
	      SetRepeat(EFalse);
	    else
	      SetRepeat(ETrue);
	    
	iAppView->UpdateRepeat();
}

void
COggPlayAppUi::SetRepeat(TBool aRepeat)
{
    iSettings.iRepeat = aRepeat;
    iSongList->SetRepeat(aRepeat);
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
	
  TRAPD( newerMind, 

	  RFbsSession::Connect();

    // Check if there is a splash mbm available, if not splash() will make a silent leave
	  TFileName *appName = new (ELeave) TFileName(Application()->AppFullName());
	  CleanupStack::PushL(appName);
	  TParsePtrC p(*appName);
    TBuf<100> iSplashMbmFileName(p.DriveAndPath());
	  iSplashMbmFileName.Append( _L("s60splash.mbm") );  // Magic string :-(
	  
	  CFbsBitmap* iBitmap = new (ELeave) CFbsBitmap;
	  CleanupStack::PushL(iBitmap);
	  User::LeaveIfError( iBitmap->Load( iSplashMbmFileName,0,EFalse));
    
	  CFbsScreenDevice *iDevice = NULL;

	  TDisplayMode dispMode = CCoeEnv::Static()->ScreenDevice()->DisplayMode();
	  iDevice = CFbsScreenDevice::NewL(_L("scdv"),dispMode);
	  CleanupStack::PushL(iDevice);
	  
	  CFbsBitGc *iGc = NULL;
	  User::LeaveIfError( iDevice->CreateContext(iGc));
	  CleanupStack::PushL(iGc);
	  iGc->BitBlt(TPoint(0,0), iBitmap );

	  iDevice->Update();
	  CleanupStack::PopAndDestroy(4);  // appName iBitmap iGc iDevice;
  );

  RFbsSession::Disconnect();
}
#endif // #if defined(SERIES60_SPLASH)



void
COggPlayAppUi::SetProcessPriority()
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

void
COggPlayAppUi::SetThreadPriority()
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

void COggPlayAppUi::SetVolumeGainL(TGainType aNewGain)
{
	iOggPlayback->SetVolumeGain(aNewGain);
	iSettings.iGainType = aNewGain;

#if defined(SERIES60)
	iSettingsView->VolumeGainChangedL();
#endif
}

////////////////////////////////////////////////////////////////
//
// COggPlayDocument class
//
///////////////////////////////////////////////////////////////
CFileStore* COggPlayDocument::OpenFileL(TBool /*aDoOpen*/,const TDesC& aFilename, RFs& /*aFs*/)
{
#ifdef SERIES60
	iAppUi->OpenFileL(aFilename);
#else
	aFilename; // To shut up the compiler warning
#endif

	return NULL;
}


CEikAppUi* COggPlayDocument::CreateAppUiL(){
    iAppUi = new (ELeave) COggPlayAppUi;
    return iAppUi;	
}



////////////////////////////////////////////////////////////////
//
// COggSongList class
//
///////////////////////////////////////////////////////////////

void COggSongList::ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback)
{
	iPlayingIdx = ENoFileSelected;
    iAppView = aAppView;
    iOggPlayback = aOggPlayback;
    iNewFileList = EFalse;
}


COggSongList::~COggSongList()
{
  iFileList.Reset();
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
            iFileList.Append(iAppView->GetFileAbsoluteIndex(i));
            if (i == aPlaying)
                iPlayingIdx = iFileList.Count()-1;
        }
    }
    SetPlaying(iPlayingIdx);
}

void 
COggSongList::SetPlaying(TInt aPlaying)
{   
    
    iPlayingIdx = aPlaying;
    if (aPlaying != ENoFileSelected)
    {
        // Check that the file is still there (media hasn't been removed)
        if( iOggPlayback->Info(RetrieveFileName(iFileList[aPlaying]), ETrue) == KErrOggFileNotFound )
            iPlayingIdx = ENoFileSelected;
    }
    
    if (iPlayingIdx != ENoFileSelected)
    {
        iAppView->SelectItemFromAbsoluteIndex(iFileList[iPlayingIdx]);
    }
    iAppView->Update();
}

const TDesC &
COggSongList::RetrieveFileName(TInt anAbsoluteIndex)
{
    if (anAbsoluteIndex == ENoFileSelected) 
        return KNullDesC;
    return (iAppView->iOggFiles->FindFromIndex(anAbsoluteIndex));
}


const TInt 
COggSongList::GetPlayingAbsoluteIndex()
{
    if (iPlayingIdx != ENoFileSelected)
        return ( iFileList[iPlayingIdx] );
    return(ENoFileSelected);
}

const TDesC& 
COggSongList::GetPlaying()
{
    if (iPlayingIdx == ENoFileSelected)
        return KNullDesC;
    return(RetrieveFileName(iFileList[iPlayingIdx]));
}

const TBool 
COggSongList::AnySongPlaying()
{
    if (iPlayingIdx ==ENoFileSelected)
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
        if ( GetPlayingAbsoluteIndex() == iAppView->GetFileAbsoluteIndex(idx) )
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


////////////////////////////////////////////////////////////////
//
// COggNormalPlay class
//
///////////////////////////////////////////////////////////////
COggNormalPlay::COggNormalPlay ()
{
}

COggNormalPlay::~COggNormalPlay ()
{
}

void COggNormalPlay::ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback)
{
	COggSongList::ConstructL(aAppView, aOggPlayback);
}

   
const TDesC & COggNormalPlay::GetNextSong()
{

    int nSongs= iFileList.Count();
    if ( (iPlayingIdx == ENoFileSelected) || (nSongs <=0) )
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }
    
    if (iPlayingIdx+1<nSongs) {
        // We are in the middle of the playlist. Now play the next song.
        SetPlaying(iPlayingIdx+1);
    }
	else if (iRepeat)
    {
		// We are at the end of the playlist, repeat it 
		SetPlaying(0);
    }
    else 
    {
        // We are at the end of the playlist, stop here.
        SetPlaying(ENoFileSelected);
	}
    if (iPlayingIdx == ENoFileSelected)
    {
        return (KNullDesC);
    }
    return RetrieveFileName( iFileList[iPlayingIdx] );
}

const TDesC & COggNormalPlay::GetPreviousSong()
{
    
    int nSongs= iFileList.Count();
    if ( (iPlayingIdx == ENoFileSelected) || (nSongs <=0) )
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }

    if (iPlayingIdx-1>=0) {
        // We are in the middle of the playlist. Now play the previous song.
        SetPlaying(iPlayingIdx-1);
    }
	else if (iRepeat)
    {
		// We are at the top of the playlist, repeat it 
		SetPlaying(nSongs-1);
    }
    else 
    {
        // We are at the top of the playlist, stop here.
        SetPlaying(ENoFileSelected);
		return(KNullDesC);
	}
    return RetrieveFileName(iFileList[iPlayingIdx]);
}


////////////////////////////////////////////////////////////////
//
// COggRandomPlay class
//
///////////////////////////////////////////////////////////////
COggRandomPlay::COggRandomPlay ()
{
}

COggRandomPlay::~COggRandomPlay ()
{
    iRandomMemory.Reset();
}

void COggRandomPlay::ConstructL(COggPlayAppView* aAppView, CAbsPlayback* aOggPlayback)
{
    COggSongList::ConstructL(aAppView, aOggPlayback);
    // Initialize the random seed
    TTime t;
    t.HomeTime();
    iSeed = t.DateTime().MicroSecond();
}




const TDesC & COggRandomPlay::GetNextSong()
{
    int nSongs= iFileList.Count();
    if ( (iPlayingIdx == ENoFileSelected) || (nSongs <=0) )
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }
    
    if (iNewFileList || ( (iRandomMemory.Count() == 0) && iRepeat) )
    {
        // Build a new list, if the playlist has changed, or if we have played all tunes and repeat is on
        // Build internal 'not already played' list, at beginning list is full
        iRandomMemory.Reset();
        for (TInt i=0; i<iFileList.Count(); i++)
        {
            iRandomMemory.Append(i);
        }
    }
    if (iNewFileList)
    {
        // Remove the file currently played from the list
        iRandomMemory.Remove(iPlayingIdx);
        iNewFileList=EFalse;
    }
    if ( iRandomMemory.Count() == 0)
    {
        SetPlaying(ENoFileSelected);
        return(KNullDesC);
    }

    TReal rnd = Math::FRand(iSeed);
    TInt picked= (int)(rnd * ( iRandomMemory.Count() ) );
  
    TInt songIdx = iRandomMemory[picked];
    iRandomMemory.Remove(picked);
    SetPlaying(songIdx);
    return RetrieveFileName( iFileList[songIdx] );
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
  iBitRate(0),
  iChannels(2),
  iFileSize(0),
  iRate(44100),
  iTime(0),
  iAlbum(),
  iArtist(),
  iGenre(),
  iTitle(),
  iTrackNumber()
{
}
