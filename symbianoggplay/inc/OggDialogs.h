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

#ifndef _DIALOG_H

#include <eikdialg.h>
#include <frmtview.h>
#include <txtrich.h>
#include <coemain.h>
#include <barsread.h>
#include <eiksbfrm.h>


class CHotkeyDialog : public CEikDialog
{
 public:

  CHotkeyDialog(int *aHotKeyIdx, int* anAlarmActive, TTime* anAlarmTime);

  TBool OkToExitL(TInt aButtonId);
  void PreLayoutDynInitL();
  int *iHotKeyIndex;
  int *iAlarmActive;
  TTime *iAlarmTime;
};

class COggInfoDialog : public CEikDialog
{
 public:

  void SetFileName(const TDesC& aFileName);
  void SetFileSize(TInt aFileSize);
  void SetRate(TInt aRate);
  void SetChannels(TInt aChannels);
  void SetTime(TInt aTime);
  void SetBitRate(TInt aBitRate);

  void PreLayoutDynInitL();
 private:

  TFileName iFileName;
  TInt      iFileSize;
  TInt      iRate;
  TInt      iChannels;
  TInt      iTime;
  TInt      iBitRate;
};

class COggPlayListInfoDialog : public CEikDialog
{
public:

  void SetFileName(const TDesC& aFileName);
  void SetFileSize(TInt aFileSize);
  void SetPlayListEntries(TInt aPlayListEntries);

  void PreLayoutDynInitL();

private:
  TFileName iFileName;
  TInt      iFileSize;
  TInt      iPlayListEntries;
};

#if !(defined(SERIES60) || defined(SERIES80) )

class COggAboutDialog : public CEikDialog
{
 public:

  void SetVersion(const TDesC& aVersion);
  void PreLayoutDynInitL();

 private:

  TBuf<128> iVersion;
};


#else

// Needed by Series 60 because of small screen,
// UIQ can do without.


class CScrollableRichTextControl : public CCoeControl
{
public:

        // second-phase construction
        CScrollableRichTextControl() {}; 
        ~CScrollableRichTextControl(); // destructor
    void UpdateModelL(CRichText * aRichText);
        void Draw(const TRect& aRect) const;
        void Quit(); // does termination
        void Draw(const TRect& /* aRect */);
        TCoeInputCapabilities InputCapabilities() const;
        TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
        
        void ConstructFromResourceL(TResourceReader& aReader);
        
private:
    void UpdateScrollIndicatorL();
        
    CRichText* iRichText; // global text object pointer, not owned.
        TStreamId iStreamId; // required when storing and restoring global text
        // text layout and view stuff
        CTextLayout* iLayout; // text layout
        CTextView* iTextView; // text view
        TRect iViewRect; // rectangle through which to view text
    
    CEikScrollBarFrame * iSBFrame ; //Scrollbar
};

class CScrollableTextDialog : public CEikDialog
{
public:
    
    SEikControlInfo CreateCustomControlL(TInt aControlType);
    void UpdateModelL(CRichText * aRichText);
    
private:
    CScrollableRichTextControl * iScrollableControl;
};



class COggAboutDialog : public  CScrollableTextDialog
{
public:
    
    void SetVersion(const TDesC& aVersion);
    void PreLayoutDynInitL();
    ~COggAboutDialog();
private:
    
    CParaFormatLayer* iParaFormatLayer;
    CCharFormatLayer* iCharFormatLayer;
    TBuf<128> iVersion;
    CRichText* iRichText; 
};

class COggInfoWinDialog : public  CScrollableTextDialog
{
public:
    
    void SetInfoWinL(const TDesC& msg1, const TDesC& msg2 );
    void PreLayoutDynInitL();

    ~COggInfoWinDialog();
private:
    
    CParaFormatLayer* iParaFormatLayer;
    CCharFormatLayer* iCharFormatLayer;
    HBufC *iMsg1;
    HBufC *iMsg2;
    CRichText* iRichText; 
};
#endif

#endif
