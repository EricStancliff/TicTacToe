#include "OSGViewerWidget.h"

#include "osgViewer/CompositeViewer"
#include "osgViewer/GraphicsWindow"

OSGViewerWidget::OSGViewerWidget(QWidget* parent, Qt::WindowFlags f) :
    QOpenGLWidget(parent, f),
    m_graphWindow(nullptr),
    m_compViewer(nullptr)
{

    m_graphWindow = new osgViewer::GraphicsWindowEmbedded(this->x(), this->y(),this->width(), this->height());

    m_compViewer = new osgViewer::CompositeViewer;

    osg::Camera* camera = new osg::Camera;
    camera->setViewport(0, 0, this->width(), this->height());
    camera->setClearColor(osg::Vec4(0.f, 0.f, 1.f, 1.f));
    camera->setGraphicsContext(m_graphWindow);

    osgViewer::View* view = new osgViewer::View;
    view->setCamera(camera);

    m_compViewer->addView(view);
    m_compViewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
}

OSGViewerWidget::~OSGViewerWidget()
{

}

void OSGViewerWidget::resizeGL(int width, int height)
{
    m_graphWindow->getEventQueue()->windowResize(x(), y(), width, height);
    m_graphWindow->resized(x(), y(), width, height);
    
    std::vector<osg::Camera*> cameras;
    m_compViewer->getCameras(cameras);

    cameras[0]->setViewport(0, 0, width / 2, height);
}

void OSGViewerWidget::paintEvent(QPaintEvent* paintEvent)
{

}

void OSGViewerWidget::paintGL()
{

}
