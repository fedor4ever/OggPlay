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

#include "OggPlayAppView.h"
#include "OggOs.h"

#include <eikclb.h>     // CEikColumnListbox
#include <eikclbd.h>    // CEikColumnListBoxData
#include <eiktxlbm.h>   // CEikTextListBox(Model)
#include <eikmenub.h>   // CEikMenuBar
#include <apgtask.h>    // TApaTask
#ifdef SERIES60
#include <aknkeys.h>	
#else
#include <quartzkeys.h>	// EStdQuartzKeyConfirm etc.
#endif
#include <stdlib.h>

COggPlayAppView::COggPlayAppView() :
  CCoeControl(),
  MCoeControlObserver(),
  MCoeControlContext()
{
  iControls= 0;
  iOggFiles= 0;
  iActivateCount= 0;
  iPosChanged= -1;
  iApp= 0;
  iBoldFont= 0;
}

COggPlayAppView::~COggPlayAppView()
{
  if (iControls) { delete iControls; iControls= 0; }
  if (iBoldFont) iCoeEnv->ScreenDevice()->ReleaseFont(iBoldFont);
}

void
COggPlayAppView::ConstructL(COggPlayAppUi *aApp, const TRect& aRect)
{
  iApp = aApp;
  CreateWindowL();
  SetRect(aRect);

  SetComponentsToInheritVisibility(EFalse);

  iOggFiles= new TOggFiles(iApp->iOggPlayback);
 
  iControls = new(ELeave)CArrayPtrFlat<CCoeControl>(10);

  iIconFileName.Copy(iApp->Application()->AppFullName());
  iIconFileName.SetLength(iIconFileName.Length() - 3);
  iIconFileName.Append(_L("mbm"));

  iBackgroundFileName.Copy(iApp->Application()->AppFullName());
  iBackgroundFileName.SetLength(iBackgroundFileName.Length() - 3);
  iBackgroundFileName.Append(_L("bmp"));

  // construct the two canvases for flip-open and closed mode:
  //----------------------------------------------------------

  // flip-open mode:
  iCanvas[0]= new(ELeave) COggCanvas;
  iControls->AppendL(iCanvas[0]);
  CleanupStack::PushL(iCanvas[0]);
  iCanvas[0]->SetContainerWindowL(*this);
  iCanvas[0]->SetControlContext(this);
  iCanvas[0]->SetObserver(this);
  CleanupStack::Pop();
  TPoint pBM(Position());
  pBM.iY= 0;
  iCanvas[0]->SetExtent(pBM,TSize(KFullScreenWidth,KFullScreenHeight));

  // flip-closed mode:
  iCanvas[1]= new(ELeave) COggCanvas;
  iControls->AppendL(iCanvas[1]);
  CleanupStack::PushL(iCanvas[1]);
  iCanvas[1]->SetContainerWindowL(*this);
  iCanvas[1]->SetControlContext(this);
  iCanvas[1]->SetObserver(this);
  CleanupStack::Pop();
  pBM.iY= 0;
  iCanvas[1]->SetExtent(pBM,TSize(208,144));

  // set up all the controls:
  CreateDefaultSkin();

  iCanvas[0]->SetFocus(ETrue);

  // start the canvas refresh timer (20 ms):
  iTimer= CPeriodic::New(CActive::EPriorityStandard);
  TCallBack* iCallBack= new TCallBack(COggPlayAppView::CallBack,this);
  iTimer->Start(TTimeIntervalMicroSeconds32(1000000),TTimeIntervalMicroSeconds32(75000),*iCallBack);

  ActivateL();
}

void 
COggPlayAppView::CreateDefaultSkin()
{

  // Create and set up the flip-closed mode canvas:
  //-----------------------------------------------

  iCanvas[1]->ConstructL(iIconFileName, 18);

  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());
  //TFontSpec fs(_L("Courier"),14);
  //fs.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
  //iCoeEnv->ScreenDevice()->GetNearestFontInPixels(iBoldFont,fs);
  
  // setup text lines:
  iTitle[1] = new COggText(6,  55, Size().iWidth- 12, 16); iCanvas[1]->AddControl(iTitle[1]); iTitle[1]->SetFont(iFont);
  iAlbum[1] = new COggText(6,  75, Size().iWidth- 12, 16); iCanvas[1]->AddControl(iAlbum[1]); iAlbum[1]->SetFont(iFont);
  iArtist[1]= new COggText(6,  95, Size().iWidth- 12, 16); iCanvas[1]->AddControl(iArtist[1]); iArtist[1]->SetFont(iFont);
  iClock[1] = new COggText(134,  15, 59, 14); iCanvas[1]->AddControl(iClock[1]); iClock[1]->SetFont(iFont);
  iAlarm[1] = new COggText(134,  30, 59, 14); iCanvas[1]->AddControl(iAlarm[1]); iAlarm[1]->SetFont(iFont);
  iPlayed[1]= new COggText(60,  120, Size().iWidth-120, 16); iPlayed[1]->SetFont(iFont);
  iPlayed[1]->SetText(_L("00:00 / 00:00"));
  iCanvas[1]->AddControl(iPlayed[1]);

  // setup icons:

  iPlaying[1]= new COggIcon(38, 119, 20, 16); 
  iPlaying[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,34,35));
  iCanvas[1]->AddControl(iPlaying[1]);

  iPaused[1]= new COggIcon(38, 119, 20, 16); 
  iPaused[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,16,17));
  iPaused[1]->Hide(); 
  iCanvas[1]->AddControl(iPaused[1]);

  iAlarmIcon[1]= new COggIcon(185, 30, 20, 16);
  iAlarmIcon[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,39,40));
  iAlarmIcon[1]->Hide();
  iCanvas[1]->AddControl(iAlarmIcon[1]);

  // setup sliders:
  iVolume[1]= new COggSlider(74, 11, 60, 16);
  iVolume[1]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,20,21));
  iVolume[1]->SetObserver(this);
  iCanvas[1]->AddControl(iVolume[1]);

  // this is the eye of the fish (it blinks to the full hour):
  iEye[1]= new COggIcon(41, 16, 16, 16);
  iEye[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,22,22));
  iEye[1]->SetBlinkFrequency(2);
  iEye[1]->Hide();
  iCanvas[1]->AddControl(iEye[1]);

  iRepeat[1]= new COggIcon(145, 117, 20, 16);
  iRepeat[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,23,24));
  iRepeat[1]->MakeVisible(iApp->iRepeat);
  iCanvas[1]->AddControl(iRepeat[1]);

  // These controls appear just in flip-open mode:
  iAnalyzer[1]= 0;
  iPosition[1]= 0;
  iPlayButton[1]= 0;
  iPauseButton[1]= 0;
  iStopButton[1]= 0;
  iListBox[1]= 0;
  iScrollBar[1]= 0;

  // Create and set up the flip-open mode canvas:
  //---------------------------------------------

  iCanvas[0]->ConstructL(iIconFileName, 19);

  // most of the controls are reused in flip-open mode:

  iTitle[0]= iTitle[1]; iCanvas[0]->AddControl(iTitle[0]);
  iAlbum[0]= iAlbum[1]; iCanvas[0]->AddControl(iAlbum[0]);
  iArtist[0]= iArtist[1]; iCanvas[0]->AddControl(iArtist[0]);
  iClock[0]= iClock[1]; iCanvas[0]->AddControl(iClock[0]);
  iAlarm[0]= iAlarm[1]; iCanvas[0]->AddControl(iAlarm[0]);
  iAlarmIcon[0]= iAlarmIcon[1]; iCanvas[0]->AddControl(iAlarmIcon[0]);
  iVolume[0]= iVolume[1]; iCanvas[0]->AddControl(iVolume[0]);

  // and some of them do not appear at all:

  iEye[0]= 0; 
  iPlayed[0]= 0;
  iPlaying[0]= 0;
  iRepeat[0]= 0;

  // others appear only in flip-open mode:  
  
  iAnalyzer[0]= new COggAnalyzer(138, 70, 70, 30);
  iAnalyzer[0]->SetBarIcon(iEikonEnv->CreateIconL(iIconFileName,29,30));
  iAnalyzer[0]->SetObserver(this);
  iCanvas[0]->AddControl(iAnalyzer[0]);

  const TInt y(Size().iHeight- 27);

  iPosition[0]= new COggSlider(85, y+1, 110, 16);
  iPosition[0]->SetStyle(2);
  iPosition[0]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,25,26));
  iPosition[0]->SetObserver(this);
  iCanvas[0]->AddControl(iPosition[0]);

  iStopButton[0]= new COggButton(11, y-1, 20, 20);
  iStopButton[0]->SetNormalIcon(iEikonEnv->CreateIconL(iIconFileName, 31, 32));
  iStopButton[0]->SetDimmedIcon(iEikonEnv->CreateIconL(iIconFileName, 33, 33));
  iStopButton[0]->SetObserver(this);
  iCanvas[0]->AddControl(iStopButton[0]);

  iPlayButton[0]= new COggButton(46, y, 20, 20);
  iPlayButton[0]->SetNormalIcon(iEikonEnv->CreateIconL(iIconFileName, 34, 35));
  iPlayButton[0]->SetDimmedIcon(iEikonEnv->CreateIconL(iIconFileName, 36, 36));
  iPlayButton[0]->SetObserver(this);
  iCanvas[0]->AddControl(iPlayButton[0]);

  iPauseButton[0]= new COggButton(46, y, 20, 20);
  iPauseButton[0]->SetNormalIcon(iEikonEnv->CreateIconL(iIconFileName, 37, 38));
  iPauseButton[0]->SetObserver(this);
  iPauseButton[0]->MakeVisible(EFalse);
  iCanvas[0]->AddControl(iPauseButton[0]);

  iPauseButton[0]->SetObserver(this);

  iScrollBar[0]= new COggScrollBar(188, 113, 16, 96); 
  iScrollBar[0]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,27,28));
  iScrollBar[0]->SetStyle(1);

  iListBox[0]= new COggListBox(6, 116, 182, 96);
  iListBox[0]->SetFont(iFont);
  iListBox[0]->SetVertScrollBar(iScrollBar[0]);
  iListBox[0]->SetObserver(this);
  CGulIcon* titleIcon = iEikonEnv->CreateIconL(iIconFileName,0,1);
  CGulIcon* albumIcon = iEikonEnv->CreateIconL(iIconFileName,2,3);
  CGulIcon* artistIcon= iEikonEnv->CreateIconL(iIconFileName,4,5);
  CGulIcon* genreIcon = iEikonEnv->CreateIconL(iIconFileName,6,7);
  CGulIcon* folderIcon= iEikonEnv->CreateIconL(iIconFileName,8,9);
  CGulIcon* fileIcon  = iEikonEnv->CreateIconL(iIconFileName,10,11);
  CGulIcon* backIcon  = iEikonEnv->CreateIconL(iIconFileName,12,13);
  CGulIcon* playIcon  = iEikonEnv->CreateIconL(iIconFileName,34,35);
  CGulIcon* pausedIcon= iEikonEnv->CreateIconL(iIconFileName,16,17);
  CColumnListBoxData* cd= iListBox[0]->GetColumnListBoxData();
  CArrayPtr<CGulIcon>* icons= cd->IconArray();
  if (!icons) {
    icons = new CArrayPtrFlat<CGulIcon>(3);
    cd->SetIconArray(icons);
  }
  icons->Reset();
  icons->AppendL(titleIcon);
  icons->AppendL(albumIcon);
  icons->AppendL(artistIcon);
  icons->AppendL(genreIcon);
  icons->AppendL(folderIcon);
  icons->AppendL(fileIcon);
  icons->AppendL(backIcon);
  icons->AppendL(playIcon);
  icons->AppendL(pausedIcon);

  iScrollBar[0]->SetAssociatedControl(iListBox[0]);

  iCanvas[0]->AddControl(iListBox[0]);
  iCanvas[0]->AddControl(iScrollBar[0]);
}

void 
COggPlayAppView::GotoFlipClosed()
{
  // modify controls for flip-closed mode display
  iTitle[1]->iy= 55;
  iAlbum[1]->iy= 75;
  iArtist[1]->iy= 95;
  iVolume[1]->ix= 74;
  iVolume[1]->iy= 11;
  iClock[1]->ix= 134;
  iClock[1]->iy= 15;
  iAlarm[1]->iy= 30;
  iAlarmIcon[1]->iy= 30;
}

void
COggPlayAppView::GotoFlipOpen()
{
  // modify controls for flip-open mode display
  iTitle[0]->iy= 20;
  iAlbum[0]->iy= 36;
  iArtist[0]->iy= 52;
  iVolume[0]->ix= 36;
  iVolume[0]->iy= 79;
  iClock[0]->ix= 73;
  iClock[0]->iy= 3;
  iAlarm[0]->iy= 3;
  iAlarmIcon[0]->iy= 3;
}

TBool
COggPlayAppView::IsFlipOpen()
{
#if defined(SERIES60)
	return 1;
#else
	return iCoeEnv->ScreenDevice()->CurrentScreenMode() == 0;
#endif
}

void COggPlayAppView::Activated()
{
  if (iActivateCount == 0) MakeVisible(ETrue);
  iActivateCount++;
}

void COggPlayAppView::Deactivated()
{
  iActivateCount--;
  if (iActivateCount == 0) MakeVisible(EFalse);
}

void COggPlayAppView::ChangeLayout(TBool aSmall)
{
  TRect	rect(CEikonEnv::Static()->ScreenDevice()->SizeInPixels());
  if (!aSmall)
    rect = static_cast<CEikAppUi*>(iEikonEnv->AppUi())->ClientRect();
  SetRect(rect);

  if (aSmall) {
    iMode=1;  
    GotoFlipClosed();
    Update();
    iCanvas[1]->DrawControl();
    iCanvas[1]->MakeVisible(ETrue);
    iCanvas[0]->MakeVisible(EFalse);
  } else {
    iMode= 0;
    GotoFlipOpen();
    Update();
    iCanvas[0]->DrawControl();
    iCanvas[0]->MakeVisible(ETrue);
    iCanvas[1]->MakeVisible(EFalse);
  }
}

void
COggPlayAppView::AppToForeground(const TBool aForeground) const
{
  RWsSession& ws=iEikonEnv->WsSession();
  RWindowGroup& rw = iEikonEnv->RootWin();
  TInt winId = rw.Identifier();
  TApaTask tApatsk(ws);
  tApatsk.SetWgId(winId);
  if (aForeground) {
    tApatsk.BringToForeground();
    iApp->ActivateOggViewL();
  }
  else
    tApatsk.SendToBackground();
}

TInt
COggPlayAppView::GetNSongs()
{
  return iListBox[0]->CountText();
}

TBuf<512>
COggPlayAppView::GetFileName(TInt idx)
{
  CDesCArray* arr= GetTextArray();
  if (!arr) return 0;

  TPtrC sel,msel;
  if(idx >= 0 && idx < arr->Count()) {
    sel.Set((*arr)[idx]);
    TextUtils::ColumnText(msel, 2, &sel);
    return msel;
  }
  return msel;
}

TInt
COggPlayAppView::GetItemType(TInt idx)
{
  CDesCArray* arr= GetTextArray();
  if (!arr) return 0;

  TPtrC sel,msel;
  if(idx >= 0 && idx < arr->Count()) {
    sel.Set((*arr)[idx]);
    TextUtils::ColumnText(msel, 0, &sel);
    TLex parse(msel);
    TInt atype;
    parse.Val(atype);
    return atype;
  }
  return -1;
}

void
COggPlayAppView::SelectSong(TInt idx)
{
  if (iListBox[0]) {
    iListBox[0]->SetCurrentItemIndex(idx);
    iCurrentSong= GetFileName(idx);
    UpdateControls();
  }
}

TInt
COggPlayAppView::GetSelectedIndex()
{
  return iListBox[0]->CurrentItemIndex();
}

CDesCArray*
COggPlayAppView::GetTextArray()
{
  return iListBox[0]->GetTextArray();
}

void
COggPlayAppView::ShufflePlaylist()
{
  if (iApp->iViewBy!=COggPlayAppUi::ETitle && iApp->iViewBy!=COggPlayAppUi::EFileName) return;

  CDesCArray *arr= GetTextArray();
  TTime t;
  t.HomeTime();
  srand(t.DateTime().MicroSecond());
  TInt offset(0);
  if (GetItemType(0)==6) offset=1;
  for (TInt i=0; i<arr->Count(); i++) {
    float rnd= (float)rand()/RAND_MAX;
    TInt pick= (int)rnd*(arr->Count()-i);
    pick += i;
    TBuf<512> buf((*arr)[pick]);
    if (pick>=offset) {
      arr->Delete(pick);
      arr->InsertL(offset,buf);
    }
  }
  if (iListBox[iMode]) iListBox[iMode]->Redraw();
  SelectSong(0);
  Invalidate();
  TBuf<128> buf;
  iEikonEnv->ReadResource(buf, R_OGG_STRING_5);
  if (!IsFlipOpen()) User::InfoPrint(buf);
}

void
COggPlayAppView::SetTime(TInt64 aTime)
{
  if (iPosition[iMode]) iPosition[iMode]->SetMaxValue(aTime.GetTInt());
}

void
COggPlayAppView::ToggleRepeat()
{
  if (iApp->iRepeat) {
    iApp->iRepeat=0; 
    if (iRepeat[iMode]) iRepeat[iMode]->Hide();
  } else {
    iApp->iRepeat=1;
    if (iRepeat[iMode]) iRepeat[iMode]->Show();
  }
}

TInt
COggPlayAppView::CallBack(TAny* aPtr)
{
  COggPlayAppView* self= (COggPlayAppView*)aPtr;

  if (self->iApp->iForeground) {

    if (self->iAnalyzer[self->iMode] && self->iAnalyzer[self->iMode]->Style()>0) {
      if (self->iApp->iOggPlayback->State()==TOggPlayback::EPlaying)
	self->iAnalyzer[self->iMode]->RenderWaveform((short int[2][512])self->iApp->iOggPlayback->GetDataChunk());
    }
    
    if (self->iPosition[self->iMode] && self->iPosChanged<0) 
      self->iPosition[self->iMode]->SetValue(self->iApp->iOggPlayback->Position().GetTInt());
    
    self->iCanvas[self->iMode]->Refresh();
  }

  return 1;
}

void
COggPlayAppView::Invalidate()
{
  iCanvas[iMode]->Invalidate();
}

void
COggPlayAppView::InitView()
{
  // set up the layout of the listbox:
  CColumnListBoxData* cd= iListBox[0]->GetColumnListBoxData(); 
  TInt w = Size().iWidth;
  cd->SetColumnWidthPixelL(0,18);
  cd->SetGraphicsColumnL(0, ETrue);
  cd->SetColumnAlignmentL(0, CGraphicsContext::ECenter);
  cd->SetColumnWidthPixelL(1, w-18);
  cd->SetColumnAlignmentL(1, CGraphicsContext::ELeft);
  cd->SetColumnWidthPixelL(2, w);
  cd->SetColumnAlignmentL(2, CGraphicsContext::ELeft);

  // fill the list box with some initial content:
  if (!iOggFiles->ReadDb(iApp->iDbFileName,iCoeEnv->FsSession())) {
    iOggFiles->CreateDb();
    iOggFiles->WriteDb(iApp->iDbFileName, iCoeEnv->FsSession());
  }

  const TBuf<16> dummy;
  //iApp->iViewBy= COggPlayAppUi::ETop;
  FillView(COggPlayAppUi::ETop, COggPlayAppUi::ETop, dummy);

  if (iAnalyzer[0]) iAnalyzer[0]->SetStyle(iApp->iAnalyzerState[0]);
  if (iAnalyzer[1]) iAnalyzer[1]->SetStyle(iApp->iAnalyzerState[1]);
}

void
COggPlayAppView::FillView(COggPlayAppUi::TViews theNewView, COggPlayAppUi::TViews thePreviousView, const TDesC& aSelection)
{
  if (theNewView==COggPlayAppUi::ETop) {
    GetTextArray()->Reset();
    TBuf<32> dummy, btitles, balbums, bartists, bgenres, bsubfolders, bfiles;
    iEikonEnv->ReadResource(btitles, R_OGG_STRING_6);
    iEikonEnv->ReadResource(balbums, R_OGG_STRING_7);
    iEikonEnv->ReadResource(bartists, R_OGG_STRING_8);
    iEikonEnv->ReadResource(bgenres, R_OGG_STRING_9);
    iEikonEnv->ReadResource(bsubfolders, R_OGG_STRING_10);
    iEikonEnv->ReadResource(bfiles, R_OGG_STRING_11);
    TOggFiles::AppendLine(*GetTextArray(), COggPlayAppUi::ETitle, btitles, dummy);
    TOggFiles::AppendLine(*GetTextArray(), COggPlayAppUi::EAlbum, balbums, dummy);
    TOggFiles::AppendLine(*GetTextArray(), COggPlayAppUi::EArtist, bartists, dummy);
    TOggFiles::AppendLine(*GetTextArray(), COggPlayAppUi::EGenre, bgenres, dummy);
    TOggFiles::AppendLine(*GetTextArray(), COggPlayAppUi::ESubFolder, bsubfolders, dummy);
    TOggFiles::AppendLine(*GetTextArray(), COggPlayAppUi::EFileName, bfiles, dummy);
  }
  else if (thePreviousView==COggPlayAppUi::ETop) {
    TBuf<16> dummy;
    switch (theNewView) {
    case COggPlayAppUi::ETitle: iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, dummy); break;
    case COggPlayAppUi::EAlbum: iOggFiles->FillAlbums(*GetTextArray(), dummy, dummy); break;
    case COggPlayAppUi::EArtist: iOggFiles->FillArtists(*GetTextArray(), dummy); break;
    case COggPlayAppUi::EGenre: iOggFiles->FillGenres(*GetTextArray(), dummy, dummy, dummy); break;
    case COggPlayAppUi::ESubFolder: iOggFiles->FillSubFolders(*GetTextArray()); break;
    case COggPlayAppUi::EFileName: iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, dummy); break;
    default: break;
    }
    TBuf<256> back;
    back.Num(6);
    back.Append(KColumnListSeparator);
    back.Append(_L(".."));
    back.Append(KColumnListSeparator);
    back.AppendNum(thePreviousView);
    GetTextArray()->InsertL(0,back);
  }
  else {
    TBuf<16> dummy;
    switch (thePreviousView) {
    case COggPlayAppUi::ETitle    : iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, dummy); break;
    case COggPlayAppUi::EAlbum    : iOggFiles->FillTitles(*GetTextArray(), aSelection, dummy, dummy, dummy); break;
    case COggPlayAppUi::EArtist   : iOggFiles->FillTitles(*GetTextArray(), dummy, aSelection, dummy, dummy); break;
    case COggPlayAppUi::EGenre    : iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, aSelection, dummy); break;
    case COggPlayAppUi::ESubFolder: iOggFiles->FillTitles(*GetTextArray(), dummy, dummy, dummy, aSelection); break;
    case COggPlayAppUi::EFileName : iOggFiles->FillFileNames(*GetTextArray(), dummy, dummy, dummy, dummy); break;
    default: break;
    }
    if (aSelection.Length()>0) {
      TBuf<256> back;
      back.Num(6);
      back.Append(KColumnListSeparator);
      back.Append(_L(".."));
      back.Append(KColumnListSeparator);
      back.AppendNum(thePreviousView);
      GetTextArray()->InsertL(0,back);
    }
  }

  iApp->iViewBy= theNewView;

  UpdateListbox();
  if (theNewView!=COggPlayAppUi::ETop) SelectSong(1); else SelectSong(0);
  Invalidate();
}

void
COggPlayAppView::Update()
{
  UpdateAnalyzer();
  UpdateListbox();
  UpdateControls();
  UpdateSongPosition();
  UpdateClock(ETrue);
  UpdateVolume();
  UpdatePlaying();
}

void
COggPlayAppView::UpdateAnalyzer()
{
  if (iAnalyzer[iMode]) iAnalyzer[iMode]->Clear();
}

void
COggPlayAppView::UpdateListbox()
{
  if (iApp->iViewBy!=COggPlayAppUi::ETitle && 
      iApp->iViewBy!=COggPlayAppUi::EFileName) {
    if (iListBox[iMode]) iListBox[iMode]->Redraw();
    return;
  }

  CDesCArray* txt= GetTextArray();
  for (TInt i=0; i<txt->Count(); i++) {
    TPtrC sel;
    sel.Set((*txt)[i]);
    TBuf<512> buf(sel);

    // the icons being used here:
    // 0: title
    // 5: file
    // 6: back
    // 7: playing
    // 8: paused
    if (buf[0]!=L'6') {
      if (GetFileName(i)==iApp->iCurrentSong && iApp->iCurrentSong.Length()>0) {
	if (iApp->iOggPlayback->State()==TOggPlayback::EPaused) buf[0]='8'; else buf[0]='7'; 
      } 
      else {
	if (iApp->iViewBy==COggPlayAppUi::ETitle) buf[0]='0'; else buf[0]='5';
      }
    }

    txt->Delete(i);
    txt->InsertL(i,buf);
  }
  if (iListBox[iMode]) iListBox[iMode]->Redraw();
}

void 
COggPlayAppView::UpdateControls()
{
  TBool canPlay= 
    (iApp->iOggPlayback->State()==TOggPlayback::EPaused || 
     iApp->iOggPlayback->State()==TOggPlayback::EClosed ||
     iApp->iOggPlayback->State()==TOggPlayback::EFirstOpen);

  if (iPlayButton[iMode] && canPlay) 
    iPlayButton[iMode]->SetDimmed((GetSelectedIndex()<0 || GetItemType(GetSelectedIndex())==6 || 
				   iApp->iViewBy==COggPlayAppUi::EArtist || iApp->iViewBy==COggPlayAppUi::EAlbum || 
				   iApp->iViewBy==COggPlayAppUi::EGenre || iApp->iViewBy==COggPlayAppUi::ESubFolder ||
				   iApp->iViewBy==COggPlayAppUi::ETop) 
				  && !(iApp->iOggPlayback->State()==TOggPlayback::EPaused));

  if (iPlayButton[iMode]) iPlayButton[iMode]->MakeVisible(canPlay);
  if (iPauseButton[iMode]) iPauseButton[iMode]->MakeVisible(!canPlay);

  TBool canStop= (iApp->iOggPlayback->State()==TOggPlayback::EPaused || 
		  iApp->iOggPlayback->State()==TOggPlayback::EPlaying);
    
  if (iStopButton[iMode]) iStopButton[iMode]->SetDimmed( !canStop );
  if (iPosition[iMode]) iPosition[iMode]->SetDimmed( !canStop );
}

void
COggPlayAppView::UpdateSongPosition()
{
  if (!iApp->iForeground) return;

  // Update the song position displayed in the menubar (flip open) 
  // or in the TOggCanvas (flip closed):
  //------------------------------------

  TBuf<64> mbuf;
  TInt sec= iApp->iOggPlayback->Position().GetTInt()/1000;
  TInt min= sec/60;
  sec-= min*60;
  TInt sectot= iApp->iOggPlayback->Time().GetTInt()/1000;
  TInt mintot= sectot/60;
  sectot-= mintot*60;
  mbuf.Format(_L("%02d:%02d / %02d:%02d"), min, sec, mintot, sectot);

  if (IsFlipOpen()) {
    CEikMenuBar* mb = iEikonEnv->AppUiFactory()->MenuBar();
    if (!mb) return;
    // As we cannot officially get the TitleArray, we have to steal it (hack)
    CEikMenuBar::CTitleArray *ta;
    ta = *(CEikMenuBar::CTitleArray**)((char*)mb + sizeof(*mb) - 4*sizeof(void*));
    ta->At(1)->iData.iText.Copy(mbuf);
    mb->SetSize(mb->Size());
    mb->DrawNow();
  } else {
    if (iPlayed[iMode]) iPlayed[iMode]->SetText(mbuf);
  }

}

void
COggPlayAppView::UpdateClock(TBool forceUpdate)
{
  // This is called every second

  // Update the clock only every minute (or if forceUpdate is true):
  TTime now;
  now.HomeTime();
  TDateTime dtNow(now.DateTime());
  if (dtNow.Minute()==0 && dtNow.Second()==1 && iEye[iMode]) iEye[iMode]->Hide();
  if (dtNow.Second()!=0 && !forceUpdate) return;

  // Update the clock:
  _LIT(KDateString,"%-B%:0%J%:1%T%:3%+B");
  TBuf<256> buf;
  now.FormatL(buf,KDateString);
  if (iClock[iMode]) iClock[iMode]->SetText(buf);

  // It's not easy to find out what these code lines do (-;
  if (iEye[iMode]) {
    if (dtNow.Minute()==0 && dtNow.Second()==0) {
      iEye[iMode]->Blink();
    } else iEye[iMode]->Hide();
  }
}

void
COggPlayAppView::UpdateVolume()
{
  if (iVolume[iMode]) iVolume[iMode]->SetValue(iApp->iVolume);
}

void COggPlayAppView::UpdatePlaying()
{
  if (iTitle[iMode]) {
    if (iApp->iOggPlayback->Title().Length()==0) 
      iTitle[iMode]->SetText(iApp->iOggPlayback->FileName());
    else
      iTitle[iMode]->SetText(iApp->iOggPlayback->Title());
  }
  if (iAlbum[iMode]) iAlbum[iMode]->SetText(iApp->iOggPlayback->Album());
  if (iArtist[iMode]) iArtist[iMode]->SetText(iApp->iOggPlayback->Artist());

  if (iPlaying[iMode]) 
    iPlaying[iMode]->MakeVisible(iApp->iOggPlayback->State()==TOggPlayback::EPlaying);

  if (iPaused[iMode]) {
    if (iApp->iOggPlayback->State()==TOggPlayback::EPaused)
      iPaused[iMode]->Blink();
    else
      iPaused[iMode]->Hide();
  }
}

void 
COggPlayAppView::SetAlarm()
{
  iApp->iAlarmTriggered= 0;
  TTime now;
  now.HomeTime();
  TDateTime dtAlarm(iApp->iAlarmTime.DateTime());
  TDateTime dtNow(now.DateTime());
  dtAlarm.SetDay(dtNow.Day());
  dtAlarm.SetMonth(dtNow.Month());
  dtAlarm.SetYear(dtNow.Year());
  iApp->iAlarmTime= dtAlarm;
  if (iApp->iAlarmTime<now) iApp->iAlarmTime+= TTimeIntervalDays(1);
  if (iApp->iAlarmActive) {
    TBuf<256> buft;
    _LIT(KDateString,"%-B%:0%J%:1%T%:3%+B");
    iApp->iAlarmTime.FormatL(buft,KDateString);
    if (iAlarm[iMode]) iAlarm[iMode]->SetText(buft);
    if (iAlarmIcon[iMode]) iAlarmIcon[iMode]->Show();
  } else {
    ClearAlarm();
  }
}

void 
COggPlayAppView::ClearAlarm()
{
  if (iAlarm[iMode]) iAlarm[iMode]->SetText(_L(""));
  if (iAlarmIcon[iMode]) iAlarmIcon[iMode]->Hide();
}

TInt
COggPlayAppView::CountComponentControls() const
{
  return iControls->Count();
}

CCoeControl*
COggPlayAppView::ComponentControl(TInt aIndex) const
{
  return iControls->At(aIndex);
}

void
COggPlayAppView::HandleControlEventL(CCoeControl* aControl, TCoeEvent aEventType)
{
}

TKeyResponse
COggPlayAppView::OfferKeyEventL(const TKeyEvent& aKeyEvent, TEventCode aType)
{

  TInt code     = aKeyEvent.iCode;
  TInt modifiers= aKeyEvent.iModifiers & EAllStdModifiers;
  TInt iHotkey  = ((COggPlayAppUi *)iEikonEnv->EikAppUi())->iHotkey;

  if (iHotkey && aType == EEventKey &&
     code == keycodes[iHotkey-1].code && modifiers==keycodes[iHotkey-1].mask) {
    iApp->ActivateOggViewL();
    return EKeyWasConsumed;
  }

//FIXME: This needs to be done in a more elegant way. But for now...
#ifdef SERIES60  
  if(code == EStdKeyMenu) {
#else  
  if(code == EQuartzKeyConfirm) {
#endif
    if (IsFlipOpen()) {
      if (iApp->iOggPlayback->State()==TOggPlayback::EPlaying ||
	  iApp->iOggPlayback->State()==TOggPlayback::EPaused) iApp->HandleCommandL(EOggStop);
      else iApp->HandleCommandL(EOggPlay);
    } else {
      if (iApp->iOggPlayback->State()==TOggPlayback::EPlaying ||
	  iApp->iOggPlayback->State()==TOggPlayback::EPaused) iApp->HandleCommandL(EOggPauseResume);
      else iApp->HandleCommandL(EOggPlay);
    }
    return EKeyWasConsumed;
  }

  if (code>0) {
    if (aKeyEvent.iScanCode==EStdKeyDeviceD) { iApp->NextSong(); return EKeyWasConsumed; }           // jog dial up
    else if (aKeyEvent.iScanCode==EStdKeyDeviceE) { iApp->PreviousSong(); return EKeyWasConsumed; }  // jog dial down
    else if (aKeyEvent.iScanCode==167) { AppToForeground(EFalse); return EKeyWasConsumed; }          // back key <-
    else if (aKeyEvent.iScanCode==174) { iApp->HandleCommandL(EOggStop); return EKeyWasConsumed; }   // "C" key
    else if (aKeyEvent.iScanCode==173) { iApp->HandleCommandL(EOggShuffle); return EKeyWasConsumed; }// menu button
    else if (aKeyEvent.iScanCode==133) { // "*"
      if (iTitle[iMode]) iTitle[iMode]->ScrollNow();
      if (iAlbum[iMode]) iAlbum[iMode]->ScrollNow();
      if (iArtist[iMode]) iArtist[iMode]->ScrollNow();
      return EKeyWasConsumed;
    }
    else if (aKeyEvent.iScanCode==127) { // "#"
      ToggleRepeat();
      return EKeyWasConsumed;
    }
    // 48..57 == "0"..."9"
    // 172 jog dial press
    // 179    == OK
  }

  if (iApp->iOggPlayback->State()==TOggPlayback::EPlaying || !IsFlipOpen() ) {
    // volume
    if (code==0 && aType==EEventKeyDown) { 
//FIXME: key fix
#if !defined(SERIES60)
    if (aKeyEvent.iScanCode==EStdQuartzKeyTwoWayUp || aKeyEvent.iScanCode==EStdQuartzKeyTwoWayDown) {
		if (aKeyEvent.iScanCode==EStdQuartzKeyTwoWayUp && iApp->iVolume<100) iApp->iVolume+= 5; 
		if (aKeyEvent.iScanCode==EStdQuartzKeyTwoWayDown && iApp->iVolume>0) iApp->iVolume-= 5;
		iApp->iOggPlayback->SetVolume(iApp->iVolume);
		UpdateVolume();
		return EKeyWasConsumed;
      }
#endif
    }
  } else if (iListBox[iMode]) {
    // move current index in the list box
    if (code==0 && aType==EEventKeyDown) {
      TInt idx= iListBox[iMode]->CurrentItemIndex();
//FIXME
#if !defined(SERIES60)
      if (aKeyEvent.iScanCode==EStdQuartzKeyTwoWayUp && idx>0) {
	SelectSong(idx-1);
	return EKeyWasConsumed;
      }
      if (aKeyEvent.iScanCode==EStdQuartzKeyTwoWayDown) {
	SelectSong(idx+1);
	return EKeyWasConsumed;
      }
#endif
    }
  }

  return EKeyWasNotConsumed;

}

void
COggPlayAppView::OggControlEvent(COggControl* c, TInt aEventType, TInt aValue)
{
  if (c==iListBox[iMode]) {
    if (aEventType==1) UpdateControls(); // the current item index changed
    if (aEventType==2) iApp->HandleCommandL(EOggPlay); // an item was selected (by tipping on it)
  }
}

void 
COggPlayAppView::OggPointerEvent(COggControl* c, const TPointerEvent& p)
{
  if (c==iPosition[iMode]) {
    if (p.iType==TPointerEvent::EButton1Down || p.iType==TPointerEvent::EDrag) {
      iPosChanged= iPosition[iMode]->CurrentValue();
    }
    if (p.iType==TPointerEvent::EButton1Up) {
      iApp->iOggPlayback->SetPosition(TInt64(iPosChanged));
      iPosChanged= -1;
    }
  }

  if (c==iVolume[iMode]) {
    iApp->iVolume= iVolume[iMode]->CurrentValue();
    iApp->iOggPlayback->SetVolume(iApp->iVolume);
  }

  if (c==iPlayButton[iMode] && p.iType==TPointerEvent::EButton1Down) {
    if (iApp->iOggPlayback->State()==TOggPlayback::EPaused)
      iApp->HandleCommandL(EOggPauseResume);
    else
      iApp->HandleCommandL(EOggPlay);
  }
  if (c==iPauseButton[iMode] && p.iType==TPointerEvent::EButton1Down) iApp->HandleCommandL(EOggPauseResume);
  if (c==iStopButton[iMode] && p.iType==TPointerEvent::EButton1Down) iApp->HandleCommandL(EOggStop);

  if (c==iAnalyzer[iMode]) {
    iApp->iAnalyzerState[iMode]= iAnalyzer[iMode]->Style();
  }
}

