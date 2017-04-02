#pragma once
#include <osgGA/GUIEventHandler>

struct ClickEventHandler : public osgGA::GUIEventHandler
{
public:
    ClickEventHandler();
    ~ClickEventHandler();

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
};
