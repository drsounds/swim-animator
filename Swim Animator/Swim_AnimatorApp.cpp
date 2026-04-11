/***************************************************************
 * Name:      Swim_AnimatorApp.cpp
 * Purpose:   Code for Application Class
 * Author:    Alexander Forselius (drsounds@gmail.com)
 * Created:   2026-04-11
 * Copyright: Alexander Forselius ()
 * License:
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#include "Swim_AnimatorApp.h"
#include "Swim_AnimatorMain.h"

IMPLEMENT_APP(Swim_AnimatorApp);

bool Swim_AnimatorApp::OnInit()
{
    Swim_AnimatorFrame* frame = new Swim_AnimatorFrame(0L);
    
    frame->Show();
    
    return true;
}
