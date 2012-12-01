#!/bin/bash
ocamlc -o testme -g test.ml || exit
#./testme
make || exit
if [ -e oqamldebug ]
then
    ./oqamldebug testme
else
    $PWD/oqamldebug.app/Contents/MacOS/oqamldebug $PWD/testme
fi
