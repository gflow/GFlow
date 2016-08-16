#!/bin/bash -xe

# This script will accept the output from GFlow (*.asc) and quickly convert it to various image files. In can be easily executed using only one input -- The .asc file
# Dependencies include Imagemagick. 

# Convert to .png although the user can specify .jpg, .tif, etc.
png=$(basename $1 .asc).png
# Call Gview using an exponential classification (e) and color scale =topo. Resize image to 2048.
./gview.x -e -c topo -o - $1 | convert ppm:- -resize 2048 $png
# Display image
open $png &


