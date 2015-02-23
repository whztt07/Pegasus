/****************************************************************************************/
/*                                                                                      */
/*                                    Pegasus Editor                                    */
/*                                                                                      */
/****************************************************************************************/

//! \file	Editor.h
//! \author	Kevin Boulanger
//! \date	02nd June 2013
//! \brief	Main window of Pegasus Editor

#include "Editor.h"
#include "Log.h"
#include "Assertion.h"
#include "Application/ApplicationManager.h"
#include "Settings/SettingsDialog.h"

#include <QUndoStack>
#include <QSplashScreen>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QCloseEvent>
#include <QApplication>
#include <QFileDialog>


Editor * Editor::sInstance = nullptr;
Settings * Editor::sSettings = nullptr;

//----------------------------------------------------------------------------------------

Editor::Editor(QApplication * parentApplication)
:   QMainWindow(nullptr),
    mApplicationManager(nullptr),
    mQtApplication(parentApplication),
    mMainViewportDockWidget(nullptr),
    mSecondaryViewportDockWidget(nullptr),
    mTimelineDockWidget(nullptr),
    mHistoryDockWidget(nullptr),
    mAssetLibraryWidget(nullptr),
    mCodeEditorWidget(nullptr),
    mConsoleDockWidget(nullptr),
    mTextureEditorDockWidget(nullptr)
{
    sInstance = this;

    // Create the log manager
    mLogManager = new LogManager(this);

    // Create the assertion manager
    mAssertionManager = new AssertionManager(this);

    // Create the undo stack
    mUndoStack = new QUndoStack(this);

    // Create the splash screen (it becomes visible once this class is initialized,
    // set by the application class)
    //! \todo Finish implementing the splash screen
    //mSplashScreen = new QSplashScreen(Qpixmap(EDITOR_SPLASH_IMAGE_FILE));
    //mSplashScreen->show();

    // Set the title of the main window
    //! \todo Use a constant in the configuration header
    setWindowTitle(tr("Pegasus Editor"));

    // Set the initial window size
    resize(INITIAL_WINDOW_WIDTH, INITIAL_WINDOW_HEIGHT);
    //setWindowState(windowState() | Qt::WindowMaximized);

    // Create the objects used for the main window user interface
    CreateActions();
    CreateToolBars();
    CreateStatusBar();

    //! Finish handling the initialization of the Pegasus engine.
    CreateDockWidgets();
    CreateMenu();

    // Create the settings object, read the setting values from the settings file and apply them
	if (sSettings == nullptr)
    {
        sSettings = new Settings(this);
    }

    // Write the log console header
    ED_LOG("Pegasus Editor settings loaded");

    // Test messages
    /****/
    //! \todo Remove completely, this is kept for now to debug job000035
	//for (int i = 0; i < 100; i++)
 //   {
 //       if ((i / 10) & 1)
 //       {
 //           if (i == 12) mConsoleDockWidget->AddMessage('CRIT', QString("Hello, world %1!").arg(i));
 //           else if (i == 13) mConsoleDockWidget->AddMessage('ERR_', QString("Hello, world %1!").arg(i));
 //           else if (i == 14) mConsoleDockWidget->AddMessage('ASRT', QString("Hello, world %1!").arg(i));
 //           else if (i == 15) mConsoleDockWidget->AddMessage('WNDW', QString("Hello, world %1!").arg(i));
 //           else mConsoleDockWidget->AddMessage('WARN', QString("Hello, world %1!").arg(i));
 //       }
 //       else
 //       {
 //           mConsoleDockWidget->AddMessage(QString("Hello, world %1!").arg(i));
 //       }
 //   }

    // Create the application manager
    mApplicationManager = new ApplicationManager(this, this);

    connect(mApplicationManager, SIGNAL(ApplicationLoaded()),
            this, SLOT(UpdateUIForAppLoaded()));
    connect(mApplicationManager, SIGNAL(ApplicationFinished()),
            this, SLOT(UpdateUIForAppClosed()));

    connect(mApplicationManager, SIGNAL(ApplicationLoaded()),
            mTimelineDockWidget, SLOT(UpdateUIForAppLoaded()));
    connect(mApplicationManager, SIGNAL(ApplicationFinished()),
            mTimelineDockWidget, SLOT(UpdateUIForAppClosed()));

    connect(mApplicationManager, SIGNAL(ApplicationFinished()),
            mCodeEditorWidget, SLOT(UpdateUIForAppFinished()));
    connect(mApplicationManager, SIGNAL(ApplicationLoaded()),
            mAssetLibraryWidget, SLOT(UpdateUIForAppLoaded()));
    connect(mApplicationManager, SIGNAL(ApplicationFinished()),
            mAssetLibraryWidget, SLOT(UpdateUIForAppFinished()));
   
    connect(sSettings, SIGNAL(OnCodeEditorStyleChanged()),
            mCodeEditorWidget, SLOT(OnSettingsChanged())); 

    connect(mApplicationManager, SIGNAL(ApplicationLoaded()),
            mTextureEditorDockWidget, SLOT(UpdateUIForAppLoaded()));
    connect(mApplicationManager, SIGNAL(ApplicationFinished()),
            mTextureEditorDockWidget, SLOT(UpdateUIForAppClosed()));

    sSettings->NotifySettingsChanged();

}

//----------------------------------------------------------------------------------------

Editor::~Editor()
{
    //! \todo Handle the closing of the apps that are still open

    if (mAssertionManager != nullptr)
    {
        delete mAssertionManager;
    }
    if (mLogManager != nullptr)
    {
        delete mLogManager;
    }
    sInstance = nullptr;
}

//----------------------------------------------------------------------------------------

void Editor::RaiseSplashScreen()
{
	if (mSplashScreen != nullptr)
    {
        mSplashScreen->raise();
    }
}

//----------------------------------------------------------------------------------------

void Editor::CloseSplashScreen()
{
	if (mSplashScreen != nullptr)
    {
        delete mSplashScreen;
        mSplashScreen = nullptr;
    }
}

//----------------------------------------------------------------------------------------

void Editor::PushUndoCommand(QUndoCommand * command)
{
    if (command != nullptr)
    {
        mUndoStack->push(command);
    }
    else
    {
        ED_FAILSTR("Invalid command pushed to the undo manager");
    }
}

//----------------------------------------------------------------------------------------

ViewportWidget * Editor::GetViewportWidget(ViewportType viewportType) const
{
    switch (viewportType)
    {
        case VIEWPORTTYPE_MAIN:
            return mMainViewportDockWidget->GetViewportWidget();

        case VIEWPORTTYPE_SECONDARY:
            return mSecondaryViewportDockWidget->GetViewportWidget();

        case VIEWPORTTYPE_TEXTURE_EDITOR_PREVIEW:
            return mTextureEditorDockWidget->GetViewportWidget();

        case VIEWPORTTYPE_MESH_EDITOR_PREVIEW:
            //! \todo Add support for mesh editor
            ED_FAILSTR("The mesh editor preview viewport is not implemented yet");
            return nullptr;

        default:
            ED_FAILSTR("Invalid viewport widget type (%d), it should be < %d", viewportType, NUM_VIEWPORT_TYPES);
            return nullptr;
    }
}

//----------------------------------------------------------------------------------------

void Editor::CreateActions()
{
	mActionFileOpenApp = new QAction(tr("&Open App..."), this);
	mActionFileOpenApp->setIcon(QIcon(":/Toolbar/File/OpenApp24.png"));
	mActionFileOpenApp->setShortcut(tr("Ctrl+O"));
	mActionFileOpenApp->setStatusTip(tr("Open an existing app"));
	connect(mActionFileOpenApp, SIGNAL(triggered()), this, SLOT(OpenApp()));

	mActionFileReloadApp = new QAction(tr("&Reload App"), this);
	//! \todo Use the correct icon
    mActionFileReloadApp->setIcon(QIcon(":/Toolbar/File/refresh-512.png"));
	mActionFileReloadApp->setShortcut(tr("Ctrl+R"));
	mActionFileReloadApp->setStatusTip(tr("Reload the current app"));
	connect(mActionFileReloadApp, SIGNAL(triggered()), this, SLOT(ReloadApp()));
    mActionFileReloadApp->setEnabled(false);

    mActionFileCloseApp = new QAction(tr("&Close App"), this);
	//! \todo Use the correct icon
    mActionFileCloseApp->setIcon(QIcon(":/Toolbar/File/SaveScene24.png"));
	mActionFileCloseApp->setShortcut(tr("Shift+F4"));
	mActionFileCloseApp->setStatusTip(tr("Close the current app"));
	connect(mActionFileCloseApp, SIGNAL(triggered()), this, SLOT(CloseApp()));
    mActionFileCloseApp->setEnabled(false);

    mActionFileQuit = new QAction(tr("&Quit"), this);
	mActionFileQuit->setShortcut(tr("Alt+F4"));
	mActionFileQuit->setStatusTip(tr("Quit Pegasus Editor"));
	connect(mActionFileQuit, SIGNAL(triggered()), this, SLOT(Quit()));


    mActionEditUndo = mUndoStack->createUndoAction(this, tr("&Undo"));
	mActionEditUndo->setIcon(QIcon(":/Toolbar/Edit/Undo16.png"));
	mActionEditUndo->setShortcut(tr("Ctrl+Z"));
	mActionEditUndo->setStatusTip(tr("Undo the last command"));

    mActionEditRedo = mUndoStack->createRedoAction(this, tr("&Redo"));
	mActionEditRedo->setIcon(QIcon(":/Toolbar/Edit/Redo16.png"));
	mActionEditRedo->setShortcut(tr("Ctrl+Y"));
	mActionEditRedo->setStatusTip(tr("Redo the last command"));

    mActionEditPreferences = new QAction(tr("&Preferences..."), this);
	mActionEditPreferences->setShortcut(tr("Ctrl+K"));
	mActionEditPreferences->setStatusTip(tr("Open the preferences dialog"));
	connect(mActionEditPreferences, SIGNAL(triggered()), this, SLOT(OpenPreferences()));


	mActionViewShowFullscreenViewport = new QAction(tr("Show &Fullscreen Viewport"), this);
	mActionViewShowFullscreenViewport->setCheckable(true);
	mActionViewShowFullscreenViewport->setChecked(false);
	mActionViewShowFullscreenViewport->setIcon(QIcon(":/Toolbar/View/FullScreenViewport24.png"));
	mActionViewShowFullscreenViewport->setStatusTip(tr("Open a fullscreen viewport of the scene"));
	connect(mActionViewShowFullscreenViewport, SIGNAL(triggered()), this, SLOT(ShowFullscreenViewport()));


    mActionCreatePrimitiveBox = new QAction(tr("&Box"), this);
	mActionCreatePrimitiveBox->setStatusTip(tr("Create a box object"));
	connect(mActionCreatePrimitiveBox, SIGNAL(triggered()), this, SLOT(CreateBox()));

    mActionCreatePrimitiveSphere = new QAction(tr("&Sphere"), this);
	mActionCreatePrimitiveSphere->setStatusTip(tr("Create a sphere object"));
	connect(mActionCreatePrimitiveSphere, SIGNAL(triggered()), this, SLOT(CreateSphere()));


    mActionWindowMainViewport = new QAction(tr("&Main Viewport"), this);
	mActionWindowMainViewport->setStatusTip(tr("Open the main viewport window"));
	connect(mActionWindowMainViewport, SIGNAL(triggered()), this, SLOT(OpenMainViewportWindow()));

    mActionWindowSecondaryViewport = new QAction(tr("&Secondary Viewport"), this);
	mActionWindowSecondaryViewport->setStatusTip(tr("Open the secondary viewport window"));
	connect(mActionWindowSecondaryViewport, SIGNAL(triggered()), this, SLOT(OpenSecondaryViewportWindow()));

    mActionWindowTimeline = new QAction(tr("&Timeline"), this);
	mActionWindowTimeline->setStatusTip(tr("Open the timeline window"));
	connect(mActionWindowTimeline, SIGNAL(triggered()), this, SLOT(OpenTimelineWindow()));

    mActionWindowHistory = new QAction(tr("&History"), this);
	mActionWindowHistory->setStatusTip(tr("Open the history window"));
	connect(mActionWindowHistory, SIGNAL(triggered()), this, SLOT(OpenHistoryWindow()));

    mActionWindowConsole = new QAction(tr("&Console"), this);
	mActionWindowConsole->setStatusTip(tr("Open the console window"));
	connect(mActionWindowConsole, SIGNAL(triggered()), this, SLOT(OpenConsoleWindow()));

    mActionWindowCodeEditor = new QAction(tr("&Code Editor"), this);
	mActionWindowCodeEditor->setStatusTip(tr("Open the code editor"));
	connect(mActionWindowCodeEditor, SIGNAL(triggered()), this, SLOT(OpenCodeEditorWindow()));

    mActionWindowAssetLibrary = new QAction(tr("&Asset Library"), this);
	mActionWindowAssetLibrary->setStatusTip(tr("Open the asset library"));
	connect(mActionWindowAssetLibrary, SIGNAL(triggered()), this, SLOT(OpenAssetLibraryWindow()));

    mActionWindowTextureEditor = new QAction(tr("Te&xture Editor"), this);
	mActionWindowTextureEditor->setStatusTip(tr("Open the texture editor window"));
	connect(mActionWindowTextureEditor, SIGNAL(triggered()), this, SLOT(OpenTextureEditorWindow()));

    mActionHelpIndex = new QAction(tr("&Index..."), this);
	mActionHelpIndex->setShortcut(tr("F1"));
	mActionHelpIndex->setStatusTip(tr("Open the help for Pegasus Editor"));
	connect(mActionHelpIndex, SIGNAL(triggered()), this, SLOT(HelpIndex()));

	mActionHelpAboutQt = new QAction(tr("&About Qt..."), this);
	mActionHelpAboutQt->setStatusTip(tr("Open the Qt about dialog box"));
	connect(mActionHelpAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

	mActionHelpAbout = new QAction(tr("&About..."), this);
	mActionHelpAbout->setStatusTip(tr("Open the about dialog box"));
	connect(mActionHelpAbout, SIGNAL(triggered()), this, SLOT(About()));
}

//----------------------------------------------------------------------------------------

void Editor::CreateMenu()
{
    QMenu * fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(mActionFileOpenApp);
    fileMenu->addAction(mActionFileReloadApp);
    fileMenu->addAction(mActionFileCloseApp);
    fileMenu->addSeparator();
    fileMenu->addAction(mActionFileQuit);

    QMenu * editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(mActionEditUndo);
    editMenu->addAction(mActionEditRedo);
    editMenu->addSeparator();
    editMenu->addAction(mActionEditPreferences);

    QMenu * viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(mActionViewShowFullscreenViewport);

    menuBar()->addMenu(mAssetLibraryWidget->CreateNewAssetMenu(tr("&Create"), this));

    QMenu * windowMenu = menuBar()->addMenu(tr("&Window"));
	//mToolbarMenu = windowMenu->addMenu(tr("&Toolbars"));
	//mDockMenu = windowMenu->addMenu(tr("&Windows"));
    //! \todo Temporary. Use the toolbar and dock menus with checkboxes
    windowMenu->addAction(mActionWindowMainViewport);
    windowMenu->addAction(mActionWindowSecondaryViewport);
    windowMenu->addAction(mActionWindowTimeline);
    windowMenu->addAction(mActionWindowHistory);
    windowMenu->addAction(mActionWindowConsole);
    windowMenu->addAction(mActionWindowCodeEditor);
    windowMenu->addAction(mActionWindowAssetLibrary);
    windowMenu->addAction(mActionWindowTextureEditor);

    QMenu * helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(mActionHelpIndex);
    helpMenu->addSeparator();
    helpMenu->addAction(mActionHelpAboutQt);
    helpMenu->addAction(mActionHelpAbout);

}

//----------------------------------------------------------------------------------------

void Editor::CreateToolBars()
{
    QToolBar * fileToolBar = addToolBar(tr("File"));
    fileToolBar->setObjectName("FileToolBar");
    fileToolBar->setIconSize(QSize(16, 16));
	fileToolBar->setAllowedAreas(Qt::TopToolBarArea);
	fileToolBar->addAction(mActionFileOpenApp);
	fileToolBar->addAction(mActionFileReloadApp);
    fileToolBar->addAction(mActionFileCloseApp);

	QToolBar * editToolBar = addToolBar(tr("Edit"));
    editToolBar->setObjectName("EditToolBar");
	editToolBar->setIconSize(QSize(16, 16));
	editToolBar->setAllowedAreas(Qt::TopToolBarArea);
	editToolBar->addAction(mActionEditUndo);
	editToolBar->addAction(mActionEditRedo);

    QToolBar * viewToolBar = addToolBar(tr("View"));
    viewToolBar->setObjectName("ViewToolBar");
	viewToolBar->setIconSize(QSize(16, 16));
	viewToolBar->setAllowedAreas(Qt::TopToolBarArea);
	viewToolBar->addAction(mActionViewShowFullscreenViewport);

	// Create and place the view actions for the toolbars to the toolbar menu
	// (in alphabetical order)
	//toolbarMenu->addAction(_editToolBar->toggleViewAction());
	//toolbarMenu->addAction(_fileToolBar->toggleViewAction());
	//toolbarMenu->addAction(_viewToolBar->toggleViewAction());
}

//----------------------------------------------------------------------------------------

void Editor::CreateDockWidgets()
{
    // Set the dock widget areas to assign to the four main window corners
    setCorner(Qt::TopLeftCorner    , Qt::TopDockWidgetArea );
    setCorner(Qt::TopRightCorner   , Qt::TopDockWidgetArea);
    setCorner(Qt::BottomLeftCorner , Qt::BottomDockWidgetArea );
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);

    // Allow nesting of dock widgets within dock areas
    setDockNestingEnabled(true);

    // Create the dock widgets and assign their initial position
    //! \todo Use the correct icons for the docks, and add them to the menu and toolbar

    mMainViewportDockWidget = new ViewportDockWidget(VIEWPORTTYPE_MAIN, this);
    //mMainViewportDockWidget->setWindowIcon(QIcon(QPixmap(":/res/qt.png")));
    addDockWidget(Qt::TopDockWidgetArea, mMainViewportDockWidget);

    mSecondaryViewportDockWidget = new ViewportDockWidget(VIEWPORTTYPE_SECONDARY, this);
    //mSecondaryViewportDockWidget->setWindowIcon(QIcon(QPixmap(":/res/qt.png")));
    addDockWidget(Qt::TopDockWidgetArea, mSecondaryViewportDockWidget);

    mTimelineDockWidget = new TimelineDockWidget(this);
    //mTimelineDockWidget->setWindowIcon(QIcon(QPixmap(":/res/qt.png")));
    addDockWidget(Qt::BottomDockWidgetArea, mTimelineDockWidget);

    mHistoryDockWidget = new HistoryDockWidget(mUndoStack, this);
    //mHistoryDockWidget->setWindowIcon(QIcon(QPixmap(":/res/qt.png")));
    addDockWidget(Qt::RightDockWidgetArea, mHistoryDockWidget);

    mConsoleDockWidget = new ConsoleDockWidget(this);
    //mConsoleDockWidget->setWindowIcon(QIcon(QPixmap(":/res/qt.png")));
    addDockWidget(Qt::BottomDockWidgetArea, mConsoleDockWidget);

    mCodeEditorWidget = new CodeEditorWidget(this);
    addDockWidget(Qt::BottomDockWidgetArea, mCodeEditorWidget);
    mCodeEditorWidget->setFloating(true);

    mAssetLibraryWidget = new AssetLibraryWidget(this, mCodeEditorWidget);
    // ** do not add the Code library as default, instead hide it **
    addDockWidget(Qt::RightDockWidgetArea, mAssetLibraryWidget);        
    mAssetLibraryWidget->setFloating(true);

    mTextureEditorDockWidget = new TextureEditorDockWidget(this);
    //mTextureEditorDockWidget->setWindowIcon(QIcon(QPixmap(":/res/qt.png")));
    addDockWidget(Qt::RightDockWidgetArea, mTextureEditorDockWidget);

    // Create and place the view actions for the dock widgets to the dock menu
    // (in alphabetical order)
    //! \todo To implement from N3D editor
}

//----------------------------------------------------------------------------------------

void Editor::CreateStatusBar()
{
	statusBar()->showMessage(tr("Ready"));
}

//----------------------------------------------------------------------------------------

void Editor::InternalCloseAndPumpEvents()
{
   mApplicationManager->CloseApplication(false);
   while (!mApplicationManager->PollApplicationThreadIsDone())
   {
       //let the operating system process any extra events internally, such as detection of destruction of child windows
       mQtApplication->processEvents();
   }
   mApplicationManager->OnApplicationFinished();
}

//----------------------------------------------------------------------------------------

void Editor::closeEvent(QCloseEvent * event)
{
     // Close the current application if one is open
    if (mApplicationManager != nullptr)
    {
        if (mApplicationManager->IsApplicationOpened())
        {
            InternalCloseAndPumpEvents();
        }
    }
 
	//	// Save the application settings and destroy the settings object
    if (sSettings != nullptr)
    {
        sSettings->Save();
    }
    delete sSettings;
    sSettings = nullptr;
}

//----------------------------------------------------------------------------------------

void Editor::OpenApp()
{
    //! \todo If a scene is open, handle its saving and closing (maybe use closeEvent?)

	// Open the file dialog box
	//! \todo Manage the current directory
#if PEGASUS_PLATFORM_WINDOWS
    QString filter("Pegasus application (*.dll)");
#else
#error "Unhandled platform for the Pegasus editor"
#endif
    //! \todo Use a directory that comes from the preferences
    QString fileName = QFileDialog::getOpenFileName(this, "Load application",
													QString("../../../Bin/VS11/Win32/Dev"), filter);

	// Import the file to the current scene
	if (!fileName.isEmpty())
    {
        //if an application is already opened, close it first
        if (mApplicationManager->IsApplicationOpened())
        {
            InternalCloseAndPumpEvents();
        }

        mApplicationManager->OpenApplication(fileName);
        //! \todo Handle errors
    }
}

//----------------------------------------------------------------------------------------

void Editor::ReloadApp()
{
    ED_ASSERTSTR(mApplicationManager->IsApplicationOpened(), "An application must be opened to be able to reload it");

    // Get the filename of the current app
    const QString appFileName = mApplicationManager->GetApplication()->GetFileName();

    // Close the current app
    InternalCloseAndPumpEvents();

    // Open the app again with the cached file name
    mApplicationManager->OpenApplication(appFileName);
}

//----------------------------------------------------------------------------------------

void Editor::CloseApp()
{
    if (mApplicationManager->IsApplicationOpened())
    {
        mApplicationManager->CloseApplication();
    }
}

//----------------------------------------------------------------------------------------

void Editor::Quit()
{
    //! \todo Properly quit the editor
}

//----------------------------------------------------------------------------------------

void Editor::OpenPreferences()
{
	SettingsDialog * settingsDialog = nullptr;
    settingsDialog = new SettingsDialog(this);
	settingsDialog->show();
}

//----------------------------------------------------------------------------------------

void Editor::ShowFullscreenViewport()
{
    //! /todo Show the viewport window in fullscreen
}

//----------------------------------------------------------------------------------------

void Editor::CreateBox()
{
    //! /todo Create a box primitive
}

//----------------------------------------------------------------------------------------

void Editor::CreateSphere()
{
    //! /todo Create a sphere primitive
}

//----------------------------------------------------------------------------------------

void Editor::OpenMainViewportWindow()
{
    mMainViewportDockWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenSecondaryViewportWindow()
{
    mSecondaryViewportDockWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenTimelineWindow()
{
    mTimelineDockWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenHistoryWindow()
{
    mHistoryDockWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenConsoleWindow()
{
    mConsoleDockWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenCodeEditorWindow()
{
    mCodeEditorWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenAssetLibraryWindow()
{
    mAssetLibraryWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenTextureEditorWindow()
{
    mTextureEditorDockWidget->show();
}

//----------------------------------------------------------------------------------------

void Editor::HelpIndex()
{
    //! /todo Show the help
}

//----------------------------------------------------------------------------------------

void Editor::About()
{
    //! /todo Show the about dialog box
}

//----------------------------------------------------------------------------------------

void Editor::UpdateUIForAppLoaded()
{
    mActionFileReloadApp->setEnabled(true);
    mActionFileCloseApp->setEnabled(true);
}

//----------------------------------------------------------------------------------------

void Editor::UpdateUIForAppClosed()
{
    mActionFileReloadApp->setEnabled(false);
    mActionFileCloseApp->setEnabled(false);
}

//----------------------------------------------------------------------------------------

void Editor::OpenPreferencesAppearance()
{
	SettingsDialog * settingsDialog = nullptr;
    settingsDialog = new SettingsDialog(this);
    settingsDialog->SetCurrentPage(SettingsDialog::PAGE_APPEARANCE);
	settingsDialog->show();
}

//----------------------------------------------------------------------------------------

void Editor::OpenPreferencesConsole()
{
	SettingsDialog * settingsDialog = nullptr;
    settingsDialog = new SettingsDialog(this);
    settingsDialog->SetCurrentPage(SettingsDialog::PAGE_CONSOLE);
	settingsDialog->show();
}
