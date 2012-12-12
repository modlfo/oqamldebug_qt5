#CONFIG -= app_bundle
QT += network
VERSION=0.9.0

HEADERS       = mainwindow.h \
                arguments.h \
                textdiff.h \
                ocamlsourcehighlighter.h \
                ocamldebughighlighter.h \
                ocamldebug.h \
                ocamlwatch.h \
                ocamlstack.h \
                breakpoint.h \
                ocamlrun.h \
                debuggercommand.h \
                ocamlbreakpoint.h \
                highlighter.h \
                filesystemwatcher.h \
                options.h \
                ocamlsource.h
SOURCES       = main.cpp \
                arguments.cpp \
                textdiff.cpp \
                ocamlrun.cpp \
                ocamlbreakpoint.cpp \
                ocamlstack.cpp \
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

DEFINES += VERSION=\\\"$$VERSION\\\"

ICON=images/oqamldebug.icns

# install
DISTFILES += $$RESOURCES readme.html 
DISTFILES += images/copy.png images/oqamldebug.png images/cut.png images/debug-backstep.png images/debug-down.png \
                 images/debug-finish.png images/debugger.png images/debug-interrupt.png \
                 images/debug-next.png images/debug-previous.png images/debug-reverse.png \
                 images/debug-run.png images/debug-step.png images/debug-up.png \
                 images/open.png images/delete.png images/paste.png

unix {
    target.path = $$(PREFIX)/bin
    INSTALLS += target

    tags.target = tags
    tags.commands = ctags  --line-directives=yes --languages=all --fields=iaS --extra=+q --langmap=yacc:.y,c++:.cpp.h,c:.c --yacc-kinds=+l --c-kinds=+dpefglmstuv --c++-kinds=+cpdefmstuv -f $$tags.target  $$HEADERS $$SOURCES
    tags.depends = $$HEADERS $$SOURCES
    QMAKE_EXTRA_TARGETS += tags
}
