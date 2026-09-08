---
title: Keep a wobbly click from becoming a band selection
category: fix
release: 0.2.0
targets: []
credit: [gibbo101]
---

A left-button drag on the tactical map now has to travel four percent of the view height, and at least four pixels, before it becomes a band selection; it was a fixed four pixels at every resolution. A band that is released within a fifth of a second and spans less than a sixth of the view height, or that never grew past the starting distance, is treated as the click it was meant to be and gives the order. A click that wobbles or flicks onward, as it easily does on a trackpad, no longer clears the current selection.
