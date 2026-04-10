#pragma once
#include <wx/defs.h>

enum {
    ID_TOOL_SELECT = wxID_HIGHEST + 200,
    ID_TOOL_RECT,
    ID_TOOL_CIRCLE,
    ID_TOOL_TEXT,
    ID_TOOL_BEZIER,
    ID_PALETTE_IMPORT,
    ID_PALETTE_EXPORT,
    ID_ASSET_ADD,
    ID_ASSET_REMOVE,
    ID_ASSET_RENAME,
    ID_HIER_TOGGLE_VISIBLE,
    ID_HIER_TOGGLE_LOCKED,
    ID_HIER_RENAME,
    // Context-menu actions on the drawing canvas
    ID_CTX_DELETE,
    ID_CTX_GROUP,
    ID_CTX_UNGROUP,
    ID_CTX_PROPERTIES,
    // Settings dialog
    ID_SETTINGS_SNAP,
    ID_SETTINGS_GUIDE_COLOUR,
    // SMIL animation toolbar
    ID_SMIL_PLAY_BWD,
    ID_SMIL_REC,
    ID_SMIL_PLAY_FWD,
    // Scene panel
    ID_SCENE_ADD,
    ID_SCENE_REMOVE,
    ID_SCENE_RENAME,
    ID_SCENE_MOVE_UP,
    ID_SCENE_MOVE_DOWN,
    ID_SCENE_LIST,
    // Keyframe panel
    ID_KF_SET,
    ID_KF_REMOVE,
    // File menu
    ID_FILE_CLOSE,
    // Edit menu
    ID_EDIT_SELECTALL,
    // View menu
    ID_VIEW_TOOLBAR,
    // Help menu
    ID_HELP_ABOUT,
};
