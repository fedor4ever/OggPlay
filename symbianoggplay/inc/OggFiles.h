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

class COggPlayback;

class TOggFile : public CBase
{
private :
  TOggFile();

public:
  virtual ~TOggFile();

  static TOggFile* NewL();
  static TOggFile* NewL( const TDesC& aTitle, 
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
  TBool Write(TInt aLineNumber, HBufC* aBuf );

  HBufC* iTitle;
  HBufC* iAlbum;
  HBufC* iArtist;
  HBufC* iGenre;
  HBufC* iFileName;
  HBufC* iSubFolder;
  HBufC* iShortName;
  HBufC* iTrackNumber;
  HBufC* iTrackTitle;
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

class TOggFiles : public CBase, public MOggFilesSearchBackgroundProcess
{
 public:

  TOggFiles(COggPlayback* anOggPlayback);
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

  static void AppendLine(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aText, const TDesC& aFileName);
  static void AppendTitleAndArtist(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aTitle, const TDesC& aDelim, const TDesC& aArtist, const TDesC& aFileName);
  
  TInt SearchAllDrives(CEikDialog * aDialog, TInt aDialogID,RFs& session);
  TInt SearchSingleDrive(const TDesC& aDir, CEikDialog * aDialog, TInt aDialogID,RFs& session);
  
 public:
     // from MOggFilesSearchBackgroundProcess
     void  FileSearchStepL();
     TBool FileSearchIsProcessDone() const ;
     void  FileSearchProcessFinished();
     void  FileSearchDialogDismissedL(TInt /*aButtonId*/);
     TInt  FileSearchCycleError(TInt aError);
     void  FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles);

 protected:

  void AddDirectory(const TDesC& aDir, RFs& session);
  void AddFile(const TDesC& aFile);
  
  TInt AddDirectoryStart(const TDesC& aDir, RFs& session);
  void AddDirectoryStop();
  void ClearFiles();

  CArrayPtrFlat<TOggFile>* iFiles; 
  COggPlayback*            iOggPlayback;
  TOggKey                  iOggKeyTitles;
  TOggKey                  iOggKeyAlbums;
  TOggKey                  iOggKeyArtists;
  TOggKey                  iOggKeyGenres;
  TOggKey                  iOggKeySubFolders;
  TOggKey                  iOggKeyFileNames;
  TOggKey                  iOggKeyTrackTitle;
  TInt                     iVersion;

  // Following members are only valid during the directory search:
  CDirScan* iDs;
  TBool     iDirScanFinished;
  TDesC   * iDirScanDir;
  RFs     * iDirScanSession;
  TInt      iNbDirScanned;
  TInt      iNbFilesFound;
  TInt      iCurrentIndexInDirectory;
  TInt      iCurrentDriveIndex;
  CDir    * iDirectory;
  CDesC16ArrayFlat * iPathArray;

};

#endif
