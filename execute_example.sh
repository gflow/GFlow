#!/bin/bash -x

# This script will execute Gflow with 4-5 random pairwise computations (until convergence = 1N) using 4 cpus on Mac OSX-
# and output only the current density summation. 
# The default inputs include 588 node pairs that solve > 34.25 million unknowns. 
# Gflow must be compiled locally before executing this script. Dependencies for Gflow include: openmpi, hypre, and petsc. 
# If you would like to execute a random shuffle of all pairwise, please install 'coreutils'as well.

# Example flags are commented below to demonstrate example execution of GFlow

which mpiexec

# Set and add PETSc to PATH
export PETSC_DIR=/usr/local/Cellar/petsc/3.7.3/real
export LD_LIBRARY_PATH=${PETSC_DIR}/lib:$LD_LIBRARY_PATH

# Set Output Directory: Default is Current Directory
OUTPUT_DIR=.

# DEBUG: Set number of random pairs to calculate from all possible pairs (Currently n=5). Must be used with -node_pairs flag
# Allows exact number of test pairs. Remove or comment line for all pairwise. Requires 'coreutils' and 'all.tsv' from inputs.
# gshuf all.tsv -n 5 > ${OUTPUT_DIR}/shuf.tsv 

# Set the Clock
SECONDS=0
date
# Set Number of processes (CPUs) and call Gflow. Currently n = 4
	# -Set Prefix for output files: Currently = 'local_'
	# -Set Output Directory: Set to default above
	# -Set Habitat Map or resistance surface (.asc)
	# -Set Focal Nodes or Source/Destination Points (.txt list of point pairs to calculate or .asc grid)
	# -Calculate only desired node pairs if input (e.g., '${OUTPUT_DIR}/shuf.tsv \' from gshuf above). Currently null.
	# -Set convergence factor (Flag examples; '-converge_at 4N /' or '-converge_at 0.9999 /') Currently set to '1N' for testing. 
	#  Flag is optional, remove if not in use.
	# -Shuffles pairs for random selection. Input is binary. Currently = 1 to shuffle.
	# -Print effective resistance to log file. Supply path for .csv
	# -Set output format -- 'asc' or 'amps' -- Default = asc
	# -Select output options. 1 = Only summation; 0 = Pairwise calculations + Summation

mpiexec -n 4 ./gflow.x \
	-output_prefix local_ \
	-output_directory ${OUTPUT_DIR} \
	-habitat resistance.asc \
	-nodes nodes \
	-node_pairs \
	-converge_at 1N \
	-shuffle_node_pairs 1 \
	-effective_resistance ${OUTPUT_DIR}/R_eff.csv \
	-output_format asc \
	-output_final_current_only 1
 

: "walltime: $SECONDS seconds"
