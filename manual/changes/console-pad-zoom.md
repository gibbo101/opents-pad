---
title: Give the controller a zoom in place of a resolution
category: feature
release: 0.2.0
targets:
- type: key
  id: PadZoomWidth
  effect: added
- type: key
  id: PadZoomHeight
  effect: added
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme a game renders at a zoom rather than at `ScreenWidth` and `ScreenHeight`: a render height from a fixed ladder with the width following the display's shape, saved as `PadZoomWidth` and `PadZoomHeight` in `sun.ini`. The right shoulder with the right stick steps the zoom in play, keeping the view's center, and the console-style options screen has a Zoom row in place of a resolution list. A saved size whose shape no longer matches the display falls back to the baseline of `768` high, `600` on a Steam Deck. The keyboard scheme and its resolution settings are unchanged.
