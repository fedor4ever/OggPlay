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

#include "OggControls.h"
#include "OggTremor.h"
#include "OggPlayAppView.h"
#include "OggDialogs.h" 

#include <OggPlay.rsg>

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

COggActive::COggActive(COggPlayAppUi* theAppUi) :
  iServer(0),
  iPhone(0),
  iLine(0),
  iLineStatus(),
  iRequestStatusChange(),
  iTimer(0),
  iCallBack(0),
  iInterrupted(0),
  iAppUi(theAppUi)
{
  iServer= new RTelServer;
  int ret= iServer->Connect();
  if (ret!=KErrNone) return;


  ret=iServer->LoadPhoneModule(KTsyName);
  if (ret!=KErrNone) return;

  int nPhones;
  ret=iServer->EnumeratePhones(nPhones);
  if (ret!=KErrNone) return;

  RTelServer::TPhoneInfo PInfo;
  ret= iServer->GetPhoneInfo(0,PInfo);
  if (ret!=KErrNone) return;

  iPhone= new RPhone;
  ret= iPhone->Open(*iServer,PInfo.iName);
  if (ret!=KErrNone) return;

  int nLines;
  ret= iPhone->EnumerateLines(nLines);
  if (ret!=KErrNone) return;

  RPhone::TLineInfo LInfo;
  for (int i=0; i<1; i++) {
    ret= iPhone->GetLineInfo(i,LInfo);
    if (ret!=KErrNone) return;
  }

  iLine= new RLine;
}

TInt
COggActive::CallBack(TAny* aPtr)
{
  COggActive* self= (COggActive*)aPtr;

  self->iAppUi->NotifyUpdate();

  RPhone::TLineInfo LInfo;
  self->iPhone->GetLineInfo(0,LInfo);
  self->iLine->Open(*self->iPhone,LInfo.iName);
  int nCalls=-1;
  self->iLine->EnumerateCall(nCalls);
  self->iLine->Close();

  bool isRinging= nCalls>0;
  bool isIdle   = nCalls==0;

  if (isRinging && !self->iInterrupted) {

    // the phone is ringing or someone is making a call, pause the music if any
    if (self->iAppUi->iOggPlayback->State()==TOggPlayback::EPlaying) {
      self->iInterrupted= 1;
      self->iAppUi->HandleCommandL(EOggPauseResume);
      return 1;
    }

  }

  if (self->iInterrupted) {
    // our music was interrupted by a phone call, now what
    if (isIdle) {
      // okay, the phone is idle again, let's continue with the music
      self->iInterrupted= 0;
      if (self->iAppUi->iOggPlayback->State()==TOggPlayback::EPaused)
	self->iAppUi->HandleCommandL(EOggPauseResume);
      return 1;
    }
  }

  return 1;
}

void
COggActive::IssueRequest()
{
  iTimer= CPeriodic::New(CActive::EPriorityStandard);
  iCallBack= new TCallBack(COggActive::CallBack,this);
  iTimer->Start(TTimeIntervalMicroSeconds32(1000000),TTimeIntervalMicroSeconds32(1000000),*iCallBack);
}

COggActive::~COggActive()
{
  if (iLine) { 
    iLine->Close();
    delete iLine; 
    iLine= 0;
  }
  if (iPhone) { 
    iPhone->Close();
    delete iPhone; 
    iPhone= 0; 
  }
  if (iServer) { 
    iServer->Close();
    delete iServer; 
    iServer= 0;
  }
}


////////////////////////////////////////////////////////////////
//
// App UI class, COggPlayAppUi
//
////////////////////////////////////////////////////////////////

void
COggPlayAppUi::ConstructL()
{
  BaseConstructL();

  iIniFileName.Copy(Application()->AppFullName());
  iIniFileName.SetLength(iIniFileName.Length() - 3);
  iIniFileName.Append(_L("ini"));

  iDbFileName.Copy(Application()->AppFullName());
  iDbFileName.SetLength(iDbFileName.Length() - 3);
  iDbFileName.Append(_L("db"));

  iRepeat= 1;
  iCurrent= -1;
  iHotkey= 0;
  iVolume= 100;
  iTryResume= 0;
  iAlarmTriggered= 0;
  iAlarmActive= 0;
  iAlarmTime.Set(_L("20030101:120000.000000"));
  iViewBy= ETitle;
  iAnalyzerState[0]= 0;
  iAnalyzerState[1]= 0;

  iOggPlayback= new TOggPlayback(iEikonEnv, this);

  ReadIniFile();

  iAppView=new(ELeave) COggPlayAppView;
  iAppView->ConstructL(this, ClientRect());

  AddToStackL(iAppView); // Receiving Keyboard Events 
  iAppView->InitView();

  SetHotKey();

  iActive= new(ELeave) COggActive(this);
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

  SetProcessPriority();
  SetThreadPriority();

  ActivateOggViewL();
}


COggPlayAppUi::~COggPlayAppUi()
{
  RemoveFromStack(iAppView);

  if (iFOView) {
    DeregisterView(*iFOView);
    delete iFOView;
  }
  if (iFCView) {
    DeregisterView(*iFCView);
    delete iFCView;
  }

  if (iActive) { delete iActive; iActive=0; }
  WriteIniFile();
  if (iOggPlayback) { delete iOggPlayback; iOggPlayback=0; }
  iEikonEnv->RootWin().CancelCaptureKey(iCapturedKeyHandle);
}

void COggPlayAppUi::ActivateOggViewL()
{
  TVwsViewId viewId;
  viewId.iAppUid = KOggPlayUid;
  viewId.iViewUid = iAppView->IsFlipOpen() ? KOggPlayUidFOView : KOggPlayUidFCView;
  
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
  if (iTryResume>0 && iOggPlayback->State()==TOggPlayback::EPaused) {
    if (iOggPlayback->State()!=TOggPlayback::EPaused) iTryResume= 0;
    else {
      iTryResume--;
      if (iTryResume==0 && iOggPlayback->State()==TOggPlayback::EPaused) HandleCommandL(EOggPauseResume);
    }
  }

  iAppView->UpdateClock();

  if (iOggPlayback->State()!=TOggPlayback::EPlaying &&
      iOggPlayback->State()!=TOggPlayback::EPaused) return; 

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

  if (iCurrent+1==nSongs && iRepeat) {
    // We are at the end of the playlist, and "repeat" is enabled
    if (iAppView->GetItemType(0)==6) SetCurrent(1); else SetCurrent(0);
  } 
  else if (iCurrent+1<nSongs) {
    // We are in the middle of the playlist. Now play the next song.
    SetCurrent(iCurrent+1);
  }
  else {
    // We are at the end of the playlist. Stop playing.
    SetCurrent(-1);
    return;
  }

  if (iCurrentSong.Length()>0 && iOggPlayback->Open(iCurrentSong)==KErrNone) {
    iOggPlayback->Play();
    iAppView->SetTime(iOggPlayback->Time());
    iAppView->Update();
  } else SetCurrent(-1);

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

  switch (aCommand) {

  case EOggPlay: {
    if (iViewBy==ETop) {
      TInt i= iAppView->GetSelectedIndex();
      if (i>=ETitle && i<=EFileName) HandleCommandL(EOggViewByTitle+i);
      break;
    }
    if (iAppView->GetItemType(idx)==6) {
      // the back item was selected: show the previous view
      TLex parse(curFile);
      TInt previousView;
      parse.Val(previousView);
      if (previousView==ETop) {
	//iViewBy= ETop;
	TBuf<16> dummy;
	iAppView->FillView(ETop, ETop, dummy);
      }
      else HandleCommandL(EOggViewByTitle+previousView);
      break;
    }
    if (iViewBy==EAlbum || iViewBy==EArtist || iViewBy==EGenre || iViewBy==ESubFolder) {
      iAppView->FillView(ETitle, iViewBy, curFile);
      //iViewBy= ETitle;
      if (iCurrentSong.Length()>0) SetCurrent(iCurrentSong);
      break;
    }
    if (iViewBy==ETitle || iViewBy==EFileName) {
      if (curFile.Length()>0) {
	if (iOggPlayback->State()==TOggPlayback::EPlaying ||
	    iOggPlayback->State()==TOggPlayback::EPaused) iOggPlayback->Stop();
	if (iOggPlayback->Open(curFile)==KErrNone) {
	  iOggPlayback->SetVolume(iVolume);
	  if (iOggPlayback->State()!=TOggPlayback::EPlaying) {
	    iAppView->SetTime(iOggPlayback->Time());
	    iOggPlayback->Play();
	  }
	  SetCurrent(idx);
	} else SetCurrent(-1);
      }
    }
    break;
  }
    
  case EOggStop: {
    if (iOggPlayback->State()==TOggPlayback::EPlaying ||
	iOggPlayback->State()==TOggPlayback::EPaused) {
      iOggPlayback->Stop();
      SetCurrent(-1);
    }
    break;
  }

  case EOggPauseResume: {
    if (iOggPlayback->State()==TOggPlayback::EPlaying) iOggPlayback->Pause();
    else if (iOggPlayback->State()==TOggPlayback::EPaused) iOggPlayback->Play();
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
#endif
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
    iAppView->iOggFiles->CreateDb();
    iAppView->iOggFiles->WriteDb(iDbFileName, iCoeEnv->FsSession());
    TBuf<16> dummy;
    //iViewBy= ETop;
    iAppView->FillView(ETop, ETop, dummy);
    break;
  }

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
  if (aMenuId==R_FILE_MENU) {
    if (iRepeat) aMenuPane->SetItemButtonState(EOggRepeat, EEikMenuItemSymbolOn);
    TBool isSongList= ((iViewBy==ETitle) || (iViewBy==EFileName));
    aMenuPane->SetItemDimmed(EOggInfo   , iCurrentSong.Length()==0 && (iAppView->GetSelectedIndex()<0 || !isSongList));
    aMenuPane->SetItemDimmed(EOggShuffle, iAppView->GetNSongs()<1 || !isSongList);
  }

  /*
  if (aMenuId==R_VIEWBY_MENU) {
    switch (iViewBy) {
    case ETitle    : aMenuPane->SetItemButtonState(EOggViewByTitle, EEikMenuItemSymbolOn); break;
    case EAlbum    : aMenuPane->SetItemButtonState(EOggViewByAlbum, EEikMenuItemSymbolOn); break;
    case EArtist   : aMenuPane->SetItemButtonState(EOggViewByArtist, EEikMenuItemSymbolOn); break;
    case EGenre    : aMenuPane->SetItemButtonState(EOggViewByGenre, EEikMenuItemSymbolOn); break;
    case EFileName : aMenuPane->SetItemButtonState(EOggViewByFileName, EEikMenuItemSymbolOn); break;
    case ESubFolder: aMenuPane->SetItemButtonState(EOggViewBySubFolder, EEikMenuItemSymbolOn); break;
    default: break;
    }
  }
  */
}

void
COggPlayAppUi::ReadIniFile()
{
  RFile in;
  if(in.Open(iCoeEnv->FsSession(), iIniFileName,
	     EFileRead|EFileStreamText) != KErrNone) return;

  TFileText tf;
  tf.Set(in);
  TBuf<128> line;

  iHotkey= 0;
  if(tf.Read(line) == KErrNone) {
    TLex parse(line);
    parse.Val(iHotkey);
  };

  iRepeat= 1;
  if (tf.Read(line) == KErrNone) {
    TLex parse(line);
    parse.Val(iRepeat);
  };

  iVolume= 100;
  if (tf.Read(line) == KErrNone) {
    TLex parse(line);
    parse.Val(iVolume);
    if (iVolume<0) iVolume= 0;
    else if (iVolume>100) iVolume= 100;
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

  iAnalyzerState[0]= 0;
  if (tf.Read(line) == KErrNone) {
    TLex parse(line);
    parse.Val(iAnalyzerState[0]);
  }

  iAnalyzerState[1]= 0;
  if (tf.Read(line) == KErrNone) {
    TLex parse(line);
    parse.Val(iAnalyzerState[1]);
  }

  //iViewBy= ETitle;
  //if (tf.Read(line) == KErrNone) {
  //TLex parse(line);
  //parse.Val(iViewBy);
  //}

  //tf.Read(iAlbum);
  //tf.Read(iArtist);
  //tf.Read(iGenre);
  //tf.Read(iSubFolder);

  in.Close();
}

void
COggPlayAppUi::WriteIniFile()
{
  RFile out;
  if(out.Replace(iCoeEnv->FsSession(), iIniFileName,
		 EFileWrite|EFileStreamText) != KErrNone) return;

  TFileText tf;
  tf.Set(out);

  TBuf<64> num;
  num.Num(iHotkey);
  tf.Write(num);

  num.Num(iRepeat);
  tf.Write(num);

  num.Num(iVolume);
  tf.Write(num);

  num.Num(iAlarmTime.Int64());
  tf.Write(num);

  num.Num(iAnalyzerState[0]);
  tf.Write(num);

  num.Num(iAnalyzerState[1]);
  tf.Write(num);

  //num.Num(iViewBy);
  //tf.Write(num);

  //tf.Write(iAlbum);
  //tf.Write(iArtist);
  //tf.Write(iGenre);
  //tf.Write(iSubFolder);

  out.Close();
}

void 
COggPlayAppUi::HandleForegroundEventL(TBool aForeground)
{
  CEikAppUi::HandleForegroundEventL(aForeground);
  iForeground= aForeground;
}

void
COggPlayAppUi::SetProcessPriority()
{
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
}

void
COggPlayAppUi::SetThreadPriority()
{
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
}


