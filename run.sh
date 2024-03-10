#!/bin/bash

echo "start"

choice=1

./PreliminaryJudge \
"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/lib64 && ./cmake-build-release/main" \
-m ./maps/map$choice.txt


echo "end"