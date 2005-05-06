/*
 *  Copyright (c) 2005 OggPlay Team.
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
#include "OggDialogsS80.h"
#include "OggPlay.h"
#include <eiklabel.h>
#include <eikchlst.h>
#include <eikchkbx.h>
#include "OggPlay.hrh"
#include <OggPlay.rsg>
#include <eikon.rsg>
#include <gulbordr.h>
#include "OggUserHotkeys.h"


CSettingsS80Dialog::CSettingsS80Dialog(TOggplaySettings *aSettings)
{
  iSettings = aSettings;
}


TBool
CSettingsS80Dialog::OkToExitL(int /* aButtonId */)
{
  iSettings->iScanmode = iScanDirControl->CurrentItem();  
  iSettings->iAutoplay = static_cast <TInt> (iAutostartControl->State());
  
  COggPlayAppUi * appUi = static_cast <COggPlayAppUi*> (CEikonEnv::Static()->AppUi());
  appUi->SetRepeat( static_cast <TInt> (iRepeatControl->State()) );
  appUi->SetRandomL( static_cast <TInt> (iRandomControl->State()) );
  
  UpdateSoftkeysFromControls();
  return ETrue;
}


void
CSettingsS80Dialog::PreLayoutDynInitL()
{
  iScanDirControl = static_cast <CEikChoiceList*> (Control(EOggSettingScanDir));
  iAutostartControl = static_cast <CEikCheckBox*> (Control(EOggSettingAutoPlayId));
  iRepeatControl = static_cast <CEikCheckBox*> (Control(EOggSettingRepeatId));
  iRandomControl = static_cast <CEikCheckBox*> (Control(EOggSettingRandomId));
  iCbaControl[0][0] = static_cast <CEikChoiceList*> (Control(EOggSettingCba01));
  iCbaControl[0][1] = static_cast <CEikChoiceList*> (Control(EOggSettingCba02));
  iCbaControl[0][2] = static_cast <CEikChoiceList*> (Control(EOggSettingCba03));
  iCbaControl[0][3] = static_cast <CEikChoiceList*> (Control(EOggSettingCba04));
  iCbaControl[1][0] = static_cast <CEikChoiceList*> (Control(EOggSettingCba11));
  iCbaControl[1][1] = static_cast <CEikChoiceList*> (Control(EOggSettingCba12));
  iCbaControl[1][2] = static_cast <CEikChoiceList*> (Control(EOggSettingCba13));
  iCbaControl[1][3] = static_cast <CEikChoiceList*> (Control(EOggSettingCba14));
 
  iScanDirControl->SetCurrentItem(iSettings->iScanmode);
  iAutostartControl->SetState(static_cast <CEikButtonBase::TState>(iSettings->iAutoplay));
  
  COggPlayAppUi * appUi = static_cast <COggPlayAppUi*> (CEikonEnv::Static()->AppUi());
  iRepeatControl->SetState(static_cast <CEikButtonBase::TState>(appUi->iSettings.iRepeat));
  iRandomControl->SetState(static_cast <CEikButtonBase::TState>(appUi->iRandom));
  UpdateControlsFromSoftkeys();
}




void 
CSettingsS80Dialog::UpdateSoftkeysFromControls()
{
    // Update softkeys
	for (TInt i=0; i<4; i++)
	{
	    iSettings->iSoftKeysIdle[i] = 
	        COggUserHotkeysS80::MapRssListToCommand(iCbaControl[0][i]->CurrentItem());
        iSettings->iSoftKeysPlay[i] = 
	        COggUserHotkeysS80::MapRssListToCommand(iCbaControl[1][i]->CurrentItem());

	}
}

void 
CSettingsS80Dialog::UpdateControlsFromSoftkeys()
{    
	for (TInt i=0; i<4; i++)
	{	
	
	   iCbaControl[0][i]->SetCurrentItem( 
	      COggUserHotkeysS80::MapCommandToRssList(iSettings->iSoftKeysIdle[i]) );
	   iCbaControl[1][i]->SetCurrentItem( 
	      COggUserHotkeysS80::MapCommandToRssList(iSettings->iSoftKeysPlay[i]) );
	}
}

///////////////////////////////////////////////

CCodecsS80Dialog::CCodecsS80Dialog()
{

}

TBool
CCodecsS80Dialog::OkToExitL(TInt /*aButtonId*/)
{
  for (TInt i=0; i<iNbOfLines; i++)
  {
  	  DeleteLine(555+i);
  }
  return(ETrue);
}

void
CCodecsS80Dialog::PreLayoutDynInitL()
  {
    // Build the menu on the fly, according to the list plugins.
  	COggPlayAppUi * appUi = static_cast <COggPlayAppUi*> (CEikonEnv::Static()->AppUi());
    CDesCArrayFlat * extensions = appUi->iOggPlayback->GetPluginListL().SupportedExtensions();
 	CleanupStack::PushL(extensions);
 	
    TInt foo;
    iNbOfLines = extensions->Count();
    for (TInt i=0; i<iNbOfLines; i++)
    {
       CCodecsLabel * ctrl = static_cast<CCodecsLabel *> 
         (CreateLineByTypeL( (*extensions)[i],0, 555+i, EOggCodecPresentationControl,&foo ));
       ctrl->ConstructL( (*extensions)[i] );
    }
    CleanupStack::PopAndDestroy(extensions);
    
    // Change the CBA     
    TBuf<50> tmp;
	iEikonEnv->ReadResource(tmp, R_OGG_CBA_SELECT);
	CEikButtonGroupContainer& cba = ButtonGroupContainer();
    cba.SetCommandL(0,EUserSelectCBA,tmp); 
    cba.UpdateCommandObserverL(0,*this); // We will receive the commands for that button
    cba.DrawNow();
  }
  
  
CCodecsS80Dialog::~CCodecsS80Dialog()
{
}

SEikControlInfo CCodecsS80Dialog::CreateCustomControlL(TInt aControlType)
{
    if (aControlType == EOggCodecPresentationControl) 
    { 
        iContainer = new (ELeave) CCodecsLabel();
    } 
    SEikControlInfo info = {iContainer,0,0};
    return info;
}


void CCodecsS80Dialog::ProcessCommandL (TInt aCommandId)
{
	if (aCommandId == EUserSelectCBA)
	{
		// Tell the control that it has been selected.
		// by simulating a rigth arrow click.
		TKeyEvent event;
		event.iScanCode = EStdKeyRightArrow;
	    OfferKeyEventL(event, EEventKey);
	}
}

///
void CCodecsLabel::ConstructFromResourceL(TResourceReader& aReader)
{
    iCodecExtension = aReader.ReadTPtrC().AllocL();
    //Set current plugin
    SetLabel();
}

void CCodecsLabel::ConstructL(const TDesC &anExtension)
{
	
	iCodecExtension = anExtension.AllocL();   
	 //Set current plugin
    SetLabel();
}

TKeyResponse CCodecsLabel::OfferKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
{
	if (aType==EEventKey && aKeyEvent.iScanCode==EStdKeyRightArrow)
	{
     	CCodecsSelectionS80Dialog *cd = new CCodecsSelectionS80Dialog(*iCodecExtension);
		cd->ExecuteLD(R_DIALOG_CODEC_SELECTION);
		
		//Update the label
		SetLabel();
		return(EKeyWasConsumed)	;
	}
    return (EKeyWasNotConsumed);
}

void CCodecsLabel::SetLabel()
{
	COggPlayAppUi * appUi = static_cast <COggPlayAppUi*> (CEikonEnv::Static()->AppUi());
	CPluginInfo * info = appUi->iOggPlayback->GetPluginListL().GetSelectedPluginInfo(*iCodecExtension);
	if ( info == NULL )
	{
	   // Codec has been disabled
	    TBuf <50> tmp;
   		iEikonEnv->ReadResource(tmp,R_OGG_DISABLED);
		SetTextL(tmp);
	}
	else SetTextL(*info->iName);
}

TCoeInputCapabilities CCodecsLabel::InputCapabilities() const
{ 
   return TCoeInputCapabilities::ENone;
}


CCodecsLabel::~CCodecsLabel()
{
	delete iCodecExtension;
}

///////////////////////////////////////////////////
CCodecsS80Container::CCodecsS80Container(const TDesC &aCodecExtension, MEikCommandObserver* commandObserver) :
iCodecExtension (aCodecExtension)
{
 iCommandObserver = commandObserver;
}

void CCodecsS80Container::ConstructFromResourceL(TResourceReader& /*aReader*/)
{
    ConstructL();
}
void CCodecsS80Container::ConstructL()
{
    TSize aSize(402,155); // The maximal size of an S80 dialog
    SetSize(aSize);
    
    COggPlayAppUi * appUi = static_cast <COggPlayAppUi*> (CEikonEnv::Static()->AppUi());
 
	iPluginList = &appUi->iOggPlayback->GetPluginListL();
     
    iListBox = new (ELeave) CEikTextListBox;

    iListBox->ConstructL(this);
    iListBox->SetContainerWindowL(*this);
    iListBox->SetListBoxObserver(this);
    
    // Create an array to hold the messages
    iMessageList = new (ELeave) CDesCArrayFlat(10);
	CArrayPtrFlat <CPluginInfo> * list = iPluginList->GetPluginInfoList(iCodecExtension);
    for(TInt i=0; i<list->Count(); i++)
    {
        iMessageList->AppendL(*list->At(i)->iName);
    }

    TBuf <50> tmp;
   	iEikonEnv->ReadResource(tmp,R_OGG_USE_NO_CODEC);
	iMessageList->AppendL(tmp);
   
    // Give it to the control
    CTextListBoxModel* model = iListBox->Model();
    model->SetItemTextArray(iMessageList);
    model->SetOwnershipType(ELbmOwnsItemArray); // transfer ownership of iMessageList
    iListBox->SetFocus(ETrue);
    iInputFrame = CCknInputFrame::NewL(iListBox, ETrue, NULL);
    iInputFrame->SetRect(TRect(TPoint(0,0), TSize(150,155)));
    iInputFrame->ActivateL();
    iInputFrame->SetContainerWindowL(*this);

    iCurrentIndex = iListBox->CurrentItemIndex();
    
    ///// Create the description pane
    
    iDescriptionBox = new (ELeave) CEikColumnListBox();
    iDescriptionBox->ConstructL(this);
    iDescriptionBox->SetContainerWindowL(*this);
    iDescriptionBox->SetBorder(TGulBorder::ESingleDotted);
    TRect aRect(TPoint(155,40), TSize(245,100));
    iDescriptionBox->SetRect(aRect);
    
    model = iDescriptionBox->Model();
    model->SetItemTextArray(new (ELeave) CDesCArrayFlat(10));
    model->SetOwnershipType(ELbmOwnsItemArray);
    RefreshListboxModel();
    // Set the listbox's first column to fill the listbox's whole width
    CColumnListBoxData* data = iDescriptionBox->ItemDrawer()->ColumnData();
    data->SetColumnWidthPixelL(0, 100);
    data->SetColumnWidthPixelL(1, 150);
     
    SetFocus(ETrue);
    ActivateL();	
}



void CCodecsS80Container::RefreshListboxModel()
{
    TBuf<64+64+1> listboxBuf;
    
    CDesCArrayFlat* array = iCoeEnv->ReadDesCArrayResourceL(R_CODEC_INFO_ITEMS);
    CDesCArray* modelArray = static_cast<CDesCArray*>(iDescriptionBox->Model()->ItemTextArray());

    modelArray->Reset();
    
    CPluginInfo * pluginInfo = NULL;
    CArrayPtrFlat <CPluginInfo> *infoList = iPluginList->GetPluginInfoList(iCodecExtension);
    if ( infoList )
       if (iCurrentIndex <= infoList->Count()-1 )
           pluginInfo = infoList->At(iCurrentIndex);
	   
    for( TInt i=0; i<4; i++ ) 
    {
        listboxBuf.Zero();
        
        TBuf<64> title;
        TBuf<64> value; 
        title.Copy(array->MdcaPoint(i));
        if ( pluginInfo == NULL )
        {
    	  value = KNullDesC;
        }
        else 
        {    
        
    	    switch (i)
	        {
	        case 0:
	            // Name
	            value = pluginInfo->iName->Left(64);
	            break;
	        case 1:
	            // Supplier
	            value = pluginInfo->iSupplier->Left(64);
	            break;
	        case 2:
	            // Version
	            value.Format(_L("%i"), pluginInfo->iVersion);
	            break;
	        case 3:
	            // Version
	            value = pluginInfo->iControllerUid.Name().Left(64);
	            break;
	            
	        }
        }
        listboxBuf.Format(_L("%S\t%S"), &title, &value);
        modelArray->AppendL(listboxBuf);
    }

    iDescriptionBox->HandleItemAdditionL();
    DrawDeferred();
    delete array;
}

void CCodecsS80Container::HandleListBoxEventL(CEikListBox* /*aListBox*/,TListBoxEvent aEventType)
{
	if (aEventType == EEventSelectionChanged)
	{	
	   iCurrentIndex = iListBox->CurrentItemIndex();
	   RefreshListboxModel();
	}
	
}

CCodecsS80Container* CCodecsS80Container::NewL( const TDesC & aCodecExtension,MEikCommandObserver* commandObserver)
    {
    CCodecsS80Container* self = new (ELeave) CCodecsS80Container(aCodecExtension,commandObserver);
    return self;
    }


TInt CCodecsS80Container::CountComponentControls() const
    {
    return 2;
    }


CCoeControl* CCodecsS80Container::ComponentControl(TInt aIndex) const
{
    switch(aIndex)
    {
    	case 0:
    	  return iInputFrame;
    	case 1:
    	  return iDescriptionBox;
    }
    return NULL;
}


void CCodecsS80Container::Draw(const TRect& aRect) const
    {    
    CWindowGc& gc = SystemGc();
    gc.SetBrushColor(KRgbWhite);
    gc.SetBrushStyle(CGraphicsContext::ESolidBrush);
    gc.DrawRect(aRect);
    }


CCodecsS80Container::~CCodecsS80Container ()
    {
    delete(iInputFrame);
    delete(iDescriptionBox);
    }


TCoeInputCapabilities CCodecsS80Container::InputCapabilities() const
    {
    return TCoeInputCapabilities::ENone;
    }

void CCodecsS80Container::ProcessCommandL (TInt aCommandId)
{
	if (aCommandId==EUserSelectCBA)
	{
	// User has selected a new codec.
	  CPluginInfo * pluginInfo = NULL;
      CArrayPtrFlat <CPluginInfo> *infoList = iPluginList->GetPluginInfoList(iCodecExtension);
      if ( infoList )
         if (iCurrentIndex <= infoList->Count()-1 )
            pluginInfo = infoList->At(iCurrentIndex);
         
	 if (pluginInfo)
		iPluginList->SelectPluginL(iCodecExtension, pluginInfo->iControllerUid);
	 else
	 	iPluginList->SelectPluginL(iCodecExtension, TUid::Null());
	 
	 // Call the original process command (to dismiss the dialog)
	 iCommandObserver->ProcessCommandL(EEikCmdCanceled);
	 
	}
}

TKeyResponse CCodecsS80Container::OfferKeyEventL(
    const TKeyEvent& aKeyEvent,
    TEventCode aType )
  {
  return iListBox->OfferKeyEventL( aKeyEvent, aType );
  }
  

////////////////////////////////////////////////////////////////
//
// class CCodecsS80Dialog
//
////////////////////////////////////////////////////////////////

CCodecsSelectionS80Dialog::CCodecsSelectionS80Dialog(const TDesC &aCodecExtension) :
iCodecExtension(aCodecExtension)
{
}

void CCodecsSelectionS80Dialog::PreLayoutDynInitL()
{
 // Set the CBA
    	 
    TBuf<50> tmp;
	iEikonEnv->ReadResource(tmp, R_OGG_CBA_SELECT);
	CEikButtonGroupContainer& cba = ButtonGroupContainer();
    cba.SetCommandL(0,EUserSelectCBA,tmp); 
    cba.UpdateCommandObserverL(0,*iContainer); // We will receive the commands for that button
    cba.DrawNow();
    
    
}

SEikControlInfo CCodecsSelectionS80Dialog::CreateCustomControlL(TInt aControlType)
{
    if (aControlType == EOggCodecSelectionControl) 
    { 
        iContainer = CCodecsS80Container::NewL(iCodecExtension, ButtonCommandObserver()) ;
    } 
    SEikControlInfo info = {iContainer,0,0};
    return info;
}

