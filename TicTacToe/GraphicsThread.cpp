#include "GraphicsThread.h"

#include <osgViewer/CompositeViewer>
#include <osgViewer/GraphicsWindow>

#include <osgQt/GraphicsWindowQt>

#include <osg/PositionAttitudeTransform>

#include <QApplication>
#include <QGraphicsProxyWidget>
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
    m_rootGroup = new osg::Group;

    std::vector<osg::Camera*> cameras;
    m_osgViewer->getCameras(cameras);

    Q_ASSERT(cameras.size());

    if (cameras.size())
    {
        cameras[0]->addChild(m_rootGroup);
    }

    osg::Geode* pyramidGeode = new osg::Geode();
    osg::Geometry* pyramidGeometry = new osg::Geometry();

    //Associate the pyramid geometry with the pyramid geode
    //   Add the pyramid geode to the root node of the scene graph.

    pyramidGeode->addDrawable(pyramidGeometry);
    m_rootGroup->addChild(pyramidGeode);

    //Declare an array of vertices. Each vertex will be represented by
    //a triple -- an instances of the vec3 class. An instance of
    //osg::Vec3Array can be used to store these triples. Since
    //osg::Vec3Array is derived from the STL vector class, we can use the
    //push_back method to add array elements. Push back adds elements to
    //the end of the vector, thus the index of first element entered is
    //zero, the second entries index is 1, etc.
    //Using a right-handed coordinate system with 'z' up, array
    //elements zero..four below represent the 5 points required to create
    //a simple pyramid.

    osg::Vec3Array* pyramidVertices = new osg::Vec3Array;
    pyramidVertices->push_back(osg::Vec3(0, 0, 0)); // front left
    pyramidVertices->push_back(osg::Vec3(10, 0, 0)); // front right
    pyramidVertices->push_back(osg::Vec3(10, 10, 0)); // back right
    pyramidVertices->push_back(osg::Vec3(0, 10, 0)); // back left
    pyramidVertices->push_back(osg::Vec3(5, 5, 10)); // peak

        //Associate this set of vertices with the geometry associated with the
        //geode we added to the scene.

    pyramidGeometry->setVertexArray(pyramidVertices);

    //Next, create a primitive set and add it to the pyramid geometry.
    //Use the first four points of the pyramid to define the base using an
    //instance of the DrawElementsUint class. Again this class is derived
    //from the STL vector, so the push_back method will add elements in
    //sequential order. To ensure proper backface cullling, vertices
    //should be specified in counterclockwise order. The arguments for the
    //constructor are the enumerated type for the primitive
    //(same as the OpenGL primitive enumerated types), and the index in
    //the vertex array to start from.

    osg::DrawElementsUInt* pyramidBase =
        new osg::DrawElementsUInt(osg::PrimitiveSet::QUADS, 0);
    pyramidBase->push_back(3);
    pyramidBase->push_back(2);
    pyramidBase->push_back(1);
    pyramidBase->push_back(0);
    pyramidGeometry->addPrimitiveSet(pyramidBase);

    //Repeat the same for each of the four sides. Again, vertices are
    //specified in counter-clockwise order.

    osg::DrawElementsUInt* pyramidFaceOne =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    pyramidFaceOne->push_back(0);
    pyramidFaceOne->push_back(1);
    pyramidFaceOne->push_back(4);
    pyramidGeometry->addPrimitiveSet(pyramidFaceOne);

    osg::DrawElementsUInt* pyramidFaceTwo =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    pyramidFaceTwo->push_back(1);
    pyramidFaceTwo->push_back(2);
    pyramidFaceTwo->push_back(4);
    pyramidGeometry->addPrimitiveSet(pyramidFaceTwo);

    osg::DrawElementsUInt* pyramidFaceThree =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    pyramidFaceThree->push_back(2);
    pyramidFaceThree->push_back(3);
    pyramidFaceThree->push_back(4);
    pyramidGeometry->addPrimitiveSet(pyramidFaceThree);

    osg::DrawElementsUInt* pyramidFaceFour =
        new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    pyramidFaceFour->push_back(3);
    pyramidFaceFour->push_back(0);
    pyramidFaceFour->push_back(4);
    pyramidGeometry->addPrimitiveSet(pyramidFaceFour);

    //Declare and load an array of Vec4 elements to store colors.

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)); //index 0 red
    colors->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f)); //index 1 green
    colors->push_back(osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f)); //index 2 blue
    colors->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)); //index 3 white
    colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)); //index 4 red

        //The next step is to associate the array of colors with the geometry,
        //assign the color indices created above to the geometry and set the
        //binding mode to _PER_VERTEX.

    pyramidGeometry->setColorArray(colors);
    pyramidGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    //Now that we have created a geometry node and added it to the scene
    //we can reuse this geometry. For example, if we wanted to put a
    //second pyramid 15 units to the right of the first one, we could add
    //this geode as the child of a transform node in our scene graph.

    // Declare and initialize a transform node.
    osg::PositionAttitudeTransform* pyramidTwoXForm =
        new osg::PositionAttitudeTransform();

    // Use the 'addChild' method of the osg::Group class to
    // add the transform as a child of the root node and the
    // pyramid node as a child of the transform.

    m_rootGroup->addChild(pyramidTwoXForm);
    pyramidTwoXForm->addChild(pyramidGeode);

    // Declare and initialize a Vec3 instance to change the
    // position of the model in the scene

    osg::Vec3 pyramidTwoPosition(15, 0, 0);
    pyramidTwoXForm->setPosition(pyramidTwoPosition);

    // switch off lighting as we haven't assigned any normals.
    m_rootGroup->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
}

