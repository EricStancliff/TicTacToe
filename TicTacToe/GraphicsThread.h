#pragma once

#include <functional>
#include <condition_variable>
#include <assert.h>

#include <osg/ref_ptr>

#include <QThread>
#include <QReadWriteLock>

#include <OSGViewerWidget.h>

namespace osg
{
    class Group;
    class PositionAttitudeTransform;
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

protected:
    virtual void run();

    void createBoard();

    void updateBoard();

    std::vector < std::function<void()>> m_tasks;
    
    OSGViewerWidget* m_osgViewer;

    QReadWriteLock m_RWLock;

    osg::ref_ptr<osg::Group> m_rootGroup;

    std::vector<osg::ref_ptr<osg::Geode>> m_boardLines;
    osg::ref_ptr<osg::PositionAttitudeTransform> m_boardTransform;

    std::condition_variable m_condition;
    std::mutex m_conditionMutex;

    bool m_done;
    bool m_threadsWaiting;
};