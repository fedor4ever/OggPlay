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

#include <OggPlay.rsg>

enum
{
  EScreenModeFlipOpen,
  EScreenModeFlipClosed
};


COggViewBase::COggViewBase(COggPlayAppView& aOggViewCtl)
: iOggViewCtl(aOggViewCtl)
{
}

COggViewBase::~COggViewBase()
{
}

void COggViewBase::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
#if defined(UIQ)
	// On UIQ, in FC mode, we need to manually re-configure the screen device
	TPixelsAndRotation sizeAndRotation;
	CEikonEnv::Static()->ScreenDevice()->GetDefaultScreenSizeAndRotation(sizeAndRotation);
	CEikonEnv::Static()->ScreenDevice()->SetScreenSizeAndRotation(sizeAndRotation);
#endif

	iOggViewCtl.Activated(this);
}

void COggViewBase::ViewDeactivated()
{
	iOggViewCtl.Deactivated(this);
}

void COggViewBase::ViewConstructL()
{
}


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

#if defined(UIQ)
TBool COggSplashView::ViewScreenModeCompatible(TInt /* aScreenMode */)
	{
	return ETrue;
	}
#endif

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

#if defined(UIQ)
TVwsViewIdAndMessage COggFOView::ViewScreenDeviceChangedL()
{
  return TVwsViewIdAndMessage(TVwsViewId(KOggPlayUid, KOggPlayUidFCView));
}

TBool COggFOView::ViewScreenModeCompatible(TInt aScreenMode)
{
  return (aScreenMode == EScreenModeFlipOpen);
}
#endif

void COggFOView::ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage)
{
  COggViewBase::ViewActivatedL(aPrevViewId, aCustomMessageId, aCustomMessage);  

#if defined(SERIES60)
	COggS60Utility::RemoveStatusPane(); 
#elif defined(UIQ)
	CEikonEnv::Static()->AppUiFactory()->MenuBar()->MakeVisible(ETrue);
#endif

  iOggViewCtl.ChangeLayout(0);
 
#if defined(SERIES60) || defined (SERIES80)
  COggPlayAppUi* appUi = (COggPlayAppUi*) CEikonEnv::Static()->AppUi();
  appUi->UpdateSoftkeys(ETrue);
#endif
}


#if defined(UIQ) || defined(SERIES60SUI)
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

#if defined(UIQ)
TVwsViewIdAndMessage COggFCView::ViewScreenDeviceChangedL()
{
  return TVwsViewIdAndMessage(TVwsViewId(KOggPlayUid, KOggPlayUidFOView));
}

TBool COggFCView::ViewScreenModeCompatible(TInt aScreenMode)
{
  return (aScreenMode == EScreenModeFlipClosed);
}
#endif

void COggFCView::ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage)
{
  COggViewBase::ViewActivatedL(aPrevViewId, aCustomMessageId, aCustomMessage);

#if defined(SERIES60)
	COggS60Utility::RemoveStatusPane(); 
#elif defined(UIQ)
	CEikonEnv::Static()->AppUiFactory()->MenuBar()->MakeVisible(EFalse);
#endif

  iOggViewCtl.ChangeLayout(1);

#if defined(SERIES60)
  COggPlayAppUi* appUi = (COggPlayAppUi*) CEikonEnv::Static()->AppUi();
  appUi->UpdateSoftkeys(ETrue);
#endif
}
#endif

#if defined(SERIES60)
void COggS60Utility::DisplayStatusPane(TInt aTitleID)
{
  // Enable statuspane
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_USUAL);
  if (aTitleID)
    {
    TBuf<32> buf;
    CAknTitlePane* titlePane = (CAknTitlePane *) statusPane->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
    CEikonEnv::Static()->ReadResource(buf, aTitleID);
    titlePane->SetTextL(buf);
    }
}

_LIT(KOggPlayTitle, "OggPlay");
void COggS60Utility::RemoveStatusPane()
{
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_EMPTY);

  // Restore title
  CAknTitlePane* titlePane = (CAknTitlePane *) statusPane->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
  titlePane->SetTextL(KOggPlayTitle);

  // Restore softkeys
  ((COggPlayAppUi*) appUi)->UpdateSoftkeys();
}


COggSettingsView::COggSettingsView(COggPlayAppView& aOggViewCtl)
: COggViewBase(aOggViewCtl)
{
}

COggSettingsView::~COggSettingsView()
{
	delete iContainer;
}

void COggSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* cba = CEikButtonGroupContainer::Current();
  cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
  cba->DrawNow();

  COggS60Utility::DisplayStatusPane(R_OGG_SETTINGS);

  if (!iContainer)
    {
	COggPlayAppUi* appUi = (COggPlayAppUi *) CEikonEnv::Static()->AppUi();
    iContainer = new(ELeave) COggSettingItemList(*appUi);
    iContainer->SetMopParent(appUi);

    iContainer->ConstructFromResourceL(R_OGGPLAY_DISPLAY_SETTING_ITEM_LIST);
    iContainer->LoadSettingsL();
	iContainer->ActivateL();

	CEikonEnv::Static()->EikAppUi()->AddToViewStackL(*this, iContainer);
    }
}

void COggSettingsView::ViewDeactivated()
{
  if (iContainer)
  {
	CEikonEnv::Static()->EikAppUi()->RemoveFromViewStack(*this, iContainer);

	delete iContainer;
	iContainer = NULL;
  }
}

TVwsViewId COggSettingsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, KOggPlayUidSettingsView);
}


#if defined(PLUGIN_SYSTEM)
void COggSettingsView::VolumeGainChangedL()
{
	if (iContainer)
		iContainer->VolumeGainChangedL();
}

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
  iCodecSelection = new(ELeave) COggplayCodecSelectionSettingItemList();
  iCodecSelection->SetMopParent(appUi);
  iCodecSelection->ConstructL(appUi->ClientRect());
  appUi->AddToViewStackL(*this, iCodecSelection);

  // Push new softkeys
  appUi->Cba()->AddCommandSetToStackL(R_USER_SELECT_BACK_CBA);

  // Transfer the softkey command ("select") to iCodecSelect
  appUi->Cba()->UpdateCommandObserverL(0,*iCodecSelection); 
  }

void COggPluginSettingsView::ViewDeactivated()
  {
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  appUi->Cba()->RemoveCommandObserver(0); 
  appUi->RemoveFromViewStack(*this, iCodecSelection);

  delete iCodecSelection;
  iCodecSelection = NULL;
  }
#else
COggPlaybackOptionsView::COggPlaybackOptionsView(COggPlayAppView& aOggViewCtl)
: COggViewBase(aOggViewCtl)
{
}

COggPlaybackOptionsView::~COggPlaybackOptionsView()
{
}

void COggPlaybackOptionsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* cba = CEikButtonGroupContainer::Current();
  cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
  cba->DrawNow();

  COggS60Utility::DisplayStatusPane(R_OGG_PLAYBACK);

  if (!iContainer)
    {
	COggPlayAppUi* appUi = (COggPlayAppUi *) CEikonEnv::Static()->AppUi();
    iContainer = new(ELeave) COggSettingItemList(*appUi);
    iContainer->SetMopParent(appUi);

    iContainer->ConstructFromResourceL(R_OGGPLAY_DISPLAY_PLAYBACK_OPTIONS_ITEM_LIST);
    iContainer->LoadSettingsL();
	iContainer->ActivateL();

	CEikonEnv::Static()->EikAppUi()->AddToViewStackL(*this, iContainer);
    }
}

void COggPlaybackOptionsView::ViewDeactivated()
{
  if (iContainer)
  {
	CEikonEnv::Static()->EikAppUi()->RemoveFromViewStack(*this, iContainer);

	delete iContainer;
	iContainer = NULL;
  }
}

TVwsViewId COggPlaybackOptionsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, KOggPlayUidPlaybackOptionsView);
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
#endif // PLUGIN_SYSTEM

  
COggUserHotkeysView::COggUserHotkeysView(COggPlayAppView& aOggViewCtl)
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

	COggPlayAppUi* appUi = (COggPlayAppUi*) CEikonEnv::Static()->AppUi();
	iUserHotkeysContainer = new(ELeave) COggUserHotkeysControl(appUi->iSettings);
	iUserHotkeysContainer->SetMopParent(appUi);
	iUserHotkeysContainer->ConstructL(appUi->ClientRect());
	appUi->AddToViewStackL(*this, iUserHotkeysContainer);

	// Push new softkeys
	appUi->Cba()->AddCommandSetToStackL(R_USER_CLEAR_BACK_CBA);
	}

void COggUserHotkeysView::ViewDeactivated()
  {
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  appUi->RemoveFromViewStack(*this, iUserHotkeysContainer);

  delete iUserHotkeysContainer;
  iUserHotkeysContainer = NULL;
  }


COggAlarmSettingsView::COggAlarmSettingsView(COggPlayAppView& aOggViewCtl)
: COggViewBase(aOggViewCtl)
{
}

COggAlarmSettingsView::~COggAlarmSettingsView()
{
}

void COggAlarmSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* cba=CEikButtonGroupContainer::Current();
  cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
  cba->DrawNow();

  COggS60Utility::DisplayStatusPane(R_OGG_ALARM_S60);

  if (!iContainer)
    {
	COggPlayAppUi* appUi = (COggPlayAppUi *) CEikonEnv::Static()->AppUi();
    iContainer = new(ELeave) COggSettingItemList(*appUi);
    iContainer->SetMopParent(appUi);

    iContainer->ConstructFromResourceL(R_OGGPLAY_ALARM_S60_SETTING_ITEM_LIST);
    iContainer->LoadSettingsL();
	iContainer->ActivateL();

	CEikonEnv::Static()->EikAppUi()->AddToViewStackL(*this, iContainer);
    }
}

void COggAlarmSettingsView::ViewDeactivated()
{
  if (iContainer)
  {
	CEikonEnv::Static()->EikAppUi()->RemoveFromViewStack(*this, iContainer);

	delete iContainer;
	iContainer = NULL;
  }
}

TVwsViewId COggAlarmSettingsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, KOggPlayUidAlarmSettingsView);
}

#endif // SERIES60
