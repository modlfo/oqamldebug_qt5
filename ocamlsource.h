#ifndef OCAMLSOURCE_H
#define OCAMLSOURCE_H

#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTimer>
#include "ocamldebug.h"
#include "ocamlsourcehighlighter.h"
#include "filesystemwatcher.h"
class OCamlSourceLineNumberArea ;
class OCamlSourceSearch ;

class OCamlSource : public QPlainTextEdit
{
    Q_OBJECT

    public:
        OCamlSource();
        virtual ~OCamlSource();

        bool loadFile(const QString &fileName);
        QString userFriendlyCurrentFile();
        QString currentFile() { return curFile; }
        QString stopDebugging( const QString &file, int start_char, int end_char, bool after) ;
        void lineNumberAreaPaintEvent(QPaintEvent *event);
        int lineNumberAreaWidth();
        bool fromUserLoaded() const { return _from_user_loaded ; }
        void setFromUserLoaded( bool v ) { _from_user_loaded = v ;}
        void breakPointList( const BreakPoints &b );

    private slots:
        void searchTextChanged( const QString & ) ;
        void nextTextSearch() ;
    signals:
        void debugger( const QString &, bool show_command );
        void displayVariable( const QString & );
        void printVariable( const QString & );
        void releaseFocus();
    protected:
        virtual void keyPressEvent ( QKeyEvent * e );
        void closeEvent(QCloseEvent *event);
        void contextMenuEvent(QContextMenuEvent *event);
        void mousePressEvent ( QMouseEvent * e );

        private slots:
            void newBreakpoint ( );
            void printVar ( );
            void displayVar ( );
        void markCurrentLocation();
        void markBreakPoints(bool);
        void fileChanged ( );
        void resizeEvent(QResizeEvent *event);

        private slots:
            void updateLineNumberAreaWidth(int newBlockCount);
        void updateLineNumberArea(const QRect &, int);

    private:
        void resizeLineSearch();
        void setCurrentFile(const QString &fileName);
        QString strippedName(const QString &fullFileName);

        QString curFile;
        OCamlSourceHighlighter *highlighter;
        bool _after;
        int _start_char ;
        int _end_char ;
        int _breapoint_position;
        QString _selected_text;
        QTimer *markCurrentLocationTimer;
        int timer_index;
        FileSystemWatcher *file_watch_p;
        OCamlSourceLineNumberArea *lineNumberArea;
        OCamlSourceSearch *lineSearchArea;
        bool _from_user_loaded;
        BreakPoints _breakpoints;
};


class OCamlSourceLineNumberArea : public QWidget
{
    public:
        OCamlSourceLineNumberArea( OCamlSource *editor ) ;
        QSize sizeHint() const;

    protected:
        void paintEvent( QPaintEvent *event );

    private:
        OCamlSource *codeEditor;
};

class OCamlSourceSearch : public QLineEdit
{
    public:
        OCamlSourceSearch( OCamlSource *editor ) ;

    private:
        OCamlSource *codeEditor;
};
#endif
