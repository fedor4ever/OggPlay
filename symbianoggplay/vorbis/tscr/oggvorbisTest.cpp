#include <e32std.h>
#include <ivorbiscodec.h>
#include <ivorbisfile.h>
#include <stdio.h>
#include <reent.h>
#include "ActiveConsole.h"
#include "oggvorbistest.h"
#include <mda\common\audio.h>


CIvorbisTest * CIvorbisTest::NewL(ETestType aType)
    {
    CIvorbisTest* self = new (ELeave) CIvorbisTest;
    CleanupStack::PushL( self );
    self->ConstructL ( aType );
    CleanupStack::Pop( self );
    return self;
    }

void CIvorbisTest::ConstructL(ETestType aType)
    {
    iTestType = aType;
    if (aType == EStreamAudio)
        {
        iOutputStream =  CMdaAudioOutputStream::NewL(*this);
        }
    }

void CIvorbisTest::SetOutputFileL(const TDesC8 &aFileName)
    {
    iFileOut = fopen( reinterpret_cast <const char *> ( aFileName.Ptr() ),"w");
    }

void CIvorbisTest::OpenFileL(const TDesC8 &aFileName)
    {
    FILE * aFile;
    char * filename ;
    filename = (char *) aFileName.Ptr();
    aFile = fopen((char *) aFileName.Ptr() , "r+");
    
    if(ov_open(aFile, &iVf, NULL, 0) < 0) {
        fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
        User::Leave(KErrNotSupported);
        }
        /* Throw the comments plus a few lines about the bitstream we're
    decoding */
    
    char **ptr=ov_comment(&iVf,-1)->user_comments;
    vorbis_info *vi=ov_info(&iVf,-1);
    while(*ptr){
        fprintf(stderr,"%s\n",*ptr);
        ++ptr;
        }
    fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
    fprintf(stderr,"\nDecoded length: %ld samples\n",
        (long)ov_pcm_total(&iVf,-1));
    fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&iVf,-1)->vendor);
    
    switch(vi->rate)
        {
        case 8000:
            iRate = TMdaAudioDataSettings::ESampleRate8000Hz;
            break;
        case 11025:
            iRate = TMdaAudioDataSettings::ESampleRate11025Hz;
            break;
        case 16000:
            iRate = TMdaAudioDataSettings::ESampleRate16000Hz;
            break;
        case 22050:
            iRate = TMdaAudioDataSettings::ESampleRate22050Hz;
            break;
        case 32000:
            iRate = TMdaAudioDataSettings::ESampleRate32000Hz;
            break;
        case 44100:
            iRate = TMdaAudioDataSettings::ESampleRate44100Hz;
            break;
        case 48000:
            iRate = TMdaAudioDataSettings::ESampleRate48000Hz;
            break;
        default:
            User::Leave(KErrNotSupported);
        }
    
    switch(vi->channels)
        {
        case 1:
            iNbChannels =  TMdaAudioDataSettings::EChannelsMono;
            break;
        case 2:
            iNbChannels =  TMdaAudioDataSettings::EChannelsStereo;
            break;
        default:
            User::Leave(KErrNotSupported);
        }
    TMdaAudioDataSettings settings;
    settings.Query();
    fprintf(stderr,"defaults %x\n", settings.iCaps);
    
    if (iTestType == EStreamAudio)
        {
        // Initialize the Audio Stream
        settings.iSampleRate = iRate;
        settings.iChannels = iNbChannels;
        settings.iVolume = settings.iMaxVolume/2;
        fprintf(stderr,"volume:%i\n", settings.iVolume );
        iOutputStream->Open(&settings);
        }
    }


int CIvorbisTest::FillSampleBufferL()
    {

    // Fill one buffer of PCM samples

    int current_section;
    TDes8 * bufPtr ;

    // Swap buffers
    // So that there is always one buffer ready waiting to
    // be streamed.

    if (iCurrentBuffer == 0)
        {
        bufPtr = &iPcmout1;
        iCurrentBuffer = 1;
        }
    else
        {
        bufPtr = &iPcmout2;
        iCurrentBuffer = 0;
        }
    
    // Decode the ogg file
    long ret=ov_read(&iVf,(char *) bufPtr->Ptr(),bufPtr->MaxLength(),&current_section);
    
    if (ret == 0) 
        {
        /* EOF */
        EndOfFileL();
        } 
    else if (ret < 0) 
        {
        /* error in the stream.  Not a problem, just reporting it in
        case we (the app) cares.  In this case, we don't. */
        } 
    else
        {
        /* we don't bother dealing with sample rate changes, etc, but
        you'll have to*/
        bufPtr->SetLength(ret);
        BufferFullL(*bufPtr);
        }
    return ret;
    }


void CIvorbisTest::BufferFullL(TDes8 &aBuffer)
    {
    switch (iTestType)
        {
        case EStreamAudio:
            // Write the buffer to the audio stream
            iOutputStream->WriteL( aBuffer );
            break;
        case EWriteFile:
            fwrite(aBuffer.Ptr(),1,aBuffer.Length(),iFileOut);
            break;
        }
    }


void CIvorbisTest::EndOfFileL()
    {
    }


CIvorbisTest:: ~CIvorbisTest()
    {
    
    /* cleanup */
    ov_clear(&iVf);
    
    fprintf(stderr,"Done.\n");
    
    CloseSTDLIB();
    delete( iOutputStream );
    
    }


/////////////////////////////////////
// Functions used for Audio Streaming
/////////////////////////////////////


void CIvorbisTest::MaoscPlayComplete(TInt /*aError*/) 
    {
    CActiveScheduler::Stop();
    }


void CIvorbisTest::MaoscBufferCopied(TInt aError, const TDesC8& /*aBuffer*/) 
    {   
    if (aError)
        {
        aError = 0;
        }
    FillSampleBufferL();
    }


void CIvorbisTest::MaoscOpenComplete(TInt aError) 
    {
    if (aError)
        {
        fprintf(stderr,"Error: %i", aError);
        aError = 0;
        }
    fprintf(stderr,"MaxVol: %i\n", iOutputStream->MaxVolume());
    iOutputStream->SetVolume(iOutputStream->MaxVolume()/2);
    iOutputStream->SetAudioPropertiesL(iRate,iNbChannels);
    iOutputStream->SetPriority(EMdaPriorityMax, EMdaPriorityPreferenceQuality);
    FillSampleBufferL();  
    FillSampleBufferL(); 
    }


