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

#include <eikappui.h>

#include "OggViews.h"
#include "OggPlayAppView.h"

#include <eikmenub.h>

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
