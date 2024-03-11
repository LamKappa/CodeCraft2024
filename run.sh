#!/bin/bash

echo "start"

choice=-3.9

../PreliminaryJudge -l ERR \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ../cmake-build-release/main" \
-m ../maps/map$choice.txt


echo "end"