#ifndef ARGUMENTS_H
#define ARGUMENTS_H
#include <QStringList>

class Arguments
{
    public:
        Arguments( const QStringList & args );
        Arguments( const QString & args );

        const QString & ocamlApp() const { return _ocamlapp ; }
        const QStringList & ocamlAppArguments() const { return _app_arguments ; }
        const QStringList & ocamlDebugArguments() const { return _ocamldebug_arguments ; }

        const bool isEmpty() const { return _arguments.isEmpty(); }
        const QStringList & all() const { return _arguments; }
        QString toString() const ;
    private:
        void calcArguments();
        QStringList _arguments;
        QStringList _app_arguments,_ocamldebug_arguments;
        QString _ocamlapp;
};

#endif
