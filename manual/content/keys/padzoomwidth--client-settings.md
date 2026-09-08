---
key: PadZoomWidth
scope: client-settings
label: Controller zoom width
see_also: [PadZoomHeight, ControlScheme]
when_omitted:
  kind: value
  value: "0"
---

The render width the game plays at under the `Controller` control scheme, saved with `PadZoomHeight` as the pair that makes up the controller's zoom. The game writes the pair itself: `0` in either, or a width more than a percent away from the one the display gives the height, is replaced by the baseline when a game starts, a height of `768` on most displays and `600` on a Steam Deck, with the width the display gives it. A saved pair that still fits the display is used as it is.

The width is never chosen on its own. Stepping the zoom, with a shoulder button and the right stick in play or the Zoom row on the console-style options screen, picks a height from a fixed ladder and derives the width from it: the map's columns take the shape of the display left beside the sidebar, rounded to an even number, and the sidebar's 168 columns are added, so the picture fills the display without bars. The sidebar is drawn at the height of its own five-row panel whatever the map's zoom and scaled to fill the display's height, so its size on the display never changes with the zoom, and the map's share of the width follows. The keyboard scheme reads `ScreenWidth` and `ScreenHeight` instead and never this pair.
