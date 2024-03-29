# DUE TO RECENT GITHUB POLICY CHANGES ALL MY PROJECTS HAVE BEEN MOVED TO [GITLAB](https://gitlab.com/pegasusepsilon)

# Pegasus Epsilon's Desktop Environment (pede)

![My current setup, showing pede taking a whopping 952kB of RAM](https://live.staticflickr.com/65535/49795270571_6f23c61ba4_o.png)

Pegasus Epsilon's Desktop Environment (pede) is an attempt to build a better
mousetrap. I think I'm doing a pretty decent job, honestly.

## Some information about pede:

- It does not reparent
- It does not composite (we use xcompmgr for that stuff)
- It does not read any configuration files other than the power button image data
- It does not draw any UI widgets other than its power button
  - No, not even a confirmation dialog when you click the button. Be careful.
  - No, not even window decorations.
  - No, not even title bars.
  - No, not even resize indicators.

This DE is not for the faint of heart. It is for the extreme minimalist.

## What *does* pede do, then?

- ~~It takes less than a megabyte of RAM, even with how fat linux has gotten~~ Unfortunately a recent rewrite took it to 1,100kB on load - just 76kB larger than a megabyte. Xlib shenanigans see that grow to 2,232kB before stabilizing, occasionally going back down. A rewrite in xcb will help, and it's already started, but it will be a while.
- It positions windows. It positions windows ***well***.
- It closes windows politely if they ask for politeness, rudely if they do not
- It has workspaces
- It lets you switch between workspaces
  - default: super+[1234], super+(left|right) arrow
- It lets you move windows between workspaces
  - default: super+shift+[1234], super+shift(left|right) arrow
- It lets you move and resize windows with the mouse
  - default: Super+left-click = move
  - default: Super+right-click = resize
- It lets you restack windows
  - default: alt+tab
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
- pulse-volume in ~/.pede/
- pede_badge-symbolic.svg in /usr/share/icons/hicolor/scalable/places/
- pede.desktop in /usr/share/xsessions/
