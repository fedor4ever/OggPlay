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

#if defined(SERIES60)
#include <aknnotewrappers.h>
#include <aknmessagequerydialog.h>
_LIT(KTsyName,"phonetsy.tsy");
#endif

#if defined(SERIES80)
#include <eikEnv.h>
#include "OggDialogsS80.h"
_LIT(KTsyName,"phonetsy.tsy");
#endif

#if defined(UIQ)
#include <quartzkeys.h>	// EStdQuartzKeyConfirm etc.
_LIT(KTsyName,"erigsm.tsy");
#endif

#include <eiklabel.h>   // CEikLabel
#include <gulicon.h>	// CGulIcon
#include <apgwgnam.h>	// CApaWindowGroupName
#include <eikmenup.h>	// CEikMenuPane
#include <barsread.h>

#include "OggControls.h"

#if defined(PLUGIN_SYSTEM)
#include "OggPluginAdaptor.h"
#else
#include "OggTremor.h"
#endif

#include "OggPlayAppView.h"
#include "OggDialogs.h" 
#include "OggLog.h"
#include "OggFilesSearchDialogs.h"


#if !defined(SERIES60V3)
// Minor compatibility tweak
#define ReadResourceL ReadResource
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

CApaDocument* COggPlayApplication::CreateDocumentL()
	{
	return new (ELeave)COggPlayDocument(*this);
	}

TUid COggPlayApplication::AppDllUid() const
	{
	TUid id = { KOggPlayApplicationUidValue };
	return id;
	}

// COggPlayDocument class
CFileStore* COggPlayDocument::OpenFileL(TBool /*aDoOpen*/,const TDesC& aFilename, RFs& /*aFs*/)
	{
	iAppUi->OpenFileL(aFilename);
	return NULL;
	}

#if defined(SERIES60)
COggPlayDocument::COggPlayDocument(CAknApplication& aApp)
#else
COggPlayDocument::COggPlayDocument(CEikApplication& aApp)
#endif
: CEikDocument(aApp)
	{
	}

CEikAppUi* COggPlayDocument::CreateAppUiL()
	{
    CEikAppUi* appUi = new (ELeave) COggPlayAppUi;
    return appUi;
	}


#if defined(MONITOR_TELEPHONE_LINE)
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
			TRACEF(_L("GSM is active"));
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
			TRACEF(_L("GSM is idle"));

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


COggStartUpAO::COggStartUpAO(COggPlayAppUi& aAppUi)
: CActive(EPriorityHigh), iAppUi(aAppUi)
	{
	CActiveScheduler::Add(this);
	}

COggStartUpAO::~COggStartUpAO()
	{
	}

void COggStartUpAO::NextStartUpState()
	{
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrNone);

	SetActive();
	}

void COggStartUpAO::RunL()
	{
	iAppUi.NextStartUpState();
	}

void COggStartUpAO::DoCancel()
	{
	}


COggStartUpEmbeddedAO::COggStartUpEmbeddedAO(COggPlayAppUi& aAppUi, TOggFiles& aFiles)
: CActive(EPriorityHigh), iAppUi(aAppUi), iFiles(aFiles)
	{
	CActiveScheduler::Add(this);
	}

COggStartUpEmbeddedAO::~COggStartUpEmbeddedAO()
	{
	}

void COggStartUpEmbeddedAO::CreateDbWithSingleFile(TFileName& aFileName)
	{
	iFiles.CreateDbWithSingleFile(aFileName, iStatus);
	SetActive();
	}

void COggStartUpEmbeddedAO::RunL()
	{
	iAppUi.RunningEmbeddedDbReady(iStatus.Int());
	}

void COggStartUpEmbeddedAO::DoCancel()
	{
	iFiles.CancelOutstandingRequest();
	}


// App UI class, COggPlayAppUi
COggPlayAppUi::COggPlayAppUi()
: iViewBy(ETop)
	{
	}

_LIT(KLogFile,"C:\\Logs\\OggPlay\\OggPlay.log");
void COggPlayAppUi::ConstructL()
	{
	// Delete the existing log
	iCoeEnv->FsSession().Delete(KLogFile);

	// Trace that we have at least reached the start of the code
	TRACEF(_L("COggPlayAppUi::ConstructL()"));

#if defined(SERIES60SUI)
	BaseConstructL(EAknEnableSkin);
#else
	BaseConstructL();
#endif

	// Construct the splash view
	TRACEF(_L("COggPlayAppUi::ConstructL(): Construct Splash views..."));

    iSplashFOView = new(ELeave) COggSplashView(KOggPlayUidSplashFOView);
	iSplashFOView->ConstructL();
	RegisterViewL(*iSplashFOView);

#if defined(UIQ)
    iSplashFCView = new(ELeave) COggSplashView(KOggPlayUidSplashFCView);
	iSplashFCView->ConstructL();
	RegisterViewL(*iSplashFCView);
#endif

	// Construct required startup objects
	TRACEF(_L("COggPlayAppUi::ConstructL(): Construct StartUp objects..."));
	iStartUpAO = new(ELeave) COggStartUpAO(*this);

#if defined(SERIES60)
	iStartUpErrorDlg = new(ELeave) CAknErrorNote(ETrue);
#endif

	iEikonEnv->ReadResourceL(iUnassignedTxt, R_OGG_STRING_13);
	iEikonEnv->ReadResourceL(iStartUpErrorTxt1, R_OGG_ERROR_31);
	iEikonEnv->ReadResourceL(iStartUpErrorTxt2, R_OGG_ERROR_32);

	// Wait for the view server to activate the splash
	// view before completing the startup process
	iStartUpState = EActivateSplashView;
	}

void COggPlayAppUi::NextStartUpState(TInt aErr)
	{
	TRACEF(COggLog::VA(_L("COggPlayAppUi::NextStartupState: %d %d"), (TInt) iStartUpState, aErr));		
	if (iStartUpState == EStartUpFailed)
		return;

	if (aErr == KErrNone)
		iStartUpAO->NextStartUpState();
	else
		StartUpError(aErr);
	}

void COggPlayAppUi::NextStartUpState()
{
	switch(iStartUpState)
	{
	case EActivateSplashView:
		iStartUpState = EStartOggPlay;
		StartOggPlay();
		break;

	case EStartOggPlay:
		iStartUpState = EActivateStartUpView;
		ActivateStartUpView();
		break;

	case EActivateStartUpView:
		iStartUpState = EPostConstruct;
		PostConstruct();
		break;

	case EPostConstruct:
		iStartUpState = EStartRunningEmbedded;
		StartRunningEmbedded();
		break;

	case EStartRunningEmbedded:
		iStartUpState = EStartUpComplete;

		// Delete the startup AOs, we have finished with them now
		delete iStartUpAO;
		iStartUpAO = NULL;

		delete iStartUpEmbeddedAO;
		iStartUpEmbeddedAO = NULL;

#if defined(SERIES60)
		// Delete the startup error dialog, we have finished with that too
		delete iStartUpErrorDlg;
		iStartUpErrorDlg = NULL;
#endif
		break;

	default:
		User::Panic(_L("COPAUI::NSUS"), 0);
		break;
	}
}

void COggPlayAppUi::StartOggPlay()
	{
	TRAPD(err, StartOggPlayL());

#if defined(PLUGIN_SYSTEM)
	NextStartUpState(err);
#else
	if (err != KErrNone)
		StartUpError(err);
#endif
	}

void COggPlayAppUi::ActivateStartUpView()
	{
	TRAPD(err, ActivateStartUpViewL());
	NextStartUpState(err);
	}

void COggPlayAppUi::ActivateStartUpViewL()
	{
	iFOView = new(ELeave) COggFOView(*iAppView);
	RegisterViewL(*iFOView);  

#if defined(UIQ) || defined(SERIES60SUI)
	iFCView = new(ELeave) COggFCView(*iAppView);
	RegisterViewL(*iFCView);
#endif

#if defined(SERIES60)
	iSettingsView = new(ELeave) COggSettingsView(*iAppView);
	RegisterViewL(*iSettingsView);

	iUserHotkeysView = new(ELeave) COggUserHotkeysView(*iAppView);
	RegisterViewL(*iUserHotkeysView);

#if defined(PLUGIN_SYSTEM)
    iCodecSelectionView = new(ELeave) COggPluginSettingsView(*iAppView);
    RegisterViewL(*iCodecSelectionView);
#else
	iPlaybackOptionsView = new(ELeave) COggPlaybackOptionsView(*iAppView);
	RegisterViewL(*iPlaybackOptionsView);
#endif

	iAlarmSettingsView = new(ELeave) COggAlarmSettingsView(*iAppView);
	RegisterViewL(*iAlarmSettingsView);
#endif

	ActivateOggViewL();
	}

void COggPlayAppUi::StartUpError(TInt aErr)
	{
	// Format a string with the state and error code
	TBuf<128> errorTxt;
	errorTxt.Format(iStartUpErrorTxt2, iStartUpState, aErr);

	// Mark that the startup has failed
	iStartUpState = EStartUpFailed;

	// Display an error dialog and exit
#if defined(SERIES60)
	TRAPD(err, iStartUpErrorDlg->ExecuteLD(errorTxt));
	iStartUpErrorDlg = NULL;
#else
	TRAPD(err, iEikonEnv->InfoWinL(iStartUpErrorTxt1, errorTxt));
#endif

	Exit();
	}

void COggPlayAppUi::StartOggPlayL()
	{
	TFileName fileName(Application()->AppFullName());
	TParsePtr parse(fileName);

#if defined(SERIES60V3)
	TFileName privatePath;
	User::LeaveIfError(iCoeEnv->FsSession().PrivatePath(privatePath));

	TBuf<2> appDriveLetter(parse.Drive());
	iIniFileName.Copy(appDriveLetter);
	iIniFileName.Append(privatePath);
	iIniFileName.Append(_L("OggPlay.ini"));
 
	iDbFileName.Copy(appDriveLetter);
	iDbFileName.Append(privatePath);
	iDbFileName.Append(_L("OggPlay.db"));

	iSkinFileDir.Copy(appDriveLetter);
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

	TTime homeTime;
	homeTime.HomeTime();
	iRndSeed = homeTime.Int64();
	iRndMax = KMaxTInt ; iRndMax++;

	iSkins= new(ELeave) CDesCArrayFlat(3);
	FindSkins();
	
	iOggMsgEnv = new(ELeave) COggMsgEnv(iSettings.iWarningsEnabled);

#if defined(PLUGIN_SYSTEM)
	iOggPlayback = new(ELeave) COggPluginAdaptor(iOggMsgEnv, this);
#else
	iOggPlayback = new(ELeave) COggPlayback(iOggMsgEnv, this);
#endif

	iOggPlayback->ConstructL();

	ReadIniFile();

 	iAppView = new(ELeave) COggPlayAppView;
	iAppView->ConstructL(this);

	SetRandomL(iSettings.iRandom);

#if defined(SERIES60SUI)
	// Install the font.
	// We don't use add because then the font would be removed when the app exits.
	// That would be fine, except there seems to be a bug whereby once the font
	// is removed it can't be added again. Besides removing doesn't even do what you want,
	// it should release the lock on the font file and allow the application to be uninstalled. It doesn't!
	
	// Changed my mind! Unbelievably installing a font causes other applications to randomly start
	// rendering some of their characters using OggPlay's font. This is crazy!
	// iTypefaceStore = CFbsTypefaceStore::NewL(iCoeEnv->ScreenDevice());

	// TFileName fontFile;
	// fontFile.Copy(appDriveLetter);
	// fontFile.Append(_L("\\OggPlayFonts\\OggPlayVera.ttf"));

#if defined(__WINS__)
	// fontFile.Copy(_L("C:\\OggPlayFonts\\OggPlayVera.ttf"));
#endif

	// User::LeaveIfError(iTypefaceStore->InstallFile(fontFile, iFontId));
#endif

	ReadSkin(iCurrentSkin);
	
	AddToStackL(iAppView); // Receiving Keyboard Events 
	iAppView->InitView();

	SetHotKey();

#if defined(MONITOR_TELEPHONE_LINE)
	iActive = new(ELeave) COggActive;
	iActive->ConstructL(this);
	iActive->IssueRequest();
#endif

	// Resist any attempts by the OS to give us anything other than foreground priority
	// Disable process priority changes
	CEikonEnv::Static()->WsSession().ComputeMode(RWsSession::EPriorityControlDisabled);

	// Change the process priority to EPriorityForeground
	RProcess process;
	process.SetPriority(EPriorityForeground);
	}

void COggPlayAppUi::PostConstruct()
	{
	TRAPD(err, PostConstructL());
	NextStartUpState(err);
	}

void COggPlayAppUi::PostConstructL()
{
	// Ideally we just de-register (and delete) the splash view now that we are done with it. 
	// However, on S60 V1 we can't do this if OggPlay is running embedded: for some reason we get a CONE 30 panic
	// if the apps key is pressed (the app that embeds us must try to access the splash view I guess)
	// Similarly S90 doesn't like us de-registering the splash view either :-(
#if !defined(SERIES90)
	if (!iIsRunningEmbedded)
		{
		// Delete the splash view. We won't be seeing it again
		DeregisterView(*iSplashFOView);

		delete iSplashFOView;
		iSplashFOView = NULL;
		}
#endif

#if defined(UIQ)
	// In FC mode the splash view is also used to launch
	// the main FC view, so we can't de-register it here, :-(
	// DeregisterView(*iSplashFCView);

	// delete iSplashFCView;
	// iSplashFCView = NULL;
#endif

	// Set the playback volume
	iOggPlayback->SetVolume(iVolume);

	// Start a song playing if autoplay is enabled
	if (iSettings.iAutoplay && !iIsRunningEmbedded)
		{
		// Restore the view
		for (TInt i = 0 ; i<iRestoreStack.Count() ; i++)
			{
			iAppView->SelectItem(iRestoreStack[i]);
			HandleCommandL(EOggPlay);
			}

		iAppView->SelectItem(iRestoreCurrent);

		// Start playing
		NextSong();
		}

	// Restart display updates
	iAppView->RestartCallBack();

	// Initialise the alarm
	iAppView->SetAlarm();
	}

void COggPlayAppUi::StartRunningEmbedded()
	{
	if (iIsRunningEmbedded)
		{
		iStartUpEmbeddedAO = new(ELeave) COggStartUpEmbeddedAO(*this, *(iAppView->iOggFiles));
		iStartUpEmbeddedAO->CreateDbWithSingleFile(iFileName);
		}
	else
		NextStartUpState(KErrNone);
	}

void COggPlayAppUi::RunningEmbeddedDbReady(TInt aErr)
	{
	if (aErr == KErrNone)
		{
		// No repeat when running in embedded mode, override the default iSettings.iRepeat
		iSongList->SetRepeat(EFalse);

		// Start playing
		NextSong();
		}

#if defined(SERIES80)
	// Apparently on S80 we are always running embedded.
	// The app is launched with C:\MyFiles\OggPlay as the filename

	// Consequently we just fall back to the previous method of ignoring errors
	// Not ideal really as it would be better to give the user a startup error (if there really is an error)
	if (aErr != KErrNone)
		{
		aErr = KErrNone;
		iIsRunningEmbedded = EFalse;
		}
#endif

	NextStartUpState(aErr);
	}

COggPlayAppUi::~COggPlayAppUi()
	{
#if defined(SERIES60SUI)
	// if (iTypefaceStore)
		{
		// This doesn't release the font as it should (the file locks are not removed)
		// Worse still it also stops the font from ever being added successfully again!
		// iTypefaceStore->RemoveFile(iFontId);
		// TRAP_IGNORE(iTypefaceStore->RemoveFontFileLocksL());
		}
	// delete iTypefaceStore;
#endif

	if (iDoorObserver)
		iDoorObserver->NotifyExit(MApaEmbeddedDocObserver::ENoChanges); 

	if (iStartUpEmbeddedAO)
		iStartUpEmbeddedAO->Cancel();
	delete iStartUpEmbeddedAO;

	if (iStartUpAO)
		iStartUpAO->Cancel();
	delete iStartUpAO;

#if defined (SERIES60)
	delete iStartUpErrorDlg;
#endif

	if (iSplashFOView)
		{
		DeregisterView(*iSplashFOView);
		delete iSplashFOView;
		}

	if (iFOView)
		{
		DeregisterView(*iFOView);
		delete iFOView;
		}

	if (iSplashFCView)
		{
		DeregisterView(*iSplashFCView);
		delete iSplashFCView;
		}

	if (iFCView)
		{
		DeregisterView(*iFCView);
		delete iFCView;
		}

#if defined(SERIES60)
	if (iSettingsView)
		{
		DeregisterView(*iSettingsView);
		delete iSettingsView;
		}

	if (iUserHotkeysView)
		{
		DeregisterView(*iUserHotkeysView);
		delete iUserHotkeysView;
		}

#if defined(PLUGIN_SYSTEM)
	if (iCodecSelectionView)
		{
		DeregisterView(*iCodecSelectionView);
		delete iCodecSelectionView;
		}
#endif

	if (iPlaybackOptionsView)
		{
		DeregisterView(*iPlaybackOptionsView);
  		delete iPlaybackOptionsView;
		}

	if (iAlarmSettingsView)
		{
		DeregisterView(*iAlarmSettingsView);
  		delete iAlarmSettingsView;
		}
#endif // SERIES60

	if (iAppView)
		RemoveFromStack(iAppView);
	delete iAppView;

#if defined(MONITOR_TELEPHONE_LINE)
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
  TUid viewUid;
  viewUid = iAppView->IsFlipOpen() ? KOggPlayUidFOView : KOggPlayUidFCView;
  ActivateOggViewL(viewUid);
}

void COggPlayAppUi::ActivateOggViewL(const TUid aViewId)
{
	TVwsViewId viewId;
	viewId.iAppUid = KOggPlayUid;
	viewId.iViewUid = aViewId;
	
	iMainViewActive = (aViewId == KOggPlayUidFOView) || (aViewId == KOggPlayUidFCView);
	ActivateViewL(viewId);
}

void COggPlayAppUi::HandleApplicationSpecificEventL(TInt aType, const TWsEvent& aEvent)
{
	if (aType == EEventUser+1)
		ActivateOggViewL();
	else 
		CEikAppUi::HandleApplicationSpecificEventL(aType, aEvent);
}

#if !defined(PLUGIN_SYSTEM)
void COggPlayAppUi::NotifyStreamOpen(TInt aErr)
	{
	NextStartUpState(aErr);
	}
#endif

void COggPlayAppUi::NotifyUpdate()
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

void COggPlayAppUi::NotifyPlayInterrupted()
	{
	HandleCommandL(EOggPauseResume);
	iTryResume = 2;
	}

void COggPlayAppUi::NotifyPlayComplete()
	{
	NextSong();
	}

#if defined(DELAY_AUDIO_STREAMING_OPEN)
void COggPlayAppUi::NotifyFileOpen(TInt aErr)
	{
	if (aErr != KErrNone)
		{
		FileOpenErrorL(aErr);
		HandleCommandL(EOggStop);
		return;
		}

	iOggPlayback->Play();

#if !defined(DELAY_AUDIO_STREAMING_START)
	iAppView->SetTime(iOggPlayback->Time());
	iAppView->Update();
	UpdateSoftkeys();
#endif
	}
#endif

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

void COggPlayAppUi::NotifyFatalPlayError()
	{
	// Try to exit cleanly
	HandleCommandL(EEikCmdExit);
	}

void COggPlayAppUi::SetHotKey()
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
	// Return if somebody else than main view has taken focus, except if we override this check.
	// Others must control the CBA themselves
	TVwsViewId viewId;
	GetActiveViewId(viewId);
	if (!aForce && (viewId.iViewUid != KOggPlayUidFOView) && (viewId.iViewUid != KOggPlayUidFCView)) 
		return;
  
#if defined(SERIES60)
	if(iOggPlayback->State() == CAbsPlayback::EPlaying)
		COggUserHotkeysS60::SetSoftkeys(ETrue);
	else
		COggUserHotkeysS60::SetSoftkeys(EFalse);
#else
	if(iOggPlayback->State() == CAbsPlayback::EPlaying)
		COggUserHotkeysS80::SetSoftkeys(ETrue);
	else
		COggUserHotkeysS80::SetSoftkeys(EFalse);
#endif

	Cba()->DrawNow();
#endif // S60||S80
	}

void COggPlayAppUi::HandleCommandL(TInt aCommand)
	{
	TBuf<256> buf;
	switch (aCommand)
		{
		case EOggAbout:
			{
#if defined(SERIES80)
			COggAboutDialog *d = new(ELeave) COggAboutDialog;
			d->ExecuteLD(R_DIALOG_ABOUT_S80);
#else
			COggAboutDialog *d = new(ELeave) COggAboutDialog;
			d->ExecuteLD(R_DIALOG_ABOUT);
#endif
			break;
			}

		case EOggPlay:
			PlaySelect();
			break;
		
		case EOggStop:
			Stop();
			break;
		
		case EOggPauseResume:
			PauseResume();
			break;
		
		case EOggInfo:
			ShowFileInfo();
			break;
		
		case EOggShuffle:
			SetRandomL(!iSettings.iRandom);
			break;
		
		case EOggRepeat:
		    ToggleRepeat();
			break;
		
		case EOggOptions:
			{
#if defined(UIQ)
			CHotkeyDialog *hk = new(ELeave) CHotkeyDialog(&iHotkey, &(iSettings.iAlarmActive), &(iSettings.iAlarmTime));
			if (hk->ExecuteLD(R_DIALOG_HOTKEY)==EEikBidOk)
				{
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

#if defined(PLUGIN_SYSTEM)
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

		case EOggPlaybackOptions:
			ActivateOggViewL(KOggPlayUidPlaybackOptionsView);
			break;

#if defined(SERIES60)
		case EOggAlarmSettings:
			ActivateOggViewL(KOggPlayUidAlarmSettingsView);
			break;
#endif

		case EOggNextSong:
			NextSong();
			break;
		
		case EOggPrevSong:
			PreviousSong();
			break;
		
		case EOggViewByTitle:
		case EOggViewByAlbum:
		case EOggViewByArtist:
		case EOggViewByGenre:
		case EOggViewBySubFolder:
		case EOggViewByFileName:
		case EOggViewByPlayList:
			iAppView->FillView((TViews)(aCommand-EOggViewByTitle), ETop, buf);
			break;

		case EOggViewRebuild:
			{
			iIsRunningEmbedded = EFalse;
			HandleCommandL(EOggStop);

#if defined(SEARCH_OGGS_FROM_ROOT)
			COggFilesSearchDialog *d = new(ELeave) COggFilesSearchDialog(iAppView->iOggFiles);
			if (iSettings.iScanmode == TOggplaySettings::EMmcOgg)
				iAppView->iOggFiles->SearchSingleDrive(KMmcSearchDir, d, R_DIALOG_FILES_SEARCH, iCoeEnv->FsSession());
#if defined(SERIES80)
			else if (iSettings.iScanmode == TOggplaySettings::ECustomDir)
				iAppView->iOggFiles->SearchSingleDrive(iSettings.iCustomScanDir, d, R_DIALOG_FILES_SEARCH, iCoeEnv->FsSession());
#endif
			else
				iAppView->iOggFiles->SearchAllDrives(d, R_DIALOG_FILES_SEARCH, iCoeEnv->FsSession());
#else
			iAppView->iOggFiles->CreateDb(iCoeEnv->FsSession());
#endif

			iAppView->iOggFiles->WriteDbL(iDbFileName, iCoeEnv->FsSession());
			iViewHistoryStack.Reset();

			iAppView->FillView(ETop, ETop, buf);
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
		case EOggSkinTen:
			ReadSkin(aCommand-EOggSkinOne);
			iAppView->HandleNewSkinLayout();
			break;

#if defined(SERIES60) || defined(SERIES80)
		case EUserStopPlayingCBA:
 			HandleCommandL(EOggStop);
			break;

		case EUserPauseCBA:
			iOggPlayback->Pause(); // does NOT call UpdateS60Softkeys
			iAppView->Update();
			UpdateSoftkeys();
			break;

		case EUserPlayCBA:
			HandleCommandL(EOggPlay);
			iAppView->Update();
			break;

		case EUserBackCBA:
			SelectPreviousView();
			break;

		case EOggUserHotkeys:
			ActivateOggViewL(KOggPlayUidUserView);
			break;

		case EUserHotKeyCBABack:
			ActivateOggViewL();
			break;
	
		case EUserFastForward:
			iAppView->OggSeek(KFfRwdStep);
			break;

		case EUserRewind:
			iAppView->OggSeek(-KFfRwdStep);
			break;

		case EUserListBoxPageDown:
			iAppView->ListBoxPageDown();
			break;

		case EUserListBoxPageUp:
			iAppView->ListBoxPageUp();
			break;

		case EUserVolumeHelp: 
			iEikonEnv->ReadResourceL(buf, R_OGG_STRING_14);
			User::InfoPrint(buf);
			break;

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

#if defined(SERIES60)
		case EAknSoftkeyBack:
#endif
		case EEikCmdExit:
			// Only do exit operations if we have actually finished starting up
			if (iStartUpState == EStartUpComplete)
				{
				HandleCommandL(EOggStop);
				WriteIniFile();
				}

			Exit();
			break;
		}
	}

void COggPlayAppUi::PlaySelect()
	{
	TInt idx = iAppView->GetSelectedIndex();

	if (iViewBy==ETop)
		{
		SelectNextView();
		return;
		}

	if (iAppView->GetItemType(idx) == COggListBox::EBack)
		{
		// the back item was selected: show the previous view
		SelectPreviousView();
		return;
		}

	if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy == EPlayList) 
		{
		SelectNextView();
		return;
		}

	if (iViewBy==ETitle || iViewBy==EFileName)
		{      
        if ((iOggPlayback->State()==CAbsPlayback::EPaused) &&
            (iSongList->IsSelectedFromListBoxCurrentlyPlaying()))
			{
            iOggPlayback->Resume();

			#if !defined(DELAY_AUDIO_STREAMING_START)
			iAppView->Update();
			UpdateSoftkeys();
			#endif

			return;
			}

		iOggPlayback->Stop();
		iSongList->SetPlayingFromListBox(idx);

		iFileName = iSongList->GetPlaying();
		TInt err = iOggPlayback->Open(iFileName);
		if (err == KErrNone)
			{
#if !defined(DELAY_AUDIO_STREAMING_OPEN)
			iOggPlayback->Play();

#if !defined(DELAY_AUDIO_STREAMING_START)
			iAppView->SetTime(iOggPlayback->Time());
			iAppView->Update();
			UpdateSoftkeys();
#endif
#endif
			}
		else
			{
			FileOpenErrorL(err);

			iSongList->SetPlayingFromListBox(ENoFileSelected);
			UpdateSoftkeys();
			}
		}
	}

void COggPlayAppUi::PauseResume()
	{  
	if (iOggPlayback->State() == CAbsPlayback::EPlaying)
		{
		iOggPlayback->Pause();
		iAppView->Update();
		UpdateSoftkeys();
		}
	else if (iOggPlayback->State() == CAbsPlayback::EPaused)
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
	if (iOggPlayback->State() == CAbsPlayback::EPlaying)
		{
		iOggPlayback->Pause();
		
		iAppView->Update();
		UpdateSoftkeys();
		}
	}

void COggPlayAppUi::Stop()
	{
	iOggPlayback->Stop();
	UpdateSoftkeys();

	iSongList->SetPlayingFromListBox(ENoFileSelected); // calls iAppview->Update
	}

void COggPlayAppUi::SelectPreviousView()
	{
	if (!iViewHistoryStack.Count())
		return;

	TInt previousListboxLine = (TInt) iViewHistoryStack[iViewHistoryStack.Count()-1];
	iViewHistoryStack.Remove(iViewHistoryStack.Count()-1);

	const TInt previousView = iAppView->GetViewName(0);
	if (previousView == ETop)
		{
		TBuf<16> dummy;
		iAppView->FillView(ETop, ETop, dummy);
		}
	else
		HandleCommandL(EOggViewByTitle+previousView);

	// Select the entry which were left.
	iAppView->SelectItem(previousListboxLine);
	}

void COggPlayAppUi::SelectNextView()
	{
	TInt idx = iAppView->GetSelectedIndex();
	if (iViewBy == ETop)
		{
		if (idx>=ETitle && idx<=EPlayList)
			{
			iViewHistoryStack.Append(idx);
			HandleCommandL(EOggViewByTitle+idx);
			}

		  return;
		}
		
	if (!(iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==EPlayList))
		return;

	if (iAppView->GetItemType(idx) == COggListBox::EBack)
		return;
    
	iViewHistoryStack.Append(idx);

	TBuf<128> filter;
	iAppView->GetFilterData(idx, filter);
	if ((iViewBy == EPlayList) || (iViewBy == ESubFolder))
		iAppView->FillView(EFileName, iViewBy, filter);
	else
		iAppView->FillView(ETitle, iViewBy, filter);
	}

void COggPlayAppUi::ShowFileInfo()
	{
    TBool songPlaying = iSongList->AnySongPlaying();
	TInt selectedIndex = iAppView->GetSelectedIndex();
	if (!songPlaying && (selectedIndex<0))
		return;

	COggListBox::TItemTypes itemType;
	TOggPlayList* playList = NULL;
	TBool gotFileInfo = EFalse;
	if (!songPlaying)
		{
		// No song is playing, show info for selected song if possible
		itemType = iAppView->GetItemType(selectedIndex);
		if (itemType==COggListBox::EBack)
			return;

		if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==ETop)
			return;

		if (itemType == COggListBox::EPlayList)
			playList = (TOggPlayList*) iAppView->GetFile(selectedIndex);
		else
			{
			iFileName = iAppView->GetFileName(selectedIndex);
			TInt err = iOggPlayback->FullInfo(iFileName, *this);
			if (err != KErrNone)
				{
				FileOpenErrorL(err);
				return;
				}
			}
		}
	else
		{
		// Show info for selected song if possible, if not show info for the playing song
		TBool gotSong = ETrue;
		itemType = iAppView->GetItemType(selectedIndex);
		if (itemType==COggListBox::EBack)
			gotSong = EFalse;

		if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder || iViewBy==ETop)
			gotSong = EFalse;

		if (gotSong)
			{
			if (itemType == COggListBox::EPlayList)
				playList = (TOggPlayList*) iAppView->GetFile(selectedIndex);
			else
				{
				iFileName = iAppView->GetFileName(selectedIndex);
				TInt err = iOggPlayback->FullInfo(iFileName, *this);
				if (err != KErrNone)
					{
					FileOpenErrorL(err);
					return;
					}
				}
			}
		else if (iSongList->GetPlayingFile()->FileType() == TOggFile::EPlayList)
			{
			itemType = COggListBox::EPlayList;
			playList = (TOggPlayList *) iSongList->GetPlayingFile();
			}
		else
			{
			itemType = COggListBox::EFileName;
			gotFileInfo = ETrue;
			}
		}

	if (itemType == COggListBox::EPlayList)
		{
		const TDesC& fileName = *(playList->iFileName);
		COggPlayListInfoDialog *d = new(ELeave) COggPlayListInfoDialog(fileName, FileSize(fileName), playList->Count());
		d->ExecuteLD(R_DIALOG_PLAYLIST_INFO);
		}
	else if (gotFileInfo)
		{
		COggFileInfoDialog *d = new(ELeave) COggFileInfoDialog(iOggPlayback->DecoderFileInfo());
		d->ExecuteLD(R_DIALOG_INFO);
		}
	}

void COggPlayAppUi::FileInfoCallback(TInt aErr, const TOggFileInfo& aFileInfo)
	{
	if (aErr == KErrNone)	
		{
		COggFileInfoDialog *d = NULL;
		TRAP(aErr,
			{
			d = new(ELeave) COggFileInfoDialog(aFileInfo);
			d->ExecuteLD(R_DIALOG_INFO);
			});
		}
	else
		{
		// Display error message
		FileOpenErrorL(aErr);
		}
	}

void COggPlayAppUi::NextSong()
	{
	// This function guarantees that a next song will be played.
	// This is important since it is also being used if an alarm is triggered.
	// If neccessary the current view will be switched.	
	if (iSongList->AnySongPlaying()) 
		{
        // If a song is currently playing, stop playback and play the next song (if there is one)
		iFileName = iSongList->GetNextSong();
		if (!iFileName.Length())
			{
            HandleCommandL(EOggStop);
			return;
			}

		iOggPlayback->Stop();
		TInt err = iOggPlayback->Open(iFileName);
        if (err == KErrNone)
			{
#if !defined(DELAY_AUDIO_STREAMING_OPEN)
            iOggPlayback->Play();

#if !defined(DELAY_AUDIO_STREAMING_START)
            iAppView->SetTime(iOggPlayback->Time());
			UpdateSoftkeys();
            iAppView->Update();
#endif
#endif
			}
		else
			{
			FileOpenErrorL(err);
            HandleCommandL(EOggStop);
			}
		}
    else
		{
        // Switch view 
        if (iViewBy == ETop)
			{
            HandleCommandL(EOggPlay); // select the current category
            HandleCommandL(EOggPlay); // select the 1st song in that category
            HandleCommandL(EOggPlay); // play it!
			}
		else if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder)
			{
            HandleCommandL(EOggPlay); // select the 1st song in the current category
            HandleCommandL(EOggPlay); // play it!
			}
		else
			{
            // if no song is playing, play the currently selected song
			if (iAppView->GetItemType(iAppView->GetSelectedIndex())==COggListBox::EBack)
				iAppView->SelectItem(1);

			HandleCommandL(EOggPlay); // play it!
			} 
		}
	}

void COggPlayAppUi::PreviousSong()
	{
	if (iSongList->AnySongPlaying()) 
		{
        // If a song is currently playing, stop playback and play the previous song (if there is one)
		iFileName = iSongList->GetPreviousSong();
		if (!iFileName.Length())
			{
            HandleCommandL(EOggStop);
			return;
			}

		iOggPlayback->Stop();
		TInt err = iOggPlayback->Open(iFileName);
		if (err == KErrNone)
			{
#if !defined(DELAY_AUDIO_STREAMING_OPEN)
            iOggPlayback->Play();

#if !defined(DELAY_AUDIO_STREAMING_START)
			iAppView->SetTime(iOggPlayback->Time());
            UpdateSoftkeys();
            iAppView->Update();
#endif
#endif
			}
		else
			{
			FileOpenErrorL(err);
            HandleCommandL(EOggStop);
			}
		}
    else
		{
        if (iViewBy==ETop)
			{
            HandleCommandL(EOggPlay); // select the current category
            HandleCommandL(EOggPlay); // select the 1st song in that category
            HandleCommandL(EOggPlay); // play it!
			}
		else if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder)
			{
            HandleCommandL(EOggPlay); // select the 1st song in the current category
            HandleCommandL(EOggPlay); // play it!
			}
		else
			{
            // if no song is playing, play the currently selected song
			if (iAppView->GetItemType(iAppView->GetSelectedIndex())==COggListBox::EBack) iAppView->SelectItem(1);
            HandleCommandL(EOggPlay); // play it!
			}
		}
	}

void COggPlayAppUi::DynInitMenuPaneL(TInt aMenuId, CEikMenuPane* aMenuPane)
	{
	if (aMenuId == R_FILE_MENU) 
		{
#if defined(SERIES60)
		// If the splash screen is still visible we can't do anything other than exit
		if (iStartUpState != EStartUpComplete)
			{
			aMenuPane->DeleteBetweenMenuItems(0, 4);
			return;
			}
#endif

		// "Repeat" on/off entry, UIQ uses check box, Series 60 uses variable argument string.
#if defined(UIQ)
		if (iSettings.iRepeat) 
			aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);

		// UIQ_? for the random stuff
#elif defined(SERIES60)
		TBuf<50> buf;
		iEikonEnv->ReadResourceL(buf, iSettings.iRandom ? R_OGG_RANDOM_OFF : R_OGG_RANDOM_ON);
		aMenuPane->SetItemTextL(EOggShuffle, buf);
#endif

		TBool isSongList= ((iViewBy==ETitle) || (iViewBy==EFileName) || (iViewBy == EPlayList));
		aMenuPane->SetItemDimmed(EOggInfo, (!iSongList->AnySongPlaying()) && (iAppView->GetSelectedIndex()<0 || !isSongList));
		}
	
	if (aMenuId == R_SKIN_MENU)
		{
		for (TInt i = 0 ; i<iSkins->Count() ; i++)
			{
			CEikMenuPaneItem::SData item;
			item.iText.Copy((*iSkins)[i]);
			item.iText.SetLength(item.iText.Length()-4);
			item.iCommandId= EOggSkinOne+i;
			item.iCascadeId= 0;
			item.iFlags= 0;

			aMenuPane->AddMenuItemL(item);
			}
		}

#if defined(UIQ)
	if (aMenuId == R_POPUP_MENU)
		{
		if (iSettings.iRepeat)
			aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);

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
	TRAPD(err, ds->SetScanDataL(iSkinFileDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
	if (err != KErrNone)
		{
        TRACEF(COggLog::VA(_L("Error in FindSkins: %d"), err));		
		CleanupStack::PopAndDestroy(ds);
		return;
		}

	CDir* c = NULL;	
	TFileName fullname;
	for( ; ; )
		{
		ds->NextL(c);
		if (!c)
			break;
		
		for (TInt i = 0 ; i<c->Count() ; i++) 
			{
			const TEntry e= (*c)[i];
			
			fullname.Copy(ds->FullPath());
			fullname.Append(e.iName);

			TParsePtrC p(fullname);			
			if (p.Ext()==_L(".skn") || p.Ext()==_L(".SKN"))
				{
				iSkins->AppendL(p.NameAndExt());
				if (iSkins->Count()==10) break;
				}
			}

		delete c; c=NULL;
		}
	
	CleanupStack::PopAndDestroy(ds);
	}

void COggPlayAppUi::ReadIniFile()
	{
    // Set some default values (for first time users)
#if defined(SERIES60)
	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EHotKeyBack;
    iSettings.iSoftKeysPlay[0] = TOggplaySettings::EHotKeyBack;

	iSettings.iUserHotkeys[TOggplaySettings::EFastForward] = '3';
    iSettings.iUserHotkeys[TOggplaySettings::ERewind] = '1';

    iSettings.iUserHotkeys[TOggplaySettings::EPageUp] = '2';
    iSettings.iUserHotkeys[TOggplaySettings::EPageDown] = '8';

    iSettings.iUserHotkeys[TOggplaySettings::ENextSong] = '6';
    iSettings.iUserHotkeys[TOggplaySettings::EPreviousSong] = '4';

    iSettings.iUserHotkeys[TOggplaySettings::EKeylock] = '*';

    iSettings.iUserHotkeys[TOggplaySettings::EPauseResume] = '5';
	iSettings.iUserHotkeys[TOggplaySettings::EStop] = EStdKeyBackspace;

	iSettings.iUserHotkeys[TOggplaySettings::EVolumeBoostUp] = '9';
	iSettings.iUserHotkeys[TOggplaySettings::EVolumeBoostDown] = '7';

    iSettings.iScanmode = TOggplaySettings::EFullScan;
#else
#if defined(SERIES80)
	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EPlay;
    iSettings.iSoftKeysIdle[1] = TOggplaySettings::EHotKeyBack;
    iSettings.iSoftKeysIdle[2] = TOggplaySettings::EHotkeyVolumeHelp;
    iSettings.iSoftKeysIdle[3] = TOggplaySettings::EHotKeyExit;
    iSettings.iSoftKeysPlay[0] = TOggplaySettings::EPause;
    iSettings.iSoftKeysPlay[1] = TOggplaySettings::ENextSong;
    iSettings.iSoftKeysPlay[2] = TOggplaySettings::EFastForward;
    iSettings.iSoftKeysPlay[3] = TOggplaySettings::ERewind;
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

    iAnalyzerState = EDecay; 

#if !defined(PLUGIN_SYSTEM)
	iSettings.iBufferingMode = EBufferThread;
	iSettings.iThreadPriority = ENormal;
#endif

    // Open the file
    RFile in;
    TInt err = in.Open(iCoeEnv->FsSession(), iIniFileName, EFileRead | EFileStreamText);    
    if (err != KErrNone)
		{
        TRACEF(COggLog::VA(_L("ReadIni: %d"), err ));
        return;
		}
	
	TFileText tf;
	tf.Set(in);

	// Read in the fields
	TInt ini_version = 0;

	TInt val = (TInt) IniRead32(tf);
	if ( val == 0xdb ) // Our magic number ! (see writeini below)
		{
		// Followed by version number
		ini_version = (TInt) IniRead32(tf);
		TRACEF(COggLog::VA(_L("Inifile version %d"), ini_version ));
		}
	else
		{
		// Not the magic number so this must be an old version
		// Seek back to beginning of the file and start parsing from there
		tf.Seek(ESeekStart);
		}
	
	iHotkey = (TInt) IniRead32(tf, 0, KMaxKeyCodes);
	iSettings.iRepeat = (TBool) IniRead32(tf, 1, 1);
	iVolume = (TInt) IniRead32(tf, KMaxVolume, KMaxVolume);
	
	TInt64 tmp64 = IniRead64(tf);
	TTime t(tmp64);
	iSettings.iAlarmTime = t;

	iAnalyzerState = (TInt) IniRead32(tf, 0, 3);
	val = IniRead32(tf);  // For backward compatibility
	iCurrentSkin = (TInt) IniRead32(tf, 0, (iSkins->Count() - 1));
   
	if (ini_version >= 2)
		{
		TInt restore_stack_count = (TInt) IniRead32(tf);      
		for (TInt i = 0; i < restore_stack_count; i++)
			iRestoreStack.Append((TInt) IniRead32(tf));
	
		iRestoreCurrent = (TInt) IniRead32(tf);
		iSettings.iScanmode = (TInt) IniRead32(tf);

#if defined(SERIES80)
		IniReadDes(tf, iSettings.iCustomScanDir, KMmcSearchDir);
#endif

		iSettings.iAutoplay = (TInt) IniRead32(tf);
		iSettings.iManeuvringSpeed = (TInt) IniRead32(tf);
      
		// For backwards compatibility for number of hotkeys
		TInt num_of_hotkeys = 0;
		if (ini_version<7) 
			num_of_hotkeys = (ini_version >= 5) ? TOggplaySettings::ENofHotkeysV5 : TOggplaySettings::ENofHotkeysV4;
		else 
			num_of_hotkeys = (TInt)  IniRead32(tf);
 
		for (TInt j = TOggplaySettings::KFirstHotkeyIndex; j < num_of_hotkeys ; j++) 
			iSettings.iUserHotkeys[j] = (TInt) IniRead32(tf);
      
		iSettings.iWarningsEnabled = (TBool) IniRead32(tf);
		if (ini_version<7) 
			{
			// The way the softkeys is stored has changed, ignore the value from the ini file.
            IniRead32(tf); 
        	IniRead32(tf);
        	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EHotKeyExit;
        	iSettings.iSoftKeysPlay[0] = TOggplaySettings::EHotKeyExit;
			}
        else
			{
			iSettings.iSoftKeysIdle[0] = (TInt) IniRead32(tf);
      		iSettings.iSoftKeysPlay[0] = (TInt) IniRead32(tf);
			}

#if defined(SERIES80)
		iSettings.iSoftKeysIdle[1] = (TInt) IniRead32(tf);
		iSettings.iSoftKeysPlay[1] = (TInt) IniRead32(tf);
		iSettings.iSoftKeysIdle[2] = (TInt) IniRead32(tf);
		iSettings.iSoftKeysPlay[2] = (TInt) IniRead32(tf);
		iSettings.iSoftKeysIdle[3] = (TInt) IniRead32(tf);
		iSettings.iSoftKeysPlay[3] = (TInt) IniRead32(tf);
#endif

		iSettings.iRandom = (TBool) IniRead32(tf, 0, 1);
		} // version 2 onwards

#if defined(PLUGIN_SYSTEM)
	TInt nbController = IniRead32(tf);
	for (TInt j = 0 ; j<nbController ; j++)
		{
		TFileName extension;
		if (tf.Read(extension) != KErrNone)
			extension = KNullDesC;

		TUid uid;
		uid.iUid = IniRead32(tf);
		TRAPD(nevermind, iOggPlayback->GetPluginListL().SelectPluginL(extension, uid));

		TRACEF(COggLog::VA(_L("Looking for controller %S:%x Result:%i"), &extension, uid.iUid, nevermind));
		}
#endif

	if (ini_version>=7)
		{
		iSettings.iGainType = IniRead32(tf);
		if (ini_version<12)
			{
			// Adjust the setting to the new range
			switch(iSettings.iGainType)
				{
				case EMinus21dB:
					iSettings.iGainType = EMinus18dB;
					break;

				case EMinus18dB:
					iSettings.iGainType = EMinus12dB;
					break;

				case EMinus15dB:
					iSettings.iGainType = ENoGain;
					break;

				case EMinus12dB:
					iSettings.iGainType = EStatic6dB;
					break;

				case EMinus9dB:
					iSettings.iGainType = EStatic12dB;
					break;

				default:
					break;
				}
			}

		iOggPlayback->SetVolumeGain((TGainType) iSettings.iGainType);
		}

#if !defined(PLUGIN_SYSTEM)
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
		if (ini_version<12)
			{
			// Adjust the setting to the new range
			switch(iSettings.iAlarmGain)
				{
				case EMinus21dB:
					iSettings.iAlarmGain = EMinus18dB;
					break;

				case EMinus18dB:
					iSettings.iAlarmGain = EMinus12dB;
					break;

				case EMinus15dB:
					iSettings.iAlarmGain = ENoGain;
					break;

				case EMinus12dB:
					iSettings.iAlarmGain = EStatic6dB;
					break;

				case EMinus9dB:
					iSettings.iAlarmGain = EStatic12dB;
					break;

				default:
					break;
				}
			}

		iSettings.iAlarmSnooze = IniRead32(tf);
		}

	in.Close();
	}

TInt32 COggPlayAppUi::IniRead32(TFileText& aFile, TInt32 aDefault, TInt32 aMaxValue)
	{
	TInt32 val = aDefault;
	TBuf<128> line;

	if (aFile.Read(line) == KErrNone)
		{
		TLex parse(line);
		if (parse.Val(val) == KErrNone)
			{
			if (val > aMaxValue)
				val = aDefault;
			}
		else
			val = aDefault;
		}

	return val;
	}
  
TInt64 COggPlayAppUi::IniRead64(TFileText& aFile, TInt64 aDefault)
	{
	TInt64 val = aDefault;
	TBuf<128> line;

	if (aFile.Read(line) == KErrNone)
		{
		TLex parse(line);
		if (parse.Val(val) != KErrNone)
			val = aDefault;
		}

	return val;
	}

#if defined(SERIES80)
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

void COggPlayAppUi::WriteIniFile()
	{
    TRACEF(COggLog::VA(_L("COggPlayAppUi::WriteIniFile() %S, %d"), &iIniFileName, iIsRunningEmbedded));
    
    // Accessing the MMC , using the TFileText is extremely slow. 
    // We'll do everything using the C:\ drive then move the file to it's final destination in MMC.
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
    
    RFile out;    
    if (out.Replace(iCoeEnv->FsSession(), fileName, EFileWrite | EFileStreamText) != KErrNone)
		return;
    
	TRACEF(_L("Writing Inifile..."));
	TFileText tf;
	tf.Set(out);
    
    // this should do the trick for forward compatibility:
	TInt magic=0xdb;
	TInt iniversion=12;

	TBuf<64> num;
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
	if (iIsRunningEmbedded)
		{
	    num.Num(iRestoreStack.Count());
		tf.Write(num);
    
		for ( TInt i = 0; i < iRestoreStack.Count(); i++ )
			{
			num.Num(iRestoreStack[i]);
			tf.Write(num);
			}
    
		num.Num(iRestoreCurrent);
		tf.Write(num);
		}
	else
		{
	    num.Num(iViewHistoryStack.Count());
		tf.Write(num);
    
		for (TInt i = 0 ; i<iViewHistoryStack.Count() ; i++)
			{
			num.Num(iViewHistoryStack[i]);
			tf.Write(num);
			}
    
	    num.Num(iAppView->GetSelectedIndex());
		tf.Write(num);
		}
	
	num.Num(iSettings.iScanmode);
	tf.Write(num);

#if defined(SERIES80)	
	tf.Write(iSettings.iCustomScanDir);
#endif

	num.Num(iSettings.iAutoplay);
	tf.Write(num);

 	num.Num(iSettings.iManeuvringSpeed);
	tf.Write(num);
    
    num.Num((TUint) TOggplaySettings::ENofHotkeys);
    tf.Write(num);
    
    TInt j;
    for (j = TOggplaySettings::KFirstHotkeyIndex; j<TOggplaySettings::ENofHotkeys ; j++) 
		{
        num.Num(iSettings.iUserHotkeys[j]);
        tf.Write(num);
		}
    
	num.Num(iSettings.iWarningsEnabled ? 1 : 0);
    tf.Write(num);
    
    for (j = 0 ; j<KNofSoftkeys ; j++)
	    {
        num.Num(iSettings.iSoftKeysIdle[j]);
        tf.Write(num);
    
        num.Num(iSettings.iSoftKeysPlay[j]);
        tf.Write(num);
		}
    
    num.Num(iSettings.iRandom);
    tf.Write(num);

#if defined(PLUGIN_SYSTEM)
    CDesCArrayFlat * supportedExtensionList = iOggPlayback->GetPluginListL().SupportedExtensions();
    TRAPD(err,
		{
        CleanupStack::PushL(supportedExtensionList);
        
        num.Num(supportedExtensionList->Count());
        tf.Write(num);
        for (j = 0 ; j<supportedExtensionList->Count() ; j++)
			{
            tf.Write((*supportedExtensionList)[j]);
            CPluginInfo * selected = iOggPlayback->GetPluginListL().GetSelectedPluginInfo((*supportedExtensionList)[j]);
            if (selected)
            	num.Num((TInt)selected->iControllerUid.iUid) ;
            else
            	num.Num(0);
            tf.Write(num);
			}
        
        CleanupStack::PopAndDestroy(1);
	    });
#endif
    
    num.Num(iSettings.iGainType);
    tf.Write(num);

#if !defined(PLUGIN_SYSTEM)
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

	TRACEF(_L("Writing Inifile... DONE"));
	}

void COggPlayAppUi::HandleForegroundEventL(TBool aForeground)
	{
	iForeground = aForeground;

#if defined(SERIES60)
	CAknAppUi::HandleForegroundEventL(aForeground);
#elif defined(UIQ)
	CQikAppUi::HandleForegroundEventL(aForeground);
#else
	CEikAppUi::HandleForegroundEventL(aForeground);
#endif

	if (iStartUpState != EStartUpComplete)
		return;

	if (aForeground)
		{
#if defined(UIQ)
		// UIQ deactivates our view when we are moved to the background
		// Consequently we need to re-activate it when we are moved back into the foreground
		ActivateOggViewL();
#endif

#if defined(SERIES60SUI)
		// With S60 Scalable UI the layout may have changed, so we need to activate the correct view
		if (iMainViewActive)
			ActivateOggViewL();
#endif

		iAppView->RestartCallBack();
		}
	else
		iAppView->StopCallBack();
	}

void COggPlayAppUi::ToggleRandomL()
	{
	SetRandomL(!iSettings.iRandom);
	}

void COggPlayAppUi::SetRandomL(TBool aRandom)
	{
    // Toggle random
    iSettings.iRandom = aRandom;
	COggSongList* songList;
    if (iSettings.iRandom)
		songList = COggRandomPlay::NewL(this, iAppView, iSongList);
    else
		songList = COggNormalPlay::NewL(iAppView, iSongList);

	delete iSongList;
	iSongList = songList;

    iSongList->SetRepeat(iSettings.iRepeat);
    iAppView->UpdateRandom();
	}

void COggPlayAppUi::ToggleRepeat()
	{
	if (iSettings.iRepeat)
	      SetRepeat(EFalse);
	    else
	      SetRepeat(ETrue);
	}

void COggPlayAppUi::SetRepeat(TBool aRepeat)
	{
    iSettings.iRepeat = aRepeat;

    iSongList->SetRepeat(aRepeat);
    iAppView->UpdateRepeat();
	}

TBool COggPlayAppUi::ProcessCommandParametersL(TApaCommand /*aCommand*/, TFileName& /*aDocumentName*/,const TDesC8& /*aTail*/)
	{
    return ETrue;
	}

void COggPlayAppUi::OpenFileL(const TDesC& aFileName)
	{
	TRACEF(COggLog::VA(_L("COggPlayAppUi::OpenFileL: %S"), &aFileName));		

	iFileName = aFileName;
	iIsRunningEmbedded = ETrue;
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
#if defined(PLUGIN_SYSTEM)
	iSettingsView->VolumeGainChangedL();
#else
	iPlaybackOptionsView->VolumeGainChangedL();
#endif
#endif
	}

#if !defined(PLUGIN_SYSTEM)
void COggPlayAppUi::SetBufferingModeL(TBufferingMode aNewBufferingMode)
{
	TInt err = ((COggPlayback *) iOggPlayback)->SetBufferingMode(aNewBufferingMode);
	if (err == KErrNone)
		iSettings.iBufferingMode = aNewBufferingMode;
	else
	{
		// Pop up a message (reset the value in the playback options too)
		TBuf<128> buf, tbuf;
		iEikonEnv->ReadResourceL(tbuf, R_OGG_ERROR_20);
		iEikonEnv->ReadResourceL(buf, R_OGG_ERROR_29);		
		iOggMsgEnv->OggErrorMsgL(tbuf, buf);		

#if defined(SERIES60)
		iPlaybackOptionsView->BufferingModeChangedL();
#endif
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

void COggPlayAppUi::ReadSkin(TInt aSkin)
	{
	iCurrentSkin = aSkin;

	TFileName buf(iSkinFileDir);
	if (iCurrentSkin >= iSkins->MdcaCount())
		{
		if (iCurrentSkin == 0)
			{
			// There are no skins at all, so leave with KErrNotFound
			User::Leave(KErrNotFound);
			}
		else
			{
			// The index is out of range, so reset to the first skin
			iCurrentSkin = 0;
			}
		}

	buf.Append((*iSkins)[iCurrentSkin]);
	iAppView->ReadSkin(buf);
	}

#if defined(SERIES60SUI)
void COggPlayAppUi::HandleResourceChangeL(TInt aType)
	{
	CAknAppUi::HandleResourceChangeL(aType);
	
	if (aType == KEikDynamicLayoutVariantSwitch)
		{
		if (iMainViewActive && iForeground)
			ActivateOggViewL();
		}
	}
#endif

void COggPlayAppUi::FileOpenErrorL(TInt aErr)
	{
	TBuf<64> buf, buf2;
	CEikonEnv::Static()->ReadResourceL(buf, R_OGG_ERROR_14);

	buf2.Format(buf, aErr);
	iOggMsgEnv->OggErrorMsgL(buf2, iFileName);
	}

TInt COggPlayAppUi::Rnd(TInt aMax)
	{
	TInt64 max64(aMax);
	TInt64 rnd(Math::Rand(iRndSeed));
	TInt64 result64((rnd * max64) / iRndMax);

#if defined(SERIES60V3)
	return (TInt) result64;
#else
	return result64.GetTInt();
#endif
	}


COggSongList::COggSongList(COggPlayAppView* aAppView)
: iAppView(aAppView)
	{
	}

void COggSongList::ConstructL(COggSongList* aSongList)
	{
	if (aSongList)
		{
		// Copy the state from the other song list
		RPointerArray<TOggFile>& fileList = aSongList->iFileList;
		TInt i, count = fileList.Count();
		for (i = 0 ; i<count ; i++)
			iFileList.Append(fileList[i]);
		
		RPointerArray<TOggFile>& fullFileList = aSongList->iFullFileList;
		for (i = 0 ; i<count ; i++)
			iFullFileList.Append(fullFileList[i]);

		iPlayingIdx = aSongList->iPlayingIdx;
		iFullListPlayingIdx = aSongList->iFullListPlayingIdx;
		}
	else
		iPlayingIdx = ENoFileSelected;
	}

COggSongList::~COggSongList()
	{
	iFileList.Close();
	iFullFileList.Close();
	}

void COggSongList::SetPlayingFromListBox(TInt aPlaying)
	{
    // Change the currently played song
    // It is possible that the current list is not the one on the list box on screen 
    // (User has navigated while playing tune).

    // Update the file list, according to what is displayed in the list box.
    iFileList.Reset();
	iFullFileList.Reset();
    iPlayingIdx = ENoFileSelected;

	if (aPlaying != ENoFileSelected)
		{
		TInt i;
		TInt iMax = iAppView->GetTextArray()->Count();
		for (i = 0 ; i<iMax ; i++)
			{
			if (iAppView->HasAFileName(i))
				{
				// We're dealing with a file, not the "back" button or something similar
				iFileList.Append(iAppView->GetFile(i));

				if (i == aPlaying)
					iPlayingIdx = iFileList.Count()-1;
				}
			}

		// Now generate the full list of files to play
		iMax = iFileList.Count();
		for (i = 0 ; i<iMax ; i++)
			{
			if (i == iPlayingIdx)
				iFullListPlayingIdx = iFullFileList.Count();

			AddFile(iFileList[i], i);
			}
		}

    SetPlaying(iPlayingIdx);
	}

void COggSongList::AddFile(TOggFile* aFile, TInt aPlayingIdx)
	{
	if (aFile->FileType() == TOggFile::EFile)
		{
		aFile->iPlayingIndex = aPlayingIdx;
		iFullFileList.Append(aFile);
		}
	else
		{
		// Go through playlist entries
		TOggPlayList* playList = (TOggPlayList*) aFile;

		TInt i, iMax = playList->Count();
		for (i = 0 ; i<iMax ; i++)
			AddFile((*playList)[i], aPlayingIdx);
		}
	}

void COggSongList::SetPlaying(TInt aPlaying)
	{
    iPlayingIdx = aPlaying;
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

const TOggFile* COggSongList::GetPlayingFile()
	{
    if (iPlayingIdx != ENoFileSelected)
        return iFileList[iPlayingIdx];

	return NULL;
	}

const TDesC& COggSongList::GetPlaying()
	{
    if (iPlayingIdx == ENoFileSelected)
        return KNullDesC;

	TOggFile* playingFile = iFullFileList[iFullListPlayingIdx];
    return(*(playingFile->iFileName));
	}

TBool COggSongList::AnySongPlaying()
	{
    return (iPlayingIdx != ENoFileSelected);
	}

TBool COggSongList::IsSelectedFromListBoxCurrentlyPlaying()
	{
    TInt idx = iAppView->GetSelectedIndex();
    if (iAppView->HasAFileName(idx))
		{
        if (GetPlayingFile() == iAppView->GetFile(idx))
            return ETrue;
		}

	return EFalse;
	}

void COggSongList::SetRepeat(TBool aRepeat)
	{
    iRepeat = aRepeat;
	}


COggNormalPlay* COggNormalPlay::NewL(COggPlayAppView* aAppView, COggSongList* aSongList)
	{
	COggNormalPlay* self = new(ELeave) COggNormalPlay(aAppView);
	CleanupStack::PushL(self);
	self->ConstructL(aSongList);

	CleanupStack::Pop(self);
	return self;
	}

COggNormalPlay::COggNormalPlay(COggPlayAppView* aAppView)
: COggSongList(aAppView)
	{
	}

COggNormalPlay::~COggNormalPlay()
	{
	}
   
const TDesC& COggNormalPlay::GetNextSong()
	{
    TInt nSongs = iFullFileList.Count();
    if ((iPlayingIdx == ENoFileSelected) || !nSongs)
		{
        SetPlaying(ENoFileSelected);
        return KNullDesC;
		}

	if (iFullListPlayingIdx+1<nSongs)
		{
        // We are in the middle of the song list. Now play the next song.
		iFullListPlayingIdx++;
		}
	else if (iRepeat)
		{
		// We are at the end of the song list, repeat it
		iFullListPlayingIdx = 0;
		}
    else 
		{
        // We are at the end of the playlist, stop here.
        SetPlaying(ENoFileSelected);
        return KNullDesC;
		}

	TOggFile* file = iFullFileList[iFullListPlayingIdx];
	SetPlaying(file->iPlayingIndex);
	return *(file->iFileName);
	}

const TDesC& COggNormalPlay::GetPreviousSong()
	{
    TInt nSongs= iFullFileList.Count();
    if ((iPlayingIdx == ENoFileSelected) || !nSongs)
		{
        SetPlaying(ENoFileSelected);
        return KNullDesC;
		}

	if (iFullListPlayingIdx>=1)
		{
        // We are in the middle of the playlist. Now play the previous song.
		iFullListPlayingIdx--;
		}
	else if (iRepeat)
		{
		// We are at the top of the playlist, repeat it
		iFullListPlayingIdx = nSongs-1;
		}
    else 
		{
        // We are at the top of the playlist, stop here.
        SetPlaying(ENoFileSelected);
		return KNullDesC;
		}

	TOggFile* file = iFullFileList[iFullListPlayingIdx];
	SetPlaying(file->iPlayingIndex);
	return *(file->iFileName);
	}


COggRandomPlay* COggRandomPlay::NewL(COggPlayAppUi* aAppUi, COggPlayAppView* aAppView, COggSongList* aSongList)
	{
	COggRandomPlay* self = new(ELeave) COggRandomPlay(aAppUi, aAppView);
	CleanupStack::PushL(self);
	self->ConstructL(aSongList);

	CleanupStack::Pop(self);
	return self;
	}

COggRandomPlay::COggRandomPlay(COggPlayAppUi* aAppUi, COggPlayAppView* aAppView)
: COggSongList(aAppView), iAppUi(aAppUi)
	{
	}

COggRandomPlay::~COggRandomPlay()
	{
	iRandomMemory.Close();
	}

void COggRandomPlay::ConstructL(COggSongList* aSongList)
	{
	COggSongList::ConstructL(aSongList);

	if (aSongList)
		GenerateRandomListL();
	}

void COggRandomPlay::GenerateRandomListL()
	{
	// Reset the list
	iRandomMemory.Reset();

	// Generate a new random list
	RArray<TInt> numArray;
	CleanupClosePushL(numArray);
	TInt i, iMax = iFullFileList.Count();
	for (i = 0 ; i<iMax ; i++)
		User::LeaveIfError(numArray.Append(i));

	// Add the first song
	if (iMax)
		{
		iRandomMemory.Append(iFullListPlayingIdx);
		numArray.Remove(iFullListPlayingIdx);
		iRandomMemoryIdx = 0;
		iMax--;
		}

	// Add the rest
	TInt rndEntry;
	for (i = 0 ; i<iMax ; i++)
		{
		rndEntry = iAppUi->Rnd(numArray.Count());
		User::LeaveIfError(iRandomMemory.Append(numArray[rndEntry]));
		numArray.Remove(rndEntry);
		}

	CleanupStack::PopAndDestroy(&numArray);
	}

void COggRandomPlay::SetPlayingFromListBox(TInt aPlaying)
	{
	COggSongList::SetPlayingFromListBox(aPlaying);
	GenerateRandomListL();
	}

const TDesC& COggRandomPlay::GetNextSong()
	{
    TInt nSongs = iFullFileList.Count();
    if ((iPlayingIdx == ENoFileSelected) || !nSongs)
		{
        SetPlaying(ENoFileSelected);
        return KNullDesC;
		}

	if (iRandomMemoryIdx+1<nSongs)
		{
        // We are in the middle of the song list. Now play the next song.
		iRandomMemoryIdx++;
		}
	else if (iRepeat)
		{
		// We are at the end of the song list, repeat it
		// Find a new track to start with and generate a new random list
		TInt fullFileListIdx;
		do
			{
			fullFileListIdx = iAppUi->Rnd(nSongs);
			}
		while (fullFileListIdx == iFullListPlayingIdx);

		iFullListPlayingIdx = fullFileListIdx;
		GenerateRandomListL();
		}
    else 
		{
        // We are at the end of the playlist, stop here.
        SetPlaying(ENoFileSelected);
        return KNullDesC;
		}

	iFullListPlayingIdx = iRandomMemory[iRandomMemoryIdx];
	TOggFile* file = iFullFileList[iFullListPlayingIdx];
	SetPlaying(file->iPlayingIndex);
	return *(file->iFileName);
	}

const TDesC& COggRandomPlay::GetPreviousSong()
	{
    TInt nSongs= iFullFileList.Count();
    if ((iPlayingIdx == ENoFileSelected) || !nSongs)
		{
        SetPlaying(ENoFileSelected);
        return KNullDesC;
		}

	if (iRandomMemoryIdx>=1)
		{
        // We are in the middle of the playlist. Now play the previous song.
		iRandomMemoryIdx--;
		}
	else if (iRepeat)
		{
		// We are at the top of the playlist, repeat it
		// Find a new track to start with and generate a new random list
		TInt fullFileListIdx;
		do
			{
			fullFileListIdx = iAppUi->Rnd(nSongs);
			}
		while (fullFileListIdx == iFullListPlayingIdx);

		iFullListPlayingIdx = fullFileListIdx;
		GenerateRandomListL();
		}
    else 
		{
        // We are at the top of the playlist, stop here.
        SetPlaying(ENoFileSelected);
		return KNullDesC;
		}

	iFullListPlayingIdx = iRandomMemory[iRandomMemoryIdx];
	TOggFile* file = iFullFileList[iFullListPlayingIdx];
	SetPlaying(file->iPlayingIndex);
	return *(file->iFileName);
	}


CAbsPlayback::CAbsPlayback(MPlaybackObserver* aObserver)
: iState(CAbsPlayback::EClosed), iObserver(aObserver)
	{
	}

CAbsPlayback::TState CAbsPlayback::State()
	{
	return iState;
	}
   
const TOggFileInfo& CAbsPlayback::DecoderFileInfo()
	{
	return iFileInfo;
	}

TInt CAbsPlayback::BitRate()
	{
	return iFileInfo.iBitRate;
	}

TInt CAbsPlayback::Channels()
	{
	return iFileInfo.iChannels;
	}

TInt CAbsPlayback::FileSize()
	{
	return iFileInfo.iFileSize;
	}

TInt CAbsPlayback::Rate()
	{
	return iFileInfo.iRate;
	}

TInt64 CAbsPlayback::Time()
	{
	return iFileInfo.iTime;
	}

const TDesC& CAbsPlayback::Album()
	{
	return iFileInfo.iAlbum;
	}

const TDesC& CAbsPlayback::Artist()
	{
	return iFileInfo.iArtist;
	}

const TFileName& CAbsPlayback::FileName()
	{
	return iFileInfo.iFileName;
	}

const TDesC& CAbsPlayback::Genre()
	{
	return iFileInfo.iGenre;
	}

const TDesC& CAbsPlayback::Title()
	{
	return iFileInfo.iTitle;
	}

const TDesC& CAbsPlayback::TrackNumber()
	{
	return iFileInfo.iTrackNumber;
	}

void CAbsPlayback::ClearComments()
	{
	iFileInfo.iArtist.SetLength(0);
	iFileInfo.iTitle.SetLength(0);
	iFileInfo.iAlbum.SetLength(0);
	iFileInfo.iGenre.SetLength(0);
	iFileInfo.iTrackNumber.SetLength(0);
	iFileInfo.iFileName.SetLength(0);
	}

void CAbsPlayback::SetVolumeGain(TGainType /*aGain*/)
	{
	/* No gain by default */
	}
