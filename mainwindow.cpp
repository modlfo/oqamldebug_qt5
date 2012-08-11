#include "mainwindow.h"
#include <QtGui>
#include "options.h"
#include "ocamlsource.h"
#include "ocamldebug.h"
#include <QFileInfo>

MainWindow::MainWindow(const QStringList &arguments)
{
    ocamldebug = NULL;
    help_p = NULL;
    _arguments = arguments;
    if (_arguments.isEmpty())
    {
        _arguments = Options::get_opt_strlst("ARGUMENTS");
        QDir::setCurrent( Options::get_opt_str("WORKING_DIRECTORY") );
    }
    else
    {
        Options::set_opt("WORKING_DIRECTORY",QDir::currentPath() ) ;
        Options::set_opt("ARGUMENTS",_arguments);
    }

    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    mdiArea->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    setCentralWidget( mdiArea );
    connect( mdiArea, SIGNAL( subWindowActivated( QMdiSubWindow* ) ),
             this, SLOT( updateMenus() ) );
    windowMapper = new QSignalMapper( this );
    connect( windowMapper, SIGNAL( mapped( QWidget* ) ),
             this, SLOT( setActiveSubWindow( QWidget* ) ) );

    _ocamldebug = Options::get_opt_str("OCAMLDEBUG" , QString() );
    while (_ocamldebug.isEmpty())
    {
        if (QMessageBox::warning(this, tr("ocamldebug location"),
                tr("It is the first time that you start OQamlDebug, please enter the path to ocamldebug executable."),
                QMessageBox::Ok, QMessageBox::Abort )
            != QMessageBox::Ok)
           exit(0);
        setOCamlDebug();
    }
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateMenus();
    createDockWindows();
    readSettings();

    setWindowTitle( tr( "OQamlDebug" ) );
    setUnifiedTitleAndToolBarOnMac( true );
    ocamldebug->setFocus();
}

void MainWindow::createDockWindows()
{
    QDockWidget *dock = new QDockWidget( tr( "OCamlDebug" ), this );
    dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    ocamldebug = new OCamlDebug( this , _ocamldebug, _arguments);
    dock->setObjectName("OCamlDebugDock");
    connect ( ocamldebug , SIGNAL( stopDebugging( const QString &, int , int , bool) ) , this ,SLOT( stopDebugging( const QString &, int , int , bool) ) );
    connect ( ocamldebug , SIGNAL( debuggerStarted( bool) ) , this ,SLOT( debuggerStarted( bool) ) );
    dock->setWidget( ocamldebug );
    addDockWidget( Qt::BottomDockWidgetArea, dock );
    mainMenu->addAction( dock->toggleViewAction() );

    ocamldebug->startDebug();
}

void MainWindow::closeEvent( QCloseEvent *event )
{
    writeSettings();
    mdiArea->closeAllSubWindows();
    if ( mdiArea->currentSubWindow() )
    {
        event->ignore();
    }
    else
    {
        event->accept();
    }
}

void MainWindow::open()
{
    QStringList fileNames = QFileDialog::getOpenFileNames( this,
            tr ("OCaml Sources" ) ,
            Options::get_opt_str("SOURCE_ML_DIR"),
            tr ("OCaml Source (*.ml)")
            );

    bool first = true;
    for (QStringList::const_iterator itSrc = fileNames.begin() ; itSrc != fileNames.end() ; ++itSrc )
    {
        openOCamlSource( *itSrc , true );
        if (first)
        {
            QFileInfo info (*itSrc);
            QString dir = info.absoluteDir().path();
            Options::set_opt("SOURCE_ML_DIR", dir );
            first =  false;
        }
    }
    if (ocamldebug)
        ocamldebug->setFocus();
}

QMdiSubWindow* MainWindow::openOCamlSource(const QString &fileName, bool from_user_loaded)
{
    if ( !fileName.isEmpty() )
    {
        QMdiSubWindow *existing = findMdiChild( fileName );
        if ( existing )
        {
            mdiArea->setActiveSubWindow( existing );
            OCamlSource *child = qobject_cast<OCamlSource *>( existing );
            if (child)
                child->setFromUserLoaded( child->fromUserLoaded() || from_user_loaded );
            return NULL;
        }

        if ( ! from_user_loaded )
        {
            existing = findMdiChildNotLoadedFromUser( );
            if ( existing )
            {
                mdiArea->setActiveSubWindow( existing );
                OCamlSource *child = qobject_cast<OCamlSource *>( existing );
                if (child)
                {
                    child->setFromUserLoaded( false );
                    if ( child->loadFile( fileName ) )
                    {
                        statusBar()->showMessage( tr( "File loaded" ), 2000 );
                        child->show();
                    }
                    else
                    {
                        child->close();
                    }
                }
                return NULL;
            }
        }

        OCamlSource *child = createMdiChild();
        child->setFromUserLoaded( from_user_loaded );
        if ( child->loadFile( fileName ) )
        {
            statusBar()->showMessage( tr( "File loaded" ), 2000 );
            child->show();
        }
        else
        {
            child->close();
        }
        return findMdiChild( fileName );
    }
    return NULL;
}

void MainWindow::setOCamlDebugArgs()
{
    bool ok;
    QStringList arguments = _arguments;
    if (arguments.isEmpty())
        arguments = Options::get_opt_strlst("ARGUMENTS");
    QString text = QInputDialog::getText(this, tr("OCamlDebug Command Line Arguments"),
            tr("Arguments:"), QLineEdit::Normal,
            arguments.join(" "), &ok);
    if (ok && !text.isEmpty())
    {
        _arguments = text.split(" ", QString::SkipEmptyParts);
        Options::set_opt("ARGUMENTS",_arguments);
        if (ocamldebug)
            ocamldebug->setArguments( _arguments );
    }
}

void MainWindow::setWorkingDirectory()
{
    QString working_directory = QFileDialog::getExistingDirectory(this, 
            tr("Working Directory"),
            Options::get_opt_str("WORKING_DIRECTORY") 
            );
    if (!working_directory.isEmpty())
    {
        Options::set_opt("WORKING_DIRECTORY", working_directory);
        QDir::setCurrent( Options::get_opt_str("WORKING_DIRECTORY") );
    }
}

void MainWindow::setOCamlDebug()
{
    QFileInfo ocamldebug_exx_info ( Options::get_opt_str("OCAMLDEBUG") );
    QString ocamldebug_exx = QFileDialog::getOpenFileName(this, 
            tr("OCamlDebug Executable"),
            ocamldebug_exx_info.absolutePath(),
#if defined(Q_OS_UNIX)
            "OCamlDebug (ocamldebug)"
#else
            "OCamlDebug (ocamldebug.exe)"
#endif
#if defined(Q_OS_MAC)
            ,NULL,
            QFileDialog::DontUseNativeDialog
#endif
            );
    if (!ocamldebug_exx.isEmpty())
    {
        _ocamldebug=ocamldebug_exx;
        Options::set_opt("OCAMLDEBUG", _ocamldebug);
        if (ocamldebug)
            ocamldebug->setOCamlDebug( _ocamldebug );
    }
}

void MainWindow::paste()
{
    if ( activeMdiChild() )
        activeMdiChild()->paste();
}

void MainWindow::cut()
{
    if ( activeMdiChild() )
        activeMdiChild()->cut();
}

void MainWindow::copy()
{
    if ( activeMdiChild() )
        activeMdiChild()->copy();
}

void MainWindow::help()
{
    if ( help_p == NULL )
    {
        QFile h ( ":/readme.html" );
        if ( h.open(QFile::ReadOnly) )
        {
            help_p = new QTextBrowser();
            help_p->setReadOnly(true);
            help_p->setWindowTitle(tr("Help"));
            help_p->setHtml( h.readAll() );
        }
    }
    help_p->show();
}

void MainWindow::about()
{
    QMessageBox::about( this, tr( "About OQamlDebug" ),
                        tr( "<b>OQamlDebug</b> graphical frontend for OCamlDebug<BR>"
                            "License: GPLv3 (C) Sebastien Fricker"
                            ));
                            
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = ( activeMdiChild() != 0 );
    closeAct->setEnabled( hasMdiChild );
    closeAllAct->setEnabled( hasMdiChild );
    tileAct->setEnabled( hasMdiChild );
    cascadeAct->setEnabled( hasMdiChild );
    nextAct->setEnabled( hasMdiChild );
    previousAct->setEnabled( hasMdiChild );
    separatorAct->setVisible( hasMdiChild );

    bool hasSelection = ( activeMdiChild() &&
                          activeMdiChild()->textCursor().hasSelection() );
    copyAct->setEnabled( hasSelection );
    cutAct->setEnabled( hasSelection );
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction( closeAct );
    windowMenu->addAction( closeAllAct );
    windowMenu->addSeparator();
    windowMenu->addAction( tileAct );
    windowMenu->addAction( cascadeAct );
    windowMenu->addSeparator();
    windowMenu->addAction( nextAct );
    windowMenu->addAction( previousAct );
    windowMenu->addAction( separatorAct );

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    separatorAct->setVisible( !windows.isEmpty() );

    for ( int i = 0; i < windows.size(); ++i )
    {
        QWidget *widget_p = windows.at( i )->widget();
        OCamlSource *child = qobject_cast<OCamlSource *>( widget_p );

        QString text;
        if (child )
        {
            if ( i < 9 )
            {
                text = tr( "&%1 %2" ).arg( i + 1 )
                    .arg( child->userFriendlyCurrentFile() );
            }
            else
            {
                text = tr( "%1 %2" ).arg( i + 1 )
                    .arg( child->userFriendlyCurrentFile() );
            }

            QAction *action  = windowMenu->addAction( text );
            action->setCheckable( true );
            action ->setChecked( widget_p == activeMdiChild() );
            connect( action, SIGNAL( triggered() ), windowMapper, SLOT( map() ) );
            windowMapper->setMapping( action, windows.at( i ) );
        }
    }
}

OCamlSource *MainWindow::createMdiChild()
{
    OCamlSource *child = new OCamlSource;
    mdiArea->addSubWindow( child );

    connect( child, SIGNAL( copyAvailable( bool ) ),
             cutAct, SLOT( setEnabled( bool ) ) );

    connect( child, SIGNAL( copyAvailable( bool ) ),
             copyAct, SLOT( setEnabled( bool ) ) );

    connect( child, SIGNAL( debugger( const QString & ) ),
             ocamldebug, SLOT( debugger( const QString & ) ) );

    return child;
}

void MainWindow::createActions()
{
    setWorkingDirectoryAct = new QAction(  tr( "&Working directory..." ), this );
    setWorkingDirectoryAct->setStatusTip( tr( "Set current working directory" ) );
    connect( setWorkingDirectoryAct, SIGNAL( triggered() ), this, SLOT( setWorkingDirectory() ) );

    setOcamlDebugAct = new QAction(  tr( "&Set OCamlDebug Executable..." ), this );
    setOcamlDebugAct->setStatusTip( tr( "Set OCamlDebug executable" ) );
    connect( setOcamlDebugAct, SIGNAL( triggered() ), this, SLOT( setOCamlDebug() ) );

    setOcamlDebugArgsAct = new QAction(  tr( "&Command Line Arguments..." ), this );
    setOcamlDebugArgsAct->setStatusTip( tr( "Set OCamlDebug command line arguments" ) );
    connect( setOcamlDebugArgsAct, SIGNAL( triggered() ), this, SLOT( setOCamlDebugArgs() ) );

    openAct = new QAction( QIcon( ":/images/open.png" ), tr( "&Open OCaml Source..." ), this );
    openAct->setShortcuts( QKeySequence::Open );
    openAct->setStatusTip( tr( "Open an existing OCaml source file" ) );
    connect( openAct, SIGNAL( triggered() ), this, SLOT( open() ) );

    exitAct = new QAction( tr( "E&xit" ), this );
    exitAct->setShortcuts( QKeySequence::Quit );
    exitAct->setStatusTip( tr( "Exit the application" ) );
    connect( exitAct, SIGNAL( triggered() ), qApp, SLOT( closeAllWindows() ) );

    copyAct = new QAction( QIcon( ":/images/copy.png" ), tr( "&Copy" ), this );
    copyAct->setShortcuts( QKeySequence::Copy );
    copyAct->setStatusTip( tr( "Copy the current selection's contents to the "
                               "clipboard" ) );
    connect( copyAct, SIGNAL( triggered() ), this, SLOT( copy() ) );

    cutAct = new QAction( QIcon( ":/images/cut.png" ), tr( "&Cut" ), this );
    cutAct->setShortcuts( QKeySequence::Cut );
    cutAct->setStatusTip( tr( "Cut the current selection's contents to the "
                               "clipboard" ) );
    connect( cutAct, SIGNAL( triggered() ), this, SLOT( cut() ) );


    pasteAct = new QAction( QIcon( ":/images/paste.png" ), tr( "&Paste" ), this );
    pasteAct->setShortcuts( QKeySequence::Paste );
    pasteAct->setStatusTip( tr( "Paste contents of the clipboard" ) );
    connect( pasteAct, SIGNAL( triggered() ), this, SLOT( paste() ) );

    closeAct = new QAction( tr( "Cl&ose" ), this );
    closeAct->setStatusTip( tr( "Close the active window" ) );
    connect( closeAct, SIGNAL( triggered() ),
             mdiArea, SLOT( closeActiveSubWindow() ) );

    closeAllAct = new QAction( tr( "Close &All" ), this );
    closeAllAct->setStatusTip( tr( "Close all the windows" ) );
    connect( closeAllAct, SIGNAL( triggered() ),
             mdiArea, SLOT( closeAllSubWindows() ) );

    tileAct = new QAction( tr( "&Tile" ), this );
    tileAct->setStatusTip( tr( "Tile the windows" ) );
    connect( tileAct, SIGNAL( triggered() ), mdiArea, SLOT( tileSubWindows() ) );

    cascadeAct = new QAction( tr( "&Cascade" ), this );
    cascadeAct->setStatusTip( tr( "Cascade the windows" ) );
    connect( cascadeAct, SIGNAL( triggered() ), mdiArea, SLOT( cascadeSubWindows() ) );

    nextAct = new QAction( tr( "Ne&xt" ), this );
    nextAct->setShortcuts( QKeySequence::NextChild );
    nextAct->setStatusTip( tr( "Move the focus to the next window" ) );
    connect( nextAct, SIGNAL( triggered() ),
             mdiArea, SLOT( activateNextSubWindow() ) );

    previousAct = new QAction( tr( "Pre&vious" ), this );
    previousAct->setShortcuts( QKeySequence::PreviousChild );
    previousAct->setStatusTip( tr( "Move the focus to the previous "
                                   "window" ) );
    connect( previousAct, SIGNAL( triggered() ),
             mdiArea, SLOT( activatePreviousSubWindow() ) );

    separatorAct = new QAction( this );
    separatorAct->setSeparator( true );

    helpAct = new QAction( tr( "&Help" ), this );
    connect( helpAct, SIGNAL( triggered() ), this, SLOT( help() ) );

    aboutAct = new QAction( tr( "&About" ), this );
    aboutAct->setStatusTip( tr( "Show the application's About box" ) );
    connect( aboutAct, SIGNAL( triggered() ), this, SLOT( about() ) );

    aboutQtAct = new QAction( tr( "About &Qt" ), this );
    aboutQtAct->setStatusTip( tr( "Show the Qt library's About box" ) );
    connect( aboutQtAct, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

    debuggerStartAct = new QAction( QIcon( ":/images/debugger.png" ), tr( "&Start" ), this );
    debuggerStartAct->setShortcut( QKeySequence("Ctrl+S") );
    debuggerStartAct->setCheckable( true );
    debuggerStartAct->setStatusTip( tr( "Start/Stop the ocamldebug process" ) );
    connect( debuggerStartAct, SIGNAL( triggered(bool) ), this, SLOT( debuggerStart(bool) ) );

    debugReverseAct = new QAction( QIcon( ":/images/debug-reverse.png" ), tr( "&Reverse" ), this );
    debugReverseAct->setShortcut( QKeySequence("Ctrl+E") );
    debugReverseAct->setStatusTip( tr( "Reverse execution of the application" ) );
    connect( debugReverseAct, SIGNAL( triggered() ), this, SLOT( debugReverse() ) );

    debugRunAct = new QAction( QIcon( ":/images/debug-run.png" ), tr( "&Run" ), this );
    debugRunAct->setShortcut( QKeySequence("Ctrl+R") );
    debugRunAct->setStatusTip( tr( "Execution of the application" ) );
    connect( debugRunAct, SIGNAL( triggered() ), this, SLOT( debugRun() ) );

    debugNextAct = new QAction( QIcon( ":/images/debug-next.png" ), tr( "&Next" ), this );
    debugNextAct->setShortcut( QKeySequence("Ctrl+N") );
    debugNextAct->setStatusTip( tr( "Step over" ) );
    connect( debugNextAct, SIGNAL( triggered() ), this, SLOT( debugNext() ) );

    debugPreviousAct = new QAction( QIcon( ":/images/debug-previous.png" ), tr( "&Previous" ), this );
    debugPreviousAct->setShortcut( QKeySequence("Ctrl+P") );
    debugPreviousAct->setStatusTip( tr( "Back step over" ) );
    connect( debugPreviousAct, SIGNAL( triggered() ), this, SLOT( debugPrevious() ) );

    debugFinishAct = new QAction( QIcon( ":/images/debug-finish.png" ), tr( "&Finish" ), this );
    debugFinishAct->setShortcut( QKeySequence("Ctrl+F") );
    debugFinishAct->setStatusTip( tr( "Run until function exit" ) );
    connect( debugFinishAct, SIGNAL( triggered() ), this, SLOT( debugFinish() ) );

    debugBackStepAct = new QAction( QIcon( ":/images/debug-backstep.png" ), tr( "&Back Step" ), this );
    debugBackStepAct->setShortcut( QKeySequence("Ctrl+B") );
    debugBackStepAct->setStatusTip( tr( "Step into" ) );
    connect( debugBackStepAct, SIGNAL( triggered() ), this, SLOT( debugBackStep() ) );

    debugStepAct = new QAction( QIcon( ":/images/debug-step.png" ), tr( "&Step" ), this );
    debugStepAct->setShortcut( QKeySequence("Ctrl+S") );
    debugStepAct->setStatusTip( tr( "Back step into" ) );
    connect( debugStepAct, SIGNAL( triggered() ), this, SLOT( debugStep() ) );

    debugInterruptAct = new QAction( QIcon( ":/images/debug-interrupt.png" ), tr( "&Interrupt" ), this );
    debugInterruptAct->setShortcut( QKeySequence("Ctrl+C") );
    debugInterruptAct->setStatusTip( tr( "Interrupt the execution of the application" ) );
    connect( debugInterruptAct, SIGNAL( triggered() ), this, SLOT( debugInterrupt() ) );


    debugDownAct = new QAction( QIcon( ":/images/debug-down.png" ), tr( "&Down" ), this );
    debugDownAct->setShortcut(  QKeySequence("Ctrl+D") );
    debugDownAct->setStatusTip( tr( "Call stack down" ) );
    connect( debugDownAct, SIGNAL( triggered() ), this, SLOT( debugDown() ) );


    debugUpAct = new QAction( QIcon( ":/images/debug-up.png" ), tr( "&Up" ), this );
    debugUpAct->setShortcut( QKeySequence("Ctrl+U") );
    debugUpAct->setStatusTip( tr( "Call stack up" ) );
    connect( debugUpAct, SIGNAL( triggered() ), this, SLOT( debugUp() ) );

}

void MainWindow::createMenus()
{
    mainMenu = menuBar()->addMenu( tr( "&Main" ) );
    mainMenu->addAction( setOcamlDebugAct );
    mainMenu->addAction( setOcamlDebugArgsAct );
    mainMenu->addAction( setWorkingDirectoryAct );
    mainMenu->addAction( openAct );
    mainMenu->addSeparator();
    mainMenu->addAction( exitAct );

    debugMenu = menuBar()->addMenu( tr( "&Debug" ) );
    debugMenu->addAction( debuggerStartAct );
    debugMenu->addAction( debugReverseAct );
    debugMenu->addAction( debugPreviousAct );
    debugMenu->addAction( debugBackStepAct );
    debugMenu->addAction( debugStepAct );
    debugMenu->addAction( debugNextAct );
    debugMenu->addAction( debugFinishAct );
    debugMenu->addAction( debugRunAct );
    debugMenu->addAction( debugInterruptAct );
    debugMenu->addAction( debugDownAct );
    debugMenu->addAction( debugUpAct );

    editMenu = menuBar()->addMenu( tr( "&Edit" ) );
    editMenu->addAction( copyAct );
    editMenu->addAction( cutAct );
    editMenu->addAction( pasteAct );

    windowMenu = menuBar()->addMenu( tr( "&Window" ) );
    updateWindowMenu();
    connect( windowMenu, SIGNAL( aboutToShow() ), this, SLOT( updateWindowMenu() ) );

    menuBar()->addSeparator();

    helpMenu = menuBar()->addMenu( tr( "&Help" ) );
    helpMenu->addAction( helpAct );
    helpMenu->addAction( aboutAct );
    helpMenu->addAction( aboutQtAct );
}

void MainWindow::createToolBars()
{
    mainToolBar = addToolBar( tr( "Main" ) );
    mainToolBar->setObjectName("MainToolBar");
    mainToolBar->addAction( openAct );

    editToolBar = addToolBar( tr( "Edit" ) );
    editToolBar->setObjectName("EditToolBar");
    editToolBar->addAction( copyAct );
    editToolBar->addAction( cutAct );
    editToolBar->addAction( pasteAct );

    debugToolBar = addToolBar( tr( "Debug" ) );
    debugToolBar->setObjectName("DebugToolBar");
    debugToolBar->addAction( debuggerStartAct );
    debugToolBar->addAction( debugReverseAct );
    debugToolBar->addAction( debugPreviousAct );
    debugToolBar->addAction( debugBackStepAct );
    debugToolBar->addAction( debugStepAct );
    debugToolBar->addAction( debugNextAct );
    debugToolBar->addAction( debugFinishAct );
    debugToolBar->addAction( debugRunAct );
    debugToolBar->addAction( debugInterruptAct );
    debugToolBar->addAction( debugDownAct );
    debugToolBar->addAction( debugUpAct );
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage( tr( "Ready" ) );
}

void MainWindow::readSettings()
{
    Options::restore_window_position("mainwindow",this);
    Options::restore_window_position(this);
}

void MainWindow::writeSettings()
{
    Options::save_window_position("mainwindow",this);
    Options::save_window_position(mdiArea);
}

OCamlSource *MainWindow::activeMdiChild()
{
    if ( QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow() )
        return qobject_cast<OCamlSource *>( activeSubWindow->widget() );
    return 0;
}

QMdiSubWindow *MainWindow::findMdiChildNotLoadedFromUser()
{
    foreach ( QMdiSubWindow * window, mdiArea->subWindowList() )
    {
        OCamlSource *mdiChild = qobject_cast<OCamlSource *>( window->widget() );
        if (mdiChild)
        {
            if ( ! mdiChild->fromUserLoaded() )
                return window;
        }
    }
    return 0;
}

QMdiSubWindow *MainWindow::findMdiChild( const QString &fileName )
{
    QString canonicalFilePath = QFileInfo( fileName ).canonicalFilePath();

    foreach ( QMdiSubWindow * window, mdiArea->subWindowList() )
    {
        OCamlSource *mdiChild = qobject_cast<OCamlSource *>( window->widget() );
        if (mdiChild)
        {
            if ( mdiChild->currentFile() == canonicalFilePath )
                return window;
        }
    }
    return 0;
}

void MainWindow::setActiveSubWindow( QWidget *window )
{
    if ( !window )
        return;
    mdiArea->setActiveSubWindow( qobject_cast<QMdiSubWindow *>( window ) );
}

void MainWindow::stopDebugging( const QString &file, int start_char, int end_char, bool after) 
{
    statusBar()->showMessage( QString() );
    if ( ! file.isEmpty() )
    {
        openOCamlSource( file , false );
        OCamlSource *current_source_p = activeMdiChild();
        QString text = current_source_p->stopDebugging( file, start_char, end_char, after) ;
        if ( ! text.isEmpty() )
        {
            if (after)
                statusBar()->showMessage( tr( "Stopped after '%1'" ).arg(text) );
            else
                statusBar()->showMessage( tr( "Stopped before '%1'" ).arg(text) );
        }

        foreach ( QMdiSubWindow * window, mdiArea->subWindowList() )
        {
            OCamlSource *mdiChild = qobject_cast<OCamlSource *>( window->widget() );
            if (mdiChild)
            {
                if (mdiChild != current_source_p)
                    mdiChild->stopDebugging( QString(), 0, 0, after) ;
            }
        }
        if (ocamldebug)
            ocamldebug->setFocus();
    }
}

void MainWindow::debugUp()
{
    if (ocamldebug)
        ocamldebug->debugger("up");
}

void MainWindow::debugDown()
{
    if (ocamldebug)
        ocamldebug->debugger("down");
}

void MainWindow::debugInterrupt()
{
    if (ocamldebug)
        ocamldebug->debuggerInterrupt();
}

void MainWindow::debugRun()
{
    if (ocamldebug)
        ocamldebug->debugger("run");
}

void MainWindow::debugFinish()
{
    if (ocamldebug)
        ocamldebug->debugger("finish");
}

void MainWindow::debugReverse()
{
    if (ocamldebug)
        ocamldebug->debugger("reverse");
}

void MainWindow::debugStep()
{
    if (ocamldebug)
        ocamldebug->debugger("step");
}

void MainWindow::debugBackStep()
{
    if (ocamldebug)
        ocamldebug->debugger("backstep");
}

void MainWindow::debugNext()
{
    if (ocamldebug)
        ocamldebug->debugger("next");
}

void MainWindow::debugPrevious()
{
    if (ocamldebug)
        ocamldebug->debugger("previous");
}

void MainWindow::debuggerStarted(bool b)
{
    debuggerStartAct->setChecked(b) ;
}

void MainWindow::debuggerStart(bool b)
{
    if (ocamldebug)
    {
        if (b)
            ocamldebug->startDebug();
        else
            ocamldebug->stopDebug();
    }
}

