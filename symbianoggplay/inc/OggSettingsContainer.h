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
#include <aknsettingitemlist.h>  


class COggplayDisplaySettingItemList : public CAknSettingItemList
{
public:
  COggplayDisplaySettingItemList(TOggplaySettings& aData);
  
protected:
  virtual CAknSettingItem* CreateSettingItemL(TInt aSettingId);
  
private:
  TOggplaySettings& iData;
};


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
        void ConstructL(const TRect& aRect);
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
        COggplayDisplaySettingItemList* iListBox;
    };

#endif

// End of File
