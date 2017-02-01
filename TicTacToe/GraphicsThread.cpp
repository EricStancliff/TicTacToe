#include "GraphicsThread.h"

#include <QApplication>
GraphicsThread::GraphicsThread(QObject *parent) : QThread(parent),
    m_done(false)
{

}

GraphicsThread::~GraphicsThread()
{

}

//not going to lock here.  How can you have a word tear on a bool? you can't!  
void GraphicsThread::setDone(bool done)
{
    m_done = done;
}

void GraphicsThread::addTask(std::function<void()> task)
{
    QWriteLocker lock(&m_taskRWLock);
    m_tasks.push_back(task);
}

void GraphicsThread::run()
{
    this->moveToThread(this);
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
            QReadLocker lock(&m_taskRWLock);
            hasTasks = !m_tasks.empty();
        }

        if (hasTasks)
        {

            std::vector<std::function<void()>> localTasks;
            //scope the lock
            {
                QWriteLocker lock(&m_taskRWLock);
                std::move(localTasks.begin(), localTasks.end(), std::back_inserter(m_tasks));
                m_tasks.clear();
            }

            //execute tasks
            for (auto&& task : localTasks)
                task();

        }

        //let Qt's event queue process
        QApplication::processEvents();

        sleep(0);
    }
}