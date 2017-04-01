#include "OSGViewerWidget.h"


//osg
#include <osgQt/GraphicsWindowQt>

//Qt
#include <QHBoxLayout>




OSGViewerWidget::OSGViewerWidget(QWidget* parent, Qt::WindowFlags f, osgViewer::ViewerBase::ThreadingModel threadingModel) : QWidget(parent, f)
{
    setThreadingModel(threadingModel);

    osgQt::GLWidget* widget1 = addViewWidget(createGraphicsWindow());

    QHBoxLayout* layout = new QHBoxLayout;
    layout->addWidget(widget1, 0, 0);
    setLayout(layout);

    m_qglWidgets.push_back(widget1);
}

osgQt::GLWidget* OSGViewerWidget::addViewWidget(osgQt::GraphicsWindowQt* gw)
{
    osgViewer::View* view = new osgViewer::View;
    addView(view);

    osg::Camera* camera = view->getCamera();
    camera->setGraphicsContext(gw);

    const osg::GraphicsContext::Traits* traits = gw->getTraits();

    camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));

    return gw->getGLWidget();
}

osgQt::GraphicsWindowQt* OSGViewerWidget::createGraphicsWindow()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->doubleBuffer = true;
    traits->alpha = ds->getMinimumNumAlphaBits();
    traits->stencil = ds->getMinimumNumStencilBits();
    traits->sampleBuffers = ds->getMultiSamples();
    traits->samples = ds->getNumMultiSamples();

    return new osgQt::GraphicsWindowQt(traits.get());
}
