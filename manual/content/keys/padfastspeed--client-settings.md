---
key: PadFastSpeed
scope: client-settings
label: Controller fast pointer
see_also: [PadPointerSpeed, PadScrollSpeed, ControlScheme]
when_omitted:
  kind: value
  value: "5"
---

The figure runs from `1` to `10`, and the read holds it to that range. It sets how much a held shoulder button speeds the pointer in play under the `Controller` control scheme: the pointer's pace is multiplied by one plus a quarter of the figure, so `5` gives just over double, `1` a little over the plain pace, and `10` three and a half times it. The edge scroll speeds up with the pointer.

The console-style options screen has a Fast Pointer row that steps the value and writes it back to `sun.ini`. The setting has no effect under the `KeyboardMouse` scheme.
