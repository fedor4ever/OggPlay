#include <e32std.h>
#include <e32base.h>
#include <mdaaudiooutputstream.h>
#include <stdio.h>
#include <stdlib.h>
#include <MdaAudioSampleEditor.h>
#include "MadDecoder.h"
//#include "bstdfile.h"


//#define OUTPUT_BUFFER_SIZE	 1152*8*2 /* Must be an integer multiple of 4. */
#define OUTPUT_BUFFER_SIZE	 9210


class CMadtest: public CBase
{
public:
  static CMadtest* NewL();
  void SetOutputFileL(const TDesC8 & aFileName);
  void SetInputFileL(const TDesC8 & aFileName);

  int MpegAudioDecoder(void);


  ~CMadtest();

private:
  void ConstructL();
        

  FILE *iInputFp;
  FILE *iOutputFp;
//	unsigned char OutputBuffer[OUTPUT_BUFFER_SIZE];
  TDes8* OutputBuffer;


  CMadDecoder* iMt;

  TBuf<256>                iTitle;
  TBuf<256>                iAlbum;
  TBuf<256>                iArtist;
  TBuf<256>                iGenre;
  TBuf<256>                iTrackNumber;
  TInt64                   iTime;
  TInt                     iRate;
  TInt                     iChannels;
  TInt                     iFileSize;
  TInt64                   iBitRate;
  TFileName                iFileName;
};

