---
title: Add a controller control scheme with console-style shell screens
category: feature
release: 0.2.0
breaking: true
migration:
- Add `ControlScheme=KeyboardMouse` to the `[Options]` section of `sun.ini` to keep the original dialogs while a controller is connected. A settings file without the key reads as `Auto`.
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
- type: command
  id: fixed:graphic-menu-back
  effect: added
- type: command
  id: fixed:continue-mission-restatement
  effect: added
credit: [gibbo101]
---

A new `ControlScheme` setting chooses between the original keyboard and mouse dialogs and console-style screens driven by the arrow keys, Enter, and Escape, by the first connected XInput controller, or by the mouse. Its default, `Auto`, selects the controller scheme whenever a controller is connected, so an existing install whose settings file has no `ControlScheme` line shows the console-style screens once a controller is plugged in; this is a deliberate change for existing installs, and `ControlScheme=KeyboardMouse` keeps the dialogs. At launch `Auto` waits up to two seconds for a controller only when the system lists a controller-class device or a controller was seen earlier; with no controller the shell starts at once. A controller button pressed on a keyboard-scheme menu screen switches to the controller scheme and shows that screen again as its console version.

Under the controller scheme the shell and the scenario loading screen render at 640x400, the size their artwork was drawn for. The game select page and the two main menu pages keep their artwork and take the controller: the d-pad or stick moves between the buttons by position, accept or Enter activates, back or Escape takes the page's Back button, and an unselected disc is darkened. Skirmish setup, options, and load are console-style screens of rows that edit the same settings as the dialogs: skirmish setup has a numbered map list with its preview and the player colors as swatches, starts the game on accept from any row, and begins its game speed at the saved option; the options screen has a Control Scheme row, a Zoom row, the game settings, and rows to the Audio and Controls screens, and the options page gains a Switch To Controller button; the campaign screen is a side select with the two emblems and the difficulty; the load list shows the saves newest first. The mission briefing turns its pages on accept, turns back a page on back, and plays its video on the third face button, its last page closing with OK before a mission and with Resume Mission from the in-game menu. On the campaign's map select the d-pad and left stick step between the targets and accept picks the focused one. A fullscreen movie is presented alone, with the split sidebar and bar off screen until it ends. Under `KeyboardMouse` the original dialogs are unchanged.
