#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class OCamlSource;
class OCamlDebug;
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
    void cut();
    void paste();
    void debuggerStart(bool);
    void debuggerStarted(bool b);
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
    void about();
    void help();
    void updateMenus();
    void updateWindowMenu();
    OCamlSource *createMdiChild();
    void setActiveSubWindow(QWidget *window);
    void stopDebugging( const QString &, int , int , bool) ;

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    void createDockWindows();
    OCamlSource *activeMdiChild();
    OCamlDebug *ocamldebug ;
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
    QAction *openAct;
    QAction *setOcamlDebugAct;
    QAction *setWorkingDirectoryAct;
    QAction *setOcamlDebugArgsAct;
    QAction *exitAct;
    QAction *copyAct;
    QAction *cutAct;
    QAction *pasteAct;
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
};

#endif
