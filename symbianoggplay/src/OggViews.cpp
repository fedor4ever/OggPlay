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

#include <eikappui.h>
#include <eikmenub.h>
#include <aknlists.h>
#include <akntitle.h>

#include "OggLog.h"
#include "OggViews.h"
#include "OggPlayAppView.h"
#include "OggSettingsContainer.h"
#include "OggUserHotkeys.h"

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


////////////////////////////////////////////////////////////////
//
// class COggSettingsView
//
////////////////////////////////////////////////////////////////

COggSettingsView::COggSettingsView(COggPlayAppView& aOggViewCtl)
: COggViewBase(aOggViewCtl)
{
}

COggSettingsView::~COggSettingsView()
{
}

void COggSettingsView::ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
				  const TDesC8& /*aCustomMessage*/)
{
  TPixelsAndRotation sizeAndRotation;
  CEikonEnv::Static()->ScreenDevice()->GetDefaultScreenSizeAndRotation(sizeAndRotation);
  CEikonEnv::Static()->ScreenDevice()->SetScreenSizeAndRotation(sizeAndRotation);

#if defined(SERIES60)
  CEikButtonGroupContainer* Cba=CEikButtonGroupContainer::Current();
  if(Cba) {
    Cba->AddCommandSetToStackL(R_USER_HOTKEYS_CBA);
    Cba->DrawNow();
  }
  CEikonEnv::Static()->AppUiFactory()->StatusPane()->MakeVisible(ETrue);
#endif

  if (!iContainer)
    {
    iContainer = new (ELeave) COggSettingsContainer;
    iContainer->ConstructL( ((CAknAppUi*)CEikonEnv::Static()->AppUi())->ClientRect() );
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
  
#ifdef SERIES60
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();
  // Remove status pane
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->MakeVisible(EFalse);
  // Restore softkeys
  ((COggPlayAppUi*) appUi)->UpdateSeries60Softkeys();
#endif
  
}

TVwsViewId COggSettingsView::ViewId() const
{
  return TVwsViewId(KOggPlayUid, KOggPlayUidSettingsView);
}
////////////////////////////////////////////////////////////////
//
// class COggUserView
//
////////////////////////////////////////////////////////////////

COggUserHotkeysView::COggUserHotkeysView(COggPlayAppView& aOggViewCtl )
: COggViewBase(aOggViewCtl), iOggViewCtl(aOggViewCtl)
	{
  ;
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
#ifdef SERIES60
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();

	iUserHotkeysContainer = new (ELeave) COggUserHotkeys(*iOggViewCtl.iHotkeys );
	iUserHotkeysContainer->SetMopParent(appUi);
	TRect rec(0,44,176,188); // FIXIT
	iUserHotkeysContainer->ConstructL( rec );
  appUi->AddToStackL(*this, iUserHotkeysContainer);

  // Push new softkeys
  appUi->Cba()->AddCommandSetToStackL(R_USER_HOTKEYS_CBA);

  // Enable statuspane
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  CAknTitlePane* titlePane=(CAknTitlePane*)statusPane->ControlL(TUid::Uid(EEikStatusPaneUidTitle));
  TBuf<32> buf;
  CEikonEnv::Static()->ReadResource(buf,R_OGG_USER_HOTKEYS);
  titlePane->SetTextL(buf);

  iOggViewCtl.SetRect( TRect(0,0,176,208)); // FIXIT

  statusPane->MakeVisible(ETrue);
#endif
	}


void COggUserHotkeysView::ViewDeactivated()
  {
#ifdef SERIES60
  CAknAppUi* appUi = (CAknAppUi*)CEikonEnv::Static()->AppUi();

  // Remove status pane
  CEikStatusPane* statusPane = CEikonEnv::Static()->AppUiFactory()->StatusPane();
  statusPane->MakeVisible(EFalse);
  iOggViewCtl.SetRect(appUi->ClientRect());
  
  appUi->RemoveFromStack(iUserHotkeysContainer);

  // Restore softkeys
  ((COggPlayAppUi*) appUi)->UpdateSeries60Softkeys();

  delete iUserHotkeysContainer;
  iUserHotkeysContainer = NULL;
#endif
  }

