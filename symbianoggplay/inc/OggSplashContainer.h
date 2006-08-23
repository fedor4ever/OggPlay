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

// Startup splash screen control
class CSplashContainer :public CCoeControl
    {
public:
    void ConstructL();
    void ShowSplashL();
    ~CSplashContainer();

public: // Functions from base classes
    void Draw(const TRect& aRect) const;

private: // New Functions
	static TInt TimerExpired(TAny* aPtr);

private: //data
    COggTimer* iDisplayTimer;
	CFbsBitmap* iBitmap;
    };

#endif

// End of File
