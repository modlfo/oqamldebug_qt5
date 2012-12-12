ocamlc -o testme.exe -g test.ml 

qmake CONFIG+=debug CONFIG-=release CONFIG-=app_bundle
nmake 
debug\oqamldebug.exe testme.exe
REM devenv /DebugExe debug\oqamldebug.exe testme.exe 
