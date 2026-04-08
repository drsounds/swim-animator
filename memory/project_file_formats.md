---
name: Spacely file format decisions
description: File format conventions and decisions made for the project
type: project
---

All new file formats in Spacely must be XML-based (CLAUDE.md updated to reflect this). The old "JSON-based" directive was superseded when the project decided to use open-standard XML formats.

The .drw drawing format was migrated to SVG (.svg) using wxXmlDocument. Spacely-specific data not expressible in standard SVG is stored under the namespace xmlns:swim="https://swim.spacify.se/TR/". The old plain-text .drw format was dropped with no backward compat (clean break was acceptable at this stage).

Future planned formats (from Tasks.md): .spa (ZIP project container), .spac (SMIL scene), .spm (movie storyboard) — all XML-based.

**Why:** Standardization, interoperability with SVG viewers, and debuggability.
**How to apply:** When designing any new file format in this project, use XML. Use wxXmlDocument (already linked via the xml wxWidgets component). Spacely-specific extensions go in the swim: namespace.
