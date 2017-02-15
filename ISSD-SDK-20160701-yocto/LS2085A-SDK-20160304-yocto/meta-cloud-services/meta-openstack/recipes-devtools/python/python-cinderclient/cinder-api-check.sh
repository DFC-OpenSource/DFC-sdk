#! /bin/bash

CMD="cinder list"

data=$($CMD 2>&1)
res=$?
if [ ${res} -eq 127 ]; then
    exit 0
elif [ ${res} -ne 0 ]; then
    echo "OpenStack \"cinder api\" failed: "
    echo $data
    exit $res
fi
exit 0
