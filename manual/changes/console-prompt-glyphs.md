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

The accept and back prompts on the console-style screens, and the prompts on the mission briefing under the controller scheme, carry the button's glyph before the text: the colored Xbox letters, the Steam Deck's white letters, or the colored PlayStation shapes. A new `PromptStyle` setting picks the set, or plain text, and the console-style options screen gains a Button Prompts row for it. The default, `Auto`, shows the Deck set on a Steam Deck, the PlayStation set for a PlayStation controller, the Xbox set for any other connected controller, and text with no controller connected.
