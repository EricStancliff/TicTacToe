
#include <QMessageBox>
#include <QVBoxLayout>

#include <osgQt/GraphicsWindowQt>


#include "TMainWindow.h"
#include "TApp.h"
#include "OSGViewerWidget.h"
#include "GraphicsThread.h"
#include "GameMoveManager.h"
#include <QOpenGLContext>

TMainWindow::TMainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_glWidget(nullptr)
{
    //create UI components
    m_ui.setupUi(this);

    //create my openGL widget and put in the center
    createOpenGLContext();

    //Tool Bar Actions
    connect(m_ui.actionNew_Game, SIGNAL(triggered(bool)), this, SLOT(handleNewGame()));
    connect(m_ui.actionUndo, SIGNAL(triggered(bool)), this, SLOT(handleUndo()));

    //Menu Actions
    connect(m_ui.actionAbout, SIGNAL(triggered(bool)), this, SLOT(handleAbout()));
    connect(m_ui.actionExit, SIGNAL(triggered(bool)), this, SLOT(handleQuit()));
}

TMainWindow::~TMainWindow()
{
    //tell graphics that we want our viewer back
    tApp->getGraphicsThread()->setOSGViewer(nullptr);

    auto mainThread = QThread::currentThread();
    auto osgViewer = m_glWidget;

    tApp->getGraphicsThread()->addTaskBlocking([mainThread, osgViewer]() {
        //move to our thread
        auto glWidgetList = osgViewer->getGLWidgets();
        for (auto&& glWidget : glWidgetList)
            glWidget->context()->moveToThread(mainThread);
    });
}


void TMainWindow::handleQuit()
{
    //peace!
    tApp->quit();
}

void TMainWindow::handleAbout()
{
    //popup box, dynamically allocate since we're not going to exec it
    //we don't need it to be modal, so we don't want it going out of scope
    QMessageBox* msgBox = new QMessageBox(this);  //parenting for positioning, not memory cleanup
    msgBox->setText("Sample TicTacToe by EStancliff");
    //clean up memory on close
    msgBox->setModal(false);
    msgBox->setAttribute(Qt::WA_DeleteOnClose);
    msgBox->show();
}

void TMainWindow::handleUndo()
{

}

void TMainWindow::handleNewGame()
{
    tApp->getGameManager()->clearGame();
    tApp->getGraphicsThread()->addTask([]() {
        //clear the board
    });
}

void TMainWindow::createOpenGLContext()
{
    //create gl context widget
    m_glWidget = new OSGViewerWidget(this);

    //set main layout
    QVBoxLayout* mainLayout = new QVBoxLayout();
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    centralWidget()->setLayout(mainLayout);

    //add to the main layout
    mainLayout->addWidget(m_glWidget);

    auto osgViewer = m_glWidget;

    //render first time in this thread, and let it draw to get
    //around Qt's rules on thread affinity for OpenGLContext and
    //swapping buffers on hidden objects.
    osgViewer->frame();
    QApplication::processEvents();

    //move to graphics thread
    auto glWidgetList = osgViewer->getGLWidgets();
    for (auto&& glWidget : glWidgetList)
        glWidget->context()->moveToThread(tApp->getGraphicsThread());


    //move graphics to current thread
    tApp->getGraphicsThread()->addTaskBlocking([osgViewer]() {
        tApp->getGraphicsThread()->setOSGViewer(osgViewer);
        tApp->getGraphicsThread()->init();
    });

}
