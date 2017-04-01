#pragma once

#include <functional>
#include <condition_variable>

#include <osg/ref_ptr>

#include <QThread>
#include <QReadWriteLock>

#include <OSGViewerWidget.h>

namespace osg
{
    class Group;
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

    std::vector < std::function<void()>> m_tasks;
    
    OSGViewerWidget* m_osgViewer;

    QReadWriteLock m_RWLock;

    osg::ref_ptr<osg::Group> m_rootGroup;

    std::condition_variable m_condition;
    std::mutex m_conditionMutex;

    bool m_done;
    bool m_threadsWaiting;
};