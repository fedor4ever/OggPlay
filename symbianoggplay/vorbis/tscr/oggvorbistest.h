#include <e32std.h>
#include <e32base.h>
#include <mdaaudiooutputstream.h>
#include <ivorbiscodec.h>
#include <ivorbisfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <MdaAudioSampleEditor.h>

class CIvorbisTest: public CBase, public MMdaAudioOutputStreamCallback
    {
    public:
        typedef enum 
            { EWriteFile,
            EStreamAudio } ETestType;
        
        static CIvorbisTest * NewL(ETestType aType);
        void OpenFileL(const TDesC8 &aFileName);
        void EndOfFileL();
        void BufferFullL(TDes8 & aBuffer);
        int FillSampleBufferL();
        void SetOutputFileL(const TDesC8 & aFileName);

        // Destruction
        ~CIvorbisTest();

        void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);

    private:
        void MaoscPlayComplete(TInt aError) ;
        void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer) ;
        void MaoscOpenComplete(TInt aError) ;
        
        
        void ConstructL(ETestType aType);
        
        CMdaAudioOutputStream *iOutputStream;
        
        OggVorbis_File iVf;
        TBuf8<4096> iPcmout1;    
        TBuf8<4096> iPcmout2;
        TInt iCurrentBuffer;
        
        TInt iRate, iNbChannels;
        ETestType iTestType;

        FILE * iFileOut;
        CMdaAudioRecorderUtility *iPlayer;
        
        TMdaAudioDataSettings audioSets;

    };
