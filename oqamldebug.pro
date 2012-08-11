CONFIG -= app_bundle

HEADERS       = mainwindow.h \
                ocamlsourcehighlighter.h \
                ocamldebughighlighter.h \
                ocamldebug.h \
                highlighter.h \
                filesystemwatcher.h \
                options.h \
                ocamlsource.h
SOURCES       = main.cpp \
                ocamlsourcehighlighter.cpp \
                ocamldebughighlighter.cpp \
                filesystemwatcher.cpp \
                ocamldebug.cpp \
                mainwindow.cpp \
                options.cpp \
                ocamlsource.cpp
RESOURCES     = oqamldebug.qrc

# install
target.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/oqamldebug
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS oqamldebug.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/oqamldebug
INSTALLS += target sources

