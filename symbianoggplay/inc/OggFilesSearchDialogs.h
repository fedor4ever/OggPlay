
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
        
        static COggFilesSearchContainer* NewL( MOggFilesSearchBackgroundProcess *aBackgroundProcess);
        
        ~COggFilesSearchContainer ();
        void UpdateControl();
        
    public: // from CoeControl
                
        void ConstructL( const TRect& aRect);
        TInt CountComponentControls() const;
        CCoeControl* ComponentControl(TInt aIndex) const;
        void Draw(const TRect& aRect) const;
        TCoeInputCapabilities InputCapabilities() const; 

    private:
        void COggFilesSearchContainer::ConstructFromResourceL(TResourceReader& aReader);

    public:
         MOggFilesSearchBackgroundProcess *iBackgroundProcess;
         
    private:
        CArrayPtrFlat <CEikLabel> * iLabels;
        COggFilesSearchAO * iAO;
    };

class COggFilesSearchAO : public CTimer
{
public:
    COggFilesSearchAO( COggFilesSearchContainer * aContainer );
    void StartL();
private:
	void RunL();
    COggFilesSearchContainer * iContainer;
    TTime iTime;
};

#endif