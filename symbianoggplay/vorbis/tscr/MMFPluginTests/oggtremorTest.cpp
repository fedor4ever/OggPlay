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
        iVolume+=10;
        iPlayer->SetVolume(iVolume);
        return(ETrue);
        break;
        }
    case '5' :
    case 'j' :
        iConsole->Printf(_L("Volume Down!\n"));
        iVolume-=10;
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




////////////////////////////////////////////////////////////////
//
// CAbsPlayback
//
////////////////////////////////////////////////////////////////

// Just the constructor is defined in this class, everything else is 
// virtual

CAbsPlayback::CAbsPlayback(MPlaybackObserver* anObserver) :
  iState(CAbsPlayback::EClosed),
  iObserver(anObserver),
  
  iBitRate(0),
  iChannels(2),
  iFileSize(0),
  iRate(44100),
  iTime(0),
  iAlbum(),
  iArtist(),
  iGenre(),
  iTitle(),
  iTrackNumber()
{
}
