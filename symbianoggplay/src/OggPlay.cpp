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
#include <e32math.h>
#include <hal.h>

#if defined(SERIES80)
#include <eikEnv.h>
#endif

#if defined(UIQ)
#include <quartzkeys.h>	// EStdQuartzKeyConfirm etc.
#endif

#include <eiklabel.h>   // CEikLabel
#include <gulicon.h>	// CGulIcon
#include <apgwgnam.h>	// CApaWindowGroupName
#include <eikmenup.h>	// CEikMenuPane
#include <barsread.h>

#if defined(SERIES60V3)
#include <eikstart.h>
#endif

#if defined(SERIES60)
#include <aknnotewrappers.h>
#include <aknmessagequerydialog.h>
#endif

#include "OggControls.h"
#include "OggTremor.h"

#if defined(PLUGIN_SYSTEM)
#include "OggPluginAdaptor.h"

#if defined(SERIES60V3)
#include "S60V3ImplementationUIDs.hrh"
#else
#include "ImplementationUIDs.hrh"
#endif
#endif

#include "OggPlayAppView.h"
#include "OggDialogs.h" 

#if defined(SERIES80)
#include "OggDialogsS80.h"
#endif

#include "OggLog.h"
#include "OggFilesSearchDialogs.h"

#if defined(SERIES60) || defined(SERIES80)
_LIT(KTsyName,"phonetsy.tsy");
#else // UIQ
_LIT(KTsyName,"erigsm.tsy");
#endif

#if !defined(SERIES60V3)
// Minor compatibility tweak
#define ReadResourceL ReadResource
#endif


// Application entry point
#if defined(SERIES60V3)
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
CFileStore* COggPlayDocument::OpenFileL(TBool aDoOpen, const TDesC& aFilename, RFs& /* aFs */)
	{
	TRACEF(COggLog::VA(_L("COggPlayDocument::OpenFileL: %d, %S"), (TInt) aDoOpen, &aFilename));

	iAppUi->OpenFileL(aFilename);
	return NULL;
	}

COggPlayDocument::COggPlayDocument(CEikApplication& aApp)
#if defined(UIQ)
: CQikDocument(aApp)
#else
: CEikDocument(aApp)
#endif
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


#if defined(UIQ)
COggViewActivationAO::COggViewActivationAO(COggPlayAppUi& aAppUi)
: CActive(EPriorityIdle), iAppUi(aAppUi)
	{
	CActiveScheduler::Add(this);
	}

COggViewActivationAO::~COggViewActivationAO()
	{
	}

void COggViewActivationAO::ActivateOggView()
	{
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrNone);

	SetActive();
	}

void COggViewActivationAO::RunL()
	{
	iAppUi.ActivateOggViewL();
	}
	
void COggViewActivationAO::DoCancel()
	{
	}
#endif

	
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

	// Read the device uid
	HAL::Get(HALData::EMachineUid, iMachineUid);
	TRACEF(COggLog::VA(_L("Phone UID: %x"), iMachineUid));

#if defined(__WINS__)
	// Set the machine Uid for testing purposes
	// iMachineUid = EMachineUid_E61;
#endif

#if defined(SERIES60SUI)
	BaseConstructL(EAknEnableSkin);
#else
	BaseConstructL();
#endif

	// Construct the splash views
	TRACEF(_L("COggPlayAppUi::ConstructL(): Construct Splash views..."));

	// Splash view (used for flip open mode on UIQ and both flip open and flip closed on S60)
    iSplashFOView = new(ELeave) COggSplashView(KOggPlayUidSplashFOView);
	iSplashFOView->ConstructL();
	RegisterViewL(*iSplashFOView);

#if defined(UIQ)
	// UIQ has a second Splash view for flip closed mode
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

#if defined(UIQ)
		// Create an AO for handling UIQ application switches
		// An application switch brings Oggplay back to the splash view
		// The splash view then schedules an event to re-launch the main view
		iOggViewActivationAO = new(ELeave) COggViewActivationAO(*this);
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

	if (err != KErrNone)
		StartUpError(err);
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

#if defined(UIQ)
	// Set the default view
	// This is the view that gets activated in FO mode when the user switches back to OggPlay
	// Note that FC mode works differently (the "default" FC view is always the splash FC view)
	SetDefaultViewL(*iFOView);
#endif

#if defined(UIQ) || defined(SERIES60SUI)
	iFCView = new(ELeave) COggFCView(*iAppView);
	RegisterViewL(*iFCView);
#endif

#if defined(SERIES60)
	iSettingsView = new(ELeave) COggSettingsView(*iAppView);
	RegisterViewL(*iSettingsView);

	iUserHotkeysView = new(ELeave) COggUserHotkeysView(*iAppView);
	RegisterViewL(*iUserHotkeysView);

	iPlaybackOptionsView = new(ELeave) COggPlaybackOptionsView(*iAppView);
	RegisterViewL(*iPlaybackOptionsView);

#if defined(PLUGIN_SYSTEM)
    iCodecSelectionView = new(ELeave) COggPluginSettingsView(*iAppView);
    RegisterViewL(*iCodecSelectionView);
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
	TParsePtrC parse(fileName);

#if defined(SERIES60V3)
	TFileName privatePath;
	User::LeaveIfError(iCoeEnv->FsSession().PrivatePath(privatePath));

	TBuf<2> appDriveLetter(parse.Drive());
	iIniFileName.Copy(appDriveLetter);
	iIniFileName.Append(privatePath);
	iIniFileName.Append(_L("OggPlay.ogi"));
 
	iDbFileName.Copy(appDriveLetter);
	iDbFileName.Append(privatePath);
	iDbFileName.Append(_L("OggPlay.db"));

	iSkinFileDir.Copy(appDriveLetter);
	iSkinFileDir.Append(privatePath);
	iSkinFileDir.Append(_L("import\\"));
#else
	iIniFileName.Copy(fileName);
	iIniFileName.SetLength(iIniFileName.Length() - 3);
	iIniFileName.Append(_L("ogi"));

	iDbFileName.Copy(fileName);
	iDbFileName.SetLength(iDbFileName.Length() - 3);
	iDbFileName.Append(_L("db"));

	iSkinFileDir.Copy(parse.DriveAndPath());
#endif

#if defined(__WINS__)
	// The emulator doesn't like to write to the Z: drive, avoid that.
	iIniFileName = _L("C:\\oggplay.ogi");
	iDbFileName = _L("C:\\oggplay.db");
#endif

	TTime homeTime;
	homeTime.HomeTime();
	iRndSeed = homeTime.Int64();
	iRndMax = KMaxTInt ; iRndMax++;

	iSkins = new(ELeave) CDesCArrayFlat(3);
	FindSkins();
	
	iOggMsgEnv = new(ELeave) COggMsgEnv(iSettings.iWarningsEnabled);

	iPluginSupportedList = new(ELeave) CPluginSupportedList;
	iPluginSupportedList->ConstructL();

	iOggMTPlayback = new(ELeave) COggPlayback(iOggMsgEnv, *iPluginSupportedList, this, iMachineUid);
	iOggMTPlayback->ConstructL();

#if defined(PLUGIN_SYSTEM)
	iOggMMFPlayback = new(ELeave) COggPluginAdaptor(iOggMsgEnv, *iPluginSupportedList, this);
	iOggMMFPlayback->ConstructL();
#endif

	iOggPlayback = iOggMTPlayback;
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

#if defined(UIQ)
	SetHotKey();
#endif

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
	// However older versions of the view server seem to get confused if we get rid of the splash view
#if defined(SERIES60V3)
	// Delete the splash view. We won't be seeing it again
	DeregisterView(*iSplashFOView);

	delete iSplashFOView;
	iSplashFOView = NULL;
#endif

	// Set the playback volume
	iOggMTPlayback->SetVolume(iVolume);
	iOggMTPlayback->SetVolumeGain((TGainType) iSettings.iGainType);
	if (iOggMMFPlayback)
		{
		iOggMMFPlayback->SetVolume(iVolume);
		iOggMMFPlayback->SetVolumeGain((TGainType) iSettings.iGainType);
		}

	// Set buffering mode
	TInt err = ((COggPlayback *) iOggMTPlayback)->SetBufferingMode((TBufferingMode) iSettings.iBufferingMode);
	if (err != KErrNone)
		iSettings.iBufferingMode = ENoBuffering;

	// Set thread priority
	((COggPlayback *) iOggMTPlayback)->SetThreadPriority((TStreamingThreadPriority) iSettings.iThreadPriority);

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

#if defined(UIQ)
	if (iOggViewActivationAO)
		iOggViewActivationAO->Cancel();
	delete iOggViewActivationAO;
#endif

	if (iStartUpEmbeddedAO)
		iStartUpEmbeddedAO->Cancel();
	delete iStartUpEmbeddedAO;

	if (iStartUpAO)
		iStartUpAO->Cancel();
	delete iStartUpAO;

#if defined(SERIES60)
	delete iStartUpErrorDlg;
#endif

	if (iSplashFOView)
		{
		DeregisterView(*iSplashFOView);
		delete iSplashFOView;
		}

	if (iSplashFCView)
		{
		DeregisterView(*iSplashFCView);
		delete iSplashFCView;
		}

	if (iFOView)
		{
		DeregisterView(*iFOView);
		delete iFOView;
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

	if (iPlaybackOptionsView)
		{
		DeregisterView(*iPlaybackOptionsView);
  		delete iPlaybackOptionsView;
		}

#if defined(PLUGIN_SYSTEM)
	if (iCodecSelectionView)
		{
		DeregisterView(*iCodecSelectionView);
		delete iCodecSelectionView;
		}
#endif

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

	delete iOggMTPlayback;
	delete iOggMMFPlayback;
	delete iPluginSupportedList;

	if (iCapturedKeyHandle)
		iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);

	delete iSkins;
	delete iOggMsgEnv ;
	delete iSongList;

	iViewHistoryStack.Close();
	iRestoreStack.Close();

	COggLog::Exit();  
	}

#if defined(UIQ)
void COggPlayAppUi::ActivateOggView()
	{
	// Asynchronously activate the view
	iOggViewActivationAO->ActivateOggView();
	}
#endif

void COggPlayAppUi::ActivateOggViewL()
	{
	TUid viewUid = iAppView->IsFlipOpen() ? KOggPlayUidFOView : KOggPlayUidFCView;
	ActivateOggViewL(viewUid);
	}

void COggPlayAppUi::ActivateOggViewL(const TUid aViewId)
	{
	TVwsViewId viewId;
	viewId.iAppUid = KOggPlayUid;
	viewId.iViewUid = aViewId;

	ActivateViewL(viewId);
	}

void COggPlayAppUi::NotifyStreamOpen(TInt aErr)
	{
	NextStartUpState(aErr);
	}

void COggPlayAppUi::NotifyUpdate()
	{
	// Try to resume after sound device was stolen
	if ((iTryResume>0) && (iOggPlayback->State() == CAbsPlayback::EPaused))
		{
		if (iOggPlayback->State() != CAbsPlayback::EPaused)
			iTryResume = 0;
		else
			{
			iTryResume--;
			if ((iTryResume == 0) && (iOggPlayback->State() == CAbsPlayback::EPaused))
				HandleCommandL(EOggPauseResume);
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

void COggPlayAppUi::NotifyFileOpen(TInt aErr)
	{
	if (aErr != KErrNone)
		{
		FileOpenErrorL(iFileName, aErr);
		HandleCommandL(EOggStop);
		return;
		}

	iOggPlayback->Play();
	}

void COggPlayAppUi::NotifyPlayStarted()
	{
	// Update the appView and the softkeys
	iAppView->Update();
	UpdateSoftkeys();
	}

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

#if defined(UIQ)
void COggPlayAppUi::SetHotKey()
	{
	if (iCapturedKeyHandle)
		iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);
	iCapturedKeyHandle = 0;

	if (!iHotkey || !KHotkeyKeycodes[iHotkey-1].code)
		return;

	iCapturedKeyHandle = iEikonEnv->RootWin().CaptureKey(KHotkeyKeycodes[iHotkey-1].code,
	KHotkeyKeycodes[iHotkey-1].mask, KHotkeyKeycodes[iHotkey-1].mask, 2);
	}
#endif

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
#if defined(SERIES60)
			COggAboutDialog *d = new(ELeave) COggAboutDialog;
			d->ExecuteLD(R_DIALOG_ABOUT);
#elif defined(SERIES80)
			COggAboutDialog *d = new(ELeave) COggAboutDialog;
			d->ExecuteLD(R_DIALOG_ABOUT_S80);
#else
			CEikDialog *d = new(ELeave) CEikDialog;
			d->ExecuteLD(R_DIALOG_ABOUT_UIQ);
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
			CCodecsS80Dialog *cd = new(ELeave) CCodecsS80Dialog;
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
		case EOggSkinEleven:
		case EOggSkinTwelve:
		case EOggSkinThirteen:
		case EOggSkinFourteen:
		case EOggSkinFifteen:
		case EOggSkinSixteen:
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
	if (iViewBy==ETop)
		{
		SelectNextView();
		return;
		}

	TInt idx = iAppView->GetSelectedIndex();
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
			return;
			}

		iOggPlayback->Stop();

		iSongList->SetPlayingFromListBox(idx);
		iFileName = iSongList->GetPlaying();

		TUid controllerUid;
		TInt err = PlaybackFromFileName(iFileName, iOggPlayback, controllerUid);
		if (err == KErrNone)
			err = iOggPlayback->Open(iFileName, controllerUid);

		if (err != KErrNone)
			{
			FileOpenErrorL(iFileName, err);

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
		iOggPlayback->Resume();
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

	TInt previousListboxLine = iViewHistoryStack[iViewHistoryStack.Count()-1];
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
		iViewHistoryStack.Append(idx);
		HandleCommandL(EOggViewByTitle+idx);
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
			const TDesC& fileName = iAppView->GetFileName(selectedIndex);

			TUid controllerUid;
			CAbsPlayback* oggPlayback;
			TInt err = PlaybackFromFileName(fileName, oggPlayback, controllerUid);
			if (err == KErrNone)
				err = oggPlayback->FullInfo(fileName, controllerUid, *this);

			if (err != KErrNone)
				{
				FileOpenErrorL(fileName, err);
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
				const TDesC& fileName = iAppView->GetFileName(selectedIndex);

				TUid controllerUid;
				CAbsPlayback* oggPlayback;
				TInt err = PlaybackFromFileName(fileName, oggPlayback, controllerUid);
				if (err == KErrNone)
					err = oggPlayback->FullInfo(fileName, controllerUid, *this);

				if (err != KErrNone)
					{
					FileOpenErrorL(fileName, err);
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
		FileOpenErrorL(aFileInfo.iFileName, aErr);
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

		TUid controllerUid;
		TInt err = PlaybackFromFileName(iFileName, iOggPlayback, controllerUid);
		if (err == KErrNone)
			err = iOggPlayback->Open(iFileName, controllerUid);

        if (err != KErrNone)
			{
			FileOpenErrorL(iFileName, err);
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
			if (iAppView->GetItemType(iAppView->GetSelectedIndex()) == COggListBox::EBack)
				iAppView->SelectItem(1);

			HandleCommandL(EOggPlay); // select the 1st song in the current category
            HandleCommandL(EOggPlay); // play it!
			}
		else
			{
            // If no song is playing, play the currently selected song
			if (iAppView->GetItemType(iAppView->GetSelectedIndex()) == COggListBox::EBack)
				iAppView->SelectItem(1);

			HandleCommandL(EOggPlay); // play it!
			} 
		}
	}

void COggPlayAppUi::PreviousSong()
	{
	// If there isn't a song playing we try to play the previous song (if there is one)
	if (!iSongList->AnySongPlaying())
		{
		if ((iViewBy != ETitle) && (iViewBy != EFileName))
			return;

		TInt idx = iAppView->GetSelectedIndex();
		if (idx == 0)
			return;

		idx--;
		if (iAppView->GetItemType(idx) == COggListBox::EBack)
			return;

		iSongList->SetPlayingFromListBox(idx);
		iFileName = iSongList->GetPlaying();
		}
	else
		{
		// If a song is currently playing, stop playback and play the previous song
		iFileName = iSongList->GetPreviousSong();
		}

	if (!iFileName.Length())
		{
		HandleCommandL(EOggStop);
		return;
		}

	iOggPlayback->Stop();

	TUid controllerUid;
	TInt err = PlaybackFromFileName(iFileName, iOggPlayback, controllerUid);
	if (err == KErrNone)
		err = iOggPlayback->Open(iFileName, controllerUid);

	if (err != KErrNone)
		{
		FileOpenErrorL(iFileName, err);
		HandleCommandL(EOggStop);
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

#if defined(UIQ)
		if (iSettings.iRepeat) 
			aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);

		if (iSettings.iRandom) 
			aMenuPane->SetItemButtonState(EOggShuffle, EEikMenuItemSymbolOn);

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
			item.iCommandId = EOggSkinOne+i;
			item.iCascadeId = 0;
			item.iFlags= 0;

			aMenuPane->AddMenuItemL(item);
			}
		}

#if defined(UIQ)
	if (aMenuId == R_POPUP_MENU)
		{
		if (iSettings.iRepeat)
			aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);

		if (iSettings.iRandom) 
			aMenuPane->SetItemButtonState(EOggShuffle, EEikMenuItemSymbolOn);

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
			if (p.Ext().CompareF(_L(".skn")) == 0)
				{
				iSkins->AppendL(p.NameAndExt());
				if (iSkins->Count() == 16)
					break;
				}
			}

		delete c;
		c = NULL;
		}
	
	CleanupStack::PopAndDestroy(ds);
	}

void COggPlayAppUi::ReadIniFile()
	{
    TRACEF(COggLog::VA(_L("COggPlayAppUi::ReadIniFile() %S"), &iIniFileName));

	// Set some default values (for first time users)
#if defined(SERIES60)
	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EHotkeyBack;
    iSettings.iSoftKeysPlay[0] = TOggplaySettings::EHotkeyBack;

	iSettings.iUserHotkeys[TOggplaySettings::EHotkeyFastForward] = '3';
    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyRewind] = '1';

    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPageUp] = '2';
    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPageDown] = '8';

    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyNextSong] = '6';
    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPreviousSong] = '4';

    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyKeylock] = '*';

    iSettings.iUserHotkeys[TOggplaySettings::EHotkeyPauseResume] = '5';
	iSettings.iUserHotkeys[TOggplaySettings::EHotkeyStop] = EStdKeyBackspace;

	iSettings.iUserHotkeys[TOggplaySettings::EHotkeyVolumeBoostUp] = '9';
	iSettings.iUserHotkeys[TOggplaySettings::EHotkeyVolumeBoostDown] = '7';
#elif defined(SERIES80)
	iSettings.iSoftKeysIdle[0] = TOggplaySettings::EHotkeyPlay;
    iSettings.iSoftKeysIdle[1] = TOggplaySettings::EHotkeyBack;
    iSettings.iSoftKeysIdle[2] = TOggplaySettings::EHotkeyVolumeHelp;
    iSettings.iSoftKeysIdle[3] = TOggplaySettings::EHotkeyExit;
    iSettings.iSoftKeysPlay[0] = TOggplaySettings::EHotkeyPause;
    iSettings.iSoftKeysPlay[1] = TOggplaySettings::EHotkeyNextSong;
    iSettings.iSoftKeysPlay[2] = TOggplaySettings::EHotkeyFastForward;
    iSettings.iSoftKeysPlay[3] = TOggplaySettings::EHotkeyRewind;

	iSettings.iCustomScanDir = KFullScanString;
#endif
 
    iSettings.iWarningsEnabled = ETrue;
	iSettings.iGainType = ENoGain;
	iVolume = KMaxVolume;

	iSettings.iAlarmTime.Set(_L("20030101:120000.000000"));
	iSettings.iAlarmVolume = 7;
	iSettings.iAlarmGain = ENoGain;
	iSettings.iAlarmSnooze = 1;

    iAnalyzerState = EDecay;
    iSettings.iScanmode = TOggplaySettings::EFullScan;

	iSettings.iBufferingMode = EBufferThread;

#if defined(SERIES60V3)
	iSettings.iThreadPriority = EHigh;
#else
	iSettings.iThreadPriority = ENormal;
#endif

	iSettings.iMp3Dithering = ETrue;

	// Open the file
	RFile in;
    TInt err = in.Open(iCoeEnv->FsSession(), iIniFileName, EFileShareReadersOnly);
    if (err != KErrNone)
		{
        TRACEF(COggLog::VA(_L("ReadIni: File open error: %d"), err));
        return;
		}

	TInt fileSize = 0;
	err = in.Size(fileSize);
	if ((err != KErrNone) || (fileSize>1024))
		{
		TRACEF(COggLog::VA(_L("ReadIni: File size error: %d, File size: %d"), err, fileSize));
		in.Close();
		return;
		}

	TBuf8<1024> iniFileBuf;
	err = in.Read(iniFileBuf);
	in.Close();

	if (err != KErrNone)
		{
		TRACEF(COggLog::VA(_L("ReadIni: File read error: %d"), err));
		return;
		}

	// Read the data
	TPtrC8 iniFileData(iniFileBuf);
	TInt magic = IniRawRead32(iniFileData);
	if (magic == 0x2A49474F) // Our magic number ! (see writeini below)
		{
		TRAPD(err, ReadIniFileL(iniFileData));
		if (err != KErrNone)
			{
			// We tolerate all errors when reading the ini file
			TRACEF(COggLog::VA(_L("ReadIni: File parsing error: %d"), err));
			return;
			}
		}
	else
		{
		TRACEF(COggLog::VA(_L("ReadIni: File stamp error")));
		return;
		}
	}

void COggPlayAppUi::ReadIniFileL(TPtrC8& aIniFileData)
	{
	// Read the version number
	TInt iniVersion = IniRead32L(aIniFileData, 1, 2);
	TRACEF(COggLog::VA(_L("ReadIni: version %d"), iniVersion));

	// Read the OS version
	TInt iniOsVersion = IniRead32L(aIniFileData, 0, 5);

#if defined(SERIES60)
#if !defined(SERIES60V3)
#if !defined(SERIES60SUI)
	TInt osVersion = 0;
#else
	TInt osVersion = 1;
#endif
#else
	TInt osVersion = 2;
#endif
#elif defined(SERIES80)
#if !defined(SERIES90)
	TInt osVersion = 3;
#else
	TInt osVersion = 4;
#endif
#else
	TInt osVersion = 5;
#endif

#if !defined(__WINS__)
	// In principle we could cope with OS version mismatches but
	// it's far more likely that if the version doesn't match the file is corrupt.
	if (osVersion != iniOsVersion)
		{
		// The only possible reason for a mismatch is the user
		// has upgraded from a S60 version to a S60 SUI version 
		if ((osVersion>1) || (iniOsVersion>1))
			User::Leave(KErrCorrupt);
		}
#endif

	// Determine if this .ogi file was written with the non-mmf or mmf version
	TBool mmfDataPresent = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;

#if defined(UIQ)
	iHotkey = IniRead32L(aIniFileData, 0, KMaxKeyCodes);
#else
	/* iHotkey = */ IniRead32L(aIniFileData, 0, 0);
#endif

	iSettings.iRepeat = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;
	iVolume = IniRead32L(aIniFileData, 0, KMaxVolume);
	
	iSettings.iAlarmTime = TTime(IniRead64L(aIniFileData));

	iAnalyzerState = IniRead32L(aIniFileData, (TInt32) EZero, (TInt32) EDecay);
	iCurrentSkin = IniRead32L(aIniFileData, 0, KMaxTInt);
	if (iCurrentSkin>(iSkins->Count() - 1)) // Check that the skin exists (the user may have deleted some of them)
		iCurrentSkin = 0; // Reset to the default skin

	TInt restoreStackCount = IniRead32L(aIniFileData, 0, 2);
	TInt i;
	for (i = 0 ; i<restoreStackCount ; i++)
		iRestoreStack.Append(IniRead32L(aIniFileData, 0, KMaxTInt));
	
	iRestoreCurrent = IniRead32L(aIniFileData, 0, KMaxTInt);

#if defined(SERIES80)
	iSettings.iScanmode = IniRead32L(aIniFileData, (TInt) TOggplaySettings::EFullScan, (TInt) TOggplaySettings::ECustomDir);
#else
	iSettings.iScanmode = IniRead32L(aIniFileData, (TInt) TOggplaySettings::EFullScan, (TInt) TOggplaySettings::EMmcFull);
#endif

	iSettings.iAutoplay = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;
	/* iSettings.iManeuvringSpeed = */ IniRead32L(aIniFileData, 0, 0);
      
	TInt numHotKeys = IniRead32L(aIniFileData, 0, (TInt) TOggplaySettings::ENumHotkeys);
	for (i = TOggplaySettings::EFirstHotkeyIndex ; i<numHotKeys ; i++) 
		iSettings.iUserHotkeys[i] = IniRead32L(aIniFileData, 0, KMaxTInt);
      
	iSettings.iWarningsEnabled = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;
	iSettings.iRandom = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;
	iSettings.iGainType = IniRead32L(aIniFileData, (TInt) EMinus24dB, (TInt) EStatic12dB);

	iSettings.iAlarmActive = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;
	iSettings.iAlarmVolume = IniRead32L(aIniFileData, 1, 10);
	iSettings.iAlarmGain = IniRead32L(aIniFileData, (TInt) EMinus24dB, (TInt) EStatic12dB);
	iSettings.iAlarmSnooze = IniRead32L(aIniFileData, 0, 3);

	iSettings.iBufferingMode = IniRead32L(aIniFileData, (TInt) EBufferThread, (TInt) ENoBuffering);
	iSettings.iThreadPriority = IniRead32L(aIniFileData, (TInt) ENormal, (TInt) EHigh);

	TInt maxSoftKeyIndex = ((TInt) TOggplaySettings::ENumHotkeys) - 1;
	iSettings.iSoftKeysIdle[0] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);
	iSettings.iSoftKeysPlay[0] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);

	// Version 2 adds MAD mp3 dithering
	if (iniVersion == 2)
		iSettings.iMp3Dithering = IniRead32L(aIniFileData, 0, 1) ? ETrue : EFalse;

#if defined(SERIES80)
	iSettings.iSoftKeysIdle[1] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);
	iSettings.iSoftKeysPlay[1] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);
	iSettings.iSoftKeysIdle[2] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);
	iSettings.iSoftKeysPlay[2] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);
	iSettings.iSoftKeysIdle[3] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);
	iSettings.iSoftKeysPlay[3] = IniRead32L(aIniFileData, 0, maxSoftKeyIndex);

	TRAP(err, IniReadDesL(aIniFileData, iSettings.iCustomScanDir, iniVersion));
	if (err != KErrNone)
		{
		// Reset the values if the ReadDesL() fails
		iSettings.iScanmode = TOggplaySettings::EFullScan;
		iSettings.iCustomScanDir = KFullScanString;
		User::Leave(err);
		}
#endif

	// For the non MMF version that's the end of the file
	if (!mmfDataPresent)
		return;

#if defined(PLUGIN_SYSTEM)
	TInt numExtensions = IniRead32L(aIniFileData, 0, KMaxTInt);
	for (i = 0 ; i<numExtensions ; i++)
		{
		TFileName extension;
		IniReadDesL(aIniFileData, extension, iniVersion);

		TUid uid;
		uid.iUid = (TInt32) IniRead32L(aIniFileData);

		// The old ini file will probably be looking for the OggPlay controller
		// It's not there anymore, so ignore the request to use it.
		if ((iniVersion == 1) && (uid.iUid == ((TInt32) KOggPlayUidControllerImplementation)))
			continue;

		TRAPD(err, iPluginSupportedList->SelectPluginL(extension, uid));
		TRACEF(COggLog::VA(_L("Looking for controller %S:%x Result:%i"), &extension, uid.iUid, err));
		}
#endif
	}

TInt COggPlayAppUi::IniRawRead32(TPtrC8& aDes)
	{
	if (aDes.Length()<4)
		User::Leave(KErrCorrupt);

	TInt val;
	Mem::Copy(&val, aDes.Ptr(), 4);

	aDes.Set(aDes.Ptr()+4, aDes.Length()-4);
	return val;
	}

TInt COggPlayAppUi::IniRead32L(TPtrC8& aDes)
	{
	TInt val;
	TLex8 desLex(aDes);
	User::LeaveIfError(desLex.Val(val));

	TInt bytesToAdvance = desLex.Offset()+1;
	aDes.Set(aDes.Ptr()+bytesToAdvance, aDes.Length()-bytesToAdvance);
	return val;
	}

TInt COggPlayAppUi::IniRead32L(TPtrC8& aDes, TInt aLowerLimit, TInt aUpperLimit)
	{
	TInt val;
	TLex8 desLex(aDes);
	User::LeaveIfError(desLex.Val(val));

	if ((val<aLowerLimit) || (val>aUpperLimit))
		User::Leave(KErrCorrupt);

	TInt bytesToAdvance = desLex.Offset()+1;
	aDes.Set(aDes.Ptr()+bytesToAdvance, aDes.Length()-bytesToAdvance);
	return val;
	}

TInt64 COggPlayAppUi::IniRead64L(TPtrC8& aDes)
	{
	TInt64 val;
	TLex8 desLex(aDes);
	User::LeaveIfError(desLex.Val(val));

	TInt bytesToAdvance = desLex.Offset()+1;
	aDes.Set(aDes.Ptr()+bytesToAdvance, aDes.Length()-bytesToAdvance);
	return val;
	}

TInt64 COggPlayAppUi::IniRead64L(TPtrC8& aDes, TInt64 aLowerLimit, TInt64 aUpperLimit)
	{
	TInt64 val;
	TLex8 desLex(aDes);
	User::LeaveIfError(desLex.Val(val));

	if ((val<aLowerLimit) || (val>aUpperLimit))
		User::Leave(KErrCorrupt);

	TInt bytesToAdvance = desLex.Offset()+1;
	aDes.Set(aDes.Ptr()+bytesToAdvance, aDes.Length()-bytesToAdvance);
	return val;
	}

const TUint8 KIniValueTerminator = '\n';
const TUint16 KIniValueTerminator16 = L'\n';
void COggPlayAppUi::IniReadDesL(TPtrC8& aDes, TDes& aValue, TInt aIniVersion)
	{
	aValue.Zero();
	TInt maxBytes = aValue.MaxSize();

	TInt i;
	TChar char16;
	for (i = 0 ; i<maxBytes ; i += 2)
		{
		if (aIniVersion == 1)
			{
			// Check for at least one 8 bit character
			if (!aDes.Length())
				User::Leave(KErrCorrupt);

			// Check for terminator
			if (aDes[0] == KIniValueTerminator)
				break;
			}
		else // aIniVersion>=2
			{
			// Check for at least one 16 bit character
			if (aDes.Length()<2)
				User::Leave(KErrCorrupt);

			// Check for terminator
			char16 = ((TUint16) aDes[0]) | (((TUint16) (aDes[1])) << 8);
			if (char16 == KIniValueTerminator16)
				break;
			}

		// Check for at least one 16 bit character
		if (aDes.Length()<2)
			User::Leave(KErrCorrupt);

		// Append it to the output string
		char16 = ((TUint16) aDes[0]) | (((TUint16) (aDes[1])) << 8);
		aValue.Append(char16);

		// Advance one 16 bit character
		aDes.Set(aDes.Ptr()+2, aDes.Length()-2);
		}
	
	// Check if we ran out of bytes before reaching the terminator
	if (i == maxBytes)
		User::Leave(KErrOverflow);

	// Advance past terminator
	if (aIniVersion == 1)
		aDes.Set(aDes.Ptr()+1, aDes.Length()-1);
	else // aIniVersion>=2
		aDes.Set(aDes.Ptr()+2, aDes.Length()-2);
	}

void COggPlayAppUi::WriteIniFile()
	{
    TRACEF(COggLog::VA(_L("COggPlayAppUi::WriteIniFile() %S, %d"), &iIniFileName, iIsRunningEmbedded));
    
	// Output buffer to write data to
	TBuf8<1024> iniFileBuf;
	iniFileBuf.SetLength(10);

	// Magic identifier string
	iniFileBuf[0] = 'O';
	iniFileBuf[1] = 'G';
	iniFileBuf[2] = 'I';
	iniFileBuf[3] = '*';

	// Ini file version number
	// Please increase the version number when adding stuff
	// (also update the range check in ReadIniFile())
	iniFileBuf[4] = '2';
	iniFileBuf[5] = KIniValueTerminator;

	// Ini file OS version
#if defined(SERIES60)
#if !defined(SERIES60V3)
#if !defined(SERIES60SUI)
	iniFileBuf[6] = '0'; // Plain S60 (e.g. Sendo X)
	iniFileBuf[7] = KIniValueTerminator;
#else
	iniFileBuf[6] = '1'; // S60 + SUI (e.g Nokia N90)
	iniFileBuf[7] = KIniValueTerminator;
#endif
#else
	iniFileBuf[6] = '2'; // S60 V3 (e.g. Nokia E60)
	iniFileBuf[7] = KIniValueTerminator;
#endif
#elif defined(SERIES80)
#if !defined(SERIES90)
	iniFileBuf[6] = '3'; // S80 (e.g Nokia 9500)
	iniFileBuf[7] = KIniValueTerminator;
#else
	iniFileBuf[6] = '4'; // S90 (e.g Nokia 7700)
	iniFileBuf[7] = KIniValueTerminator;
#endif
#else
	iniFileBuf[6] = '5'; // UIQ (e.g. SE P900)
	iniFileBuf[7] = KIniValueTerminator;
#endif

#if defined(PLUGIN_SYSTEM)
	// MMF initialisation data present
	iniFileBuf[8] = '1';
	iniFileBuf[9] = KIniValueTerminator;
#else
	// No MMF initialisation data
	iniFileBuf[8] = '0';
	iniFileBuf[9] = KIniValueTerminator;
#endif

	// Write the data
	iniFileBuf.AppendNum(iHotkey);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iRepeat ? 1 : 0);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iVolume);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iAlarmTime.Int64());
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iAnalyzerState);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iCurrentSkin);
	iniFileBuf.Append(KIniValueTerminator);
	
	TInt i;
	if (iIsRunningEmbedded)
		{
		// Leave the restore stack unchanged
		TInt restoreStackCount = iRestoreStack.Count();
		iniFileBuf.AppendNum(restoreStackCount);
		iniFileBuf.Append(KIniValueTerminator);

		for (i = 0; i < restoreStackCount ; i++)
			{
			iniFileBuf.AppendNum(iRestoreStack[i]);
			iniFileBuf.Append(KIniValueTerminator);
			}

		iniFileBuf.AppendNum(iRestoreCurrent);
		iniFileBuf.Append(KIniValueTerminator);
		}
	else
		{
		// Write a new restore stack
		TInt viewHistoryStackCount = iViewHistoryStack.Count();
		iniFileBuf.AppendNum(viewHistoryStackCount);
		iniFileBuf.Append(KIniValueTerminator);
    
		for (i = 0 ; i<viewHistoryStackCount ; i++)
			{
			iniFileBuf.AppendNum(iViewHistoryStack[i]);
			iniFileBuf.Append(KIniValueTerminator);
			}
    
		iniFileBuf.AppendNum(iAppView->GetSelectedIndex());
		iniFileBuf.Append(KIniValueTerminator);
		}
	
	iniFileBuf.AppendNum(iSettings.iScanmode);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iAutoplay ? 1 : 0);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iManeuvringSpeed);
	iniFileBuf.Append(KIniValueTerminator);
    
	iniFileBuf.AppendNum((TInt) TOggplaySettings::ENumHotkeys);
	iniFileBuf.Append(KIniValueTerminator);
    
    for (i = TOggplaySettings::EFirstHotkeyIndex; i<TOggplaySettings::ENumHotkeys ; i++) 
		{
		iniFileBuf.AppendNum(iSettings.iUserHotkeys[i]);
		iniFileBuf.Append(KIniValueTerminator);
		}
    
	iniFileBuf.AppendNum(iSettings.iWarningsEnabled ? 1 : 0);
	iniFileBuf.Append(KIniValueTerminator);
    
	iniFileBuf.AppendNum(iSettings.iRandom ? 1 : 0);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iGainType);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iAlarmActive ? 1 : 0);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iAlarmVolume);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iAlarmGain);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iAlarmSnooze);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iBufferingMode);
	iniFileBuf.Append(KIniValueTerminator);

	iniFileBuf.AppendNum(iSettings.iThreadPriority);
	iniFileBuf.Append(KIniValueTerminator);

	for (i = 0 ; i<KNofSoftkeys ; i++)
		{
		iniFileBuf.AppendNum(iSettings.iSoftKeysIdle[i]);
		iniFileBuf.Append(KIniValueTerminator);
    
		iniFileBuf.AppendNum(iSettings.iSoftKeysPlay[i]);
		iniFileBuf.Append(KIniValueTerminator);
		}

	iniFileBuf.AppendNum(iSettings.iMp3Dithering ? 1 : 0);
	iniFileBuf.Append(KIniValueTerminator);

#if defined(SERIES80)
	HBufC8* customScanDir8 = HBufC8::NewMaxLC(iSettings.iCustomScanDir.Size());
	Mem::Copy((TAny *) customScanDir8->Ptr(), iSettings.iCustomScanDir.Ptr(), iSettings.iCustomScanDir.Size());

	iniFileBuf.Append(*customScanDir8);
	iniFileBuf.Append(KIniValueTerminator);
	iniFileBuf.Append(0);
	CleanupStack::PopAndDestroy(customScanDir8);
#endif

#if defined(PLUGIN_SYSTEM)
	CDesCArrayFlat * supportedExtensionList = iPluginSupportedList->SupportedExtensions();
	CleanupStack::PushL(supportedExtensionList);
    
	TInt supportedExtensionListCount = supportedExtensionList->Count();
	iniFileBuf.AppendNum(supportedExtensionListCount);
	iniFileBuf.Append(KIniValueTerminator);

	for (i = 0 ; i<supportedExtensionListCount ; i++)
		{
		TPtrC supportedExtension((*supportedExtensionList)[i]);
		HBufC8* supportedExtension8 = HBufC8::NewMaxLC(supportedExtension.Size());
		Mem::Copy((TAny *) supportedExtension8->Ptr(), supportedExtension.Ptr(), supportedExtension.Size());

		iniFileBuf.Append(*supportedExtension8);
		iniFileBuf.Append(KIniValueTerminator);
		iniFileBuf.Append(0);
		CleanupStack::PopAndDestroy(supportedExtension8);

		const CPluginInfo* selected = iPluginSupportedList->GetSelectedPluginInfo(supportedExtension);
		if (selected)
			{
			iniFileBuf.AppendNum((TInt) selected->iControllerUid.iUid);
			iniFileBuf.Append(KIniValueTerminator);
			}
		else
			{
			iniFileBuf.AppendNum(0);
			iniFileBuf.Append(KIniValueTerminator);
			}
		}
        
	CleanupStack::PopAndDestroy(supportedExtensionList);
#endif

	// Now we've got all the data, write the file
    RFile out;
	TInt err = out.Replace(iCoeEnv->FsSession(), iIniFileName, EFileWrite);
    if (err != KErrNone)
		{
        TRACEF(COggLog::VA(_L("WriteIni: File replace error: %d"), err ));
		return;
		}

	err = out.Write(iniFileBuf);
	if (err != KErrNone)
		{
        TRACEF(COggLog::VA(_L("WriteIni: File write error: %d"), err ));
		}

	out.Close();
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

	// Ignore the intial foreground event as we won't be ready
	if (iStartUpState != EStartUpComplete)
		return;

	if (aForeground)
		{
#if defined(SERIES60SUI)
		// With S60 Scalable UI the layout may have changed
		// Check to see if we need to activate a different view
		TVwsViewId viewId;
		GetActiveViewId(viewId);

		TBool flipOpen = iAppView->IsFlipOpen();
		if ((flipOpen && (viewId.iViewUid == KOggPlayUidFCView)) || (!flipOpen && (viewId.iViewUid == KOggPlayUidFOView)))
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

TBool COggPlayAppUi::ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName, const TDesC8& aTail)
	{
	TRACEF(COggLog::VA(_L("COggPlayAppUi::ProcessCPL: %d, %S"), (TInt) aCommand, &aDocumentName));

	// On UIQ and Series 80 we don't want to return ETrue if we are simply being asked to run
	if (aCommand == EApaCommandRun)
		return CEikAppUi::ProcessCommandParametersL(aCommand, aDocumentName, aTail);

	return ETrue;
	}

void COggPlayAppUi::OpenFileL(const TDesC& aFileName)
	{
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

void COggPlayAppUi::SetVolumeGainL(TGainType aNewGain)
	{
	iOggPlayback->SetVolumeGain(aNewGain);
	iSettings.iGainType = aNewGain;

#if defined(SERIES60)
	iPlaybackOptionsView->VolumeGainChangedL();
#endif
	}

void COggPlayAppUi::SetMp3Dithering(TBool aDithering)
	{
	((COggPlayback *) iOggPlayback)->SetMp3Dithering(aDithering);
	iSettings.iMp3Dithering = aDithering;
	}

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

	if ((aType == KEikDynamicLayoutVariantSwitch) && iForeground)
		{
		TVwsViewId viewId;
		GetActiveViewId(viewId);

		// If one of the main views is active, we need to switch to the other one
		if ((viewId.iViewUid == KOggPlayUidFOView) || (viewId.iViewUid == KOggPlayUidFCView))
			ActivateOggViewL();
		}
	}
#endif

void COggPlayAppUi::FileOpenErrorL(const TDesC& aFileName, TInt aErr)
	{
	TBuf<64> buf, buf2;
	CEikonEnv::Static()->ReadResourceL(buf, R_OGG_ERROR_14);

	buf2.Format(buf, aErr);
	iOggMsgEnv->OggErrorMsgL(buf2, aFileName);
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

TInt COggPlayAppUi::PlaybackFromFileName(const TDesC& aFileName, CAbsPlayback*& aOggPlayback, TUid& aControllerUid)
	{
	// Find the selected plugin, corresponding to file type
	TParsePtrC p(aFileName);
	TPtrC pp(p.Ext());
	if (!pp.Length())
		return KErrNotSupported;

    const CPluginInfo* info = iPluginSupportedList->GetSelectedPluginInfo(pp.Mid(1));
	if (info)
		{
		aOggPlayback = (info->iControllerUid == KOggPlayInternalController) ? iOggMTPlayback : iOggMMFPlayback;
		aControllerUid = info->iControllerUid;

		return KErrNone;
		}

	return KErrNotSupported;
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
        iAppView->SelectFile(iFileList[iPlayingIdx]);
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
