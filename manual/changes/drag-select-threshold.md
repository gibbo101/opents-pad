---
title: Keep a wobbly click from becoming a band selection
category: fix
release: 0.2.0
targets:
- type: system
  id: target-selection
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme, a left-button drag on the tactical map has to travel four percent of the view height, and at least four pixels, before it becomes a band selection. A band released within a fifth of a second that spans less than a sixth of the view height, or that never grew past the starting distance, is treated as the click it was meant to be and gives the order, so a click that wobbles or flicks onward on a trackpad does not clear the current selection. The `KeyboardMouse` scheme keeps the engine's fixed four pixels and no flick rule.
