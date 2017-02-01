#pragma once
#include <QOpenGLWidget>


namespace osgViewer
{
    class GraphicsWindowEmbedded;
    class CompositeViewer;
}

//let's try to hide as much of the camera/view management in this class
//as far as the world is concerned, this is just another QWidget... maybe...
class OSGViewerWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    OSGViewerWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    virtual ~OSGViewerWidget();

protected:

    virtual void paintEvent(QPaintEvent* paintEvent);
    virtual void paintGL();
    virtual void resizeGL(int width, int height);

    osgViewer::GraphicsWindowEmbedded* m_graphWindow;
    osgViewer::CompositeViewer*        m_compViewer;
};
