# GFlow
Software for modeling circuit theory-based connectivity

## Installation

### Mac OS X

The easiest way to setup your environment is with the
[Homebrew](http://brew.sh) package manager.
After Homebrew is set up, run the following commands to install
the required packages:

    brew update
    brew install openmpi
    brew install homebrew/science/hypre
    brew install homebrew/science/petsc

**Note**: Advanced users may build their own versions of PETSc and an MPI library, `gflow` does not require
any specific preconditioner (we recommend `hypre`) nor does it depend on a particular
MPI implementation

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
