# Spacely / Swim Animator

A vector drawing and SMIL animation authoring tool built with C++23 and wxWidgets.

## Features

### SVG Drawing Editor (`.svg`)
- Draw rectangles, ellipses, text, and cubic Bézier curves
- Select, move, resize, and delete shapes
- Multi-select with Ctrl+click or rubber-band drag
- Group/ungroup shapes (Ctrl+G); double-click to enter group isolation mode
- Reuse external SVG files as embedded components (SVG ref / mount)
- Cut, copy, and paste with OS clipboard integration
- Full undo/redo via wxCommandProcessor
- Snap-to-grid, snap-to-object (centre/edge/proportional alignment)
- Pan (middle-mouse drag) and zoom (scroll wheel)
- Right-click context menu on canvas

### SMIL Animation Editor (`.smil`)
- Multiple scenes per file, each with its own SVG canvas and independent timeline
- Keyframe animation for all numeric and colour properties
- Interpolation per keyframe: **Linear**, **Bézier**, or **Cubic** (ease-in-out)
- **REC mode** — toggle the record button and every edit automatically creates a keyframe at the current frame
- Flash-style **Keyframe Panel** docked at the top: expandable per-object property rows, draggable playhead, diamond keyframe markers
- **Scene Panel** for adding, removing, renaming, and reordering scenes
- Scenes can be embedded inline or reference an external `.smil` file
- Animation stored as standard W3C SMIL `<animate>` tags inside SVG elements

### Swim Animator Project (`.spa`)
- ZIP archive or flat-folder bundle containing scene files and assets
- Asset Manager panel for images, audio, video, and other project files
- Lazy asset extraction: binary content is only read from the bundle on demand

### Property Editor
- **Properties tab** — typed controls for position, size, colour, stroke, border radius, and Bézier control points; keyframe indicator when a SMIL document is active
- **XML Attributes tab** — lists every raw SVG attribute of the selected shape as an editable key-value pair; changes are submitted as undoable commands

### Other
- Dockable, floatable panel layout via wxAUI (all panels can be rearranged or detached)
- GIMP palette import/export (`.gpl`)
- Object hierarchy panel with visibility and lock toggles, drag-to-reorder
- Colour swatch panel with foreground/background indicator

## Building

### Prerequisites

- CMake ≥ 3.16
- A C++17-capable compiler (g++ or clang++)
- wxWidgets ≥ 3.2 with components: `core base aui xml`

On Debian/Ubuntu:
```bash
sudo apt install libwxgtk3.2-dev cmake build-essential
```

### CMake build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/spacily-app
```

### CodeLite (IDE)

Open `spacely.workspace` in CodeLite. The project is configured for a `Debug` target using `spacely.mk`.

## File Formats

All formats are XML-based.

| Extension | Format | Description |
|---|---|---|
| `.svg` | Standard SVG 1.1 + `swim:` namespace | Single-page vector drawing |
| `.smil` | W3C SMIL + `swim:` namespace | Multi-scene animation (SMIL `<animate>` tags) |
| `.spa` | ZIP archive or folder | Swim Animator Project bundle |
| `.sly` | Custom XML | Generic Spacely document |

The `swim:` namespace (`https://spacify.se/swim/1.0`) carries application-specific metadata such as scene names, frame rates, and per-keyframe interpolation types that have no standard equivalents.

## Project Structure

```
src/
  App.cpp / App.h              wxApp entry point; document type registration
  MainFrame.cpp / .h           Top-level window; AUI layout; toolbar/panel wiring
  DrawDoc.cpp / .h             SVG document model (DrawShape tree, serialisation)
  DrawView.cpp / .h            SVG view + interactive DrawCanvas
  DrawShape.h                  Shape struct (Rect, Circle, Text, Bézier, Group, SVGRef)
  DrawCommands.cpp / .h        Undoable commands for shape mutations
  SmilDoc.cpp / .h             SMIL animation document (scenes, keyframes, fps)
  SmilView.cpp / .h            SMIL view + SmilCanvas (bridges DrawCanvas via proxy)
  SmilTypes.cpp / .h           Animation data model (Keyframe, Track, Element, Scene)
  SpaDoc.cpp / .h              Project bundle document (ZIP/folder I/O)
  SpaView.cpp / .h             Project overview view
  PropPanel.cpp / .h           Properties + XML Attributes dockable panel
  HierarchyPanel.cpp / .h      Object hierarchy tree panel
  KeyframePanel.cpp / .h       Flash-style timeline / keyframe editor panel
  ScenePanel.cpp / .h          Scene list panel for SMIL documents
  AssetManagerPanel.cpp / .h   Asset browser panel for .spa projects
  ColorSwatchPanel.cpp / .h    Colour palette + FG/BG indicator
  SnapEngine.cpp / .h          Snap-to-object and snap-to-grid logic
  SnapSettings.cpp / .h        Snap configuration (persisted to user data dir)
```

## Roadmap

- [ ] Playback engine for SMIL animations (play forward / backward)
- [ ] `.spmx` Swim Animator Movie — multi-scene movie container with storyboard panel and audio/voice timeline
- [ ] Export `.spmx` to `.ogv` via ffmpeg
- [ ] Reuse `.smil` files as referenced scenes inside a movie project
- [ ] SMIL interactivity (JS-based, with code editor panel)
- [ ] Text file editing with syntax highlighting
- [ ] HH:MM:SS:FF timecode display for scene durations
