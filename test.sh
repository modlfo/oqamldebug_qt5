#!/bin/bash
ocamlc -o testme -g test.ml || exit
#./testme
qmake CONFIG+=debug CONFIG-=release CONFIG-=app_bundle
make -j4 || exit
./bin/oqamldebug testme 'a string with quotes(")'
