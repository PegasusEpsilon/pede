# Pegasus Epsilon's Desktop Environment (pede)

Pegasus Epsilon's Desktop Environment (pede) is an attempt to build a better
mousetrap. I think I'm doing a pretty decent job, honestly.

## Some information about pede:

- It does not reparent
- It does not composite
- It does not handle window maximization states (yet, it's on my TODO)
- It does not read any configuration files other than the power button image data
- It does not draw any UI widgets other than its power button
  - No, not even a confirmation dialog when you click the button. Be careful.
  - No, not even window decorations.
  - No, not even title bars.
  - No, not even resize indicators.

This DE is not for the faint of heart. It is for the extreme minimalist.

## What *does* pede do, then?

- It takes less than a megabyte of RAM, even with how fat linux has gotten
- It positions windows
- It closes windows politely if they ask for politeness, rudely if they do not
- It has workspaces
- It lets you switch between workspaces (Super+[1234])
- It lets you move windows between workspaces (Super+Shift+[1234])
- It lets you move and resize windows with the mouse
- It lets you alt+tab (barely, I know, it's on my TODO)
- It snaps windows to the edge of the screen (but not each other yet, also TODO)
- It draws a compositor-friendly power button in a configurable location
- It compiles quickly, which is good, because...
- It is reconfigured by editing the source and recompiling
- It relaunches itself when it receives the USR1 signal, without letting X close
- It will improve over time, until I am happy with it
- And if you want it to do more, you can edit the source.

If you try pede, like it, work on it, whatever, let me know. I won't be
accepting most pull requests, but I encourage you to make them anyway. If you
make a change I like, I'll merge it. But know that the roadmap is in flux, and
I currently hope to have a module loading system in this thing eventually, so
the end user can actually reconfigure it in a slightly more sensible manner.

Anyway, that's it. Go play with the code.
