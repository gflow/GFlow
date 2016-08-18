# GFlow
[![DOI](https://zenodo.org/badge/23503/Pbleonard/GFlow.svg)](https://zenodo.org/record/60312#.V7OSfZMrIUE)

Software for modeling circuit theory-based connectivity at any scale. We developed GFlow to solve large ecological problems in a High Performance Computing environment. If solving a small problem or if you are willing to sacrifice some performance, it can be deployed on a desktop computer.

## Installation

### [Please download latest release with sample data.] (https://github.com/Pbleonard/GFlow/releases/latest)

### Mac OS X
The easiest way to setup your environment is with the
[Homebrew](http://brew.sh) package manager.
After Homebrew is set up, run the following commands to install
the required packages:

    brew update
    brew install openmpi
    brew install homebrew/science/hypre
    brew install homebrew/science/petsc
    brew install coreutils					# optional

**Note**: Advanced users may build their own versions of PETSc and an MPI library, `gflow` does not require
any specific preconditioner (we recommend `hypre`) nor does it depend on a particular
MPI implementation.

With the required packages installed, `gflow` can be built by running the following command:

     make
     
If you encounter errors related to PETSc, you may have to edit the `Makefile` to change the 
value of the `PETSC_DIR` variable.

Currently, there is no mechanism to automatically copy the `gflow.x` binary to a centrally-located
directory.


### Linux

*Coming soon*...


### Windows

*Not happening*...


## Running 

*Instructions apply to Mac Desktop Computers only. Advanced use for cluster computing follows a very similar proceedure sans submission script details.*

**Easiest using Terminal** 

1. Navigate to the directory where you downloaded GFlow. 

2. Extract the zipped example input files (`inputs.tar.gz`) into the current directory. *E.g., using terminal:*
```
    gunzip -c inputs.tar.gz | tar xopf -
```
3. Open the commented example `execute_example.sh` script with your favorite text editor and examine the format, default settings, and save any necessary 
adjustments. Otherwise, the script is ready to submit and solve the example problem.

4. If terminal is not open, open now and navigate to GFlow directory. Execute script:
```
    sh execute_example.sh
```

