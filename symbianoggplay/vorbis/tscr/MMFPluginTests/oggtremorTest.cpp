#include <ivorbiscodec.h>
#include <ivorbisfile.h>
#include <stdio.h>
#include <reent.h>
#include "ActiveConsole.h"
#include "oggtremortest.h"
#include <mda\common\audio.h>
#ifndef MMF_AVAILABLE
#include <e32uid.h>
#include <e32svr.h>
#endif

CIvorbisTest * CIvorbisTest::NewL(CConsoleBase *aConsole)
    {
    CIvorbisTest* self = new (ELeave) CIvorbisTest;
    CleanupStack::PushL( self );
    self->ConstructL (aConsole );
    CleanupStack::Pop( self );
    return self;
    }

TInt CIvorbisTest::ProcessKeyPress(TChar aChar) 
{
    switch(aChar)
    {
    case '1' :
        iConsole->Printf(_L("Playing!"));
        iPlayer->OpenFileL(_L("C:\\test.ogg"));
        iPlayer->Play();
        return(ETrue);
        break;
    case '2' :
        iConsole->Printf(_L("Stopping!"));
        iPlayer->Stop();
        return(ETrue);
        break;
    default:
        return(EFalse);
    }
}


#ifdef MMF_AVAILABLE
////////////////////////////////////////////////////
// Interface using the MMF
////////////////////////////////////////////////////

void CIvorbisTest::ConstructL(CConsoleBase *aConsole)
    {
    iConsole = aConsole;
    iPlayer = CMdaAudioPlayerUtility::NewL(
        *this);
    RDebug::Print(_L("Leaving constructl"));
    }
CIvorbisTest::~CIvorbisTest()
{
    delete (iPlayer);
}

void CIvorbisTest::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
{
    RDebug::Print(_L("MapcInitComplete"));
}

void CIvorbisTest::MapcPlayComplete(TInt aError)
{
    RDebug::Print(_L("PlayComplete"));
}
#else


////////////////////////////////////////////////////
// Interface when MMF is not available
////////////////////////////////////////////////////


void CIvorbisTest::ConstructL(CConsoleBase *aConsole)
    {
    iConsole = aConsole;
    // Load the plugin.
    // Dynamically load the DLL
    const TUid KOggDecoderLibraryUid={0x101FD21D};
    TUidType uidType(KDynamicLibraryUid,KOggDecoderLibraryUid);
    User::LeaveIfError ( iLibrary.Load(_L("OGGTREMORCONTROLLER.DLL"), uidType) );

    // Function at ordinal 1 creates new C
    TLibraryFunction entry=iLibrary.Lookup(1);
    // Call the function to create new CMessenger
    iPlayer = (CPseudoMMFController*) entry();
    iPlayer->SetObserver(*this);
    iPlayer->OpenFile(_L("C:\\test.ogg"));
}

CIvorbisTest::~CIvorbisTest()
{
     delete (iPlayer);
    // Finished with the DLL
    //
    iLibrary.Close();
}

void CIvorbisTest::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
{
    
    RDebug::Print(_L("MapcInitComplete"));
    
}

void CIvorbisTest::MapcPlayComplete(TInt aError)
{
    RDebug::Print(_L("PlayComplete"));
}
#endif


