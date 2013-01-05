#include "mainwindow.h"
#include <QtGui>
#include "options.h"
#include "arguments.h"
#include "ocamlsource.h"
#include "ocamlrun.h"
#include "ocamldebug.h"
#include "ocamlbreakpoint.h"
#include "ocamlstack.h"
#include "ocamlwatch.h"
#include <QFileInfo>

MainWindow::MainWindow(const Arguments &arguments) : _arguments( arguments )
{
    ocamldebug = NULL;
    ocamldebug_dock  = NULL;
    ocamlbreakpoints_dock  = NULL;
    ocamlbreakpoints  = NULL;
    ocamlstack_dock  = NULL;
    ocamlstack  = NULL;
    ocamlrun_dock  = NULL;
    filebrowser_dock  = NULL;
    ocamlrun  = NULL;
    filebrowser  = NULL;
    filebrowser_model_p  = NULL;

    setWindowIcon( QIcon( ":/images/oqamldebug.png" ) );
    setDockNestingEnabled( true );
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
        Options::set_opt( "ARGUMENTS", _arguments.all() );
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
    _ocamldebug_init_script = Options::get_opt_str("OCAMLDEBUG_INIT_SCRIPT" , QString() );
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
    ocamlDebugFocus();
}

void MainWindow::createDockWindows()
{
    Arguments args( _arguments );
    filebrowser_dock = new QDockWidget( tr( "Source File Browser" ), this );
    filebrowser_dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    filebrowser = new QTreeView( );
    filebrowser_model_p = new QFileSystemModel;
    filebrowser_model_p->setNameFilters( QStringList() << "*.ml" );
    filebrowser_model_p->setReadOnly( true );
    filebrowser_model_p->setFilter( QDir::AllDirs | QDir::AllEntries | QDir::NoDot );
    filebrowser_model_p->setNameFilterDisables( false  );
    filebrowser->setModel( filebrowser_model_p );
    filebrowser_dock->setObjectName("FileBrowser");
    filebrowser_dock->setWidget( filebrowser );
    filebrowser_dock->setTitleBarWidget( new QLabel(filebrowser_dock) );
    connect( filebrowser_model_p, SIGNAL( rootPathChanged ( const QString & ) ), this, SLOT( fileBrowserPathChanged( const QString & ) ) );
    QString dir = QDir::current().path();
    filebrowser_model_p->setRootPath( dir );
    filebrowser->setItemsExpandable( false );
    filebrowser->setRootIndex( filebrowser_model_p->index( dir ) );
    filebrowser->setSortingEnabled( true );
    filebrowser->setRootIsDecorated( false );
    addDockWidget( Qt::BottomDockWidgetArea, filebrowser_dock );
    filebrowser_dock->toggleViewAction()->setIcon( QIcon( ":/images/open.png" ) );
    mainMenu->addAction( filebrowser_dock->toggleViewAction() );
    mainToolBar->addAction( filebrowser_dock->toggleViewAction() );
    connect( filebrowser, SIGNAL( activated ( const QModelIndex & ) ), this, SLOT( fileBrowserItemActivated( const QModelIndex & ) ) );

    ocamlrun_dock = new QDockWidget( tr( "Application Output" ), this );
    ocamlrun_dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    ocamlrun = new OCamlRun( ocamlrun_dock, args );
    ocamlrun_dock->setObjectName("OCamlRun");
    ocamlrun_dock->setWidget( ocamlrun );
    addDockWidget( Qt::BottomDockWidgetArea, ocamlrun_dock );
    ocamlrun_dock->toggleViewAction()->setIcon( QIcon( ":/images/terminal.png" ) );
    windowMenu->addAction( ocamlrun_dock->toggleViewAction() );
    debugWindowToolBar->addAction( ocamlrun_dock->toggleViewAction() );

    ocamldebug_dock = new QDockWidget( tr( "OCamlDebug" ), this );
    ocamldebug_dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    ocamldebug = new OCamlDebug( ocamldebug_dock , ocamlrun, _ocamldebug, args, _ocamldebug_init_script );
    ocamldebug_dock->setObjectName("OCamlDebugDock");
    connect ( ocamldebug , SIGNAL( stopDebugging( const QString &, int , int , bool) ) , this ,SLOT( stopDebugging( const QString &, int , int , bool) ) );
    connect ( ocamldebug , SIGNAL( debuggerStarted( bool) ) , this ,SLOT( debuggerStarted( bool) ) );
    connect ( ocamldebug , SIGNAL( breakPointList( const BreakPoints &) ) , this ,SLOT( breakPointList( const BreakPoints &) ) );
    connect ( ocamldebug , SIGNAL( debuggerStarted( bool) ) , ocamlrun ,SLOT( debuggerStarted( bool) ) );
    ocamldebug_dock->setWidget( ocamldebug );
    ocamldebug_dock->toggleViewAction()->setIcon( QIcon( ":/images/oqamldebug.png" ) );
    addDockWidget( Qt::BottomDockWidgetArea, ocamldebug_dock );
    windowMenu->addAction( ocamldebug_dock->toggleViewAction() );
    debugWindowToolBar->addAction( ocamldebug_dock->toggleViewAction() );

    ocamldebug->startDebug();

    QList<int> watch_ids = Options::get_opt_intlst( "WATCH_IDS" );
    for (QList<int>::const_iterator itId = watch_ids.begin(); itId != watch_ids.end(); ++itId)
        createWatchWindow( *itId );

    ocamlbreakpoints_dock = new QDockWidget( tr( "Breakpoints" ), this );
    ocamlbreakpoints_dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    ocamlbreakpoints = new OCamlBreakpoint( ocamlbreakpoints_dock );
    ocamlbreakpoints_dock->setObjectName("Breakpoints");
    connect( ocamlbreakpoints, SIGNAL( debugger( const DebuggerCommand & ) ), ocamldebug, SLOT( debugger( const DebuggerCommand & ) ) );
    connect ( ocamldebug , SIGNAL( stopDebugging( const QString &, int , int , bool) ) , ocamlbreakpoints  ,SLOT( stopDebugging( const QString &, int , int , bool) ) );
    connect ( ocamldebug , SIGNAL( debuggerStarted( bool) ) , ocamlbreakpoints  ,SLOT( debuggerStarted( bool) ) );
    connect ( ocamldebug , SIGNAL( breakPointList( const BreakPoints &) ) , ocamlbreakpoints ,SLOT( breakPointList( const BreakPoints &) ) );
    connect ( ocamldebug , SIGNAL( breakPointHit( const QList<int> &) ) , ocamlbreakpoints ,SLOT( breakPointHit( const QList<int> &) ) );
    ocamlbreakpoints_dock->setWidget( ocamlbreakpoints );
    addDockWidget( Qt::BottomDockWidgetArea, ocamlbreakpoints_dock );
    ocamlbreakpoints_dock->toggleViewAction()->setIcon( QIcon( ":/images/breakpoints.png" ) );
    windowMenu->addAction( ocamlbreakpoints_dock->toggleViewAction() );
    debugWindowToolBar->addAction( ocamlbreakpoints_dock->toggleViewAction() );


    ocamlstack_dock = new QDockWidget( tr( "Stack" ), this );
    ocamlstack_dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    ocamlstack = new OCamlStack( ocamlstack_dock );
    ocamlstack_dock->setObjectName("Stack");
    connect( ocamlstack, SIGNAL( debugger( const DebuggerCommand & ) ), ocamldebug, SLOT( debugger( const DebuggerCommand & ) ) );
    connect ( ocamldebug , SIGNAL( stopDebugging( const QString &, int , int , bool) ) , ocamlstack  ,SLOT( stopDebugging( const QString &, int , int , bool) ) );
    connect ( ocamldebug , SIGNAL( debuggerStarted( bool) ) , ocamlstack ,SLOT( debuggerStarted( bool) ) );
    connect ( ocamldebug , SIGNAL( debuggerCommand( const QString &, const QString &) ) , ocamlstack ,SLOT( debuggerCommand( const QString &, const QString &) ) );
    ocamlstack_dock->setWidget( ocamlstack );
    addDockWidget( Qt::BottomDockWidgetArea, ocamlstack_dock );
    ocamlstack_dock->toggleViewAction()->setIcon( QIcon( ":/images/callstack.png" ) );
    windowMenu->addAction( ocamlstack_dock->toggleViewAction() );
    debugWindowToolBar->addAction( ocamlstack_dock->toggleViewAction() );
}

void MainWindow::createWatchWindow()
{
    int watch_id ;
    for ( watch_id=1 ; watch_id < 10; watch_id++ )
    {
        if ( !_watch_ids.contains( watch_id ) )
        {
            createWatchWindow( watch_id );
            return;
        }
    }
}

void MainWindow::createWatchWindow( int watch_id )
{
    _watch_ids << watch_id ;

    QDockWidget *dock = new QDockWidget( tr( "Watch %1" ).arg( QString::number(watch_id) ), this );
    dock->setAttribute( Qt::WA_DeleteOnClose );
    dock->setAllowedAreas( Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea | Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea );
    OCamlWatch *ocamlwatch = new OCamlWatch( dock, watch_id );
    connect ( ocamldebug , SIGNAL( stopDebugging( const QString &, int , int , bool) ) , ocamlwatch ,SLOT( stopDebugging( const QString &, int , int , bool) ) );
    connect ( ocamldebug , SIGNAL( debuggerCommand( const QString &, const QString &) ) , ocamlwatch ,SLOT( debuggerCommand( const QString &, const QString &) ) );
    connect( ocamlwatch, SIGNAL( debugger( const DebuggerCommand & ) ), ocamldebug, SLOT( debugger( const DebuggerCommand & ) ) );
    connect( ocamlwatch, SIGNAL( destroyed( QObject* ) ), this, SLOT( watchWindowDestroyed( QObject* ) ) );
    connect ( ocamldebug , SIGNAL( debuggerStarted( bool) ) , ocamlwatch ,SLOT( debuggerStarted( bool) ) );
    dock->setObjectName(QString("OCamlWatchDock%1").arg( QString::number(watch_id) ));
    dock->setWidget( ocamlwatch );
    addDockWidget( Qt::BottomDockWidgetArea, dock );

    _watch_windows.append( ocamlwatch );
    Options::set_opt( "WATCH_IDS", _watch_ids );
}

void MainWindow::watchWindowDestroyed( QObject *o )
{
    OCamlWatch *watch = static_cast<OCamlWatch*>( o );
    if ( watch )
    {
        if ( _watch_windows.contains( watch ) )
        {
            Options::set_opt( QString("OCamlWatch%0_VARIABLES").arg(watch->id), QStringList() );
            _watch_ids.removeAll( watch->id );
            _watch_windows.removeAll( watch );
            Options::set_opt( "WATCH_IDS", _watch_ids );
        }
    }
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
    Arguments arguments = _arguments;
    if (arguments.isEmpty())
        arguments = Arguments( Options::get_opt_strlst("ARGUMENTS") );
    QString text = QInputDialog::getText(this, tr("OCamlDebug Command Line Arguments"),
            tr("Arguments:"), QLineEdit::Normal,
            arguments.toString(), &ok);
    if (ok && !text.isEmpty())
    {
        _arguments = Arguments( text );
        Options::set_opt("ARGUMENTS",_arguments.all());
        if (ocamldebug)
        {
            Arguments args( _arguments );
            ocamldebug->setArguments( args );
            ocamldebug->setOCamlDebug( _ocamldebug );
            ocamldebug->setInitializationScript( _ocamldebug_init_script );
        }
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

void MainWindow::setOCamlDebugInitScript()
{
    QFileInfo ocamldebug_exx_info ( Options::get_opt_str("OCAMLDEBUG_INIT_SCRIPT") );
    _ocamldebug_init_script = QFileDialog::getOpenFileName(this, 
            tr("OCamlDebug Executable"),
            ocamldebug_exx_info.absolutePath(),
#if defined(Q_OS_UNIX)
            "OCamlDebug Initialization Script (*)"
#else
            "OCamlDebug Initialization Script (*.*)"
#endif
#if defined(Q_OS_MAC)
            ,NULL,
            QFileDialog::DontUseNativeDialog
#endif
            );
    Options::set_opt("OCAMLDEBUG_INIT_SCRIPT", _ocamldebug_init_script);
    if (ocamldebug)
        ocamldebug->setInitializationScript( _ocamldebug_init_script );
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
        {
            Arguments args( _arguments );
            ocamldebug->setOCamlDebug( _ocamldebug );
        }
    }
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
                        tr( "<center><b>OQamlDebug</b> graphical frontend for OCamlDebug</center><BR>"
                            "<TABLE border=\"0\"><TR>"
                            "<TH>Homepage:</TH>"  "<TD><a href=\"https://forge.ocamlcore.org/projects/oqamldebug/\">https://forge.ocamlcore.org/projects/oqamldebug/</a></TD>"
                            "</TR><TR>"
                            "<TH>Version:</TH>"  "<TD>%1</TD>"
                            "</TR><TR>"
                            "<TH>License:</TH>"   "<TD><a href=\"http://www.gnu.org/licenses/gpl-3.0.en.html\">GPLv3</a> &copy;&nbsp;<a href=\"mailto:sebastien.fricker@gmail.com\">S&eacute;bastien Fricker</a></TD>"
                            "</TR></TABLE>"
                            ).arg( VERSION ) );
                            
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

    if ( ocamldebug_dock )
    {
        windowMenu->addSeparator();
        windowMenu->addAction( ocamldebug_dock->toggleViewAction() );
    }
    if ( ocamlbreakpoints_dock )
        windowMenu->addAction( ocamlbreakpoints_dock->toggleViewAction() );
    if ( ocamlstack_dock )
        windowMenu->addAction( ocamlstack_dock->toggleViewAction() );
    if ( ocamlrun_dock )
        windowMenu->addAction( ocamlrun_dock->toggleViewAction() );

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
             copyAct, SLOT( setEnabled( bool ) ) );

    connect( child, SIGNAL( debugger( const DebuggerCommand & ) ),
             ocamldebug, SLOT( debugger( const DebuggerCommand & ) ) );

    connect( child, SIGNAL( releaseFocus() ),
             this, SLOT( ocamlDebugFocus() ) );

    connect( child, SIGNAL( printVariable( const QString & ) ),
             this, SLOT( printVariable( const QString & ) ) );

    connect( child, SIGNAL( watchVariable( const QString & ) ),
             this, SLOT( watchVariable( const QString & ) ) );

    connect( child, SIGNAL( displayVariable( const QString & ) ),
             this, SLOT( displayVariable( const QString & ) ) );

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

    setOcamlDebugInitScriptAct = new QAction(  tr( "&OCamlDebug Initialization Script..." ), this );
    setOcamlDebugInitScriptAct->setStatusTip( tr( "Set an initialization script executed just after ocamldebug is initialized." ) );
    connect( setOcamlDebugInitScriptAct, SIGNAL( triggered() ), this, SLOT( setOCamlDebugInitScript() ) );

    createWatchWindowAct = new QAction( QIcon( ), tr( "&Create Watch Window..." ), this );
    createWatchWindowAct->setShortcut( QKeySequence("Ctrl+W") );
    createWatchWindowAct->setIcon( QIcon(":/images/watch.png") );
    createWatchWindowAct->setStatusTip( tr( "Create a variable watch window" ) );
    connect( createWatchWindowAct, SIGNAL( triggered() ), this, SLOT( createWatchWindow() ) );

    exitAct = new QAction( tr( "E&xit" ), this );
    exitAct->setShortcuts( QKeySequence::Quit );
    exitAct->setStatusTip( tr( "Exit the application" ) );
    connect( exitAct, SIGNAL( triggered() ), qApp, SLOT( closeAllWindows() ) );

    copyAct = new QAction( QIcon( ":/images/copy.png" ), tr( "&Copy" ), this );
    copyAct->setShortcuts( QKeySequence::Copy );
    copyAct->setStatusTip( tr( "Copy the current selection's contents to the "
                               "clipboard" ) );
    connect( copyAct, SIGNAL( triggered() ), this, SLOT( copy() ) );

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
    debuggerStartAct->setShortcut( QKeySequence("Ctrl+Shift+S") );
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
    debugNextAct->setStatusTip( tr( "Step over (Mouse Wheel with Ctrl)" ) );
    connect( debugNextAct, SIGNAL( triggered() ), this, SLOT( debugNext() ) );

    debugPreviousAct = new QAction( QIcon( ":/images/debug-previous.png" ), tr( "&Previous" ), this );
    debugPreviousAct->setShortcut( QKeySequence("Ctrl+P") );
    debugPreviousAct->setStatusTip( tr( "Back step over (Mouse Wheel with Ctrl)" ) );
    connect( debugPreviousAct, SIGNAL( triggered() ), this, SLOT( debugPrevious() ) );

    debugFinishAct = new QAction( QIcon( ":/images/debug-finish.png" ), tr( "&Finish" ), this );
    debugFinishAct->setShortcut( QKeySequence("Ctrl+F") );
    debugFinishAct->setStatusTip( tr( "Run until function exit" ) );
    connect( debugFinishAct, SIGNAL( triggered() ), this, SLOT( debugFinish() ) );

    debugBackStepAct = new QAction( QIcon( ":/images/debug-backstep.png" ), tr( "&Back Step" ), this );
    debugBackStepAct->setShortcut( QKeySequence("Ctrl+B") );
    debugBackStepAct->setStatusTip( tr( "Step into (Mouse Wheel with Shift)" ) );
    connect( debugBackStepAct, SIGNAL( triggered() ), this, SLOT( debugBackStep() ) );

    debugStepAct = new QAction( QIcon( ":/images/debug-step.png" ), tr( "&Step" ), this );
    debugStepAct->setShortcut( QKeySequence("Ctrl+S") );
    debugStepAct->setStatusTip( tr( "Back step into (Mouse Wheel with Shift)" ) );
    connect( debugStepAct, SIGNAL( triggered() ), this, SLOT( debugStep() ) );

    debugInterruptAct = new QAction( QIcon( ":/images/debug-interrupt.png" ), tr( "&Interrupt" ), this );
    debugInterruptAct->setShortcut( QKeySequence("Ctrl+C") );
    debugInterruptAct->setStatusTip( tr( "Interrupt the execution of the application" ) );
    connect( debugInterruptAct, SIGNAL( triggered() ), this, SLOT( debugInterrupt() ) );


    debugDownAct = new QAction( QIcon( ":/images/debug-down.png" ), tr( "&Down" ), this );
    debugDownAct->setShortcut(  QKeySequence("Ctrl+D") );
    debugDownAct->setStatusTip( tr( "Call stack down (Mouse Wheel with Shift+Ctrl)" ) );
    connect( debugDownAct, SIGNAL( triggered() ), this, SLOT( debugDown() ) );


    debugUpAct = new QAction( QIcon( ":/images/debug-up.png" ), tr( "&Up" ), this );
    debugUpAct->setShortcut( QKeySequence("Ctrl+U") );
    debugUpAct->setStatusTip( tr( "Call stack up (Mouse Wheel with Shift+Ctrl)" ) );
    connect( debugUpAct, SIGNAL( triggered() ), this, SLOT( debugUp() ) );

}

void MainWindow::createMenus()
{
    mainMenu = menuBar()->addMenu( tr( "&Main" ) );
    mainMenu->addAction( setOcamlDebugAct );
    mainMenu->addAction( setOcamlDebugArgsAct );
    mainMenu->addAction( setOcamlDebugInitScriptAct );
    mainMenu->addAction( setWorkingDirectoryAct );
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
    debugMenu->addSeparator();
    debugMenu->addAction( debugDownAct );
    debugMenu->addAction( debugUpAct );
    debugMenu->addSeparator();
    debugMenu->addAction( createWatchWindowAct );

    editMenu = menuBar()->addMenu( tr( "&Edit" ) );
    editMenu->addAction( copyAct );

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

    editToolBar = addToolBar( tr( "Edit" ) );
    editToolBar->setObjectName("EditToolBar");
    editToolBar->addAction( copyAct );

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

    debugWindowToolBar = addToolBar( tr( "Debug Windows" ) );
    debugWindowToolBar->addAction( createWatchWindowAct );
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
        ocamlDebugFocus();
    }
}

void MainWindow::ocamlDebugFocus()
{
    if (ocamldebug)
        ocamldebug->setFocus();
}

void MainWindow::debugUp()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "up", DebuggerCommand::HIDE_DEBUGGER_OUTPUT ) );
}

void MainWindow::debugDown()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "down",DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugInterrupt()
{
    if (ocamldebug)
        ocamldebug->debuggerInterrupt();
}

void MainWindow::debugRun()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "run",DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugFinish()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "finish",DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugReverse()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "reverse", DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugStep()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "step", DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugBackStep()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "backstep", DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugNext()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "next", DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::debugPrevious()
{
    if (ocamldebug)
        ocamldebug->debugger( DebuggerCommand( "previous", DebuggerCommand::HIDE_DEBUGGER_OUTPUT) );
}

void MainWindow::breakPointList(const BreakPoints &b)
{
    foreach ( QMdiSubWindow * window, mdiArea->subWindowList() )
    {
        OCamlSource *mdiChild = qobject_cast<OCamlSource *>( window->widget() );
        if (mdiChild)
            mdiChild->breakPointList( b );
    }
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

void MainWindow::displayVariable( const QString &val )
{
    if ( ocamldebug )
    {
        QString command = QString("display %1").arg(val);
        ocamldebug->debugger( DebuggerCommand( command , DebuggerCommand::SHOW_ALL_OUTPUT) );
    }
}

void MainWindow::printVariable( const QString &val )
{
    if ( ocamldebug )
    {
        QString command = QString("print %1").arg(val);
        ocamldebug->debugger( DebuggerCommand( command , DebuggerCommand::SHOW_ALL_OUTPUT) );
    }
}

void MainWindow::watchVariable( const QString &val )
{
    if ( ocamldebug )
    {
        if ( _watch_windows.isEmpty() )
            createWatchWindow();
        if ( !_watch_windows.isEmpty() )
            _watch_windows.last()->watch( val, true );
    }
}

void MainWindow::fileBrowserItemActivated( const QModelIndex &item ) 
{
    QString file = filebrowser_model_p->filePath( item ) ;
    QFileInfo fileInfo( file );
    if (  fileInfo.isDir() )
    {
        filebrowser_model_p->setRootPath( file );
        filebrowser->setRootIndex( filebrowser_model_p->index( file ) );
    }
    else
    {
        openOCamlSource( file, true );
        ocamlDebugFocus();
    }
}

void MainWindow::fileBrowserPathChanged( const QString &path )
{
    QLabel *label_p = qobject_cast<QLabel*>(filebrowser_dock->titleBarWidget());
    if (label_p)
        label_p->setText( path );
}
