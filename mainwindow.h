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
class QTreeView;
class QFileSystemModel;
class QTextBrowser;
class QPlainTextEdit;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
class QModelIndex;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(const Arguments &);
    QMdiSubWindow* openOCamlSource(const QString &fileName, bool from_user_loaded);

protected:
    void closeEvent(QCloseEvent *event);

private slots:
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
    void setWorkingDirectory();
    void setOCamlDebugArgs();
    void setOCamlDebugInitScript();
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
    void fileBrowserItemActivated( const QModelIndex &item ) ;
    void fileBrowserPathChanged( const QString &path );

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
    QTreeView *filebrowser ;
    QFileSystemModel *filebrowser_model_p ;
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
    QAction *createWatchWindowAct;
    QAction *setOcamlDebugAct;
    QAction *setWorkingDirectoryAct;
    QAction *setOcamlDebugArgsAct;
    QAction *setOcamlDebugInitScriptAct;
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
    Arguments _arguments;
    QString _ocamldebug;
    QString _ocamldebug_init_script;

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
    QDockWidget *filebrowser_dock ;

    QList<int> _watch_ids;
    QList<OCamlWatch*> _watch_windows;
};

#endif
