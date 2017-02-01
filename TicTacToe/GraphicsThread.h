#pragma once

#include <functional>

#include <QThread>
#include <QReadWriteLock>

class GraphicsThread : public QThread
{
    Q_OBJECT

public:
    GraphicsThread(QObject *parent = nullptr);
    virtual ~GraphicsThread();

    //tell me to stop!
    void setDone(bool done);

    //post a function to execute later in Graphics Thread
    //be sure to check the scope of your variables in your lambdas!
    void addTask(std::function<void()> task);

protected:
    virtual void run();

    std::vector < std::function<void()>> m_tasks;

    QReadWriteLock m_taskRWLock;

    bool m_done;
};