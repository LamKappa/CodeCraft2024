#!/bin/bash

echo "start"

choice=-3.13

../PreliminaryJudge -s 2 -l ERR -d ../output.txt \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ./main" \
-m ../maps/map$choice.txt


echo "end"