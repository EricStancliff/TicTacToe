
#include <QMessageBox>
#include <QVBoxLayout>

#include "TMainWindow.h"
#include "TApp.h"
#include "OSGViewerWidget.h"
#include "GraphicsThread.h"
#include "GameMoveManager.h"

TMainWindow::TMainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_glWidget(nullptr)
{
    //create UI components
    m_ui.setupUi(this);

    //create my openGL widget and put in the center
    createOpenGLContext();

    connect(m_ui.actionNew_Game, SIGNAL(triggered(bool)), this, SLOT(handleNewGame()));
    connect(m_ui.actionUndo, SIGNAL(triggered(bool)), this, SLOT(handleUndo()));
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
}
