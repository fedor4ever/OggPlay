
#ifndef OGGFILESSEARCHDIALOGS_H
#define OGGFILESSEARCHDIALOGS_H

#include <OggPlatform.h>

#include <e32base.h>
#include <eikdialg.h>
#include <barsread.h>
#include <eiklabel.h>

#if defined(SERIES60)
#include <akndialog.h>
#endif

#include "OggControls.h"

const TInt KOggFileScanStepComplete = 1;
class MOggFilesSearchBackgroundProcess
{
public:
    virtual void FileSearchStep(TRequestStatus& aRequestStatus)= 0;
	virtual void ScanNextPlayList(TRequestStatus& aRequestStatus) = 0;
	virtual void CancelOutstandingRequest() = 0;

	virtual void FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles, TInt &aNbPlayLists) = 0;
};


class COggFilesSearchContainer;
class COggFilesSearchAO;
#if defined(SERIES60)
class COggFilesSearchDialog : public CAknDialog
#else
class COggFilesSearchDialog : public CEikDialog
#endif
	{
public:
    COggFilesSearchDialog(MOggFilesSearchBackgroundProcess *aBackgroundProcess);
    SEikControlInfo CreateCustomControlL(TInt aControlType);

private:
    COggFilesSearchContainer *iContainer;
    MOggFilesSearchBackgroundProcess *iBackgroundProcess;
	};

class CAknsBasicBackgroundControlContext;
class COggFilesSearchContainer : public CCoeControl
    {
public:        
	COggFilesSearchContainer(COggFilesSearchDialog& aDlg, MOggFilesSearchBackgroundProcess* aBackgroundProcess, CEikButtonGroupContainer* aCba);
    ~COggFilesSearchContainer();

	void UpdateControl();

#if defined(SERIES60) || defined(SERIES80)
        void UpdateCba();
#endif
	
	// from CCoeControl
    TSize MinimumSize();

    void Draw(const TRect& aRect) const;

#if defined(SERIES60V3)
	void SizeChanged();
	TTypeUid::Ptr MopSupplyObject(TTypeUid aId);
#endif

private:
	COggFilesSearchContainer(COggFilesSearchDialog& aDlg);
    void ConstructFromResourceL(TResourceReader& aReader);

private:
	COggFilesSearchDialog& iParent;
    MOggFilesSearchBackgroundProcess* iBackgroundProcess;
    CEikButtonGroupContainer* iCba;
     
	TBuf<128> iTitleTxt;
	TBuf<128> iLine1Txt;
	TBuf<128> iLine2Txt;
	TBuf<128> iLine3Txt;
		
	COggFilesSearchAO* iAO;
	CFbsBitmap* iFishBmp; 
    CFbsBitmap* iFishMask;

    TPoint iFishPosition;
	TInt iFishMinX;
	TInt iFishMaxX;
	TInt iFishStepX;

	TInt iFoldersCount;
	TInt iFilesCount;
	TInt iPlayListCount;

#if defined(SERIES60V3)
	CAknsBasicBackgroundControlContext* iBgContext;
#endif

	friend class COggFilesSearchAO;
	};

class COggFilesSearchAO : public CActive
{
public:
    COggFilesSearchAO( COggFilesSearchContainer * aContainer );
	~COggFilesSearchAO();

    void StartL();

private:
	enum TSearchStatus { EScanningFiles, EScanningPlaylists, EScanningComplete };

private:
	void RunL();
	void DoCancel();

	void SelfComplete();
	static TInt CallBack(TAny* aPtr);

private:
	TSearchStatus iSearchStatus;
    COggFilesSearchContainer* iContainer;

	CPeriodic* iTimer;
	TCallBack* iCallBack;
};

#endif
