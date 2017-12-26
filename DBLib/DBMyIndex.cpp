#include <hubDB/DBException.h>
#include <hubDB/DBMyIndex.h>

using namespace HubDB::Index;
using namespace HubDB::Exception;
using namespace std;

LoggerPtr DBMyIndex::logger(Logger::getLogger("HubDB.Index.DBMyIndex"));

DBMyIndex::DBMyIndex(DBBufferMgr &bufferMgr, DBFile &file, AttrTypeEnum attrType, ModType mode, bool unique) : DBIndex(
        bufferMgr, file, attrType, mode, unique), nodeBlockView(attrType) {
}

void DBMyIndex::initializeIndex() {
    if (bufMgr.getBlockCnt(file) != 0) {
        throw DBIndexException("can not initializie exisiting table");
    }

    DBBACB firstBlock = bufMgr.fixNewBlock(file);
    metaBlockView.initializeIndex(firstBlock);
    //firstBlock.setModified();
    bufMgr.unfixBlock(firstBlock);
}

void DBMyIndex::find(const DBAttrType &val, DBListTID &tids) {
    DBBACB metaBlock = bufMgr.fixBlock(file, rootBlockNo, DBBCBLockMode::LOCK_SHARED);
    if (metaBlockView.isBTreeEmpty(metaBlock)) {
        return;
    }

    BlockNo rootNodeBlockNo = metaBlockView.getRootNodeBlockNo(metaBlock);
    DBBACB rootNode = bufMgr.fixBlock(file, rootNodeBlockNo, DBBCBLockMode::LOCK_SHARED);

//    DBBACB& nodeLaufvariable = rootNode;
//    while(!nodeBlockView.isLeafBlock(nodeLaufvariable)) {
//        BlockNo childNodeBlockNo = nodeBlockView.getChildNodeBlockNoForValue(nodeLaufvariable, val);
//        DBBACB childNode = bufMgr.fixBlock(file, childNodeBlockNo, DBBCBLockMode::LOCK_FREE);
//        bufMgr.unfixBlock(nodeLaufvariable);
//        nodeLaufvariable = childNode;
//    }
//
//    if(nodeBlockView.containsValue(nodeLaufvariable, val)) {
//        tids.push_back(nodeBlockView.getTIDFor(nodeLaufvariable, val));
//    }
}

void DBMyIndex::insert(const DBAttrType &val, const TID &tid) {

}

void DBMyIndex::remove(const DBAttrType &val, const DBListTID &tid) {

}

string DBMyIndex::toString(string linePrefix) const {
    return DBIndex::toString(linePrefix);
}


/**
 * ab hier Set-Up
 */
// registerClass()-Methode am Ende dieser Datei: macht die Klasse der Factory bekannt
int rMyIdx = DBMyIndex::registerClass();
// Funktion bekannt machen
extern "C" void *createDBMyIndex(int nArgs, va_list &ap);


/**
 * Fuegt createDBMyIndex zur globalen factory method-map hinzu
 */
int DBMyIndex::registerClass() {
    setClassForName("DBMyIndex", createDBMyIndex);
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
extern "C" void *createDBMyIndex(int nArgs, va_list &ap) {
    // Genau 5 Parameter
    if (nArgs != 5) {
        throw DBException("Invalid number of arguments:" + nArgs);
    }
    DBBufferMgr *bufMgr = va_arg(ap, DBBufferMgr *);
    DBFile *file = va_arg(ap, DBFile *);
    enum AttrTypeEnum attrType = (enum AttrTypeEnum) va_arg(ap, int);
    ModType m = (ModType) va_arg(ap, int);
    bool unique = (bool) va_arg(ap, int);
    return new DBMyIndex(*bufMgr, *file, attrType, m, unique);
}

