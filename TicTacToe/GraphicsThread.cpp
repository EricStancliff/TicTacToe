#include "GraphicsThread.h"

#include <osgViewer/CompositeViewer>
#include <osgViewer/GraphicsWindow>

#include <osgQt/GraphicsWindowQt>

#include <osg/PositionAttitudeTransform>
#include <osg/LineSegment>
#include <osgText/Text>
#include <QApplication>


GraphicsThread::GraphicsThread(QObject *parent) : QThread(parent),
    m_done(false),
    m_osgViewer(nullptr),
    m_threadsWaiting(false)
{

}

GraphicsThread::~GraphicsThread()
{
    m_osgViewer->deleteLater();
}

//not going to lock here.  How can you have a word tear on a bool? you can't!  
void GraphicsThread::setDone(bool done)
{
    m_done = done;
}

void GraphicsThread::addTask(std::function<void()> task)
{
    QWriteLocker lock(&m_RWLock);
    m_tasks.push_back(task);
}

void GraphicsThread::addTaskBlocking(std::function<void()> task)
{
    std::unique_lock<std::mutex> lock(m_conditionMutex);

    {
        QWriteLocker lock(&m_RWLock);
        m_tasks.push_back(task);
        m_threadsWaiting = true;
    }

    m_condition.wait(lock);
}


void GraphicsThread::init()
{
    QReadLocker lock(&m_RWLock);
    createBoard();
}

void GraphicsThread::run()
{
    this->moveToThread(this);

    //test the rescale function
#ifdef _DEBUG
    testRescaleRange();
#endif
    //okay, so we know setting our sleep time to anything less than 16ms doesn't matter without
    //adjusting the Windows timer resolution.  But sleep 0 I beleive is a special case that lets
    //us steal the processor, only giving up context to the OS when it deems neccessary.
    //like not sleeping, but not evil.
    //we wanna go fast!
    while (!m_done)
    {
        //would it be faster to make a local copy
        //of the tasks and let go of the lock before processing them?
        //if one of the functions adds another task we could deadlock, let's do it.

        //we'll avoid the costly write lock most of the time
        //by checking if we even have functions to process at all first
        bool hasTasks = false;
        bool otherThreadsWaiting = false;
        //scope the lock
        {
            QReadLocker lock(&m_RWLock);
            hasTasks = !m_tasks.empty();
        }

        if (hasTasks)
        {

            std::vector<std::function<void()>> localTasks;
            //scope the lock
            {
                QWriteLocker lock(&m_RWLock);
                std::move(m_tasks.begin(), m_tasks.end(), std::back_inserter(localTasks));
                m_tasks.clear();
                otherThreadsWaiting = m_threadsWaiting;
                m_threadsWaiting = false;
            }

            //execute tasks
            for (auto&& task : localTasks)
                task();

            if (otherThreadsWaiting)
            {
                std::unique_lock<std::mutex> lock(m_conditionMutex);
                lock.unlock();
                m_condition.notify_all();
            }

        }

        updateBoard();

        //step viewer
        if (m_osgViewer)
            m_osgViewer->frame();

        //let Qt's event queue process
        QApplication::processEvents();

        sleep(0);
    }
}

void GraphicsThread::createBoard()
{
    if (!m_osgViewer)
        return;

    std::vector<osg::Camera*> cameras;
    m_osgViewer->getCameras(cameras);

    assert(cameras.size());

    osg::Camera* camera = nullptr;

    if (cameras.size())
    {
        camera = cameras[0];
    }

    if (!camera)
        return;

    m_rootGroup = new osg::Group;

    camera->addChild(m_rootGroup);

    m_boardTransform = new osg::PositionAttitudeTransform;
    m_rootGroup->addChild(m_boardTransform);

    osg::Geode* lineGeode = new osg::Geode;
    osg::Geometry* segment = new osg::Geometry;
    
    auto xMax = camera->getViewport()->width();
    auto yMax = camera->getViewport()->height();


    osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
    color->push_back(osg::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));

    segment->setColorArray(color, osg::Array::BIND_OVERALL);

    segment->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

    lineGeode->addDrawable(segment);
    m_boardLines.push_back(lineGeode);

    m_boardTransform->addChild(lineGeode);

}

void GraphicsThread::updateBoard()
{
    if (!m_osgViewer)
        return;

    std::vector<osg::Camera*> cameras;
    m_osgViewer->getCameras(cameras);

    assert(cameras.size());

    osg::Camera* camera = nullptr;

    if (cameras.size())
    {
        camera = cameras[0];
    }

    if (!camera)
        return;

    auto xMax = camera->getViewport()->width();
    auto yMax = camera->getViewport()->height();
    // set the projection matrix
    camera->setProjectionMatrixAsOrtho2D(0, xMax, 0, yMax);

    //update position of board
    for (int i = 0; i < m_boardLines.size(); ++i)
    {
        osg::Geode* lineGeode = m_boardLines.at(i);
        osg::Geometry* segment = dynamic_cast<osg::Geometry*>(lineGeode->getDrawable(0));

        //clockwise
        osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array;
        points->push_back(osg::Vec3d(20.0, (yMax / 3.0), 0.0));
        points->push_back(osg::Vec3d((xMax - 20.0), (yMax / 3.0), 0.0));
        points->push_back(osg::Vec3d((xMax - 20.0), ((yMax / 3.0) - 10.0), 0.0));
        points->push_back(osg::Vec3d(20.0, ((yMax / 3.0) - 10.0), 0.0));

        segment->setVertexArray(points);
    }
    m_boardTransform->setPosition(camera->getInverseViewMatrix().getTrans());
}