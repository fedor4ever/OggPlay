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

#ifndef _OggFiles_h
#define _OggFiles_h

#include "e32base.h"
#include "gulutil.h"

#include "OggPlay.h"

class TOggPlayback;

class TOggFile
{
 public:

  TOggFile();

  TOggFile(const TBuf<256>& aTitle, 
	   const TBuf<256>& anAlbum,
	   const TBuf<256>& anArtist,
	   const TBuf<256>& aGenre,
	   const TFileName& aSubFolder,
	   const TBuf<512>& aFileName,
	   const TFileName& aShortName);

  TBool Read(TFileText& tf);
  TBool Write(TFileText& tf);

  TBuf<256> iTitle;
  TBuf<256> iAlbum;
  TBuf<256> iArtist;
  TBuf<256> iGenre;
  TBuf<512> iFileName;
  TFileName iSubFolder;
  TFileName iShortName;
};

class TOggKey : public TKeyArrayFix
{
 public:

  TOggKey(TInt anOrder);

  void SetFiles(CArrayFixFlat<TOggFile>* theFiles);

  virtual TAny* At(TInt anIndex) const;

 protected:

  CArrayFixFlat<TOggFile>* iFiles;
  TInt                     iOrder;
  TOggFile*                iSample;
};

class TOggFiles
{
 public:

  TOggFiles(TOggPlayback* anOggPlayback);

  void  CreateDb();
  TBool ReadDb(const TFileName& aFileName, RFs& session);
  void  WriteDb(const TFileName& aFileName, RFs& session);

  void FillTitles(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TDesC& aGenre, const TFileName& aSubFolder);
  void FillAlbums(CDesCArray& arr, const TDesC& anArtist, const TFileName& aSubFolder);
  void FillArtists(CDesCArray& arr, const TFileName& aSubFolder);
  void FillGenres(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TFileName& aSubFolder);
  void FillSubFolders(CDesCArray& arr);
  void FillFileNames(CDesCArray& arr, const TDesC& anAlbum, const TDesC& anArtist, const TDesC& aGenre, const TFileName& aSubFolder);

  static void AppendLine(CDesCArray& arr, COggPlayAppUi::TViews aType, const TDesC& aText, const TBuf<512>& aFileName);

 protected:

  void AddDirectory(const TDesC& aDir);

  CArrayFixFlat<TOggFile>* iFiles; 
  TOggPlayback*            iOggPlayback;
  TOggKey                  iOggKeyTitles;
  TOggKey                  iOggKeyAlbums;
  TOggKey                  iOggKeyArtists;
  TOggKey                  iOggKeyGenres;
  TOggKey                  iOggKeySubFolders;
  TOggKey                  iOggKeyFileNames;

};

#endif
