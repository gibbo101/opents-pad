---
key: PadZoomHeight
scope: client-settings
label: Controller zoom height
see_also: [PadZoomWidth, ControlScheme]
when_omitted:
  kind: value
  value: "0"
---

The render height the game plays at under the `Controller` control scheme; with `PadZoomWidth` it is the controller's zoom. A smaller height renders fewer pixels and scales them up to the display, so the sprites look bigger. The value is one of `480`, `540`, `600`, `660`, `720`, `768`, `840`, `900`, `1080`, `1200`, and `1440`, never past the display's own height, so the picture is never scaled down. `0`, or a pair whose shape no longer matches the display's, is replaced by the baseline at the next game: `768`, or `600` on a Steam Deck.

The game writes the pair itself each time the zoom steps, from the right shoulder with the right stick in play or from the Zoom row on the console-style options screen, which applies when the screen closes; [Controller scheme](/systems/controller-scheme/#zoom) has the stepping. The keyboard scheme never reads it.
