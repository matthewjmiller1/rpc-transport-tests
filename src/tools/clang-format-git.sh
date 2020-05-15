#!/bin/bash

FILES="src/rt_client_server/*.cc"
FILES="$FILES src/rt_client_server/*.hpp"
FILES="${FILES} src/rt_client_server/transports/*/*.cc"
FILES="${FILES} src/rt_client_server/transports/*/*.hpp"

# Running it twice corrects some bugs in clang-format.
for run in {1..2}
do
    git clang-format HEAD^ -- $FILES
done

