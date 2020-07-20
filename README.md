# lcdreader-img

Read pixel values from a picture of a (low-resolution) LCD screen.

### Note:

On different platforms, `cv::waitKey()` function might not return different key values
depend on whether Shift key is pressed; however some of the keyboard shortcuts depends on that.
Because (as far as I know) there's no way to solve the problem in pure OpenCV,
if `USE_X11` is 1 in the makefile then X library is used to determine the state of the Shift key.
On Windows, `make USE_X11=0` is required.

### Usage:
* Read input from `in.png` (or the file specified on the command line),
  output binary data to `out.txt`.
* If a file `config.txt` exists, the positions of the corners position are saved to/
  loaded from that file. If the file is empty, default configuration is used.

Command-line shortcut: Enter `--help` for details.

The command-line flag format must be `-<flag>=<value`. For example `-f=a.mp4`.

### Keyboard shortcuts:

* `wesd`: Move the corresponding grid corner (assume QWERTY keyboard layout) to the
  mouse pointer. Drag-and-drop the corners is also supported.
* `q`: Quit.
* `78`: Change preview binary opacity.
* `90`: Change edge.
* `p`: Toggle preview/grid.
* `t`: Toggle showing only image.
* `hjkl`: Move the output cursor.
* `YUIO`: Move the most recently moved corner by 1 image pixel (depends on the zoom factor).
* `z`: Set the pixel under the output cursor to 0.
* `x`: Set the pixel under the output cursor to 1.
* `u1`: Allow undistorting the image. The image is undistorted assuming the pinhole camera model.
* `u0`: Disallow undistorting the image, only homography transformation is used. This is the default and is preferred if the image part is small (the distortion is not significant) or there are on a few corner/anchor points.
* `H`: Go back 99 frames.
* `L`: Go forward 101 frames.
* `g`: Go to frame number (read from stdin)
* `n`: Go to next video frame.
* `<space>`: Toggle play/pause.
* `rf`: Refresh the image.
* `c`: Set data manual according to a hexadecimal digit pattern (enter 4 digits)
* `rc`: Recognize the hexadecimal digits. Output to stdout.
* `o`: Output.

### Mouse control:

* Drag corner/anchor on the input image to move.
* Move mouse on the transformed image to move the output cursor.
* Click on the transformed image to add an anchor point. (must be on the grid)
