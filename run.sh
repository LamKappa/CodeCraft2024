#!/bin/bash

echo "start"

choice=-3.11

../PreliminaryJudge  \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ../cmake-build-release/main" \
-m ../maps/map$choice.txt 2> ../log.txt


echo "end"