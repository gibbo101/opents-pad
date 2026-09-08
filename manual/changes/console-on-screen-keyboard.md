---
title: Add an on-screen keyboard for names under the controller scheme
category: feature
release: 0.2.0
targets:
- type: key
  id: ControlScheme
  effect: changed
- type: command
  id: fixed:console-keyboard-navigate
  effect: added
- type: command
  id: fixed:console-keyboard-finish
  effect: added
- type: command
  id: fixed:console-keyboard-cancel
  effect: added
- type: command
  id: fixed:console-keyboard-delete
  effect: added
credit: [gibbo101]
---

Under the `Controller` control scheme the player name on the skirmish and LAN screens and the save name in the in-game menu are typed on an on-screen keyboard: a row of digits over three rows of letters and a few marks, with Space, Delete, Caps, and Done beneath them, and a field showing the text. The controller's menu button opens it from any row of the skirmish and LAN games screens and from the save box, and accept opens it from the game list's Name row. The d-pad or stick moves between keys, accept types the focused one, the third face button deletes, the fourth adds a space, the menu button or accept on Done finishes, and back leaves the text as it was; a real keyboard types straight in, with Enter finishing and Escape backing out, and the mouse picks keys. The LAN screens use it for chat: each lobby has a Chat row showing the latest message that opens a chat screen with the lobby's messages and a row to type a new one, sent to the game's players or, before joining, to everyone in the lobby.
