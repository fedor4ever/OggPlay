#include <e32std.h>
#include <mad.h>
#include <stdio.h>
#include <reent.h>
#include "ActiveConsole.h"
//#include "madtremor.h"
#include "madtest.h"
#include <mda\common\audio.h>

#include <limits.h>
#include <string.h> // memmove

#define MadErrorString(x) mad_stream_errorstr(x)

CMadtest * CMadtest::NewL()
    {
    CMadtest* self = new (ELeave) CMadtest;
    CleanupStack::PushL( self );
    self->ConstructL();
    CleanupStack::Pop( self );
    return self;
    }

void CMadtest::ConstructL()
{
  iMt=new(ELeave)CMadDecoder;
  OutputBuffer = new(ELeave) TBuf8<OUTPUT_BUFFER_SIZE>;
  OutputBuffer->SetMax();

  _LIT(KText,"Hello World!");

//  TBufC<16> buf1(KText);
//  TPtr ptr = buf1.Des();

//  TPtr iPtr=iTitle.Des();
  iMt->ParseTags(iTitle);

}




/****************************************************************************
 * Main decoding loop. This is where mad is used.							*
 ****************************************************************************/
int CMadtest::MpegAudioDecoder(void)
{
  iMt->Clear();
  iMt->Open(iInputFp);

	/* This is the decoding loop. */
	TInt ret;
  do
	{
    ret=iMt->Read(*OutputBuffer,0);
   	if(ret==0) {
      fprintf(stderr,"end of input stream\n");
      break;
    }
    if(ret<0) {
      fprintf(stderr,"decoding error\n");
      //FIXME!
//      if(!MAD_RECOVERABLE(iMf->iStream.error)) {
//					fprintf(stderr,"unrecoverable frame level error (%s).\n",
//							MadErrorString(&iMf->iStream));
//      }
      break;
    }
		if(fwrite(OutputBuffer,1,ret,iOutputFp)!=ret)
				{
					fprintf(stderr,"PCM write error \n");
//					iMf->iStatus=2;
					break;
				}

    fprintf(stderr,"%ul milliseconds\n",iMt->Position());
	}while(1);


  iMt->Close();
  fclose(iOutputFp);

	/* Accounting report if no error occurred. */
	if(1) //FIXME !iMt->iStatus)
	{
		char	Buffer[80];

		/* The duration timer is converted to a human readable string
		 * with the versatile, but still constrained mad_timer_string()
		 * function, in a fashion not unlike strftime(). The main
		 * difference is that the timer is broken into several
		 * values according some of it's arguments. The units and
		 * fracunits arguments specify the intended conversion to be
		 * executed.
		 *
		 * The conversion unit (MAD_UNIT_MINUTES in our example) also
		 * specify the order and kind of conversion specifications
		 * that can be used in the format string.
		 *
		 * It is best to examine libmad's timer.c source-code for details
		 * of the available units, fraction of units, their meanings,
		 * the format arguments, etc.
		 */
		mad_timer_string(iMt->iTimer,Buffer,"%lu:%02lu.%03u",
						 MAD_UNITS_MINUTES,MAD_UNITS_MILLISECONDS,0);
		fprintf(stderr,"%lu frames decoded (%s).\n",
				iMt->iFrameCount,Buffer);
	}

	return(1); 
}

void CMadtest::SetOutputFileL(const TDesC8 &aFileName)
    {
    iOutputFp = fopen( reinterpret_cast <const char *> ( aFileName.Ptr() ),"w");
    }

void CMadtest::SetInputFileL(const TDesC8 &aFileName)
    {
    iInputFp = fopen((char *) aFileName.Ptr() , "r+");
    }



CMadtest:: ~CMadtest()
    {
    
    /* cleanup */
   
    fprintf(stderr,"Done.\n");
    
    CloseSTDLIB();
    
    }


























































