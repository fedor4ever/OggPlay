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


#ifdef MMF_AVAILABLE
////////////////////////////////////////////////////
// Interface using the MMF
////////////////////////////////////////////////////
CIvorbisTest * CIvorbisTest::NewL()
    {
    CIvorbisTest* self = new (ELeave) CIvorbisTest;
    CleanupStack::PushL( self );
    self->ConstructL ( );
    CleanupStack::Pop( self );
    return self;
    }

void CIvorbisTest::ConstructL()
    {

    iPlayer = CMdaAudioPlayerUtility::NewFilePlayerL(
        _L("C:\\test.ogg"),
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
    iPlayer->Play();
}

void CIvorbisTest::MapcPlayComplete(TInt aError)
{
    RDebug::Print(_L("PlayComplete"));
    
    CActiveScheduler::Stop();
}
#else


////////////////////////////////////////////////////
// Interface when MMF is not available
////////////////////////////////////////////////////

CIvorbisTest * CIvorbisTest::NewL()
    {
    CIvorbisTest* self = new (ELeave) CIvorbisTest;
    CleanupStack::PushL( self );
    self->ConstructL ( );
    CleanupStack::Pop( self );
    return self;
    }

void CIvorbisTest::ConstructL()
{
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
    iPlayer->Play();
    
}

void CIvorbisTest::MapcPlayComplete(TInt aError)
{
    RDebug::Print(_L("PlayComplete"));
    CActiveScheduler::Stop(); 
}
#endif


