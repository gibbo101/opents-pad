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

Under the `Controller` control scheme the player name on the skirmish and LAN screens and the save name in the pause menu open an on-screen keyboard: a row of digits over three rows of letters and a few marks, with Space, Delete, Caps, and Done beneath them, and a field showing the text. The d-pad or stick moves between keys, A types the focused one, X deletes, Y adds a space, and B backs out leaving the text as it was; a real keyboard types straight in, with Enter finishing and Escape backing out. The mouse can pick keys too. The LAN screens use it for chat: each has a Chat row showing the latest message that opens a chat screen with the lobby's messages and a row to type a new one, sent to the game's players or, before joining, to everyone in the lobby.
