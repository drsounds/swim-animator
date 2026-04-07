Add support for fill color in beizer curve.

/feature-dev:feature-dev Add support for color swatches, import and export in the GIMP Palette format (.gpl format), and assigner of back and foreground colors like in many 2D drawing software.

/feature-dev:feature-dev replace fill and border colors with generic brush type that can be either gradient or solid.

/feature-dev:feature-dev Add a text object type, with colors, size, style (bold italic) and fonts, ability to resize them in a box (layout) or artistic (size whole text).

/feature-dev:feature-dev Convert all current file formats to XML files based on standards of appropriate types with current and future specific properties and types under the namespace xmlns:swim="https://swim.spacify.se/TR/" for easy editing and debugging, and all future formats should be XML files instead of binary files. Deprecate the .drw format and instead read and write those files as standardized .svg XML files. Create a new ZIP based file type .spa "Swim Animator Project" which will be based on a zipped folder but optionally be read and written as folder, which will allow for embedding current and previous file types as well as external resources will embeded into those zip bundles (images, assets etc). Create a feature which allows managing external assets in the project, the list of assets is stored in "assets.json" in the project (zipped) file. The types of asset that should be supported at this time are images (.png, .jpeg, .gif), .txt (text files), audio (.ogg, .wav), video (.ogv), vector graphics (.svg), scenes (.smil), and create a feature to manage assets in a speciffic .spa project.

/feature-dev:feature-dev Create a new SMIL based derivative of the .svg type, based on SVG xml which adds support for keyframe animation of all current and future properties of all now and future object types, with a dockable keyframe panel, default docked at the top like in the older versions of Macromedia (a.k.a Adobe) Flash studio, which lists all objects with expandable properties as keyframe timelines. This subtype adds new paremeters to this file type such duration (stored as seconds but displayed and edited as a wxWidgets Duration field). Create a subclass called .spac (Swim Animator Scene) intended for scenes. Everything is stored in SMIL language. Current and future specific Swim features is stored in the namespace xmlns:swim. All links src is relative to the project directory.

/feature-dev Add support for reusing any other .smil file in a project as ref with editable location in the editor.

/feature-dev Create a .spm, (Swim Animator Movie file), an XML based format which can house a set of .smil files in custom order to become scenes in a editable movie storyboard, provided as a (at default at bottom) dockable panel at the bottom. Apart to playing the scens, it should support a timeline which allows adding music and voice imported to the project assets.

/feature-dev:feature-dev Add support for exporting the .spm as a .ogv through ffmpeg.

/feature-dev:feature-dev Add support for interactivity to the .svg and .smil, based on SMIL interactivity in JS, with flash like code editing panels, utilizing wxWidgets code editor.
