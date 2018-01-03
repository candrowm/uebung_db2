#include <hubDB/DBException.h>
#include <hubDB/DBMyIndex2.h>
#include <math.h>

using namespace HubDB::Index;
using namespace HubDB::Exception;
using namespace std;

LoggerPtr DBMyIndex2::logger(Logger::getLogger("HubDB.Index.DBMyIndex2"));

DBMyIndex2::DBMyIndex2(DBBufferMgr &bufferMgr, DBFile &file, AttrTypeEnum attrType, ModType mode, bool unique) : DBIndex(
        bufferMgr, file, attrType, mode, unique), nodeBlockView(attrType) {
}


void DBMyIndex2::initializeIndex() {
    if (bufMgr.getBlockCnt(file) != 0) {
        throw DBIndexException("can not initializie exisiting table");
    }

    DBBACB firstBlock = bufMgr.fixNewBlock(file);
    metaBlockView.initializeIndex(firstBlock);
    firstBlock.setModified();
    bufMgr.unfixBlock(firstBlock);
    bufMgr.flushBlock(firstBlock);
}

void DBMyIndex2::find(const DBAttrType &val, DBListTID &tids) {
    DBBACB metaBlock = bufMgr.fixBlock(file, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
    if (metaBlockView.isBTreeEmpty(metaBlock)) {
        return;
    }

    BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);
    DBBACB rootNode = bufMgr.fixBlock(file, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);

    DBBACB &nodeLaufvariable = rootNode;
//    while(!nodeBlockView.isLeafBlock(nodeLaufvariable)) {
//        BlockNo childNodeBlockNo = nodeBlockView.getChildNodeBlockNoForValue(nodeLaufvariable, val);
//        DBBACB childNode = bufMgr.fixBlock(file, childNodeBlockNo, DBBCBLockMode::LOCK_FREE);
//        bufMgr.unfixBlock(nodeLaufvariable);
//        nodeLaufvariable = childNode;
//    }

    if (nodeBlockView.containsValue(nodeLaufvariable, val)) {
        tids.push_back(nodeBlockView.getTIDFor(nodeLaufvariable, val));
    }

    bufMgr.unfixBlock(nodeLaufvariable);
    bufMgr.unfixBlock(metaBlock);
}

void DBMyIndex2::insert(const DBAttrType &val, const TID &tid) {
    DBBACB metaBlock = bufMgr.fixBlock(file, metaBlockNo, DBBCBLockMode::LOCK_EXCLUSIVE);

    DBBACB leafNodeForInsert = findLeafForInsert(val, metaBlock);


    int numberOfExistingKeys = nodeBlockView.getNumberOfKeysExistingInNode(leafNodeForInsert);
    if (numberOfExistingKeys == nodeBlockView.getMaxKeysPerNode(leafNodeForInsert)) {
        //TODO SPLIT

//        NodeValuesAsVectors overflowedNode = nodeBlockView.getOverflowedNode(leafNodeForInsert, val, tid);
//        unsigned long sizeOfOverflowedNode = overflowedNode.keysVector.size();
//        {
//            if (sizeOfOverflowedNode != nodeBlockView.getMaxKeysPerNode(leafNodeForInsert) + 1) {
//                throw DBException("Im Knoten wurde der Overflow nicht richtig erreicht!");
//            }
//        }
//
//        //int splitPosition = (int)ceil((float)sizeOfOverflowedNode/2.0);
//        int splitPosition = (int)((float)sizeOfOverflowedNode/2.0);
//
//        DBBACB newRightChild = bufMgr.fixNewBlock(file);
//        if (nodeBlockView.isRootNode(leafNodeForInsert)) {
//            DBBACB newRootNode = bufMgr.fixNewBlock(file);
//
//            metaBlockView.setRootNodeBlockNo(metaBlock, newRootNode.getBlockNo());
//            //TODO hier weiter....
//        } else {
//
//        }
    } else {
        nodeBlockView.addValueToLeafNodeWithoutSplit(leafNodeForInsert, val, tid);
    }
    bufMgr.unfixBlock(leafNodeForInsert);
    bufMgr.unfixBlock(metaBlock);
    bufMgr.flushBlock(leafNodeForInsert);
    bufMgr.flushBlock(metaBlock);
}

DBBACB DBMyIndex2::findLeafForInsert(const DBAttrType &val, DBBACB &metaBlock) {
    DBBACB rootBlock = getRootBlockExclusively(metaBlock);

    DBBACB *nodeLaufvariable = &rootBlock;
    while (!nodeBlockView.isLeafBlock(*nodeLaufvariable)) {
        BlockNo childNodeBlockNo = nodeBlockView.getChildNodeBlockNoForValue(*nodeLaufvariable, val);
        DBBACB childNode = bufMgr.fixBlock(file, childNodeBlockNo, DBBCBLockMode::LOCK_EXCLUSIVE);
        bufMgr.unfixBlock(*nodeLaufvariable);
        nodeLaufvariable = &childNode;
    }

    if (nodeBlockView.containsValue(*nodeLaufvariable, val)) {
        throw DBException("Duplikat ist nicht zugelassen!");
    }
    return *nodeLaufvariable;
}


DBBACB DBMyIndex2::getRootBlockExclusively(DBBACB metaBlock) {
    if (metaBlockView.isBTreeEmpty(metaBlock)) {
        //TODO freien Block suchen

        DBBACB rootBlock = bufMgr.fixNewBlock(file);
        nodeBlockView.initializeRootBlock(rootBlock);
        metaBlockView.setRootNodeBlockNo(metaBlock, rootBlock.getBlockNo());
        return rootBlock;
    }
    return bufMgr.fixBlock(file, metaBlockView.getRootNodeBlockNo(metaBlock), DBBCBLockMode::LOCK_EXCLUSIVE);
}


void DBMyIndex2::remove(const DBAttrType &val, const DBListTID &tid) {

}

string DBMyIndex2::toString(string linePrefix) const {
    return DBIndex::toString(linePrefix);
}


/**
 * ab hier Set-Up
 */
// registerClass()-Methode am Ende dieser Datei: macht die Klasse der Factory bekannt
int rMyId2x = DBMyIndex2::registerClass();
// Funktion bekannt machen
extern "C" void *createDBMyIndex2(int nArgs, va_list &ap);


/**
 * Fuegt createDBMyIndex2 zur globalen factory method-map hinzu
 */
int DBMyIndex2::registerClass() {
    setClassForName("DBMyIndex2", createDBMyIndex2);
    return 0;
}















/**
 * Gerufen von HubDB::Types::getClassForName von DBTypes, um DBIndex zu erstellen
 * - DBBufferMgr *: Buffermanager
 * - DBFile *: Dateiobjekt
 * - attrType: Attributtp
 * - ModeType: READ, WRITE
 * - bool: unique Indexattribut
 */
extern "C" void *createDBMyIndex2(int nArgs, va_list &ap) {
    // Genau 5 Parameter
    if (nArgs != 5) {
        throw DBException("Invalid number of arguments:" + nArgs);
    }
    DBBufferMgr *bufMgr = va_arg(ap, DBBufferMgr *);
    DBFile *file = va_arg(ap, DBFile *);
    enum AttrTypeEnum attrType = (enum AttrTypeEnum) va_arg(ap, int);
    ModType m = (ModType) va_arg(ap, int);
    bool unique = (bool) va_arg(ap, int);
    return new DBMyIndex2(*bufMgr, *file, attrType, m, unique);
}

