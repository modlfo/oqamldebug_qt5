ocamlc -o testme.exe -g test.ml || exit

qmake CONFIG+=debug CONFIG-=release CONFIG-=app_bundle
nmake 
debug\oqamldebug testme.exe 'a string with quotes(")'
