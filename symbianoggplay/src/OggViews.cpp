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
  EScreenModeFlipOpen = 0,
  EScreenModeFlipClosed
};


////////////////////////////////////////////////////////////////
//
// class COggViewBase
//
////////////////////////////////////////////////////////////////
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
  TPixelsAndRotation sizeAndRotation;
  CEikonEnv::Static()->ScreenDevice()->GetDefaultScreenSizeAndRotation(sizeAndRotation);
  CEikonEnv::Static()->ScreenDevice()->SetScreenSizeAndRotation(sizeAndRotation);
  iOggViewCtl.Activated();
}

void COggViewBase::ViewDeactivated()
{
  iOggViewCtl.Deactivated();
}

void COggViewBase::ViewConstructL()
{
}


////////////////////////////////////////////////////////////////
//
// class COggFOView
//
////////////////////////////////////////////////////////////////
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
  
#if defined(SERIES60) || defined (SERIES80)
  ((COggPlayAppUi*) CEikonEnv::Static()->AppUi())->UpdateSoftkeys(ETrue); // we KNOW we're the right view!
#endif
  iOggViewCtl.ChangeLayout(EFalse);
}


////////////////////////////////////////////////////////////////
//
// class COggFCView
//
////////////////////////////////////////////////////////////////
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
////////////////////////////////////////////////////////////////
//
// class COggS60Utility
//
////////////////////////////////////////////////////////////////
void COggS60Utility::DisplayStatusPane( TInt aTitleID )
{
  // Enable statuspane
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->SwitchLayoutL(R_AVKON_STATUS_PANE_LAYOUT_USUAL);
  if( aTitleID )
    {
    CAknTitlePane* titlePane=(CAknTitlePane*)statusPane->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
    TBuf<32> buf;
    CEikonEnv::Static()->ReadResource(buf,aTitleID);
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


////////////////////////////////////////////////////////////////
//
// class COggSettingsView
//
////////////////////////////////////////////////////////////////
COggSettingsView::COggSettingsView(COggPlayAppView& aOggViewCtl,TUid aId )
: COggViewBase(aOggViewCtl)
{
    iUid = aId;
}

COggSettingsView::~COggSettingsView()
{
}

void COggSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
				  const TDesC8& /*aCustomMessage*/)
{
  CEikButtonGroupContainer* Cba=CEikButtonGroupContainer::Current();
  if(Cba) {
    Cba->AddCommandSetToStackL(R_USER_EMPTY_BACK_CBA);
    Cba->DrawNow();
  }
  CEikonEnv::Static()->AppUiFactory()->StatusPane()->MakeVisible(ETrue);

  COggS60Utility::DisplayStatusPane(R_OGG_SETTINGS);

  if (!iContainer)
    {
    iContainer = new (ELeave) COggSettingsContainer;
    iContainer->ConstructL( ((CEikAppUi*)CEikonEnv::Static()->AppUi())->ClientRect(),
       iUid);
    ((CCoeAppUi*)CEikonEnv::Static()->AppUi())->AddToStackL( *this, iContainer );
    }
}

void COggSettingsView::ViewDeactivated()
{
  if ( iContainer )
  {
    ((CCoeAppUi*)CEikonEnv::Static()->AppUi())->RemoveFromViewStack( *this, iContainer );
     delete iContainer;
     iContainer = NULL;
  }

  COggS60Utility::RemoveStatusPane(); 
}

void COggSettingsView::VolumeGainChangedL()
{
	if (iContainer)
		iContainer->VolumeGainChangedL();
}

TVwsViewId COggSettingsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, iUid);
}


#ifdef PLUGIN_SYSTEM
////////////////////////////////////////////////////////////////
//
// class COggPluginSettingsView
//
////////////////////////////////////////////////////////////////
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

void COggPluginSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, 
                            TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
  {
  COggS60Utility::DisplayStatusPane( R_OGG_CODEC_SELECTION);

  COggPlayAppUi* appUi = (COggPlayAppUi*)CEikonEnv::Static()->AppUi();
  iCodecSelection = new (ELeave) COggplayCodecSelectionSettingItemList(  );
  iCodecSelection->SetMopParent(appUi);
  iCodecSelection->ConstructL( appUi->ClientRect() );
  appUi->AddToStackL(*this, iCodecSelection);

  // Push new softkeys
  appUi->Cba()->AddCommandSetToStackL( R_USER_SELECT_BACK_CBA );
  // Transfert the softkey command ("select") to iCodecSelect
  appUi->Cba()->UpdateCommandObserverL(0,*iCodecSelection); 
 
  // Added in order to get the CBA drawn
  iOggViewCtl.SetRect( appUi->ApplicationRect() );
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

  
////////////////////////////////////////////////////////////////
//
// class COggUserHotkeysView
//
////////////////////////////////////////////////////////////////
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


void COggUserHotkeysView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, 
                            TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
	{
  COggS60Utility::DisplayStatusPane(R_OGG_USER_HOTKEYS);

  COggPlayAppUi* appUi = (COggPlayAppUi*)CEikonEnv::Static()->AppUi();
	iUserHotkeysContainer = new (ELeave) COggUserHotkeysControl( appUi->iSettings );
	iUserHotkeysContainer->SetMopParent(appUi);
	iUserHotkeysContainer->ConstructL( appUi->ClientRect() );
  appUi->AddToStackL(*this, iUserHotkeysContainer);

  // Push new softkeys
  appUi->Cba()->AddCommandSetToStackL(R_USER_CLEAR_BACK_CBA);

  // Added in order to get the CBA drawn
  iOggViewCtl.SetRect( appUi->ApplicationRect() );
	}

void COggUserHotkeysView::ViewDeactivated()
  {
  COggS60Utility::RemoveStatusPane();

  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  appUi->RemoveFromStack(iUserHotkeysContainer);
  delete iUserHotkeysContainer;
  iUserHotkeysContainer = NULL;

  }


#ifdef SERIES60_SPLASH_WINDOW_SERVER
////////////////////////////////////////////////////////////////
//
// class COggSplashView
//
////////////////////////////////////////////////////////////////
COggSplashView::COggSplashView(COggPlayAppView& aOggViewCtl )
: COggViewBase(aOggViewCtl), iOggViewCtl(aOggViewCtl)
	{
	}

COggSplashView::~COggSplashView()
	{
	}

TVwsViewId COggSplashView::ViewId() const
	{
	return TVwsViewId(KOggPlayUid, KOggPlayUidSplashView);
	}

void COggSplashView::ViewActivatedL(const TVwsViewId& aPrevViewId, 
                                    TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/)
{
    COggPlayAppUi* appUi = (COggPlayAppUi*)CEikonEnv::Static()->AppUi();
    iSplashContainer = new (ELeave) CSplashContainer();
    iSplashContainer->SetMopParent(appUi);
    iSplashContainer->ConstructL( appUi->ApplicationRect(), aPrevViewId );
    appUi->AddToStackL(*this, iSplashContainer);
}

void COggSplashView::ViewDeactivated()
{
    CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
    appUi->RemoveFromStack(iSplashContainer);
    delete iSplashContainer;
    iSplashContainer = NULL;
    
}
#endif /* SERIES60_SPLASH_WINDOW_SERVER */
#endif
