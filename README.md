# lcdreader-img

Read pixel values from a picture of a (low-resolution) LCD screen.

Usage:
* Read input from `in.png` (or the file specified on the command line),
  output binary data to `out.txt`.
* If a file `config.txt` exists, the positions of the corners position are saved to/
  loaded from that file. If the file is empty, default configuration is used.

Command-line shortcut:
* Format: `key=value`
* Possible key values:
    * `f`: file name.
	* `w`, `h`: grid width/height.

Keyboard shortcuts:

* `WESD`: Move the corresponding grid corner (assume QWERTY keyboard layout) to the
  mouse pointer. Drag-and-drop the corners is also supported.
* `Q`: Quit.
* `78`: Change preview binary opacity.
* `90`: Change edge.
* `P`: Toggle preview/grid.
* `T`: Toggle showing only image.
* `Z`: Set the pixel under the cursor to 0.
* `X`: Set the pixel under the cursor to 1.
* `O`: Output.
* `N`: Go to next video frame.
