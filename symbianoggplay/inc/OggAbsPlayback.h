#include "OggOs.h"
#include "OggRateConvert.h"

#ifndef OGGABSPLAYBACK_H
#define OGGABSPLAYBACK_H


const TInt KMaxVolume = 100;
const TInt KStepVolume = 10;

const TInt KErrOggFileNotFound = -101;
const TInt KFreqArrayLength = 100; // Length of the memory of the previous freqs bin
const TInt KNumberOfFreqBins = 16; // This shouldn't be changed without making same changes
                                   // to vorbis library !

//
// MPlaybackObserver class
//------------------------------------------------------
class MPlaybackObserver 
{
 public:
  virtual void NotifyPlayComplete() = 0;
  virtual void NotifyUpdate() = 0;
  virtual void NotifyPlayInterrupted() = 0;
};

class CAbsPlayback : public CBase {

 public:

  enum TState {
    EClosed,
    EFirstOpen,
    EOpen,
    EPlaying,
    EPaused
  };

  CAbsPlayback(MPlaybackObserver* anObserver=NULL);

  // Here is a bunch of abstract methods which need to implemented for
  // each audio format:
  ////////////////////////////////////////////////////////////////

  virtual void   ConstructL() = 0;
  // Open a file and retrieve information (meta data, file size etc.) without playing it:
  virtual TInt   Info(const TDesC& aFileName, TBool silent= EFalse) = 0;

  // Open a file and prepare playback:
  virtual TInt   Open(const TDesC& aFileName) = 0;

  virtual void   Pause() = 0;
  virtual void   Play() = 0;
  virtual void   Stop() = 0;

  virtual void   SetVolume(TInt aVol) = 0;
  virtual void   SetPosition(TInt64 aPos) = 0;

  virtual TInt64 Position() = 0;
  virtual TInt64 Time() = 0;
  virtual TInt   Volume() = 0;
  
#ifdef MDCT_FREQ_ANALYSER
  virtual const TInt32 * GetFrequencyBins(TTime aTime) = 0;
#else
  virtual const void* GetDataChunk() = 0;
#endif

   // Implemented Helpers 
   ////////////////////////////////////////////////////////////////

   virtual TState State()                 { return ( iState );             };
   
   // Information about the playback file 
   virtual TInt   BitRate()               { return ( iBitRate.GetTInt() ); };
   virtual TInt   Channels()              { return ( iChannels );          };
   virtual TInt   FileSize()              { return ( iFileSize );          };
   virtual TInt   Rate()                  { return ( iRate );              };

   virtual const TDesC&     Album()       { return ( iAlbum );             };
   virtual const TDesC&     Artist()      { return ( iArtist );            };
   virtual const TFileName& FileName()    { return ( iFileName );          };
   virtual const TDesC&     Genre()       { return ( iGenre );             };
   virtual const TDesC&     Title()       { return ( iTitle );             };
   virtual const TDesC&     TrackNumber() { return ( iTrackNumber );       };

   virtual void   ClearComments()         { iArtist.SetLength(0);          \
                                            iTitle.SetLength(0);           \
                                            iAlbum.SetLength(0);           \
                                            iGenre.SetLength(0);           \
                                            iTrackNumber.SetLength(0);     \
                                            iFileName.SetLength(0);        };
   virtual void   SetObserver( MPlaybackObserver* anObserver )             \
                                          { iObserver = anObserver;        };
   virtual void   SetVolumeGain(TGainType /*aGain*/)
                                          { /* No gain by default */    };
                                          
 protected:

   TState                   iState;
   MPlaybackObserver*       iObserver;

   // File properties and tag information   
   //-------------------------------------
   TInt64                   iBitRate;
   TInt                     iChannels;
   TInt                     iFileSize;
   TInt                     iRate;
   TInt64                   iTime;
  
   enum { KMaxStringLength = 256 };
   TBuf<KMaxStringLength>   iAlbum;
   TBuf<KMaxStringLength>   iArtist;
   TFileName                iFileName;
   TBuf<KMaxStringLength>   iGenre;
   TBuf<KMaxStringLength>   iTitle;
   TBuf<KMaxStringLength>   iTrackNumber;
};

#endif