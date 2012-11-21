#!/bin/bash
ocamlc -o testme -g test.ml || exit
#./testme
make || exit
./oqamldebug testme
