---
name: project_spacely
description: Spacely wxWidgets drawing app — architecture, completed features, current state
type: project
---

Spacely is a wxWidgets C++ drawing application using the wxDoc/View pattern with wxAuiManager for dockable panels.

**Build**: CMake (`cmake -S . -B build && cmake --build build`). Also has a CodeLite `.mk` file.

**Document types registered in App.cpp:**
- `.sly` — SpacelyDoc/SpacelyView (generic)
- `.svg` — DrawDoc/DrawView (full SVG editor with shapes, groups, SVG refs)
- `.spa` — SpaDoc/SpaView (Swim Animator Project; ZIP or folder bundle with assets)
- `.smil` — SmilDoc/SmilView (SMIL animation; scenes + keyframe animation)

**Architecture:**
- All documents follow: wxDocument subclass + wxView subclass + wxPanel canvas child of the AUI notebook tab
- `MainFrame` uses `wxAuiManager` for all layout (toolbars via `.ToolbarPane()`, panels via `.Right()/.Top()/.Bottom()`)
- `DrawDoc` uses `DrawShape` structs and serializes to SVG XML with `xmlns:swim` namespace

**Key panels (all dockable via AUI):**
- `PropPanel` (Bottom) — shape/document properties + XML Attributes tab; SetSmilContext() for keyframe diamonds
- `HierarchyPanel` (Right) — object tree for DrawDoc shapes
- `ColorSwatchPanel` (Right) — palette + FG/BG
- `AssetManagerPanel` (Right) — SPA project assets
- `ScenePanel` (Right) — scene list for SmilDoc (Add/Remove/Rename/Move)
- `KeyframePanel` (Top Row 2) — Flash-style timeline; hidden unless SmilDoc is active

**Toolbars (all AUI panes):**
- `m_toolbar` (Top Row 0) — standard file/edit tools
- `m_drawToolbar` (Left) — shape drawing tools (vertical)
- `m_animToolbar` (Top Row 1) — animation toolbar: Play Bwd (stub), REC (toggle), Play Fwd (stub)

**SMIL architecture (implemented 2026-04-09):**
- `SmilTypes.h/cpp` — SmilKeyframe, SmilTrack (GetValueAt linear/Bezier/Cubic), SmilElement, SmilScene
- `SmilDoc` — scenes with embedded SVG shapes + SMIL `<animate>` tags; fps; current frame/scene; REC mode
- `SmilView` — creates `SmilCanvas` tab; on activate wires MainFrame SMIL panels
- `SmilCanvas` — hosts `DrawCanvas` child via `SyntheticDrawDoc` + `SmilProxyView` bridge:
  - `LoadFromScene()`: SmilScene.shapes → SyntheticDrawDoc (with animated values applied)
  - `WriteBackToScene()`: SyntheticDrawDoc.shapes → SmilScene + keyframes if REC
- `SmilProxyView : DrawView` — NotifySelectionChanged forwards to MainFrame + OnSmilSelectionChanged

**SMIL XML format:**
```xml
<smil xmlns="http://www.w3.org/ns/SMIL" xmlns:swim="https://spacify.se/swim/1.0" swim:fps="24">
  <body>
    <swim:scene id="s1" swim:name="Intro" swim:start="0" swim:duration="240"
                swim:pageWidth="1920" swim:pageHeight="1080" swim:bgColor="#FFFFFF">
      <svg xmlns="http://www.w3.org/2000/svg" xmlns:swim="https://swim.spacify.se/TR/">
        <rect id="elem_0" ...>
          <animate attributeName="x" values="100;300" keyTimes="0;1"
                   begin="0s" dur="10s" calcMode="spline" keySplines="0 0 1 1"
                   swim:keyInterps="linear;linear" fill="freeze"/>
        </rect>
      </svg>
    </swim:scene>
    <swim:scene id="s2" swim:refPath="other.smil"/>
  </body>
</smil>
```

**Completed features:**
- Cut/copy/paste shapes
- Snapping engine
- Multi-select, grouping, hierarchy panel, SVG ref components
- Right-click context menus on canvas
- Auto-revert to Select tool after shape creation
- SMIL animation document type (scenes, keyframes, REC mode, timeline panel)

**Known limitations / future work:**
- Play backward/forward in animation toolbar: placeholder only (opens info dialog)
- Duration displayed as raw frames in scene panel, not HH:MM:SS:FF timecode field
- .spmx project type (task 2 in kanban) — not yet implemented
- `SetSmilContext()` in PropPanel is implemented but not yet called from SmilView/MainFrame
