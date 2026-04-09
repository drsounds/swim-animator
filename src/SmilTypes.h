#pragma once
#include <wx/colour.h>
#include <wx/string.h>
#include <vector>
#include <map>
#include "DrawShape.h"

// ---------------------------------------------------------------------------
// SmilInterp – easing curve applied from one keyframe to the next.
// ---------------------------------------------------------------------------
enum class SmilInterp { Linear, Bezier, Cubic };

// ---------------------------------------------------------------------------
// SmilKeyframe – one keyframe on a property track.
// ---------------------------------------------------------------------------
struct SmilKeyframe {
    int        frame  {0};
    wxString   value;                          // property value as a string
    SmilInterp interp {SmilInterp::Linear};    // easing curve into the NEXT keyframe
    // Cubic Bezier control points in normalised (0–1) coordinates.
    // Used when interp == Bezier; Cubic uses preset (0.42,0,0.58,1).
    float cx1{0.42f}, cy1{0.0f}, cx2{0.58f}, cy2{1.0f};
};

// ---------------------------------------------------------------------------
// SmilTrack – ordered keyframes for one SVG attribute on one element.
// ---------------------------------------------------------------------------
struct SmilTrack {
    SmilInterp               defaultInterp {SmilInterp::Linear};
    std::vector<SmilKeyframe> keyframes;    // kept sorted by frame

    // Interpolated string value at `frame`.
    wxString GetValueAt(int frame) const;

    // Insert or replace a keyframe at `frame`.
    void SetKeyframe(int frame, const wxString& value,
                     SmilInterp interp = SmilInterp::Linear);

    // Remove keyframe at `frame` (no-op if absent).
    void RemoveKeyframe(int frame);

    // True if there is a keyframe exactly at `frame`.
    bool HasKeyframeAt(int frame) const;
};

// ---------------------------------------------------------------------------
// SmilElement – all animated tracks for one SVG element.
// ---------------------------------------------------------------------------
struct SmilElement {
    wxString                      elementId;
    std::map<wxString, SmilTrack> tracks;   // SVG attribute name → track
};

// ---------------------------------------------------------------------------
// SmilScene – one scene in a SMIL document.
// ---------------------------------------------------------------------------
struct SmilScene {
    wxString               id;
    wxString               name           {"Scene"};
    int                    startFrame     {0};
    int                    durationFrames {240};    // 10 s at 24 fps
    bool                   embedded       {true};   // false = external file ref
    wxString               refPath;                // path when !embedded
    int                    pageWidth      {1920};
    int                    pageHeight     {1080};
    wxColour               bgColour       {255, 255, 255};
    std::vector<DrawShape>           shapes;        // SVG shape content
    std::map<wxString, SmilElement>  elements;      // elementId → animation
};
