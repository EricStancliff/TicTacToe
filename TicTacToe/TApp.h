#pragma once


#include <QApplication>

//define application wide
#define tApp static_cast<TApp*>(QApplication::instance())

class GraphicsThread;
class GameMoveManager;

class TApp : public QApplication
{
    Q_OBJECT

public:
    TApp(int argc, char *argv[]);
    virtual ~TApp();

    GameMoveManager* getGameManager() const {
        return m_gameManager;
    } 

    GraphicsThread* getGraphicsThread() const {
        return m_graphicsThread;
    }

protected:
    GameMoveManager* m_gameManager;
    GraphicsThread* m_graphicsThread;
};