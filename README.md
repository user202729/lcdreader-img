# lcdreader-img

Read pixel values from a picture of a (low-resolution) LCD screen.

Usage:
* Read input from `in.png`, output binary data to `out.txt`.
* If a file `config.txt` exists, the positions of the corners position are saved to/
  loaded from that file. If the file is empty, default configuration is used.

Keyboard shortcuts:

* `WESD`: Move the corresponding grid corner (assume QWERTY keyboard layout) to the
  mouse pointer. Drag-and-drop the corners is also supported.
* `Q`: Quit.
* `78`: Change preview binary opacity.
* `90`: Change edge.
* `P`: Toggle preview/grid.
* `B`: Toggle preview binary/median color.
* `O`: Output.
