---
title: Draw controller button glyphs on the console-style prompts
category: feature
release: 0.2.0
targets:
- type: key
  id: PromptStyle
  effect: added
credit: [gibbo101]
---

The accept and back prompts on the console-style screens, and the prompts on the mission briefing under the controller scheme, now carry the button's glyph before the text, from Kenney's public domain Input Prompts: the coloured Xbox letters, the Steam Deck's white letters, or the coloured PlayStation shapes. A new `PromptStyle` setting picks the set, or plain text, and the console-style options screen gains a Button Prompts row for it. The default, `Auto`, shows the Deck set on a Steam Deck, the PlayStation set when a Sony controller is in the system's device list, the Xbox set while a controller is connected, and text otherwise.
