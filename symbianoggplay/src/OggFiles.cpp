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

#include "OggOs.h"
#include "OggFiles.h"

#include "OggAbsPlayback.h"
#include "OggLog.h"

#include <eikfutil.h>


TOggPlayList::TOggPlayList()
{
}

TOggPlayList* TOggPlayList::NewL(const TDesC& aSubFolder, const TDesC& aFileName, const TDesC& aShortName)
  {
  TOggPlayList* self = new (ELeave) TOggPlayList();
  CleanupStack::PushL(self);

  self->iSubFolder =  HBufC::NewL(aSubFolder.Length());
  *(self->iSubFolder) = aSubFolder;

  self->iFileName =  HBufC::NewL(aFileName.Length());
  *(self->iFileName) = aFileName;

  self->iShortName =  HBufC::NewL(aShortName.Length());
  *(self->iShortName) = aShortName;

  CleanupStack::Pop(self);
  return self;
  }

TOggPlayList::~TOggPlayList()
{
  delete iSubFolder;
  delete iFileName;
  delete iShortName;
}

////////////////////////////////////////////////////////////////
//
// TOggFile
//
////////////////////////////////////////////////////////////////

TOggFile::TOggFile()
  {
  }

TOggFile* TOggFile::NewL()
  {
  TOggFile* self = new (ELeave) TOggFile();
  CleanupStack::PushL(self);
  self->SetText(self->iTitle,_L("-"));
  self->SetText(self->iAlbum,_L("-"));
  self->SetText(self->iArtist,_L("-"));
  self->SetText(self->iGenre,_L("-"));
  self->SetText(self->iFileName,_L("-"));
  self->SetText(self->iSubFolder,_L("-"));
  self->SetText(self->iShortName,_L("-"));
  self->SetText(self->iTrackNumber,_L("-"));
  self->SetText(self->iTrackTitle,_L("-"));
  CleanupStack::Pop(); // self
  //TRACE(COggLog::VA(_L("TOggFile::NewL() 0x%X"), self ));
  return self;
  }

TOggFile* TOggFile::NewL(const TInt aAbsoluteIndex,
           const TDesC& aTitle,
		   const TDesC& anAlbum,
		   const TDesC& anArtist,
		   const TDesC& aGenre,
		   const TDesC& aSubFolder,
		   const TDesC& aFileName,
		   const TDesC& aShortName,
		   const TDesC& aTrackNumber)
  {
  TOggFile* self = new (ELeave) TOggFile();
  CleanupStack::PushL(self);

  TBuf<128> buf;

#ifdef PLAYLIST_SUPPORT
  CEikonEnv::Static()->ReadResource(buf, R_OGG_STRING_13);
#else
  CEikonEnv::Static()->ReadResource(buf, R_OGG_STRING_12);
#endif

  self->iAbsoluteIndex = aAbsoluteIndex;
  if (aTitle.Length()>0) self->SetText(self->iTitle, aTitle); else self->SetText(self->iTitle, aShortName);
  if (anAlbum.Length()>0) self->SetText(self->iAlbum, anAlbum); else self->SetText(self->iAlbum, buf);
  if (anArtist.Length()>0) self->SetText(self->iArtist, anArtist); else self->SetText(self->iArtist, buf);
  if (aGenre.Length()>0) self->SetText(self->iGenre, aGenre); else self->SetText(self->iGenre,buf);
  if (aSubFolder.Length()>0) self->SetText(self->iSubFolder, aSubFolder); else self->SetText(self->iSubFolder,buf);
  self->SetText(self->iFileName, aFileName);
  self->SetText(self->iShortName, aShortName);
  if (aTrackNumber.Length()>0) self->SetText(self->iTrackNumber, aTrackNumber); else self->SetText(self->iTrackNumber, buf);

  self->SetTrackTitle();
  CleanupStack::Pop(); // self
  //TRACE(COggLog::VA(_L("TOggFile::NewL(...) 0x%X"), self ));
  return self;
  }



void
TOggFile::SetTrackTitle() 
{
  TInt track(0);
  TLex parse(*iTrackNumber);
  parse.Val(track);
  if (track>999) track=999;
  TBuf<128> buf;
  buf.NumFixedWidth(track,EDecimal,2);
  buf.Append(_L(" "));
  buf.Append(iTitle->Left(123));
  SetText(iTrackTitle,buf);
}

TOggFile::~TOggFile()
{
  //TRACE(COggLog::VA(_L("TOggFile::~TOggFile() 0x%X"), this ));
  if (iTitle) delete iTitle;
  if (iAlbum) delete iAlbum;
  if (iArtist) delete iArtist;
  if (iGenre) delete iGenre;
  if (iSubFolder) delete iSubFolder;
  if (iFileName) delete iFileName;
  if (iShortName) delete iShortName;
  if (iTrackNumber) delete iTrackNumber;
  if (iTrackTitle) delete iTrackTitle;
}

void
TOggFile::SetText(HBufC* & aBuffer, const TDesC& aText)
{
  if (aBuffer) delete aBuffer;
  aBuffer= 0;
  aBuffer= HBufC::New(aText.Length());
  *aBuffer= aText;
}

void
TOggFile::SetTextFromFileL(TFileText& aTf, HBufC* & aBuffer)
{
  TBuf<KTFileTextBufferMaxSize> buf;
  User::LeaveIfError( aTf.Read(buf) );

  if (aBuffer) 
  {
      delete(aBuffer);
      aBuffer =NULL;
  }
  
  aBuffer= buf.AllocL();  
    
}

void
TOggFile::ReadL(TFileText& tf, TInt aVersion)
{
  SetTextFromFileL( tf, iTitle );
  SetTextFromFileL( tf, iAlbum );
  SetTextFromFileL( tf, iArtist );
  SetTextFromFileL( tf, iGenre );
  SetTextFromFileL( tf, iSubFolder );
  SetTextFromFileL( tf, iFileName );
  SetTextFromFileL( tf, iShortName );
  if( aVersion == 1 )
    SetTextFromFileL( tf, iTrackNumber );
  SetTrackTitle();
}    


TBool
TOggFile::Write( TInt aLineNumber, HBufC* aBuf )
{
  aBuf->Des().Format(_L("%d\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n%S\n"), 
    aLineNumber, 
    iTitle, iAlbum, iArtist, iGenre, iSubFolder, iFileName, iShortName, iTrackNumber);
 
  return ETrue;
}


////////////////////////////////////////////////////////////////
//
// TOggKey
//
////////////////////////////////////////////////////////////////

TOggKey::TOggKey(TInt anOrder) : 
  TKeyArrayFix(0,ECmpNormal),
  iFiles(0),
  iOrder(anOrder)
{
  iSample= TOggFile::NewL();
  SetPtr(iSample);
}

TOggKey::~TOggKey() 
{
    delete iSample;
}

void
TOggKey::SetFiles(CArrayPtrFlat<TOggFile>* theFiles)
{
  iFiles= theFiles;
}

TAny*
TOggKey::At(TInt anIndex) const
{
  if (anIndex==KIndexPtr) return(&iSample->iTitle);

  if (anIndex>=0 && anIndex<iFiles->Count()) {
    switch (iOrder) {
    case COggPlayAppUi::ETitle : return (*iFiles)[anIndex]->iTitle;
    case COggPlayAppUi::EAlbum : return (*iFiles)[anIndex]->iAlbum;
    case COggPlayAppUi::EArtist: return (*iFiles)[anIndex]->iArtist;
    case COggPlayAppUi::EGenre : return (*iFiles)[anIndex]->iGenre;
    case COggPlayAppUi::ESubFolder: return (*iFiles)[anIndex]->iSubFolder;
    case COggPlayAppUi::EFileName : return (*iFiles)[anIndex]->iShortName;
    case COggPlayAppUi::ETrackTitle: return (*iFiles)[anIndex]->iTrackTitle;
    }
  }

  return 0;
}



////////////////////////////////////////////////////////////////
//
// TOggKeyNumeric
//
////////////////////////////////////////////////////////////////

TOggKeyNumeric::TOggKeyNumeric() : 
  TKeyArrayFix(_FOFF(TOggFile,iAbsoluteIndex),ECmpTInt),
  iFiles(0)
{
  SetPtr(&iInteger); 
}

TOggKeyNumeric::~TOggKeyNumeric() 
{
}

void
TOggKeyNumeric::SetFiles(CArrayPtrFlat<TOggFile>* theFiles)
{
  iFiles= theFiles;
}

TAny*
TOggKeyNumeric::At(TInt anIndex) const
{
  if (anIndex==KIndexPtr) 
      return( (TUint8 *)iPtr + iKeyOffset );

  if (anIndex>=0 && anIndex<iFiles->Count()) {
       return &( (*iFiles)[anIndex]->iAbsoluteIndex );
  }

  return 0;
}


////////////////////////////////////////////////////////////////
//
// TOggFiles
//
////////////////////////////////////////////////////////////////


TOggFiles::TOggFiles(CAbsPlayback* anOggPlayback) :
  iFiles(0),
  iPlayLists(0),
  iOggPlayback(anOggPlayback),
  iOggKeyTitles(COggPlayAppUi::ETitle),
  iOggKeyAlbums(COggPlayAppUi::EAlbum),
  iOggKeyArtists(COggPlayAppUi::EArtist),
  iOggKeyGenres(COggPlayAppUi::EGenre),
  iOggKeySubFolders(COggPlayAppUi::ESubFolder),
  iOggKeyFileNames(COggPlayAppUi::EFileName),
  // iOggKeyPlayList(COggPlayAppUi::EPlayList),
  iOggKeyTrackTitle(COggPlayAppUi::ETrackTitle),
  iOggKeyAbsoluteIndex(),
  iVersion(-1)
{
  iFiles= new(ELeave) CArrayPtrFlat<TOggFile>(10);

#ifdef PLAYLIST_SUPPORT
  iPlayLists= new(ELeave) CArrayPtrFlat<TOggPlayList>(10);
#endif

  iOggKeyTitles.SetFiles(iFiles);
  iOggKeyAlbums.SetFiles(iFiles);
  iOggKeyArtists.SetFiles(iFiles);
  iOggKeyGenres.SetFiles(iFiles);
  iOggKeySubFolders.SetFiles(iFiles);
  iOggKeyFileNames.SetFiles(iFiles);
  // iOggKeyPlayList.SetFiles(iFiles);
  iOggKeyTrackTitle.SetFiles(iFiles);
  iOggKeyAbsoluteIndex.SetFiles(iFiles);

#ifdef PLUGIN_SYSTEM
  iSupportedExtensionList = iOggPlayback->SupportedExtensions();
#endif
}

TOggFiles::~TOggFiles() {
  iFiles->ResetAndDestroy();
  delete iFiles;

#ifdef PLAYLIST_SUPPORT
  iPlayLists->ResetAndDestroy();
  delete iPlayLists;
#endif

#ifdef PLUGIN_SYSTEM
  delete iSupportedExtensionList;
#endif
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
  CEikonEnv::Static()->BusyMsgCancel();
}

void TOggFiles::AddDirectory(const TDesC& aDir,RFs& session)
{
    if (AddDirectoryStart(aDir,session) ==KErrNone) {
        while ( !FileSearchIsProcessDone() ) {
            FileSearchStepL();
        }
    }
    AddDirectoryStop();
}

TInt TOggFiles::AddDirectoryStart(const TDesC& aDir,RFs& session)
{
  TRACE(COggLog::VA(_L("Scanning directory %S for oggfiles"), &aDir ));

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
 
    if (iPathArray == NULL)
    {
        iPathArray = new (ELeave) CDesC16ArrayFlat (3);
    iDs = CDirScan::NewL(session);
        iDirScanFinished = EFalse;
        iDirScanSession = &session;
        iDirScanDir = const_cast < TDesC *> (&aDir);
        iNbDirScanned = 0;
        iNbFilesFound = 0;
        iCurrentIndexInDirectory = 0;
        iCurrentDriveIndex = 0;
        iDirectory=NULL;
    TRAPD(err,iDs->SetScanDataL(aDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
  if (err!=KErrNone) 
    {
    TRACEF(COggLog::VA(_L("Unable to setup scan directory %S for oggfiles"), &aDir ));
        delete iDs; iDs=0;
        return KErrNotFound;
    }
    }
    iPathArray->AppendL(aDir);
    return KErrNone;
}

void TOggFiles::AddDirectoryStop()
{
    delete iDs; iDs=0;
    delete iDirectory; iDirectory = 0;
    if (iOggPlayback)
        iOggPlayback->ClearComments();
    if (iPathArray)
        iPathArray->Reset();
    delete iPathArray; iPathArray = 0;
    
}

void  TOggFiles::FileSearchStepL()
{
    if (iCurrentIndexInDirectory == 0)
	{
		// Add New directory
		if (!NextDirectory())
			return;
    }

    for (; iCurrentIndexInDirectory<iDirectory->Count(); iCurrentIndexInDirectory++) 
    {
		// Add new files
		if (AddNextFileL())
			break;
	}

    if (iCurrentIndexInDirectory == iDirectory->Count())
	{
        // Go to next directory
        iCurrentIndexInDirectory = 0;
	}
}

TBool TOggFiles::NextDirectory()
{
	// Move on to the next directory
    delete iDirectory;
    iDirectory = NULL;

	TRAPD(err,iDs->NextL(iDirectory));
    if (err!=KErrNone) 
    {
        TRACEF(COggLog::VA(_L("Unable to scan directory %S for oggfiles"), &iDirScanDir ));
        delete iDs; iDs=0;
        iDirScanFinished = ETrue;
        return EFalse;
    }
        
    if (iDirectory==0) 
    {
        // No more files in that drive.
        // Any other drive to search in?
        iCurrentDriveIndex++;
        if (iCurrentDriveIndex < iPathArray->Count() ){
            // More drives to search
            TRAPD(err,iDs->SetScanDataL((*iPathArray)[iCurrentDriveIndex],
                KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
            if (err!=KErrNone) 
            {
                TPtrC aBuf((*iPathArray)[iCurrentDriveIndex]);
                TRACEF(COggLog::VA(_L("Unable to setup scan directory %S for oggfiles"), &aBuf ));
                delete iDs; iDs=0;
                iDirScanFinished = ETrue;
            }

            return EFalse;
        }
		else
		{
            // No more directories to search
            iDirScanFinished = ETrue;
            return EFalse;
        }
    }

    iNbDirScanned++;
	return ETrue;
}

TBool TOggFiles::AddNextFileL()
{
    TBuf<256> shortname;
	TBool fileFound = EFalse; 
	const TEntry e= (*iDirectory)[iCurrentIndexInDirectory];
        
    iFullname.Copy(iDs->FullPath());
    iFullname.Append(e.iName);
        
    TParsePtrC p(iFullname);
    if (IsSupportedAudioFile(p)) 
    {
        shortname.Copy(e.iName);
        shortname.Delete(shortname.Length()-4,4); // get rid of the .ogg extension
        iPath.Copy(iDs->AbbreviatedPath());
            
        if (iPath.Length()>1 && iPath[iPath.Length()-1]==L'\\')
            iPath.SetLength(iPath.Length()-1); // get rid of trailing back slash
            
        TInt err = iOggPlayback->Info(iFullname, ETrue);
        if( err == KErrNone )
        {
            TOggFile* o = TOggFile::NewL(iNbFilesFound,
                iOggPlayback->Title(), 
                iOggPlayback->Album(), 
                iOggPlayback->Artist(), 
                iOggPlayback->Genre(), 
                iPath,
                iFullname,
                shortname,
                iOggPlayback->TrackNumber());
                
            iFiles->AppendL(o);
            iNbFilesFound++;
			iCurrentIndexInDirectory++;
			fileFound = ETrue;
        }
        else
        {
            TRACEF(COggLog::VA(_L("Failed with code %d"), err ));
        }
    }
#ifdef PLAYLIST_SUPPORT
	else if (IsPlayListFile(p))
	{
		// Add the playlist to the list of playlists.
        shortname.Copy(e.iName);
        shortname.Delete(shortname.Length()-4,4); // get rid of the .m3u extension
        iPath.Copy(iDs->AbbreviatedPath());
            
        if (iPath.Length()>1 && iPath[iPath.Length()-1]==L'\\')
            iPath.SetLength(iPath.Length()-1); // get rid of trailing back slash
            
        TOggPlayList* o = TOggPlayList::NewL(iPath, iFullname, shortname);
        iPlayLists->AppendL(o);
        
		// iNbFilesFound++;
		iCurrentIndexInDirectory++;
		fileFound = ETrue;
	}
#endif

	return fileFound;
}
	
TBool TOggFiles::FileSearchIsProcessDone() const
{
    return iDirScanFinished;
};

void  TOggFiles::FileSearchProcessFinished()
{// Not used
};
void  TOggFiles::FileSearchDialogDismissedL(TInt /*aButtonId*/)
{// Not used
};

TInt  TOggFiles::FileSearchCycleError(TInt aError)
{// Not used
    return(aError);
};
void  TOggFiles::FileSearchGetCurrentStatus(TInt &aNbDir, TInt &aNbFiles)
{
    aNbDir = iNbDirScanned;
    aNbFiles = iNbFilesFound;
};

TInt TOggFiles::SearchAllDrives(CEikDialog * aDialog, TInt aDialogID,RFs& session)
{
    ClearFiles();
    
    TChar driveLetter; TDriveInfo driveInfo;
    TInt err=KErrNone;
    TInt driveNumber;
    for (driveNumber=EDriveA; driveNumber<=EDriveZ; driveNumber++) {
      session.Drive(driveInfo,driveNumber); 
      if (driveInfo.iDriveAtt == (TUint)KDriveAbsent)
          continue; 
      if ( (driveInfo.iType != (TUint) EMediaRom) 
          && (driveInfo.iType != (TUint) EMediaNotPresent)
#ifdef __WINS__
          // For some reasons, the emulator finds a non-existing drive X, which blows up everything...
          && (driveInfo.iType != EMediaHardDisk) 
#endif
          ){
          TRAP(err, session.DriveToChar(driveNumber,driveLetter));
          if (err) break;
          TBuf <10> driveName;
          driveName.Format(_L("%c:\\"), driveLetter);
          err = AddDirectoryStart(driveName,session);
          if (err) break;
      }
    }
    if ( err == KErrNone) {
        TRAP(err, aDialog->ExecuteLD(aDialogID));
    }
    else {
        delete (aDialog);
        aDialog = NULL;
    }
    AddDirectoryStop();
    return (err);
}

TInt TOggFiles::SearchSingleDrive(const TDesC& aDir, CEikDialog * aDialog, TInt aDialogID,RFs& session)
{
    ClearFiles();
    if (!EikFileUtils::PathExists(aDir)) 
    {
      TRACEF(COggLog::VA(_L("Folder %S doesn't exist"), &aDir ));
      TBuf<256> buf1,buf2;
      CEikonEnv::Static()->ReadResource(buf2, R_OGG_ERROR_24);
      buf1.Append(aDir);
      ((COggPlayAppUi*)CEikonEnv::Static()->AppUi())->iOggMsgEnv->OggErrorMsgL(buf2,buf1);
      return KErrNotFound;
    }
    TInt err = AddDirectoryStart(aDir,session);
    if ( err == KErrNone) {
        TRAP(err, aDialog->ExecuteLD(aDialogID));
    }
    else {
        delete (aDialog);
        aDialog = NULL;
    }
    AddDirectoryStop();
    return (err);
}

TBool TOggFiles::CreateDbWithSingleFile(const TDesC& aFile){

  TParsePtrC p(aFile);	
	if (IsSupportedAudioFile(p)) {
		
		ClearFiles();
#if defined(SERIES60)  
		AddFile(aFile);
#endif
		if (iFiles->Count()) {
			return ETrue;
		}
	}
	return EFalse;
}

void TOggFiles::AddFile(const TDesC& aFile){
	//_LIT(KS,"adding File %s to oggfiles");
	//OGGLOG.WriteFormat(KS,aFile.Ptr());
	
	TParsePtrC p(aFile);

	if (IsSupportedAudioFile(p)) {
		
		iFullname.Copy(aFile);

		TBuf<256> shortname;
		shortname.Append(p.Name());

		iPath.Copy(p.DriveAndPath());
		if (iPath.Length()>1 && iPath[iPath.Length()-1]==L'\\'){
			iPath.Delete(iPath.Length()-1,1); // get rid of trailing back slash
		}
		if ( iOggPlayback->Info(aFile, EFalse) == KErrNone) {

			TOggFile* o = TOggFile::NewL(
                iFiles->Count(),
                iOggPlayback->Title(), 
				iOggPlayback->Album(), 
				iOggPlayback->Artist(), 
				iOggPlayback->Genre(), 
				iPath,
				iFullname,
				shortname,
				iOggPlayback->TrackNumber());
			
			TRAPD(err, iFiles->AppendL(o));
		} 	
	}
	iOggPlayback->ClearComments();
}

TBool TOggFiles::ReadDb(const TFileName& aFileName, RFs& session)
{
  RFile in;
  if(in.Open(session, aFileName, EFileRead|EFileStreamText)== KErrNone) {

    TFileText tf;
    tf.Set(in);
    
    // check file version:
    TBuf<KTFileTextBufferMaxSize> line;
    iVersion= -1;
    if(tf.Read(line)==KErrNone) {
      TLex parse(line);
      parse.Val(iVersion);
    };
    
    if (iVersion!=0 && iVersion!=1) {
      in.Close();
      return EFalse;
    }

    while (tf.Read(line)==KErrNone) 
      {
      TOggFile* o = TOggFile::NewL();
      TRAPD( err, o->ReadL(tf,iVersion) );
      if( err )
        break;
      o->iAbsoluteIndex = iFiles->Count();
      iFiles->AppendL(o);
      }

    in.Close();
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
  if (err==KErrNone) {

    TFileText tf;
    tf.Set(out);
    
    // Write file version:
    TBuf<16> line;
    line.Num(1);
    tf.Write(line);
     
    HBufC* dat = HBufC::NewLC(KDiskWriteBufferSize);
    HBufC* tempBuf = HBufC::NewLC(KTFileTextBufferMaxSize*KNofOggDescriptors);

    // Buffer up writes. Speed issue on phones without MMC write cache.
    if (err==KErrNone) {
      for (TInt i=0; i<iFiles->Count(); i++) {
        if (!(*iFiles)[i]->Write(i,tempBuf)) 
            break;
          TInt newSize = dat->Length() + tempBuf->Length();
          if( KDiskWriteBufferSize < newSize ) {
            // Write to disk. Remove the last '\n' since Write() will add this
            dat->Des().SetLength( dat->Des().Length()-1 ); 
            tf.Write(dat->Des());
            dat->Des().Zero();
            }
          dat->Des().Append(tempBuf->Des());
        }
      }

    // Final write to disk.
    if( dat->Des().Length() > 0 )
      {
      dat->Des().SetLength( dat->Des().Length()-1 ); 
      tf.Write(dat->Des());
      }

    CleanupStack::PopAndDestroy(2); // dat + tempBuf

    if (err!=KErrNone) {
      buf.Append(_L("Error at write: "));
      buf.AppendNum(err);
      User::InfoPrint(buf);
      User::After(TTimeIntervalMicroSeconds32(1000000));
    }
    out.Close();
    CEikonEnv::Static()->BusyMsgCancel();

  } else {
    CEikonEnv::Static()->BusyMsgCancel();
    buf.SetLength(0);
    buf.Append(_L("Error at open: "));
    buf.AppendNum(err);
    User::InfoPrint(buf);
    User::After(TTimeIntervalMicroSeconds32(1000000));
  }
  
}

void
TOggFiles::AppendLine(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aText, const TInt anAbsoluteIndex)
{
  HBufC* hbuf = HBufC::NewLC(1024);
  hbuf->Des().AppendNum((TInt) aType);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().Append(aText);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().AppendNum(anAbsoluteIndex);
  arr.AppendL(hbuf->Des());
  CleanupStack::PopAndDestroy();
}

void
TOggFiles::AppendLine(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aText, const TDesC& anInternalText)
{
  HBufC* hbuf = HBufC::NewLC(1024);
  hbuf->Des().AppendNum((TInt) aType);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().Append(aText);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().Append(anInternalText);
  arr.AppendL(hbuf->Des());
  CleanupStack::PopAndDestroy();
}

void
TOggFiles::AppendTitleAndArtist(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aTitle, const TDesC& aDelim, const TDesC& aArtist, const TInt anAbsoluteIndex)
{
  
  HBufC* hbuf = HBufC::NewLC(1024);
  hbuf->Des().AppendNum((TInt) aType);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().Append(aTitle);
  hbuf->Des().Append(aDelim);
  hbuf->Des().Append(aArtist);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().AppendNum(anAbsoluteIndex);
  arr.AppendL(hbuf->Des());
  CleanupStack::PopAndDestroy();
}

void
TOggFiles::ClearFiles()
{
  //for (TInt i=0; i<iFiles->Count(); i++) delete (*iFiles)[i];
  //iFiles->Reset();

  iFiles->ResetAndDestroy();
}


TDesC & 
TOggFiles::FindFromIndex(TInt anIndex)
{
    _LIT(Empty, "-");
    TOggFile * o = TOggFile::NewL(anIndex, Empty, Empty, Empty, Empty, Empty, Empty, Empty, Empty);
    CleanupStack::PushL(o);
    TInt foundIdx=-1;
    iFiles->Find( (TOggFile * const &) *o, iOggKeyAbsoluteIndex, foundIdx );
    CleanupStack::PopAndDestroy();
    return *(*iFiles)[foundIdx]->iFileName;
}

TOggFile* TOggFiles::FindFromFileNameL(TFileName& aFileName)
{
    _LIT(Empty, "-");
    TOggFile* o = TOggFile::NewL(-1, Empty, Empty, Empty, Empty, Empty, aFileName, Empty, Empty);

	TInt foundIdx;
    // TInt notFound = iFiles->Find((TOggFile * const &) *o, iOggKeyPlayList, foundIdx );
	TBool notFound = ETrue;
	for (foundIdx = 0 ; foundIdx < iFiles->Count() ; foundIdx++)
	{
		TOggFile* o2 = (*iFiles)[foundIdx];
		if (!(o2->iFileName->Compare(*o->iFileName)))
		{
			notFound = EFalse;
			break;
		}
	}

	delete o;
	return (notFound) ? NULL : (*iFiles)[foundIdx];
}

void 
TOggFiles::FillTitles(CDesCArray& arr, const TDesC& anAlbum, 
			   const TDesC& anArtist, const TDesC& aGenre, 
			   const TDesC& aSubFolder)
{
  arr.Reset();

  if (anAlbum.Length()>0) 
	  iFiles->Sort(iOggKeyTrackTitle); 
  else {
	  if (aSubFolder.Length()>0) 
		  iFiles->Sort(iOggKeyFileNames); 
	  else 
		  iFiles->Sort(iOggKeyTitles);
	}

  if( anAlbum.Length()==0 && anArtist.Length()==0  && aGenre.Length()==0  && aSubFolder.Length()==0 ) {
    TBuf<10> buf;
    CEikonEnv::Static()->ReadResource(buf, R_OGG_BY);
    for (TInt i=0; i<iFiles->Count(); i++) {
      TOggFile& o= *(*iFiles)[i];
      AppendTitleAndArtist(arr, COggPlayAppUi::ETitle, o.iTitle->Des(), buf, o.iArtist->Des(),o.iAbsoluteIndex);
    }

  } else {
    for (TInt i=0; i<iFiles->Count(); i++) {
      TOggFile& o= *(*iFiles)[i];
      TBool select=
        (anAlbum.Length()==0 || *o.iAlbum==anAlbum) &&
        (anArtist.Length()==0 || *o.iArtist==anArtist) &&
        (aGenre.Length()==0 || *o.iGenre==aGenre) &&
        (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);
      if (select)
        AppendLine(arr, COggPlayAppUi::ETitle, o.iTitle->Des(), o.iAbsoluteIndex);
    }
  }
}

void TOggFiles::FillAlbums(CDesCArray& arr, const TDesC& /*anArtist*/, const TFileName& aSubFolder)
{
  arr.Reset();
  iFiles->Sort(iOggKeyAlbums);
  TBuf<256> lastAlbum;
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= *(*iFiles)[i];
    if (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder) {
      if (lastAlbum!=*o.iAlbum) {
	AppendLine(arr, COggPlayAppUi::EAlbum, *o.iAlbum, *o.iAlbum);
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
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= *(*iFiles)[i];
    if (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder) {
      if (lastArtist!=*o.iArtist) {
	AppendLine(arr, COggPlayAppUi::EArtist, *o.iArtist, *o.iArtist);
	lastArtist= *o.iArtist;
      }
    }
  }
}

void TOggFiles::FillFileNames(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, 
			      const TDesC& aGenre, const TFileName& aSubFolder)
{
  arr.Reset();
  iFiles->Sort(iOggKeyFileNames);
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= *(*iFiles)[i];
    TBool select=
      (anAlbum.Length()==0 || *o.iAlbum==anAlbum) &&
      (anArtist.Length()==0 || *o.iArtist==anArtist) &&
      (aGenre.Length()==0 || *o.iGenre==aGenre) &&
      (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);
    if (select) 
      AppendLine(arr, COggPlayAppUi::EFileName, *o.iShortName, o.iAbsoluteIndex);
  }
}

void TOggFiles::FillSubFolders(CDesCArray& arr)
{
  arr.Reset();
  iFiles->Sort(iOggKeySubFolders);
  TBuf<256> lastSubFolder;
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= *(*iFiles)[i];
    if (lastSubFolder!=*o.iSubFolder) {
      AppendLine(arr, COggPlayAppUi::ESubFolder, *o.iSubFolder, *o.iSubFolder);
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
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= *(*iFiles)[i];
    if (lastGenre!=*o.iGenre) {
      TBool select=
	(anAlbum.Length()==0 || *o.iAlbum==anAlbum) &&
	(anArtist.Length()==0 || *o.iAlbum==anArtist) &&
	(aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);
      if (select) {
	AppendLine(arr, COggPlayAppUi::EGenre, *o.iGenre, *o.iGenre);
	lastGenre= *o.iGenre;
      }
    }
  }
}

#ifdef PLAYLIST_SUPPORT
void TOggFiles::FillPlayLists(CDesCArray& arr)
{
  arr.Reset();
  for (TInt i=0; i<iPlayLists->Count(); i++)
  {
	  TOggPlayList& o = *(*iPlayLists)[i];
	  AppendLine(arr, COggPlayAppUi::EPlayList, *o.iShortName, *o.iFileName);
  }
}

void TOggFiles::FillPlayList(CDesCArray& arr, const TDesC& aPlayListFile)
{
  arr.Reset();
  
  RFs fs;
  TInt err = fs.Connect();

  RFile file;
  err = file.Open(fs, aPlayListFile, EFileShareReadersOnly);
  
  TInt fileSize;
  err = file.Size(fileSize);
  HBufC8* buf = HBufC8::NewL(fileSize);
  TPtr8 bufPtr(buf->Des());
  err = file.Read(bufPtr);

  TInt i = 0;
  TInt j = 0;
  while (j<fileSize)
  {
	i = j;
	TPtr8 linePtr(bufPtr);
	for ( ; i<fileSize ; i++)
	{
		TText8 nextChar = linePtr[i];
		if ((nextChar == ' ') || (nextChar == '\n') || (nextChar == '\r') || (nextChar == '\t'))
			continue;

		break;
	}

	for (j = i ; j<fileSize ; j++)
	{
		TText8 nextChar = linePtr[j];
		if (nextChar == '\n')
			break;
	}
	if (j != fileSize) j--;

	TFileName fileName(aPlayListFile);
	while (fileName[fileName.Length()-1] != '\\')
		fileName.SetLength(fileName.Length()-1);

	TFileName linePtr16;
	linePtr16.Copy(linePtr.Mid(i, j-i));
	fileName.Append(linePtr16);

	TOggFile* o = FindFromFileNameL(fileName);
	if (o)
		AppendLine(arr, COggPlayAppUi::EFileName, *(o->iShortName), o->iAbsoluteIndex);
  }
}
#endif

#ifdef PLUGIN_SYSTEM
TBool TOggFiles::IsSupportedAudioFile(TParsePtrC& p)
{
    TBool result=EFalse;
    if (p.ExtPresent())
    {
        TPtrC pp (p.Ext().Mid(1)); // Remove the . in front of the extension
      
        for (TInt i=0; i<iSupportedExtensionList->Count(); i++)
        {
            result=(pp.CompareF( (*iSupportedExtensionList)[i] ) == 0 );  
            if (result)
                break;
        }
    }
    return result;
}
#else /*PLUGIN_SYSTEM */

_LIT(KOggExt, ".ogg");
_LIT(KOggExtUC, ".OGG");
TBool TOggFiles::IsSupportedAudioFile(TParsePtrC& p)
{

  TBool result= (p.Ext().Compare(KOggExt)==0) || (p.Ext().Compare(KOggExtUC)==0); 

#ifdef MP3_SUPPORT
  result=result || (p.Ext().Compare( _L(".mp3"))==0 || p.Ext().Compare( _L(".MP3"))==0);
#endif

  return result;
}
#endif /* PLUGIN_SYSTEM */

_LIT(KPlayListExt, ".m3u");
_LIT(KPlayListExtUC, ".M3U");
TBool TOggFiles::IsPlayListFile(TParsePtrC& p)
{
  return (p.Ext().Compare(KPlayListExt) == 0) || (p.Ext().Compare(KPlayListExtUC) == 0);
}
