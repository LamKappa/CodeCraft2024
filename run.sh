#!/bin/bash

echo "start"

choice=-3.10

../PreliminaryJudge -l INFO -f 0 -d ../output.txt \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ./main" \
-m ../maps/map$choice.txt


echo "end"