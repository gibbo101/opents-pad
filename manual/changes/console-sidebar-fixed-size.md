---
title: Keep the sidebar the same size on the display at every controller zoom
category: feature
release: 0.2.0
targets:
- type: key
  id: PadZoomWidth
  effect: changed
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the sidebar is drawn at the height of its own panel, the radar and mode buttons over five rows of two cells with nothing spare below, whatever height the map renders at, and scaled to fill the display's height beside the map, so zooming the map no longer grows or shrinks the sidebar and the five rows always fill the display. The map's columns take the shape of the display left beside it, and `PadZoomWidth` is that width plus the sidebar's 168 columns; a saved width from before this change no longer fits and falls back to the baseline once. The keyboard scheme keeps the sidebar in the frame and is unchanged.
