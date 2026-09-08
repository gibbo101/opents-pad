---
key: PadZoomWidth
scope: client-settings
label: Controller zoom width
see_also: [PadZoomHeight, ControlScheme]
when_omitted:
  kind: value
  value: "0"
---

The render width the game plays at under the `Controller` control scheme, saved with `PadZoomHeight` as the pair that makes up the controller's zoom. The game writes the pair itself: `0` in either, or a pair whose shape is more than a percent away from the display's, is replaced by the baseline when a game starts, a height of `768` on most displays and `600` on a Steam Deck, with the width that gives the display's own shape. A saved pair on the display's shape is used as it is.

The width is never chosen on its own. Stepping the zoom, with a shoulder button and the right stick in play or the Zoom row on the console-style options screen, picks a height from a fixed ladder and derives the width from the display's shape, rounded to an even number, so the picture fills the display without bars. The keyboard scheme reads `ScreenWidth` and `ScreenHeight` instead and never this pair.
