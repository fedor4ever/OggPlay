
#include <OggOs.h>
#include <e32std.h>
#include <e32base.h>
#include <ivorbiscodec.h>
#include <ivorbisfile.h>

#include <MdaAudioSamplePlayer.h>
#ifndef MMF_AVAILABLE
#include <e32uid.h>
#include <OggPlayPlugin.h>
#endif


class CIvorbisTest: public CBase, public MMdaAudioPlayerCallback, public MKeyPressObserver 
    {
    public:
        
        static CIvorbisTest * NewL(CConsoleBase *aConsole);
        void OpenFileL(const TDesC8 &aFileName);

        // Destruction
        ~CIvorbisTest();

    private:
        TInt ProcessKeyPress(TChar aChar) ;
        void MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration);
        void MapcPlayComplete(TInt aError);
        
        void ConstructL(CConsoleBase *aConsole);
     
        CConsoleBase *iConsole;
#ifdef MMF_AVAILABLE
        CMdaAudioPlayerUtility *iPlayer;
#else
        CPseudoMMFController * iPlayer;
        // Use RLibrary object to interface to the DLL
        RLibrary iLibrary;
#endif

    };
