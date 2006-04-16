/*
* ============================================================================
*  Name     : CSplashView from SplashView.h
*  Part of  : Splash
*  Created  : 17/02/2003 by Eric@NewLC
*  Description:
*     Declares container control for application.
*  Version  :
*  Copyright: 
* ============================================================================
*/

#ifndef SPLASHVIEW_H
#define SPLASHVIEW_H

// INCLUDES
#include <coecntrl.h>
#include <vwsdef.h>
#include "OggHelperFcts.h"

// FORWARD DECLARATIONS

// CLASS DECLARATION

/**
*  CSplashView  container control class.
*  
*/
class CSplashContainer :public CCoeControl, MCoeControlObserver
    {
    public: // Constructors and destructor
        
        /**
        * EPOC default constructor.
        * @param aRect Frame rectangle for container.
        */
        void ConstructL(const TRect& aRect, const TVwsViewId& aPrevViewId);

        /**
        * Destructor.
        */
        ~CSplashContainer();

    public: // New functions

    public: // Functions from base classes
        void Draw(const TRect& aRect) const;

    private: // New Functions
     static TInt TimerExpired(TAny* aPtr);
	 void DrawL() const;


    private: // Functions from base classes

       /**
        * From CoeControl,CountComponentControls.
        */
        TInt CountComponentControls() const;

       /**
        * From CCoeControl,ComponentControl.
        */
        CCoeControl* ComponentControl(TInt aIndex) const;

        void HandleControlEventL(CCoeControl* /*aControl*/,TCoeEvent /*aEventType*/)  { /*Empty*/};
    private: //data
        COggTimer * iDisplayTimer;
        TVwsViewId iPrevViewId;
    };

#endif

// End of File
