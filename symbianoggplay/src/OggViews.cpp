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
#include <eikappui.h>
#include <eikmenub.h>
#if defined(SERIES60)
#include <aknlists.h>
#include <akntitle.h>
#endif

#include "OggLog.h"
#include "OggViews.h"
#include "OggPlayAppView.h"
#if defined(SERIES60)
#include "OggSettingsContainer.h"
#include "OggUserHotkeys.h"
#include "OggPluginSelectionDialogS60.h"
#endif

#if defined(SERIES80)
#include <eikenv.h>
#endif

enum
{
  EScreenModeFlipOpen,
  EScreenModeFlipClosed
};


// Class COggViewBase
COggViewBase::COggViewBase(COggPlayAppView& aOggViewCtl)
  : iOggViewCtl(aOggViewCtl)
{
}

COggViewBase::~COggViewBase()
{
}

void COggViewBase::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
				  const TDesC8& /*aCustomMessage*/)
{
#if defined(UIQ)
	// On UIQ, in FC mode, we need to manually re-configure the screen device
	TPixelsAndRotation sizeAndRotation;
	CEikonEnv::Static()->ScreenDevice()->GetDefaultScreenSizeAndRotation(sizeAndRotation);
	CEikonEnv::Static()->ScreenDevice()->SetScreenSizeAndRotation(sizeAndRotation);
#endif

	iOggViewCtl.Activated();
}

void COggViewBase::ViewDeactivated()
{
	iOggViewCtl.Deactivated();
}

void COggViewBase::ViewConstructL()
{
}


// Class COggSplashView
COggSplashView::COggSplashView(const TUid& aViewId)
: iViewId(aViewId)
{
}

void COggSplashView::ConstructL()
{
    iSplashContainer = new(ELeave) CSplashContainer();
	iSplashContainer->ConstructL();
}

COggSplashView::~COggSplashView()
{
	delete iSplashContainer;
}

TVwsViewId COggSplashView::ViewId() const
{
	return TVwsViewId(KOggPlayUid, iViewId);
}

TBool COggSplashView::ViewScreenModeCompatible(TInt /* aScreenMode */)
{
  return ETrue;
}

void COggSplashView::ViewActivatedL(const TVwsViewId& /* aPrevViewId */, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
#if defined(UIQ)
	// On UIQ, in FC mode, the splash view is activated by the FC app launcher
	// If the application is starting we display the splash screen, otherwise we do nothing
	// (In the latter case the main FC view will be activated by COggPlayAppUi::HandleForegroundEvent())
	COggPlayAppUi* appUi = (COggPlayAppUi*) CEikonEnv::Static()->AppUi();
	if (appUi->iStartUpState != COggPlayAppUi::EActivateSplashView)
		return;

	// On UIQ, in FC mode, we need to manually re-configure the screen device
	TPixelsAndRotation sizeAndRotation;
	CEikonEnv::Static()->ScreenDevice()->GetDefaultScreenSizeAndRotation(sizeAndRotation);
	CEikonEnv::Static()->ScreenDevice()->SetScreenSizeAndRotation(sizeAndRotation);
#endif

	iSplashContainer->ShowSplashL();
}

void COggSplashView::ViewDeactivated()
{
	delete iSplashContainer;
    iSplashContainer = NULL;
}

// Class COggFOView
COggFOView::COggFOView(COggPlayAppView& aOggViewCtl)
  : COggViewBase(aOggViewCtl)
{
}

COggFOView::~COggFOView()
{
}

TVwsViewId COggFOView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, KOggPlayUidFOView);
}

TVwsViewIdAndMessage COggFOView::ViewScreenDeviceChangedL()
{
  return TVwsViewIdAndMessage(TVwsViewId(KOggPlayUid, KOggPlayUidFCView));
}

TBool COggFOView::ViewScreenModeCompatible(TInt aScreenMode)
{
  return (aScreenMode == EScreenModeFlipOpen);
}

void COggFOView::ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage)
{
  COggViewBase::ViewActivatedL(aPrevViewId, aCustomMessageId, aCustomMessage);
  CEikonEnv::Static()->AppUiFactory()->MenuBar()->MakeVisible(ETrue);

#if defined(SERIES60)
  CEikonEnv::Static()->AppUiFactory()->StatusPane()->MakeVisible(EFalse);
#endif

  iOggViewCtl.ChangeLayout(EFalse);
 
#if defined(SERIES60) || defined (SERIES80)
  COggPlayAppUi* appUi = (COggPlayAppUi*) CEikonEnv::Static()->AppUi();
  appUi->UpdateSoftkeys(ETrue);
#endif
}


// Class COggFCView
COggFCView::COggFCView(COggPlayAppView& aOggViewCtl)
  : COggViewBase(aOggViewCtl)
{
}

COggFCView::~COggFCView()
{
}

TVwsViewId COggFCView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, KOggPlayUidFCView);
}

TVwsViewIdAndMessage COggFCView::ViewScreenDeviceChangedL()
{
  return TVwsViewIdAndMessage(TVwsViewId(KOggPlayUid, KOggPlayUidFOView));
}

TBool COggFCView::ViewScreenModeCompatible(TInt aScreenMode)
{
  return (aScreenMode == EScreenModeFlipClosed);
}

void COggFCView::ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage)
{
  COggViewBase::ViewActivatedL(aPrevViewId, aCustomMessageId, aCustomMessage);
  CEikonEnv::Static()->AppUiFactory()->MenuBar()->MakeVisible(EFalse);
  iOggViewCtl.ChangeLayout(ETrue);
}

#if defined(SERIES60)
void COggS60Utility::DisplayStatusPane(TInt aTitleID)
{
  // Enable statuspane
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_USUAL);
  if( aTitleID )
    {
    CAknTitlePane* titlePane = (CAknTitlePane*)statusPane->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
    TBuf<32> buf;
    CEikonEnv::Static()->ReadResource(buf, aTitleID);
    titlePane->SetTextL(buf);
    }

  statusPane->MakeVisible(ETrue);
}

void COggS60Utility::RemoveStatusPane()
{
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->MakeVisible(EFalse);

  // Restore softkeys
  ((COggPlayAppUi*) appUi)->UpdateSoftkeys();
}


// Class COggSettingsView
COggSettingsView::COggSettingsView(COggPlayAppView& aOggViewCtl,TUid aId )
: COggViewBase(aOggViewCtl)
{
    iUid = aId;
}

COggSettingsView::~COggSettingsView()
{
}

void COggSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* cba = CEikButtonGroupContainer::Current();
  cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
  cba->DrawNow();

  CEikonEnv::Static()->AppUiFactory()->StatusPane()->MakeVisible(ETrue);
  COggS60Utility::DisplayStatusPane(R_OGG_SETTINGS);

  if (!iContainer)
    {
    iContainer = new (ELeave) COggSettingsContainer;
    iContainer->ConstructL( ((CEikAppUi*)CEikonEnv::Static()->AppUi())->ClientRect(), iUid);
    ((CCoeAppUi*)CEikonEnv::Static()->AppUi())->AddToStackL( *this, iContainer );
    }
}

void COggSettingsView::ViewDeactivated()
{
  if (iContainer)
  {
	((CCoeAppUi*)CEikonEnv::Static()->AppUi())->RemoveFromViewStack(*this, iContainer);

	delete iContainer;
	iContainer = NULL;
  }

  COggS60Utility::RemoveStatusPane(); 
}

#if !defined(MULTI_THREAD_PLAYBACK)
void COggSettingsView::VolumeGainChangedL()
{
	if (iContainer)
		iContainer->VolumeGainChangedL();
}
#endif

TVwsViewId COggSettingsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, iUid);
}


#ifdef PLUGIN_SYSTEM
// Class COggPluginSettingsView
COggPluginSettingsView::COggPluginSettingsView(COggPlayAppView& aOggViewCtl )
: COggViewBase(aOggViewCtl), iOggViewCtl(aOggViewCtl)
	{
	}

COggPluginSettingsView::~COggPluginSettingsView()
	{
    delete iCodecSelection;
	}

TVwsViewId COggPluginSettingsView::ViewId() const
	{
	return TVwsViewId(KOggPlayUid, KOggPlayUidCodecSelectionView);
	}

void COggPluginSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
  {
  COggS60Utility::DisplayStatusPane(R_OGG_CODEC_SELECTION);

  COggPlayAppUi* appUi = (COggPlayAppUi*)CEikonEnv::Static()->AppUi();
  iCodecSelection = new (ELeave) COggplayCodecSelectionSettingItemList();
  iCodecSelection->SetMopParent(appUi);
  iCodecSelection->ConstructL( appUi->ClientRect() );
  appUi->AddToStackL(*this, iCodecSelection);

  // Push new softkeys
  appUi->Cba()->AddCommandSetToStackL(R_USER_SELECT_BACK_CBA);

  // Transfert the softkey command ("select") to iCodecSelect
  appUi->Cba()->UpdateCommandObserverL(0,*iCodecSelection); 
  }

void COggPluginSettingsView::ViewDeactivated()
  {
  COggS60Utility::RemoveStatusPane();
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  appUi->Cba()->RemoveCommandObserver(0); 
  appUi->RemoveFromStack(iCodecSelection);
  delete iCodecSelection;
  iCodecSelection = NULL;
  }
#endif /*PLUGIN_SYSTEM*/

  
// Class COggUserHotkeysView
COggUserHotkeysView::COggUserHotkeysView(COggPlayAppView& aOggViewCtl )
: COggViewBase(aOggViewCtl), iOggViewCtl(aOggViewCtl)
	{
	}

COggUserHotkeysView::~COggUserHotkeysView()
	{
    delete iUserHotkeysContainer;
	}


TVwsViewId COggUserHotkeysView::ViewId() const
	{
	return TVwsViewId(KOggPlayUid, KOggPlayUidUserView);
	}


void COggUserHotkeysView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
	COggS60Utility::DisplayStatusPane(R_OGG_USER_HOTKEYS);

	COggPlayAppUi* appUi = (COggPlayAppUi*)CEikonEnv::Static()->AppUi();
	iUserHotkeysContainer = new (ELeave) COggUserHotkeysControl( appUi->iSettings );
	iUserHotkeysContainer->SetMopParent(appUi);
	iUserHotkeysContainer->ConstructL( appUi->ClientRect() );
	appUi->AddToStackL(*this, iUserHotkeysContainer);

	// Push new softkeys
	appUi->Cba()->AddCommandSetToStackL(R_USER_CLEAR_BACK_CBA);
	}

void COggUserHotkeysView::ViewDeactivated()
  {
  COggS60Utility::RemoveStatusPane();

  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  appUi->RemoveFromStack(iUserHotkeysContainer);
  delete iUserHotkeysContainer;
  iUserHotkeysContainer = NULL;
  }


#if defined(MULTI_THREAD_PLAYBACK)
COggPlaybackOptionsView::COggPlaybackOptionsView(COggPlayAppView& aOggViewCtl, TUid aId)
: COggViewBase(aOggViewCtl)
{
    iUid = aId;
}

COggPlaybackOptionsView::~COggPlaybackOptionsView()
{
}

void COggPlaybackOptionsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* cba = CEikButtonGroupContainer::Current();
  cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
  cba->DrawNow();

  CEikonEnv::Static()->AppUiFactory()->StatusPane()->MakeVisible(ETrue);
  COggS60Utility::DisplayStatusPane(R_OGG_PLAYBACK);

  if (!iContainer)
    {
    iContainer = new (ELeave) COggSettingsContainer;
    iContainer->ConstructL(((CEikAppUi*)CEikonEnv::Static()->AppUi())->ClientRect(), iUid);
    ((CCoeAppUi*)CEikonEnv::Static()->AppUi())->AddToStackL(*this, iContainer);
    }
}

void COggPlaybackOptionsView::ViewDeactivated()
{
  if (iContainer)
  {
	((CCoeAppUi*) CEikonEnv::Static()->AppUi())->RemoveFromViewStack(*this, iContainer);

	delete iContainer;
	iContainer = NULL;
  }

  COggS60Utility::RemoveStatusPane(); 
}

TVwsViewId COggPlaybackOptionsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, iUid);
}

void COggPlaybackOptionsView::VolumeGainChangedL()
{
	if (iContainer)
		iContainer->VolumeGainChangedL();
}

void COggPlaybackOptionsView::BufferingModeChangedL()
{
	if (iContainer)
		iContainer->BufferingModeChangedL();
}
#endif /* MULTI_THREAD_PLAYBACK */

COggAlarmSettingsView::COggAlarmSettingsView(COggPlayAppView& aOggViewCtl, TUid aId)
: COggViewBase(aOggViewCtl)
{
    iUid = aId;
}

COggAlarmSettingsView::~COggAlarmSettingsView()
{
}

void COggAlarmSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* cba=CEikButtonGroupContainer::Current();
  cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
  cba->DrawNow();

  CEikonEnv::Static()->AppUiFactory()->StatusPane()->MakeVisible(ETrue);
  COggS60Utility::DisplayStatusPane(R_OGG_ALARM_S60);

  if (!iContainer)
    {
    iContainer = new (ELeave) COggSettingsContainer;
    iContainer->ConstructL(((CEikAppUi*)CEikonEnv::Static()->AppUi())->ClientRect(), iUid);
    ((CCoeAppUi*)CEikonEnv::Static()->AppUi())->AddToStackL(*this, iContainer);
    }
}

void COggAlarmSettingsView::ViewDeactivated()
{
  if (iContainer)
  {
	((CCoeAppUi*) CEikonEnv::Static()->AppUi())->RemoveFromViewStack(*this, iContainer);

	delete iContainer;
	iContainer = NULL;
  }

  COggS60Utility::RemoveStatusPane(); 
}

TVwsViewId COggAlarmSettingsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, iUid);
}

#endif /* SERIES60 */
