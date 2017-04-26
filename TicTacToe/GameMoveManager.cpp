#include "GameMoveManager.h"

#include <QDebug>

GameMoveManager::GameMoveManager(QObject* parent) : QThread(parent), m_currentlyUsersTurn(true), m_playerWins(0), m_aiWins(0), m_catWins(0)
{
    connect(&m_timer, &QTimer::timeout, this, &GameMoveManager::timeout);
    m_timer.start(2000);

    //temp
    m_playerWins = 2;
    m_aiWins = 4;
    m_catWins = 666;
}

GameMoveManager::~GameMoveManager()
{

}

void GameMoveManager::timeout()
{
    if (!m_currentlyUsersTurn)
        makeNextAIMove();
}

std::vector<MoveStruct> GameMoveManager::getAllCurrentMoves() const
{
    QReadLocker lock(&m_rwLock);
    //this function returns a copy, and the locker stays in scope
    //until the copy is made.
    return m_currentMoves;
}

void GameMoveManager::clearGame()
{
    QWriteLocker lock(&m_rwLock);
    m_currentMoves.clear();
    emit boardCleared();
}

bool GameMoveManager::storeUserMadeMove(const MoveStruct& move, std::string& errorMsg)
{
    if (!m_currentlyUsersTurn)
    {
        errorMsg = "Not your turn!";
        return false;
    }
    //quick bail error check
    if (move.xPos > 2 || move.yPos > 2)
    {
        errorMsg = "Not a valid move!";
        return false;
    }

    //if this happened all the time, we could do a read lock, check for error,
    //then write lock to store.  But it doesn't.  So there.
    QWriteLocker lock(&m_rwLock);

    //first move
    if (m_currentMoves.empty())
    {
        m_currentMoves.push_back(move);
        m_currentlyUsersTurn = false;
        emit moveStored(move);
        return true;
    }

    auto movePosItr = std::lower_bound(m_currentMoves.begin(), m_currentMoves.end(), move);
    if (movePosItr != m_currentMoves.end() && ( (*movePosItr).xPos == move.xPos && (*movePosItr).yPos == move.yPos ))
    {
        errorMsg = "Square already taken, pick again!";
        return false;
    }

    m_currentMoves.insert(movePosItr, move);
    m_currentlyUsersTurn = false;
    emit moveStored(move);
    return true;
}

//"AI"... haha
MoveStruct GameMoveManager::makeNextAIMove()
{
    QWriteLocker lock(&m_rwLock);

    //for now, we'll just take the next square.
    //TODO - make this less dumb.
    if (m_currentMoves.empty())
    {
        MoveStruct nextMove;
        m_currentMoves.push_back(nextMove);
        m_currentlyUsersTurn = true;
        emit moveStored(nextMove);
        return nextMove;
    }

    MoveStruct nextMove;
    bool moveFound = false;
    for (int x = 0; x <= 2; ++x)
    {
        for (int y = 0; y <= 2; ++y)
        {
            if (!moveFound)
            {
                nextMove.xPos = x;
                nextMove.yPos = y;
                auto movePosItr = std::lower_bound(m_currentMoves.begin(), m_currentMoves.end(), nextMove);
                if (movePosItr == m_currentMoves.end() || ((*movePosItr).xPos != nextMove.xPos && (*movePosItr).yPos != nextMove.yPos))
                {
                    moveFound = true;
                }
            }
        }
    }

    if (!moveFound)
    {
        //todo, find winner
        emit scoreUpdated(m_playerWins, m_aiWins, m_catWins);
        qWarning() << "Game Over!";
        m_currentMoves.clear();
        emit boardCleared();
        return MoveStruct();
    }

    auto movePosItr = std::lower_bound(m_currentMoves.begin(), m_currentMoves.end(), nextMove);
    m_currentMoves.insert(movePosItr, nextMove);
    nextMove.userMadeMove = false;
    m_currentlyUsersTurn = true;
    emit moveStored(nextMove);
    return nextMove;
}

