---
name: Spacely dependency policy
description: Dependency constraints and what libraries are available
type: project
---

wxWidgets (core, base, aui, xml) is the only external dependency. The project deliberately minimizes dependencies. Any new external library requires explicit user permission before adding.

wxXmlDocument (wx/xml/xml.h) is now linked and available for all XML parsing/writing needs.

**Why:** User preference for minimal dependencies.
**How to apply:** Always reach for wxWidgets APIs first. Do not suggest or add third-party libraries without asking.
