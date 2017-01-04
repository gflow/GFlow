#!/bin/bash -x

# This script is a documented working example of Gflow with a handful of random pairwise computations (until convergence = 1N) 
# using 4 cpus on Mac OSX where output is only the current density summation. 

# Gflow must be compiled locally before executing this script. Dependencies for GFlow include: openmpi, hypre, and petsc. 
# If you would like to execute a random shuffle of all pairwise, please install 'coreutils'as well.

# Execution flags are commented below and demonstrate an example execution of GFlow. To execute script as is: type 'sh execute_example.sh' 
# in Terminal.

which mpiexec

# Set and add PETSc to PATH (Please update this if you are using Linux or any installation proceedure that differs from the README)
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

	# -output_sum_density_filename
		# Set Output Path, file name, and format of cumulative current density.
		# For use see: https://github.com/Pbleonard/GFlow/issues/8
	# -ouput_prefix  (DEPRICATED)
		# Set Prefix for output files: Currently = 'local_'
	# -ouput_directory (DEPRICATED)
		# Set Output Directory: Set to default above
	# -habitat
		# Set Habitat Map or resistance surface (.asc)
	# -nodes
		# Set Focal Nodes or Source/Destination Points (.txt list of point pairs to calculate or .asc grid). Inputs must be points.
		# If using a .txt list, the point coorindates must be relative to the resistance surface grid. Please look at example inputs.
	# -output_format (DEPRICATED)
		# Set Desired Output formaat --'asc' or 'amps' -- Default = asc
	# -output_final_current_only (DEPRICATED)
		# Select output options. 1 = Only summation; 0 = Pairwise calculations + Summation

# OPTIONAL flags

	# -output_density_filename
		# Set Output Path, file name prefix, and format (i.e., *.asc, *.asc.gz) of pairwise calculations. Omitting this flag will 
		# discard each pairwise solve output and assume you want the cumulative output only.
		# For use see: https://github.com/Pbleonard/GFlow/issues/8
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
	-habitat resistance.asc \
	-nodes nodes \
	-converge_at 1N \
	-shuffle_node_pairs 1 \
	-effective_resistance ./R_eff.csv \
	-output_sum_density_filename "./{time}_local_sum_{iter}.asc" \
 

: "walltime: $SECONDS seconds"

