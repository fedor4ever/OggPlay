/*
 *  Copyright (c) 2003 P.Wolff
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
#ifndef OGGCONTAINER_H
#define OGGCONTAINER_H

// INCLUDES
#include "OggPlay.h"

#include <coecntrl.h>
#if defined(SERIES60)
#include <aknsettingitemlist.h> 
#include <aknpopupsettingpage.h> 
#endif



/**
*  COggSettingsContainer  container control class.
*  
*/
class COggSettingsContainer : public CCoeControl, MCoeControlObserver
    {
    public: // Constructors and destructor
        
        /**
        * EPOC default constructor.
        * @param aRect Frame rectangle for container.
        */
        void ConstructL(const TRect& aRect, TUid aId);
        /**
        * Destructor.
        */
        ~COggSettingsContainer();

    public: // New functions

    public: // Functions from base classes

    private: // Functions from base classes
        void SizeChanged();
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
        void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
        TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);

    private: //data
#if defined(SERIES60)
        CAknSettingItemList* iListBox;
#endif
    };



#if defined(SERIES60)
class COggplayDisplaySettingItemList : public CAknSettingItemList
{
public:
  COggplayDisplaySettingItemList(COggPlayAppUi& aAppUi);
  
protected:
  virtual CAknSettingItem* CreateSettingItemL(TInt aSettingId);
  
private:
  TOggplaySettings& iData;
  COggPlayAppUi& iAppUi;
};

class CGainSettingItem : public CAknEnumeratedTextPopupSettingItem 
    {
    public:  // Constructors and destructor
        CGainSettingItem( TInt aIdentifier,  COggPlayAppUi& aAppUi);
    private: // Functions from base classes
        void EditItemL ( TBool aCalledFromMenu );
    private: //data
        COggPlayAppUi& iAppUi;
    };

class CCodecSelectionSettingItem;

#ifdef MMF_AVAILABLE 
class COggplayCodecSelectionSettingItemList : public CAknSettingItemList
{
public:
  COggplayCodecSelectionSettingItemList(COggPlayAppUi& aAppUi);
  
protected:
  virtual CAknSettingItem* CreateSettingItemL(TInt aSettingId);
  virtual void EditItemL  ( TInt aIndex, TBool aCalledFromMenu );
 

private:
  TOggplaySettings& iData;
  COggPlayAppUi& iAppUi;
  TInt iSelectedIndexes[NUMBER_OF_SEARCHED_EXTENSIONS]; // These indexes will be translated to UIDs when the
                                                        // Setting list will be dismissed
  CCodecSelectionSettingItem * iCodecSelectionSettingItemArray[NUMBER_OF_SEARCHED_EXTENSIONS];
};


class CCodecSelectionSettingItem : public CAknEnumeratedTextPopupSettingItem 
    {
    public:  // Constructors and destructor
        CCodecSelectionSettingItem( TInt aResourceId, TInt &aValue,
             CAbsPlayback * aPlayback,  const TDesC & anExtension);
    private: // Functions from base classes
         // From CAknEnumeratedTextPopupSettingItem         
        void CompleteConstructionL ()  ;
        void EditItemL ( TBool aCalledFromMenu );
    public : // New Functions
        TInt32 GetSelectedUid();
    private: //data
        CExtensionSupportedPluginList *iPluginList;
        CAbsPlayback & iPlayback;
    };


/* A list, showing the info about the choosen codec */

class CCodecInfoList : public CCoeControl, public MEikCommandObserver
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
#endif /* MMF_AVAILABLE */

#endif /* SERIES60 */

#endif

// End of File
