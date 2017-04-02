#include "ClickEventHandler.h"
#include "GameMoveManager.h"
#include "TApp.h"

#include <iostream>

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
            else if (ea.getX() >= ((ea.getXmax() / 3) * 2) && ea.getX() < ea.getXmax())
                x = 1;
            else
                x = 2;

            if (ea.getY() < ea.getYmax() / 3)
                y = 0;
            else if (ea.getY() >= ((ea.getYmax() / 3) * 2) && ea.getY() < ea.getYmax())
                y = 1;
            else
                y = 2;

            MoveStruct move(x, y);

            std::string errMsg;
            
            if (tApp->getGameManager()->storeUserMadeMove(move, errMsg))
                std::cout << "Successful Move a position " << x << ", " << "y";
            else
                std::cout << errMsg;


        }
    }
    break;
    default:
        break;
    }

    return false;

}
