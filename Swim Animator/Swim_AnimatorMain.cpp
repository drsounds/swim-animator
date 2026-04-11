/***************************************************************
 * Name:      Swim_AnimatorMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Alexander Forselius (drsounds@gmail.com)
 * Created:   2026-04-11
 * Copyright: Alexander Forselius ()
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#include "Swim_AnimatorMain.h"

//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}


Swim_AnimatorFrame::Swim_AnimatorFrame(wxFrame *frame)
    : GUIFrame(frame)
{
#if wxUSE_STATUSBAR
    statusBar->SetStatusText(_("Hello Code::Blocks user!"), 0);
    statusBar->SetStatusText(wxbuildinfo(short_f), 1);
#endif
}

Swim_AnimatorFrame::~Swim_AnimatorFrame()
{
}

void Swim_AnimatorFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void Swim_AnimatorFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}

void Swim_AnimatorFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}
