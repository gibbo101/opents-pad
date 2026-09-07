---
key: PadPointerSpeed
scope: client-settings
label: Controller pointer speed
see_also: [PadFastSpeed, PadScrollSpeed, ControlScheme]
when_omitted:
  kind: value
  value: "5"
---

The figure runs from `1` to `10`, and the read holds it to that range. It scales how fast the left stick and the d-pad move the pointer in play under the `Controller` control scheme: `5` is the built-in pace, `10` twice it, and `1` a fifth of it. The stick's response is squared, so a light push stays slow at every setting. Map scrolling at the screen's edge follows the pointer's pace, so this setting governs that too.

The console-style options screen has a Pointer Speed row that steps the value and writes it back to `sun.ini`. The mouse's `ScrollRate` and `ScrollMethod` settings do not touch the controller, and this setting has no effect under the `KeyboardMouse` scheme.
