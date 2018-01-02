#include <hubDB/DBException.h>
#include <hubDB/DBServer.h>

using namespace HubDB::Server;
using namespace HubDB::Exception;

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <hubDB/DBMyIndex.h>

bool isNodeIntValuesEquals(NodeKey *nodeValues, int numberOfElements, initializer_list<int> expectedValues);

bool isNodeTIDValuesEquals(NodeValue *tidActual, int numberOfTids, initializer_list<TID> expected);

using namespace log4cxx;
using namespace log4cxx::helpers;

DBIntType createIntValue(int value) {
    return DBIntType(value);
}

TID createTID(BlockNo blockNo, uint slot) {
    TID tid;
    tid.page = blockNo;
    tid.slot = slot;
    return tid;
}

void testIndexInitialization(const std::string &indexName) {
    LOG4CXX_INFO(logger, "");
    //setup
    LoggerPtr logger(Logger::getLogger("HubDB.TestIndex.initializationtest"));

    HubDB::Index::MetaBlockView metaBlockView;

    DBBufferMgr &bufMgr = *(DBBufferMgr *) getClassForName("DBRandomBufferMgr", 1, true);
    const char *fileName = "file1.indexInitializationTest";
    try {
        bufMgr.dropFile(fileName);
    } catch (DBException e) {
        LOG4CXX_INFO(logger, "Setup hat versucht ein nicht existierendes File zu loeschen - Alles gut!!!");
    }

    bufMgr.createFile(fileName);
    DBFile &file1 = bufMgr.openFile(fileName);

    int indexIsUnique = 1;
    DBIndex &index = *(DBIndex *) getClassForName(indexName, 5, &bufMgr, &file1, AttrTypeEnum::INT, ModType::WRITE,
                                                  indexIsUnique);
    LOG4CXX_INFO(logger, "Begin testing...");
    //TEST----------------------------------------------------------------
    index.initializeIndex();
    {
        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        bufMgr.unfixBlock(metaBlock);
        bufMgr.flushBlock(metaBlock);
        bufMgr.closeFile(file1);
        bufMgr.openFile(fileName);
    }

    DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);

    if (!metaBlockView.isMetaBlock(metaBlock)) {
        LOG4CXX_WARN(logger, "angelegter Metablock wird nicht als Metablock angelegt");
        return;
    }
    if (metaBlockView.isNextFreeBlockExist(metaBlock)) {
        LOG4CXX_WARN(logger,
                     "In der Indexdatei gibt es keinen NextFreeBlock aber es das Programm gibt das Gegenteil aus!");
        return;
    }
    if (metaBlockView.getNextFreeBlockNo(metaBlock) != 0) {
        LOG4CXX_WARN(logger, "NextFreeBlock nicht richtig gesetzt! Der gesetzt Wert ist: "
                             + std::to_string(metaBlockView.getNextFreeBlockNo(metaBlock)));
        return;
    }
    if (metaBlockView.getRootNodeBlockNo(metaBlock) != 0) {
        LOG4CXX_WARN(logger, "RootNodeBlockNo ist nicht richtig gesetzt! Der gesetzt Wert ist: "
                             + std::to_string(metaBlockView.getRootNodeBlockNo(metaBlock)));
        return;
    }
    if (!metaBlockView.isBTreeEmpty(metaBlock)) {
        LOG4CXX_WARN(logger, "Baum ist nicht leer, der sollte das aber sein!");
        return;
    }

    bufMgr.unfixBlock(metaBlock);

    //CLEANUP-------------------------------------------------------------
    bufMgr.closeFile(file1);
    bufMgr.dropFile(fileName);
    delete &index;
    delete &bufMgr;
    LOG4CXX_INFO(logger, "Test finished succesfull");
}

void testInsertIntValuesWithMaxElementsPerNode_3(const std::string &indexName) {
    if (maxKeysPerNodeInTestMode != 3 || isInTestMode == false) {
        throw DBException("maxKeysPerNodeInTestMode auf 3 setzen! oder isInTestMode auf true setzen!");
    }

    LOG4CXX_INFO(logger, "");
    //setup
    LoggerPtr logger(Logger::getLogger("HubDB.TestIndex.initializationtest"));

    HubDB::Index::MetaBlockView metaBlockView;
    HubDB::Index::NodeBlockView nodeBlockView(AttrTypeEnum::INT);

    DBBufferMgr &bufMgr = *(DBBufferMgr *) getClassForName("DBRandomBufferMgr", 1, true);
    const char *fileName = "file1.indexInitializationTest";
    try {
        bufMgr.dropFile(fileName);
    } catch (DBException e) {
        LOG4CXX_INFO(logger, "Setup hat versucht ein nicht existierendes File zu loeschen - Alles gut!!!");
    }

    bufMgr.createFile(fileName);
    DBFile &file1 = bufMgr.openFile(fileName);

    int indexIsUnique = 1;
    DBMyIndex &index = *(DBMyIndex *) getClassForName(indexName, 5, &bufMgr, &file1, AttrTypeEnum::INT, ModType::WRITE,
                                                      indexIsUnique);
    index.initializeIndex();
    LOG4CXX_INFO(logger, "Begin testing...");
    //TEST-1----------------------------------------------------------------
    {
        LOG4CXX_INFO(logger, "Test1");
        index.insert(createIntValue(1), createTID(2, 1));
        bufMgr.closeFile(file1);
        file1 = bufMgr.openFile(fileName);

        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        if (metaBlockView.getRootNodeBlockNo(metaBlock) == 0) {
            LOG4CXX_WARN(logger, "RootNodeBlockNo ist nicht richtig gesetzt! Der gesetzt Wert ist: "
                                 + std::to_string(metaBlockView.getRootNodeBlockNo(metaBlock)));
            return;
        }

        BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);

        DBBACB rootNodeBlock = bufMgr.fixBlock(file1, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);
        if (metaBlockView.isBTreeEmpty(metaBlock)) {
            LOG4CXX_WARN(logger, "Baum ist leer, der sollte das aber nicht sein!");
            return;
        }
        if (!nodeBlockView.isLeafBlock(rootNodeBlock)) {
            LOG4CXX_WARN(logger, "angelegter RootNode wird nicht als Leaf erkannt!");
            return;
        }
        if ((*nodeBlockView.getKeyArraySorted(rootNodeBlock)).intValue != 1) {
            LOG4CXX_WARN(logger,
                         "Es wurde der erste Wert 1 hinzugefuegt aber dieser ist nicht in dem Knoten zu finden!");
            return;
        }
        if ((*nodeBlockView.getValueArrayAlignedToKeyArray(rootNodeBlock)).tid.page != 2 &&
            (*nodeBlockView.getValueArrayAlignedToKeyArray(rootNodeBlock)).tid.slot != 1) {
            LOG4CXX_WARN(logger, "TID mit BlockNo 2 und slot 1 wurde nicht gesetzt!");
            return;
        }

        DBListTID tidsPositiveResult;
        index.find(createIntValue(1), tidsPositiveResult);
        DBListTID tidsNegativeResult;
        index.find(createIntValue(33), tidsNegativeResult);
        if (tidsNegativeResult.size() != 0) {
            LOG4CXX_WARN(logger, "Negative Suche hat nicht funktioniert");
            return;
        }
        if (tidsPositiveResult.front().page != 2 && tidsPositiveResult.front().slot != 1) {
            LOG4CXX_WARN(logger, "Positive Suche hat nicht funktioniert");
            return;
        }


//        bufMgr.unfixBlock(metaBlock);
//        bufMgr.unfixBlock(rootNodeBlock);
        bufMgr.closeFile(file1);
        bufMgr.dropFile(fileName);
        LOG4CXX_INFO(logger, "Test1 finished!");
    }

    //TEST-2--------------------------------------------------------------
    {
        LOG4CXX_INFO(logger, "Test2");
        bufMgr.createFile(fileName);
        file1 = bufMgr.openFile(fileName);

        index.initializeIndex();
        index.insert(createIntValue(1), createTID(2, 1));
        index.insert(createIntValue(2), createTID(3, 0));

        bufMgr.closeFile(file1);
        file1 = bufMgr.openFile(fileName);

        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);

        DBBACB rootNodeBlock = bufMgr.fixBlock(file1, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);
        if (!isNodeIntValuesEquals(nodeBlockView.getKeyArraySorted(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock), {1, 2})) {
            LOG4CXX_WARN(logger, "NodeKey ist nicht gleich dem erwartetem NodeKey!");
            return;
        }
        if (!isNodeTIDValuesEquals(nodeBlockView.getValueArrayAlignedToKeyArray(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock),
                                   {createTID(2, 1), createTID(3, 0)})) {
            LOG4CXX_WARN(logger, "NodeValue-Leaf ist nicht gleich dem erwartetem NodeValue-Leaf!");
            return;
        }

        DBListTID tidsPositiveResult1;
        index.find(createIntValue(1), tidsPositiveResult1);
        DBListTID tidsPositiveResult2;
        index.find(createIntValue(2), tidsPositiveResult2);
        if (tidsPositiveResult1.front().page != 2 && tidsPositiveResult1.front().slot != 1) {
            LOG4CXX_WARN(logger, "Positive Suche hat nicht funktioniert");
            return;
        }
        if (tidsPositiveResult2.front().page != 3 && tidsPositiveResult2.front().slot != 0) {
            LOG4CXX_WARN(logger, "Positive Suche hat nicht funktioniert");
            return;
        }
//
//        bufMgr.unfixBlock(metaBlock);
//        bufMgr.unfixBlock(rootNodeBlock);
        bufMgr.closeFile(file1);
        bufMgr.dropFile(fileName);
        LOG4CXX_INFO(logger, "Test2 finished!");
    }

    //TEST-3--------------------------------------------------------------
    {
        LOG4CXX_INFO(logger, "Test3");
        bufMgr.createFile(fileName);
        file1 = bufMgr.openFile(fileName);

        index.initializeIndex();
        index.insert(createIntValue(2), createTID(3, 1));
        index.insert(createIntValue(1), createTID(2, 0));

        bufMgr.closeFile(file1);
        file1 = bufMgr.openFile(fileName);

        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);

        DBBACB rootNodeBlock = bufMgr.fixBlock(file1, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);
        if (!isNodeIntValuesEquals(nodeBlockView.getKeyArraySorted(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock), {1, 2})) {
            LOG4CXX_WARN(logger, "NodeKey ist nicht gleich dem erwartetem NodeKey!");
            return;
        }
        if (!isNodeTIDValuesEquals(nodeBlockView.getValueArrayAlignedToKeyArray(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock),
                                   {createTID(2, 0), createTID(3, 1)})) {
            LOG4CXX_WARN(logger, "NodeValue-Leaf ist nicht gleich dem erwartetem NodeValue-Leaf!");
            return;
        }


        bufMgr.unfixBlock(metaBlock);
        bufMgr.unfixBlock(rootNodeBlock);
        bufMgr.closeFile(file1);
        bufMgr.dropFile(fileName);
        LOG4CXX_INFO(logger, "Test3 finished!");
    }

    //TEST-4--------------------------------------------------------------
    {
        LOG4CXX_INFO(logger, "Test4");
        bufMgr.createFile(fileName);
        file1 = bufMgr.openFile(fileName);

        index.initializeIndex();
        index.insert(createIntValue(2), createTID(3, 1));
        index.insert(createIntValue(1), createTID(2, 0));
        index.insert(createIntValue(4), createTID(5, 4));

        bufMgr.closeFile(file1);
        file1 = bufMgr.openFile(fileName);

        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);

        DBBACB rootNodeBlock = bufMgr.fixBlock(file1, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);
        if (!isNodeIntValuesEquals(nodeBlockView.getKeyArraySorted(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock), {1, 2, 4})) {
            LOG4CXX_WARN(logger, "NodeKey ist nicht gleich dem erwartetem NodeKey!");
            return;
        }
        if (!isNodeTIDValuesEquals(nodeBlockView.getValueArrayAlignedToKeyArray(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock),
                                   {createTID(2, 0), createTID(3, 1), createTID(5, 4)})) {
            LOG4CXX_WARN(logger, "NodeValue-Leaf ist nicht gleich dem erwartetem NodeValue-Leaf!");
            return;
        }

        bufMgr.unfixBlock(metaBlock);
        bufMgr.unfixBlock(rootNodeBlock);
        bufMgr.closeFile(file1);
        bufMgr.dropFile(fileName);
        LOG4CXX_INFO(logger, "Test4 finished!");
    }

    //TEST-5-Split-------------------------------------------------------------
    {
        LOG4CXX_INFO(logger, "Test5");
        bufMgr.createFile(fileName);
        file1 = bufMgr.openFile(fileName);

        index.initializeIndex();
        index.insert(createIntValue(2), createTID(3, 1));
        index.insert(createIntValue(1), createTID(2, 0));
        index.insert(createIntValue(4), createTID(5, 4));
        index.insert(createIntValue(3), createTID(3, 4));

        bufMgr.closeFile(file1);
        file1 = bufMgr.openFile(fileName);

        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);

        DBBACB rootNodeBlock = bufMgr.fixBlock(file1, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);
        if (!isNodeIntValuesEquals(nodeBlockView.getKeyArraySorted(rootNodeBlock),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rootNodeBlock), {3})) {
            LOG4CXX_WARN(logger, "NodeKey-Root ist nicht gleich dem erwartetem NodeKey-Root!");
            return;
        }
        if (!nodeBlockView.isRootNode(rootNodeBlock)) {
            LOG4CXX_WARN(logger, "angelegter RootNode wird nicht als Root erkannt!");
            return;
        }
        if (!nodeBlockView.isInnerBlock(rootNodeBlock)) {
            LOG4CXX_WARN(logger, "angelegter RootNode wird nicht als InnerBlock erkannt!");
            return;
        }

        vector<BlockNo> childrenBlockNo = nodeBlockView.getChildrenBlockNo(rootNodeBlock);
        DBBACB leftChild = bufMgr.fixBlock(file1, childrenBlockNo[0], DBBCBLockMode::LOCK_SHARED);
        DBBACB rightChild = bufMgr.fixBlock(file1, childrenBlockNo[1], DBBCBLockMode::LOCK_SHARED);

        if (!isNodeIntValuesEquals(nodeBlockView.getKeyArraySorted(leftChild),
                                   nodeBlockView.getNumberOfKeysExistingInNode(leftChild), {1, 2})) {
            LOG4CXX_WARN(logger, "NodeKey-Root ist nicht gleich dem erwartetem NodeKey-Root!");
            return;
        }
        if (!isNodeTIDValuesEquals(nodeBlockView.getValueArrayAlignedToKeyArray(leftChild),
                                   nodeBlockView.getNumberOfKeysExistingInNode(leftChild),
                                   {createTID(2, 0), createTID(3, 1)})) {
            LOG4CXX_WARN(logger, "NodeValue-Leaf ist nicht gleich dem erwartetem NodeValue-Leaf!");
            return;
        }
        if (!isNodeIntValuesEquals(nodeBlockView.getKeyArraySorted(rightChild),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rightChild), {3, 4})) {
            LOG4CXX_WARN(logger, "NodeKey-Root ist nicht gleich dem erwartetem NodeKey-Root!");
            return;
        }
        if (!isNodeTIDValuesEquals(nodeBlockView.getValueArrayAlignedToKeyArray(rightChild),
                                   nodeBlockView.getNumberOfKeysExistingInNode(rightChild),
                                   {createTID(3, 4), createTID(5, 4)})) {
            LOG4CXX_WARN(logger, "NodeValue-Leaf ist nicht gleich dem erwartetem NodeValue-Leaf!");
            return;
        }

        DBListTID tidsPositiveResult1;
        index.find(createIntValue(1), tidsPositiveResult1);
        DBListTID tidsPositiveResult2;
        index.find(createIntValue(2), tidsPositiveResult2);
        DBListTID tidsPositiveResult3;
        index.find(createIntValue(3), tidsPositiveResult3);
        if (tidsPositiveResult1.front().page != 2 && tidsPositiveResult1.front().slot != 0) {
            LOG4CXX_WARN(logger, "Positive Suche hat nicht funktioniert");
            return;
        }
        if (tidsPositiveResult2.front().page != 3 && tidsPositiveResult2.front().slot != 1) {
            LOG4CXX_WARN(logger, "Positive Suche hat nicht funktioniert");
            return;
        }
        if (tidsPositiveResult3.front().page != 3 && tidsPositiveResult3.front().slot != 4) {
            LOG4CXX_WARN(logger, "Positive Suche hat nicht funktioniert");
            return;
        }


        bufMgr.unfixBlock(leftChild);
        bufMgr.unfixBlock(rightChild);
        bufMgr.unfixBlock(metaBlock);
        bufMgr.unfixBlock(rootNodeBlock);
        bufMgr.closeFile(file1);
        bufMgr.dropFile(fileName);
        LOG4CXX_INFO(logger, "Test5 finished!");
    }


    //CLEANUP-------------------------------------------------------------
    delete &index;
    delete &bufMgr;
    LOG4CXX_INFO(logger, "Test finished succesfull");
}

bool isNodeTIDValuesEquals(NodeValue *tidActual, int numberOfTids, initializer_list<TID> expectedValues) {
    vector<TID> expected(expectedValues.begin(), expectedValues.end());

    for (int i = 0; i < numberOfTids; i++) {
        TID isValue = (tidActual + i)->tid;
        TID expectedValue = expected[i];
        if (!(isValue == expectedValue)) {
            return false;
        }
    }
    return true;
}

bool isNodeIntValuesEquals(NodeKey *nodeValues, int numberOfElements, initializer_list<int> expectedValues) {
    vector<int> expected(expectedValues.begin(), expectedValues.end());

    for (int i = 0; i < numberOfElements; i++) {
        int isValue = (nodeValues + i)->intValue;
        int expectedValue = expected[i];
        if (isValue != expectedValue) {
            return false;
        }
    }
    return true;
}


int main(int argc, char *argv[]) {
    int rc = EXIT_SUCCESS;
    try {
        File f("../log4c++.hubDB.properties.testIndex");
        PropertyConfigurator::configure(f);

        testIndexInitialization("DBMyIndex");
        testInsertIntValuesWithMaxElementsPerNode_3("DBMyIndex");
    } catch (Exception &) {
        rc = EXIT_FAILURE;
    }

    return rc;
}
