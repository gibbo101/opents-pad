---
title: Add a controller control scheme with a console-style skirmish screen
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: added
- type: command
  id: fixed:console-menu-navigate
  effect: added
- type: command
  id: fixed:console-menu-accept
  effect: added
- type: command
  id: fixed:console-menu-back
  effect: added
credit: [gibbo101]
---

A new `ControlScheme` setting chooses between the original keyboard and mouse dialogs and console-style screens. The first console-style screen is skirmish setup: a numbered map list with its preview, the player colors as swatches, and the same settings the dialog offers, starting the same game. It is driven by the arrow keys, Enter, and Escape, or by the first connected XInput controller: d-pad or left stick to move, A to accept, B to go back, and a shoulder button to step values five at a time. The original dialogs are unchanged and remain the default.
