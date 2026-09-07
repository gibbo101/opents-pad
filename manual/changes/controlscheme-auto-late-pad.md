---
title: Let an Auto control scheme find a controller Steam attaches after launch
category: fix
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

`ControlScheme=Auto` decided at launch alone, and Steam hands a game its virtual controller a moment after the window exists, so a pad on the desktop through Big Picture landed in the keyboard scheme. Auto now keeps checking through the startup movies and waits up to two seconds for a pad before the first menu screen, so the scheme is settled before anything is shown. If a controller still turns up later, a button pressed on it while a menu screen is up switches to the controller scheme and that screen shows again as its console version.
