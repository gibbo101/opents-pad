---
title: Give the controller its own pointer and scroll speeds
category: feature
release: 0.2.0
targets:
- type: key
  id: PadPointerSpeed
  effect: added
- type: key
  id: PadFastSpeed
  effect: added
- type: key
  id: PadScrollSpeed
  effect: added
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the pointer, the right shoulder's fast pointer, and the right stick's map scroll take their pace from three new `sun.ini` settings, `PadPointerSpeed`, `PadFastSpeed`, and `PadScrollSpeed`, each `1` to `10` with `5` the built-in pace. The paces are distances on the map, so a push covers the same ground at every zoom and on every display; the stick's response is straight, and the d-pad reaches its full pace within 150 ms of a press. The console-style options screen shows rows for the three in place of the mouse's scroll rate and scroll coasting rows, which the controller does not read. Scrolling at the screen's edge under the controller moves at the pointer's own pace, and the back button is a right-button tap, so holding it while the pointer moves does not drag-scroll the map. The keyboard scheme and the mouse settings are unchanged.
