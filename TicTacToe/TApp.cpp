#include "TApp.h"

#include "GameMoveManager.h"
#include "GraphicsThread.h"
TApp::TApp(int argc, char *argv[]) : QApplication(argc, argv),
    m_graphicsThread(nullptr),
    m_gameManager(nullptr)
{
    qRegisterMetaType<MoveStruct>("MoveStruct");

    m_gameManager = new GameMoveManager(this);

    m_graphicsThread = new GraphicsThread();
    m_graphicsThread->start();

}

TApp::~TApp()
{
    m_graphicsThread->setDone(true);
    m_graphicsThread->wait();
    m_graphicsThread->deleteLater();
}
