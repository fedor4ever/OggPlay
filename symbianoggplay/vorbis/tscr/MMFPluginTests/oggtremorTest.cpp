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
        iPlayer->Open(iFilename);
        iConsole->Printf(_L("Playing %S!\n %S\n"), &iFilename,  &iPlayer->Title());
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
        iPlayer->SetVolume(iVolume);
        return(ETrue);
        break;
        }
    case '5' :
    case 'j' :
        iConsole->Printf(_L("Volume Down!\n"));
        iVolume--;
        iPlayer->SetVolume(iVolume);
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

void CIvorbisTest::NotifyPlayComplete() 
{
    iConsole->Printf(_L("Notify Play complete"));
}
void CIvorbisTest::NotifyUpdate() 
{
    iConsole->Printf(_L("Notify Update"));
}
  
void CIvorbisTest::NotifyPlayInterrupted()
{
    iConsole->Printf(_L("Notify Play Interrupted"));
}


#ifdef MMF_AVAILABLE
////////////////////////////////////////////////////
// Interface using the MMF
////////////////////////////////////////////////////

void CIvorbisTest::ConstructL(CConsoleBase *aConsole)
    {
    iConsole = aConsole;
    iPlayer = new (ELeave) COggPluginAdaptor( NULL, this );
    iPlayer->ConstructL();
    RDebug::Print(_L("Leaving constructl"));
    iFilename = _L("C:\\test.ogg");
    }
CIvorbisTest::~CIvorbisTest()
{
    delete (iPlayer);
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
    User::LeaveIfError ( iLibrary.Load(_L("OGGPLUGINDECODER.DLL"), uidType) );

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

#endif


