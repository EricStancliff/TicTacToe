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
    std::unique_lock<std::mutex> lock(m_blockingTaskMutex);

    {
        QWriteLocker lock(&m_RWLock);
        m_tasks.push_back(task);
        m_threadsWaiting = true;
    }

    m_blockingTaskComplete.wait(lock);
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
        //scope the lock
        {
            QReadLocker lock(&m_RWLock);
            hasTasks = !m_tasks.empty();
        }

        if (hasTasks)
        {

            bool otherThreadsWaiting = false;
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
                std::unique_lock<std::mutex> lock(m_blockingTaskMutex);
                lock.unlock();
                m_blockingTaskComplete.notify_all();
            }

        }

        updateBoard();
        updateGameStats();
        updateGamePieces();

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

    //make four lines for the board
    for (int i = 0; i < 4; ++i)
    {
        osg::Geode* lineGeode = new osg::Geode;
        osg::Geometry* segment = new osg::Geometry;

        osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
        color->push_back(osg::Vec4f(1.0f, 0.0f, 0.0f, 1.0f));

        segment->setColorArray(color, osg::Array::BIND_OVERALL);

        segment->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

        lineGeode->addDrawable(segment);
        m_boardLines.push_back(lineGeode);

        m_boardTransform->addChild(lineGeode);
    }

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

    // set the projection matrix of the camera, this probably doesn't belong here
    camera->setProjectionMatrixAsOrtho2D(0, xMax, 0, yMax);


    double posMultiplier;
    //update position of board
    for (int i = 0; i < m_boardLines.size(); ++i)
    {
        //here we should be able to assume all geodes are valid, each one has one drawable, and each
        //drawable is a geometry.  We'll check just to be extra safe though.
        osg::Geode* lineGeode = m_boardLines.at(i);
        assert(lineGeode);
        if (!lineGeode)
            continue;

        assert(lineGeode->getNumDrawables() == 1);

        osg::Geometry* segment = dynamic_cast<osg::Geometry*>(lineGeode->getDrawable(0));
        assert(segment);
        if (!segment)
            continue;

        //clockwise, starting at top left
        osg::Vec3Array* points = dynamic_cast<osg::Vec3Array*>(segment->getVertexArray());
        if (!points)
        {
            points = new osg::Vec3Array;
        }
        points->clear();

        //two horizontal, then two vertical
        if (i < 2)
        {
            posMultiplier = i == 0 ? 1.0 : 2.0;
            points->push_back(osg::Vec3d(20.0, (yMax / 3.0) * posMultiplier, 0.0));
            points->push_back(osg::Vec3d((xMax - 20.0), (yMax / 3.0) * posMultiplier, 0.0));
            points->push_back(osg::Vec3d((xMax - 20.0), ((yMax / 3.0)* posMultiplier) - 10.0 , 0.0));
            points->push_back(osg::Vec3d(20.0, ((yMax / 3.0)* posMultiplier) - 10.0, 0.0));
        }
        else
        {
            posMultiplier = i == 2 ? 1.0 : 2.0;
            points->push_back(osg::Vec3d((xMax / 3.0) * posMultiplier, yMax - 20, 0.0));
            points->push_back(osg::Vec3d(((xMax / 3.0)  * posMultiplier) + 10.0, yMax - 20, 0.0));
            points->push_back(osg::Vec3d(((xMax / 3.0)  * posMultiplier) + 10.0, 10.0, 0.0));
            points->push_back(osg::Vec3d((xMax / 3.0) * posMultiplier, 10.0, 0.0));
        }


        segment->setVertexArray(points);
    }


    m_boardTransform->setPosition(camera->getInverseViewMatrix().getTrans());
}

void GraphicsThread::createGameStats()
{

}

void GraphicsThread::updateGameStats()
{

}

void GraphicsThread::updateGamePieces()
{

}
