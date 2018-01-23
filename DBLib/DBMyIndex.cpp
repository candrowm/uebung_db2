#include <hubDB/DBMyIndex.h>
#include <hubDB/DBException.h>
#include <hubDB/DBRandomBufferMgr.h>
#include <cmath>

using namespace HubDB::Index;
using namespace HubDB::Exception;

LoggerPtr DBMyIndex::logger(Logger::getLogger("HubDB.Index.DBMyIndex"));


int rMyIdx = DBMyIndex::registerClass();
int blockCounter = 0;

extern "C" void *createDBMyIndex(int nArgs, va_list &ap);

/**
 * Ausgabe des Indexes zum Debuggen
 */
string DBMyIndex::toString(string linePrefix) const {
    stringstream ss;
    ss << linePrefix << "[DBMyIndex]" << endl;
    ss << DBIndex::toString(linePrefix + "\t") << endl;
    ss << linePrefix << "-----------" << endl;
    return ss.str();
}

/** Konstruktor
 * - DBBufferMgr & bufferMgr (Referenz auf Buffermanager)
 * - DBFile & file (Referenz auf Dateiobjekt)
 * - enum AttrTypeEnum (Typ des Indexattributs)
 * - ModType mode (Accesstyp: READ, WRITE - siehe DBTypes.h)
 * - bool unique (ist Attribute unique)
 */
DBMyIndex::DBMyIndex(DBBufferMgr &bufferMgr, DBFile &file,
                     enum AttrTypeEnum attrType, ModType mode, bool unique) :
        DBIndex(bufferMgr, file, attrType, mode, unique) {
    if (logger != NULL) {
        LOG4CXX_INFO(logger, "DBMyIndex()");
    }

    //std::cout << "hallo" << std::endl;
    int numberOfBlocks = bufMgr.getBlockCnt(file);
    //std::cout << "Number of Blocks:" << numberOfBlocks << std::endl;

    if (numberOfBlocks == 0) {
        DBBACB metaBlock = bufMgr.fixNewBlock(file);
        TreeStartBlock metaNode = TreeStartBlock();
        metaNode.blockNo = metaBlock.getBlockNo();

        DBBACB rootBlock = bufMgr.fixNewBlock(file);
        TreeLeafBlock *rootNode = createInitialRoot(rootBlock.getBlockNo());
        
        metaNode.rootBlockNo = rootNode->blockNo;

        metaNode.copyBlockToDBBACB(metaBlock);
        bufMgr.unfixBlock(metaBlock);

        rootNode->copyBlockToDBBACB(rootBlock);
        bufMgr.unfixBlock(rootBlock);
        delete rootNode;
    }
    
    /*
    for(int i = 1; i <= 100000; i++){
        insert(DBIntType(i),TID());
    }
    
    printAllBlocks();
    
    DBListTID tids = DBListTID(0);
    for(int i = 1; i <= 100000; i++){
        //printAllBlocks();
        cout << "remove: " << i;
        remove(DBIntType(i),tids);
    }
     */
    
    //printAllBlocks();
    //remove(DBIntType(2),tids);
    //printAllBlocks();
    // TESTZWECKE!!!
    /*
    int newSize = 20;
    for (int i = 0; i < newSize; i++) {
        insertValueFirstCall(i, TID());
        // printAllBlocks();
        // printFreeBlocks();
    }


    for (int i = 0; i < newSize; i++) {
        removeValueFirstCall(i, TID());
        //printAllBlocks();
        // printFreeBlocks();
    }
    */
    /*
    char * test = new char[MAX_STR_LEN+1];
    srand(21);
    int size = 10000;
    int *values = new int[size];
    for (int i = 0; i < size; i++) {
        int randomNumber;
        while (true) {
            bool newNumber = true;
            randomNumber = rand() % (size * 2);
            for (int j = 0; j < i; j++) {
                if (randomNumber == values[j]) {
                    newNumber = false;
                    break;
                }
            }
            if (newNumber) {
                break;
            }
        }

        values[i] = randomNumber;
        if (randomNumber == 762) {
            //return;
        }
        
        string s = to_string(values[i]);
        strncpy(&test[0], s.c_str(), MAX_STR_LEN);
        test[MAX_STR_LEN]='\0';
        insert(DBVCharType(s.c_str()),TID());
        
        //insert(DBDoubleType(values[i]), TID());

        // printAllBlocks();
    }
     */
    
    /*
    printAllBlocks();
    //return;
    std::cout << "los--------------" << std::endl;

    //  return;
    DBListTID tids = DBListTID(0);
    
    for (int i = 0; i < size; i++) {
        cout << "remove: " << i;
        
        if(i==15){
            int a = 3;
        }
        //printAllBlocks();
        string s = to_string(values[i]);
        strncpy(&test[0], s.c_str(), MAX_STR_LEN);
        test[MAX_STR_LEN]='\0';
        remove(DBVCharType(s.c_str()),tids);
        
        //remove(DBDoubleType(values[i]), tids);
    }
    printAllBlocks();
    //printFreeBlocks();
     */
    
}

DBMyIndex::~DBMyIndex() {

}

/*      BLOCK MANAGEMENT      */

DBBACB DBMyIndex::fixNewBlock() {
    BlockNo nextFreeBlockNo = getFreeBlock();
    if (nextFreeBlockNo == 0) {
        //std::cout << "Keine freien Blocks da" << std::endl;
        DBBACB returnValue = bufMgr.fixNewBlock(file);
        return returnValue;

    } else {
        DBBACB fStart = bufMgr.fixBlock(file, nextFreeBlockNo, LOCK_EXCLUSIVE);
        // std::cout << "nextFreeBlockNo: <<<<<<<<<<<<<<<<" << nextFreeBlockNo << std::endl;
        //std::cout << "Warn unberechtigt" << std::endl;
        return fStart;
    }

}

BlockNo DBMyIndex::getFreeBlock() {
    DBBACB fStart = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
    TreeStartBlock *fStartBlock = (TreeStartBlock *) fStart.getDataPtr();
    BlockNo nextFreeBlockNo = fStartBlock->nextFreeBlock;
    if (nextFreeBlockNo == 0) {
        bufMgr.unfixBlock(fStart);
        return BlockNo(0);
    }

    DBBACB nextBlock = bufMgr.fixBlock(file, nextFreeBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *t = (TreeBlock *) nextBlock.getDataPtr();

    fStartBlock->nextFreeBlock = t->nextFreeBlockNo;
    bufMgr.unfixBlock(nextBlock);

    fStartBlock->copyBlockToDBBACB(fStart);
    bufMgr.unfixBlock(fStart);
    return nextFreeBlockNo;
}

void DBMyIndex::insertFreeBlock(BlockNo blockNo) {
    // std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << std::endl;
    /*
    DBBACB fStart = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
    TreeStartBlock *fStartBlock = (TreeStartBlock *) fStart.getDataPtr();
    std::cout << fStartBlock->nextFreeBlock << " next free Nummer von Meta" << std::endl;
    if (fStartBlock->nextFreeBlock == 0) {
        // std::cout << "Neuen Freien Knoten im Metablock speichern" << std::endl;
        fStartBlock->nextFreeBlock = blockNo;
        fStartBlock->copyBlockToDBBACB(fStart);
        bufMgr.unfixBlock(fStart);
    } else {
        BlockNo nextFreeBlockNo = fStartBlock->nextFreeBlock;
        while (true) {
            DBBACB nextBlock = bufMgr.fixBlock(file, nextFreeBlockNo, LOCK_EXCLUSIVE);
            TreeBlock *t = (TreeBlock *) nextBlock.getDataPtr();
            if (t->nextFreeBlockNo == 0) {
                if(t->leaf){
                    TreeLeafBlock *treeLeafBlock = getLeafBlockFromDBBACB(nextBlock); //dont forget to delete
                    treeLeafBlock->nextFreeBlockNo = blockNo;
                    treeLeafBlock->copyBlockToDBBACB(nextBlock);
                    delete treeLeafBlock;
                }
                else{
                    TreeInnerBlock *treeInnerBlock = getInnerBlockFromDBBACB(nextBlock); //dont forget to delete
                    treeInnerBlock->nextFreeBlockNo = blockNo;
                    treeInnerBlock->copyBlockToDBBACB(nextBlock);
                    delete treeInnerBlock;
                }
                bufMgr.unfixBlock(nextBlock);
                return;
            } else {
                nextFreeBlockNo = t->nextFreeBlockNo;
                bufMgr.unfixBlock(nextBlock);
            }
        }
    }
     */

}

void DBMyIndex::printFreeBlocks() {
    DBBACB fStart = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
    TreeStartBlock *fStartBlock = (TreeStartBlock *) fStart.getDataPtr();
    if (fStartBlock->nextFreeBlock == 0) {
        // fStartBlock->nextFreeBlock=blockNo;
        // std::cout << "Kein freier Block" << std::endl;
        fStartBlock->copyBlockToDBBACB(fStart);
        bufMgr.unfixBlock(fStart);
    } else {
        BlockNo nextFreeBlockNo = fStartBlock->nextFreeBlock;
        while (true) {
            DBBACB nextBlock = bufMgr.fixBlock(file, nextFreeBlockNo, LOCK_EXCLUSIVE);
            TreeBlock *t = (TreeBlock *) nextBlock.getDataPtr();
            std::cout << "Free Block: " << t->blockNo << std::endl;
            if (t->nextFreeBlockNo == 0) {
                //t->copyBlockToDBBACB(nextBlock);
                bufMgr.unfixBlock(nextBlock);

                fStartBlock->copyBlockToDBBACB(fStart);
                bufMgr.unfixBlock(fStart);
                return;
            } else {
                nextFreeBlockNo = t->nextFreeBlockNo;
                bufMgr.unfixBlock(nextBlock);
            }
        }
    }

}

/*      LOAD TREEBLOCKS FROM DBBACB/BLOCKNO      */

TreeLeafBlock * DBMyIndex::createInitialRoot(BlockNo blockNo){
    switch(attrType){
        case INT:{
            return new TreeIntLeafBlock(blockNo);
        }
        
        case VCHAR:{
            return new TreeVarCharLeafBlock(blockNo);
        }
        case DOUBLE:{
            return new TreeDoubleLeafBlock(blockNo);
        }
        default:{
            return (TreeLeafBlock *) nullptr;
        }
    }
}

TreeInnerBlock * DBMyIndex::getInnerBlockFromDBBACB(DBBACB d){
    switch(attrType) {
        case INT: {
            TreeIntInnerBlock *intInnerBlock = new TreeIntInnerBlock(d.getBlockNo());
            intInnerBlock->copyDBBACBToBlock(d);
            return intInnerBlock;
            break;
        }
        
        case VCHAR: {
            TreeVarCharInnerBlock *varCharInnerBlock = new TreeVarCharInnerBlock(d.getBlockNo());
            varCharInnerBlock->copyDBBACBToBlock(d);
            return varCharInnerBlock;
            break;
        }
        case DOUBLE: {
            TreeDoubleInnerBlock * doubleInnerBlock = new TreeDoubleInnerBlock(d.getBlockNo());
            doubleInnerBlock->copyDBBACBToBlock(d);
            return doubleInnerBlock;
            break;
        }
        
        default:{
            return (TreeInnerBlock *) nullptr;
        }
    }
}

TreeLeafBlock * DBMyIndex::getLeafBlockFromDBBACB(DBBACB d){
    switch(attrType) {
        case INT: {
            TreeIntLeafBlock *intLeafBlock = new TreeIntLeafBlock(d.getBlockNo());
            //intLeafBlock->updatePointers();
            intLeafBlock->copyDBBACBToBlock(d);
            return intLeafBlock;
            break;
        }
        
        case VCHAR: {
            TreeVarCharLeafBlock *varCharLeafBlock = new TreeVarCharLeafBlock(d.getBlockNo());
            varCharLeafBlock->copyDBBACBToBlock(d);
            return varCharLeafBlock;
            break;
        }
        case DOUBLE:{
            TreeDoubleLeafBlock * doubleLeafBlock = new TreeDoubleLeafBlock(d.getBlockNo());
            doubleLeafBlock->copyDBBACBToBlock(d);
            return doubleLeafBlock;
            break;
        }
        
        default:{
            return (TreeLeafBlock *) nullptr;
        }
    }
}


// Das Ausgeben aller Blocks der Indexdatei
void DBMyIndex::printAllBlocks() {
    std::cout << "------------------" << std::endl;
    DBBACB fStart = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *fStartBlock = (TreeStartBlock *) fStart.getDataPtr();
    std::cout << " Root No: " << fStartBlock->rootBlockNo << std::endl;
    bufMgr.unfixBlock(fStart);

    
    for (int i = 1; i < bufMgr.getBlockCnt(file); i++) {
        try {
            DBBACB fRoot = bufMgr.fixBlock(file, i, LOCK_SHARED);
            TreeBlock *t = (TreeBlock *) fRoot.getDataPtr();
            if (!t->leaf) {
                TreeInnerBlock *treeInnerBlock = getInnerBlockFromDBBACB(fRoot);
                //fRootBlock->updatePointers();
                treeInnerBlock->printAllValues();
                delete treeInnerBlock;
            }
            if (t->leaf) {
                TreeLeafBlock *treeLeafBlock = getLeafBlockFromDBBACB(fRoot);
                //treeLeafBlock->updatePointers();
                treeLeafBlock->printAllValues();
                delete treeLeafBlock;
            }
            bufMgr.unfixBlock(fRoot);
        } catch (int e) {
            return;
        }

    }
    std::cout << "------------------" << std::endl;

}

// Wenn ein Wert geloescht wird, wird zuerst diese Funktion aufgerufen
// Danaach wird rekursiv removeValue(...) aufgerufen
void DBMyIndex::removeValueFirstCall(const DBAttrType &value, const TID &tid) {
    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
    bufMgr.unfixBlock(metaB);
    removeValue(startBlock->rootBlockNo, value, tid, 0);

}

// Diese Funktion löscht einen Wert und wird rekursiv aufgerufen
// Dabei wird angegeben, bei welchem Block angefangen wird zu lesen
// d.h. anfangs wird die Funktion mit der RootBlockNo aufgerufen, später mit einem inneren Knoten, so lange, bis der Blattknoten erreicht wird
// Der Rückgabewert der Funktion sagt, ob der untere Knoten zu wenig Werte hat (=undersized)
// Zusätzlich wird ein int-Wert zurückgeliefert, der benötigt wird, falls der zu löschende Wert ebenfalls in einem inneren Knoten enthalten ist
UndersizedAndValuePair
DBMyIndex::removeValue(BlockNo startBlockNo, const DBAttrType &value, const TID &tid, BlockNo parentBlockNo) {
    //std::cout << "REMOVE VALUE " << value << " StartBlock: " << startBlockNo << std::endl;
    DBBACB rootB = bufMgr.fixBlock(file, startBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *treeBlock = (TreeBlock *) rootB.getDataPtr();

    // Falls man den Blattknoten erwischt hat --> Wert löschen
    if (treeBlock->leaf) {
        TreeLeafBlock *treeLeafBlock = getLeafBlockFromDBBACB(rootB); //dont forget to delete
        UndersizedAndValuePair r = treeLeafBlock->removeTID(value, tid);
        if (parentBlockNo == 0) {
            // Rootknoten ist einziger Knoten und kann deshalb nicht zu wenig Werte enthalten
            r.undersized = false;
        }
        //std::cout << r.undersized << std::endl;
        treeLeafBlock->copyBlockToDBBACB(rootB);
        bufMgr.unfixBlock(rootB);

        delete treeLeafBlock;
        return r;
    }

    int position = -1;
    // Falls man den Blattknoten noch nicht erreicht hat --> Neuen rekursiven Aufruf (so lange, bis der Blattknoten erreicht wird)
    if (!treeBlock->leaf) {

        TreeInnerBlock *treeInnerBlock = getInnerBlockFromDBBACB(rootB); //dont forget to delete

        // Prüfen, welchen Block man als nächstes lesen muss, um eine Ebene "näher" an den zu löschenden Wert zu kommen
        UndersizedAndValuePair r = UndersizedAndValuePair(DBIntType(-1), false, attrType);
        for (int i = 0; i < treeInnerBlock->currentValueCounter; i++) {


            // Falls der Wert größer ist als der letzte Wert im inneren Knoten --> Letzten Pointer (=BlockNo) des inneren Knoten lesen
            // value > treeInnerBlock->values[treeInnerBlock->currentValueCounter - 1]
            if(treeInnerBlock->compare(treeInnerBlock->currentValueCounter-1, value) == -1){
                r = removeValue(treeInnerBlock->getBlockNo(treeInnerBlock->currentValueCounter), value, tid, treeInnerBlock->blockNo);
                position = treeInnerBlock->currentValueCounter;
                break;
            }

            // Alle Werte des inneren Knoten durchgehen und gucken, welcher Pointer (=BlockNo) der richtige für den Wert ist
            //value <= treeInnerBlock->values[i]
            if (treeInnerBlock->compare(i, value) >= 0) {
                r = removeValue(treeInnerBlock->getBlockNo(i), value, tid,
                                treeInnerBlock->blockNo);

                position = i;

                // Wenn der zu loeschende Wert einem Wert im inneren Knoten entspricht --> Wert im inneren Knoten aendern
                if (treeInnerBlock->compare(i, value) == 0) {
                    // TODO: KEINE AHNUNG OB
                    treeInnerBlock->setValue(i,r.getValue(attrType));
                    treeInnerBlock->copyBlockToDBBACB(rootB);
                    //bufMgr.unfixBlock(rootB);

                    // std::cout << "Shit: ich muss Wert aendern im Blattknoten zu dem Wert" << r.value
                    //    << std::endl;
                }
                break;
            }
        }

        // Wenn der Kinderknoten voll genug ist --> Keine Änderung notwendig + evtl. neuen Wert nach "oben" schieben (falls Wert des Elternknoten geloescht wird)
        if (!r.undersized) {
            delete treeInnerBlock;
            bufMgr.unfixBlock(rootB);
            return r;
        }

        // Wenn dem Blattknoten ein Wert entfernt wurde und dieser nun zu wenig Werte hat --> undersized==true
        if (r.undersized) {

            //std::cout << " Position von undersized leaf " << position << std::endl;
            int positionOfUndersizedBlock = treeInnerBlock->getBlockNo(position);

            DBBACB undersizedBlock = bufMgr.fixBlock(file, positionOfUndersizedBlock, LOCK_EXCLUSIVE);
            TreeBlock *undersizedTreeBlock = (TreeBlock *) undersizedBlock.getDataPtr();


            // Wenn sich unter dem inneren Knoten kein Blattknoten befindet, sondern wieder ein innerer Knoten
            if (!undersizedTreeBlock->leaf) {


                TreeInnerBlock *undersizedTreeInnerBlock = getInnerBlockFromDBBACB(undersizedBlock); // don't forget to delete
                // std::cout << " POSITION!!!!!!!!!!!!!!!!!!11111 " << undersizedTreeIntInnerBlock->blockNo << std::endl;


                // Moeglickeit 1: aus dem linken inneren Knoten ein Value+BlockNo Paar rueberschieben
                if (position > 0) {
                    std::cout << "Vom linken inneren Knoten etwas klauen" << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position - 1),
                                                                LOCK_EXCLUSIVE);
                    //TreeIntInnerBlock *leftNeighbourInnerBlock = (TreeIntInnerBlock *) leftNeighbourBlock.getDataPtr();
                    TreeInnerBlock *leftNeighbourInnerBlock = getInnerBlockFromDBBACB(leftNeighbourBlock); // don't forget to delete x
                    //leftNeighbourInnerBlock->updatePointers();
                    ValueAndTIDPair leftNValueAndTIDPair = leftNeighbourInnerBlock->removeBiggestBlockNo();



                    //std::cout << " Ergebnis moeglichkeit 1 Value: " << rIntValueAndTIDPair.parentValue << " " << rIntValueAndTIDPair.successful << std::endl;
                    // successful, if removing biggest blockno does not undersize left neighbor
                    if (leftNValueAndTIDPair.successful) {
                        DBAttrType * oldValue = treeInnerBlock->getValue(position - 1);

                        // adjust max value for left neighbor
                        treeInnerBlock->setValue(position - 1, leftNValueAndTIDPair.getValue(attrType));

                        // ??
                        if (position >= treeInnerBlock->currentValueCounter) {
                            position--;
                        }

                        r.undersized = false;

                        //treeInnerBlock->printAllValues();
                        //leftNeighbourInnerBlock->printAllValues();
                        //undersizedTreeInnerBlock->printAllValues();


                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;

                        undersizedTreeInnerBlock->insertBlockNo(*oldValue, leftNValueAndTIDPair.blockNo, true);
                        undersizedTreeInnerBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);
                        delete undersizedTreeInnerBlock;
                        delete oldValue;

                        leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);
                        delete leftNeighbourInnerBlock;

                        return r;

                    }
                    leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                    delete leftNeighbourInnerBlock;
                }


                // Moeglickeit 1: aus dem rechten inneren Knoten ein Value+BlockNo Paar rueberschieben
                if (position < (treeInnerBlock->currentValueCounter)) {
                    //  std::cout << "Versuche Wert zu klauen von rechts (innere Knoten)" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position + 1),
                                                                 LOCK_EXCLUSIVE);


                    TreeInnerBlock *rightNeighbourInnerBlock = getInnerBlockFromDBBACB(rightNeighbourBlock); //don't forget to delete
                    ValueAndTIDPair rightNValueAndTIDPair = rightNeighbourInnerBlock->removeSmallestBlockNo();
                    if (rightNValueAndTIDPair.successful) {
                        // removing smallest blockno from right neighbour does not undersize it
                        DBAttrType * oldValue = treeInnerBlock->getValue(position); //don't forget to delete
                        treeInnerBlock->setValue(position,rightNValueAndTIDPair.getValue(attrType));


                        std::cout << " Wert geklaut von rechts und Elternknoten angepasst " << rightNValueAndTIDPair.getValue(attrType).toString()
                                  << std::endl;


                        rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                        bufMgr.unfixBlock(rightNeighbourBlock);
                        delete rightNeighbourInnerBlock;

                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;

                        undersizedTreeInnerBlock->insertBlockNo(*oldValue, rightNValueAndTIDPair.blockNo, false);
                        undersizedTreeInnerBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);
                        delete undersizedTreeInnerBlock;
                        delete oldValue;

                        r.undersized = false;
                        return r;
                    }

                    rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    bufMgr.unfixBlock(rightNeighbourBlock);
                    delete rightNeighbourInnerBlock;

                }


                //std::cout << " Bleibt nur noch Merge der inneren Knoten" << std::endl;

                // Moeglichkeit 3: Falls das Rüberschieben eines Value+BlockNo Paar von links und rechts nicht geklappt hat --> Merge notwendig
                // 3.1: Merge mit linkem Knoten
                if (position > 0) {


                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position - 1),
                                                                LOCK_EXCLUSIVE);
                    TreeInnerBlock *leftNeighbourInnerBlock = getInnerBlockFromDBBACB(leftNeighbourBlock);

                    std::cout << "Merge mit linkem inneren Knoten " << leftNeighbourInnerBlock->blockNo << " mit "
                              << undersizedTreeInnerBlock->blockNo << " und Eltern " << treeInnerBlock->blockNo
                              << std::endl;


                    if (position >= treeInnerBlock->currentValueCounter) {
                        position--;
                    }

                    //undersizedTreeInnerBlock->printAllValues();
                    //leftNeighbourInnerBlock->printAllValues();

                    // upper bound for left neighbour becomes a key in left neighbour after merge
                    for (int i = 0; i < treeInnerBlock->currentValueCounter; i++) {
                        if (treeInnerBlock->getBlockNo(i) == leftNeighbourInnerBlock->blockNo) {
                            DBAttrType * v = treeInnerBlock->getValue(i);
                            leftNeighbourInnerBlock->setValue(leftNeighbourInnerBlock->currentValueCounter,*v);
                            delete v;
                            break;
                        }
                    }

                    //leftNeighbourInnerBlock->values[undersizedTreeIntInnerBlock->currentValueCounter] = treeIntInnerBlock->values[position];
                    //leftNeighbourInnerBlock->printAllValues();

                    for (int i = 0; i <= undersizedTreeInnerBlock->currentValueCounter; i++) {
                        if (i < undersizedTreeInnerBlock->currentValueCounter) {
                            DBAttrType * v = undersizedTreeInnerBlock->getValue(i);
                            leftNeighbourInnerBlock->setValue(leftNeighbourInnerBlock->currentValueCounter +
                                                            i + 1, *v);
                            delete v;
                        }
                        leftNeighbourInnerBlock->setBlockNo(leftNeighbourInnerBlock->currentValueCounter +
                                                          i + 1, undersizedTreeInnerBlock->getBlockNo(i));
                        //leftNeighbourInnerBlock->currentValueCounter++;
                    }

                    leftNeighbourInnerBlock->currentValueCounter = leftNeighbourInnerBlock->currentValueCounter + 1 +
                                                                   undersizedTreeInnerBlock->currentValueCounter;

                    // std::cout << "NACH MERGE: " << std::endl;
                    //leftNeighbourInnerBlock->printAllValues();

                    undersizedTreeInnerBlock->currentValueCounter = 0;
                    undersizedTreeInnerBlock->nextFreeBlockNo = BlockNo(0);

                    //TODO: loeschen von undersized Knoten, da nicht verwendet
                    //treeInnerBlock->printAllValues();

                    // replace key for left neighbour by moving all blockNos/values one index to the left
                    bool found = false;
                    for (int i = 0; i <= treeInnerBlock->currentValueCounter; i++) {
                        if (treeInnerBlock->getBlockNo(i) == undersizedTreeInnerBlock->blockNo) {
                            found = true;
                        }
                        if (found) {
                            if (i == treeInnerBlock->currentValueCounter) {
                                treeInnerBlock->setBlockNo(i - 1, treeInnerBlock->getBlockNo(i));
                                break;
                            }
                            treeInnerBlock->setBlockNo(i - 1, treeInnerBlock->getBlockNo(i));
                            DBAttrType * v = treeInnerBlock->getValue(i);
                            treeInnerBlock->setValue(i - 1,  *v);
                            delete v;
                        }
                    };
                    //treeInnerBlock->printAllValues();
                    // fix the blockno for the left neighbour
                    for (int i = 0; i < treeInnerBlock->currentValueCounter; i++) {
                        if (treeInnerBlock->getBlockNo(i) == undersizedTreeInnerBlock->blockNo) {
                            treeInnerBlock->setBlockNo(i,leftNeighbourInnerBlock->blockNo);
                        }
                    }
                    //treeInnerBlock->printAllValues();
                    // Nach dem Merge von 2 inneren Knoten muss der Elternknoten angepasst werden
                    //memcpy
                    // (&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position+1],
                    // sizeof(int) * (treeIntInnerBlock->currentValueCounter - position + 1));

                    //memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position+1],
                    //sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position + 1));

                    // treeIntInnerBlock->printAllValues();


                    treeInnerBlock->currentValueCounter = treeInnerBlock->currentValueCounter - 1;
                    //treeIntInnerBlock->printAllValues();

                    //treeIntInnerBlock->blockNos[position] = leftNeighbourInnerBlock->blockNo;
                    // treeIntInnerBlock->printAllValues();
                    //  treeIntInnerBlock->values[position] = leftNeighbourInnerBlock->values[
                    //    leftNeighbourInnerBlock->currentValueCounter - 1];

                    //treeInnerBlock->printAllValues();
                    undersizedTreeInnerBlock->copyBlockToDBBACB(undersizedBlock);
                    insertFreeBlock(undersizedTreeInnerBlock->blockNo);
                    bufMgr.unfixBlock(undersizedBlock);
                    delete undersizedTreeInnerBlock;

                    // Falls der Elternknoten == Rootknoten ist (d.h. keinen Elternknoten mehr hat) und keine Werte mehr beinhaltet
                    // Neuen Rootknoten setzen (und alten Rootknoten löschen)
                    if (treeInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;
                        BlockNo newRootBlockNo = leftNeighbourInnerBlock->blockNo;
                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                        startBlock->rootBlockNo = newRootBlockNo;

                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);

                        treeInnerBlock->currentValueCounter = 0;
                        treeInnerBlock->nextFreeBlockNo = BlockNo(0);
                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeInnerBlock->blockNo);
                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;


                        leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);
                        delete leftNeighbourInnerBlock;

                        return r;

                    }


                    leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);

                    treeInnerBlock->copyBlockToDBBACB(rootB);


                    double currentValue = treeInnerBlock->currentValueCounter;
                    double maxValue = treeInnerBlock->maxValueCounter;
                    double divisionResult = (currentValue) / maxValue;

                    bufMgr.unfixBlock(rootB);
                    delete treeInnerBlock;

                    bufMgr.unfixBlock(leftNeighbourBlock);
                    delete leftNeighbourInnerBlock;

                    if (divisionResult > 0.4) {
                        r.undersized = false;
                        return r;
                    }
                    return r;


                }


                // 3.2. Merge mit rechtem Knoten
                if (position < (treeInnerBlock->currentValueCounter)) {
                    //std::cout << "Merge mit rechtem Knoten" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position + 1),
                                                                 LOCK_EXCLUSIVE);
                    TreeInnerBlock *rightNeighbourInnerBlock = getInnerBlockFromDBBACB(rightNeighbourBlock);
                    //rightNeighbourLeafBlock->updatePointers();

                    //std::cout << "VOR MERGE r: " << std::endl;
                    // undersizedTreeIntInnerBlock->printAllValues();
                    // std::cout << "Merge folgende Knoten " << std::endl;
                    //undersizedTreeInnerBlock->printAllValues();
                    //rightNeighbourLeafBlock->printAllValues();

                    // Aus 2 inneren Knoten 1 inneren Knoten machen, d.h. Werte und BlockNos von einem Knoten zum anderen rüberschieben
                    if (position >= treeInnerBlock->currentValueCounter) {
                        position--;
                    }

                    DBAttrType * v0 = treeInnerBlock->getValue(position);
                    undersizedTreeInnerBlock->setValue(undersizedTreeInnerBlock->currentValueCounter,*v0);
                    delete v0;
                    //undersizedTreeInnerBlock->printAllValues();


                    for (int i = 0; i <= rightNeighbourInnerBlock->currentValueCounter; i++) {
                        if (i < rightNeighbourInnerBlock->currentValueCounter) {
                            DBAttrType * v = rightNeighbourInnerBlock->getValue(i);
                            undersizedTreeInnerBlock->setValue(undersizedTreeInnerBlock->currentValueCounter +
                                                                i + 1, *v); 
                            delete v;
                        }
                        undersizedTreeInnerBlock->setBlockNo(undersizedTreeInnerBlock->currentValueCounter +
                                                              i + 1, rightNeighbourInnerBlock->getBlockNo(i));
                        //leftNeighbourInnerBlock->currentValueCounter++;
                    }

                    undersizedTreeInnerBlock->currentValueCounter =
                            undersizedTreeInnerBlock->currentValueCounter + 1 +
                            rightNeighbourInnerBlock->currentValueCounter;


/*
                    memcpy(&undersizedTreeIntInnerBlock->values[undersizedTreeIntInnerBlock->currentValueCounter + 1],
                           &rightNeighbourLeafBlock->values[0],
                           sizeof(int) * (rightNeighbourLeafBlock->currentValueCounter));


                    undersizedTreeIntInnerBlock->printAllValues();

                    memcpy(&undersizedTreeIntInnerBlock->blockNos[undersizedTreeIntInnerBlock->currentValueCounter + 1],
                           &rightNeighbourLeafBlock->blockNos[0],
                           sizeof(BlockNo) * (rightNeighbourLeafBlock->currentValueCounter + 1));


                    //  undersizedTreeIntInnerBlock->printAllValues();

                    undersizedTreeIntInnerBlock->currentValueCounter =
                            undersizedTreeIntInnerBlock->currentValueCounter +
                            rightNeighbourLeafBlock->currentValueCounter + 1;
                            */

                    //undersizedTreeInnerBlock->printAllValues();

                    //std::cout << "NACH MERGE: " << std::endl;
                    //undersizedTreeIntInnerBlock->printAllValues();


                    //treeInnerBlock->printAllValues();


                    for (int i = position; i < treeInnerBlock->currentValueCounter; i++) {
                        DBAttrType * v = treeInnerBlock->getValue(i+1);
                        treeInnerBlock->setValue(i,*v);
                        treeInnerBlock->setBlockNo(i,treeInnerBlock->getBlockNo(i + 1));
                        delete v;
                    }
                    //treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter]


                    // Nach dem Merge von 2 inneren Knoten muss der Elternknoten angepasst werden
                    //  memcpy(&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position + 1],
                    //  sizeof(int) * (treeIntInnerBlock->currentValueCounter - position));
                    // memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position + 1],
                    // sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position));


                    treeInnerBlock->currentValueCounter = treeInnerBlock->currentValueCounter - 1;
                    treeInnerBlock->setBlockNo(position,undersizedTreeInnerBlock->blockNo);
                    //treeIntInnerBlock->values[position] = undersizedTreeIntInnerBlock->values[
                    // undersizedTreeIntInnerBlock->currentValueCounter - 1];

                    //treeIntInnerBlock->printAllValues();

                    rightNeighbourInnerBlock->currentValueCounter = 0;
                    rightNeighbourInnerBlock->nextFreeBlockNo = BlockNo(0);
                    rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    insertFreeBlock(rightNeighbourInnerBlock->blockNo);
                    bufMgr.unfixBlock(rightNeighbourBlock);
                    delete rightNeighbourInnerBlock;


                    undersizedTreeInnerBlock->copyBlockToDBBACB(undersizedBlock);
                    bufMgr.unfixBlock(undersizedBlock);
                    //delete undersizedTreeInnerBlock;

                    // Falls der Elternknoten == Rootknoten ist (d.h. keinen Elternknoten mehr hat) und keine Werte mehr beinhaltet
                    // Neuen Rootknoten setzen (und alten Rootknoten löschen)
                    if (treeInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig!!!!" << std::endl;
                        BlockNo newRootBlockNo = undersizedTreeInnerBlock->blockNo;
                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                        treeInnerBlock->currentValueCounter = 0;
                        treeInnerBlock->nextFreeBlockNo = BlockNo(0);
                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeInnerBlock->blockNo);


                        startBlock->rootBlockNo = newRootBlockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);
                        bufMgr.unfixBlock(rootB);

                        delete treeInnerBlock;
                        delete undersizedTreeInnerBlock;

                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }
                    treeInnerBlock->copyBlockToDBBACB(rootB);


                    double currentValue = treeInnerBlock->currentValueCounter;
                    double maxValue = treeInnerBlock->maxValueCounter;
                    double divisionResult = (currentValue) / maxValue;

                    bufMgr.unfixBlock(rootB);

                    delete undersizedTreeInnerBlock;
                    delete treeInnerBlock;

                    if (divisionResult > 0.4) {
                        r.undersized = false;
                        return r;
                    }
                    return r;


                }
                bufMgr.unfixBlock(undersizedBlock);
                return r;
            } else {
                // Falls der innere Knoten auf Blätter zeigt
                //std::cout << "Blaetter zusammenschmelzen oder rumschieben " << std::endl;
                // Der innere Knoten hat BlockNo auf Blätter

                TreeLeafBlock *undersizedTreeLeafBlock = getLeafBlockFromDBBACB(undersizedBlock); //don't forget to delete

                //std::cout << "position " << position << std::endl;

                // Moeglickeit 1: aus dem linken Blatt ein Value+TID Paar rueberschieben
                if (position > 0) {
                    //  std::cout << "moeglichkeit 1" << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position - 1),
                                                                LOCK_EXCLUSIVE);
                    TreeLeafBlock *leftNeighbourLeafBlock = getLeafBlockFromDBBACB(leftNeighbourBlock);
                    ValueAndTIDPair leftNValueAndTIDPair = leftNeighbourLeafBlock->removeBiggestTID();

                    //std::cout << " Ergebnis moeglichkeit 1 Value: " << rIntValueAndTIDPair.parentValue << " " << rIntValueAndTIDPair.successful << std::endl;

                    if (leftNValueAndTIDPair.successful) {
                        //std::cout << "AUS: " << treeIntInnerBlock->values[position-1] << " wird " << rIntValueAndTIDPair.parentValue << std::endl;
                        treeInnerBlock->setValue(position - 1, leftNValueAndTIDPair.getNeighbourValue(attrType));
                        if (position >= treeInnerBlock->currentValueCounter) {
                            position--;
                        }

                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;

                        undersizedTreeLeafBlock->insertTID(leftNValueAndTIDPair.getValue(attrType), leftNValueAndTIDPair.tid);
                        undersizedTreeLeafBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);
                        delete undersizedTreeLeafBlock;

                        leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);
                        delete leftNeighbourLeafBlock;

                        std::cout << " Wert geklaut von links und Elternknoten angepasst "
                                  << leftNValueAndTIDPair.getNeighbourValue(attrType).toString() << std::endl;

                        r.undersized = false;
                        return r;

                    }

                    leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                    delete leftNeighbourLeafBlock;
                }

                // Moeglichkeit 2: aus dem rechten Blatt ein Value+TID Paar rueberschieben
                if (position < (treeInnerBlock->currentValueCounter)) {
                    // std::cout << "zwei" << std::endl;

                    bufMgr.unfixBlock(undersizedBlock);
                    delete undersizedTreeLeafBlock;

                    DBBACB undersizedBlock2 = bufMgr.fixBlock(file, positionOfUndersizedBlock, LOCK_EXCLUSIVE);
                    TreeLeafBlock *undersizedTreeLeafBlock2 = getLeafBlockFromDBBACB(undersizedBlock2);

                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position + 1),
                                                                 LOCK_EXCLUSIVE);
                    //std::cout << rightNeighbourBlock.getLockMode() << std::endl;
                    TreeLeafBlock *rightNeighbourLeafBlock = getLeafBlockFromDBBACB(rightNeighbourBlock);

                    ValueAndTIDPair rightNValueAndTIDPair = rightNeighbourLeafBlock->removeSmallestTID();
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);


                    if (rightNValueAndTIDPair.successful) {
                        //   std::cout << "TRY UNFIX: " << rightNeighbourBlock.getBlockNo() << std::endl;


                        undersizedTreeLeafBlock2->insertTID(rightNValueAndTIDPair.getValue(attrType), rightNValueAndTIDPair.tid);
                        undersizedTreeLeafBlock2->copyBlockToDBBACB(undersizedBlock2);


                        treeInnerBlock->setValue(position,rightNValueAndTIDPair.getValue(attrType));
                        treeInnerBlock->copyBlockToDBBACB(rootB);


                        std::cout << " Wert geklaut von rechts und Elternknoten angepasst !"
                                  << rightNValueAndTIDPair.getValue(attrType).toString()
                                  << std::endl;

                        r.undersized = false;


                        bufMgr.unfixBlock(rightNeighbourBlock);
                        delete rightNeighbourLeafBlock;

                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;

                        bufMgr.unfixBlock(undersizedBlock2);
                        delete undersizedTreeLeafBlock2;
                        return r;
                    }
                    bufMgr.unfixBlock(rightNeighbourBlock);
                    delete rightNeighbourLeafBlock;
                }

                bufMgr.unfixBlock(undersizedBlock);
                //delete undersizedTreeLeafBlock;

                DBBACB undersizedBlock2 = bufMgr.fixBlock(file, positionOfUndersizedBlock, LOCK_EXCLUSIVE);
                TreeLeafBlock *undersizedTreeLeafBlock2 = getLeafBlockFromDBBACB(undersizedBlock2); //don't forget to delete
                // std::cout << undersizedTreeIntLeafBlock->blockNo << "das bekommen " << std::endl;


                // Moeglichkeit 3: Falls das Rüberschieben von einem linken oder rechten Blattknoten nicht funktioniert hat --> Merge notwendig
                // 3.1: Merge mit linkem Knoten
                if (position > 0) {

                    std::cout << " Merge mit linkem Knoten " << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position - 1),
                                                                LOCK_EXCLUSIVE);
                    TreeLeafBlock *leftNeighbourLeafBlock = getLeafBlockFromDBBACB(leftNeighbourBlock); //don't forget to delete

                    // std::cout << undersizedTreeIntLeafBlock->blockNo << "das bekommen 2" << std::endl;

                    // std::cout << "VOR MERGE: " << std::endl;
                    //leftNeighbourLeafBlock->printAllValues();
                    //treeIntInnerBlock->printAllValues();

                    // 2 Blattknoten zu 1 Blattknoten verschmelzen
                    /*
                    memcpy(&leftNeighbourLeafBlock->values[leftNeighbourLeafBlock->currentValueCounter],
                           &undersizedTreeIntLeafBlock2->values[0],
                           sizeof(int) * undersizedTreeIntLeafBlock2->currentValueCounter);

                    memcpy(&leftNeighbourLeafBlock->tids[leftNeighbourLeafBlock->currentValueCounter],
                           &undersizedTreeIntLeafBlock2->tids[0],
                           sizeof(TID) * undersizedTreeIntLeafBlock2->currentValueCounter);
                    */
                    for (int i = 0; i < undersizedTreeLeafBlock2->currentValueCounter; i++) {
                        DBAttrType * v = undersizedTreeLeafBlock2->getValue(i);
                        leftNeighbourLeafBlock->setValue(leftNeighbourLeafBlock->currentValueCounter +
                                                       i,*v);
                        leftNeighbourLeafBlock->setTID(leftNeighbourLeafBlock->currentValueCounter +
                                                     i, undersizedTreeLeafBlock2->getTID(i));
                        delete v;
                    }


                    leftNeighbourLeafBlock->currentValueCounter = leftNeighbourLeafBlock->currentValueCounter +
                                                                  undersizedTreeLeafBlock2->currentValueCounter;

                    //std::cout << "NACH MERGE: " << std::endl;
                    //leftNeighbourLeafBlock->printAllValues();
                    // std::cout << startBlockNo << std::endl;


                    undersizedTreeLeafBlock2->currentValueCounter = 0;
                    undersizedTreeLeafBlock2->copyBlockToDBBACB(undersizedBlock2);
                    //std::cout << treeIntInnerBlock->blockNo << "treeIntInnerBlock " << std::endl;

                    insertFreeBlock(undersizedTreeLeafBlock2->blockNo);


                    //TODO: loeschen von undersized Knoten, da nicht verwendet

                    // Elternknoten anpassen, nachdem 2 Blattknoten zusammengeschmolzen wurden
                    // memcpy(&treeIntInnerBlock->values[position - 1], &treeIntInnerBlock->values[position],
                    //     sizeof(int) * (treeIntInnerBlock->currentValueCounter - position + 1));
                    //  memcpy(&treeIntInnerBlock->blockNos[position - 1], &treeIntInnerBlock->blockNos[position],
                    //  sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position + 1));
                    //treeInnerBlock->printAllValues();
                    //std::cout << "JETZT" << std::endl;
                    // std::cout << "Position" << position - 1 << std::endl;
                    for (int i = position; i <= treeInnerBlock->currentValueCounter; i++) {
                        DBAttrType * v = treeInnerBlock->getValue(i);
                        treeInnerBlock->setValue(i - 1,*v);
                        treeInnerBlock->setBlockNo(i - 1,treeInnerBlock->getBlockNo(i));
                        delete v;
                    }

                    //treeInnerBlock->printAllValues();

                    treeInnerBlock->currentValueCounter = treeInnerBlock->currentValueCounter - 1;
                    treeInnerBlock->setBlockNo(position - 1,leftNeighbourLeafBlock->blockNo);
                    //treeIntInnerBlock->values[position - 1] = leftNeighbourLeafBlock->values[
                    //  leftNeighbourLeafBlock->currentValueCounter - 1];

                    // treeIntInnerBlock->printAllValues();

                    // Falls der Elternknoten der Rootknoten ist und keine Werte mehr beinhaltet --> Neuen Rootknoten setzen und alten löschen
                    if (treeInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;
                        BlockNo newRootBlockNo = leftNeighbourLeafBlock->blockNo;


                        treeInnerBlock->currentValueCounter = 0;
                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeInnerBlock->blockNo);
                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;


                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = newRootBlockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);


                        bufMgr.unfixBlock(undersizedBlock2);
                        delete undersizedTreeLeafBlock2;

                        leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);
                        delete leftNeighbourLeafBlock;

                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }

                    double currentValue = treeInnerBlock->currentValueCounter;
                    double maxValue = treeInnerBlock->maxValueCounter;

                    //std::cout << "HIER 3" << std::endl;
                    //std::cout << "HIER 3-1" << std::endl;
                    // undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);

                    //std::cout << "HIER 3-2" << std::endl;


                    // std::cout << "HIER 4" << std::endl;

                    bufMgr.unfixBlock(undersizedBlock2);
                    delete undersizedTreeLeafBlock2;

                    leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                    delete leftNeighbourLeafBlock;

                    treeInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);
                    delete treeInnerBlock;

                    double divisionResult = (currentValue) / maxValue;


                    if (divisionResult > 0.4) {
                        r.undersized = false;
                        return r;
                    }
                    return r;


                }


                // 3.2. Merge mit rechtem Knoten
                if (position < (treeInnerBlock->currentValueCounter)) {
                    std::cout << "Merge mit rechtem Knoten" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeInnerBlock->getBlockNo(position + 1),
                                                                 LOCK_EXCLUSIVE);
                    TreeLeafBlock *rightNeighbourLeafBlock = getLeafBlockFromDBBACB(rightNeighbourBlock);

                    //std::cout << "VOR MERGE: " << std::endl;
                    //undersizedTreeIntLeafBlock->printAllValues();


                    // 2 Blattknoten zu 1 Blattknoten verschmelzen
                    if (position >= treeInnerBlock->currentValueCounter) {
                        position--;
                    }

                    for (int i = 0; i < rightNeighbourLeafBlock->currentValueCounter; i++) {
                        DBAttrType * v = rightNeighbourLeafBlock->getValue(i);
                        undersizedTreeLeafBlock2->setValue(undersizedTreeLeafBlock2->currentValueCounter +
                                                            i,*v);
                        undersizedTreeLeafBlock2->setTID(undersizedTreeLeafBlock2->currentValueCounter +
                                                          i, rightNeighbourLeafBlock->getTID(i));
                        delete v;
                    }
/*
                    memcpy(&undersizedTreeIntLeafBlock2->values[undersizedTreeIntLeafBlock2->currentValueCounter],
                           &rightNeighbourLeafBlock->values[0],
                           sizeof(int) * rightNeighbourLeafBlock->currentValueCounter);

                    memcpy(&undersizedTreeIntLeafBlock->tids[undersizedTreeIntLeafBlock2->currentValueCounter],
                           &rightNeighbourLeafBlock->tids[0],
                           sizeof(TID) * rightNeighbourLeafBlock->currentValueCounter);
                           */

                    undersizedTreeLeafBlock2->currentValueCounter =
                            undersizedTreeLeafBlock2->currentValueCounter +
                            rightNeighbourLeafBlock->currentValueCounter;

                    //std::cout << "NACH MERGE: " << std::endl;
                    //  undersizedTreeIntLeafBlock2->printAllValues();


                    //treeInnerBlock->printAllValues();

                    /*
                    // Nach dem Merge von 2 Blattknoten muss der Elternknoten angepasst werden
                    memcpy(&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position + 1],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - position));
                    memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position + 1],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position));
                           */


                    for (int i = position; i < treeInnerBlock->currentValueCounter; i++) {
                        DBAttrType * v = treeInnerBlock->getValue(i+1);
                        treeInnerBlock->setValue(i,*v);
                        treeInnerBlock->setBlockNo(i,treeInnerBlock->getBlockNo(i + 1));
                        delete v;
                    }


                    treeInnerBlock->currentValueCounter = treeInnerBlock->currentValueCounter - 1;
                    treeInnerBlock->setBlockNo(position,undersizedTreeLeafBlock2->blockNo);

                    DBAttrType * v0 = undersizedTreeLeafBlock2->getValue(undersizedTreeLeafBlock2->currentValueCounter - 1);
                    treeInnerBlock->setValue(position,*v0);
                    delete v0;

                    // treeIntInnerBlock->printAllValues();
                    // std::cout << "Fertig mit merge" << std::endl;

                    rightNeighbourLeafBlock->currentValueCounter = 0;
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    insertFreeBlock(rightNeighbourLeafBlock->blockNo);
                    bufMgr.unfixBlock(rightNeighbourBlock);
                    delete rightNeighbourLeafBlock;


                    //undersizedTreeLeafBlock2->copyBlockToDBBACB(undersizedBlock2);
                    //bufMgr.unfixBlock(undersizedBlock2);
                    //delete undersizedTreeLeafBlock2;


                    // Falls der Elternknoten der Rootknoten ist und keine Werte mehr beinhaltet --> Neuen Rootknoten setzen + alten löschen
                    if (treeInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig :O" << std::endl;
                        BlockNo newRootBlockNo = undersizedTreeLeafBlock2->blockNo;
                        // std::cout << "VERSUCHE UNFIX:" << rootB.getBlockNo() << std::endl;

                        undersizedTreeLeafBlock2->copyBlockToDBBACB(undersizedBlock2);
                        bufMgr.unfixBlock(undersizedBlock2);
                        delete undersizedTreeLeafBlock2;

                        treeInnerBlock->currentValueCounter = 0;
                        treeInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeInnerBlock->blockNo);
                        bufMgr.unfixBlock(rootB);
                        delete treeInnerBlock;

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = newRootBlockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }
                    double currentValue = treeInnerBlock->currentValueCounter;
                    double maxValue = treeInnerBlock->maxValueCounter;
                    double divisionResult = (currentValue) / maxValue;

                    treeInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);
                    delete treeInnerBlock;

                    undersizedTreeLeafBlock2->copyBlockToDBBACB(undersizedBlock2);
                    bufMgr.unfixBlock(undersizedBlock2);
                    delete undersizedTreeLeafBlock2;

                    if (divisionResult > 0.4) {
                        r.undersized = false;
                        return r;
                    }


                    return r;


                }

            }

        }

    }

}

// Wenn ein Wert eingefügt wird, wird als erstes diese Funktion aufgerufen
// Im Anschluss wird insertValue(...) rekursiv aufgerufen
void DBMyIndex::insertValueFirstCall(const DBAttrType &value, const TID &tid) {
    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
    bufMgr.unfixBlock(metaB);
    insertValue(startBlock->rootBlockNo, value, tid, 0);
}

// Diese Funktion wird rekursiv aufgerufen, dabei wird die startBlockNo angegeben (anfangs der Rootknoten)
// Ziel ist es, die Funktion so lange rekursiv aufzufrufen, bis man einen Blattknoten erreicht hat
ReturnInsertValue
DBMyIndex::insertValue(BlockNo startBlockNo, const DBAttrType &value, const TID &tid, BlockNo parentBlockNo) {
    //std::cout << "INSERT VALUE " << value.toString("") << " StartBlock: " << startBlockNo << std::endl;
    //int value = ((DBIntType *)val)->getVal();
    DBBACB rootB = bufMgr.fixBlock(file, startBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *treeBlock = (TreeBlock *) rootB.getDataPtr();
    // Falls der Block ein Blattknoten ist --> Wert einfügen
    if (treeBlock->leaf) {
        TreeLeafBlock *treeLeafBlock = getLeafBlockFromDBBACB(rootB); //don't forget to delete
        bool split = treeLeafBlock->insertTID(value, tid);

        if (!split) {
            //std::cout << "kein Split notwendig" << std::endl;
            treeLeafBlock->copyBlockToDBBACB(rootB);
            bufMgr.unfixBlock(rootB);
            delete treeLeafBlock;
            return ReturnInsertValue(0, DBIntType(0), 0, attrType);
        }


        // Der Blattknoten, in dem man den neuen Wert eingefügt hat, ist zu voll --> Split des Blattknoten notwendig
        if (split) {
            //std::cout << " SPLITEN" << std::endl;
            
            // Neuer Blattknoten, auf dem Haelfte der Werte uebertragen werden, die beim alten Blattknoten geloescht werden
            DBBACB newLeafBlock = bufMgr.fixNewBlock(file);
            //DBBACB newLeafBlock = fixNewBlock();
            TreeLeafBlock * newTreeLeafBlock = treeLeafBlock->splitBlock(newLeafBlock.getBlockNo()); //don't forget to delete

            BlockNo left = treeLeafBlock->blockNo;
            BlockNo right = newTreeLeafBlock->blockNo;
            DBAttrType * newValue = treeLeafBlock->getValue(treeLeafBlock->currentValueCounter - 1); //don't forget to delete

            // Falls ein Split gemacht wurde und es keinen Elternknoten gibt --> Elternknoten erstellen + diesen als Root markieren
            if (parentBlockNo == 0) {
                DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                // Neuer Elternknoten und diesem Pointer + Werte uebergeben
                DBBACB newInnerBlock = bufMgr.fixNewBlock(file);
                //DBBACB newInnerBlock = fixNewBlock();
                TreeInnerBlock * newRoot = createNewRoot(newInnerBlock.getBlockNo()); //don't forget to delete!
                newRoot->insertBlockNo(left, *newValue, right, true);

                startBlock->rootBlockNo = newRoot->blockNo;

                //Alle änderungen speichern
                startBlock->copyBlockToDBBACB(metaB);
                newRoot->copyBlockToDBBACB(newInnerBlock);
                newTreeLeafBlock->copyBlockToDBBACB(newLeafBlock);
                treeLeafBlock->copyBlockToDBBACB(rootB);
                
                delete newRoot;
                delete newTreeLeafBlock;
                delete treeLeafBlock;
                delete newValue;
                
                //std::cout << " Meta Block anpassen" << std::endl;
                bufMgr.unfixBlock(newInnerBlock);
                bufMgr.unfixBlock(metaB);
                bufMgr.unfixBlock(newLeafBlock);
                bufMgr.unfixBlock(rootB);

                return ReturnInsertValue(0, DBIntType(0), 0, attrType);
            }

            // Andernfalls: Es gibt Elternknoden --> Diesem Pointer + Value + Pointer uebergeben
            // Elternknoten fügt Pointer+Value+Pointer hinzu (ggf. kann dort wieder ein Split entstehen)
            //std::cout << " Eltern knoten muss sich um einfuegen kuemmern" << std::endl;

            newTreeLeafBlock->copyBlockToDBBACB(newLeafBlock);
            treeLeafBlock->copyBlockToDBBACB(rootB);
            
            delete newTreeLeafBlock;
            delete treeLeafBlock;
            
            bufMgr.unfixBlock(newLeafBlock);
            bufMgr.unfixBlock(rootB);

            ReturnInsertValue r = ReturnInsertValue(left, *newValue, right, attrType);
            delete newValue;
            return r;
        }

    }
    // Falls der aufgerufene Block kein Blattknoten ist --> Passende BlockNo raussuchen und Funktion rekursiv aufrufen
    if (!treeBlock->leaf) {

        TreeInnerBlock *treeInnerBlock = getInnerBlockFromDBBACB(rootB); //don't forget to delete

        ReturnInsertValue r = ReturnInsertValue(0, DBIntType(0), 0, attrType);
        for (int i = 0; i < treeInnerBlock->currentValueCounter; i++) {
            // Falls der einzufügende Wert größer ist als der Wert ganz rechts im inneren Knoten --> Letzte BlockNo aufrufen
            // value <= treeInnerBlock->values[i]
            if (treeInnerBlock->compare(i,value) >= 0) {
                r = insertValue(treeInnerBlock->getBlockNo(i), value, tid, treeInnerBlock->blockNo);
                break;
            }
            // Nach der richtigen BlockNo für den einzufügenden Wert suchen
            // value > treeInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]
            if (treeInnerBlock->compare(treeInnerBlock->currentValueCounter-1, value) == -1) {
                r = insertValue(treeInnerBlock->getBlockNo(treeInnerBlock->currentValueCounter),
                                value, tid, treeInnerBlock->blockNo);
                break;
            }
        }

        // Wenn der untere Knoten ein BlockNo + Value + BlockNo nach oben gibt, dann muss der Wert + 2 Pointer (=BlockNos) zum inneren Konto hinzugefügt werden
        if (!r.blockNoRight == 0 && !r.blockNoLeft == 0) {
            // Neuen Wert hinzufugen, da Blattknoten gesplitet wurde
            
            bool split = treeInnerBlock->insertBlockNo(r.blockNoLeft, r.getValue(attrType), r.blockNoRight, false);
            //std::cout << " Elternknoten war fleisig und hat neuen Knoten eingeuefgt" << std::endl;

            // Nach dem Einfügen der BlockNo + Value + BlockNo muss mit Pech der Elternknoten gesplitet werden
            if (split) {
                // Split des Elternknoten
                //std::cout << "Elternknoten muss gesplitet werden" << std::endl;
                
                DBBACB newInnerBlock = bufMgr.fixNewBlock(file);
                //DBBACB newInnerBlock = fixNewBlock();
                TreeInnerBlock * newTreeInnerBlock = treeInnerBlock->splitBlock(newInnerBlock.getBlockNo()); //don't forget to delete

                DBAttrType * newParentValue = treeInnerBlock->getValue(treeInnerBlock->currentValueCounter);
                BlockNo leftBlockNo = treeInnerBlock->blockNo;
                BlockNo rightBlockNo = newTreeInnerBlock->blockNo;

                newTreeInnerBlock->copyBlockToDBBACB(newInnerBlock);
                bufMgr.unfixBlock(newInnerBlock);

                treeInnerBlock->copyBlockToDBBACB(rootB);
                bufMgr.unfixBlock(rootB);

                // Falls es keinen Eltern-Elternknoten gibt --> Neuen Knoten erstellen + diesen als Root markieren
                if (parentBlockNo == 0) {

                    // Neuen Root Knoten erstellen und Werte uebertragen (Pointer, Value, Pointer)
                    DBBACB newRootBlock = bufMgr.fixNewBlock(file);
                    //DBBACB newRootBlock = fixNewBlock();
                    TreeInnerBlock * newRoot = createNewRoot(newRootBlock.getBlockNo()); //don't forget to delete
                    newRoot->insertBlockNo(leftBlockNo,*newParentValue,rightBlockNo,true);

                    newRoot->copyBlockToDBBACB(newRootBlock);
                    bufMgr.unfixBlock(newRootBlock);

                    //newTreeRootBlock.printAllValues();

                    //std::cout << " Meta Daten anpassen, da neuer Root Block" << std::endl;

                    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                    startBlock->rootBlockNo = newRoot->blockNo;
                    startBlock->copyBlockToDBBACB(metaB);
                    bufMgr.unfixBlock(metaB);
                    
                    
                    delete treeInnerBlock;
                    delete newTreeInnerBlock;
                    delete newRoot;
                    delete newParentValue;
                    
                    return ReturnInsertValue(0, DBIntType(0), 0, attrType);

                }
                // Falls es noch Eltern-Elternknoten gibt --> Diesem Pointer + Value + Pointer übergeben
                delete treeInnerBlock;
                delete newTreeInnerBlock;
                ReturnInsertValue r = ReturnInsertValue(leftBlockNo, *newParentValue, rightBlockNo, attrType);
                delete newParentValue;
                return r;

            } else {
                treeInnerBlock->copyBlockToDBBACB(rootB);
                bufMgr.unfixBlock(rootB);
                delete treeInnerBlock;
                return ReturnInsertValue(0, DBIntType(0), 0, attrType);
                // Elternknoten hat neuen Wert bekommen, ist aber nicht voll -> Kein Split bei Eltern-Elternknoten notwendig
            }
        } else {
            // Blattknoten hat kein Split gemacht, also muss Elternknoten auch nicht gesplitet werden

            //treeInnerBlock->copyBlockToDBBACB(rootB);
            bufMgr.unfixBlock(rootB);
            delete treeInnerBlock;
            return ReturnInsertValue(0, DBIntType(0), 0, attrType);
        }
    }
}


uint TreeBlock::calculateMaxCounter(enum AttrTypeEnum attrType, bool leaf) {
    uint blockSize = DBFileBlock::getBlockSize();
    uint tidSize = sizeof(TID);
    uint boolSize = sizeof(bool);
    uint blockNoSize = sizeof(BlockNo);
    uint intSize = sizeof(int);
    uint doubleSize = sizeof(double);
    uint varCharSize = (MAX_STR_LEN + 1) * sizeof(char);

    uint intPointerSize = sizeof(int *);
    uint doublePointerSize = sizeof(double *);
    uint charPointerSize = sizeof(char *);
    uint blockNoPointerSize = sizeof(BlockNo *);
    uint tidPointerSize = sizeof(TID *);

    uint basicSize = boolSize + 2* blockNoSize + 2 * intSize;

    if (leaf) {
        if (AttrTypeEnum::INT == attrType) {
            //return 40;
            return std::floor((blockSize - basicSize - tidPointerSize - intPointerSize) / (intSize + tidSize));
        }
        if (AttrTypeEnum::DOUBLE == attrType) {
            return std::floor(
                    (blockSize - basicSize - tidPointerSize - doublePointerSize) / (doubleSize + tidSize));
        }
        if (AttrTypeEnum::VCHAR == attrType) {
            return 4;
            //return std::floor((blockSize - basicSize - tidPointerSize - charPointerSize) / (varCharSize + tidSize));
        }
    } else {
        if (AttrTypeEnum::INT == attrType) {
            // 4;
            return std::floor((blockSize - basicSize - blockNoSize - intPointerSize - blockNoPointerSize) /
             (intSize + blockNoSize));
        }
        if (AttrTypeEnum::DOUBLE == attrType) {
            return std::floor((blockSize - basicSize - blockNoSize - doublePointerSize - blockNoPointerSize) /
                              (doubleSize + blockNoSize));
        }
        if (AttrTypeEnum::VCHAR == attrType) {
            return 4;
            //return std::floor((blockSize - basicSize - blockNoSize - charPointerSize - blockNoPointerSize) /
              //               (varCharSize + blockNoSize));
        }


    }


}

void DBMyIndex::unfixBACBs(bool setDirty) {
    LOG4CXX_INFO(logger, "unfixBACBs()");
    LOG4CXX_DEBUG(logger, "setDirty: " + TO_STR(setDirty));


}


/**
 * Erstelle Indexdatei.
 */
void DBMyIndex::initializeIndex() {
    LOG4CXX_INFO(logger, "initializeIndex()");
    if (bufMgr.getBlockCnt(file) != 0)
        throw DBIndexException("can not initializie exisiting table");
}


/**
 * Sucht im Index nach einem bestimmten Wert
 * - const DBAttrType & val: zu suchender Schluesselwert
 * - DBListTID & tids: Referenz auf Liste von TID Objekten
 * Rueckgabewert: KEINER, aber die Funktion aendert die uebergebene Liste
 * von TID Objekten (siehe DBTypes.h: typedef list<TID> DBListTID;)
 */
void DBMyIndex::find(const DBAttrType &val, DBListTID &tids) {
    LOG4CXX_INFO(logger, "find()");
    //LOG4CXX_DEBUG(logger, "val:\n" + val.toString("\t"));

    //DBIntType *t = (DBIntType *) &val;
    //std::cout << " find " << t->getVal() << std::endl;
    findTIDsFirstCall(val, tids);
    //printAllBlocks();
}

void DBMyIndex::findTIDsFirstCall(const DBAttrType &val, DBListTID &tids) {
    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
    bufMgr.unfixBlock(metaB);
    //printAllBlocks();
    findTIDs(startBlock->rootBlockNo, val, tids, 0);
}

void
DBMyIndex::findTIDs(BlockNo startBlockNo, const DBAttrType &val, DBListTID &tids, BlockNo parentBlockNo) {
    //std::cout << "FIND VALUE " << val.toString("\t") << " StartBlock: " << startBlockNo << std::endl;


    DBBACB rootB = bufMgr.fixBlock(file, startBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *treeBlock = (TreeBlock *) rootB.getDataPtr();

    if (treeBlock->leaf) {

        TreeLeafBlock *treeLeafBlock = getLeafBlockFromDBBACB(rootB); //don't forget to delete
        for (int i = 0; i < treeLeafBlock->currentValueCounter; i++) {
            if (treeLeafBlock->compare(i,val) == 0) {
                tids.push_back(treeLeafBlock->getTID(i));
            }
        }
        bufMgr.unfixBlock(rootB);
        delete treeLeafBlock;
        return;
    }

    if (!treeBlock->leaf) {
        TreeInnerBlock *treeInnerBlock = getInnerBlockFromDBBACB(rootB); //don't forget to delete

        ReturnInsertValue r = ReturnInsertValue(0, DBIntType(0), 0, attrType);
        
        // val > values[currentValueCounter-1]
        if(treeInnerBlock->compare(treeInnerBlock->currentValueCounter-1, val) == -1){
            findTIDs(treeInnerBlock->getBlockNo(treeInnerBlock->currentValueCounter), val, tids, treeInnerBlock->blockNo);
            bufMgr.unfixBlock(rootB);
            delete treeInnerBlock;
            return;
        }
        
        for (int i = 0; i < treeInnerBlock->currentValueCounter; i++) {
            // val <= values[i]
            if (treeInnerBlock->compare(i, val) >= 0){
                findTIDs(treeInnerBlock->getBlockNo(i), val, tids, treeInnerBlock->blockNo);
                bufMgr.unfixBlock(rootB);
                delete treeInnerBlock;
                return;
            }
        }
    }
}

/**
 * Einfuegen eines Schluesselwertes (moeglicherweise bereits vorhangen)
 * zusammen mit einer Referenz auf eine TID.
 */
void DBMyIndex::insert(const DBAttrType &val, const TID &tid) {
    //std::cout << " insert " << val.toString() << " und tid " << tid.toString() << std::endl;
    insertValueFirstCall(val, tid);
    //printAllBlocks();


}

TreeInnerBlock * DBMyIndex::createNewRoot(BlockNo blockNo){
    switch(attrType){
        case INT:{
            return new TreeIntInnerBlock(blockNo);
        }
        
        case VCHAR:{
            return new TreeVarCharInnerBlock(blockNo);
        }
        case DOUBLE:{
            return new TreeDoubleInnerBlock(blockNo);
        }
        
        default:{
            return (TreeInnerBlock *) nullptr;
        }
    }
}

/**
 * Entfernt alle Tupel aus der Liste der tids.
 * Um schneller auf der richtigen Seite mit dem Entfernen anfangen zu koennen,
 * wird zum Suchen auch noch der zu loeschende value uebergeben
 */
void DBMyIndex::remove(const DBAttrType &val, const list<TID> &tid) {
    LOG4CXX_INFO(logger, "remove()");
    LOG4CXX_DEBUG(logger, "val:\n" + val.toString("\t"));

    removeValueFirstCall(val,tid.back());
}


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
extern "C"
void *createDBMyIndex(int nArgs, va_list &ap) {
    // Genau 5 Parameter
    if (nArgs != 5) {
        throw DBException("Invalid number of arguments");
    }
    DBBufferMgr *bufMgr = va_arg(ap, DBBufferMgr *);
    DBFile *file = va_arg(ap, DBFile *);
    enum AttrTypeEnum attrType = (enum AttrTypeEnum) va_arg(ap, int);
    ModType m = (ModType) va_arg(ap, int);
    bool unique = (bool) va_arg(ap, int);
    return new DBMyIndex(*bufMgr, *file, attrType, m, unique);
}

void TreeStartBlock::copyBlockToDBBACB(DBBACB d) {
    memmove(d.getDataPtr(), (void *) &this->blockNo, 3 * sizeof(BlockNo));
    d.setModified();
}

/*
 * INT INNER BLOCK
 */

void TreeIntInnerBlock::copyBlockToDBBACB(DBBACB d) {
    //std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = sizeof(bool) + 2*sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);

    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(int));

    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *) +
           this->maxValueCounter * sizeof(int),
           (void *) this->blockNos, (this->maxValueCounter + 1) * sizeof(BlockNo));
    d.setModified();
}

void TreeIntInnerBlock::copyDBBACBToBlock(DBBACB d){
    int basicSize = sizeof(bool) + 2*sizeof(BlockNo) + 2 * sizeof(int);
    memcpy((void *) &this->leaf, d.getDataPtr(), basicSize);
    memcpy((void *) this->values, d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *),
    this->maxValueCounter * sizeof(int));
    memcpy((void *) this->blockNos, d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *)
    + this->maxValueCounter * sizeof(int), (this->maxValueCounter + 1) * sizeof(BlockNo));
}

void TreeIntInnerBlock::updatePointers() {
    int basicSize = sizeof(bool) + 2*sizeof(BlockNo) + 2 * sizeof(int);
    values = (int *) (&this->leaf + basicSize + sizeof(int *) + sizeof(BlockNo *));
    blockNos = (BlockNo *) (&this->leaf + basicSize + sizeof(int *) + sizeof(BlockNo *) +
                            (this->maxValueCounter) * sizeof(int));
}

int TreeIntInnerBlock::removeBlockNo(BlockNo blockNo) {
    for (int i = 0; i < currentValueCounter + 1; i++) {

    }
}

/*
void TreeIntInnerBlock::insertBlockNo(BlockNo blockNoLeft, int value, BlockNo blockNoRight) {
    this->values[0] = value;
    this->blockNos[0] = blockNoLeft;
    this->blockNos[1] = blockNoRight;
    this->currentValueCounter++;
}
*/

bool TreeIntInnerBlock::insertBlockNo(BlockNo blockNoLeft, const DBAttrType &value, BlockNo blockNoRight, bool root){
    DBIntType * val = (DBIntType *) &value;
    int int_val = val->getVal();
    if(root) {
        this->values[0] = int_val;
        this->blockNos[0] = blockNoLeft;
        this->blockNos[1] = blockNoRight;
        this->currentValueCounter++;
        return false;
    }
    else{
        // Prüfen, wo man BlockNo + Value + BlockNo hinzufügen soll
        for (int i = 0; i < this->currentValueCounter; i++) {
            //value > this->values[this->currentValueCounter - 1])
            if(this->compare(this->currentValueCounter-1, value) == -1){
                // std::cout << "Fall 1" << std::endl;
                this->setValue(this->currentValueCounter, value);
                this->setBlockNo(this->currentValueCounter, blockNoLeft);
                this->setBlockNo(this->currentValueCounter+1, blockNoRight);
                break;
            }
            //value <= this->values[i]
            if(this->compare(i, value) >= 0){
                memmove(&this->values[i + 1], &this->values[i],
                       sizeof(int) * (this->currentValueCounter - i));
                memmove(&this->blockNos[i + 2], &this->blockNos[i + 1],
                       sizeof(BlockNo) * (this->currentValueCounter - i));
                this->blockNos[i] = blockNoLeft;
                this->blockNos[i + 1] = blockNoRight;
                this->values[i] = int_val;
                break;
            }
        }
        this->currentValueCounter++;
        if(this->currentValueCounter >= this->maxValueCounter){
            return true;
        }
        else{
            return false;
        }
    }
}


bool TreeIntInnerBlock::insertBlockNo(int value, BlockNo blockNo) {
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->blockNos[currentValueCounter + 1] = blockNo;
            this->currentValueCounter++;
            //std::cout << this->values[currentValueCounter - 1] << std::endl;
            //std::cout << "am ende der Liste einfuegen" << std::endl;
        } else {
            // Mitten in der Liste hinzufügen
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memmove(&values[i + 1], &values[i], (currentValueCounter - i + 1) * sizeof(int));
                    memmove(&blockNos[i + 1], &blockNos[i], (currentValueCounter - i + 1) * sizeof(BlockNo));
                    values[i] = value;
                    blockNos[i] = blockNo;
                    this->currentValueCounter++;
                    break;
                }
            }
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn leer)
        this->values[0] = value;
        this->blockNos[0] = blockNo;
        this->currentValueCounter++;
    }

    if (this->currentValueCounter >= this->maxValueCounter) {
        return true;
    }
    return false;
}

void TreeIntInnerBlock::printAllValues() {
    std::cout << "Block No Inner: " << this->blockNo << " -> ";
    if (this->currentValueCounter > 0) {
        std::cout << "(" << this->blockNos[0] << ") ";
        for (int i = 0; i < this->currentValueCounter; i++) {
            std::cout << this->values[i] << " (" << this->blockNos[i + 1] << ") ";
        }
        std::cout << endl;
    }
}

int TreeIntInnerBlock::compare(int index, const DBAttrType &val){
    DBIntType *t = (DBIntType *) &val;
    int value = t->getVal();
    if(this->values[index] > value){
        return 1;
    }
    else if(this->values[index] == value){
        return 0;
    }
    else{
        return -1;
    }
}

DBAttrType * TreeIntInnerBlock::getValue(int index){
    int value = this->values[index];
    return new DBIntType(value);
}

void TreeIntInnerBlock::setValue(int index, const DBAttrType &value){
    DBIntType * int_value = (DBIntType *) &value;
    this->values[index] = int_value->getVal();
}

BlockNo TreeIntInnerBlock::getBlockNo(int index){
    return this->blockNos[index];
}

void TreeIntInnerBlock::setBlockNo(int index, BlockNo blockNo){
    this->blockNos[index] = blockNo;
}

TreeInnerBlock * TreeIntInnerBlock::splitBlock(BlockNo blockNo){
    int offset = std::ceil((this->maxValueCounter) / 2);
    TreeIntInnerBlock * newTreeIntInnerBlock = new TreeIntInnerBlock(blockNo);
    memcpy((void *) newTreeIntInnerBlock->values,
           (void *) &(this->values[offset]),
           sizeof(int) * (this->maxValueCounter - offset));
    
    memcpy((void *) newTreeIntInnerBlock->blockNos,
           (void *) &(this->blockNos[offset]),
           sizeof(BlockNo) * (this->maxValueCounter - offset + 1));
    
    this->currentValueCounter = offset - 1;
    newTreeIntInnerBlock->currentValueCounter = this->maxValueCounter - offset;
    
    return newTreeIntInnerBlock;
}

bool TreeIntInnerBlock::insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool fromLeft){
    DBIntType * int_value = (DBIntType *) &val;
    int value = int_value->getVal();
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->blockNos[currentValueCounter + 1] = blockNo;
            this->currentValueCounter++;
            //std::cout << this->values[currentValueCounter - 1] << std::endl;
            //std::cout << "am ende der Liste einfuegen" << std::endl;
        } else {
            //std::cout << " FALL 2" << std::endl;
            // Mitten in der Liste hinzufügen
            //this->printAllValues();
            //std::cout << "Einfuegen von value" << value << std::endl;
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memmove(&values[i + 1], &values[i], (currentValueCounter - i) * sizeof(int));
                    memmove(&blockNos[i + 1], &blockNos[i], (currentValueCounter - i + 1) * sizeof(BlockNo));
                    values[i] = value;
                    blockNos[i] = blockNo;
                    this->currentValueCounter++;
                    break;
                }

            }
            //this->printAllValues();
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn nur ein BlockPointer vorhanden ist)
        //std::cout << "Liste Leer" << std::endl;
        if (fromLeft) {
            this->values[0] = value;
            this->blockNos[1] = this->blockNos[0];
            this->blockNos[0] = blockNo;
            this->currentValueCounter++;
        } else {
            this->values[0] = value;
            this->blockNos[1] = blockNo;
            this->currentValueCounter++;
        }

    }

    if (this->currentValueCounter >= this->maxValueCounter) {
        return true;
    }
    return false;
}

ValueAndTIDPair TreeIntInnerBlock::removeSmallestBlockNo() {
    //this->printAllValues();
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBIntType(this->values[0]), this->blockNos[0], true, INT);

        memmove(&values[0], &values[1], (this->currentValueCounter-1) * sizeof(int));
        memmove(&blockNos[0], &blockNos[1], (this->currentValueCounter) * sizeof(BlockNo));
        this->currentValueCounter--;

        //this->printAllValues();
        return returnPair;
    }
    return ValueAndTIDPair(DBIntType(0), BlockNo(), false, INT);
}

ValueAndTIDPair TreeIntInnerBlock::removeBiggestBlockNo() {
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBIntType(this->values[this->currentValueCounter - 1]),
                                                           this->blockNos[this->currentValueCounter], true,
                                                           DBIntType(this->values[this->currentValueCounter - 2]), INT);
        this->currentValueCounter--;
        return returnPair;
    }
    return ValueAndTIDPair(DBIntType(0), BlockNo(), false, INT);
}




TreeInnerBlock::~TreeInnerBlock(){}
TreeLeafBlock::~TreeLeafBlock(){}

/*
 * INT LEAF BLOCK
 */

void TreeIntLeafBlock::copyBlockToDBBACB(DBBACB d) {
    // std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);
    memcpy(d.getDataPtr() + basicSize + sizeof(int *)+ sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(int));
    memcpy(d.getDataPtr() + basicSize + sizeof(int *)+ sizeof(BlockNo *)+
           this->maxValueCounter * sizeof(int),
           (void *) this->tids, this->maxValueCounter * sizeof(TID));
    d.setModified();
}

void TreeIntLeafBlock::copyDBBACBToBlock(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(&this->leaf, d.getDataPtr(), basicSize);
    memcpy((void *) this->values, d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *),
          this->maxValueCounter * sizeof(int));
    memcpy((void *) this->tids, d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *) +
    this->maxValueCounter * sizeof(int),
           this->maxValueCounter * sizeof(TID));
}

void TreeIntLeafBlock::updatePointers() {
    int basicSize = sizeof(bool) + 2*sizeof(BlockNo) + 2 * sizeof(int);
    values = (int *) (&this->leaf + basicSize + sizeof(int *) + sizeof(BlockNo *));
    tids = (TID *) (&this->leaf + basicSize + sizeof(int *) + sizeof(TID *) +
                    (this->maxValueCounter) * sizeof(int));
}

bool TreeIntLeafBlock::insertTID(const DBAttrType &val, TID tid) {
    DBIntType * int_val = (DBIntType *) &val;
    int value = int_val->getVal();
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->tids[currentValueCounter] = tid;
            this->currentValueCounter++;
        } else {
            // Mitten in der Liste hinzufügen
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memmove(&values[i + 1], &values[i], (currentValueCounter - i) * sizeof(int));
                    memmove(&tids[i + 1], &tids[i], (currentValueCounter - i) * sizeof(TID));
                    values[i] = value;
                    tids[i] = tid;
                    this->currentValueCounter++;
                    break;
                }
            }
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn leer)
        this->values[0] = value;
        this->tids[0] = tid;
        this->currentValueCounter++;
    }

    if (this->currentValueCounter >= this->maxValueCounter) {
        return true;
    }
    return false;
}

UndersizedAndValuePair TreeIntLeafBlock::removeTID(const DBAttrType &val, TID tid) {
    // Am Ende der Liste löschen
    int value = ((DBIntType *) &val)->getVal();
    if (value == this->values[currentValueCounter - 1]) {
        this->currentValueCounter--;
    } else {
        // Mitten in der Liste löschen
        for (int i = 0; i < this->currentValueCounter; i++) {
            if (this->values[i] == value) {
                memmove(&values[i], &values[i + 1], (currentValueCounter - i) * sizeof(int));
                memmove(&tids[i], &tids[i + 1], (currentValueCounter - i) * sizeof(TID));
                this->currentValueCounter--;
                break;
            }
        }
    }
    std::cout << "this->values(0) " << this->values[0] << std::endl;

    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue) / maxValue;

    if (divisionResult >= 0.5) {
        UndersizedAndValuePair r = UndersizedAndValuePair(DBIntType(this->values[this->currentValueCounter-1]), false, INT);
        return r;
    }
    if(this->currentValueCounter == 0){
        UndersizedAndValuePair r = UndersizedAndValuePair(DBIntType(0), true, INT);
        return r;
    }
    UndersizedAndValuePair r = UndersizedAndValuePair(DBIntType(this->values[this->currentValueCounter-1]), true, INT);
    return r;
}

DBAttrType * TreeIntLeafBlock::getValue(int index){
    int value = this->values[index];
    return new DBIntType(value);
}

void TreeIntLeafBlock::setValue(int index, const DBAttrType &val){
    DBIntType * int_val = (DBIntType *) &val;
    this->values[index] = int_val->getVal();
}

TID TreeIntLeafBlock::getTID(int index){
    return this->tids[index];
}

void TreeIntLeafBlock::setTID(int index, TID tid){
    this->tids[index] = tid;
}

int TreeIntLeafBlock::compare(int index, const DBAttrType &val){
    DBIntType *t = (DBIntType *) &val;
    int value = t->getVal();
    if(this->values[index] > value){
        return 1;
    }
    else if(this->values[index] == value){
        return 0;
    }
    else{
        return -1;
    }
}

TreeLeafBlock * TreeIntLeafBlock::splitBlock(BlockNo blockNo){
        int offset = std::ceil((this->maxValueCounter)/2);
        TreeIntLeafBlock *newIntLeafBlock = new TreeIntLeafBlock(blockNo);
        memcpy((void *) newIntLeafBlock->values, &(this->values[offset]), sizeof(int) * (this->maxValueCounter - offset));
        memcpy((void *) newIntLeafBlock->tids, &(this->tids[offset]), sizeof(TID) * (this->maxValueCounter - offset));
        this->currentValueCounter = offset;
        newIntLeafBlock->currentValueCounter = this->maxValueCounter - offset;
        return newIntLeafBlock;
}

ValueAndTIDPair TreeIntLeafBlock::removeSmallestTID() {
    //this->printAllValues();
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBIntType(this->values[0]), this->tids[0], true, INT);

        memmove(&values[0], &values[1], (this->currentValueCounter-1) * sizeof(int));
        memmove(&tids[0], &tids[1], (this->currentValueCounter-1) * sizeof(TID));
        this->currentValueCounter--;

        //this->printAllValues();
        return returnPair;
    }
    return ValueAndTIDPair(DBIntType(0), TID(), false, INT);
}

ValueAndTIDPair TreeIntLeafBlock::removeBiggestTID() {
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBIntType(this->values[this->currentValueCounter - 1]),
                                                           this->tids[this->currentValueCounter - 1], true,
                                                           DBIntType(this->values[this->currentValueCounter - 2]), INT);
        this->currentValueCounter--;
        return returnPair;
    }
    return ValueAndTIDPair(DBIntType(0), TID(), false, INT);
}


void TreeIntLeafBlock::printAllValues() {
    std::cout << "Block No Leaf: " << this->blockNo << " -> ";
    for (int i = 0; i < this->currentValueCounter; i++) {
        //std::cout << this->values[i]<<":"<<this->tids[i].toString() << " ";
        std::cout << this->values[i] << " ";
    }
    std::cout << endl;
}



/*
 * DOUBLE INNER BLOCK
 */

int TreeDoubleInnerBlock::compare(int index, const DBAttrType &val){
    DBDoubleType *t = (DBDoubleType *) &val;
    double value = t->getVal();
    if(this->values[index] > value){
        return 1;
    }
    else if(this->values[index] == value){
        return 0;
    }
    else{
        return -1;
    }
}

DBAttrType * TreeDoubleInnerBlock::getValue(int index){
    double value = this->values[index];
    return new DBDoubleType(value);
}

void TreeDoubleInnerBlock::setValue(int index, const DBAttrType &value){
    DBDoubleType * double_value = (DBDoubleType *) &value;
    this->values[index] = double_value->getVal();
}


void TreeDoubleInnerBlock::copyDBBACBToBlock(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy((void *) &this->leaf, d.getDataPtr(), basicSize);
    memcpy((void *) this->values, d.getDataPtr() + basicSize + sizeof(double *) + sizeof(BlockNo *),
           this->maxValueCounter * sizeof(double));
    memcpy((void *) this->blockNos, d.getDataPtr() + basicSize + sizeof(double *) + sizeof(BlockNo *)
                                    + this->maxValueCounter * sizeof(double), (this->maxValueCounter + 1) * sizeof(BlockNo));
}
void TreeDoubleInnerBlock::copyBlockToDBBACB(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);
    
    memcpy(d.getDataPtr() + basicSize + sizeof(double *) + sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(double));
    
    memcpy(d.getDataPtr() + basicSize + sizeof(double *) + sizeof(BlockNo *) +
           this->maxValueCounter * sizeof(double),
           (void *) this->blockNos, (this->maxValueCounter + 1) * sizeof(BlockNo));
    d.setModified();
}
void TreeDoubleInnerBlock::updatePointers(){};

int TreeDoubleInnerBlock::removeBlockNo(BlockNo blockNo){return -1;}

void TreeDoubleInnerBlock::printAllValues(){
    std::cout << "Block No Inner: " << this->blockNo << " -> ";
    if (this->currentValueCounter > 0) {
        std::cout << "(" << this->blockNos[0] << ") ";
        for (int i = 0; i < this->currentValueCounter; i++) {
            std::cout << this->values[i] << " (" << this->blockNos[i + 1] << ") ";
        }
        std::cout << endl;
    }
}

//void insertBlockNo(BlockNo blockNoLeft, double value, BlockNo blockNoRight);
bool TreeDoubleInnerBlock::insertBlockNo(BlockNo blockNoLeft, const DBAttrType &value, BlockNo blockNoRight, bool root){
    DBDoubleType * val = (DBDoubleType *) &value;
    double double_val = val->getVal();
    if(root) {
        this->values[0] = double_val;
        this->blockNos[0] = blockNoLeft;
        this->blockNos[1] = blockNoRight;
        this->currentValueCounter++;
        return false;
    }
    else{
        // Prüfen, wo man BlockNo + Value + BlockNo hinzufügen soll
        for (int i = 0; i < this->currentValueCounter; i++) {
            //value > this->values[this->currentValueCounter - 1])
            if(this->compare(this->currentValueCounter-1, value) == -1){
                // std::cout << "Fall 1" << std::endl;
                this->setValue(this->currentValueCounter, value);
                this->setBlockNo(this->currentValueCounter, blockNoLeft);
                this->setBlockNo(this->currentValueCounter+1, blockNoRight);
                break;
            }
            //value <= this->values[i]
            if(this->compare(i, value) >= 0){
                memmove(&this->values[i + 1], &this->values[i],
                        sizeof(double) * (this->currentValueCounter - i));
                memmove(&this->blockNos[i + 2], &this->blockNos[i + 1],
                        sizeof(BlockNo) * (this->currentValueCounter - i));
                this->blockNos[i] = blockNoLeft;
                this->blockNos[i + 1] = blockNoRight;
                this->values[i] = double_val;
                break;
            }
        }
        this->currentValueCounter++;
        if(this->currentValueCounter >= this->maxValueCounter){
            return true;
        }
        else{
            return false;
        }
    }
}

BlockNo TreeDoubleInnerBlock::getBlockNo(int index){
    return this->blockNos[index];
    
}
void TreeDoubleInnerBlock::setBlockNo(int index, BlockNo blockNo){
    this->blockNos[index] = blockNo;
}

TreeInnerBlock * TreeDoubleInnerBlock::splitBlock(BlockNo blockNo){
    int offset = std::ceil((this->maxValueCounter) / 2);
    TreeDoubleInnerBlock * newTreeDoubleInnerBlock = new TreeDoubleInnerBlock(blockNo);
    memcpy((void *) newTreeDoubleInnerBlock->values,
           (void *) &(this->values[offset]),
           sizeof(double) * (this->maxValueCounter - offset));
    
    memcpy((void *) newTreeDoubleInnerBlock->blockNos,
           (void *) &(this->blockNos[offset]),
           sizeof(BlockNo) * (this->maxValueCounter - offset + 1));
    
    this->currentValueCounter = offset - 1;
    newTreeDoubleInnerBlock->currentValueCounter = this->maxValueCounter - offset;
    
    return newTreeDoubleInnerBlock;
}

bool TreeDoubleInnerBlock::insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool fromLeft){
    DBDoubleType * double_value = (DBDoubleType *) &val;
    double value = double_value->getVal();
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->blockNos[currentValueCounter + 1] = blockNo;
            this->currentValueCounter++;
            //std::cout << this->values[currentValueCounter - 1] << std::endl;
            //std::cout << "am ende der Liste einfuegen" << std::endl;
        } else {
            //std::cout << " FALL 2" << std::endl;
            // Mitten in der Liste hinzufügen
            //this->printAllValues();
            //std::cout << "Einfuegen von value" << value << std::endl;
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memmove(&values[i + 1], &values[i], (currentValueCounter - i) * sizeof(double));
                    memmove(&blockNos[i + 1], &blockNos[i], (currentValueCounter - i + 1) * sizeof(BlockNo));
                    values[i] = value;
                    blockNos[i] = blockNo;
                    this->currentValueCounter++;
                    break;
                }

            }
            //this->printAllValues();
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn nur ein BlockPointer vorhanden ist)
        //std::cout << "Liste Leer" << std::endl;
        if (fromLeft) {
            this->values[0] = value;
            this->blockNos[1] = this->blockNos[0];
            this->blockNos[0] = blockNo;
            this->currentValueCounter++;
        } else {
            this->values[0] = value;
            this->blockNos[1] = blockNo;
            this->currentValueCounter++;
        }

    }

    if (this->currentValueCounter >= this->maxValueCounter) {
        return true;
    }
    return false;
}

ValueAndTIDPair TreeDoubleInnerBlock::removeSmallestBlockNo() {
    //this->printAllValues();
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBDoubleType(this->values[0]), this->blockNos[0], true, DOUBLE);

        memmove(&values[0], &values[1], (this->currentValueCounter-1) * sizeof(double));
        memmove(&blockNos[0], &blockNos[1], (this->currentValueCounter) * sizeof(BlockNo));
        this->currentValueCounter--;

        //this->printAllValues();
        return returnPair;
    }
    return ValueAndTIDPair(DBDoubleType(0.0), BlockNo(), false, DOUBLE);
}

ValueAndTIDPair TreeDoubleInnerBlock::removeBiggestBlockNo() {
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBDoubleType(this->values[this->currentValueCounter - 1]),
                                                           this->blockNos[this->currentValueCounter], true,
                                                           DBDoubleType(this->values[this->currentValueCounter - 2]), DOUBLE);
        this->currentValueCounter--;
        return returnPair;
    }
    return ValueAndTIDPair(DBDoubleType(0.0), BlockNo(), false, DOUBLE);
}



/*
 * DOUBLE LEAF BLOCK
 */


DBAttrType * TreeDoubleLeafBlock::getValue(int index){
    double value = this->values[index];
    return new DBDoubleType(value);
}

void TreeDoubleLeafBlock::setValue(int index, const DBAttrType &val){
    DBDoubleType * double_val = (DBDoubleType *) &val;
    this->values[index] = double_val->getVal();
}

TID TreeDoubleLeafBlock::getTID(int index){
    return this->tids[index];
}

void TreeDoubleLeafBlock::setTID(int index, TID tid){
    this->tids[index] = tid;
}

int TreeDoubleLeafBlock::compare(int index, const DBAttrType &val){
    DBDoubleType *t = (DBDoubleType *) &val;
    double value = t->getVal();
    if(this->values[index] > value){
        return 1;
    }
    else if(this->values[index] == value){
        return 0;
    }
    else{
        return -1;
    }
}

void TreeDoubleLeafBlock::copyBlockToDBBACB(DBBACB d) {
    // std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);
    memcpy(d.getDataPtr() + basicSize + sizeof(double *)+ sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(double));
    memcpy(d.getDataPtr() + basicSize + sizeof(double *)+ sizeof(BlockNo *)+
           this->maxValueCounter * sizeof(double),
           (void *) this->tids, this->maxValueCounter * sizeof(TID));
    d.setModified();
}

void TreeDoubleLeafBlock::copyDBBACBToBlock(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(&this->leaf, d.getDataPtr(), basicSize);
    memcpy((void *) this->values, d.getDataPtr() + basicSize + sizeof(double *) + sizeof(BlockNo *),
           this->maxValueCounter * sizeof(double));
    memcpy((void *) this->tids, d.getDataPtr() + basicSize + sizeof(double *) + sizeof(BlockNo *) +
                                this->maxValueCounter * sizeof(double),
           this->maxValueCounter * sizeof(TID));
}

void TreeDoubleLeafBlock::updatePointers(){}

bool TreeDoubleLeafBlock::insertTID(const DBAttrType &val, TID tid){
    DBDoubleType * double_val = (DBDoubleType *) &val;
    double value = double_val->getVal();
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->tids[currentValueCounter] = tid;
            this->currentValueCounter++;
        } else {
            // Mitten in der Liste hinzufügen
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memmove(&values[i + 1], &values[i], (currentValueCounter - i) * sizeof(double));
                    memmove(&tids[i + 1], &tids[i], (currentValueCounter - i) * sizeof(TID));
                    values[i] = value;
                    tids[i] = tid;
                    this->currentValueCounter++;
                    break;
                }
            }
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn leer)
        this->values[0] = value;
        this->tids[0] = tid;
        this->currentValueCounter++;
    }
    
    if (this->currentValueCounter >= this->maxValueCounter) {
        return true;
    }
    return false;
}

TreeLeafBlock * TreeDoubleLeafBlock::splitBlock(BlockNo blockNo){
    int offset = std::ceil((this->maxValueCounter)/2);
    TreeDoubleLeafBlock *newDoubleLeafBlock = new TreeDoubleLeafBlock(blockNo);
    memcpy((void *) newDoubleLeafBlock->values, &(this->values[offset]), sizeof(double) * (this->maxValueCounter - offset));
    memcpy((void *) newDoubleLeafBlock->tids, &(this->tids[offset]), sizeof(TID) * (this->maxValueCounter - offset));
    this->currentValueCounter = offset;
    newDoubleLeafBlock->currentValueCounter = this->maxValueCounter - offset;
    return newDoubleLeafBlock;
}

void TreeDoubleLeafBlock::printAllValues(){
    std::cout << "Block No Leaf: " << this->blockNo << " -> ";
    for (int i = 0; i < this->currentValueCounter; i++) {
        //std::cout << this->values[i]<<":"<<this->tids[i].toString() << " ";
        std::cout << this->values[i] << " ";
    }
    std::cout << endl;
}

UndersizedAndValuePair TreeDoubleLeafBlock::removeTID(const DBAttrType &val, TID tid) {
    // Am Ende der Liste löschen
    double value = ((DBDoubleType *) &val)->getVal();
    if (value == this->values[currentValueCounter - 1]) {
        this->currentValueCounter--;
    } else {
        // Mitten in der Liste löschen
        for (int i = 0; i < this->currentValueCounter; i++) {
            if (this->values[i] == value) {
                memmove(&values[i], &values[i + 1], (currentValueCounter - i) * sizeof(double));
                memmove(&tids[i], &tids[i + 1], (currentValueCounter - i) * sizeof(TID));
                this->currentValueCounter--;
                break;
            }
        }
    }
    std::cout << "this->values(0) " << this->values[0] << std::endl;

    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue) / maxValue;

    if (divisionResult >= 0.5) {
        UndersizedAndValuePair r = UndersizedAndValuePair(DBDoubleType(this->values[this->currentValueCounter-1]), false, DOUBLE);
        return r;
    }
    if(this->currentValueCounter == 0){
        UndersizedAndValuePair r = UndersizedAndValuePair(DBDoubleType(0.0), true, DOUBLE);
        return r;
    }
    UndersizedAndValuePair r = UndersizedAndValuePair(DBDoubleType(this->values[this->currentValueCounter-1]), true, DOUBLE);
    return r;
}

ValueAndTIDPair TreeDoubleLeafBlock::removeSmallestTID() {
    //this->printAllValues();
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBDoubleType(this->values[0]), this->tids[0], true, DOUBLE);

        memmove(&values[0], &values[1], (this->currentValueCounter-1) * sizeof(double));
        memmove(&tids[0], &tids[1], (this->currentValueCounter-1) * sizeof(TID));
        this->currentValueCounter--;

        //this->printAllValues();
        return returnPair;
    }
    return ValueAndTIDPair(DBDoubleType(0.0), TID(), false, DOUBLE);
}

ValueAndTIDPair TreeDoubleLeafBlock::removeBiggestTID() {
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBDoubleType(this->values[this->currentValueCounter - 1]),
                                                           this->tids[this->currentValueCounter - 1], true,
                                                           DBDoubleType(this->values[this->currentValueCounter - 2]), DOUBLE);
        this->currentValueCounter--;
        return returnPair;
    }
    return ValueAndTIDPair(DBDoubleType(0.0), TID(), false, DOUBLE);
}


/*
 * VARCHAR INNER BLOCK
 */


DBAttrType * TreeVarCharInnerBlock::getValue(int index) {
    char * value = &(this->values[index*(MAX_STR_LEN+1)]);
    return new DBVCharType(value);
}

void TreeVarCharInnerBlock::setValue(int index, const DBAttrType &val) {
    DBVCharType *t = (DBVCharType *) &val;
    strncpy(&(this->values[index*(MAX_STR_LEN+1)]),(t->getVal()).c_str(),MAX_STR_LEN);
    this->values[index*(MAX_STR_LEN+1) + MAX_STR_LEN]='\0';
}

BlockNo TreeVarCharInnerBlock::getBlockNo(int index){
   return this->blockNos[index];
}
void TreeVarCharInnerBlock::setBlockNo(int index, BlockNo blockNo){
    this->blockNos[index] = blockNo;
}


int TreeVarCharInnerBlock::compare(int index, const DBAttrType &val){
    DBVCharType *t = (DBVCharType *) &val;
    DBVCharType indexValue = DBVCharType(&(this->values[index*(MAX_STR_LEN+1)]));
    if(indexValue > *t){
        return 1;
    }
    else if(indexValue == *t){
        return 0;
    }
    else{
        return -1;
    }
}
void TreeVarCharInnerBlock::copyBlockToDBBACB(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void*) &this->leaf, basicSize);
    
    memcpy(d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *),
           (void *) this->values,
           (this->maxValueCounter) * (MAX_STR_LEN + 1) * sizeof(char));
    
    memcpy(d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *)
            + (this->maxValueCounter) * (MAX_STR_LEN + 1) * sizeof(char),
           (void *) this->blockNos,
           (this->maxValueCounter + 1) * sizeof(BlockNo));
    
    d.setModified();
}

void TreeVarCharInnerBlock::copyDBBACBToBlock(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy((void *) &this->leaf, d.getDataPtr(), basicSize);
    
    memcpy((void *) this->values, d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *),
           (this->maxValueCounter) * (MAX_STR_LEN + 1) * sizeof(char));
    
    memcpy((void *) this->blockNos,
           d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *) + (this->maxValueCounter) * (MAX_STR_LEN + 1) * sizeof(char),
           (this->maxValueCounter + 1) * sizeof(BlockNo));
}
void TreeVarCharInnerBlock::updatePointers(){}


int TreeVarCharInnerBlock::removeBlockNo(BlockNo blockNo){return 0;}

void TreeVarCharInnerBlock::printAllValues(){
    std::cout << "BlockNo Inner: " << this->blockNo << " -> ";
    if(this->currentValueCounter > 0){
        std::cout << "(" << this->blockNos[0] << ") ";
        for(int i=0; i < this->currentValueCounter; i++){
            std::cout << &(this->values[i * (MAX_STR_LEN + 1)]) << " (" <<this->blockNos[i+1] << ") ";
        }
    }
    std::cout << endl;
}

bool TreeVarCharInnerBlock::insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool fromLeft){
    DBVCharType * vchar_value = (DBVCharType *) &val;
    //int value = int_value->getVal();
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        //if (value >= this->values[currentValueCounter - 1]) {
        if(this->compare(this->currentValueCounter-1, val) <= 0){
            this->setValue(currentValueCounter, val);
            this->blockNos[currentValueCounter + 1] = blockNo;
            this->currentValueCounter++;
            //std::cout << this->values[currentValueCounter - 1] << std::endl;
            //std::cout << "am ende der Liste einfuegen" << std::endl;
        } else {
            //std::cout << " FALL 2" << std::endl;
            // Mitten in der Liste hinzufügen
            //this->printAllValues();
            //std::cout << "Einfuegen von value" << value << std::endl;
            for (int i = 0; i < this->currentValueCounter; i++) {
                //if (this->values[i] >= value) {
                if(this->compare(i, val) >= 0){
                    memmove(&values[(i + 1)*(MAX_STR_LEN+1)], &values[i*(MAX_STR_LEN+1)], (currentValueCounter - i) * (MAX_STR_LEN+1) * sizeof(char));
                    memmove(&blockNos[i + 1], &blockNos[i], (currentValueCounter - i + 1) * sizeof(BlockNo));
                    this->setValue(i,val);
                    this->blockNos[i] = blockNo;
                    this->currentValueCounter++;
                    break;
                }

            }
            //this->printAllValues();
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn nur ein BlockPointer vorhanden ist)
        //std::cout << "Liste Leer" << std::endl;
        if (fromLeft) {
            this->setValue(0,val);
            this->blockNos[1] = this->blockNos[0];
            this->blockNos[0] = blockNo;
            this->currentValueCounter++;
        } else {
            this->setValue(0,val);
            this->blockNos[1] = blockNo;
            this->currentValueCounter++;
        }

    }

    if (this->currentValueCounter >= this->maxValueCounter) {
        return true;
    }
    return false;
}

ValueAndTIDPair TreeVarCharInnerBlock::removeSmallestBlockNo() {
    //this->printAllValues();
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBVCharType(&(this->values[0])), this->blockNos[0], true, VCHAR);

        memmove(&values[0], &values[MAX_STR_LEN+1], (this->currentValueCounter-1) * sizeof(char) * (MAX_STR_LEN+1));
        memmove(&blockNos[0], &blockNos[1], (this->currentValueCounter) * sizeof(BlockNo));
        this->currentValueCounter--;

        //this->printAllValues();
        return returnPair;
    }
    return ValueAndTIDPair(DBVCharType("0"), BlockNo(), false, VCHAR);
}

ValueAndTIDPair TreeVarCharInnerBlock::removeBiggestBlockNo() {
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBVCharType(&(this->values[(MAX_STR_LEN+1) * (this->currentValueCounter - 1)])),
                                                           this->blockNos[this->currentValueCounter], true,
                                                           DBVCharType(&(this->values[(MAX_STR_LEN+1) * (this->currentValueCounter - 2)])), VCHAR);
        this->currentValueCounter--;
        return returnPair;
    }
    return ValueAndTIDPair(DBVCharType("0"), BlockNo(), false, VCHAR);
}

//void TreeVarCharInnerBlock::insertBlockNo(BlockNo blockNoLeft, char *value, BlockNo blockNoRight){}
bool TreeVarCharInnerBlock::insertBlockNo(BlockNo blockNoLeft, const DBAttrType &value, BlockNo blockNoRight, bool root){
    DBVCharType *val = (DBVCharType *) &value;
    
    if(root){
        strncpy(&(this->values[0]),(val->getVal()).c_str(),MAX_STR_LEN);this->values[MAX_STR_LEN]='\0';
        this->blockNos[0] = blockNoLeft;
        this->blockNos[1] = blockNoRight;
        this->currentValueCounter++;
        return false;
    }
    else{
        //Prüfen, wo BlockNo + Value + BlockNo hinzugefügt werden soll
        for(int i = 0; i < this->currentValueCounter; i++){
            //value > this->values[this->currentValueCounter - 1]
            //i.e. have to append the value at
            if(this->compare(this->currentValueCounter-1, value) == -1){
                this->setValue(this->currentValueCounter, value);
                this->setBlockNo(this->currentValueCounter, blockNoLeft);
                this->setBlockNo(this->currentValueCounter+1, blockNoRight);
                break;
            }
            
            //value <= this->values[i]
            if(this->compare(i, value) >= 0){
                memmove(&this->values[(i+1)*(MAX_STR_LEN+1)], &this->values[i*(MAX_STR_LEN+1)], (MAX_STR_LEN + 1) * sizeof(char) * (this->currentValueCounter - i));
                memmove(&this->blockNos[i+2], &this->blockNos[i+1], sizeof(BlockNo) * (this->currentValueCounter -i));
                this->blockNos[i] = blockNoLeft;
                this->blockNos[i+1] = blockNoRight;
                this->setValue(i,value);
                break;
            }
        }
        this->currentValueCounter++;
        if(this->currentValueCounter >= this->maxValueCounter){
            return true;
        }
        else{
            return false;
        }
    }
}

//CharValueAndTIDPair removeSmallestBlockNo();
//CharValueAndTIDPair removeBiggestBlockNo();

TreeInnerBlock * TreeVarCharInnerBlock::splitBlock(BlockNo blockNo){
    int offset = std::ceil((this->maxValueCounter) / 2);
    TreeVarCharInnerBlock * newTreeVarCharInnerBlock = new TreeVarCharInnerBlock(blockNo);
    memcpy((void *) newTreeVarCharInnerBlock->values,
            (void *) &(this->values[offset*(MAX_STR_LEN+1)]),
            (MAX_STR_LEN + 1) * sizeof(char) * (this->maxValueCounter - offset + 1)
    );
    
    memcpy((void *) newTreeVarCharInnerBlock->blockNos,
           (void *) &(this->blockNos[offset]),
           sizeof(BlockNo) * (this->maxValueCounter - offset +1)
    );
    
    this->currentValueCounter = offset - 1;
    newTreeVarCharInnerBlock->currentValueCounter = this->maxValueCounter - offset;
    
    return newTreeVarCharInnerBlock;
}



/*
 * VARCHAR LEAF BLOCK
 */


DBAttrType * TreeVarCharLeafBlock::getValue(int index){
    char * value = &(this->values[index*(MAX_STR_LEN+1)]);
    return new DBVCharType(value);
}

void TreeVarCharLeafBlock::setValue(int index, const DBAttrType &val) {
    DBVCharType *t = (DBVCharType *) &val;
    strncpy(&(this->values[index * (MAX_STR_LEN + 1)]), (t->getVal()).c_str(), MAX_STR_LEN);
    this->values[index * (MAX_STR_LEN + 1) + MAX_STR_LEN] = '\0';
}

TID TreeVarCharLeafBlock::getTID(int index){
    return this->tids[index];
}
void TreeVarCharLeafBlock::setTID(int index, TID tid){
    this->tids[index] = tid;
}

int TreeVarCharLeafBlock::compare(int index, const DBAttrType &val){
    DBVCharType *t = (DBVCharType *) &val;
    DBVCharType indexValue = DBVCharType(&(this->values[index*(MAX_STR_LEN+1)]));
    if(indexValue > *t){
        return 1;
    }
    else if(indexValue == *t){
        return 0;
    }
    else{
        return -1;
    }
}

void TreeVarCharLeafBlock::copyDBBACBToBlock(DBBACB d){
    int basicSize = sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(&this->leaf, d.getDataPtr(), basicSize);
    memcpy((void *) this->values, d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *),
           this->maxValueCounter * sizeof(char) * (MAX_STR_LEN + 1));
    memcpy((void *) this->tids, d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *)
           +this->maxValueCounter * sizeof(char) * (MAX_STR_LEN + 1),
            this->maxValueCounter * sizeof(TID));
}
void TreeVarCharLeafBlock::copyBlockToDBBACB(DBBACB d){
    int basicSize = sizeof(bool) +  2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);
    memcpy(d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(char) * (MAX_STR_LEN + 1));
    memcpy(d.getDataPtr() + basicSize + sizeof(char *) + sizeof(BlockNo *)
           + this->maxValueCounter * sizeof(char) * (MAX_STR_LEN + 1),
           (void *)this->tids,
            this->maxValueCounter * sizeof(TID));
    d.setModified();
};

void TreeVarCharLeafBlock::updatePointers(){}

bool TreeVarCharLeafBlock::insertTID(const DBAttrType &val, TID tid){
    DBVCharType * char_value = (DBVCharType *) &val;
    if(this->currentValueCounter > 0){
        // Am Ende der Liste hinzufügen
        //val >= this->values[currentValueCounter - 1]
        if(this->compare(this->currentValueCounter-1, val) <= 0){
            this->setValue(currentValueCounter, val);
            this->setTID(currentValueCounter, tid);
            this->currentValueCounter++;
        }
        else{
            // Mitten in der Liste einfügen
            for(int i = 0; i < this->currentValueCounter; i++){
                //this->values[i] >= value
                if(this->compare(i, val) >= 0){
                    memmove(&values[(i+1)*(MAX_STR_LEN + 1)], &values[i*(MAX_STR_LEN + 1)], (currentValueCounter - i) * sizeof(char) * (MAX_STR_LEN + 1));
                    memmove(&tids[i+1], &tids[i], (currentValueCounter - i) * sizeof(TID));
                    this->setValue(i, val);
                    this->setTID(i, tid);
                    this->currentValueCounter++;
                    break;
                }
            }
        }
    }
    else{
        // Am Anfang der Liste hinzufügen (wenn leer)
        strncpy(&(this->values[0]),(char_value->getVal()).c_str(),MAX_STR_LEN);this->values[MAX_STR_LEN]='\0';
        this->setTID(0,tid);
        this->currentValueCounter++;
    }
    
    if(this->currentValueCounter >= this->maxValueCounter){
        return true;
    }
    else{
        return false;
    }
}

TreeLeafBlock * TreeVarCharLeafBlock::splitBlock(BlockNo blockNo){
    int offset = std::ceil((this->maxValueCounter) / 2);
    TreeVarCharLeafBlock *newVarCharLeafBlock = new TreeVarCharLeafBlock(blockNo);
    memcpy((void *) newVarCharLeafBlock->values,
           &(this->values[offset * (MAX_STR_LEN + 1)]),
           (this->maxValueCounter - offset) * sizeof(char) * (MAX_STR_LEN + 1));
    
    memcpy((void *) newVarCharLeafBlock->tids,
           &(this->tids[offset]),
           (this->maxValueCounter - offset) * sizeof(TID));
    
    this->currentValueCounter = offset;
    newVarCharLeafBlock->currentValueCounter = this->maxValueCounter - offset;
    
    return newVarCharLeafBlock;
}

void TreeVarCharLeafBlock::printAllValues(){
    std::cout << "Block No Leaf: " << this->blockNo << " -> ";
    for(int i = 0; i < this->currentValueCounter; i++){
        std::cout << &(this->values[i * (MAX_STR_LEN + 1)]) << " ";
    }
    std::cout << endl;
}

UndersizedAndValuePair TreeVarCharLeafBlock::removeTID(const DBAttrType &val, TID tid) {
    // Am Ende der Liste löschen
    //int value = ((DBIntType *) &val)->getVal();
    //if (value == this->values[currentValueCounter - 1]) {
    if(this->compare(this->currentValueCounter-1,val) == 0){
        this->currentValueCounter--;
    } else {
        // Mitten in der Liste löschen
        for (int i = 0; i < this->currentValueCounter; i++) {
            //if (this->values[i] == value) {
            if(this->compare(i,val) == 0){
                memmove(&values[i*(MAX_STR_LEN+1)], &values[(i + 1)*(MAX_STR_LEN+1)], (currentValueCounter - i) * sizeof(char) * (MAX_STR_LEN+1));
                memmove(&tids[i], &tids[i + 1], (currentValueCounter - i) * sizeof(TID));
                this->currentValueCounter--;
                break;
            }
        }
    }
    std::cout << "this->values(0) " << this->values[0] << std::endl;

    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue) / maxValue;

    if (divisionResult >= 0.5) {
        UndersizedAndValuePair r = UndersizedAndValuePair(DBVCharType(&(this->values[(this->currentValueCounter-1)*(MAX_STR_LEN+1)])), false, VCHAR);
        return r;
    }
    if(this->currentValueCounter == 0){
        UndersizedAndValuePair r = UndersizedAndValuePair(DBVCharType("0"), true, VCHAR);
        return r;
    }
    UndersizedAndValuePair r = UndersizedAndValuePair(DBVCharType(&(this->values[(this->currentValueCounter-1)*(MAX_STR_LEN+1)])), true, VCHAR);
    return r;
}

ValueAndTIDPair TreeVarCharLeafBlock::removeSmallestTID() {
    //this->printAllValues();
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBVCharType(&(this->values[0])), this->tids[0], true, VCHAR);

        memmove(&values[0], &values[MAX_STR_LEN+1], (this->currentValueCounter-1) * (MAX_STR_LEN+1) * sizeof(char));
        memmove(&tids[0], &tids[1], (this->currentValueCounter-1) * sizeof(TID));
        this->currentValueCounter--;

        //this->printAllValues();
        return returnPair;
    }
    return ValueAndTIDPair(DBVCharType("0"), TID(), false, VCHAR);
}

ValueAndTIDPair TreeVarCharLeafBlock::removeBiggestTID() {
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        ValueAndTIDPair returnPair = ValueAndTIDPair(DBVCharType(&(this->values[(this->currentValueCounter-1)*(MAX_STR_LEN+1)])),
                                                           this->tids[this->currentValueCounter - 1], true,
                                                           DBVCharType(&(this->values[(this->currentValueCounter-2)*(MAX_STR_LEN+1)])), VCHAR);
        this->currentValueCounter--;
        return returnPair;
    }
    return ValueAndTIDPair(DBVCharType("0"), TID(), false, VCHAR);
}





