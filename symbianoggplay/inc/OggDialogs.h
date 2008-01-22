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

#ifndef _OGG_DIALOG_H
#define _OGG_DIALOG_H

#if defined(SERIES60)
#include <e32base.h>
#else
#include <eikdialg.h>
#include <frmtview.h>
#include <txtrich.h>
#include <coemain.h>
#include <barsread.h>
#include <eiksbfrm.h>
#endif

#if defined(SERIES60)
class COggAboutDialog : public CBase
	{
public:
	void ExecuteLD(TInt aResourceId);
	};

class TOggFileInfo;
class COggFileInfoDialog : public CBase
	{
public:
	COggFileInfoDialog(const TOggFileInfo& aFileInfo);
	void ExecuteLD(TInt aResourceId);

private:
	TFileName iFileName;
	TInt iFileSize;
	TInt iTime;
	TInt iBitRate;
	TInt iRate;
	TInt iChannels;
	};

class COggPlayListInfoDialog : public CBase
	{
public:
	COggPlayListInfoDialog(const TDesC& aFileName, TInt aFileSize, TInt aPlayListEntries);
	void ExecuteLD(TInt aResourceId);

private:
	TFileName iFileName;
	TInt iFileSize;
	TInt iPlayListEntries;
	};
#else
class TOggFileInfo;
class COggFileInfoDialog : public CEikDialog
	{
public:
	COggFileInfoDialog(const TOggFileInfo& aFileInfo);

	// From CEikDialog
	void PreLayoutDynInitL();

private:
	TFileName iFileName;
	TInt iFileSize;
	TInt iTime;
	TInt iBitRate;
	TInt iRate;
	TInt iChannels;
	};

class COggPlayListInfoDialog : public CEikDialog
	{
public:
	COggPlayListInfoDialog(const TDesC& aFileName, TInt aFileSize, TInt aPlayListEntries);

	// From CEikDialog
	void PreLayoutDynInitL();

private:
	TFileName iFileName;
	TInt iFileSize;
	TInt iPlayListEntries;
	};
#endif

#if defined(UIQ)
class CHotkeyDialog : public CEikDialog
	{
public:
	CHotkeyDialog(TInt* aHotKeyIdx, TInt* anAlarmActive, TTime* anAlarmTime);

	TBool OkToExitL(TInt aButtonId);
	void PreLayoutDynInitL();

private:
	TInt *iHotKeyIndex;
	TInt *iAlarmActive;
	TTime *iAlarmTime;
	};
#elif !defined(SERIES60)
// Needed by SERIES80 because of small screen, UIQ can do without.
class CScrollableRichTextControl : public CCoeControl
	{
public:
	~CScrollableRichTextControl();

	void UpdateModelL(CRichText * aRichText);
	void Draw(const TRect& aRect) const;
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
	~COggAboutDialog();

	void PreLayoutDynInitL();

private:    
	CParaFormatLayer* iParaFormatLayer;
	CCharFormatLayer* iCharFormatLayer;
	CRichText* iRichText; 
	};

class COggInfoWinDialog : public  CScrollableTextDialog
	{
public:    
    COggInfoWinDialog();
    ~COggInfoWinDialog();

	void SetInfoWinL(const TDesC& msg1, const TDesC& msg2 );
	void PreLayoutDynInitL();
private:    
	CParaFormatLayer* iParaFormatLayer;
	CCharFormatLayer* iCharFormatLayer;
	HBufC *iMsg1;
	HBufC *iMsg2;
	CRichText* iRichText; 
	};
#endif // UIQ
#endif
