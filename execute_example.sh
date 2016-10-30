#!/bin/bash -x

# This script is a documented working example of Gflow with 4-5 random pairwise computations (until convergence = 1N) 
# using 4 cpus on Mac OSX and output only the current density summation. 
# The default inputs include 588 node pairs that solve > 34.25 million unknowns. 
# Gflow must be compiled locally before executing this script. Dependencies for Gflow include: openmpi, hypre, and petsc. 
# If you would like to execute a random shuffle of all pairwise, please install 'coreutils'as well.

# Execution flags are commented below and demonstrate an example execution of GFlow. To execute script as is: type 'sh execute_example.sh' 
# in Terminal.

which mpiexec

# Set and add PETSc to PATH
export PETSC_DIR=/usr/local/Cellar/petsc/3.7.3/real
export LD_LIBRARY_PATH=${PETSC_DIR}/lib:$LD_LIBRARY_PATH

# Set Output Directory: Default is Current Directory
OUTPUT_DIR=.

# DEBUG: Set number of random pairs to calculate from all possible pairs (Currently -n=5). Must be used with -node_pairs flag
# Allows exact number of test pairs. Remove or comment line for all pairwise. Requires 'coreutils' and 'all.tsv' from inputs.
	# gshuf all.tsv -n 5 > ${OUTPUT_DIR}/shuf.tsv 

# Set the Clock
SECONDS=0
date

# REQUIRED flags to execute GFlow are below. Please read descriptions or see example arguments for use.

#	Set Number of processes (CPUs) and call Gflow. Currently -n = 4 below
	# -ouput_prefix
		# Set Prefix for output files: Currently = 'local_'
	# -ouput_directory
		# Set Output Directory: Set to default above
	# -habitat
		# Set Habitat Map or resistance surface (.asc)
	# -nodes
		# Set Focal Nodes or Source/Destination Points (.txt list of point pairs to calculate or .asc grid). Inputs must be points.
	# -output_format
		# Set Desired Output formaat --'asc' or 'amps' -- Default = asc
	# -output_final_current_only
		# Select output options. 1 = Only summation; 0 = Pairwise calculations + Summation

# OPTIONAL flags

	# -node_pairs 
		# Calculate only desired node pairs if input (e.g., '${OUTPUT_DIR}/shuf.tsv \' from gshuf above). Currently not used.
	# -converge_at
		# Set Convergence Factor to stop calculating. Typically used in place of 'node_pairs' or if all pairwise is too
		# computationlly time consuing. Acceptable formats include: '4N' or '.9999'. Set to '1N' Below. 
	# -shuffle_node_pairs
		# Shuffles pairs for random selection. Input is binary. Currently set to shuffle below (= 1)
	# -effective_resistance
		# Print effective resistance to log file. Supply path for .csv

# Assigning Arguments to Flags for Execution:

mpiexec -n 4 ./gflow.x \
	-output_prefix local_ \
	-output_directory ${OUTPUT_DIR} \
	-habitat resistance.asc \
	-nodes nodes \
	-converge_at 1N \
	-shuffle_node_pairs 1 \
	-effective_resistance ${OUTPUT_DIR}/R_eff.csv \
	-output_format asc \
	-output_final_current_only 1
 

: "walltime: $SECONDS seconds"

