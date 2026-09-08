---
key: PadFastSpeed
scope: client-settings
label: Controller fast pointer
see_also: [PadPointerSpeed, PadScrollSpeed, ControlScheme]
when_omitted:
  kind: value
  value: "5"
---

The figure runs from `1` to `10`, and the read holds it to that range. It sets how much a held right shoulder speeds the pointer in play under the `Controller` control scheme: the pointer's pace is multiplied by one plus 0.24 times the figure, so `1` gives 1.24 times the plain pace, `5` gives 2.2 times, and `10` gives 3.4 times. The edge scroll speeds up with the pointer.

The console-style options screen has a Fast Pointer row that steps the value and writes it back to `sun.ini`. The setting has no effect under the `KeyboardMouse` scheme.
