/*
* ============================================================================
*  Name     : COggSettingsContainer from OggContainer.h
*  Part of  : Ogg
*  Created  : 29.11.2003 by 
*  Description:
*     Declares container control for application.
*  Version  :
*  Copyright: 
* ============================================================================
*/

#ifndef OGGCONTAINER_H
#define OGGCONTAINER_H

// INCLUDES
#include <coecntrl.h>
   
// FORWARD DECLARATIONS
class CEikLabel;        // for example labels

// CLASS DECLARATION

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

       /**
        * From CoeControl,SizeChanged.
        */
        void SizeChanged();

       /**
        * From CoeControl,CountComponentControls.
        */
        TInt CountComponentControls() const;

       /**
        * From CCoeControl,ComponentControl.
        */
        CCoeControl* ComponentControl(TInt aIndex) const;

       /**
        * From CCoeControl,Draw.
        */
        void Draw(const TRect& aRect) const;

       /**
        * From ?base_class ?member_description
        */
        // event handling section
        // e.g Listbox events
        void HandleControlEventL(CCoeControl* aControl,TCoeEvent aEventType);
        
    private: //data
        
        CEikLabel* iLabel;          // example label
        CEikLabel* iToDoLabel;      // example label
    };

#endif

// End of File
