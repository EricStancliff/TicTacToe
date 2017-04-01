#pragma once

#include <functional>

#include <osg/ref_ptr>

#include <QThread>
#include <QReadWriteLock>

namespace osgViewer
{
    class CompositeViewer;
}

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

    void setOSGViewer(osgViewer::CompositeViewer* osgViewer) { m_osgViewer = osgViewer; };

    //post a function to execute later in Graphics Thread
    //be sure to check the scope of your variables in your lambdas!
    void addTask(std::function<void()> task);

protected:
    virtual void run();

    void createBoard();

    std::vector < std::function<void()>> m_tasks;
    
    osgViewer::CompositeViewer* m_osgViewer;

    QReadWriteLock m_taskRWLock;

    osg::ref_ptr<osg::Group> m_rootGroup;

    bool m_done;
};