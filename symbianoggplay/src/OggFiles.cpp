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

#include <OggOs.h>
#include "OggFiles.h"

#include "OggAbsPlayback.h"
#include "OggLog.h"

#include <eikfutil.h>


TOggPlayList::TOggPlayList()
	{
	}
  
TOggPlayList* TOggPlayList::NewL(TInt aAbsoluteIndex, const TDesC& aSubFolder, const TDesC& aFileName, const TDesC& aShortName)
	{
	TOggPlayList* self = new(ELeave) TOggPlayList;
	CleanupStack::PushL(self);

	const TDesC& unassignedTxt = ((COggPlayAppUi *) CEikonEnv::Static()->EikAppUi())->UnassignedTxt();
	self->SetTextL(self->iTitle, aShortName);
	self->SetTextL(self->iAlbum, unassignedTxt);
	self->SetTextL(self->iArtist, unassignedTxt);
	self->SetTextL(self->iGenre, unassignedTxt);
	self->SetTextL(self->iTrackNumber, unassignedTxt);
	self->SetTrackTitleL();

	self->iSubFolder = aSubFolder.AllocL();
	self->iFileName = aFileName.AllocL();
	self->iShortName = aShortName.AllocL();

	self->iAbsoluteIndex = aAbsoluteIndex;
	CleanupStack::Pop(self);
	return self;
	}

TOggPlayList* TOggPlayList::NewL(TFileText& tf)
	{
	TOggPlayList* self = new(ELeave) TOggPlayList;
	CleanupStack::PushL(self);

	const TDesC& unassignedTxt = ((COggPlayAppUi *) CEikonEnv::Static()->EikAppUi())->UnassignedTxt();
	self->SetTextL(self->iAlbum, unassignedTxt);
	self->SetTextL(self->iArtist, unassignedTxt);
	self->SetTextL(self->iGenre, unassignedTxt);
	self->SetTextL(self->iTrackNumber, unassignedTxt);

	self->ReadL(tf);
	self->SetTextL(self->iTitle, *(self->iShortName));
	self->SetTrackTitleL();

	CleanupStack::Pop(self);
	return self;
	}

TOggPlayList::~TOggPlayList()
	{
	iPlayListEntries.Close();
	}

TInt TOggPlayList::Count()
	{
	return iPlayListEntries.Count();
	}

TOggFile* TOggPlayList::operator[] (TInt aIndex)
	{
	return iPlayListEntries[aIndex];
	}

TOggFile::TFileTypes TOggPlayList::FileType() const
	{
	return EPlayList;
	}

void TOggPlayList::SetTextFromFileL(TFileText& aTf, HBufC* &aBuffer)
	{
	TBuf<KTFileTextBufferMaxSize> buf;
	User::LeaveIfError(aTf.Read(buf));

	delete aBuffer;
	aBuffer = NULL;
  
	aBuffer= buf.AllocL();  
	}

void TOggPlayList::ReadL(TFileText& tf)
	{
	SetTextFromFileL(tf, iSubFolder);
	SetTextFromFileL(tf, iFileName);
	SetTextFromFileL(tf, iShortName);
	}

void TOggPlayList::ScanPlayListL(RFs& aFs, TOggFiles* aFiles)
	{
	RFile file;
	User::LeaveIfError(file.Open(aFs, *iFileName, EFileShareReadersOnly));
	CleanupClosePushL(file);

	TInt fileSize;
	User::LeaveIfError(file.Size(fileSize));
	HBufC8* buf = HBufC8::NewLC(fileSize);
	TPtr8 bufPtr(buf->Des());
	User::LeaveIfError(file.Read(bufPtr));

	TInt i = 0;
	TInt j = -1;
	while (j<fileSize)
		{
		i = j+1; // Start of next line

		// Skip any white space
		TText8 nextChar;
		for ( ; i<fileSize ; i++)
			{
			nextChar = bufPtr[i];
			if ((nextChar == ' ') || (nextChar == '\n') || (nextChar == '\r') || (nextChar == '\t'))
				continue;

			break;
			}

		// Check if we have reached the end
		if (i == fileSize)
			break;

		// Extract the file name
		for (j = i ; j<fileSize ; j++)
			{
			nextChar = bufPtr[j];
			if (nextChar == '\n')
				break;
			}

		// Remove any white space from the end
		do
			{
			j--;
			nextChar = bufPtr[j];
			} while ((nextChar == ' ') || (nextChar == '\r') || (nextChar == '\t'));

		// Skip comment lines
		if (bufPtr[i] == '#')
			continue;

		// Construct the filename (add the playlist path if the filename is relative)
		TFileName fileName;
		fileName.Copy(bufPtr.Mid(i, (j-i) + 1));
		if (fileName[0] == '\\')
			fileName.Insert(0, TPtrC(iFileName->Ptr(), 2));
		else if (fileName[1] != ':')
			fileName.Insert(0, TPtrC(iFileName->Ptr(), iFileName->Length()-(iShortName->Length()+4)));

		// Search for the file and add it to the list (if found)
		TOggFile* o = aFiles->FindFromFileNameL(fileName);
		if (o)
			iPlayListEntries.Append(o);
		}

	CleanupStack::PopAndDestroy(2, &file); 
	}

TInt TOggPlayList::Write(TInt aLineNumber, HBufC* aBuf)
	{
	aBuf->Des().Format(_L("%d\n%S\n%S\n%S\n"), aLineNumber, iSubFolder, iFileName, iShortName);
 
	return KErrNone;
	}


TOggFile::TOggFile()
	{
	}

_LIT(KEmpty, "-");
TOggFile* TOggFile::NewL()
	{
	TOggFile* self = new(ELeave) TOggFile;
	CleanupStack::PushL(self);

	self->SetTextL(self->iTitle, KEmpty);
	self->SetTextL(self->iAlbum, KEmpty);
	self->SetTextL(self->iArtist, KEmpty);
	self->SetTextL(self->iGenre, KEmpty);
	self->SetTextL(self->iFileName, KEmpty);
	self->SetTextL(self->iSubFolder, KEmpty);
	self->SetTextL(self->iShortName, KEmpty);
	self->SetTextL(self->iTrackNumber, KEmpty);
	self->SetTextL(self->iTrackTitle, KEmpty);

	CleanupStack::Pop(self);
	return self;
	}

TOggFile* TOggFile::NewL(TInt aAbsoluteIndex, const TDesC& aTitle, const TDesC& aAlbum, const TDesC& aArtist,
const TDesC& aGenre, const TDesC& aSubFolder, const TDesC& aFileName, const TDesC& aShortName, const TDesC& aTrackNumber)
	{
	TOggFile* self = new(ELeave) TOggFile;
	CleanupStack::PushL(self);

	const TDesC& unassignedTxt = ((COggPlayAppUi *) CEikonEnv::Static()->EikAppUi())->UnassignedTxt();
	self->iAbsoluteIndex = aAbsoluteIndex;
	if (aTitle.Length()>0)
		self->SetTextL(self->iTitle, aTitle);
	else
		self->SetTextL(self->iTitle, aShortName);
  
	if (aAlbum.Length()>0)
		self->SetTextL(self->iAlbum, aAlbum);
	else
		self->SetTextL(self->iAlbum, unassignedTxt);
  
	if (aArtist.Length()>0)
		self->SetTextL(self->iArtist, aArtist);
	else
		self->SetTextL(self->iArtist, unassignedTxt);

	if (aGenre.Length()>0)
		self->SetTextL(self->iGenre, aGenre);
	else
		self->SetTextL(self->iGenre, unassignedTxt);

	if (aSubFolder.Length()>0)
		self->SetTextL(self->iSubFolder, aSubFolder);
	else
		self->SetTextL(self->iSubFolder, unassignedTxt);

	self->SetTextL(self->iFileName, aFileName);
	self->SetTextL(self->iShortName, aShortName);

	if (aTrackNumber.Length()>0)
		self->SetTextL(self->iTrackNumber, aTrackNumber);
	else
		self->SetTextL(self->iTrackNumber, unassignedTxt);

  self->SetTrackTitleL();
  CleanupStack::Pop(self);
  return self;
  }

TOggFile* TOggFile::NewL(TFileText& tf, TInt aVersion)
  {
  TOggFile* self = new(ELeave) TOggFile();
  CleanupStack::PushL(self);
  self->ReadL(tf, aVersion);

  CleanupStack::Pop(self);
  return self;
  }

void TOggFile::SetTrackTitleL()
	{
	TInt track(0);
	TLex parse(*iTrackNumber);
	parse.Val(track);
	if (track>999)
		track = 999;

	TBuf<128> buf;
	buf.NumFixedWidth(track, EDecimal, 2);
	buf.Append(_L(" "));
	buf.Append(iTitle->Left(123));
	SetTextL(iTrackTitle, buf);
	}

TOggFile::~TOggFile()
	{
	delete iTitle;
	delete iAlbum;
	delete iArtist;
	delete iGenre;
	delete iSubFolder;
	delete iFileName;
	delete iShortName;
	delete iTrackNumber;
	delete iTrackTitle;
	}

TOggFile::TFileTypes TOggFile::FileType() const
	{
	return EFile;
	}

void TOggFile::SetTextL(HBufC* &aBuffer, const TDesC& aText)
	{
	if (aBuffer)
		{
		delete aBuffer;
		aBuffer = NULL;
		}

	aBuffer = aText.AllocL();
	}

void TOggFile::SetTextFromFileL(TFileText& aTf, HBufC*& aBuffer)
	{
	TBuf<KTFileTextBufferMaxSize> buf;
	User::LeaveIfError(aTf.Read(buf));

	if (aBuffer)
		{
		delete aBuffer;
		aBuffer = NULL;
		}

	aBuffer = buf.AllocL();
	}

void TOggFile::ReadL(TFileText& tf, TInt aVersion)
	{
	SetTextFromFileL(tf, iTitle);
	SetTextFromFileL(tf, iAlbum);
	SetTextFromFileL(tf, iArtist);
	SetTextFromFileL(tf, iGenre);
	SetTextFromFileL(tf, iSubFolder);
	SetTextFromFileL(tf, iFileName);
	SetTextFromFileL(tf, iShortName);

	if (aVersion >= 1)
		SetTextFromFileL(tf, iTrackNumber);

	SetTrackTitleL();
	}

TInt TOggFile::Write(TInt aLineNumber, HBufC* aBuf)
	{
	aBuf->Des().Format(_L("%d\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n"), 
    aLineNumber, iTitle, iAlbum, iArtist, iGenre, iSubFolder, iFileName, iShortName, iTrackNumber);
 
	return KErrNone;
	}


TOggKey::TOggKey(TInt aOrder)
: TKeyArrayFix(0, ECmpNormal), iFiles(0), iOrder(aOrder)
	{
	iSample = TOggFile::NewL();
	SetPtr(iSample);
	}

TOggKey::~TOggKey() 
	{
	delete iSample;
	}

void TOggKey::SetFiles(CArrayPtrFlat<TOggFile>* aFiles)
	{
	iFiles = aFiles;
	}

TAny* TOggKey::At(TInt aIndex) const
	{
	if (aIndex == KIndexPtr)
		return(&iSample->iTitle);

	if ((aIndex>=0) && (aIndex<iFiles->Count()))
		{
		switch (iOrder)
			{
			case COggPlayAppUi::ETitle : return (*iFiles)[aIndex]->iTitle;
			case COggPlayAppUi::EAlbum : return (*iFiles)[aIndex]->iAlbum;
			case COggPlayAppUi::EArtist: return (*iFiles)[aIndex]->iArtist;
			case COggPlayAppUi::EGenre : return (*iFiles)[aIndex]->iGenre;
			case COggPlayAppUi::ESubFolder: return (*iFiles)[aIndex]->iSubFolder;
			case COggPlayAppUi::EFileName : return (*iFiles)[aIndex]->iShortName;
			case COggPlayAppUi::ETrackTitle: return (*iFiles)[aIndex]->iTrackTitle;
			}
		}

	return 0;
	}


TOggKeyNumeric::TOggKeyNumeric()
: TKeyArrayFix(_FOFF(TOggFile, iAbsoluteIndex), ECmpTInt), iFiles(0)
	{
	SetPtr(&iInteger); 
	}

TOggKeyNumeric::~TOggKeyNumeric() 
	{
	}

void TOggKeyNumeric::SetFiles(CArrayPtrFlat<TOggFile>* aFiles)
	{
	iFiles = aFiles;
	}

TAny* TOggKeyNumeric::At(TInt aIndex) const
	{
	if (aIndex == KIndexPtr) 
		return ((TUint8 *) iPtr) + iKeyOffset;

	if ((aIndex>=0) && (aIndex<iFiles->Count()))
		return &((*iFiles)[aIndex]->iAbsoluteIndex);

	return 0;
	}


TOggFiles::TOggFiles(COggPlayAppUi* aApp)
: iApp(aApp), iOggKeyTitles(COggPlayAppUi::ETitle),
iOggKeyAlbums(COggPlayAppUi::EAlbum), iOggKeyArtists(COggPlayAppUi::EArtist),
iOggKeyGenres(COggPlayAppUi::EGenre), iOggKeySubFolders(COggPlayAppUi::ESubFolder),
iOggKeyFileNames(COggPlayAppUi::EFileName), iOggKeyTrackTitle(COggPlayAppUi::ETrackTitle)
	{
	iFiles = new(ELeave) CArrayPtrFlat<TOggFile>(10);

	iOggKeyTitles.SetFiles(iFiles);
	iOggKeyAlbums.SetFiles(iFiles);
	iOggKeyArtists.SetFiles(iFiles);
	iOggKeyGenres.SetFiles(iFiles);
	iOggKeySubFolders.SetFiles(iFiles);
	iOggKeyFileNames.SetFiles(iFiles);
	iOggKeyTrackTitle.SetFiles(iFiles);
	iOggKeyAbsoluteIndex.SetFiles(iFiles);
	}

TOggFiles::~TOggFiles()
	{
	iFiles->ResetAndDestroy();
	delete iFiles;
	}

void TOggFiles::CreateDb(RFs& session)
	{
	TBuf<256> buf;
	CEikonEnv::Static()->ReadResource(buf, R_OGG_STRING_1);
	CEikonEnv::Static()->BusyMsgL(buf);

	ClearFiles();
	AddDirectory(_L("D:\\Media files\\audio\\"),session);
	AddDirectory(_L("C:\\documents\\Media files\\audio\\"),session);
	AddDirectory(_L("D:\\Media files\\other\\"),session);
	AddDirectory(_L("C:\\documents\\Media files\\other\\"),session);
	AddDirectory(_L("D:\\Media files\\document\\"),session);
	AddDirectory(_L("C:\\documents\\Media files\\document\\"),session);

#if defined(SERIES60)  
	AddDirectory(_L("C:\\Oggs\\"),session);
	AddDirectory(_L("E:\\Oggs\\"),session);
#endif

	// Parse playlists
	TInt numFiles = iFiles->Count();
	TInt err;
	for (TInt i = 0 ; i<numFiles ; i++)
		{
		TOggFile* o = (*iFiles)[i];
		if (o->FileType() == TOggFile::EPlayList)
			{
			// Parse the playlist, ignoring any errors.
			TRAP(err, ((TOggPlayList*) o)->ScanPlayListL(session, this));
			}
		}

	CEikonEnv::Static()->BusyMsgCancel();
	}

void TOggFiles::AddDirectory(const TDesC& aDir, RFs& session)
	{
    if (AddDirectoryStart(aDir, session) == KErrNone)
		{
		TRequestStatus requestStatus;
		do
			{
			FileSearchStep(requestStatus);
			User::WaitForRequest(requestStatus);
			} while (requestStatus != KOggFileScanStepComplete);
	    }

    AddDirectoryStop();
	}

TInt TOggFiles::AddDirectoryStart(const TDesC& aDir, RFs& session)
	{
	// TRACEF(COggLog::VA(_L("Scanning directory %S for oggfiles"), &aDir));

	// Check if the drive is available (i.e. memory stick is inserted)
#if defined(UIQ)
	if (!EikFileUtils::FolderExists(aDir)) 
#else
	// UIQ_? Have had some problems with FolderExists in the WIN emulator (?)
	if (!EikFileUtils::PathExists(aDir)) 
#endif
		{
		TRACEF(COggLog::VA(_L("Folder %S doesn't exist"), &aDir ));
		return KErrNotFound;
		}
 
	if (!iPathArray)
		{
		iPathArray = new(ELeave) CDesC16ArrayFlat(3);
		iDs = CDirScan::NewL(session);
		iDirScanSession = &session;
		iDirScanDir = const_cast <TDesC *>(&aDir);
		iNbDirScanned = 0;
		iNbFilesFound = 0;
		iNbPlayListsFound = 0;
		iCurrentIndexInDirectory = -1;
		iCurrentDriveIndex = 0;
		iDirectory = NULL;

		TRAPD(err,iDs->SetScanDataL(aDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
		if (err != KErrNone) 
			{
			TRACEF(COggLog::VA(_L("Unable to setup scan directory %S for oggfiles"), &aDir));

			delete iDs ; iDs = NULL;
			return KErrNotFound;
			}
		}

	iPathArray->AppendL(aDir);
	return KErrNone;
	}

void TOggFiles::AddDirectoryStop()
{
	if (iPathArray)
        iPathArray->Reset();

	delete iDs ; iDs = NULL;
    delete iDirectory ; iDirectory = NULL;
	delete iPathArray ; iPathArray = NULL;    
}

void TOggFiles::FileSearchStep(TRequestStatus& aRequestStatus)
	{
	iFileScanRequestStatus = &aRequestStatus;
	aRequestStatus = KRequestPending;

	if (iCurrentIndexInDirectory == -1)
		{
		// Add New directory
		TOggFilesNextDir nextDir = NextDirectory();
		if (nextDir != EOggFilesNextDirReady)
			{
			TInt completeStatus = (nextDir == EOggFilesLastDirScanned) ? KOggFileScanStepComplete : KErrNone;
			User::RequestComplete(iFileScanRequestStatus, completeStatus);
			return;
			}
		}

	TInt dirCount = iDirectory->Count();
    for (; iCurrentIndexInDirectory<dirCount ; iCurrentIndexInDirectory++) 
		{
		// Try to add the next file (return if we get one)
		if (AddNextFile())
			return;
		}

	// Go to next directory
	iCurrentIndexInDirectory = -1;
	User::RequestComplete(iFileScanRequestStatus, KErrNone);
	}

TOggFiles::TOggFilesNextDir TOggFiles::NextDirectory()
	{
	// Move on to the next directory
	delete iDirectory;
	iDirectory = NULL;

	TRAPD(err, iDs->NextL(iDirectory));
	if (err != KErrNone) 
		{
		TRACEF(COggLog::VA(_L("Unable to scan directory %S for oggfiles"), &iDirScanDir ));
		delete iDs ; iDs = NULL;

		return EOggFilesLastDirScanned;
		}
        
	if (iDirectory == NULL) 
		{
		// No more files in that drive.
		// Any other drive to search in?
		iCurrentDriveIndex++;
		if (iCurrentDriveIndex < iPathArray->Count())
			{
			// More drives to search
            TRAP(err, iDs->SetScanDataL((*iPathArray)[iCurrentDriveIndex], KEntryAttNormal, ESortByName|EAscending,CDirScan::EScanDownTree));
            if (err != KErrNone) 
				{
                TPtrC aBuf((*iPathArray)[iCurrentDriveIndex]);
                TRACEF(COggLog::VA(_L("Unable to setup scan directory %S for oggfiles"), &aBuf));
                delete iDs ; iDs = NULL;

				return EOggFilesLastDirScanned;
				}

            return EOggFilesNextDriveReady;
			}
		else
			{
            // No more directories to search
			return EOggFilesLastDirScanned;
			}
		}

    iNbDirScanned++;
	iCurrentIndexInDirectory = 0;
	return EOggFilesNextDirReady;
	}

TBool TOggFiles::AddNextFile()
	{
	TBool fileFound = EFalse; 
	const TEntry& e = (*iDirectory)[iCurrentIndexInDirectory];

	TFileName fullName;
	fullName.Copy(iDs->FullPath());
	fullName.Append(e.iName);
	// TRACEF(COggLog::VA(_L("AddNextFile: %S"), &fullName));

	TParsePtrC p(fullName);
	if (iApp->PlaybackFromFileName(fullName, iOggPlayback, iControllerUid) == KErrNone) 
		{
		iShortName.Copy(p.Name());

		iPath.Copy(iDs->AbbreviatedPath());
		if (iPath.Length()>1 && iPath[iPath.Length()-1] == L'\\')
			iPath.SetLength(iPath.Length()-1); // get rid of trailing back slash

        TInt err = iOggPlayback->Info(fullName, iControllerUid, *this);
        if (err == KErrNone)
			fileFound = ETrue;
        else
			{
			// Just skip the file if there is an error
			TRACEF(COggLog::VA(_L("AddNextFile failed with code %d"), err));
			}
		}
	else if (IsPlayListFile(p))
		{
		fileFound = ETrue;
		iCurrentIndexInDirectory++;

		// Add the playlist to the list of playlists.
		TFileName path;
		path.Copy(iDs->AbbreviatedPath());
        if ((path.Length()>1) && (path[path.Length()-1] == L'\\'))
            path.SetLength(path.Length()-1); // get rid of trailing back slash

		TOggPlayList* o = NULL;
		TRAPD(err,
			{
			o = TOggPlayList::NewL(iNbPlayListsFound, path, fullName, p.Name());
			iFiles->AppendL(o);
			});
        
		if (err == KErrNone)
			iNbPlayListsFound++;
	
		User::RequestComplete(iFileScanRequestStatus, err);
		}

	return fileFound;
	}

void TOggFiles::FileInfoCallback(TInt aErr, const TOggFileInfo& aFileInfo)
	{
	iCurrentIndexInDirectory++;

	if (aErr != KErrNone)
		{
		User::RequestComplete(iFileScanRequestStatus, aErr);
		return;
		}

	TOggFile* o = NULL;
	TRAP(aErr,
		{
		o = TOggFile::NewL(iNbFilesFound, aFileInfo.iTitle, aFileInfo.iAlbum, 
		aFileInfo.iArtist, aFileInfo.iGenre, iPath, aFileInfo.iFileName, iShortName, aFileInfo.iTrackNumber);

		iFiles->AppendL(o);
		});
  
	if (aErr == KErrNone)
		iNbFilesFound++;

	User::RequestComplete(iFileScanRequestStatus, aErr);
	}

void TOggFiles::CancelOutstandingRequest()
	{
	if (iFileScanRequestStatus)
		{
		iOggPlayback->InfoCancel();
		User::RequestComplete(iFileScanRequestStatus, KErrCancel);
		}
	}

void TOggFiles::ScanNextPlayList(TRequestStatus& aRequestStatus)
	{
	iFileScanRequestStatus = &aRequestStatus;
	aRequestStatus = KRequestPending;

	TInt numFiles = iFiles->Count();
	if (iCurrentIndexInFiles == numFiles)
		{
		User::RequestComplete(iFileScanRequestStatus, KOggFileScanStepComplete);
		return;
		}

	for ( ; iCurrentIndexInFiles<numFiles ; iCurrentIndexInFiles++)
		{
		TOggFile* o = (*iFiles)[iCurrentIndexInFiles];
		if (o->FileType() == TOggFile::EPlayList)
			{
			// Parse the playlist, ignoring any errors.
			TRAPD(err, ((TOggPlayList*) o)->ScanPlayListL(*iDirScanSession, this));
			iCurrentIndexInFiles++;
			break;
			}
		}

	User::RequestComplete(iFileScanRequestStatus, KErrNone);
	}

void  TOggFiles::FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles, TInt &aNbPlayLists)
	{
    aNbDir = iNbDirScanned;
    aNbFiles = iNbFilesFound;
    aNbPlayLists = iNbPlayListsFound;
	}

TInt TOggFiles::SearchAllDrives(CEikDialog* aDialog, TInt aDialogID, RFs& session)
	{
	ClearFiles();
    
	TChar driveLetter; TDriveInfo driveInfo;
	TInt err=KErrNone;
	TInt driveNumber;
	for (driveNumber = EDriveA ; driveNumber<=EDriveZ ; driveNumber++)
		{
		session.Drive(driveInfo, driveNumber); 
		if (driveInfo.iDriveAtt == (TUint) KDriveAbsent)
			continue; 

		if ((driveInfo.iType != (TUint) EMediaRom) && (driveInfo.iType != (TUint) EMediaNotPresent)
#if defined(__WINS__)
          // For some reasons, the emulator finds a non-existing drive X, which blows up everything...
          && (driveInfo.iType != EMediaHardDisk) 
#endif
          )
			{
			TRAP(err, session.DriveToChar(driveNumber,driveLetter));
			if (err)
				break;

			TBuf <10> driveName;
			driveName.Format(_L("%c:\\"), ((TUint) driveLetter));
			err = AddDirectoryStart(driveName, session);
			if (err)
				break;
			}
		}

	if (err == KErrNone)
		{
        TRAP(err, aDialog->ExecuteLD(aDialogID));
		}
    else
        delete aDialog;

    AddDirectoryStop();
    return err;
}

TInt TOggFiles::SearchSingleDrive(const TDesC& aDir, CEikDialog* aDialog, TInt aDialogID, RFs& session)
{
    ClearFiles();
    if (!EikFileUtils::PathExists(aDir)) 
    {
	  delete aDialog;
      TRACEF(COggLog::VA(_L("Folder %S doesn't exist"), &aDir));

	  TBuf<256> buf1, buf2;
      CEikonEnv::Static()->ReadResource(buf2, R_OGG_ERROR_28);
      buf1.Append(aDir);

	  COggMsgEnv::OggErrorMsgL(buf2, buf1);
      return KErrNotFound;
    }

    TInt err = AddDirectoryStart(aDir, session);
    if (err == KErrNone)
	{
        TRAP(err, aDialog->ExecuteLD(aDialogID));
    }
    else
        delete aDialog;

	AddDirectoryStop();
    return err;
}

void TOggFiles::CreateDbWithSingleFile(const TDesC& aFile, TRequestStatus& aRequestStatus)
	{
	iFileScanRequestStatus = &aRequestStatus;
	aRequestStatus = KRequestPending;

	TInt err = KErrArgument;
	if (iApp->PlaybackFromFileName(aFile, iOggPlayback, iControllerUid) == KErrNone)
		{
		ClearFiles();

		TParsePtrC p(aFile);
		iShortName.Copy(p.Name());
		iPath.Copy(p.DriveAndPath());
		if (iPath.Length()>1 && iPath[iPath.Length()-1] == L'\\')
			iPath.Delete(iPath.Length()-1, 1); // get rid of trailing back slash

		err = iOggPlayback->Info(aFile, iControllerUid, *this);
		}

	if (err != KErrNone)
		User::RequestComplete(iFileScanRequestStatus, err);
	}

TBool TOggFiles::ReadDb(const TFileName& aFileName, RFs& session)
{
  RFile in;
  if(in.Open(session, aFileName, EFileRead|EFileStreamText)== KErrNone)
  {
    TFileText tf;
    tf.Set(in);
    
    // check file version:
    TBuf<KTFileTextBufferMaxSize> line;
	TInt err;
    TInt version= -1;
    if (tf.Read(line)==KErrNone)
	{
      TLex parse(line);
      parse.Val(version);
    }
    
	if ((version!=0) && (version!=1) && (version!=2))
	{
      in.Close();
      return EFalse;
    }

	TInt i;
	TInt numPlayLists = 0;
	RPointerArray<TOggPlayList> playLists;
	CleanupClosePushL(playLists);
	if (version==2)
	{
		if(tf.Read(line)==KErrNone)
		{
	      TLex parse(line);
		  err = parse.Val(numPlayLists);
		  if (err != KErrNone)
		  {
			  in.Close();
			  return EFalse;
		  }
		}
		else 
		{
			in.Close();
			return EFalse;
		}

		for (i = 0 ; i<numPlayLists ; i++)
		{
		  // Read in the line number.
		  err = tf.Read(line);
		  if (err != KErrNone)
		  {
			  in.Close();
			  return ETrue;
		  }

	      TOggPlayList* o = NULL;
		  TRAP (err, o = TOggPlayList::NewL(tf));
          if (err != KErrNone)
		  {
             in.Close();
			 return ETrue;
		  }
     
		  o->iAbsoluteIndex = i;
	      iFiles->AppendL(o);
		  playLists.Append(o);
		}
	}
	
    while (tf.Read(line)==KErrNone) 
      {
      TOggFile* o = NULL;
      TRAP( err, o = TOggFile::NewL(tf, version); );
      if (err != KErrNone)
	  {
         in.Close();
		 return ETrue;
	  }

      o->iAbsoluteIndex = iFiles->Count();
      iFiles->AppendL(o);
      }

    in.Close();
	
	// Parse playlists
	for (i = 0 ; i<numPlayLists ; i++)
	{
		// Parse the playlist, ignoring any errors.
		TRAP(err, playLists[i]->ScanPlayListL(session, this));
	}

	CleanupStack::PopAndDestroy(&playLists);

	return ETrue;
  }
  return EFalse;
}

void TOggFiles::WriteDbL(const TFileName& aFileName, RFs& session)
{
  CEikonEnv::Static()->BusyMsgL(R_OGG_STRING_2);
  RFile out;

  TBuf<64> buf;
  TInt err= out.Replace(session, aFileName, EFileWrite|EFileStreamText);
  if (err!=KErrNone)
  {
    CEikonEnv::Static()->BusyMsgCancel();
    buf.SetLength(0);
    buf.Append(_L("Error at open: "));
    buf.AppendNum(err);
    User::InfoPrint(buf);
    User::After(TTimeIntervalMicroSeconds32(1000000));
	return;
  }

  TFileText tf;
  tf.Set(out);
    
  // Write file version:
  TBuf<16> line;
  line.Num(2);

  err = tf.Write(line);
     
  HBufC* dat = HBufC::NewLC(KDiskWriteBufferSize);
  HBufC* tempBuf = HBufC::NewLC(KTFileTextBufferMaxSize*KNofOggDescriptors);

  // Buffer up writes. Speed issue on phones without MMC write cache.
  TInt i;

  TInt numFiles = iFiles->Count();
  if (err==KErrNone)
  {
    
	RPointerArray<TOggPlayList> playLists;
	CleanupClosePushL(playLists);
	for (i = 0 ; i<numFiles ; i++)
	{
		TOggFile* o = (*iFiles)[i];
		if (o->FileType() == TOggFile::EPlayList)
			playLists.Append((TOggPlayList*) o);
	}

	TInt numPlayLists = playLists.Count();
	line.Num(numPlayLists);
	tf.Write(line);

	for (i = 0 ; i<numPlayLists ; i++)
	{
		if ((err = playLists[i]->Write(i,tempBuf)) != KErrNone)
	        break;

	    TInt newSize = dat->Length() + tempBuf->Length();
		if( KDiskWriteBufferSize < newSize )
		{
			// Write to disk. Remove the last '\n' since Write() will add this
			dat->Des().SetLength( dat->Des().Length()-1 ); 
			err = tf.Write(dat->Des());
			if (err != KErrNone)
				break;

			dat->Des().Zero();
		}
          
		dat->Des().Append(tempBuf->Des());
	}

	CleanupStack::PopAndDestroy(&playLists);
  }

  if (err == KErrNone)
  {
      for ( i=0; i<numFiles; i++)
	  {
		TOggFile* o = (*iFiles)[i];
		if (o->FileType() == TOggFile::EPlayList)
			continue;

        if ((err = (*iFiles)[i]->Write(i,tempBuf)) != KErrNone)
            break;

        TInt newSize = dat->Length() + tempBuf->Length();
        if( KDiskWriteBufferSize < newSize )
		{
          // Write to disk. Remove the last '\n' since Write() will add this
          dat->Des().SetLength( dat->Des().Length()-1 ); 
          err = tf.Write(dat->Des());
		  if (err != KErrNone)
			break;

          dat->Des().Zero();
        }
          
		dat->Des().Append(tempBuf->Des());
      }
  }

  // Final write to disk.
  if( (dat->Des().Length() > 0) && (err == KErrNone))
  {
    dat->Des().SetLength( dat->Des().Length()-1 ); 
    err = tf.Write(dat->Des());
  }

  CleanupStack::PopAndDestroy(2); // dat + tempBuf

  if (err!=KErrNone)
  {
    buf.Append(_L("Error at write: "));
    buf.AppendNum(err);
    User::InfoPrint(buf);
    User::After(TTimeIntervalMicroSeconds32(1000000));
  }

  out.Close();
  CEikonEnv::Static()->BusyMsgCancel();
}

void TOggFiles::AppendLine(CDesCArray& arr, COggListBox::TItemTypes aType, const TDesC& aText, const TInt anAbsoluteIndex)
{
  TInt lineLength = aText.Length() + 15;
  HBufC* hbuf = HBufC::NewLC(lineLength);
  TPtr hbufDes = hbuf->Des();
  hbufDes.AppendNum((TInt) aType);
  hbufDes.Append(KColumnListSeparator);
  hbufDes.Append(aText);
  hbufDes.Append(KColumnListSeparator);
  hbufDes.AppendNum(anAbsoluteIndex);
  hbufDes.Append(KColumnListSeparator);
  hbufDes.AppendNum(0);
  arr.AppendL(hbufDes);
  CleanupStack::PopAndDestroy();
}

void TOggFiles::AppendLine(CDesCArray& arr, COggListBox::TItemTypes aType, const TDesC& aText, const TDesC& anInternalText)
{
  TInt lineLength = aText.Length() + anInternalText.Length() + 15;
  HBufC* hbuf = HBufC::NewLC(lineLength);
  TPtr hbufDes = hbuf->Des();
  hbufDes.AppendNum((TInt) aType);
  hbufDes.Append(KColumnListSeparator);
  hbufDes.Append(aText);
  hbufDes.Append(KColumnListSeparator);
  hbufDes.Append(anInternalText);
  arr.AppendL(hbufDes);
  CleanupStack::PopAndDestroy();
}

void TOggFiles::AppendTitleAndArtist(CDesCArray& arr, COggListBox::TItemTypes aType, const TDesC& aTitle, const TDesC& aDelim, const TDesC& aArtist, const TInt anAbsoluteIndex)
	{
	TInt lineLength = aTitle.Length() + aDelim.Length() + aArtist.Length() + 15;
	HBufC* hBuf = HBufC::NewLC(lineLength);
	TPtr hDes = hBuf->Des();

	hDes.AppendNum((TInt) aType);
	hDes.Append(KColumnListSeparator);
	hDes.Append(aTitle);
	hDes.Append(aDelim);
	hDes.Append(aArtist);
	hDes.Append(KColumnListSeparator);
	hDes.AppendNum(anAbsoluteIndex);
	hDes.Append(KColumnListSeparator);
	hDes.AppendNum(0);
	arr.AppendL(hDes);
	
	CleanupStack::PopAndDestroy(hBuf);
	}

void TOggFiles::ClearFiles()
	{
	iCurrentIndexInFiles = 0;
	iFiles->ResetAndDestroy();
	}

TDesC& TOggFiles::FindFromIndex(TInt anIndex)
	{
    TOggFile * o = TOggFile::NewL(anIndex, KEmpty, KEmpty, KEmpty, KEmpty, KEmpty, KEmpty, KEmpty, KEmpty);
    CleanupStack::PushL(o);

	TInt foundIdx = -1;
    iFiles->Find((TOggFile * const &) *o, iOggKeyAbsoluteIndex, foundIdx);

	CleanupStack::PopAndDestroy(o);
    return *(*iFiles)[foundIdx]->iFileName;
	}

TOggFile* TOggFiles::FindFromFileNameL(TFileName& aFileName)
	{
    TOggFile* o = TOggFile::NewL(-1, KEmpty, KEmpty, KEmpty, KEmpty, KEmpty, aFileName, KEmpty, KEmpty);

	TInt foundIdx;
	TBool found = EFalse;
	for (foundIdx = 0 ; foundIdx < iFiles->Count() ; foundIdx++)
		{
		TOggFile* o2 = (*iFiles)[foundIdx];
		if (!(o2->iFileName->Compare(*o->iFileName)))
			{
			found = ETrue;
			break;
			}
		}

	delete o;
	return (found) ? (*iFiles)[foundIdx] : NULL ;
	}

void TOggFiles::FillTitles(CDesCArray& arr, const TDesC& anAlbum, 
const TDesC& anArtist, const TDesC& aGenre, const TDesC& aSubFolder)
	{
	arr.Reset();
	if (anAlbum.Length()>0) 
		iFiles->Sort(iOggKeyTrackTitle); 
	else
		{
		if (aSubFolder.Length()>0) 
			iFiles->Sort(iOggKeyFileNames); 
		else 
			iFiles->Sort(iOggKeyTitles);
		}

	if (anAlbum.Length()==0 && anArtist.Length()==0  && aGenre.Length()==0  && aSubFolder.Length()==0)
		{
		TBuf<10> buf;
		CEikonEnv::Static()->ReadResource(buf, R_OGG_BY);
		for (TInt i = 0 ; i<iFiles->Count() ; i++)
			{
			TOggFile& o= *(*iFiles)[i];

			// Playlists don't appear in titles view
			if (o.FileType() == TOggFile::EPlayList)
				continue;

			AppendTitleAndArtist(arr, COggListBox::ETitle, *(o.iTitle), buf, *(o.iArtist), (TInt) &o);
			}
		}
	else
		{
		for (TInt i = 0 ; i<iFiles->Count() ; i++)
			{
			TOggFile& o = *(*iFiles)[i];

			// Playlists don't appear in titles view
			if (o.FileType() == TOggFile::EPlayList)
				continue;

			TBool select = (anAlbum.Length()==0 || *o.iAlbum==anAlbum) && (anArtist.Length()==0 || *o.iArtist==anArtist)
			&& (aGenre.Length()==0 || *o.iGenre==aGenre) && (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);

			if (select)
				AppendLine(arr, COggListBox::ETitle, *(o.iTitle), (TInt) &o);
			}
		}
	}

void TOggFiles::FillAlbums(CDesCArray& arr, const TDesC& /*anArtist*/, const TFileName& aSubFolder)
	{
	arr.Reset();
	iFiles->Sort(iOggKeyAlbums);

	TBuf<256> lastAlbum;
	for (TInt i = 0 ; i<iFiles->Count() ; i++)
		{
		TOggFile& o= *(*iFiles)[i];

		// Playlists don't appear in album view
		if (o.FileType() == TOggFile::EPlayList)
			continue;

		if (aSubFolder.Length()==0 || *(o.iSubFolder)==aSubFolder)
			{
			if (lastAlbum!=*o.iAlbum)
				{
				AppendLine(arr, COggListBox::EAlbum, *(o.iAlbum), *(o.iAlbum));
				lastAlbum= *o.iAlbum;
				}
			}
		}
	}

void TOggFiles::FillArtists(CDesCArray& arr, const TFileName& aSubFolder)
	{
	arr.Reset();
	iFiles->Sort(iOggKeyArtists);

	TBuf<256> lastArtist;
	for (TInt i = 0 ; i<iFiles->Count() ; i++)
		{
		TOggFile& o= *(*iFiles)[i];

		// Playlists don't appear in artist view
		if (o.FileType() == TOggFile::EPlayList)
			continue;

		if (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder)
			{
			if (lastArtist!=*o.iArtist)
				{
				AppendLine(arr, COggListBox::EArtist, *o.iArtist, *o.iArtist);
				lastArtist= *o.iArtist;
				}
			}
		}
	}

void TOggFiles::FillFileNames(CDesCArray& arr, const TDesC& anAlbum,
const TDesC& anArtist, const TDesC& aGenre, const TFileName& aSubFolder)
	{
	arr.Reset();
	iFiles->Sort(iOggKeyFileNames);

	for (TInt i = 0 ; i<iFiles->Count() ; i++)
		{
		TOggFile& o = *(*iFiles)[i];

		// Playlists don't appear in files view
		if (o.FileType() == TOggFile::EPlayList)
			continue;

		TBool select = (anAlbum.Length()==0 || *o.iAlbum==anAlbum) && (anArtist.Length()==0 || *o.iArtist==anArtist)
		&& (aGenre.Length()==0 || *o.iGenre==aGenre) && (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);

		if (select)
			AppendLine(arr, COggListBox::EFileName, *o.iShortName, (TInt) &o);
		}
	}

void TOggFiles::FillSubFolders(CDesCArray& arr)
	{
	arr.Reset();
	iFiles->Sort(iOggKeySubFolders);

	TBuf<256> lastSubFolder;
	for (TInt i = 0 ; i<iFiles->Count() ; i++)
		{
		TOggFile& o= *(*iFiles)[i];

		// Playlists don't appear in sub folders view
		if (o.FileType() == TOggFile::EPlayList)
			continue;

		if (lastSubFolder != *o.iSubFolder)
			{
			AppendLine(arr, COggListBox::ESubFolder, *o.iSubFolder, *o.iSubFolder);
			lastSubFolder= *o.iSubFolder;
			}
		}
	}

void TOggFiles::FillGenres(CDesCArray& arr, const TDesC& anAlbum,
const TDesC& anArtist, const TFileName& aSubFolder)
	{
	arr.Reset();
	iFiles->Sort(iOggKeyGenres);

	TBuf<256> lastGenre;
	for (TInt i = 0 ; i<iFiles->Count() ; i++)
		{
		TOggFile& o= *(*iFiles)[i];

		// Playlists don't appear in genre view
		if (o.FileType() == TOggFile::EPlayList)
			continue;

		if (lastGenre!=*o.iGenre)
			{
			TBool select = (anAlbum.Length()==0 || *o.iAlbum==anAlbum) && (anArtist.Length()==0 || *o.iAlbum==anArtist)
			&& (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);

			if (select)
				{
				AppendLine(arr, COggListBox::EGenre, *o.iGenre, *o.iGenre);
				lastGenre= *o.iGenre;
				}
			}
		}
	}

void TOggFiles::FillPlayLists(CDesCArray& arr)
	{
	arr.Reset();
	iFiles->Sort(iOggKeyFileNames);

	TInt numFiles = iFiles->Count();
	for (TInt i=0 ; i<numFiles ; i++)
		{
		TOggFile* o = (*iFiles)[i];
		if (o->FileType() == TOggFile::EPlayList)
			AppendLine(arr, COggListBox::EPlayList, *(o->iShortName), (TInt) o);
		}
	}

void TOggFiles::FillPlayList(CDesCArray& arr, const TDesC& aPlayListFile)
	{
	arr.Reset();

	TInt playListInt(0);
	TLex parse(aPlayListFile);
	parse.Val(playListInt);
	TOggPlayList* playList = (TOggPlayList*) playListInt;

	// Loop through the entries and add them
	TInt numEntries = playList->Count();
	for (TInt i = 0 ; i<numEntries ; i++)
		{
		TOggFile* o = (*playList)[i];
		if (o->FileType() == TOggFile::EPlayList)
			AppendLine(arr, COggListBox::EPlayList, *(o->iShortName), (TInt) o);
		else
			AppendLine(arr, COggListBox::EFileName, *(o->iShortName), (TInt) o);
		}
	}

_LIT(KPlayListExt, ".m3u");
TBool TOggFiles::IsPlayListFile(TParsePtrC& p)
	{
	return (p.Ext().CompareF(KPlayListExt) == 0);
	}
