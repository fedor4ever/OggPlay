/*
 *  Copyright (c) 2004 P. Wolff
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

#include "TremorDecoder.h"
#include "OggLog.h"
#include "Utf8Fix.h"

#include <charconv.h>
#include <limits.h>
#include <string.h>
#include <utf.h>
#include <f32file.h>


CTremorDecoder::~CTremorDecoder() 
{
  ov_clear(&iVf);
}

int CTremorDecoder::Clear() 
{
  return ov_clear(&iVf);
}

int CTremorDecoder::Open(FILE* f,const TDesC& /*aFilename*/) 
{
  Clear();
  int ret=ov_open(f, &iVf, NULL, 0);
  if(ret>=0) {
    vi= ov_info(&iVf,-1);
  }
  return ret;
}

int CTremorDecoder::OpenInfo(FILE* f,const TDesC& /*aFilename*/) 
{
  Clear();
  int ret=ov_test(f, &iVf, NULL, 0);
  if(ret>=0) {
    vi= ov_info(&iVf,-1);
  }
  return ret;
}

int CTremorDecoder::Close(FILE*) 
{
  return Clear();
}

int CTremorDecoder::Read(TDes8& aBuffer,int Pos) 
{
  return ov_read( &iVf,(char *) &(aBuffer.Ptr()[Pos]),
        aBuffer.MaxLength()-Pos,&iCurrentSection);
}

void CTremorDecoder::ParseTags(TDes& aTitle, TDes& aArtist, TDes& aAlbum, TDes& aGenre, TDes& aTrackNumber) 
{
  char** ptr=ov_comment(&iVf,-1)->user_comments;
  aArtist.SetLength(0);
  aTitle.SetLength(0);
  aAlbum.SetLength(0);
  aGenre.SetLength(0);
  aTrackNumber.SetLength(0);
  while (*ptr) {
    char* s= *ptr;
    TBuf<256> buf;
    GetString(buf,s);
    buf.UpperCase();
    if (buf.Find(_L("ARTIST="))==0) GetString(aArtist, s+7); 
    else if (buf.Find(_L("TITLE="))==0) GetString(aTitle, s+6);
    else if (buf.Find(_L("ALBUM="))==0) GetString(aAlbum, s+6);
    else if (buf.Find(_L("GENRE="))==0) GetString(aGenre, s+6);
    else if (buf.Find(_L("TRACKNUMBER="))==0) GetString(aTrackNumber,s+12);
    ++ptr;
  }
}

void CTremorDecoder::GetString(TDes& aBuf, const char* aStr)
{
  // according to ogg vorbis specifications, the text should be UTF8 encoded
  TPtrC8 p((const unsigned char *)aStr);
  CnvUtfConverter::ConvertToUnicodeFromUtf8(aBuf,p);

  // in the real world there are all kinds of coding being used!
  // so we try to find out what it is:

#if ! ( defined(__VC32__) || defined(__WINSCW__) )
  TInt i= j_code((char*)aStr,strlen(aStr));
  if (i==BIG5_CODE) {
    CCnvCharacterSetConverter* conv= CCnvCharacterSetConverter::NewL();
    RFs theRFs;
    theRFs.Connect();
    CCnvCharacterSetConverter::TAvailability a= conv->PrepareToConvertToOrFromL(KCharacterSetIdentifierBig5, theRFs);
    if (a==CCnvCharacterSetConverter::EAvailable) {
      TInt theState= CCnvCharacterSetConverter::KStateDefault;
      conv->ConvertToUnicode(aBuf, p, theState);
    }
    theRFs.Close();
    delete conv;
  }
#endif
}


TInt CTremorDecoder::Channels()
{
  return vi->channels;
}

TInt CTremorDecoder::Rate()
{
  return vi->rate;
}

TInt CTremorDecoder::Bitrate()
{
  return vi->bitrate_nominal;
}

TInt64 CTremorDecoder::Position()
{ 
  ogg_int64_t pos= ov_time_tell(&iVf);
  unsigned int hi(0);
  return TInt64(hi,(TInt)pos);
}

void CTremorDecoder::Setposition(TInt64 aPosition)
{ 
  ogg_int64_t pos= aPosition.GetTInt();
  ov_time_seek( &iVf, pos);

}

TInt64 CTremorDecoder::TimeTotal()
{
  
  TInt64 iTime;
  
  ogg_int64_t pos(0);
  pos= ov_time_total(&iVf,-1);
  unsigned int hi(0);
  iTime.Set(hi,(TInt)pos);
  
  return iTime;
  
}

TInt CTremorDecoder::FileSize()
{
  return (TInt) ov_raw_total(&iVf,-1);
}

void CTremorDecoder::GetFrequencyBins(TInt32* aBins,TInt NumberOfBins)
{
  TBool active=EFalse;
  if(NumberOfBins) active=ETrue;
  ov_getFreqBin(&iVf, active, aBins);  
}
