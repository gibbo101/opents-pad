---
key: PadSnap
scope: client-settings
label: Controller unit snap
see_also: [PadPointerSpeed, ControlScheme]
when_omitted:
  kind: value
  value: "4"
---

How far a resting controller pointer is drawn onto a nearby unit or building under the `Controller` control scheme, in eighths of a cell: `0` turns the snap off, `4` is half a cell, and `10` the most, a cell and a quarter. The read holds the figure to `0` to `10`.

When the left stick and the d-pad come to rest, the game resolves the pointer's map position as it would for a click and, unless the pointer is already over an object, looks through the cells around it for the closest unit, infantry, aircraft or building within that distance, leaving out cloaked units of other players, and warps the pointer onto its center. The pointer is never pulled while it is traveling or while the accept button is held, so a band selection is not disturbed.

The console-style options screen has a Unit Snap row that steps the value, showing Off at `0`, and writes it back to `sun.ini`. The setting has no effect under the `KeyboardMouse` scheme.
