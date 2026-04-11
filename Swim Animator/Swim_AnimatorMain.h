/***************************************************************
 * Name:      Swim_AnimatorMain.h
 * Purpose:   Defines Application Frame
 * Author:    Alexander Forselius (drsounds@gmail.com)
 * Created:   2026-04-11
 * Copyright: Alexander Forselius ()
 * License:
 **************************************************************/

#ifndef SWIM_ANIMATORMAIN_H
#define SWIM_ANIMATORMAIN_H



#include "Swim_AnimatorApp.h"


#include "GUIFrame.h"

class Swim_AnimatorFrame: public GUIFrame
{
    public:
        Swim_AnimatorFrame(wxFrame *frame);
        ~Swim_AnimatorFrame();
    private:
        virtual void OnClose(wxCloseEvent& event);
        virtual void OnQuit(wxCommandEvent& event);
        virtual void OnAbout(wxCommandEvent& event);
};

#endif // SWIM_ANIMATORMAIN_H
