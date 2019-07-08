# lcdreader-img

Read pixel values from a picture of a (low-resolution) LCD screen.

Usage:
* Read input from `in.png` (or the file specified on the command line),
  output binary data to `out.txt`.
* If a file `config.txt` exists, the positions of the corners position are saved to/
  loaded from that file. If the file is empty, default configuration is used.

Command-line shortcut: Enter `--help` for details.

Keyboard shortcuts:

* `WESD`: Move the corresponding grid corner (assume QWERTY keyboard layout) to the
  mouse pointer. Drag-and-drop the corners is also supported.
* `Q`: Quit.
* `78`: Change preview binary opacity.
* `90`: Change edge.
* `P`: Toggle preview/grid.
* `T`: Toggle showing only image.
* `HJKL`: Move the output cursor.
* `Z`: Set the pixel under the output cursor to 0.
* `X`: Set the pixel under the output cursor to 1.
* `U1`: Allow undistorting the image. The image is undistorted assuming the pinhole camera model.
* `U0`: Disallow undistorting the image, only homography transformation is used. This is the default and is preferred if the image part is small (the distortion is not significant) or there are on a few corner/anchor points.
* `N`: Go to next video frame.
* `RF`: Refresh the image.
* `C`: Set data manual according to a hexadecimal digit pattern (enter 4 digits)
* `RC`: Recognize the hexadecimal digits. Output to stdout.
* `O`: Output.

Mouse control:

* Drag corner/anchor on the input image to move.
* Move mouse on the transformed image to move the output cursor.
* Click on the transformed image to add an anchor point. (must be on the grid)
