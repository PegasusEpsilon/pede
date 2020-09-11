# Pegasus Epsilon's Desktop Environment (pede)

![My current setup, showing pede taking a whopping 952kB of RAM](https://live.staticflickr.com/65535/49795270571_6f23c61ba4_o.png)

Pegasus Epsilon's Desktop Environment (pede) is an attempt to build a better
mousetrap. I think I'm doing a pretty decent job, honestly.

## Some information about pede:

- It does not reparent
- It does not composite
- ~~It does not handle window maximization states (yet, it's on my TODO)~~ It does now!
- It does not read any configuration files other than the power button image data
- It does not draw any UI widgets other than its power button
  - No, not even a confirmation dialog when you click the button. Be careful.
  - No, not even window decorations.
  - No, not even title bars.
  - No, not even resize indicators.

This DE is not for the faint of heart. It is for the extreme minimalist.

## What *does* pede do, then?

- ~~It takes less than a megabyte of RAM, even with how fat linux has gotten~~ Unfortunately a leak in Xlib causes this to be incorrect, and as far as I can tell, impossible to fix. I will rewrite under xcb soon, in the hopes of fixing this issue.
- It positions windows. It positions windows ***well***.
- It closes windows politely if they ask for politeness, rudely if they do not
- It has workspaces
- It lets you switch between workspaces (Super+[1234], Super+(left|right) arrow)
- It lets you move windows between workspaces (Super+Shift+[1234], Super+Shift+(left|right) arrow)
- It lets you move and resize windows with the mouse
  - Super+left-click = move
  - Super+right-click = resize
- It lets you alt+tab (barely, I know, it's on my TODO)
- It snaps windows to the edge of the screen
- It snaps windows to each other (kind of, might leave it this way, I kinda like it?)
- It draws a compositor-friendly power button in a configurable location
- It compiles quickly, which is good, because...
- It is reconfigured by editing the source and recompiling
- It relaunches itself when it receives the USR1 signal, without letting X close
- It will improve over time, until I am happy with it
- And if you want it to do more, you can edit the source.

If you try pede, like it, work on it, whatever, let me know. I won't be
accepting most pull requests, but I encourage you to make them anyway. If you
make a change I like, I'll merge it. But know that the roadmap is in flux.

Anyway, that's it. Go play with the code.

Places files go on my system:
- pede_badge-symbolic.svg in /usr/share/icons/hicolor/scalable/places/
- pede.desktop in /usr/share/xsessions/
