
#ifndef OGGFILESSEARCHDIALOGS_H
#define OGGFILESSEARCHDIALOGS_H

#include <e32base.h>
#include <eikdialg.h>
#include <barsread.h>
#include <eiklabel.h>
#include "OggControls.h"

class  COggFilesSearchContainer;
class  COggFilesSearchAO;

class MOggFilesSearchBackgroundProcess
{
public: // interface
    virtual void  FileSearchStepL()=0;
    virtual TBool FileSearchIsProcessDone() const =0;
    virtual void  FileSearchProcessFinished() { }
    virtual void  FileSearchDialogDismissedL(TInt /*aButtonId*/) { }
    virtual TInt  FileSearchCycleError(TInt aError) { return aError; }
    virtual void  FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles) = 0;
};


class COggFilesSearchDialog : public CEikDialog
{

public:
    COggFilesSearchDialog(MOggFilesSearchBackgroundProcess *aBackgroundProcess);

    SEikControlInfo CreateCustomControlL(TInt aControlType);
    
private:
    COggFilesSearchContainer * iContainer;
    MOggFilesSearchBackgroundProcess *iBackgroundProcess;
};


class COggFilesSearchContainer : public CCoeControl
    {
    public: 
        
        COggFilesSearchContainer::COggFilesSearchContainer();
        
        static COggFilesSearchContainer* NewL( MOggFilesSearchBackgroundProcess *aBackgroundProcess,
            CEikButtonGroupContainer * aCba);
        
        ~COggFilesSearchContainer ();
        void UpdateControl();
        
#if defined(SERIES60) || defined(SERIES80)
        void UpdateCba();
#endif
	
	public: // from CoeControl
                
        void ConstructL( const TRect& aRect);
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
        TCoeInputCapabilities InputCapabilities() const; 

    private:
        void ConstructFromResourceL(TResourceReader& aReader);

    public:
         MOggFilesSearchBackgroundProcess *iBackgroundProcess;
         
    private:
        CArrayPtrFlat <CEikLabel> * iLabels;
        
        CFont* iFontLatinPlain;
        CFont* iFontLatinBold12;
        COggFilesSearchAO * iAO;
        CEikButtonGroupContainer * iCba;
        CFbsBitmap * ifish1; 
        CFbsBitmap * ifishmask; 
        TInt iFishPosition;
    };

class COggFilesSearchAO : public CActive
{
public:
    COggFilesSearchAO( COggFilesSearchContainer * aContainer );
	~COggFilesSearchAO();

    void StartL();
private:
	void RunL();
	void DoCancel();

	void SelfComplete();
	static TInt CallBack(TAny* aPtr);

    COggFilesSearchContainer * iContainer;

	CPeriodic* iTimer;
	TCallBack* iCallBack;
};

#endif