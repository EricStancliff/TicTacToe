#pragma once

#include "GameMoveManager.h"


#include <functional>
#include <condition_variable>
#include <assert.h>

#include <osg/ref_ptr>

#include <QThread>
#include <QReadWriteLock>
#include <QFile>

class OSGViewerWidget;

namespace osg
{
    class Group;
    class PositionAttitudeTransform;
    class Camera;
    class Texture2D;
    class Geometry;
    class Geode;
}

namespace osgText
{
    class Text;
}

namespace
{
    template <class T>
    inline T rescaleRange(T value, T oldMin, T oldMax, T newMin, T newMax)
    {
        return  ((((newMax - newMin) * (value - oldMin)) / (oldMax - oldMin)) + newMin);
    }

    //TEST
    void testRescaleRange()
    {
        assert(rescaleRange(.5, 0.0, 1.0, 0.0, 100.0) == 50.0);
        assert(rescaleRange(.25, 0.0, 1.0, 0.0, 100.0) == 25.0);
        assert(rescaleRange(3.0, 2.0, 4.0, 0.0, 2.0) == 1.0);
        assert(rescaleRange(50, 0, 100, 0, 10) == 5);
    }
}

//this class is a QThread because we need to tell the QOpenGLContext that we own it
//so we can render in our own thread.  This is only really important if you're doing a lot of work in the UI
//thread and you don't want it to impact your FPS, which probably won't impact us in this case.
class GraphicsThread : public QThread
{
    Q_OBJECT

public:
    GraphicsThread(QObject *parent = nullptr);
    virtual ~GraphicsThread();

    //tell me to stop!
    void setDone(bool done);

    void init();

    void setOSGViewer(OSGViewerWidget* osgViewer) {m_osgViewer = osgViewer; };

    //post a function to execute later in Graphics Thread
    //be sure to check the scope of your variables in your lambdas!
    void addTask(std::function<void()> task);

    //add a task and block until it's completion
    void addTaskBlocking(std::function<void()> task);

protected slots:
    void handleMoveStored(const MoveStruct& move);
    void handleBoardCleared();

protected:
    virtual void run();

    void createBoard();

    void updateBoard();

    void createGameStats();

    void updateGameStats();

    void updateGamePieces();

    osg::Camera* getCamera() const;

    std::vector < std::function<void()>> m_tasks;
    std::condition_variable m_blockingTaskComplete;
    std::mutex m_blockingTaskMutex;


    OSGViewerWidget* m_osgViewer;

    QReadWriteLock m_RWLock;

    osg::ref_ptr<osg::Group> m_rootGroup;

    std::vector<osg::ref_ptr<osg::Geode>> m_boardLines;
    osg::ref_ptr<osg::PositionAttitudeTransform> m_boardTransform;

    osg::ref_ptr<osgText::Text> m_gameStats;

    bool m_done;
    bool m_threadsWaiting;

    std::vector<MoveStruct> m_currentMoves;

    struct DisplayedMove
    {
        osg::ref_ptr<osg::Texture2D> texture;
        osg::ref_ptr<osg::Geometry> geometry;
        osg::ref_ptr<osg::Geode> geode;
    };

    std::vector<DisplayedMove> m_displayedMoves;

    QFile m_xFile;
    QFile m_oFile;
};