/*
 *  Copyright (c) 2005 OggPlay Team
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

#ifndef _OGGDIALOGSS80_H

#include "OggPlay.h"
#include <eikdialg.h>
#include <frmtview.h>
#include <txtrich.h>
#include <coemain.h>
#include <barsread.h>
#include <eiksbfrm.h>
#include <eikchlst.h>
#include <eikchkbx.h>
#include <eiklbbut.h>
#include <badesca.h>
#include <eiktxlbm.h>
#include <ckninfrm.h>
#include <eikclb.h>


class CCodecsLabel;
     
class CSettingsS80Dialog : public CEikDialog
{
 public:
  CSettingsS80Dialog(TOggplaySettings *aSettings);

  TBool OkToExitL(TInt aButtonId);
  void PreLayoutDynInitL();
  TOggplaySettings *iSettings;
  
 private:
  //new functions
  void UpdateRskFromControls();
  void UpdateControlsFromRsk();
  
 private:
  //data
   CEikChoiceList* iScanDirControl ;
   CEikCheckBox*  iAutostartControl;
   CEikCheckBox* iRepeatControl;
   CEikCheckBox* iRandomControl;
   CEikChoiceList* iCbaControl [4] ;
   
};



class CCodecsS80Dialog : public CEikDialog, public MEikCommandObserver
{
 public:
  CCodecsS80Dialog();
  ~CCodecsS80Dialog();

  TBool OkToExitL(TInt aButtonId);
  void PreLayoutDynInitL();
  SEikControlInfo CreateCustomControlL(TInt aControlType);

 public: // from MEikCommandObserver
   void ProcessCommandL (TInt aCommandId);
   
  
 private:
  //data
     CCodecsLabel * iContainer;
     TInt iNbOfLines;

};

class CCodecsLabel : public CEikLabel
{
	// from CEikLabel
	TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
	TCoeInputCapabilities InputCapabilities() const; 
    void ConstructFromResourceL(TResourceReader& aReader);
    
    // New functions
    public:
	void ConstructL(const TDesC &anExtension);
    ~CCodecsLabel();
	private:
	void SetLabel();
	HBufC * iCodecExtension;
};

class CCodecsS80Container : public CCoeControl, public MEikCommandObserver, public MEikListBoxObserver
{
  public: 
        
   CCodecsS80Container(const TDesC & aCodecExtension, MEikCommandObserver* commandObserver);
        
   static CCodecsS80Container* NewL( const TDesC & aCodecExtension, MEikCommandObserver* commandObserver);
        
   ~CCodecsS80Container ();
        
  public: // from CoeControl
                
   void ConstructL( );
   void ConstructFromResourceL(TResourceReader& aReader);
   TInt CountComponentControls() const;
   CCoeControl* ComponentControl(TInt aIndex) const;
   void Draw(const TRect& aRect) const;
   TCoeInputCapabilities InputCapabilities() const; 
   TKeyResponse OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType );

  public: // from MEikCommandObserver
   void ProcessCommandL (TInt aCommandId);
         
  public: // from MEikListBoxObserver
   void HandleListBoxEventL(CEikListBox* aListBox,TListBoxEvent aEventType);
  private:
       //Functions
   void RefreshListboxModel();
  private:
       //Data
    CEikButtonGroupContainer * iCba;
        
    // For the codec selection list    
	CEikTextListBox *iListBox;
	CDesCArrayFlat *iMessageList;
	CCknInputFrame *iInputFrame;
	
	// For the codec description frame
	CEikColumnListBox *iDescriptionBox;
	CDesCArrayFlat    *iDescriptionList;
    CPluginSupportedList *iPluginList;
    MEikCommandObserver* iCommandObserver;
    
    TInt iCurrentIndex;
    const TDesC & iCodecExtension;
 };
    
    

class CCodecsSelectionS80Dialog : public CEikDialog
{

public:
    CCodecsSelectionS80Dialog(const TDesC &aCodecExtension);

    SEikControlInfo CreateCustomControlL(TInt aControlType);
    void PreLayoutDynInitL();
    // From MEikCommandObserver
    void ProcessCommandL (TInt /*aCommandId*/);
   //CCoeControl* CreateCustomCommandControlL(TInt aControlType);
    
private:
    CCodecsS80Container * iContainer;
    const TDesC & iCodecExtension;
};


#endif
