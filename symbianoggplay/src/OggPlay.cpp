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
#include "OggPlay.h"

#ifdef SERIES60
#include <aknkeys.h>	// EStdQuartzKeyConfirm etc.
#include <akndialog.h>   // for about
_LIT(KTsyName,"phonetsy.tsy");
#else
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
#include "OggTremor.h"
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

const TInt ENoFileSelected = -1;

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

////////////////////////////////////////////////////////////////
//
// COggActive class
//
///////////////////////////////////////////////////////////////

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
	// Series 60 reports 3 lines 0='Fax' 1='Data' and 2='VoiceLine1' 
	// (and 'VoiceLine1' reports '1' in EnumerateCall() when in idle ?!?)
	// Should probably use the name instead of the magic number in iLineToMonitor..
	TInt linesToTest = nofLines;
	iLineToMonitor = 2;     // i.e. 'VoiceLine1' only
	iLineCallsReportedAtIdle = 1;
#endif
	
	// Test code
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
			User::LeaveIfError( iLine->Open(*iPhone,LInfo.iName) );
			TInt nCalls=-1;
			iLine->EnumerateCall(nCalls);
			iLine->Close();
			TRACEF(COggLog::VA(_L("Line%d:%S=%d"), i, &LInfo.iName, nCalls ));
		}
    }
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
	iTimer= CPeriodic::New(CActive::EPriorityStandard);
	iCallBack= new (ELeave) TCallBack(COggActive::CallBack,this);
	iTimer->Start(TTimeIntervalMicroSeconds32(1000000),TTimeIntervalMicroSeconds32(1000000),*iCallBack);
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
#if defined(SERIES60)
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
	
	//    iViewStack = new (ELeave) RArray<TInt>();
	
	iRepeat= 1;
	iCurrent= -1;
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
	iOggPlayback= new(ELeave) COggPlayback(iOggMsgEnv, this);
	iOggPlayback->ConstructL();

	ReadIniFile();
	
	iAppView=new(ELeave) COggPlayAppView;
	iAppView->ConstructL(this, ClientRect());
	HandleCommandL(EOggSkinOne+iCurrentSkin);
	
	AddToStackL(iAppView); // Receiving Keyboard Events 
	
	iAppView->InitView();
	
	SetHotKey();
	iIsRunningEmbedded = EFalse;
	
	iActive= new (ELeave) COggActive();
	iActive->ConstructL(this);
	iActive->IssueRequest();
	
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
	iSettingsView=new(ELeave) COggSettingsView(*iAppView);
	RegisterViewL(*iSettingsView);
  iUserHotkeys=new(ELeave) COggUserHotkeysView(*iAppView);
	RegisterViewL(*iUserHotkeys);
#endif

	SetProcessPriority();
	SetThreadPriority();
	
	ActivateOggViewL();

#if defined(SERIES60_SPLASH)
	iEikonEnv->RootWin().SetOrdinalPosition(0,ECoeWinPriorityNormal);
#endif
}

void COggPlayAppUi::PostConstructL()
{

  if(iSettings.iAutoplay) {
    for(TInt i=0;i<iRestoreStack.Count();i++) {
      TRACEF(COggLog::VA(_L("Setting istack[%d]:%d"), i,iRestoreStack[i]));
      iAppView->SelectSong(iRestoreStack[i]);
      HandleCommandL(EOggPlay);
    }
    iAppView->SelectSong(iRestoreCurrent);
  }

  if ( iIsRunningEmbedded || (iSettings.iAutoplay && iIsStartup) 
      && iOggPlayback->State()!=CAbsPlayback::EPlaying ) {
     NextSong();
	}
  iIsStartup=EFalse;

}

COggPlayAppUi::~COggPlayAppUi()
{
	WriteIniFile();
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
	if (iSettingsView) {
		DeregisterView(*iSettingsView);
  	delete iSettingsView;
	}

  if( iUserHotkeys ) {
    DeregisterView(*iUserHotkeys);
    delete iUserHotkeys;
    }

	if (iActive) { delete iActive; iActive=0; }
	if (iOggPlayback) { delete iOggPlayback; iOggPlayback=0; }
	iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);
	
	delete iIniFileName;
	delete iSkins;
	delete iOggMsgEnv ;
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
	
	if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==ETop) {
		SetCurrent(-1);
		return;
	}
	
	if (iCurrent<0) {
		iAppView->Update();
		return;
	}
	
	int nSongs= iAppView->GetNSongs();
	
	if (iCurrent+1==nSongs && iRepeat && (!iIsRunningEmbedded)) {
		// We are at the end of the playlist, and "repeat" is enabled
 		if (iAppView->GetItemType(0)==6) 
      SetCurrent(1);
    else
      SetCurrent(0);
    if( iOggPlayback->Info(iCurrentSong, ETrue) != KErrOggFileNotFound )
      iAppView->SelectSong(iCurrent);
    else
      SetCurrent(ENoFileSelected);
  } 
	else if (iCurrent+1<nSongs) {
		// We are in the middle of the playlist. Now play the next song.
    // First check that current still exists (i.e. media still present)
    SetCurrent(iCurrent+1);
    if( iOggPlayback->Info(iCurrentSong, ETrue) != KErrOggFileNotFound )
      iAppView->SelectSong(iCurrent);
    else
      SetCurrent(ENoFileSelected);
	}
	else {
		// We are at the end of the playlist. Stop playing.
		SetCurrent(ENoFileSelected);
		if (iIsRunningEmbedded) {
			iIsRunningEmbedded = EFalse;
		}
		return;
	}
	
	if (iCurrentSong.Length()>0 && iOggPlayback->Open(iCurrentSong)==KErrNone) {
		iOggPlayback->Play();
		iAppView->SetTime(iOggPlayback->Time());
		iAppView->Update();
	} else SetCurrent(ENoFileSelected);
	
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

#if defined(SERIES60)
void
COggPlayAppUi::UpdateSeries60Softkeys()
{
  // Return if somebody else than main view has taken focus, 
  // others must control the CBA themselves
  TVwsViewId viewId;
  GetActiveViewId(viewId);
  if( viewId.iViewUid != KOggPlayUidFOView )
    return;

  if( iOggPlayback->State() == CAbsPlayback::EPaused ||
			iOggPlayback->State() == CAbsPlayback::EPlaying )
    {
    Cba()->AddCommandSetToStackL(R_PLAY_CBA);
    }
  else
    {
    Cba()->AddCommandSetToStackL(R_IDLE_CBA);
    }
  Cba()->DrawNow();
}
#endif

void
COggPlayAppUi::HandleCommandL(int aCommand)
{
	if(aCommand == EOggAbout) {
		COggAboutDialog *d = new COggAboutDialog();
		TBuf<128> buf;
		iEikonEnv->ReadResource(buf, R_OGG_VERSION);
		d->SetVersion(buf);
		d->ExecuteLD(R_DIALOG_ABOUT);
		return;
	}
	
	int idx = iAppView->GetSelectedIndex();
	const TDesC& curFile= iAppView->GetFileName(idx);
//  TRACE(COggLog::VA(_L("HandleCommandL -Index=%d, icurrent=%d, curFile='%S'"), idx, iCurrent,&curFile ));    // FIXIT

  switch (aCommand) {
		
	case EOggPlay: {
		if (iViewBy==ETop) {
			if (idx>=ETitle && idx<=EFileName) 
			{
				iViewHistoryStack.Append(idx);
				HandleCommandL(EOggViewByTitle+idx);
			}
			break;
		}
		if (iAppView->GetItemType(idx)==6) {
			// the back item was selected: show the previous view
			
			TInt previousListboxLine = (TInt&) (iViewHistoryStack[iViewHistoryStack.Count()-1]);
			iViewHistoryStack.Remove(iViewHistoryStack.Count()-1);
			
			TLex parse(curFile);
			TInt previousView;
			parse.Val(previousView);
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
			iAppView->SelectSong(previousListboxLine);
#endif
			break;
		}
		if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) 
		{
			iViewHistoryStack.Append(idx);
			
			HBufC *aBuf;
			aBuf = curFile.Alloc();
			CleanupStack::PushL(aBuf);
			iAppView->FillView(ETitle, iViewBy, *aBuf);
			CleanupStack::PopAndDestroy();
			
			//iViewBy= ETitle;
			if (iCurrentSong.Length()>0) SetCurrent(iCurrentSong);
			break;
		}
		if (iViewBy==ETitle || iViewBy==EFileName) {
			if (curFile.Length()>0) {
				if (iOggPlayback->State()==CAbsPlayback::EPaused && idx==iCurrent)
						iOggPlayback->Play();
        else {
				  if (iOggPlayback->State()==CAbsPlayback::EPlaying)
            iOggPlayback->Pause();
				  if (iOggPlayback->Open(curFile)==KErrNone) {
					  iOggPlayback->SetVolume(iVolume);
					  if (iOggPlayback->State()!=CAbsPlayback::EPlaying) {
						  iAppView->SetTime(iOggPlayback->Time());
						  iOggPlayback->Play();
					  }
					  SetCurrent(idx);
				  } else SetCurrent(-1);
        }
			}
		}
		break;
				   }
		
	case EOggStop: {
		if (iOggPlayback->State()==CAbsPlayback::EPlaying ||
			iOggPlayback->State()==CAbsPlayback::EPaused) {
			iOggPlayback->Stop();
			SetCurrent(-1);
		}
		break;
				   }
		
	case EOggPauseResume: {
		if (iOggPlayback->State()==CAbsPlayback::EPlaying) 
      iOggPlayback->Pause();
//		else if (iOggPlayback->State()==CAbsPlayback::EPaused) 
//      iOggPlayback->Play();
		else 
      HandleCommandL(EOggPlay);
		iAppView->Update();
		break;
						  }
		
	case EOggInfo: {
		ShowFileInfo();
		break;
				   }
		
	case EOggShuffle: {
		HandleCommandL(EOggStop);
		iAppView->ShufflePlaylist();
		break;
					  }
		
	case EOggRepeat: {
		iAppView->ToggleRepeat();
		break;
					 }
		
	case EOggOptions: {
#if !defined(SERIES60)
		CHotkeyDialog *hk = new CHotkeyDialog(&iHotkey, &iAlarmActive, &iAlarmTime);
		if (hk->ExecuteLD(R_DIALOG_HOTKEY)==EEikBidOk) {
			SetHotKey();
			iAppView->SetAlarm();
		}
#else
    ActivateOggViewL(KOggPlayUidSettingsView);
//    ActivateLocalViewL(KOggPlayUidSettingsView);
#endif
		break;
					  }
		
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
        {
			TBuf<16> dummy;
			//iViewBy= aCommand-EOggViewByTitle;
			iAppView->FillView((TViews)(aCommand-EOggViewByTitle), ETop, dummy);
			if (iCurrentSong.Length()>0) SetCurrent(iCurrentSong);
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
		iAppView->ReadSkin(buf);
		iAppView->Update();
		iAppView->Invalidate();
		break;
	  }
		
#if defined(SERIES60)
  case EAknSoftkeyBack: {
		HandleCommandL(EOggStop);
    Exit();
    break;
  }
  case EUserStopPlayingCBA : {
 		HandleCommandL(EOggStop);
    break;
    }

  case EOggUserHotkeys :
    ActivateOggViewL(KOggPlayUidUserView);
    break;

  case EUserHotKeyCBABack: {
    ActivateOggViewL(KOggPlayUidFOView);
    }
    break;
#endif

	case EEikCmdExit: {
		Exit();
		break;
    }
  }
}

void
COggPlayAppUi::ShowFileInfo()
{
	if (iCurrentSong.Length()==0 && iAppView->GetSelectedIndex()<0) return;
	if (iCurrentSong.Length()==0) {
		// no song is playing, show info for selected song if possible
		if (iAppView->GetItemType(iAppView->GetSelectedIndex())==6) return;
		if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==ETop) return;
		if (iOggPlayback->Info(iAppView->GetFileName(iAppView->GetSelectedIndex()))!=KErrNone) return;
	}
	COggInfoDialog *d = new COggInfoDialog();
	d->SetFileName(iOggPlayback->FileName());
	d->SetRate(iOggPlayback->Rate());
	d->SetChannels(iOggPlayback->Channels());
	d->SetFileSize(iOggPlayback->FileSize());
	d->SetTime(iOggPlayback->Time().GetTInt());
	d->SetBitRate(iOggPlayback->BitRate()/1000);
	if (iCurrent<0) iOggPlayback->ClearComments();
	d->ExecuteLD(R_DIALOG_INFO);
}

void
COggPlayAppUi::SetCurrent(int aCurrent)
{
	iCurrent= aCurrent;
	iCurrentSong= iAppView->GetFileName(aCurrent);
	if (iAppView->GetItemType(aCurrent)==6) iCurrentSong.SetLength(0);
	iAppView->Update();
}

void
COggPlayAppUi::SetCurrent(const TDesC& aFileName)
{
	iCurrent= -1;
	for (TInt i=0; i<iAppView->GetTextArray()->Count(); i++) {
		const TDesC& fn= iAppView->GetFileName(i);
		if (fn==aFileName) {
			iCurrent=i;
			iCurrentSong= aFileName;
			break;
		}
	}
	iAppView->Update();
}

void
COggPlayAppUi::NextSong()
{
	// This function guarantees that a next song will be played.
	// This is important since it is also being used if an alarm is triggered.
	// If neccessary the current view will be switched.
	
	if (iViewBy==ETop) {
		HandleCommandL(EOggPlay); // select the current category
		HandleCommandL(EOggPlay); // select the 1st song in that category
		HandleCommandL(EOggPlay); // play it!
	} else if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) {
		HandleCommandL(EOggPlay); // select the 1st song in the current category
		HandleCommandL(EOggPlay); // play it!
	} else {
		if (iCurrent<0) {
			// if no song is playing, play the currently selected song
			if (iAppView->GetItemType(iAppView->GetSelectedIndex())==6) iAppView->SelectSong(1);
			HandleCommandL(EOggPlay); // play it!
		} else {
			// if a song is currently playing, find and play the next song
			if (iCurrent+1<iAppView->GetNSongs()) {
				if (iAppView->GetItemType(iCurrent+1)==6) iCurrent++;
				if (iCurrent+1<iAppView->GetNSongs()) iAppView->SelectSong(iCurrent+1);
				else {
					HandleCommandL(EOggStop);
					return;
				}
				HandleCommandL(EOggPlay);
			} else HandleCommandL(EOggStop);
		}
	}
}

void
COggPlayAppUi::PreviousSong()
{
	if (iViewBy==ETop) {
		HandleCommandL(EOggPlay); // select the current category
		HandleCommandL(EOggPlay); // select the 1st song in that category
		iAppView->SelectSong(iAppView->GetNSongs()-1); // select the last song in that category
		HandleCommandL(EOggPlay); // play it!
	} else if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) {
		HandleCommandL(EOggPlay); // select the 1st song in the current category
		iAppView->SelectSong(iAppView->GetNSongs()-1); // select the last song in that category
		HandleCommandL(EOggPlay); // play it!
	}
	else {
		if (iCurrent<0) {
			// if no song is playing, start playing the selected song if possible
			if (iAppView->GetItemType(iAppView->GetSelectedIndex())!=6) HandleCommandL(EOggPlay);
			return;
		} else {
			// if a song is playing, find and play the previous song if possible
			if (iCurrent-1>=0) {
				iAppView->SelectSong(iCurrent-1);
				if (iAppView->GetItemType(iCurrent-1)==6) HandleCommandL(EOggStop);
				else HandleCommandL(EOggPlay);
			} else HandleCommandL(EOggStop);
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
        if (iRepeat) 
            aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);
#else
    // FIXIT - Should perhaps be in the options menu instead ??
    TBuf<50> buf;
    iEikonEnv->ReadResource(buf, (iRepeat) ? R_OGG_REPEAT_ON : R_OGG_REPEAT_OFF );
    aMenuPane->SetItemTextL( EOggRepeat, buf );
#endif
		TBool isSongList= ((iViewBy==ETitle) || (iViewBy==EFileName));
		aMenuPane->SetItemDimmed(EOggInfo   , iCurrentSong.Length()==0 && (iAppView->GetSelectedIndex()<0 || !isSongList));
		aMenuPane->SetItemDimmed(EOggShuffle, iAppView->GetNSongs()<1 || !isSongList);
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
	
#if !defined(SERIES60)
	if (aMenuId==R_POPUP_MENU) {
		if (iRepeat) aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);
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
  TInt iIniversion=0;
	if( (err=in.Open(iCoeEnv->FsSession(), iIniFileName->Des(),
		EFileRead|EFileStreamText)) != KErrNone) {
		TRACEF(COggLog::VA(_L("ReadIni:%d"), err ));
		return;
	}
	
	TFileText tf;
	tf.Set(in);
	TBuf<128> line;


	iHotkey= 0;
	if(tf.Read(line) == KErrNone) {
    TInt rval;
		TLex parse(line);
		parse.Val(rval);
    if(rval==0xdb) { // our magic number ! (see writeini below)
      // followed by version number
	    if (tf.Read(line) == KErrNone) {
		    parse=line;
		    parse.Val(iIniversion);
        TRACEF(COggLog::VA(_L("Inifile version %d"),iIniversion));
    	};
      // now read the real hotkey
	    if (tf.Read(line) == KErrNone) {
		    parse=line;
		    parse.Val(rval);
    	};
    }
    iHotkey=rval;
	};
	
	iRepeat= 1;
	if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		parse.Val(iRepeat);
	};
	
	iVolume= KMaxVolume;
	if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		parse.Val(iVolume);
		if (iVolume<0) iVolume= 0;
		else if (iVolume>KMaxVolume) iVolume= KMaxVolume;
	};
	
	iAlarmTime.Set(_L("20030101:120000.000000"));
	if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		TInt64 i64;
		if (parse.Val(i64)==KErrNone) {
			TTime t(i64);
			iAlarmTime= t;
		}
	}
	
	iAnalyzerState= 0;
	if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		parse.Val(iAnalyzerState);
	}
	
	if (tf.Read(line) == KErrNone) {
		// for backward compatibility
		// this line is not used anymore
	}
	
	iCurrentSkin= 0;
	if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		parse.Val(iCurrentSkin);
		if (iCurrentSkin<0) iCurrentSkin=0;
		if (iCurrentSkin>=iSkins->Count()) iCurrentSkin= iSkins->Count()-1;
	}

  if(iIniversion>=2) {

    TInt iRestoreStackCount=0;
  if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		  parse.Val(iRestoreStackCount);      
	  }

    TInt iTemp;
    for(TInt i=0;i<iRestoreStackCount;i++) {
     if (tf.Read(line) == KErrNone) {
		    TLex parse(line);
		    parse.Val(iTemp);
        iRestoreStack.Append(iTemp);
      }
    }
	  if (tf.Read(line) == KErrNone) {
		  TLex parse(line);
		  parse.Val(iRestoreCurrent);
	  }
  
  if (tf.Read(line) == KErrNone) {
		  TLex parse(line);
		  parse.Val(iSettings.iScanmode);
	  };

  if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		parse.Val(iSettings.iAutoplay);
	};
	
  if (tf.Read(line) == KErrNone) {
		TLex parse(line);
		parse.Val(iSettings.iManeuvringSpeed);
	};

  for( TInt j=TOggplaySettings::KFirstHotkeyIndex; j<TOggplaySettings::ENofHotkeys; j++ ) 
    {
    if (tf.Read(line) == KErrNone) {
		  TLex parse(line);
      parse.Val(iSettings.iUserHotkeys[j]);
      }
  	}

  if (tf.Read(line) == KErrNone) {
      TLex parse(line);
      parse.Val(iSettings.iWarningsEnabled);
  };

  } // version 2 onwards

	in.Close();
}

void
COggPlayAppUi::WriteIniFile()
{
	RFile out;
	TRACE(COggLog::VA(_L("COggPlayAppUi::WriteIniFile() %S"), iIniFileName ));
	if(out.Replace(iCoeEnv->FsSession(), iIniFileName->Des(),
		EFileWrite|EFileStreamText) != KErrNone) return;
  
  TRACEF(_L("Writing Inifile..."));

	TFileText tf;
	tf.Set(out);
	
	TBuf<64> num;

  // this should do the trick for forward compatibility:
  TInt magic=0xdb;
  TInt iniversion=3;
  
  num.Num(magic);
  tf.Write(num);
  num.Num(iniversion);
  tf.Write(num);

	num.Num(iHotkey);
	tf.Write(num);
	
	num.Num(iRepeat);
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
  
  for(TInt i=0;i<iViewHistoryStack.Count();i++) {
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

  for( TInt j=TOggplaySettings::KFirstHotkeyIndex; j<TOggplaySettings::ENofHotkeys; j++ ) 
    {
    num.Num(iSettings.iUserHotkeys[j]);
	  tf.Write(num);
  	}

 	num.Num(iSettings.iWarningsEnabled);
	tf.Write(num);

	//please increase iIniversion when adding stuff
	
	out.Close();
}

void 
COggPlayAppUi::HandleForegroundEventL(TBool aForeground)
{
	CEikAppUi::HandleForegroundEventL(aForeground);
	iForeground= aForeground;
}


#if defined(SERIES60_SPLASH)
void 
COggPlayAppUi::ShowSplash()
{
	// This is a so-called pre Symbian OS v6.1 direct screen drawing rutine
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
			T.SetPriority(EPriorityAbsoluteHigh);
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
		iAppView->SelectSong(0);
		iIsRunningEmbedded = ETrue;
	}
	if (iDoorObserver) {
		iDoorObserver->NotifyExit(MApaEmbeddedDocObserver::ENoChanges); 
	}

}

////////////////////////////////////////////////////////////////
//
// COggPlayDocument class
//
///////////////////////////////////////////////////////////////
#ifdef SERIES60
CFileStore* COggPlayDocument::OpenFileL(TBool /*aDoOpen*/,const TDesC& aFilename, RFs& /*aFs*/){
	iAppUi->OpenFileL(aFilename);
	return NULL;
}
#endif

CEikAppUi* COggPlayDocument::CreateAppUiL(){
    iAppUi = new (ELeave) COggPlayAppUi;
    return iAppUi;	
}
