# Dreampresent: The Dreamcast Presentation Tool

This is a presentation tool that runs on **Sega Dreamcast**. It's written in 
**Ruby** and uses [mruby](https://mruby.org/).

This tool was initially developed by Yuji Yokoo (@yujiyokoo) for the
**RubyConf TW 2019** event (later it was updated for **RubyConf AU 2020**).

## Usage

You can use `A` or `START` to move forward, and use `B` to go back to the
previous page.

Hold `A + B + Start` to quit.

The bottom of the screen shows the page progress and time progress (blue
dreamcast swirl for page, red mruby for time).

The time is currently hardcoded to `35` minutes.

If you press `Right` on the D-pad, you move the time forward by 5 minutes,
and back by 5 minutes if you press `Left`.

## Final note

The initial source is available [here](https://github.com/yujiyokoo/dreampresent).
This version of the source code is almost the same but adapted to be included as
an example in KallistiOS, with Yuji Yokoo approval.
