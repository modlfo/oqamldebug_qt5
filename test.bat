ocamlc -o testme.exe -g test.ml || exit

qmake CONFIG+=debug CONFIG-=release CONFIG-=app_bundle
nmake 
devenv /DebugExe debug\oqamldebug.exe testme.exe 'a string with quotes(")'
