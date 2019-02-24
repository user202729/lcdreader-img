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
	* `s`: Number of frames to skip at the begin of the video. Default 0.

Keyboard shortcuts:

* `WESD`: Move the corresponding grid corner (assume QWERTY keyboard layout) to the
  mouse pointer. Drag-and-drop the corners is also supported.
* `q`: Quit.
* `78`: Change preview binary opacity.
* `90`: Change edge.
* `p`: Toggle preview/grid.
* `t`: Toggle showing only image.
* `z`: Set the pixel under the cursor to 0.
* `x`: Set the pixel under the cursor to 1.
* `o`: Output.
* `H`: Go back 99 frames.
* `L`: Go forward 101 frames.
* `g`: Go to frame number (read from stdin)
* `n`: Go to next video frame.
* `f`: Show the current frame number.
