CONFIG -= app_bundle
VERSION=0.0.0

HEADERS       = mainwindow.h \
                ocamlsourcehighlighter.h \
                ocamldebughighlighter.h \
                ocamldebug.h \
                ocamlwatch.h \
                highlighter.h \
                filesystemwatcher.h \
                options.h \
                ocamlsource.h
SOURCES       = main.cpp \
                ocamlsourcehighlighter.cpp \
                ocamldebughighlighter.cpp \
                filesystemwatcher.cpp \
                ocamlwatch.cpp \
                ocamldebug.cpp \
                mainwindow.cpp \
                options.cpp \
                ocamlsource.cpp
RESOURCES     = oqamldebug.qrc
FORMS         =

# install
DISTFILES += $$RESOURCES readme.html 
DISTFILES += images/copy.png images/cut.png images/debug-backstep.png images/debug-down.png \
                 images/debug-finish.png images/debugger.png images/debug-interrupt.png \
                 images/debug-next.png images/debug-previous.png images/debug-reverse.png \
                 images/debug-run.png images/debug-step.png images/debug-up.png \
                 images/open.png images/paste.png

unix {
    target.path = $$(PREFIX)/bin
    INSTALLS += target
}
