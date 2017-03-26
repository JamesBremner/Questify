#include <string>
#include <vector>
#include <map>
#include <wx/wx.h>
#include <wx/app.h>
#include <wx/richtext/richtextctrl.h>

#include "raven_sqlite.h"

using namespace std;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit();
};

// Define a new frame type: this is going to be our main frame
class MyFrame : public wxFrame
{
public:

    // ctor(s)
    MyFrame(const wxString& title);

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnGenerate(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    void onCheckBox(wxCommandEvent& event);
    void onTextCrtrl(wxCommandEvent& event);

    wxSizer * ConstructQuestion( wxWindow* panel, const string& sid );

private:
    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()

};

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // menu items
    Minimal_Quit = wxID_EXIT,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    Minimal_About = wxID_ABOUT,

};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(Minimal_Quit,  MyFrame::OnQuit)
    EVT_MENU(Minimal_About, MyFrame::OnAbout)
    EVT_SIZE(MyFrame::OnSize)
END_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
IMPLEMENT_APP(MyApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

    // create the main application window
    MyFrame *frame = new MyFrame(_T("Questify"));

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    frame->Show(true);

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// Connection to db containing questions and answers
raven::sqlite::cDB DB( "questify.dat" );

// Maps of widgets IDs to database question IDs
map< int, int > checkboxMap;
map< int, int > text1Map;
map< int, int > text2Map;

// frame constructor
MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title)
{
    // set the frame icon
    SetIcon(wxICON(sample));

#if wxUSE_MENUS
    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, _T("&About...\tF1"), _T("Show about dialog"));

    fileMenu->Append(Minimal_Quit, _T("E&xit\tAlt-X"), _T("Quit this program"));

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, _T("&File"));
    menuBar->Append(helpMenu, _T("&Help"));

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText(_T("Welcome to Questify!"));
#endif // wxUSE_STATUSBAR

    wxPanel * panel = new wxPanel(this,-1,wxPoint(-1,-1),wxSize(500,500));

    wxStaticBoxSizer * szr = new wxStaticBoxSizer( wxHORIZONTAL, this,
            "Please check all that apply to your EXERCISE & SPORTS Activity because of the accident");



    // loop over questions in first column

    wxBoxSizer * szr_col1 = new wxBoxSizer( wxVERTICAL );
    DB.Query("SELECT rowid FROM question WHERE column = 1;");
    vector< string > vID( DB.myResultA );
    for( auto& rowid : vID )
    {
        szr_col1->Add(
            ConstructQuestion( panel, rowid ) );
    }

    // loop over questions in second column

     wxBoxSizer * szr_col2 = new wxBoxSizer( wxVERTICAL );
    DB.Query("SELECT rowid FROM question WHERE column = 2;");
    vID = DB.myResultA;
    for( auto& rowid : vID )
    {
        szr_col2->Add( ConstructQuestion( panel, rowid ) );
    }

    Bind( wxEVT_CHECKBOX, &MyFrame::onCheckBox, this );
    Bind( wxEVT_TEXT, &MyFrame::onTextCrtrl, this );

    szr->Add( szr_col1,0, wxALL, 10 );
    szr->Add( szr_col2,0, wxALL, 10 );
    panel->SetSizerAndFit(szr);

}

/** Create a quetion display

    @param[in] panel to display question in
    @param[in] sid strinf containing db question ID
*/
wxSizer * MyFrame::ConstructQuestion( wxWindow* panel, const string& sid )
{
    // read question from database
    int db_rowid = atoi( sid.c_str() );
    DB.Query( "SELECT text, text_input, text2, text_input2, text3 "
              " FROM question WHERE rowid = %d;", db_rowid );

    // construct horizontal box sizer to layout question
    wxBoxSizer * szr = new wxBoxSizer( wxHORIZONTAL );

    // assign unique widget ID
    wxWindowID idc = wxWindow::NewControlId();

    // map widget ID to db question ID
    checkboxMap.insert( make_pair( idc, db_rowid ) );

    // construct question checkbox
    szr->Add( new wxCheckBox(panel,idc,DB.myResultA[0] ));

    // check if there is text input expected
    if( DB.myResultA[1] == "1" )
    {
        idc = wxWindow::NewControlId();
        text1Map.insert( make_pair( idc, db_rowid ) );
        szr->Add( new wxTextCtrl( panel,idc,""));
    }

    // check if second text to be displayed
    if( DB.myResultA[2].length() )
        szr->Add( new wxStaticText(panel,-1,DB.myResultA[2]));

    // check if there is a second text input
    if( DB.myResultA[3] == "1" )
    {
        idc = wxWindow::NewControlId();
        text2Map.insert( make_pair( idc, db_rowid ) );
        szr->Add( new wxTextCtrl( panel,idc,""));
    }

    // check if third text to be displayed
    if( DB.myResultA[4].length() )
        szr->Add( new wxStaticText(panel,-1,DB.myResultA[4]));

    return szr;
}



// event handlers

// user has clicked question checkbox
void MyFrame::onCheckBox(wxCommandEvent& event)
{
    // find question id for the question
    auto it = checkboxMap.find( event.GetId() );
    if( it == checkboxMap.end() )
        return;

    // ensure the question has a row in the answer table
    DB.Query("INSERT INTO answer ( question ) VALUES ( %d );", it->second );

    // update answer with new status of checkbox
    DB.Query("UPDATE answer SET checkbox = %d WHERE question = %d;",
             (int)((wxCheckBox*)FindWindowById( it->first ))->IsChecked(),
             it->second);
}

// user has changed text in text input fields
void MyFrame::onTextCrtrl(wxCommandEvent& event)
{
    // find if one the the first text input fields
    auto it = text1Map.find( event.GetId() );
    if( it != text1Map.end() )
    {
         // ensure the question has a row in the answer table
        DB.Query("INSERT INTO answer ( question ) VALUES ( %d );", it->second );

        // update answer with new text
        DB.Query("UPDATE answer SET text1 = '%s' WHERE question = %d;",
                 ((wxTextCtrl*)FindWindowById( it->first ))->GetValue().mb_str().data(),
                 it->second );
    }
    else
    {
        // find if one the the second text input fields
        auto it = text2Map.find( event.GetId() );
        if( it != text2Map.end() )
        {
              // ensure the question has a row in the answer table
            DB.Query("INSERT INTO answer ( question ) VALUES ( %d );", it->second );

            // update answer with new text
            DB.Query("UPDATE answer SET text2 = '%s' WHERE question = %d;",
                     ((wxTextCtrl*)FindWindowById( it->first ))->GetValue().mb_str().data(),
                     it->second );
        }
    }
}

void MyFrame::OnSize(wxSizeEvent& )
{

}

void MyFrame::OnGenerate(wxCommandEvent& )
{

}


void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox("Starter",
                 _T("Starter"),
                 wxOK | wxICON_INFORMATION,
                 this);
}
