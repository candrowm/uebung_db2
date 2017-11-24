//
// Created by martin on 19.11.17.
//

#include "hubDB/DBMyBufferMgr.h"
#include <hubDB/DBMyBufferMgr.h>
#include <hubDB/DBException.h>
#include <hubDB/DBMonitorMgr.h>
#include <chrono>


class BlockList {

    class BlockElement {
    public:
        BlockElement(int pos) {
            this->position = pos;
            this->successor = NULL;
        }

        int position;
        BlockElement *successor;
    };

    BlockElement *head;

    BlockElement *end;

public:
    BlockList(void) {
        head = end = NULL;
    }

    ~BlockList() {
    }

    void addElement(int pos) {
        BlockElement *neuesListenelement = new BlockElement(pos);

        if (isEmpty())
            end = head = neuesListenelement;
        else {
            end->successor = neuesListenelement;
            end = neuesListenelement;
        }
    }

    bool isEmpty() {
        return (head == NULL) ? true : false;
    }

    void checkAndAddElement(int pos){
       // if(!this->containsElement(pos)){
            addElement(pos);
       // }
    }


    void deleteList() {
        if (isEmpty())
            return;

        while (head->successor != NULL) {
            BlockElement *secondLastElement = head;
            while (secondLastElement->successor != end) {
                secondLastElement = secondLastElement->successor;
            }

            delete end;
            secondLastElement->successor = NULL;
            end = secondLastElement;
        }

        delete head;
    }

    int getAndRemoveFirstElement(){
        if(!this->isEmpty()){
            int returnValue = head->position;
            deleteElement(head->position);
            return returnValue;
        } else {
            return -1;
        }
    }

    void showAllElements(void) {
        BlockElement *p = head;

        while (p != NULL) {
            std::cout << "Position: " << p->position << std::endl;
            p = p->successor;
        }
        std::cout << "--------\n";
    }

    bool containsElement (int pos){
        BlockElement *p = head;

        while (p != NULL) {
            if(p->position==pos){
                return true;
            }
            p = p->successor;
        }
        return false;
    }

    void deleteElement(int pos) {
        BlockElement *p = head;
        BlockElement *predecessor = NULL;

        if (head->position == pos) {
            delete head;
            head = p->successor;
            return;
        } else {
            while (p != NULL) {
                predecessor = p;
                p = p->successor;
                if (p->position == pos) {
                    predecessor->successor=p->successor;
                    if(p->successor==NULL){
                        this->end=predecessor;
                    }
                    delete p;
                    return;
                }
            }
        }
    }
};


using namespace HubDB::Manager;
using namespace HubDB::Exception;

LoggerPtr DBMyBufferMgr::logger(Logger::getLogger("HubDB.Buffer.DBMyBufferMgr"));
int myBMgr = DBMyBufferMgr::registerClass();
BlockList *blockList;

extern "C" void *createDBMyBufferMgr(int nArgs, va_list &ap);



DBMyBufferMgr::DBMyBufferMgr(bool doThreading, int cnt) :
        DBBufferMgr(doThreading, cnt),
        bcbList(NULL) {
    if (logger != NULL) LOG4CXX_INFO(logger, "DBMyBufferMgr()");


    maxBlockCnt = 10;
    bcbList = new DBBCB *[maxBlockCnt];
    for (int i = 0; i < maxBlockCnt; i++) {
        bcbList[i] = NULL;
    }

    mapSize = cnt / 32 + 1;
    bitMap = new int[mapSize];

    blockList = new BlockList();
    for(int i = 0; i < maxBlockCnt; i++){
        blockList->addElement(i);
    }

    //std::chrono::time_point<std::chrono::system_clock> lastUsingArray [maxBlockCnt];
    //lastUsing=lastUsingArray;

    for (int i = 0; i < mapSize; ++i) {
        bitMap[i] = 0;
    }

    for (int i = 0; i < cnt; ++i) {
        setBit(i);
    }

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
        delete[] bitMap;
        blockList->deleteList();
        delete blockList;
    }
}

string DBMyBufferMgr::toString(string linePrefix) const {
    stringstream ss;
    ss << linePrefix << "[DBMyBufferMgr]" << endl;
    ss << DBBufferMgr::toString(linePrefix + "\t");
    lock();

    uint i, sum = 0;
    for (i = 0; i < maxBlockCnt; ++i) {
        if (getBit(i) == 1)
            ++sum;
    }
    ss << linePrefix << "unfixedPages( size: " << sum << " ):" << endl;
    for (i = 0; i < maxBlockCnt; ++i) {
        if (getBit(i) == 1)
            ss << linePrefix << i << endl;
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



    int i = findBlock(file, blockNo);

    LOG4CXX_DEBUG(logger, "i:" + TO_STR(i));

    //blockList->showAllElements();
    for(int i = 0;  i < maxBlockCnt; i++){
       // std::cout << "Position i: " << i << "-> Status: " << getBit(i) << "\n";
    }

    if(i>=0 && !blockList->isEmpty()){
       // std::cout << " Element in Liste bereits vorhanden -> Einfach nur loeschen! " << i <<  "\n";
        try{
            blockList->deleteElement(i);
        } catch (int e){

        }

      //  std::cout << " geklappt \n";
    }


    // Wenn der Block nicht im Buffer vorhanden ist --> Freien Platz suchen und laden
    if (i == -1) {

        int positionOfUnfixedBlock = blockList->getAndRemoveFirstElement();
        i=positionOfUnfixedBlock;
       // std::cout << i << " Position of new block " << "\n";

        if (i == -1)
            throw DBBufferMgrException("no more free pages");

        if (bcbList[i] != NULL) {
            // wenn Block nicht dirty ist --> Block muss im Speicher geschrieben werden, bevor er entfernt wird
            if (bcbList[i]->getDirty() == false)
                flushBCBBlock(*bcbList[i]);
            delete bcbList[i];
        }
        bcbList[i] = new DBBCB(file, blockNo);
        //lastUsing[i] = std::chrono::high_resolution_clock::now();
        // falls read == true --> Block aus dem Sekundärspeicher (in den FileBlock) laden?
        if (read == true)
            fileMgr.readFileBlock(bcbList[i]->getFileBlock());
    }

    DBBCB *rc = bcbList[i];
    if (rc->grantAccess(mode) == false) {
        rc = NULL;
    } else {
        // setzen auf 0, d.h. Block ist fixiert und darf nicht herausgenommen werden
        unsetBit(i);
    }

    LOG4CXX_DEBUG(logger, "rc: " + TO_STR(rc));
    return rc;
}

void DBMyBufferMgr::unfixBlock(DBBCB &bcb) {
    LOG4CXX_INFO(logger, "unfixBlock()");
    LOG4CXX_DEBUG(logger, "bcb:\n" + bcb.toString("\t"));
    LOG4CXX_DEBUG(logger, "this:\n" + toString("\t"));
    bcb.unlock();
    int i = findBlock(&bcb);
    if (bcb.getDirty() == true) {
        delete bcbList[i];
        bcbList[i] = NULL;
        setBit(i);
    } else if (bcb.isUnlocked() == true) {
        setBit(i);
    }
    blockList->addElement(i);
    //std::cout << "ADD: Position " << i << " hinzugefuegt \n";
   // blockList->showAllElements();
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
            setBit(i);
        }
    }
}

int DBMyBufferMgr::findBlock(DBFile &file, BlockNo blockNo) {
    LOG4CXX_INFO(logger, "findBlock()");
    int pos = -1;
    for (int i = 0; pos == -1 && i < maxBlockCnt; ++i) {
        if (bcbList[i] != NULL &&
            bcbList[i]->getFileBlock() == file &&
            bcbList[i]->getFileBlock().getBlockNo() == blockNo)
            pos = i;
    }
    LOG4CXX_DEBUG(logger, "pos: " + TO_STR(pos));
    return pos;
}

int DBMyBufferMgr::findBlock(DBBCB *bcb) {
    LOG4CXX_INFO(logger, "findBlock()");
    int pos = -1;
    for (int i = 0; pos == -1 && i < maxBlockCnt; ++i) {
        if (bcbList[i] == bcb)
            pos = i;
    }
    LOG4CXX_DEBUG(logger, "pos: " + TO_STR(pos));
    return pos;
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
