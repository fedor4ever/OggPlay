
#include <OggOs.h>
#include <e32std.h>
#include <e32base.h>
#include <ivorbiscodec.h>
#include <ivorbisfile.h>

#include <MdaAudioSamplePlayer.h>
#include "OggPluginAdaptor.h"

#ifndef MMF_AVAILABLE
#include <e32uid.h>
#include <OggPlayPlugin.h>
#endif



class CIvorbisTest: public CBase, public MKeyPressObserver, public MPlaybackObserver
    {
    public:
        
        static CIvorbisTest * NewL(CConsoleBase *aConsole);

        // Destruction
        ~CIvorbisTest();

        virtual void NotifyPlayComplete();
        virtual void NotifyUpdate();
        virtual void NotifyPlayInterrupted();
    private:
        TInt ProcessKeyPress(TChar aChar) ;
        
        void ConstructL(CConsoleBase *aConsole);
     
        CConsoleBase *iConsole;
        TInt iVolume;
        TBuf<100> iFilename;
        TInt iFilenameIdx;
        COggPluginAdaptor * iPlayer;

    };
