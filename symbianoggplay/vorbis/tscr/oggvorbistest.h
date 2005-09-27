#include <e32std.h>
#include <e32base.h>
#include <f32file.h>
#include <mdaaudiooutputstream.h>
#include <ivorbiscodec.h>
#include <ivorbisfile.h>
#include <MdaAudioSampleEditor.h>

class CIvorbisTest: public CBase, public MMdaAudioOutputStreamCallback
    {
    public:
        typedef enum 
            { EWriteFile,
            EStreamAudio } ETestType;
        
        static CIvorbisTest * NewL(ETestType aType);
        void OpenFileL(const TDesC& aFileName);
        void EndOfFileL();
        void BufferFullL(TDes8& aBuffer);
        int FillSampleBufferL();
        void SetOutputFileL(const TDesC& aFileName);

        // Destruction
        ~CIvorbisTest();

        void MoscoStateChangeEvent(CBase* aObject, TInt aPreviousState, TInt aCurrentState, TInt aErrorCode);

    private:
        void ConstructL(ETestType aType);

        void MaoscPlayComplete(TInt aError) ;
        void MaoscBufferCopied(TInt aError, const TDesC8& aBuffer) ;
        void MaoscOpenComplete(TInt aError) ;
        
	private:
		CMdaAudioOutputStream *iOutputStream;
        
        OggVorbis_File iVf;
        TBuf8<4096> iPcmout1;    
        TBuf8<4096> iPcmout2;
        TInt iCurrentBuffer;
        
        TInt iRate, iNbChannels;
        ETestType iTestType;

        RFile* iFileOut;
        CMdaAudioRecorderUtility *iPlayer;

        TMdaAudioDataSettings audioSets;
		RFs iFs;
    };
