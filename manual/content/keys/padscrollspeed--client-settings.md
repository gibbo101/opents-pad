---
key: PadScrollSpeed
scope: client-settings
label: Controller stick scroll speed
see_also: [PadPointerSpeed, PadFastSpeed, ControlScheme]
when_omitted:
  kind: value
  value: "5"
---

The figure runs from `1` to `10`, and the read holds it to that range. It scales how fast the right stick scrolls the map in play under the `Controller` control scheme: at `5` a full push crosses about three view heights a second, `10` doubles that, and `1` gives a fifth of it. The stick's response is squared, so a light push scrolls gently at every setting.

The console-style options screen has a Stick Scroll Speed row that steps the value and writes it back to `sun.ini`. The mouse's `ScrollRate` and `ScrollMethod` settings do not touch the controller, and this setting has no effect under the `KeyboardMouse` scheme.
