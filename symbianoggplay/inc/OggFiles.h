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

#include <e32base.h>
#include <eikdialg.h>

#include "OggPlay.h"
#include "OggFilesSearchDialogs.h"

const TInt KTFileTextBufferMaxSize = 0x100;
const TInt KDiskWriteBufferSize = 20000;
const TInt KNofOggDescriptors = 9;         /** Nof descriptor fields in an internal ogg store */

class COggPlayback;
class TOggFiles;
class TOggFile : public CBase
	{
public:
	enum TFileTypes { EFile, EPlayList };

public:
	~TOggFile();

	static TOggFile* NewL();
	static TOggFile* NewL(TFileText& tf, TInt aVersion);
	static TOggFile* NewL(TInt aAbsoluteIndex, const TDesC& aTitle, const TDesC& anAlbum, const TDesC& anArtist,
	const TDesC& aGenre, const TDesC& aSubFolder, const TDesC& aFileName, const TDesC& aShortName, const TDesC& aTrackNumber);

	void SetTextL(HBufC* & aBuffer, const TDesC& aText);
	void SetTrackTitleL();
	void SetTextFromFileL(TFileText& aTf, HBufC* & aBuffer);

	void ReadL(TFileText& tf, TInt aVersion);
	TInt Write(TInt aLineNumber, HBufC* aBuf);

	virtual TFileTypes FileType() const;

protected:
	TOggFile();

public:
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
	TInt iPlayingIndex;
	};

class TOggPlayList : public TOggFile
	{
public:
	static TOggPlayList* NewL(TFileText& tf);
	static TOggPlayList* NewL(TInt aAbsoluteIndex, const TDesC& aSubFolder, const TDesC& aFileName, const TDesC& aShortName);
	~TOggPlayList();

	void ReadL(TFileText& tf);
	TInt Write(TInt aLineNumber, HBufC* aBuf );

	void SetTextFromFileL(TFileText& aTf, HBufC* & aBuffer);

	TInt Count();
	void ScanPlayListL(RFs& aFs, TOggFiles* aFiles);
	TOggFile* operator[] (TInt aIndex);

	// From TOggFile
	TFileTypes FileType() const;

private:
	TOggPlayList();

public:
	RPointerArray<TOggFile> iPlayListEntries;
	RPointerArray<TOggFile> iUrls;	
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
	TInt iOrder;
	TOggFile* iSample;
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

class TOggFiles : public CBase, public MOggFilesSearchBackgroundProcess, public MFileInfoObserver
	{
public:
	enum TOggFilesNextDir { EOggFilesNextDirReady, EOggFilesNextDriveReady, EOggFilesLastDirScanned };

public:
	TOggFiles(COggPlayAppUi* aApp);
	~TOggFiles();

	void CreateDb(RFs& session);
	void CreateDbWithSingleFile(const TDesC& aFile, TRequestStatus& aRequestStatus);

	TBool ReadDb(const TFileName& aFileName, RFs& session);
	void WriteDbL(const TFileName& aFileName, RFs& session);

	void FillTitles(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TDesC& aGenre, const TDesC& aSubFolder);
	void FillAlbums(CDesCArray& arr, const TDesC& anArtist, const TFileName& aSubFolder);
	void FillArtists(CDesCArray& arr, const TFileName& aSubFolder);
	void FillGenres(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TFileName& aSubFolder);
	void FillSubFolders(CDesCArray& arr);
	void FillFileNames(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TDesC& aGenre, const TFileName& aSubFolder);

	void FillPlayLists(CDesCArray& arr);
	void FillStreams(CDesCArray& arr);
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
	void FileSearchStep(TRequestStatus& aRequestStatus);
	void ScanNextPlayList(TRequestStatus& aRequestStatus);
	void CancelOutstandingRequest();
	void FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles, TInt &aNbPlayLists);

private:
	void AddDirectory(const TDesC& aDir, RFs& session);
  
	TInt AddDirectoryStart(const TDesC& aDir, RFs& session);
	void AddDirectoryStop();
	void ClearFiles();

	TBool IsPlayListFile(TParsePtrC& p);

	TOggFilesNextDir NextDirectory();
	TBool AddNextFile();
  
	// From MFileInfoObserver
	void FileInfoCallback(TInt aErr, const TOggFileInfo& aFileInfo);

private:
	COggPlayAppUi *iApp;
	CArrayPtrFlat<TOggFile>* iFiles; 

	CAbsPlayback* iOggPlayback;
	TUid iControllerUid;

	TOggKey iOggKeyTitles;
	TOggKey iOggKeyAlbums;
	TOggKey iOggKeyArtists;
	TOggKey iOggKeyGenres;
	TOggKey iOggKeySubFolders;
	TOggKey iOggKeyFileNames;
	TOggKey iOggKeyTrackTitle;
	TOggKeyNumeric iOggKeyAbsoluteIndex;

	// Following members are only valid during the directory search:
	CDirScan* iDs;
	TDesC* iDirScanDir;
	RFs* iDirScanSession;
	TInt iNbDirScanned;
	TInt iNbFilesFound;
	TInt iNbPlayListsFound;
	TInt iCurrentIndexInDirectory;
	TInt iCurrentIndexInFiles;
	TInt iCurrentDriveIndex;
	CDir* iDirectory;
	CDesC16ArrayFlat* iPathArray;

	TRequestStatus* iFileScanRequestStatus;
	TFileName iPath;
	TFileName iShortName;
	};
#endif
