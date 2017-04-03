#include "GraphicsThread.h"
#include "ClickEventHandler.h"
#include "TApp.h"
#include <OSGViewerWidget.h>

#include <osgViewer/CompositeViewer>
#include <osgViewer/GraphicsWindow>

#include <osgQt/GraphicsWindowQt>
#include <osgQt/QFontImplementation>


#include <osg/PositionAttitudeTransform>
#include <osg/LineSegment>
#include <osg/Texture2D>

#include <osgText/Text>

#include <osgDB/ReadFile>

#include <QApplication>
#include <QDir>
#include <QDebug>


GraphicsThread::GraphicsThread(QObject *parent) : QThread(parent),
    m_done(false),
    m_osgViewer(nullptr),
    m_threadsWaiting(false)
{
    connect(tApp->getGameManager(), &GameMoveManager::moveStored, this, &GraphicsThread::handleMoveStored);
    connect(tApp->getGameManager(), &GameMoveManager::boardCleared, this, &GraphicsThread::handleBoardCleared);
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

osg::Camera* GraphicsThread::getCamera() const
{
    if (!m_osgViewer)
        return nullptr;

    std::vector<osg::Camera*> cameras;
    m_osgViewer->getCameras(cameras);

    assert(cameras.size());  //why  would we have no cameras?

    if (cameras.size())
    {
        return cameras[0];
    }
}


void GraphicsThread::init()
{
    assert(m_osgViewer);
    if (!m_osgViewer)
        return;

    createBoard();
    createGameStats();

    std::vector<osgViewer::View*> views;
    m_osgViewer->getViews(views);
    for (auto&& view : views)
        view->addEventHandler(new ClickEventHandler);

    m_xFile.setFileName(QDir::cleanPath(QApplication::applicationDirPath() + QDir::separator() + ".." + QDir::separator() + ".." + QDir::separator() + 
        "TicTacToe" + QDir::separator() + "Resources" + QDir::separator() + "X_Icon.png"));
    if (!m_xFile.exists())
        qCritical() << "No Icons Found!!";
}

void GraphicsThread::handleMoveStored(const MoveStruct& move)
{
    m_currentMoves.push_back(move);
}

void GraphicsThread::handleBoardCleared()
{
    m_currentMoves.clear();
}

void GraphicsThread::run()
{
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
    auto camera = getCamera();
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
        color->push_back(osg::Vec4f(1.0f, 0.0f, 0.0f, 0.8f));

        segment->setColorArray(color, osg::Array::BIND_OVERALL);

        segment->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

        osg::StateSet* stateset = segment->getOrCreateStateSet();
        stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

        lineGeode->addDrawable(segment);
        m_boardLines.push_back(lineGeode);

        m_boardTransform->addChild(lineGeode);
    }

}

void GraphicsThread::updateBoard()
{
    auto camera = getCamera();
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
        //forcing the board in the back a bit so the text is on top
        if (i < 2)
        {
            posMultiplier = i == 0 ? 1.0 : 2.0;
            points->push_back(osg::Vec3d(20.0, (yMax / 3.0) * posMultiplier, -0.1));
            points->push_back(osg::Vec3d((xMax - 20.0), (yMax / 3.0) * posMultiplier, -0.1));
            points->push_back(osg::Vec3d((xMax - 20.0), ((yMax / 3.0)* posMultiplier) - 10.0 , -0.1));
            points->push_back(osg::Vec3d(20.0, ((yMax / 3.0)* posMultiplier) - 10.0, -0.1));
        }
        else
        {
            posMultiplier = i == 2 ? 1.0 : 2.0;
            points->push_back(osg::Vec3d((xMax / 3.0) * posMultiplier, yMax - 20, -0.1));
            points->push_back(osg::Vec3d(((xMax / 3.0)  * posMultiplier) + 10.0, yMax - 20, -0.1));
            points->push_back(osg::Vec3d(((xMax / 3.0)  * posMultiplier) + 10.0, 20.0, -0.1));
            points->push_back(osg::Vec3d((xMax / 3.0) * posMultiplier, 20.0, -0.1));
        }

        segment->setVertexArray(points);
    }

    m_boardTransform->setPosition(camera->getInverseViewMatrix().getTrans());
}

void GraphicsThread::createGameStats()
{
    assert(m_boardTransform);
    if (!m_boardTransform)
        return;

    osg::Geode* textGeode = new osg::Geode;

    m_gameStats = new osgText::Text;

    m_gameStats->setFont(new osgText::Font(new osgQt::QFontImplementation(QFont("Arial"))));
    m_gameStats->setColor(osg::Vec4(1.0f, 1.0f, 1.0f, 0.6f));
    m_gameStats->setCharacterSize(15.0f);

    textGeode->addDrawable(m_gameStats);

    m_boardTransform->addChild(textGeode);
}

void GraphicsThread::updateGameStats()
{
    if (!m_gameStats)
        return;

    m_gameStats->setText("Score: Player - 0 / Computer - 0");

    auto camera = getCamera();
    if (!camera)
        return;


    auto xMax = camera->getViewport()->width();
    auto yMax = camera->getViewport()->height();

    m_gameStats->setPosition(osg::Vec3d(20.0, yMax - 25.0, 0.0));

}

void GraphicsThread::updateGamePieces()
{
    if (!m_boardTransform.valid())
        return;

    auto camera = getCamera();
    if (!camera)
        return;

    auto xMax = camera->getViewport()->width();
    auto yMax = camera->getViewport()->height();


    int i = 0;
    for (auto&& move : m_currentMoves)
    {
        if (i >= m_displayedMoves.size())
        {
            osg::Geometry* geom = new osg::Geometry;

            osg::Vec2Array* texcoords = new osg::Vec2Array;
            texcoords->push_back(osg::Vec2f(0.0f, 1.0f));
            texcoords->push_back(osg::Vec2f(0.0f, 0.0f));
            texcoords->push_back(osg::Vec2f(1.0f, 0.0f));
            texcoords->push_back(osg::Vec2f(1.0f, 1.0f));
            geom->setTexCoordArray(0, texcoords);

            osg::Vec4Array* colors = new osg::Vec4Array;
            colors->push_back(osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f));
            geom->setColorArray(colors, osg::Array::BIND_OVERALL);

            geom->addPrimitiveSet(new osg::DrawArrays(GL_QUADS, 0, 4));

            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(geom);
            m_boardTransform->addChild(geode);

            osg::Texture2D* texture = new osg::Texture2D;
            texture->setDataVariance(osg::Object::DYNAMIC);
            texture->setImage(osgDB::readImageFile(m_xFile.fileName().toStdString()));

            osg::StateSet* stateset = geom->getOrCreateStateSet();
            stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
            stateset->setMode(GL_BLEND, osg::StateAttribute::ON);
            stateset->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

            m_displayedMoves.push_back(std::make_pair(texture, geom));

        }

        if (m_displayedMoves.size() > i)
        {
            auto texture = m_displayedMoves[i].first;
            auto geometry = m_displayedMoves[i].second;

            osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray());
            if (!vertices)
                vertices = new osg::Vec3Array;

            vertices->clear();

            //calc min/max positions of texture
            double texXMin = move.xPos == 0 ? 10.0 : move.xPos == 1 ? (xMax / 3) + 10.0 : ((xMax / 3) * 2) + 10.0;
            double texXMax = move.xPos == 0 ? (xMax / 3) - 10.0 : move.xPos == 1 ? ((xMax / 3) * 2) - 10.0 : xMax - 10.0;
            double texYMin = move.yPos == 0 ? ((yMax / 3) * 2) + 10.0 : move.yPos == 1 ? (yMax / 3) + 10.0 : 10.0;
            double texYMax = move.yPos == 0 ? yMax - 10.0 : move.yPos == 1 ? ((yMax / 3)*2) - 10.0 : (yMax / 3) - 10.0;

            vertices->push_back(osg::Vec3d(texXMin, texYMax, 0));
            vertices->push_back(osg::Vec3d(texXMax, texYMax, 0));
            vertices->push_back(osg::Vec3d(texXMax, texYMin, 0));
            vertices->push_back(osg::Vec3d(texXMin, texYMin, 0));

            geometry->setVertexArray(vertices);

        }
        ++i;
    }
}
