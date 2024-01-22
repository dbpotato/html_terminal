#!/bin/bash

SCRIPT_DIR=$(dirname "$0")

cp $SCRIPT_DIR/WebAppDataTemplate.cpp $SCRIPT_DIR/WebAppData.cpp

for filename in $SCRIPT_DIR/webapp/*; do
    if [ -f "$filename" ]; then
        base=$(basename $filename);
        echo "_resources[\"$base\"] =  R\"\"\"(" >> $SCRIPT_DIR/WebAppData.cpp;
        cat "$filename" >> $SCRIPT_DIR/WebAppData.cpp;
        echo -e ")\"\"\";\n" >> $SCRIPT_DIR/WebAppData.cpp;
    fi
done

echo -e "\n};\n" >> $SCRIPT_DIR/WebAppData.cpp
