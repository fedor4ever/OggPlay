/*
 *  Copyright (c) 2005 OggPlay developpers
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
#ifndef OGGPLUGINSELECTIONDIALOGS60_H
#define OGGPLUGINSELECTIONDIALOGS60_H

// INCLUDES
#include "OggPlay.h"

#if ( defined(SERIES60) && defined(PLUGIN_SYSTEM) )
#include <coecntrl.h>
#include <aknsettingitemlist.h> 
#include <aknpopupsettingpage.h> 


class CEikTextListBox;

/// Control hosting listbox for plugin assignments.
//

class COggplayCodecSelectionSettingItemList : public CCoeControl ,public MEikCommandObserver
  {
public:
  COggplayCodecSelectionSettingItemList ( );
  void ConstructL(const TRect& aRect);
  ~COggplayCodecSelectionSettingItemList();

private : // New
  void RefreshListboxModel();
  static TInt TimerExpired(TAny* aPtr);
  
private : // Framework
  TInt CountComponentControls() const;
  CCoeControl* ComponentControl(TInt aIndex) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType );
  void ProcessCommandL (TInt aCommandId);
 
private:
  CEikTextListBox *iListBox;      // Owned
  CDesCArrayFlat *iExtensionList; // Owned
  COggTimer *iWaitTimer;          //Owned
  CPluginSupportedList *iPluginSupportedList; // Not Owned
  

  
  };


/* A list, showing the info about the choosen codec */

class CCodecInfoList :  public MEikCommandObserver, public CCoeControl
  {
public:
  CCodecInfoList( CPluginInfo& aPluginInfo );
  void ConstructL(const TRect& aRect);
  ~CCodecInfoList();

private : // New
	void RefreshListboxModel();

private : // Framework
  TInt CountComponentControls() const;
  CCoeControl* ComponentControl(TInt aIndex) const;
  TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType );

private: // From MEikCommandObserver
    void ProcessCommandL (TInt aCommandId);
private:
  CPluginInfo& iPluginInfo;
  CEikTextListBox *iListBox;
  CActiveSchedulerWait iWait;	
  CEikButtonGroupContainer *iCba;

  };
  
#endif /* SERIES60 && MMF_AVAILABLE */

#endif /* OGGPLUGINSELECTIONDIALOGS60_H */

// End of File
