#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ocamldebug.h"

class OCamlSource;
class OCamlRun;
class OCamlStack;
class OCamlBreakpoint;
class OCamlDebug;
class OCamlWatch;
QT_BEGIN_NAMESPACE
class QAction;
class QTextBrowser;
class QPlainTextEdit;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const QStringList &);
    QMdiSubWindow* openOCamlSource(const QString &fileName, bool from_user_loaded);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void open();
    void copy();
    void debuggerStart(bool);
    void debuggerStarted(bool b);
    void breakPointList(const BreakPoints &b);
    void debugRun();
    void debugUp();
    void debugDown();
    void debugInterrupt();
    void debugFinish();
    void debugReverse();
    void debugStep();
    void debugBackStep();
    void debugNext();
    void debugPrevious();
    void setOCamlDebug();
    void setOCamlRun();
    void setWorkingDirectory();
    void setOCamlDebugArgs();
    void about();
    void help();
    void updateMenus();
    void updateWindowMenu();
    OCamlSource *createMdiChild();
    void setActiveSubWindow(QWidget *window);
    void stopDebugging( const QString &, int , int , bool) ;
    void ocamlDebugFocus();
    void createWatchWindow();
    void watchWindowDestroyed( QObject* );
    void displayVariable( const QString & );
    void printVariable( const QString & );
    void watchVariable( const QString & );

private:
    void createWatchWindow( int watch_id );
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    void createDockWindows();
    OCamlSource *activeMdiChild();
    OCamlDebug *ocamldebug ;
    OCamlBreakpoint *ocamlbreakpoints ;
    OCamlStack *ocamlstack ;
    OCamlRun *ocamlrun ;
    QMdiSubWindow *findMdiChild(const QString &fileName);
    QMdiSubWindow *findMdiChildNotLoadedFromUser();

    QMdiArea *mdiArea;
    QSignalMapper *windowMapper;

    QMenu *mainMenu;
    QMenu *debugMenu;
    QMenu *editMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;
    QToolBar *mainToolBar;
    QToolBar *editToolBar;
    QToolBar *debugToolBar;
    QToolBar *debugWindowToolBar;
    QAction *openAct;
    QAction *createWatchWindowAct;
    QAction *setOcamlDebugAct;
    QAction *setOcamlRunAct;
    QAction *setWorkingDirectoryAct;
    QAction *setOcamlDebugArgsAct;
    QAction *exitAct;
    QAction *copyAct;
    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *separatorAct;
    QAction *aboutAct;
    QAction *helpAct;
    QAction *aboutQtAct;
    QStringList _arguments;
    QString _ocamldebug;
    QString _ocamlrun;

    QAction *debuggerStartAct;
    QAction *debugUpAct;
    QAction *debugDownAct;
    QAction *debugInterruptAct;
    QAction *debugRunAct;
    QAction *debugReverseAct;
    QAction *debugStepAct;
    QAction *debugBackStepAct;
    QAction *debugPreviousAct;
    QAction *debugNextAct;
    QAction *debugFinishAct;

    QTextBrowser *help_p;
    QDockWidget *ocamldebug_dock ;
    QDockWidget *ocamlbreakpoints_dock ;
    QDockWidget *ocamlstack_dock ;
    QDockWidget *ocamlrun_dock ;

    QList<int> _watch_ids;
    QList<OCamlWatch*> _watch_windows;
};

#endif
