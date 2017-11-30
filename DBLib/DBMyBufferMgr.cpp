

#include <hubDB/DBTypes.h>
#include <hubDB/DBBufferMgr.h>
#include <hubDB/DBMyBufferMgr.h>

using namespace HubDB::Manager;
using namespace HubDB::Exception;


LoggerPtr DBMyBufferMgr::logger(Logger::getLogger("HubDB.Buffer.DBMyBufferMgr"));
int myBMgr = DBMyBufferMgr::registerClass();

extern "C" void *createDBMyBufferMgr(int nArgs, va_list &ap);

DBMyBufferMgr::DBMyBufferMgr(bool doThreading, int bufferBlock)
        : DBBufferMgr(doThreading, bufferBlock), bcbList(NULL), unfixedFramesPositionList() {
    std::cout << "Hello MyBufferManager!" << std::endl;

    if (logger != NULL) LOG4CXX_INFO(logger, "DBMyBufferMgr()");

    bcbList = new DBBCB *[maxBlockCnt];
    for (int i = 0; i < maxBlockCnt; i++) {
        bcbList[i] = NULL;
    }

    unfixedFramesPositionList.reserve(maxBlockCnt);

    if (logger != NULL) LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));
}

DBMyBufferMgr::~DBMyBufferMgr() {
    LOG4CXX_INFO(logger, "~DBMyBufferMgr()");
    LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));
    if (bcbList != NULL) {
        for (int i = 0; i < maxBlockCnt; ++i) {
            if (bcbList[i] != NULL) {
                try {
                    flushBCBBlock(*bcbList[i]);
                } catch (DBException &e) {}
                delete bcbList[i];
            }
        }
        delete[] bcbList;
    }
}


string DBMyBufferMgr::toString(string linePrefix) const {
    stringstream ss;
    ss << linePrefix << "[DBMyBufferMgr]" << endl;
    ss << DBBufferMgr::toString(linePrefix + "\t");
    lock();

    ss << linePrefix << "unfixedPages( size: " << unfixedFramesPositionList.size() << " ):" << endl;

    for (int unfixedFramePosition : unfixedFramesPositionList) {
        ss << linePrefix << unfixedFramePosition << endl;
    }


    ss << linePrefix << "bcbList( size: " << maxBlockCnt << " ):" << endl;
    for (int i = 0; i < maxBlockCnt; ++i) {
        ss << linePrefix << "bcbList[" << i << "]:";
        if (bcbList[i] == NULL)
            ss << "NULL" << endl;
        else
            ss << endl << bcbList[i]->toString(linePrefix + "\t");
    }
    ss << linePrefix << "-------------------" << endl;
    unlock();
    return ss.str();
}

int DBMyBufferMgr::registerClass() {
    setClassForName("DBMyBufferMgr", createDBMyBufferMgr);
    return 0;
}

DBBCB *DBMyBufferMgr::fixBlock(DBFile &file, BlockNo blockNo, DBBCBLockMode mode, bool read) {
    LOG4CXX_INFO(logger, "fixBlock()");
    LOG4CXX_DEBUG(logger, "file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger, "blockNo: " + TO_STR(blockNo));
    LOG4CXX_DEBUG(logger, "mode: " + DBBCB::LockMode2String(mode));
    LOG4CXX_DEBUG(logger, "read: " + TO_STR(read));
    LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));

    int framePositionOfBlock = findFramePostionsOfBlock(file, blockNo);

    LOG4CXX_DEBUG(logger, "framePositionOfBlock:" + TO_STR(framePositionOfBlock));

    if (framePositionOfBlock == NOT_FOUND_POSITION) {
        framePositionOfBlock = tryFindAppropriateReplacementPosition();

        if (bcbList[framePositionOfBlock] != NULL) {
            if (bcbList[framePositionOfBlock]->getDirty() == false)
                flushBCBBlock(*bcbList[framePositionOfBlock]);
            delete bcbList[framePositionOfBlock];
        }

        bcbList[framePositionOfBlock] = new DBBCB(file, blockNo);
        if (read == true) {
            fileMgr.readFileBlock(bcbList[framePositionOfBlock]->getFileBlock());
        }
    }

    DBBCB *rc = bcbList[framePositionOfBlock];
    if (rc->grantAccess(mode) == false) {
        rc = NULL;
        removeFixedFrameFromUnfixedFramePositionList(framePositionOfBlock);
        unfixedFramesPositionList.push_back(framePositionOfBlock);
    } else {
        removeFixedFrameFromUnfixedFramePositionList(framePositionOfBlock);
    }

    LOG4CXX_DEBUG(logger, "rc: " + TO_STR(rc));
    return rc;
}

int DBMyBufferMgr::tryFindAppropriateReplacementPosition() const {
    int appropriateReplacementPosition = NOT_FOUND_POSITION;

    for (int i = 0; i < maxBlockCnt; i++) {
        bool foundEmptyPosition = (bcbList[i] == NULL);
        if (foundEmptyPosition) {
            return i;
        }
    }

    if (appropriateReplacementPosition == NOT_FOUND_POSITION) {
        bool unfixedFrameExist = unfixedFramesPositionList.size() > 0;
        if (unfixedFrameExist) {
            appropriateReplacementPosition = unfixedFramesPositionList[0];
        } else {
            throw DBBufferMgrException("no more free pages");
        }
    }
    return appropriateReplacementPosition;
}

void DBMyBufferMgr::removeFixedFrameFromUnfixedFramePositionList(int framePositionOfFixedBlock) {
    for (int i = 0; i < unfixedFramesPositionList.size(); i++) {
        if (unfixedFramesPositionList[i] == framePositionOfFixedBlock) {
            unfixedFramesPositionList.erase(unfixedFramesPositionList.begin() + i);
            break;
        }
    }
}

int DBMyBufferMgr::findFramePostionsOfBlock(DBFile &file, BlockNo blockNo) {
    LOG4CXX_INFO(logger, "findFramePostionsOfBlock()");
    int pos = NOT_FOUND_POSITION;
    for (int i = 0; pos == NOT_FOUND_POSITION && i < maxBlockCnt; ++i) {
        if (bcbList[i] != NULL &&
            bcbList[i]->getFileBlock() == file &&
            bcbList[i]->getFileBlock().getBlockNo() == blockNo)
            pos = i;
    }
    LOG4CXX_DEBUG(logger, "pos: " + TO_STR(pos));
    return pos;
}

void DBMyBufferMgr::unfixBlock(DBBCB &bcb) {
    LOG4CXX_INFO(logger, "unfixBlock()");
    LOG4CXX_DEBUG(logger, "bcb:\n" + bcb.toString("\t"));
    LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));

    bcb.unlock();
    int framePositionOfBlock = findFramePostionsOfBlock(&bcb);
    if (bcb.getDirty() == true) {
        delete bcbList[framePositionOfBlock];
        bcbList[framePositionOfBlock] = NULL;
    } else if (bcb.isUnlocked() == true) {
        unfixedFramesPositionList.push_back(framePositionOfBlock);
    }
}

int DBMyBufferMgr::findFramePostionsOfBlock(DBBCB *bcb) {
    LOG4CXX_INFO(logger, "findBlock()");
    int pos = -1;
    for (int i = 0; pos == -1 && i < maxBlockCnt; ++i) {
        if (bcbList[i] == bcb)
            pos = i;
    }
    LOG4CXX_DEBUG(logger, "pos: " + TO_STR(pos));
    return pos;
}

bool DBMyBufferMgr::isBlockOfFileOpen(DBFile &file) const {
    LOG4CXX_INFO(logger, "isBlockOfFileOpen()");
    LOG4CXX_DEBUG(logger, "file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));
    for (int i = 0; i < maxBlockCnt; ++i) {
        if (bcbList[i] != NULL && bcbList[i]->getFileBlock() == file) {
            LOG4CXX_DEBUG(logger, "rc: true");
            return true;
        }
    }
    LOG4CXX_DEBUG(logger, "rc: false");
    return false;
}

void DBMyBufferMgr::closeAllOpenBlocks(DBFile &file) {
    LOG4CXX_INFO(logger, "closeAllOpenBlocks()");
    LOG4CXX_DEBUG(logger, "file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));
    for (int i = 0; i < maxBlockCnt; ++i) {
        if (bcbList[i] != NULL && bcbList[i]->getFileBlock() == file) {
            if (bcbList[i]->isUnlocked() == false)
                throw DBBufferMgrException("can not close fileblock because it is still lock");
            flushBCBBlock(*bcbList[i]);
            delete bcbList[i];
            bcbList[i] = NULL;
            removeFixedFrameFromUnfixedFramePositionList(i);
        }
    }
}


extern "C" void *createDBMyBufferMgr(int nArgs, va_list &ap) {
    DBMyBufferMgr *b = NULL;
    bool t;
    uint c;
    switch (nArgs) {
        case 1:
            t = va_arg(ap, int);
            b = new DBMyBufferMgr(t);
            break;
        case 2:
            t = va_arg(ap, int);
            c = va_arg(ap, int);
            b = new DBMyBufferMgr(t, c);
            break;
        default:
            throw DBException("Invalid number of arguments");
    }
    return b;
}






