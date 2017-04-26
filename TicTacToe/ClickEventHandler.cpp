#include "ClickEventHandler.h"
#include "GameMoveManager.h"
#include "GraphicsThread.h"
#include "TApp.h"

#include <QDebug>

ClickEventHandler::ClickEventHandler()
{

}

ClickEventHandler::~ClickEventHandler()
{

}

bool ClickEventHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
    if (ea.getHandled()) 
        return false;

    switch (ea.getEventType())
    {
    case(osgGA::GUIEventAdapter::RELEASE):
    {
        if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
        {
            //we need to test which region the click was closest to...
            int x = -1;
            int y = -1;

            if (ea.getX() < ea.getXmax() / 3)
                x = 0;
            else if (ea.getX() >= (ea.getXmax() / 3) && ea.getX() < ((ea.getXmax() / 3) * 2))
                x = 1;
            else
                x = 2;

            if (ea.getY() < ea.getYmax() / 3)
                y = 2;
            else if (ea.getY() >= (ea.getYmax() / 3) && ea.getY() < ((ea.getYmax() / 3) * 2))
                y = 1;
            else
                y = 0;

            MoveStruct move(x, y, true);

            std::string errMsg;
            
            if (tApp->getGameManager()->storeUserMadeMove(move, errMsg))
                 qWarning() << "Successful Move a position " << x << ", " << y;
            else
                qWarning() << QString::fromStdString(errMsg);

            tApp->getGraphicsThread()->setUserMessage(errMsg);
        }
    }
    break;
    default:
        break;
    }

    return false;

}
