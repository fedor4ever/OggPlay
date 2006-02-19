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

#ifndef __OggViews_h
#define __OggViews_h

#include <coecntrl.h>

#if defined(SERIES60_SPLASH_WINDOW_SERVER)
#include "OggSplashCOntainer.h"
#endif

class COggPlayAppView;
class COggUserHotkeysControl;

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

#if defined(SERIES60)
class COggS60Utility
{
public:
  static void DisplayStatusPane( TInt aTitleID );
  static void RemoveStatusPane();
};

class COggSettingsContainer;

class COggSettingsView : public COggViewBase
{
  public:

  COggSettingsView(COggPlayAppView&,  TUid aViewUid);
  ~COggSettingsView();
  virtual TVwsViewId ViewId() const;

#if !defined(MULTI_THREAD_PLAYBACK)
  void VolumeGainChangedL();
#endif

  COggSettingsContainer* iContainer;
  private:
  
    // implements MCoeView:
  virtual void ViewDeactivated();
  virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			      const TDesC8& /*aCustomMessage*/);

  TUid iUid;
  
};

class COggUserHotkeysView : public COggViewBase
	{
	public:
		COggUserHotkeysView(COggPlayAppView& aOggViewCtl);
		~COggUserHotkeysView();
		virtual TVwsViewId ViewId() const;
		virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			const TDesC8& /*aCustomMessage*/);
    void ViewDeactivated();

  private:
		COggUserHotkeysControl* iUserHotkeysContainer;
        COggPlayAppView& iOggViewCtl;
	};

#ifdef PLUGIN_SYSTEM
class COggplayCodecSelectionSettingItemList;
class COggPluginSettingsView : public COggViewBase
	{
	public:
		COggPluginSettingsView(COggPlayAppView& aOggViewCtl);
		~COggPluginSettingsView();
		virtual TVwsViewId ViewId() const;
		virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			const TDesC8& /*aCustomMessage*/);
    void ViewDeactivated();

  private:
	COggplayCodecSelectionSettingItemList* iCodecSelection;
    COggPlayAppView& iOggViewCtl;
	};
	
#endif
	
#ifdef SERIES60_SPLASH_WINDOW_SERVER
class COggSplashView : public COggViewBase
	{
	public:
		COggSplashView(COggPlayAppView& aOggViewCtl);
		~COggSplashView();
		virtual TVwsViewId ViewId() const;
		virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, 
			const TDesC8& /*aCustomMessage*/);
    void ViewDeactivated();

  private:
        COggPlayAppView& iOggViewCtl;
        CSplashContainer *iSplashContainer;
	};
#endif /*SERIES60_SPLASH_WINDOW_SERVER*/

#if defined(MULTI_THREAD_PLAYBACK)
class COggPlaybackOptionsView : public COggViewBase
{
public:
  COggPlaybackOptionsView(COggPlayAppView&,  TUid aViewUid);
  ~COggPlaybackOptionsView();
 
  virtual TVwsViewId ViewId() const;
  void VolumeGainChangedL();
  void BufferingModeChangedL();

public:
  COggSettingsContainer* iContainer;

private:
  // implements MCoeView:
  virtual void ViewDeactivated();
  virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/);

private:
  TUid iUid;
};
#endif

class COggAlarmSettingsView : public COggViewBase
{
public:
  COggAlarmSettingsView(COggPlayAppView&,  TUid aViewUid);
  ~COggAlarmSettingsView();
 
  virtual TVwsViewId ViewId() const;

public:
  COggSettingsContainer* iContainer;

private:
  // implements MCoeView:
  virtual void ViewDeactivated();
  virtual void ViewActivatedL(const TVwsViewId& /*aPrevViewId*/, TUid /*aCustomMessageId*/, const TDesC8& /*aCustomMessage*/);

private:
  TUid iUid;
};
#endif /* SERIES_60 */
#endif
