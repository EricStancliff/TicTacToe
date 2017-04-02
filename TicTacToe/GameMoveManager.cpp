#include "GameMoveManager.h"

GameMoveManager::GameMoveManager(QObject* parent) : QObject(parent)
{
    //boring
}

GameMoveManager::~GameMoveManager()
{

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
        emit moveStored(move);
        return true;
    }

    auto movePosItr = std::lower_bound(m_currentMoves.begin(), m_currentMoves.end(), move);
    if (movePosItr != m_currentMoves.end() && (*movePosItr) == move)
    {
        errorMsg = "Square already taken, pick again!";
        return false;
    }

    m_currentMoves.insert(movePosItr, move);
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
        emit moveStored(nextMove);
        return nextMove;
    }

    //my copy
    MoveStruct lastMove = m_currentMoves.back();
    ++lastMove.xPos;
    ++lastMove.yPos;
    emit moveStored(lastMove);
    return lastMove;
}

