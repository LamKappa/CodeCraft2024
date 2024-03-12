#!/bin/bash

echo "start"

choice=-3.12

../PreliminaryJudge -s 6 -l ERR -f 0 \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ./main" \
-m ../maps/map$choice.txt


echo "end"