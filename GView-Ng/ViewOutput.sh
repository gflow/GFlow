#!/bin/bash -xe

make

# This script will accept the output from GFlow (*.asc) and quickly convert it to various image files. In can be easily executed using only one input -- The .asc file
# Dependencies include Imagemagick. 

# Call Gview using an exponential classification (e) and color scale =topo. Resize image to 2048.
./gview.x -e -c topo --width 4096 $1
# Display image
open $(basename $1 .asc).png &


