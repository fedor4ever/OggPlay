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

#if defined(SERIES60)
#include <aknappui.h>
#include <aknapp.h>
#include <eikdoc.h>
#include <stdio.h>
const TInt UID3=0x0BF72853; // FIXME!!

const TInt KFullScreenWidth = 176;
const TInt KFullScreenHeight = 188;

#else
#include <qikappui.h>
#include <qikapplication.h>
#include <qikdocument.h>

const TInt KFullScreenWidth = 208;
const TInt KFullScreenHeight = 320;

#endif

#include <coeccntx.h>  // MCoeControlContex
#include <e32base.h>
#include <etel.h>

#include <OggPlay.rsg>
#include "OggPlay.hrh"
#include "OggTremor.h"

#include "OggViews.h"
#include "OggControls.h"
#include "OggMsgEnv.h"

// some global constants:
//-----------------------
//FIXME

const TUid KOggPlayUid1 = { 0x1000007a };
const TUid KOggPlayUid2 = { 0x100039ce };
const TUid KOggPlayUid  = { UID3 }; //0x1000A661;

const TUid KOggPlayUidFOView = { UID3 +1 };
const TUid KOggPlayUidFCView = { UID3 +2 };

//_LIT(KAudioResourceFile, "Z:\\System\\Apps\\Audio\\audio.mbm");
_LIT(KAudioResourceFile, "Z:\\System\\Apps\\OggPlay\\OggPlay.mbm");
_LIT(KQTimeResourceFile, "Z:\\System\\Apps\\QTime\\QTime.mbm");

const TInt KMaxFileNameLength = 0x200;


static const struct {
  int code; 
  int mask;
} keycodes[] = {
  { EKeyApplication1,	0 }, // camera
  { EKeyApplication0,	0 }, // browser
  { EKeyDevice0,	0 }, // power
  { 0,			0 }, // flip open
  // EKeyDevice8 confirm (jog dial push)
  // EKeyDevice1/2  two way controller
  // EKeyDevice4..7 four way controller 
  // EKeyDial
};


// Forward declarations:
//----------------------

class COggPlayAppUi;
class COggPlayAppView;
class COggFOView;

class CEikColumnListBox;
class CEikBitmapButton;


// COggActive
// A utility class monitoring the telephone line
//----------------------------------------------

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


#ifdef SERIES60
class COggPlayAppUi : public CAknAppUi, public MPlaybackObserver
#else
class COggPlayAppUi : public CQikAppUi, public MPlaybackObserver
#endif
{
public:

  void ConstructL();
  ~COggPlayAppUi();

  // the views supported by the listbox:
  enum TViews { ETitle=0, EAlbum, EArtist, EGenre, ESubFolder, EFileName, ETop, ETrackTitle };

  // global settings stored in the ini file:
  int iHotkey;
  int iVolume;            // [0...100]
  int iRepeat;            // 0=off; 1=on
  int iAnalyzerState;     // 0= off; 1=on; 2= on with peaks
  TViews iViewBy;
  TTime iAlarmTime;       // the alarm has been set to this time

  // global status:
  int iCurrent;           // index of the file which is currently being played
  TBuf<512> iCurrentSong; // full path and filename of the current song
  int iTryResume;         // after iTryResume seconds try resuming music (after sound device was stolen)
  int iAlarmTriggered;    // did the alarm clock go off?
  int iAlarmActive;       // has an alarm time been set?

  TBool iForeground;      // is the application currently in the foreground?
  TBool iIsRunningEmbedded;

  TFileName iDbFileName;
  
  COggPlayAppView* iAppView;
  COggPlayback*    iOggPlayback;
  COggActive*      iActive;

  // from MPlaybackObserver:
  virtual void NotifyPlayComplete();
  virtual void NotifyUpdate();
  virtual void NotifyPlayInterrupted();

  // high level functions:
  void NextSong();
  void PreviousSong();
  void ShowFileInfo();

  void ActivateOggViewL();

  // from CQikAppUi:
  void HandleCommandL(int aCommand);
  void HandleApplicationSpecificEventL(TInt aType, const TWsEvent& aEvent);
  void HandleForegroundEventL(TBool aForeground);
  void DynInitMenuPaneL(int aMenuId, CEikMenuPane* aMenuPane);
  TBool ProcessCommandParametersL(TApaCommand aCommand, TFileName& aDocumentName,const TDesC8& aTail);

  void OpenFileL(const TDesC& aFileName);

private: 

  void ReadIniFile();
  void WriteIniFile();
  void SetHotKey();
  TBool IsAlarmTime();
  void SetCurrent(int aCurrent);
  void SetCurrent(const TDesC& aFileName);
  void SetProcessPriority();
  void SetThreadPriority();
  void FindSkins();
#if defined(SERIES60_SPLASH)
  void ShowSplashL();
  TBool iSplashActive;
#endif


  int iCapturedKeyHandle;
  HBufC* iIniFileName;
  TFileName iSkinFileDir;
  TInt iCurrentSkin;
  CDesCArray* iSkins;

  COggFOView* iFOView;
  COggFCView* iFCView;
  COggMsgEnv* iOggMsgEnv;
  RArray<TInt> iViewHistoryStack;
};

#ifdef SERIES60
class COggPlayDocument : public CEikDocument
#else
class COggPlayDocument : public CQikDocument
#endif
{
public:
#ifdef SERIES60
  COggPlayDocument(CAknApplication& aApp) : CEikDocument(aApp) {}

  CFileStore*  OpenFileL(TBool aDoOpen ,const TDesC& aFilename, RFs& aFs);

#else
  COggPlayDocument(CQikApplication& aApp) : CQikDocument(aApp) {}
#endif
  
  CEikAppUi* CreateAppUiL() ;
};

#ifdef SERIES60
class COggPlayApplication : public CAknApplication
#else
class COggPlayApplication : public CQikApplication
#endif
{
private: 
  // from CApaApplication
  CApaDocument* CreateDocumentL() { return new (ELeave)COggPlayDocument(*this); }
  TUid AppDllUid() const { TUid id = { UID3 }; return id; }
};


#endif
