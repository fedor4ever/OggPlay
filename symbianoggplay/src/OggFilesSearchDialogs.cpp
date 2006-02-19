
#include "OggFilesSearchDialogs.h"
#include "Oggplay.hrh"
#include <Oggplay.rsg>
#include <coemain.h>
#ifdef SERIES60
#include <aknappui.h>
#include <avkon.rsg>
#endif
#ifdef SERIES80
#include <eikenv.h>
#include <eikappui.h>
#include <eikbtgpc.h>
#include <eikcore.rsg>
#endif

#ifdef UIQ
#include <eikappui.h>
#endif

#include <eikapp.h>


COggFilesSearchDialog::COggFilesSearchDialog(MOggFilesSearchBackgroundProcess *aBackgroundProcess)
{
    iBackgroundProcess = aBackgroundProcess;
}


SEikControlInfo COggFilesSearchDialog::CreateCustomControlL(TInt aControlType)
{
    if (aControlType == EOggFileSearchControl) 
    { 
        iContainer = COggFilesSearchContainer::NewL(iBackgroundProcess,&ButtonGroupContainer());
    } 
    SEikControlInfo info = {iContainer,0,0};
    return info;
}


/////////////////////

COggFilesSearchContainer::COggFilesSearchContainer()
    {
    }

void COggFilesSearchContainer::ConstructFromResourceL(TResourceReader& aReader)
{
    TInt width=aReader.ReadInt16();
    TInt height=aReader.ReadInt16();
    TSize containerSize (width, height);
    SetSize(containerSize);
    
    // Prepare fonts.
    TFontSpec fs(_L("LatinPlain12"), 12);
    CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(iFontLatinPlain,fs);
    TFontSpec fs2(_L("LatinBold12"), 12);
    CCoeEnv::Static()->ScreenDevice()->GetNearestFontInPixels(iFontLatinBold12,fs2);

    
#ifdef PLAYLIST_SUPPORT
	iLabels = new(ELeave)CArrayPtrFlat<CEikLabel>(7);
    
    TPtrC text[] = { aReader.ReadTPtrC(),  aReader.ReadTPtrC(),  aReader.ReadTPtrC(), aReader.ReadTPtrC(), _L("0"), _L("0"),_L("0") };
    const CFont* fonts[] = {iFontLatinBold12, iFontLatinPlain, iFontLatinPlain,iFontLatinPlain, iFontLatinBold12,iFontLatinBold12,iFontLatinBold12};
	const TInt  PosValues [][4] = { {10, 2, 156, 16}, {2, 25, 140, 16}, {2,45,140,16}, {2,65,140,16}, {145,25,20,16}, {145,45,30,16}, {145,65,20,16} };
	const TInt Colors[][3] = { {0,0,255}, {0,0,0}, {0,0,0},{0,0,0},{0,0,0},{0,0,0},{0,0,0} };
    const TGulAlignmentValue Align[] = { EHCenterVCenter,EHRightVTop,EHRightVTop,EHRightVTop,EHLeftVTop,EHLeftVTop,EHLeftVTop };
    for (TInt i=0; i<7; i++)
#else
    iLabels = new(ELeave)CArrayPtrFlat<CEikLabel>(5);
    
    TPtrC text[] = { aReader.ReadTPtrC(),  aReader.ReadTPtrC(),  aReader.ReadTPtrC(), _L("0"), _L("0") };
    const CFont* fonts[] = {iFontLatinBold12, iFontLatinPlain, iFontLatinPlain,iFontLatinBold12,iFontLatinBold12};
    const TInt  PosValues [][4] = { {10, 2, 156, 16}, {2, 25, 140, 16}, {2,45,140,16}, {145,25,20,16}, {145,45,40,16} };
    const TInt Colors[][3] = { {0,0,255}, {0,0,0}, {0,0,0},{0,0,0},{0,0,0} };
    const TGulAlignmentValue Align[] = { EHCenterVCenter,EHRightVTop,EHRightVTop,EHLeftVTop,EHLeftVTop };
    for (TInt i=0; i<5; i++)
#endif
    {
        CEikLabel * c = new(ELeave) CEikLabel();
  
        c->SetContainerWindowL(*this);
        c->OverrideColorL(0x36,TRgb(Colors[i][0], Colors[i][1], Colors[i][2]) );
        TPoint pos (PosValues[i][0],PosValues[i][1]);
        TSize size (PosValues[i][2],PosValues[i][3]);
        c->SetExtent(pos,size);
        c->SetTextL( text[i] );
        c->SetFont(fonts[i]);
        c->SetAlignment(Align[i]);
        iLabels->AppendL(c);
    }

     //Load bitmaps

    TFileName fileName;
    fileName.Copy(CEikonEnv::Static()->EikAppUi()->Application()->AppFullName());
    // Parse and strip the application name
    TParsePtr parse(fileName);
    // Copy back only drive and path
    fileName.Copy(parse.DriveAndPath());
    fileName.Append(_L("fish.mbm"));
    ifish1 = iEikonEnv->CreateBitmapL(fileName, 0) ; 
    ifishmask = iEikonEnv->CreateBitmapL(fileName, 1) ;
    iFishPosition = 40;

    ActivateL();	
    iAO = new (ELeave) COggFilesSearchAO(this);
    iAO->StartL();
}


COggFilesSearchContainer* COggFilesSearchContainer::NewL(
                                      MOggFilesSearchBackgroundProcess *aBackgroundProcess ,
                                      CEikButtonGroupContainer * aCba)
    {
    COggFilesSearchContainer* self = new (ELeave) COggFilesSearchContainer;
    self->iBackgroundProcess = aBackgroundProcess;
    self->iCba = aCba;
    return self;
    }


TInt COggFilesSearchContainer::CountComponentControls() const
    {
#ifdef PLAYLIST_SUPPORT
    return 7;
#else
    return 5;
#endif
    }


CCoeControl* COggFilesSearchContainer::ComponentControl(TInt aIndex) const
    {
    return ((*iLabels)[aIndex]);
    }

void COggFilesSearchContainer::Draw(const TRect& aRect) const
    {    
    CWindowGc& gc = SystemGc();
    gc.SetBrushColor(KRgbWhite);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(aRect);

#ifdef PLAYLIST_SUPPORT
	gc.BitBltMasked(TPoint(iFishPosition,90),ifish1,
        TRect(0,0, 42, 28), ifishmask, ETrue);
#else
    gc.BitBltMasked(TPoint(iFishPosition,60),ifish1,
        TRect(0,0, 42, 28), ifishmask, ETrue);
#endif

    }


#if defined(SERIES60) || defined(SERIES80)
void COggFilesSearchContainer::UpdateCba()
{
#if defined(SERIES60)
	iCba->SetCommandSetL(  R_AVKON_SOFTKEYS_OK_EMPTY );
#elif defined(SERIES80)
    iCba->SetCommandSetL(  R_EIK_BUTTONS_CONTINUE );
#endif

    iCba->DrawNow();
}
#endif

COggFilesSearchContainer::~COggFilesSearchContainer ()
    {
    if (iFontLatinPlain)
    {
        CCoeEnv::Static()->ScreenDevice()->ReleaseFont(iFontLatinPlain);
    }
    if (iFontLatinBold12)
    {
        CCoeEnv::Static()->ScreenDevice()->ReleaseFont(iFontLatinBold12);
    }

    iLabels->ResetAndDestroy();
    delete iLabels;
    delete ifish1;
    delete ifishmask;
    delete iAO;
    }


TCoeInputCapabilities COggFilesSearchContainer::InputCapabilities() const
    {
    return TCoeInputCapabilities::ENone;
    }

void COggFilesSearchContainer::UpdateControl()
{
#ifdef PLAYLIST_SUPPORT    
    TInt aNbDir, aNbFiles, aNbPlayLists;
    iBackgroundProcess->FileSearchGetCurrentStatus(aNbDir, aNbFiles, aNbPlayLists);

	TBuf<10> number;
    number.AppendNum(aNbDir);
    (*iLabels)[4]->SetTextL(number); 

	TBuf<10> number2;
    number2.AppendNum(aNbFiles);
    (*iLabels)[5]->SetTextL(number2); 

	TBuf<10> number3;
    number3.AppendNum(aNbPlayLists);
    (*iLabels)[6]->SetTextL(number3);
#else
    TInt aNbDir, aNbFiles;
    iBackgroundProcess->FileSearchGetCurrentStatus(aNbDir,aNbFiles);
    TBuf<10> number;
    number.AppendNum(aNbDir);
    (*iLabels)[3]->SetTextL(number); 
     TBuf<10> number2;
    number2.AppendNum(aNbFiles);
    (*iLabels)[4]->SetTextL(number2); 
#endif

    DrawNow();
    iFishPosition = iFishPosition - 5;
    if (iFishPosition <5)
        iFishPosition = 120;
}


COggFilesSearchAO::COggFilesSearchAO( COggFilesSearchContainer * aContainer)
: CActive(EPriorityIdle), iContainer(aContainer)
{
    CActiveScheduler::Add(this);
}

COggFilesSearchAO::~COggFilesSearchAO()
{
	Cancel();

	delete iCallBack;
	delete iTimer;
}

void COggFilesSearchAO::RunL()
{
    // Run one iteration of the long process.
    MOggFilesSearchBackgroundProcess *longProcess = iContainer->iBackgroundProcess;

#ifdef PLAYLIST_SUPPORT
	if (longProcess->FileSearchIsProcessDone())
		longProcess->ScanNextPlayList();
	else
#endif
		longProcess->FileSearchStepL();
    

#ifdef PLAYLIST_SUPPORT
	if (longProcess->PlayListScanIsProcessDone() )
#else
    if (longProcess->FileSearchIsProcessDone() )
#endif
    {
        iContainer->UpdateControl();

#if defined(SERIES60) || defined(SERIES80)
		iContainer->UpdateCba();
#endif
		iTimer->Cancel();
    } 
    else
		SelfComplete();
}

void COggFilesSearchAO::DoCancel()
{
	if (iTimer)
		iTimer->Cancel();
}

void COggFilesSearchAO::StartL()
{
	iTimer = CPeriodic::New(CActive::EPriorityStandard);
	iCallBack = new (ELeave) TCallBack(COggFilesSearchAO::CallBack, iContainer);
	iTimer->Start(TTimeIntervalMicroSeconds32(100000), TTimeIntervalMicroSeconds32(100000), *iCallBack);

	SelfComplete();
}

void COggFilesSearchAO::SelfComplete()
{
	TRequestStatus* status = &iStatus;
	User::RequestComplete(status, KErrNone);

	SetActive();
}

TInt COggFilesSearchAO::CallBack(TAny* aPtr)
{
	((COggFilesSearchContainer *) aPtr)->UpdateControl();
	return 1;
}

