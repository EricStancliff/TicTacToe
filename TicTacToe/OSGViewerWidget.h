#pragma once
#include <QOpenGLWidget>
#include <osgViewer/CompositeViewer>

namespace osgQt
{
    class GLWidget;
    class GraphicsWindowQt;
}

//class based on OSG example for Qt with OSG Viewer
//osgViewerQt.cpp OpenSceneGraph version 3.4.0

class OSGViewerWidget : public QWidget, public osgViewer::CompositeViewer
{
    Q_OBJECT
public:
    OSGViewerWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags(), osgViewer::ViewerBase::ThreadingModel threadingModel = osgViewer::CompositeViewer::SingleThreaded);
    virtual ~OSGViewerWidget() {};

    osgQt::GLWidget* addViewWidget(osgQt::GraphicsWindowQt* gw);

    osgQt::GraphicsWindowQt* createGraphicsWindow();

    std::vector<osgQt::GLWidget*> getGLWidgets() const { return m_qglWidgets; }

protected:
    std::vector<osgQt::GLWidget*> m_qglWidgets;

};
