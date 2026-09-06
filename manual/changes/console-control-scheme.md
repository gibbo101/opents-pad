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
- type: command
  id: fixed:graphic-menu-navigate
  effect: added
- type: command
  id: fixed:graphic-menu-activate
  effect: added
credit: [gibbo101]
---

A new `ControlScheme` setting chooses between the original keyboard and mouse dialogs and console-style screens. The first console-style screen is skirmish setup: a numbered map list with its preview, the player colors as swatches, and the same settings the dialog offers, starting the same game. It is driven by the arrow keys, Enter, and Escape, or by the first connected XInput controller: d-pad or left stick to move, A to accept, B to go back, and a shoulder button to step values five at a time. Under the controller scheme the game select page and the two main menu pages keep their artwork and take the pad as well: the d-pad or stick moves between the buttons by position, A or Enter activates, Tiberian Sun starts selected, and an unselected disc is darkened so the selected one stands out. The new campaign and options screens have console-style versions too, the campaign one stepping through the campaigns with the difficulty under it, and the options one with a Control Scheme row, the display, game, and sound settings, and the resolution list, and the options page gains a Switch To Controller button so the keyboard scheme can switch as well. The setting defaults to `Auto`, which picks the controller scheme when a controller is connected at launch. Under the controller scheme the shell menus render at 640x400 so they fill any screen. The original dialogs are unchanged and remain the default.
