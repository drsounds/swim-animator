#include "SmilTypes.h"
#include <algorithm>
#include <cmath>

// ---------------------------------------------------------------------------
// SmilTrack
// ---------------------------------------------------------------------------

wxString SmilTrack::GetValueAt(int frame) const {
    if (keyframes.empty()) return wxString();
    if (frame <= keyframes.front().frame) return keyframes.front().value;
    if (frame >= keyframes.back().frame)  return keyframes.back().value;

    for (size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const SmilKeyframe& k0 = keyframes[i];
        const SmilKeyframe& k1 = keyframes[i + 1];
        if (frame < k0.frame || frame > k1.frame) continue;
        if (k0.frame == k1.frame) return k0.value;

        float t = (float)(frame - k0.frame) / (float)(k1.frame - k0.frame);

        // Apply Bezier/Cubic easing.
        if (k0.interp == SmilInterp::Bezier || k0.interp == SmilInterp::Cubic) {
            float cx1 = k0.cx1, cy1 = k0.cy1, cx2 = k0.cx2, cy2 = k0.cy2;
            if (k0.interp == SmilInterp::Cubic) { cx1=0.42f; cy1=0.0f; cx2=0.58f; cy2=1.0f; }
            // Solve for Bezier parameter x at horizontal position t (Newton, 3 iters).
            float x = t;
            for (int j = 0; j < 3; ++j) {
                float mt = 1.0f - x;
                float f  = 3*mt*mt*x*cx1 + 3*mt*x*x*cx2 + x*x*x - t;
                float df = 3*mt*mt*cx1 + 6*mt*x*(cx2 - cx1) + 3*x*x*(1.0f - cx2);
                if (std::fabs(df) > 1e-6f) x -= f / df;
            }
            float mt = 1.0f - x;
            t = 3*mt*mt*x*cy1 + 3*mt*x*x*cy2 + x*x*x;
        }

        double v0 = 0, v1 = 0;
        if (k0.value.ToDouble(&v0) && k1.value.ToDouble(&v1)) {
            double v = v0 + (double)t * (v1 - v0);
            if (v == std::floor(v)) return wxString::Format("%d", (int)v);
            return wxString::Format("%.4g", v);
        }
        return k0.value;  // non-numeric: snap
    }
    return keyframes.back().value;
}

void SmilTrack::SetKeyframe(int frame, const wxString& value, SmilInterp interp) {
    for (SmilKeyframe& k : keyframes) {
        if (k.frame == frame) { k.value = value; k.interp = interp; return; }
    }
    SmilKeyframe kf;
    kf.frame = frame; kf.value = value; kf.interp = interp;
    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), kf,
                               [](const SmilKeyframe& a, const SmilKeyframe& b) {
                                   return a.frame < b.frame;
                               });
    keyframes.insert(it, kf);
}

void SmilTrack::RemoveKeyframe(int frame) {
    keyframes.erase(
        std::remove_if(keyframes.begin(), keyframes.end(),
                       [frame](const SmilKeyframe& k){ return k.frame == frame; }),
        keyframes.end());
}

bool SmilTrack::HasKeyframeAt(int frame) const {
    for (const SmilKeyframe& k : keyframes)
        if (k.frame == frame) return true;
    return false;
}
