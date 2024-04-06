#!/bin/bash

seed=$RANDOM

choice=2
#seed=5898399
#seed=19550
#seed=29367
seed=10

echo "start seed:"$seed

../SemiFinalJudge -s $seed -l ERR -f 0 -d ../output.txt \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ./main" \
-m ../maps/map$choice.txt



echo "end"