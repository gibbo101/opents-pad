---
title: Slide the sidebar in and out over the map under the controller scheme
category: feature
release: 0.2.0
targets:
- type: key
  id: PadSidebarSticky
  effect: added
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the sidebar starts away from the map, which takes the whole width under the bar at all times, and the bar ends in a Sidebar tab showing the fourth face button's glyph, which a mouse click also uses, with the credits on the panel's own strip alone. The fourth face button slides the panel in over the map with the pad on it, and slides it out again; anything else that lets the pad go, such as placing a building, slides it out too. The right shoulder with the fourth face button slides it in and keeps it there, and the same again lets it go; that sticky state is saved as the new `PadSidebarSticky` setting and a sticky panel is back on the next scenario's first tick. Nothing else opens the panel. The panel only ever covers the map, so showing or hiding it never moves the view; while it is in, the view may scroll past the map's edge by the strip the panel covers, so the edge can be seen beside it. The keyboard scheme is unchanged.
