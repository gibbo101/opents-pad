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

The width is never chosen on its own: stepping the zoom picks a height, and the width follows as the map's columns on the shape of the display left beside the sidebar, rounded to an even number, plus the sidebar's 168 columns, so the picture fills the display without bars. [Controller scheme](/systems/controller-scheme/#zoom) has the ladder the height steps along and why the sidebar's share of the display never changes with it. The keyboard scheme reads `ScreenWidth` and `ScreenHeight` instead and never this pair.
