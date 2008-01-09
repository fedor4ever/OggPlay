/*
*  Copyright (c) 2007 S. Fisher
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


#ifndef OGGFILEINFO_H
#define OGGFILEINFO_H

const TInt KMaxCommentLength = 256;
class TOggFileInfo
	{
public:
	TFileName iFileName;
	TInt iBitRate;
	TInt iChannels;
	TInt iFileSize;
	TInt iRate;
	TInt64 iTime;
  
	TBuf<KMaxCommentLength> iAlbum;
	TBuf<KMaxCommentLength> iArtist;
	TBuf<KMaxCommentLength> iGenre;
	TBuf<KMaxCommentLength> iTitle;
	TBuf<KMaxCommentLength> iTrackNumber;
	};
#endif
