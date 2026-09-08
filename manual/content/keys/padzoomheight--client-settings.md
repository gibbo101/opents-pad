---
key: PadZoomHeight
scope: client-settings
label: Controller zoom height
see_also: [PadZoomWidth, ControlScheme]
when_omitted:
  kind: value
  value: "0"
---

The render height the game plays at under the `Controller` control scheme; with `PadZoomWidth` it is the controller's zoom. A smaller height renders fewer pixels and scales them up to the display, so the sprites look bigger. The game steps it along a ladder of `480`, `540`, `600`, `660`, `720`, `768`, `840`, `900`, `1080`, `1200`, and `1440`, never past the display's own height, so the picture is never scaled down.

Holding a shoulder button and pushing the right stick up zooms in one step, and down zooms out, with a step every quarter second the stick stays pushed. Each step keeps the centre of the view where it was, saves the new pair to `sun.ini`, and names the new size in the message list. The Zoom row on the console-style options screen steps the same ladder and applies when the screen closes. `0`, or a pair whose shape no longer matches the display's, is replaced by the baseline at the next game: `768`, or `600` on a Steam Deck. The keyboard scheme never reads it.
