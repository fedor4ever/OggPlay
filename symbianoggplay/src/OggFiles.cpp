/*
 *  Copyright (c) 2003 L. H. Wilden. All rights reserved.
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

#include "OggFiles.h"

#include "OggTremor.h"


////////////////////////////////////////////////////////////////
//
// TOggFile
//
////////////////////////////////////////////////////////////////

TOggFile::TOggFile() :
  iTitle(),
  iAlbum(),
  iArtist(),
  iGenre(),
  iFileName(),
  iSubFolder(),
  iShortName()
{
}

TOggFile::TOggFile(const TBuf<256>& aTitle,
		   const TBuf<256>& anAlbum,
		   const TBuf<256>& anArtist,
		   const TBuf<256>& aGenre,
		   const TFileName& aSubFolder,
		   const TBuf<512>& aFileName,
		   const TFileName& aShortName)
{
  iTitle= aTitle;
  iAlbum= anAlbum;
  iArtist= anArtist;
  iGenre= aGenre;
  iSubFolder= aSubFolder;
  iFileName= aFileName;
  iShortName= aShortName;
  if (iTitle.Length()==0) CEikonEnv::Static()->ReadResource(iTitle, R_OGG_STRING_12);
  if (iAlbum.Length()==0) CEikonEnv::Static()->ReadResource(iAlbum, R_OGG_STRING_12);
  if (iArtist.Length()==0) CEikonEnv::Static()->ReadResource(iArtist, R_OGG_STRING_12);
  if (iGenre.Length()==0) CEikonEnv::Static()->ReadResource(iGenre, R_OGG_STRING_12);
}

TBool
TOggFile::Read(TFileText& tf)
{
  return 
    tf.Read(iTitle)==KErrNone &&
    tf.Read(iAlbum)==KErrNone &&
    tf.Read(iArtist)==KErrNone &&
    tf.Read(iGenre)==KErrNone &&
    tf.Read(iSubFolder)==KErrNone &&
    tf.Read(iFileName)==KErrNone &&
    tf.Read(iShortName)==KErrNone;
}

TBool
TOggFile::Write(TFileText& tf)
{
  tf.Write(iTitle);
  tf.Write(iAlbum);
  tf.Write(iArtist);
  tf.Write(iGenre);
  tf.Write(iSubFolder);
  tf.Write(iFileName);
  tf.Write(iShortName);
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
  iSample= new TOggFile();
  SetPtr(iSample);
}

void
TOggKey::SetFiles(CArrayFixFlat<TOggFile>* theFiles)
{
  iFiles= theFiles;
}

TAny*
TOggKey::At(TInt anIndex) const
{
  if (anIndex==KIndexPtr) return(&iSample->iTitle);

  if (anIndex>=0 && anIndex<iFiles->Count()) {
    switch (iOrder) {
    case COggPlayAppUi::ETitle : return &(*iFiles)[anIndex].iTitle;
    case COggPlayAppUi::EAlbum : return &(*iFiles)[anIndex].iAlbum;
    case COggPlayAppUi::EArtist: return &(*iFiles)[anIndex].iArtist;
    case COggPlayAppUi::EGenre : return &(*iFiles)[anIndex].iGenre;
    case COggPlayAppUi::ESubFolder: return &(*iFiles)[anIndex].iSubFolder;
    case COggPlayAppUi::EFileName : return &(*iFiles)[anIndex].iShortName;
    }
  }

  return 0;
}


////////////////////////////////////////////////////////////////
//
// TOggFiles
//
////////////////////////////////////////////////////////////////


TOggFiles::TOggFiles(TOggPlayback* anOggPlayback) :
  iFiles(0),
  iOggPlayback(anOggPlayback),
  iOggKeyTitles(COggPlayAppUi::ETitle),
  iOggKeyAlbums(COggPlayAppUi::EAlbum),
  iOggKeyArtists(COggPlayAppUi::EArtist),
  iOggKeyGenres(COggPlayAppUi::EGenre),
  iOggKeySubFolders(COggPlayAppUi::ESubFolder),
  iOggKeyFileNames(COggPlayAppUi::EFileName)
{
  iFiles= new CArrayFixFlat<TOggFile>(10);
  iOggKeyTitles.SetFiles(iFiles);
  iOggKeyAlbums.SetFiles(iFiles);
  iOggKeyArtists.SetFiles(iFiles);
  iOggKeyGenres.SetFiles(iFiles);
  iOggKeySubFolders.SetFiles(iFiles);
  iOggKeyFileNames.SetFiles(iFiles);
}

void TOggFiles::CreateDb()
{
  TBuf<256> buf;
  CEikonEnv::Static()->ReadResource(buf, R_OGG_STRING_1);
  CEikonEnv::Static()->BusyMsgL(buf);
  iFiles->Reset();
  AddDirectory(_L("D:\\Media files\\audio\\"));
  AddDirectory(_L("C:\\documents\\Media files\\audio\\"));
  AddDirectory(_L("D:\\Media files\\other\\"));
  AddDirectory(_L("C:\\documents\\Media files\\other\\"));
  AddDirectory(_L("D:\\Media files\\document\\"));
  AddDirectory(_L("C:\\documents\\Media files\\document\\"));
  CEikonEnv::Static()->BusyMsgCancel();
}

void TOggFiles::AddDirectory(const TDesC& aDir)
{

  RFs session;
  User::LeaveIfError(session.Connect());

  // Check if the drive is available (i.e. memory stick is inserted)
  //if (session.CheckDisk(aDir)!=KErrNone) return;
 
  CDirScan* ds = CDirScan::NewL(session);
  TRAPD(err,ds->SetScanDataL(aDir,KEntryAttNormal,ESortByName|EAscending,CDirScan::EScanDownTree));
  if (err!=KErrNone) {
    delete ds;
    return;
  }

  CDir* c=0;
  while (1==1) {

    ds->NextL(c);
    if (c==0) break;

    for (TInt i=0; i<c->Count(); i++) {

      const TEntry e= (*c)[i];

      TBuf<512> fullname;
      fullname.Append(ds->FullPath());
      fullname.Append(e.iName);

      TParse p;
      p.Set(fullname,NULL,NULL);

      if (p.Ext()==_L(".ogg") || p.Ext()==_L(".OGG")) {
	
	TBuf<256> shortname;
	shortname.Append(e.iName);
	shortname.Delete(shortname.Length()-4,4); // get rid of the .ogg extension

	TBuf<256> path(ds->AbbreviatedPath());
	if (path.Length()>1 && path[path.Length()-1]==L'\\')
	  path.Delete(path.Length()-1,1); // get rid of trailing back slash

	iOggPlayback->Info(fullname, EFalse);	

	TOggFile o(iOggPlayback->Title(), 
		   iOggPlayback->Album(), 
		   iOggPlayback->Artist(), 
		   iOggPlayback->Genre(), 
		   path,
		   fullname,
		   shortname);

	iFiles->AppendL(o);

      }
    }
    delete c;
  }
  delete ds;
  iOggPlayback->ClearComments();
}

TBool TOggFiles::ReadDb(const TFileName& aFileName, RFs& session)
{
  RFile in;
  if(in.Open(session, aFileName, EFileRead|EFileStreamText)== KErrNone) {

    TFileText tf;
    tf.Set(in);
    
    // check file version:
    TBuf<512> line;
    TInt iVersion= -1;
    if(tf.Read(line)==KErrNone) {
      TLex parse(line);
      parse.Val(iVersion);
    };
    
    if (iVersion==0) {  
      TOggFile o;
      while (tf.Read(line)==KErrNone) {
	if (!o.Read(tf)) break;
	iFiles->AppendL(o);
      }
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
  TInt err= out.Replace(session, aFileName, EFileWrite|EFileStreamText);
  if (err==KErrNone) {

    TFileText tf;
    tf.Set(out);
    
    // write file version:
    TBuf<16> line;
    line.Num(0);
    tf.Write(line);
     
    for (TInt i=0; i<iFiles->Count(); i++) {
      line.Num(i);
      tf.Write(line);
      if (!(*iFiles)[i].Write(tf)) break;
    }
    
    out.Close();
  } else {
    TBuf<16> buf;
    buf.Num(err);
    User::InfoPrint(buf);
  }
  CEikonEnv::Static()->BusyMsgCancel();
}

void
TOggFiles::AppendLine(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aText, const TBuf<512>& aFileName)
{
  TBuf<1024> buf;
  buf.AppendNum(aType);
  buf.Append(KColumnListSeparator);
  buf.Append(aText);
  buf.Append(KColumnListSeparator);
  buf.Append(aFileName);
  arr.AppendL(buf);
}

void TOggFiles::FillTitles(CDesCArray& arr, const TDesC& anAlbum, 
			   const TDesC& anArtist, const TDesC& aGenre, 
			   const TFileName& aSubFolder)
{
  arr.Reset();
  iFiles->Sort(iOggKeyTitles);
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= (*iFiles)[i];
    TBool select=
      (anAlbum.Length()==0 || o.iAlbum==anAlbum) &&
      (anArtist.Length()==0 || o.iArtist==anArtist) &&
      (aGenre.Length()==0 || o.iGenre==aGenre) &&
      (aSubFolder.Length()==0 || o.iSubFolder==aSubFolder);
    if (select)
      AppendLine(arr, COggPlayAppUi::ETitle, o.iTitle, o.iFileName);
  }
}

void TOggFiles::FillAlbums(CDesCArray& arr, const TDesC& anArtist, const TFileName& aSubFolder)
{
  arr.Reset();
  iFiles->Sort(iOggKeyAlbums);
  TBuf<256> lastAlbum;
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= (*iFiles)[i];
    if (aSubFolder.Length()==0 || o.iSubFolder==aSubFolder) {
      if (lastAlbum!=o.iAlbum) {
	AppendLine(arr, COggPlayAppUi::EAlbum, o.iAlbum, o.iAlbum);
	lastAlbum= o.iAlbum;
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
    TOggFile& o= (*iFiles)[i];
    if (aSubFolder.Length()==0 || o.iSubFolder==aSubFolder) {
      if (lastArtist!=o.iArtist) {
	AppendLine(arr, COggPlayAppUi::EArtist, o.iArtist, o.iArtist);
	lastArtist= o.iArtist;
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
    TOggFile& o= (*iFiles)[i];
    TBool select=
      (anAlbum.Length()==0 || o.iAlbum==anAlbum) &&
      (anArtist.Length()==0 || o.iAlbum==anArtist) &&
      (aGenre.Length()==0 || o.iGenre==aGenre) &&
      (aSubFolder.Length()==0 || o.iSubFolder==aSubFolder);
    if (select) 
      AppendLine(arr, COggPlayAppUi::EFileName, o.iShortName, o.iFileName);
  }
}

void TOggFiles::FillSubFolders(CDesCArray& arr)
{
  arr.Reset();
  iFiles->Sort(iOggKeySubFolders);
  TBuf<256> lastSubFolder;
  for (TInt i=0; i<iFiles->Count(); i++) {
    TOggFile& o= (*iFiles)[i];
    if (lastSubFolder!=o.iSubFolder) {
      AppendLine(arr, COggPlayAppUi::ESubFolder, o.iSubFolder, o.iSubFolder);
      lastSubFolder= o.iSubFolder;
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
    TOggFile& o= (*iFiles)[i];
    if (lastGenre!=o.iGenre) {
      TBool select=
	(anAlbum.Length()==0 || o.iAlbum==anAlbum) &&
	(anArtist.Length()==0 || o.iAlbum==anArtist) &&
	(aSubFolder.Length()==0 || o.iSubFolder==aSubFolder);
      if (select) {
	AppendLine(arr, COggPlayAppUi::EGenre, o.iGenre, o.iGenre);
	lastGenre= o.iGenre;
      }
    }
  }
}
