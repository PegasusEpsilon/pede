# Pegasus Epsilon's Desktop Environment (pede)

![My current setup, showing pede taking a whopping 956kB of RAM](https://lh3.googleusercontent.com/rwtuRRqJVGeWGR11LKNZB3kfgP9U7b2hE_FsXQ_6VKiDkBjKJvSldJIX_JebVAWhUppetyGjaiZIWVA77Vbj0dNGPw-e85SG53IM8x4niosrPmlB4whiVOvYg89REkGZ3TUiYQfJ3QNldiJPqIWPUybs7eH9ivDxraRrw8ayqGeMeYz1xyHOrxCc6aYIFlDiQL2XjqwcIsZvQ-w6WmykysH88qgBQQ7UMKg5dRjmFexS60KwYRtNGetdJ1DmI90Ch0hAG8pwObdDWPf91nRoCL3JzprwM0Pqh0qVawTkCM7Gq-nT0-7F8yAQC4XBLHGR8aA6f84pTdT5S_SWLK2RjMrylSGzxgcfPvdKTxgKDZZr80c1KsE94rzExaTjS1bv27rRMFTISuzHn2rbuO3Dx_ZatasR9cjQTGkLQwWQ_-1CJUDhldC4m0eTFOfQZsKNoEFLfBCvbit9js74YY5D0VcKuj_bI8Cs04oYUQcNJsND80fkObmM4LfsKRLbn6TS7Im9ueRhM7FCz6BWxveXx4y7IOyxzHzSu6U0NZe4JzftfZUp5k_z3GAMV9kfLvvSSMrcj2Egst-L0TLIf4AaoIqrBSN_CrmGt5JEh6U5PcU0bL6AR9N6m9FKYtFDoREhRc7L158RiegUBzOmbkPYtIUwZ6yBME-H5z4bPiFlFjAv5v71neCPOobtyxaI5qc=w2560-h1080-no)

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

- It takes less than a megabyte of RAM, even with how fat linux has gotten
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
