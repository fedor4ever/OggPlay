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

#ifndef __OggViews_h
#define __OggViews_h

#include <coecntrl.h>

class COggPlayAppView;

class COggViewBase : public CBase, public MCoeView
{
 public:

  COggViewBase(COggPlayAppView& aOggViewCtl);
  ~COggViewBase();
  
 protected:		

  // implements MCoeView:
  virtual void ViewDeactivated();
  virtual void ViewConstructL();
  virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			      const TDesC8& /*aCustomMessage*/);
  
  COggPlayAppView& iOggViewCtl;
};

class COggFOView : public COggViewBase
{
 public:

  COggFOView(COggPlayAppView& aOggViewCtl);
  ~COggFOView();
  virtual TVwsViewId ViewId() const;
  virtual TVwsViewIdAndMessage ViewScreenDeviceChangedL();
  virtual TBool ViewScreenModeCompatible(TInt aScreenMode);
  virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			      const TDesC8& /*aCustomMessage*/);
};

class COggFCView : public COggViewBase
{
 public:

  COggFCView(COggPlayAppView& aOggViewCtl);
  ~COggFCView();
  virtual TVwsViewId ViewId() const;
  virtual TVwsViewIdAndMessage ViewScreenDeviceChangedL();
  virtual TBool ViewScreenModeCompatible(TInt aScreenMode);
  virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			      const TDesC8& /*aCustomMessage*/);
};

#endif
