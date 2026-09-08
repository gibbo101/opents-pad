---
title: Add a console-style in-game menu under the controller scheme
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
credit: [gibbo101]
---

Under the `Controller` control scheme the in-game menu is a console-style screen rendered at the shell's 640x400 size on the mission's score plate, with Game Options, Audio Options, Controls, Mission Briefing in a campaign, Save Game, Load Game, Restart Mission, Abort Mission, and Return To Mission as rows; Restart and Abort each ask as a row whose value flips between No and Yes. Game Options opens the options screen without its display rows or the campaign difficulty; Save Game offers the save under a suggested name, the mission's side and number ahead of its name in a campaign, which the menu button edits on the on-screen keyboard, and accept writes a fresh slot and reports beneath the menu; Load Game opens the console-style load list. In a LAN game Save and Load take the multiplayer paths and Restart becomes Surrender. The `KeyboardMouse` dialogs are unchanged.
