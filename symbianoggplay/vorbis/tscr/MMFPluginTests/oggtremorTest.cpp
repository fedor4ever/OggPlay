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
    case 92 :
        iConsole->Printf(_L("Playing %S!"), &iFilename);
        iPlayer->OpenFileL(iFilename);
        iPlayer->Play();
        return(ETrue);
        break;
    case '2' :
    case 'a':
        iConsole->Printf(_L("Stopping!\n"));
        iPlayer->Stop();
        return(ETrue);
        break;    
    case '3' :
    case 'd':
        iConsole->Printf(_L("Pause!\n"));
        iPlayer->Pause();
        return(ETrue);
        break;
    case '4' :
    case 'g' :
        {
        iConsole->Printf(_L("Volume Up!\n"));
        iVolume++;
        TInt aVol = iVolume/10.0 * iPlayer->MaxVolume();
        iPlayer->SetVolume(iVolume/10.0 * iPlayer->MaxVolume());
        return(ETrue);
        break;
        }
    case '5' :
    case 'j' :
        iConsole->Printf(_L("Volume Down!\n"));
        iVolume--;
        iPlayer->SetVolume(iVolume/10.0 * iPlayer->MaxVolume());
        return(ETrue);
        break;
    case '6':
    case 'm':
        switch(iFilenameIdx)
        {
        case 0:
            iFilename = _L("C:\\test.ogg");
            break;

        case 1:
            iFilename = _L("C:\\test.mp3");
            break;
            
        case 2:
            iFilename = _L("C:\\test.aac");
            iFilenameIdx = -1;
            break;
        }
        iFilenameIdx++;

        iConsole->Printf(_L("New FileName %S"), &iFilename);
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
    iFilename = _L("C:\\test.ogg");
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


