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

The accept and back prompts on the console-style screens, and the prompts on the mission briefing under the controller scheme, now carry the button's glyph before the text: a coloured ring and letter in the Xbox style, a pale ring and white letter in the Steam Deck style, or the four PlayStation shapes. A new `PromptStyle` setting picks the set, or plain text, and the console-style options screen gains a Button Prompts row for it. The default, `Auto`, shows the Xbox set while a controller is connected and text otherwise.
