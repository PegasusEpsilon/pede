# Pegasus Epsilon's Desktop Environment (pede)

![My current setup, showing pede taking a whopping 956kB of RAM](https://lh3.googleusercontent.com/qtFfsWdVjdimoeKdtvgyClS4_1ravyIqtP6OSUnRZFQ0ULlfiPuIXSqC9JQTVZurvwT8MgQlfkbLYn9_I2F-GQu4pVl-vTOY1iDkR_-E6oPlG0FhkXzHQh3PixTeq7imN_bifN38898aXjLf8YdswXxew3BPspI4WR1kgSDnh3h3TzGPYDMwvLGr_wrPonf4HY_DhD2YwL0WeCb4uuCHvd_D4b48XU8H1-nBz-2ynC2HZNBjIOG5sjYbeeMAaewdxW-q4OWCwOqXTViYplHkjA6ZAYkYvuJZe2BB7cqblK9HLfcBScZ_2-cB_QM9R3ymTbaBqFlyqLTEFX4QrkgFpGvVrg8HNdj1B-VwwVUYxW6jt-_R27muJlrg8lRfGpSVpx8wvzHUq13Not-qjrbIOvm8Su0_cD-OGwSKJ6Asck5-B6pJR7DEZX4U9Y9lHgSjBkSQCSJZrcZx79U2_qKqw7JRuZ_UPqLnXp0Hw-tcJl0oPVQyffAJU3tasfZHv__FixAD3I-wU7ymeS2JFZMBsTmbCgpMSPOV4jxaN85VTsrCdfIdmI69nwJsYTcrwTuE9WxTcEB3rBEJ0Q7hZeWAn4piCk93XVRptfhPJFByU6VSuxIOgdZcEYmcV73J8x_USzq3bUyz0rJSdBI8RzSNgsidg2426-m5qzGtBhHDRnqks-hZEfibiCByGT8jFu0=w2560-h1080-no)

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
- It snaps windows to each other (kind of, and only when resizing, currently)
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
