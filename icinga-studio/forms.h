///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun 17 2015)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __FORMS_H__
#define __FORMS_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include <wx/sizer.h>
#include <wx/statusbr.h>
#include <wx/frame.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/statbmp.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class MainFormBase
///////////////////////////////////////////////////////////////////////////////
class MainFormBase : public wxFrame 
{
	private:

	protected:
		wxMenuBar* m_MenuBar;
		wxTreeCtrl* m_TypesTree;
		wxListCtrl* m_ObjectsList;
		wxPropertyGrid* m_PropertyGrid;
		wxStatusBar* m_StatusBar;

		// Virtual event handlers, overide them in your derived class
		virtual void OnQuitClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAboutClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnTypeSelected( wxTreeEvent& event ) { event.Skip(); }
		virtual void OnObjectSelected( wxListEvent& event ) { event.Skip(); }


	public:

		MainFormBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Icinga Studio"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 800,569 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );

		~MainFormBase() override;

};

///////////////////////////////////////////////////////////////////////////////
/// Class ConnectFormBase
///////////////////////////////////////////////////////////////////////////////
class ConnectFormBase : public wxDialog 
{
	private:

	protected:
		wxTextCtrl* m_HostText;
		wxTextCtrl* m_PortText;
		wxTextCtrl* m_UserText;
		wxTextCtrl* m_PasswordText;
		wxStdDialogButtonSizer* m_Buttons;
		wxButton* m_ButtonsOK;
		wxButton* m_ButtonsCancel;

	public:

		ConnectFormBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Icinga Studio - Connect"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~ConnectFormBase() override;

};

///////////////////////////////////////////////////////////////////////////////
/// Class AboutFormBase
///////////////////////////////////////////////////////////////////////////////
class AboutFormBase : public wxDialog 
{
	private:

	protected:
		wxStaticText* m_VersionLabel;

	public:

		AboutFormBase( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("About Icinga Studio"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE ); 
		~AboutFormBase() override;

};

#endif //__FORMS_H__
