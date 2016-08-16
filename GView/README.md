# GView
Simple viewer for GFlow output. Viewer allows for rapid review of individual point pair calculations or small pairwise runs.
## Installation

### Mac OS X

The easiest way to setup your environment is with the
[Homebrew](http://brew.sh) package manager.
After Homebrew is set up, run the following commands to install
the required package:

    brew update
    brew install imagemagick

With the required package installed, `GView` can be built by running the following command:

     make
     

### Linux

*Coming soon*...


### Windows

*Not happening*...

## Running

1. Locate output from GFlow and take note of the filepath. 
2. Navigate to folder with Gview. 
3. Open `ViewOutput.sh` in your favorite text editor to examine options for file output and coloramps and save any changes.
4. Type the following into Terminal where `/local.asc` is the actual file path of the Gflow output.
```
sh ViewOutput.sh /local.asc
```
