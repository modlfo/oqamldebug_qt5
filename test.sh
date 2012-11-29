#!/bin/bash
ocamlc -o testme -g test.ml || exit
#./testme
make || exit
if [ -e oqamldebug ]
then
    ./oqamldebug testme
else
    open ./oqamldebug.app --args $PWD/testme
fi
