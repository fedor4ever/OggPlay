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
  ResetControls();
  iTextArray= new CDesCArrayFlat(10);
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

  //iSkinFileDir.Copy(iApp->Application()->AppFullName());
  //iSkinFileDir.SetLength(iSkinFileDir.Length()-);

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
  
  //iCanvas[0]->ConstructL(iIconFileName, 19);
  //iCanvas[1]->ConstructL(iIconFileName, 18);
  //CreateDefaultSkin();

  iCanvas[0]->SetFocus(ETrue);

  // start the canvas refresh timer (20 ms):
  iTimer= CPeriodic::New(CActive::EPriorityStandard);
  TCallBack* iCallBack= new TCallBack(COggPlayAppView::CallBack,this);
  iTimer->Start(TTimeIntervalMicroSeconds32(1000000),TTimeIntervalMicroSeconds32(75000),*iCallBack);

  ActivateL();
}

void
COggPlayAppView::ReadSkin(const TFileName& aFileName)
{
  iCanvas[0]->ClearControls();
  iCanvas[1]->ClearControls();
  ResetControls();

  iIconFileName= aFileName.Left(aFileName.Length()-4);
  iIconFileName.Append(_L(".mbm"));
  iCanvas[0]->ConstructL(iIconFileName, 19);
  iCanvas[1]->ConstructL(iIconFileName, 18);

  TOggParser p(aFileName);
  if (p.iState!=TOggParser::ESuccess) {
    p.ReportError();
    User::Leave(KErrNone);
    return;
  }
  if (!p.ReadHeader()) {
    p.ReportError();
    User::Leave(KErrNone);
    return;
  }
  p.ReadToken();
  if (p.iToken!=_L("FlipOpen")) {
    p.iState= TOggParser::EFlipOpenExpected;
    p.ReportError();
    User::Leave(KErrNone);
    return;
  }
  p.ReadToken();
  if (p.iToken!=KBeginToken) {
    p.iState= TOggParser::EBeginExpected;
    p.ReportError();
    User::Leave(KErrNone);
    return;
  }
  ReadCanvas(0,p);
  if (p.iState!=TOggParser::ESuccess) {
    p.ReportError();
    User::Leave(KErrNone);
    return;
  }

  p.ReadToken();
  if (p.iToken==_L("FlipClosed")) {
    p.ReadToken();
    if (p.iToken!=KBeginToken) {
      p.iState= TOggParser::EBeginExpected;
      p.ReportError();
      User::Leave(KErrNone);
      return;
    }
    ReadCanvas(1,p);
    if (p.iState!=TOggParser::ESuccess) {
      p.ReportError();
      User::Leave(KErrNone);
      return;
    }
  }
  //User::InfoPrint(_L("New skin installed."));
}

void 
COggPlayAppView::ResetControls() {
  iClock[0]= 0; iClock[1]= 0;
  iAlarm[0]= 0; iAlarm[1]= 0;
  iAlarmIcon[0]=0; iAlarmIcon[1]= 0;
  iPlayed[0]= 0; iPlayed[1]= 0;
  iPaused[0]= 0; iPaused[1]= 0;
  iPlaying[0]= 0; iPlaying[1]= 0;
  iPlayButton[0]= 0; iPlayButton[1]=0;
  iPauseButton[0]= 0; iPauseButton[1]= 0;
  iStopButton[0]=0; iStopButton[1]= 0;
  iListBox[0]=0; iListBox[1]= 0;
  iTitle[0]=0; iTitle[1]= 0;
  iAlbum[0]=0; iAlbum[1]= 0;
  iArtist[0]=0; iArtist[1]= 0;
  iGenre[0]=0; iGenre[1]= 0;
  iTrackNumber[0]=0; iTrackNumber[1]= 0;
  iEye[0]= 0; iEye[1]= 0;
  iPosition[0]= 0; iPosition[1]= 0;
  iVolume[0]= 0; iVolume[1]= 0;
  iAnalyzer[0]= 0; iAnalyzer[1]= 0;
  iRepeat[0]= 0; iRepeat[1]= 0;
  iScrollBar[0]= 0; iScrollBar[1]= 0;
}

void
COggPlayAppView::ReadCanvas(TInt aCanvas, TOggParser& p)
{
  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());

  bool stop(EFalse);
  while (!stop) {
    stop= !p.ReadToken() || p.iToken==KEndToken;

    COggControl* c(0);

    if (p.iToken==_L("Clock")) { 
      c= new COggText();
      iClock[aCanvas]= (COggText*)c; 
      iClock[aCanvas]->SetFont(iFont); 
    }
    else if (p.iToken==_L("Alarm")) { 
      c= new COggText();
      iAlarm[aCanvas]= (COggText*)c;
      iAlarm[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Title")) { 
      c= new COggText();
      iTitle[aCanvas]= (COggText*)c;
      iTitle[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Album")) { 
      c= new COggText();
      iAlbum[aCanvas]= (COggText*)c;
      iAlbum[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Artist")) { 
      c= new COggText();
      iArtist[aCanvas]= (COggText*)c;
      iArtist[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Genre")) { 
      c= new COggText();
      iGenre[aCanvas]= (COggText*)c;
      iGenre[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("TrackNumber")) { 
      c= new COggText();
      iTrackNumber[aCanvas]= (COggText*)c;
      iTrackNumber[aCanvas]->SetFont(iFont);
    }
    else if (p.iToken==_L("Played")) { 
      c= new COggText();
      iPlayed[aCanvas]= (COggText*)c;
      iPlayed[aCanvas]->SetFont(iFont);
      iPlayed[aCanvas]->SetText(_L("00:00 / 00:00"));
    }
    else if (p.iToken==_L("StopButton")) { 
      c= new COggButton();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iStopButton[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PlayButton")) { 
      c= new COggButton();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iPlayButton[aCanvas]= (COggButton*)c;
    }
    else if (p.iToken==_L("PauseButton")) { 
      c= new COggButton();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iPauseButton[aCanvas]= (COggButton*)c;
      iPauseButton[aCanvas]->MakeVisible(EFalse);
    }
    else if (p.iToken==_L("PlayingIcon")) { 
      c= new COggIcon();
      c->SetBitmapFile(iIconFileName);
      iPlaying[aCanvas]= (COggIcon*)c;
    }
    else if (p.iToken==_L("PausedIcon")) { 
      c= new COggIcon();
      c->SetBitmapFile(iIconFileName);
      iPaused[aCanvas]= (COggIcon*)c;
      iPaused[aCanvas]->Hide();
    }
    else if (p.iToken==_L("AlarmIcon")) { 
      c= new COggIcon();
      c->SetBitmapFile(iIconFileName);
      iAlarmIcon[aCanvas]= (COggIcon*)c;
      iAlarmIcon[aCanvas]->Hide();
    }
    else if (p.iToken==_L("RepeatIcon")) { 
      c= new COggIcon();
      c->SetBitmapFile(iIconFileName);
      iRepeat[aCanvas]= (COggIcon*)c;
      iRepeat[aCanvas]->MakeVisible(iApp->iRepeat);
    }
    else if (p.iToken==_L("Analyzer")) {
      c= new COggAnalyzer();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iAnalyzer[aCanvas]= (COggAnalyzer*)c;
    }
    else if (p.iToken==_L("Position")) {
      c= new COggSlider();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iPosition[aCanvas]= (COggSlider*)c;
    }
    else if (p.iToken==_L("Volume")) {
      c= new COggSlider();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iVolume[aCanvas]= (COggSlider*)c;
    }
    else if (p.iToken==_L("ScrollBar")) {
      c= new COggScrollBar();
      c->SetBitmapFile(iIconFileName);
      iScrollBar[aCanvas]= (COggScrollBar*)c;
      if (iListBox[aCanvas]) {
	iListBox[aCanvas]->SetVertScrollBar(iScrollBar[aCanvas]);
	iScrollBar[aCanvas]->SetAssociatedControl(iListBox[aCanvas]);
      }
    }
    else if (p.iToken==_L("ListBox")) {
      c= new COggListBox();
      c->SetBitmapFile(iIconFileName);
      c->SetObserver(this);
      iListBox[aCanvas]= (COggListBox*)c;
      SetupListBox(iListBox[aCanvas]);
      if (iScrollBar[aCanvas]) {
	iScrollBar[aCanvas]->SetAssociatedControl(iListBox[aCanvas]);
	iListBox[aCanvas]->SetVertScrollBar(iScrollBar[aCanvas]);
      }
    }

    if (c) {
      //c->SetObserver(this);
      c->Read(p);
      iCanvas[aCanvas]->AddControl(c);
    }
  }

  p.Debug(_L("Canvas read."));

}

void 
COggPlayAppView::CreateDefaultSkin()
{

  // Create and set up the flip-closed mode canvas:
  //-----------------------------------------------

  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());
  //TFontSpec fs(_L("Courier"),14);
  //fs.iFontStyle.SetStrokeWeight(EStrokeWeightBold);
  //iCoeEnv->ScreenDevice()->GetNearestFontInPixels(iBoldFont,fs);
  
  // setup text lines:

  iTitle[1] = new COggText(); 
  iTitle[1]->SetPosition(6,  55, Size().iWidth- 12, 16); 
  iCanvas[1]->AddControl(iTitle[1]); 
  iTitle[1]->SetFont(iFont);

  iAlbum[1] = new COggText();
  iAlbum[1]->SetPosition(6,  75, Size().iWidth- 12, 16); 
  iCanvas[1]->AddControl(iAlbum[1]); 
  iAlbum[1]->SetFont(iFont);

  iArtist[1]= new COggText();
  iArtist[1]->SetPosition(6,  95, Size().iWidth- 12, 16); 
  iCanvas[1]->AddControl(iArtist[1]); 
  iArtist[1]->SetFont(iFont);

  iClock[1] = new COggText();
  iClock[1]->SetPosition(134,  15, 59, 14); 
  iCanvas[1]->AddControl(iClock[1]); 
  iClock[1]->SetFont(iFont);

  iAlarm[1] = new COggText();
  iAlarm[1]->SetPosition(134,  30, 59, 14); 
  iCanvas[1]->AddControl(iAlarm[1]); 
  iAlarm[1]->SetFont(iFont);

  iPlayed[1]= new COggText();
  iPlayed[1]->SetPosition(60,  120, Size().iWidth-120, 16); 
  iPlayed[1]->SetFont(iFont);
  iPlayed[1]->SetText(_L("00:00 / 00:00"));
  iCanvas[1]->AddControl(iPlayed[1]);

  // setup icons:

  iPlaying[1]= new COggIcon();
  iPlaying[1]->SetPosition(38, 119, 20, 16); 
  iPlaying[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,34,35));
  iCanvas[1]->AddControl(iPlaying[1]);

  iPaused[1]= new COggIcon();
  iPaused[1]->SetPosition(38, 119, 20, 16); 
  iPaused[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,16,17));
  iPaused[1]->Hide(); 
  iCanvas[1]->AddControl(iPaused[1]);

  iAlarmIcon[1]= new COggIcon();
  iAlarmIcon[1]->SetPosition(185, 30, 20, 16);
  iAlarmIcon[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,39,40));
  iAlarmIcon[1]->Hide();
  iCanvas[1]->AddControl(iAlarmIcon[1]);

  // setup sliders:

  iVolume[1]= new COggSlider();
  iVolume[1]->SetPosition(74, 11, 60, 16);
  iVolume[1]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,20,21));
  iVolume[1]->SetObserver(this);
  iCanvas[1]->AddControl(iVolume[1]);

  // this is the eye of the fish (it blinks to the full hour):

  iEye[1]= new COggIcon();
  iEye[1]->SetPosition(41, 16, 16, 16);
  iEye[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,22,22));
  iEye[1]->SetBlinkFrequency(2);
  iEye[1]->Hide();
  iCanvas[1]->AddControl(iEye[1]);

  iRepeat[1]= new COggIcon();
  iRepeat[1]->SetPosition(145, 117, 20, 16);
  iRepeat[1]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,23,24));
  iRepeat[1]->MakeVisible(iApp->iRepeat);
  iCanvas[1]->AddControl(iRepeat[1]);

  // These controls appear just in flip-open mode for the default skin

  const TInt y(Size().iHeight- 27);

  iAnalyzer[1]= 0;
  iPosition[1]= 0;
  iPlayButton[1]= 0;
  iPauseButton[1]= 0;
  iStopButton[1]= 0;
  iScrollBar[1]= 0;
  iListBox[1]= 0;

  // Create and set up the flip-open mode canvas:
  //---------------------------------------------

  iTitle[0] = new COggText();
  iTitle[0]->SetPosition(6,  20, Size().iWidth- 12, 16); 
  iCanvas[0]->AddControl(iTitle[0]); 
  iTitle[0]->SetFont(iFont);

  iAlbum[0] = new COggText();
  iAlbum[0]->SetPosition(6,  36, Size().iWidth- 12, 16); 
  iCanvas[0]->AddControl(iAlbum[0]); 
  iAlbum[0]->SetFont(iFont);

  iArtist[0]= new COggText();
  iArtist[0]->SetPosition(6,  52, Size().iWidth- 12, 16); 
  iCanvas[0]->AddControl(iArtist[0]); 
  iArtist[0]->SetFont(iFont);

  iClock[0] = new COggText();
  iClock[0]->SetPosition(73, 3, 59, 14); 
  iCanvas[0]->AddControl(iClock[0]); 
  iClock[0]->SetFont(iFont);

  iAlarm[0] = new COggText();
  iAlarm[0]->SetPosition(134, 3, 59, 14); 
  iCanvas[0]->AddControl(iAlarm[0]); 
  iAlarm[0]->SetFont(iFont);

  iPlayed[0]= 0;
  iPlaying[0]= 0;
  iPaused[0]= 0;

  iAlarmIcon[0]= new COggIcon();
  iAlarmIcon[0]->SetPosition(185, 3, 20, 16);
  iAlarmIcon[0]->SetIcon(iEikonEnv->CreateIconL(iIconFileName,39,40));
  iAlarmIcon[0]->Hide();
  iCanvas[0]->AddControl(iAlarmIcon[0]);

  iVolume[0]= new COggSlider();
  iVolume[0]->SetPosition(36, 79, 60, 16);
  iVolume[0]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,20,21));
  iVolume[0]->SetObserver(this);
  iCanvas[0]->AddControl(iVolume[0]);

  // and some of them do not appear at all:

  iEye[0]= 0; 
  iRepeat[0]= 0;

  // others appear only in flip-open mode:  
  
  iAnalyzer[0]= new COggAnalyzer();
  iAnalyzer[0]->SetPosition(138, 70, 70, 30);
  iAnalyzer[0]->SetBarIcon(iEikonEnv->CreateIconL(iIconFileName,29,30));
  iAnalyzer[0]->SetObserver(this);
  iCanvas[0]->AddControl(iAnalyzer[0]);

  iPosition[0]= new COggSlider();
  iPosition[0]->SetPosition(85, y+1, 110, 16);
  iPosition[0]->SetStyle(2);
  iPosition[0]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,25,26));
  iPosition[0]->SetObserver(this);
  iCanvas[0]->AddControl(iPosition[0]);

  iStopButton[0]= new COggButton();
  iStopButton[0]->SetPosition(11, y-1, 20, 20);
  iStopButton[0]->SetNormalIcon(iEikonEnv->CreateIconL(iIconFileName, 31, 32));
  iStopButton[0]->SetDimmedIcon(iEikonEnv->CreateIconL(iIconFileName, 33, 33));
  iStopButton[0]->SetObserver(this);
  iCanvas[0]->AddControl(iStopButton[0]);

  iPlayButton[0]= new COggButton();
  iPlayButton[0]->SetPosition(46, y, 20, 20);
  iPlayButton[0]->SetNormalIcon(iEikonEnv->CreateIconL(iIconFileName, 34, 35));
  iPlayButton[0]->SetDimmedIcon(iEikonEnv->CreateIconL(iIconFileName, 36, 36));
  iPlayButton[0]->SetObserver(this);
  iCanvas[0]->AddControl(iPlayButton[0]);

  iPauseButton[0]= new COggButton();
  iPauseButton[0]->SetPosition(46, y, 20, 20);
  iPauseButton[0]->SetNormalIcon(iEikonEnv->CreateIconL(iIconFileName, 37, 38));
  iPauseButton[0]->SetObserver(this);
  iPauseButton[0]->MakeVisible(EFalse);
  iCanvas[0]->AddControl(iPauseButton[0]);

  iPauseButton[0]->SetObserver(this);

  iScrollBar[0]= new COggScrollBar();
  iScrollBar[0]->SetPosition(188, 113, 16, 96); 
  iScrollBar[0]->SetKnobIcon(iEikonEnv->CreateIconL(iIconFileName,27,28));
  iScrollBar[0]->SetStyle(1);

  iListBox[0]= new COggListBox();
  iListBox[0]->SetPosition(6, 116, 182, 96);
  iListBox[0]->SetVertScrollBar(iScrollBar[0]);
  SetupListBox(iListBox[0]);
  iScrollBar[0]->SetAssociatedControl(iListBox[0]);

  iCanvas[0]->AddControl(iListBox[0]);
  iCanvas[0]->AddControl(iScrollBar[0]);
}

void COggPlayAppView::SetupListBox(COggListBox* aListBox)
{
  // set up the layout of the listbox:
  CColumnListBoxData* cd= aListBox->GetColumnListBoxData(); 
  TInt w = Size().iWidth;
  cd->SetColumnWidthPixelL(0,18);
  cd->SetGraphicsColumnL(0, ETrue);
  cd->SetColumnAlignmentL(0, CGraphicsContext::ECenter);
  cd->SetColumnWidthPixelL(1, w-18);
  cd->SetColumnAlignmentL(1, CGraphicsContext::ELeft);
  cd->SetColumnWidthPixelL(2, w);
  cd->SetColumnAlignmentL(2, CGraphicsContext::ELeft);

  CFont* iFont= const_cast<CFont*>(iCoeEnv->NormalFont());
  aListBox->SetFont(iFont);
  aListBox->SetText(GetTextArray());
  aListBox->SetObserver(this);
  CGulIcon* titleIcon = iEikonEnv->CreateIconL(iIconFileName,0,1);
  CGulIcon* albumIcon = iEikonEnv->CreateIconL(iIconFileName,2,3);
  CGulIcon* artistIcon= iEikonEnv->CreateIconL(iIconFileName,4,5);
  CGulIcon* genreIcon = iEikonEnv->CreateIconL(iIconFileName,6,7);
  CGulIcon* folderIcon= iEikonEnv->CreateIconL(iIconFileName,8,9);
  CGulIcon* fileIcon  = iEikonEnv->CreateIconL(iIconFileName,10,11);
  CGulIcon* backIcon  = iEikonEnv->CreateIconL(iIconFileName,12,13);
  CGulIcon* playIcon  = iEikonEnv->CreateIconL(iIconFileName,34,35);
  CGulIcon* pausedIcon= iEikonEnv->CreateIconL(iIconFileName,16,17);
  //CColumnListBoxData* cd= aListBox->GetColumnListBoxData();
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
}

void 
COggPlayAppView::GotoFlipClosed()
{
  // modify controls for flip-closed mode display
  if (iListBox[1]) iListBox[1]->SetCurrentItemIndex(iSelected);
}

void
COggPlayAppView::GotoFlipOpen()
{
  // modify controls for flip-open mode display
  if (iListBox[0]) iListBox[0]->SetCurrentItemIndex(iSelected);
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
  return GetTextArray()->Count();
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
  iSelected= idx;
  iCurrentSong= GetFileName(idx);
  if (iListBox[iMode]) {
    iListBox[iMode]->SetCurrentItemIndex(idx);
    UpdateControls();
  }
}

TInt
COggPlayAppView::GetSelectedIndex()
{
  return iSelected;
  /*
  if (iListBox[iMode]) return iListBox[iMode]->CurrentItemIndex();
  TInt otherMode= (iMode+1)%2;
  if (iListBox[otherMode]) return iListBox[otherMode]->CurrentItemIndex();
  return -1;
  */
}

CDesCArray*
COggPlayAppView::GetTextArray()
{
  return iTextArray;
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
  for (TInt i=offset; i<arr->Count(); i++) {
    float rnd= (float)rand()/RAND_MAX;
    TInt pick= (int)(rnd*(arr->Count()-i));
    pick += i;
    if (pick>=arr->Count()) pick= arr->Count()-1;
    TBuf<512> buf((*arr)[pick]);
    arr->Delete(pick);
    arr->InsertL(offset,buf);
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

  SetTime(iApp->iOggPlayback->Time());
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

  // Update the alarm time:
  if (iApp->iAlarmActive) {
    TBuf<256> buft;
    _LIT(KDateString,"%-B%:0%J%:1%T%:3%+B");
    iApp->iAlarmTime.FormatL(buft,KDateString);
    if (iAlarm[iMode]) iAlarm[iMode]->SetText(buft);
    if (iAlarmIcon[iMode]) iAlarmIcon[iMode]->Show();
  }

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
  if (iGenre[iMode]) iGenre[iMode]->SetText(iApp->iOggPlayback->Genre());
  if (iTrackNumber[iMode]) iTrackNumber[iMode]->SetText(iApp->iOggPlayback->TrackNumber());

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
      if (iGenre[iMode]) iGenre[iMode]->ScrollNow();
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
    if (aEventType==1) {
      iSelected= iListBox[iMode]->CurrentItemIndex();
      UpdateControls(); // the current item index changed
    }
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

