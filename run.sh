#!/bin/bash

seed=$RANDOM

choice=-3.8
#seed=8564

echo "start seed:"$seed

../PreliminaryJudge -s $seed -l ERR -f 0 -d ../output.txt \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ./main" \
-m ../maps/map$choice.txt


echo "end"