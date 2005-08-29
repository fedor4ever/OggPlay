/*
*  Copyright (c) 2004 OggPlay Team
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/
#include "OggOs.h"
#include <badesca.h>
#include "OggRateConvert.h"


#ifndef OGGABSPLAYBACK_H
#define OGGABSPLAYBACK_H


const TInt KMaxVolume = 100;
const TInt KStepVolume = 10;

const TInt KErrOggFileNotFound = -101;
const TInt KFreqArrayLength = 100; // Length of the memory of the previous freqs bin
const TInt KNumberOfFreqBins = 16; // This shouldn't be changed without making same changes
                                   // to vorbis library !


#ifdef PLUGIN_SYSTEM

#include <MdaAudioSampleEditor.h> // For CMMFFormatImplementationInformation
//
// CPluginInfo class
//------------------------------------------------------
class CPluginInfo : public CBase
{
private :
  CPluginInfo();
  
public:
    ~CPluginInfo();
    static CPluginInfo* NewL( const TDesC& anExtension, 
        const CMMFFormatImplementationInformation &aFormatInfo,
        const TUid aControllerUid);
    void ConstructL ( const TDesC& anExtension, 
        const CMMFFormatImplementationInformation &aFormatInfo,
        const TUid aControllerUid);
    
    HBufC* iExtension;
    HBufC* iName;
    HBufC* iSupplier;
    TInt iVersion;
    TUid iFormatUid;
    TUid iControllerUid;
};

//
// CPluginSupportedList class
//------------------------------------------------------
class CPluginSupportedList: public CBase
{
    public:

        CPluginSupportedList();
        void ConstructL();
        ~CPluginSupportedList();
        
        void AddPluginL(const TDesC &extension,
        	const CMMFFormatImplementationInformation &aFormatInfo,
            const TUid aControllerUid);
            
        void SelectPluginL(const TDesC &extension, TUid aUid);
        CPluginInfo * GetSelectedPluginInfo(const TDesC &extension);
        CArrayPtrFlat <CPluginInfo> * GetPluginInfoList(const TDesC &extension);
		
		void AddExtension(const TDesC &extension);
		CDesCArrayFlat * SupportedExtensions();
		
    private:
    
        TInt FindListL(const TDesC &anExtension);
        
    
        typedef struct
        {
        	TBuf<10> extension;
        	CPluginInfo *selectedPlugin;
        	CArrayPtrFlat <CPluginInfo>* listPluginInfos; 
        } TExtensionList;
        
        CArrayPtrFlat <TExtensionList> * iPlugins; // Plugins that have been selected

};


#endif

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
  virtual void   Resume() = 0;
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

#ifdef PLUGIN_SYSTEM
  virtual CPluginSupportedList & GetPluginListL() = 0;
#endif
   // Implemented Helpers 
   ////////////////////////////////////////////////////////////////

   virtual TState State()                 { return ( iState );             };
   
   // Information about the playback file 
   virtual TInt   BitRate()               { return ( iBitRate ); };
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
   TInt                     iBitRate;
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
 #ifdef PLUGIN_SYSTEM
   CPluginSupportedList iPluginSupportedList;
 #endif
};

#endif