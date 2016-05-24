#!/bin/bash -x

which mpiexec

export PETSC_DIR=/usr/local/Cellar/petsc/3.6.3_4/real
export LD_LIBRARY_PATH=${PETSC_DIR}/lib:$LD_LIBRARY_PATH


OUTPUT_DIR=.

shuf all.tsv -n 2 > ${OUTPUT_DIR}/shuf.tsv
SECONDS=0
date
mpiexec -n 4 ./gflow.x \
	-output_prefix local_ \
	-output_directory ${OUTPUT_DIR} \
	-habitat bb_resist.asc \
	-nodes bbnodes \
	-node_pairs ${OUTPUT_DIR}/shuf.tsv \
	-output_format asc \
	-output_final_current_only 1 \
	-memory_info
 

: "walltime: $SECONDS seconds"

