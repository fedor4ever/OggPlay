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

#include "OggTremor.h"
#include "OggLog.h"

#include <eikfutil.h>

const TInt KTFileTextBufferMaxSize = 0x100;
const TInt KMaxFileLength = 0x200;

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

TOggFile* TOggFile::NewL(const TDesC& aTitle,
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
  CEikonEnv::Static()->ReadResource(buf, R_OGG_STRING_12);

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
    aBuffer= aBuffer->ReAllocL(buf.Length());  
  else
    aBuffer= HBufC::NewL(buf.Length());  
    
  *aBuffer = buf;
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
TOggFile::Write(TFileText& tf)
{
  tf.Write(*iTitle);
  tf.Write(*iAlbum);
  tf.Write(*iArtist);
  tf.Write(*iGenre);
  tf.Write(*iSubFolder);
  tf.Write(*iFileName);
  tf.Write(*iShortName);
  tf.Write(*iTrackNumber);
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
// TOggFiles
//
////////////////////////////////////////////////////////////////


TOggFiles::TOggFiles(COggPlayback* anOggPlayback) :
  iFiles(0),
  iOggPlayback(anOggPlayback),
  iOggKeyTitles(COggPlayAppUi::ETitle),
  iOggKeyAlbums(COggPlayAppUi::EAlbum),
  iOggKeyArtists(COggPlayAppUi::EArtist),
  iOggKeyGenres(COggPlayAppUi::EGenre),
  iOggKeySubFolders(COggPlayAppUi::ESubFolder),
  iOggKeyFileNames(COggPlayAppUi::EFileName),
  iOggKeyTrackTitle(COggPlayAppUi::ETrackTitle),
  iVersion(-1)
{
  iFiles= new(ELeave) CArrayPtrFlat<TOggFile>(10);
  iOggKeyTitles.SetFiles(iFiles);
  iOggKeyAlbums.SetFiles(iFiles);
  iOggKeyArtists.SetFiles(iFiles);
  iOggKeyGenres.SetFiles(iFiles);
  iOggKeySubFolders.SetFiles(iFiles);
  iOggKeyFileNames.SetFiles(iFiles);
  iOggKeyTrackTitle.SetFiles(iFiles);
}

TOggFiles::~TOggFiles() {
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
  CEikonEnv::Static()->BusyMsgCancel();
}

void TOggFiles::AddDirectory(const TDesC& aDir,RFs& session)
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
    return;
    }
 
  CDirScan* ds = CDirScan::NewL(session);
  TRAPD(err,ds->SetScanDataL(aDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
  if (err!=KErrNone) 
    {
    TRACEF(COggLog::VA(_L("Unable to setup scan directory %S for oggfiles"), &aDir ));
    delete ds; ds=0;
    return;
    }

  CDir* c=0;
  for(;;) 
    {
    TRAPD(err,ds->NextL(c));
    if (err!=KErrNone) 
      {
      TRACEF(COggLog::VA(_L("Unable to scan directory %S for oggfiles"), &aDir ));
      delete ds; ds=0;
      return;
      }

	  if (c==0) break;

    HBufC* fullname = HBufC::NewLC(KMaxFileLength);
    HBufC* path = HBufC::NewLC(KMaxFileLength);

    for (TInt i=0; i<c->Count(); i++) 
      {
      const TEntry e= (*c)[i];

      fullname->Des().Copy(ds->FullPath());
      fullname->Des().Append(e.iName);

      TParsePtrC p( fullname->Des() );

      if (p.Ext().Compare( _L(".ogg"))==0 || p.Ext().Compare( _L(".OGG"))==0 ) 
        {
    	  TBuf<256> shortname;
	      shortname.Append(e.iName);
	      shortname.Delete(shortname.Length()-4,4); // get rid of the .ogg extension
        path->Des().Copy(ds->AbbreviatedPath());

	      if (path->Des().Length()>1 && path->Des()[path->Des().Length()-1]==L'\\')
	        path->Des().SetLength( path->Des().Length()-1); // get rid of trailing back slash

        //TRACE(COggLog::VA(_L("Processing %S"), &fullname ));
	      TInt err = iOggPlayback->Info(*fullname, ETrue);

        if( err == KErrNone )
          {
          TOggFile* o = TOggFile::NewL(iOggPlayback->Title(), 
				        iOggPlayback->Album(), 
				        iOggPlayback->Artist(), 
				        iOggPlayback->Genre(), 
				        *path,
				        *fullname,
				        shortname,
				        iOggPlayback->TrackNumber());
	      
	        iFiles->AppendL(o);
          }
        else
          {
          TRACEF(COggLog::VA(_L("Failed with code %d"), err ));
          }
        }
      }
    delete c; c=0;
    CleanupStack::PopAndDestroy(2); // fullname + path
    }
  delete ds; c=0;
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
      iFiles->AppendL(o);
      }

    in.Close();
    return ETrue;
  }
  return EFalse;
}

void TOggFiles::WriteDb(const TFileName& aFileName, RFs& session)
{
  CEikonEnv::Static()->BusyMsgL(R_OGG_STRING_2);
  RFile out;

  TBuf<64> buf;
  TInt err= out.Replace(session, aFileName, EFileWrite|EFileStreamText);
  if (err==KErrNone) {

    TFileText tf;
    tf.Set(out);
    
    // write file version:
    TBuf<16> line;
    line.Num(1);
    tf.Write(line);
     
    if (err==KErrNone) {

      for (TInt i=0; i<iFiles->Count(); i++) {
	line.Num(i);
	tf.Write(line);
	if (!(*iFiles)[i]->Write(tf)) break;
      }
    }
    
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
    //OGGLOG.WriteFormat(_L("Error %d in WriteDB"),err);
  }
  
}

void
TOggFiles::AppendLine(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aText, const TDesC& aFileName)
{
  HBufC* hbuf = HBufC::NewLC(1024);
  hbuf->Des().AppendNum((TInt) aType);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().Append(aText);
  hbuf->Des().Append(KColumnListSeparator);
  hbuf->Des().Append(aFileName);
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

void 
TOggFiles::FillTitles(CDesCArray& arr, const TDesC& anAlbum, 
			   const TDesC& anArtist, const TDesC& aGenre, 
			   const TDesC& aSubFolder)
{
  arr.Reset();
  if (anAlbum.Length()>0) iFiles->Sort(iOggKeyTrackTitle); else iFiles->Sort(iOggKeyTitles);
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= *(*iFiles)[i];
    TBool select=
      (anAlbum.Length()==0 || *o.iAlbum==anAlbum) &&
      (anArtist.Length()==0 || *o.iArtist==anArtist) &&
      (aGenre.Length()==0 || *o.iGenre==aGenre) &&
      (aSubFolder.Length()==0 || *o.iSubFolder==aSubFolder);
    if (select)
      AppendLine(arr, COggPlayAppUi::ETitle, o.iTitle->Des(), o.iFileName->Des());
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
      AppendLine(arr, COggPlayAppUi::EFileName, *o.iShortName, *o.iFileName);
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
