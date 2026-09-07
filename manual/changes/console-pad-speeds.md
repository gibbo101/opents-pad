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

Under the `Controller` control scheme the pointer, the shoulder button's fast pointer, and the right stick's map scroll take their pace from three new `sun.ini` settings, `PadPointerSpeed`, `PadFastSpeed`, and `PadScrollSpeed`, each `1` to `10` with `5` the built-in pace. The console-style options screen shows rows for the three in place of the mouse's scroll rate and scroll coasting rows, which the controller no longer reads. Scrolling at the screen's edge under the controller moves at the pointer's own pace, and the back button is a right-button tap, so holding it while the pointer moves no longer drag-scrolls the map. The keyboard scheme and the mouse settings are unchanged.
