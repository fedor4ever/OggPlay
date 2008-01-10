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
#include "OggSplashCOntainer.h"

class COggPlayAppView;
class COggUserHotkeysControl;
class COggViewBase : public CBase, public MCoeView
{
public:
  COggViewBase(COggPlayAppView& aOggViewCtl);
  ~COggViewBase();
  
protected:
  // From MCoeView
  void ViewDeactivated();
  void ViewConstructL();
  void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);

protected:
  COggPlayAppView& iOggViewCtl;

private:
  COggViewBase* iNext;
  friend class COggPlayAppView;
};

class COggSplashView : public CBase, public MCoeView
	{
public:
	COggSplashView(const TUid& aViewId);
	~COggSplashView();

	void ConstructL();

	// From MCoeView
	TVwsViewId ViewId() const;
	void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void ViewDeactivated();

#if defined(UIQ)
	TBool ViewScreenModeCompatible(TInt aScreenMode);
#endif

private:
	TUid iViewId;
	CSplashContainer *iSplashContainer;
	};

class COggFOView : public COggViewBase
{
 public:
  COggFOView(COggPlayAppView& aOggViewCtl);
  ~COggFOView();

  TVwsViewId ViewId() const;

#if defined(UIQ)
  TVwsViewIdAndMessage ViewScreenDeviceChangedL();
  TBool ViewScreenModeCompatible(TInt aScreenMode);
#endif

  void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
};

#if defined(UIQ) || defined(SERIES60SUI)
class COggFCView : public COggViewBase
{
public:
  COggFCView(COggPlayAppView& aOggViewCtl);
  ~COggFCView();

  TVwsViewId ViewId() const;
  void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);

#if defined(UIQ)
  TVwsViewIdAndMessage ViewScreenDeviceChangedL();
  TBool ViewScreenModeCompatible(TInt aScreenMode);
#endif
};
#endif

#if defined(SERIES60)
class COggS60Utility
{
public:
  static void DisplayStatusPane(TInt aTitleID);
  static void RemoveStatusPane();
};

class COggSettingItemList;
class COggSettingsView : public COggViewBase
{
public:
	COggSettingsView(COggPlayAppView& aView);
	~COggSettingsView();

	TVwsViewId ViewId() const;

#if defined(PLUGIN_SYSTEM)
	void VolumeGainChangedL();
#endif

private:  
  // From MCoeView
  void ViewDeactivated();
  void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);

private:
  COggSettingItemList* iContainer;
};

class COggUserHotkeysView : public COggViewBase
	{
public:
	COggUserHotkeysView(COggPlayAppView& aOggViewCtl);
	~COggUserHotkeysView();

	TVwsViewId ViewId() const;
	void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
	void ViewDeactivated();

private:
	COggUserHotkeysControl* iUserHotkeysContainer;
    COggPlayAppView& iOggViewCtl;
	};

#if defined(PLUGIN_SYSTEM)
class COggplayCodecSelectionSettingItemList;
class COggPluginSettingsView : public COggViewBase
	{
public:
	COggPluginSettingsView(COggPlayAppView& aOggViewCtl);
	~COggPluginSettingsView();

	// From MCoeView
	TVwsViewId ViewId() const;
	void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
    void ViewDeactivated();

  private:
	COggplayCodecSelectionSettingItemList* iCodecSelection;
    COggPlayAppView& iOggViewCtl;
	};	
#endif

class COggPlaybackOptionsView : public COggViewBase
{
public:
  COggPlaybackOptionsView(COggPlayAppView& aView);
  ~COggPlaybackOptionsView();
 
  TVwsViewId ViewId() const;
  void VolumeGainChangedL();
  void BufferingModeChangedL();

public:
  COggSettingItemList* iContainer;

private:
  // From MCoeView
  void ViewDeactivated();
  void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
};

class COggAlarmSettingsView : public COggViewBase
{
public:
  COggAlarmSettingsView(COggPlayAppView& aView);
  ~COggAlarmSettingsView();
 
  virtual TVwsViewId ViewId() const;

public:
  COggSettingItemList* iContainer;

private:
  // From MCoeView
  void ViewDeactivated();
  void ViewActivatedL(const TVwsViewId& aPrevViewId, TUid aCustomMessageId, const TDesC8& aCustomMessage);
};

#endif // SERIES60
#endif
