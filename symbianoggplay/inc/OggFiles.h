/*
 *  Copyright (c) 2003 L. H. Wilden.
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

#ifndef _OggFiles_h
#define _OggFiles_h

#include "e32base.h"
#include "gulutil.h"

#include "OggPlay.h"
#include "OggFilesSearchDialogs.h"
#include <eikdialg.h>

const TInt KTFileTextBufferMaxSize = 0x100;
const TInt KMaxFileLength = 0x200;
const TInt KDiskWriteBufferSize = 20000;
const TInt KNofOggDescriptors = 9;         /** Nof descriptor fields in an internal ogg store */

class COggPlayback;
class TOggFiles;
class TOggFile : public CBase
{
private :
  TOggFile();

public:
  virtual ~TOggFile();

  static TOggFile* NewL();
  static TOggFile* NewL(TInt aAbsoluteIndex,
       const TDesC& aTitle, 
	   const TDesC& anAlbum,
	   const TDesC& anArtist,
	   const TDesC& aGenre,
	   const TDesC& aSubFolder,
	   const TDesC& aFileName,
	   const TDesC& aShortName,
	   const TDesC& aTrackNumber);

  void SetText(HBufC* & aBuffer, const TDesC& aText);
  void SetTrackTitle();
  void SetTextFromFileL(TFileText& aTf, HBufC* & aBuffer);

  void ReadL(TFileText& tf, TInt aVersion);
  //TBool Write(TInt aLineNumber, TFileText& tf);
  TInt Write(TInt aLineNumber, HBufC* aBuf );

  HBufC* iTitle;
  HBufC* iAlbum;
  HBufC* iArtist;
  HBufC* iGenre;
  HBufC* iFileName;
  HBufC* iSubFolder;
  HBufC* iShortName;
  HBufC* iTrackNumber;
  HBufC* iTrackTitle;
  TInt iAbsoluteIndex;
};

class TOggPlayList : public CBase
{
private :
  TOggPlayList();

public:
  ~TOggPlayList();

  static TOggPlayList* NewL();
  static TOggPlayList* NewL(TInt aAbsoluteIndex, const TDesC& aSubFolder, const TDesC& aFileName, const TDesC& aShortName);

  void ReadL(TFileText& tf);
  TInt Write(TInt aLineNumber, HBufC* aBuf );

  void SetTextFromFileL(TFileText& aTf, HBufC* & aBuffer);

  TInt NumEntries();
  void ScanPlayListL(RFs& aFs, TOggFiles* aFiles);
  TOggFile* operator[] (TInt aIndex);

public:
  HBufC* iFileName;
  HBufC* iSubFolder;
  HBufC* iShortName;
  TInt iAbsoluteIndex;

  RPointerArray<TOggFile> iPlayListEntries;
};

class TOggKey : public TKeyArrayFix
{
 public:


  TOggKey(TInt anOrder);
  virtual ~TOggKey();

  void SetFiles(CArrayPtrFlat<TOggFile>* theFiles);

  virtual TAny* At(TInt anIndex) const;

 protected:

  CArrayPtrFlat<TOggFile>* iFiles;
  TInt                     iOrder;
  TOggFile*                iSample;
};

class TOggKeyNumeric : public TKeyArrayFix
{
  public:
  TOggKeyNumeric();

  virtual ~TOggKeyNumeric();

  void SetFiles(CArrayPtrFlat<TOggFile>* theFiles);

  virtual TAny* At(TInt anIndex) const;

 protected:
  TInt iInteger;
  CArrayPtrFlat<TOggFile>* iFiles;
};

class TOggFiles : public CBase, public MOggFilesSearchBackgroundProcess
{
 public:

  TOggFiles(CAbsPlayback* anOggPlayback);
  virtual ~TOggFiles();

  void  CreateDb(RFs& session);
  TBool  CreateDbWithSingleFile(const TDesC& aFile);
  TBool ReadDb(const TFileName& aFileName, RFs& session);
  void  WriteDbL(const TFileName& aFileName, RFs& session);

  void FillTitles(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TDesC& aGenre, const TDesC& aSubFolder);
  void FillAlbums(CDesCArray& arr, const TDesC& anArtist, const TFileName& aSubFolder);
  void FillArtists(CDesCArray& arr, const TFileName& aSubFolder);
  void FillGenres(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TFileName& aSubFolder);
  void FillSubFolders(CDesCArray& arr);
  void FillFileNames(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TDesC& aGenre, const TFileName& aSubFolder);

  void FillPlayLists(CDesCArray& arr);
  void FillPlayList(CDesCArray& arr, const TDesC& aSelection);

  TDesC & FindFromIndex(TInt anIndex);
  TOggFile* FindFromFileNameL(TFileName& aFileName);

  static void AppendLine(CDesCArray& arr, COggListBox::TItemTypes aType, const TDesC& aText, const TInt anAbsoluteIndex);
  static void AppendLine(CDesCArray& arr, COggListBox::TItemTypes aType, const TDesC& aText, const TDesC& anInternalText);
 
  static void AppendTitleAndArtist(CDesCArray& arr, COggListBox::TItemTypes aType, const TDesC& aTitle, const TDesC& aDelim, const TDesC& aArtist, const TInt anAbsoluteIndex);
  
  TInt SearchAllDrives(CEikDialog * aDialog, TInt aDialogID,RFs& session);
  TInt SearchSingleDrive(const TDesC& aDir, CEikDialog * aDialog, TInt aDialogID,RFs& session);
  
 public:
     // from MOggFilesSearchBackgroundProcess
     void  FileSearchStepL();
     TBool FileSearchIsProcessDone() const ;
     void  FileSearchProcessFinished();
     void  FileSearchDialogDismissedL(TInt /*aButtonId*/);
     TInt  FileSearchCycleError(TInt aError);

#ifdef PLAYLIST_SUPPORT
	 void  FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles, TInt &aNbPlayLists);
	 void  ScanNextPlayList();
	 TBool  PlayListScanIsProcessDone() const;
#else
	 void  FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles);
#endif

 protected:

  void AddDirectory(const TDesC& aDir, RFs& session);
  void AddFile(const TDesC& aFile);
  
  TInt AddDirectoryStart(const TDesC& aDir, RFs& session);
  void AddDirectoryStop();
  void ClearFiles();

  TBool IsSupportedAudioFile(TParsePtrC& p);
  TBool IsPlayListFile(TParsePtrC& p);

  TBool NextDirectory();
  TBool AddNextFileL();
  
#ifdef PLUGIN_SYSTEM
  CDesCArrayFlat * iSupportedExtensionList;
#endif

  CArrayPtrFlat<TOggFile>* iFiles; 
  CArrayPtrFlat<TOggPlayList>* iPlayLists; 
  CAbsPlayback*            iOggPlayback;
  TOggKey                  iOggKeyTitles;
  TOggKey                  iOggKeyAlbums;
  TOggKey                  iOggKeyArtists;
  TOggKey                  iOggKeyGenres;
  TOggKey                  iOggKeySubFolders;
  TOggKey                  iOggKeyFileNames;
  TOggKey                  iOggKeyTrackTitle;
  TOggKeyNumeric           iOggKeyAbsoluteIndex;
  TInt                     iVersion;

  // Following members are only valid during the directory search:
  CDirScan* iDs;
  TBool     iDirScanFinished;
  TDesC   * iDirScanDir;
  RFs     * iDirScanSession;
  TInt      iNbDirScanned;
  TInt      iNbFilesFound;
  TInt      iNbPlayListsFound;
  TInt      iCurrentIndexInDirectory;
  TInt      iCurrentDriveIndex;
  CDir    * iDirectory;
  CDesC16ArrayFlat * iPathArray;

  TBuf<KMaxFileLength> iFullname;
  TBuf<KMaxFileLength> iPath;

  TBool iPlayListScanFinished;
};

#endif
