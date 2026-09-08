---
title: Draw the controller pointer onto a nearby unit when it stops
category: feature
release: 0.2.0
targets:
- type: key
  id: PadSnap
  effect: added
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme a pointer that comes to rest near a unit or building is warped onto it, within a distance the new `PadSnap` setting sets in eighths of a cell, `4` by default and `0` for off; the console-style options screen gains a Unit Snap row for it.
