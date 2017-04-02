#pragma once

#include <cinttypes>
#include <vector>
#include <string>

#include <QReadWriteLock>
#include <QObject>
#include <QThread>
#include <QTimer>

//this is the data struct we'll use to define a "move"
//x and y position, *should* be between 0 and 2
struct MoveStruct {

    MoveStruct() : xPos(0), yPos(0) {};
    MoveStruct(uint8_t x, uint8_t y) : xPos(x), yPos(y) {};
    MoveStruct(const MoveStruct& other) : xPos(other.xPos), yPos(other.yPos) {};
    MoveStruct(MoveStruct&& other) :  xPos(std::move(other.xPos)), yPos(std::move(other.yPos)) {};
    ~MoveStruct() {};

    uint8_t xPos;
    uint8_t yPos;

    //for our purposes, moves go sequentially from 0,0 to 2,2
    //left to right, top to bottom, 0,0 being top left
    inline bool operator< (const MoveStruct& rhs) const
    {
        if (this->yPos < rhs.yPos)
            return true;
        else if (this->yPos == rhs.yPos)
            if (this->xPos < rhs.xPos)
                return true;
        return false;
    }
    inline bool operator> (const MoveStruct& rhs) const
    {
        return (rhs < *this);
    }
    inline bool operator<=(const MoveStruct& rhs) const
    { 
        return !(*this > rhs);
    }
    inline bool operator>=(const MoveStruct& rhs) const
    { 
        return !(*this < rhs);
    }
    inline bool operator==(const MoveStruct& rhs) const
    {
        return (this->xPos == rhs.xPos && this->yPos == rhs.yPos);
    }

    //copy and move
    inline MoveStruct& operator=(const MoveStruct& other)
    {
        this->xPos = other.xPos;
        this->yPos = other.yPos;
        return *this;
    }

    inline MoveStruct& operator= (MoveStruct&& other)
    {
        this->xPos = std::move(other.xPos);
        this->yPos = std::move(other.yPos);
        return *this;
    }
};


//This class is designed to be accessed by multiple threads
//let's be responsible people

//note: performance *shouldn't* be an issue with accesses because they
//only happen on click, so QMutex will be fine, for convienience
//if we need to be faster, I'll include TBB.

class GameMoveManager : public QThread
{
    Q_OBJECT
public:
    GameMoveManager(QObject* parent = nullptr);
    virtual ~GameMoveManager();

    std::vector<MoveStruct> getAllCurrentMoves() const;
    
    //clears out all of the moves
    void clearGame();

    bool isCurrentlyUsersTurn() const { return m_currentlyUsersTurn; }

    //decides the next AI move, stores and retrns it
    MoveStruct makeNextAIMove();

    //stores a user made move, returns false if not successful, with error msg
    bool storeUserMadeMove(const MoveStruct& move, std::string& errorMsg);

signals:
    void moveStored(const MoveStruct&);
    void boardCleared();

protected slots:
    void timeout();

protected:

    QTimer m_timer;

    //we'll sort this, mostly for convienience, but also so we can
    //surf down in order and make the next "dumb" move until 
    //I make this thing smart enough to win
    std::vector<MoveStruct> m_currentMoves;

    bool m_currentlyUsersTurn;

    //if we do more reads than writes, this is a win
    mutable QReadWriteLock m_rwLock;
};