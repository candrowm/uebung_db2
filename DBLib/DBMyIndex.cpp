#include <hubDB/DBMyIndex.h>
#include <hubDB/DBException.h>
#include <cmath>
#include <hubDB/DBRandomBufferMgr.h>

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

    // std::cout << "hallo" << std::endl;
    int numberOfBlocks = bufMgr.getBlockCnt(file);
    // std::cout << "Number of Blocks:" << numberOfBlocks << std::endl;

    if (numberOfBlocks == 0) {
        DBBACB metaBlock = bufMgr.fixNewBlock(file);
        TreeStartBlock metaNode = TreeStartBlock();
        metaNode.blockNo = metaBlock.getBlockNo();


        DBBACB rootBlock = bufMgr.fixNewBlock(file);
        TreeIntLeafBlock rootNode = TreeIntLeafBlock(rootBlock.getBlockNo());
        metaNode.rootBlockNo = rootNode.blockNo;

        metaNode.copyBlockToDBBACB(metaBlock);
        bufMgr.unfixBlock(metaBlock);


        rootNode.copyBlockToDBBACB(rootBlock);
        bufMgr.unfixBlock(rootBlock);
    }

    /* TESTZWECKE!!!
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


    srand(11);
    int size = 200000;
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
        insertValueFirstCall(values[i], TID());

        // printAllBlocks();
    }

    printAllBlocks();
    //return;
    std::cout << "los--------------" << std::endl;

    //  return;
    for (int i = 0; i < size; i++) {

        removeValueFirstCall(values[i], TID());
        //bufMgr.fixBlock(file, 999999, LOCK_EXCLUSIVE);
        //  printAllBlocks();
        if (values[i] == 762) {
            //return;
        }
        if (i == size - 3) {
            // printAllBlocks();
        }
    }
    printAllBlocks();
    //printFreeBlocks();

    */

}


DBMyIndex::~DBMyIndex() {

}

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
                t->nextFreeBlockNo = blockNo;
                t->copyBlockToDBBACB(nextBlock);
                bufMgr.unfixBlock(nextBlock);
                return;
            } else {
                nextFreeBlockNo = t->nextFreeBlockNo;
                bufMgr.unfixBlock(nextBlock);
            }
        }
    }

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
            // std::cout << "Free Block: " << t->blockNo << std::endl;
            if (t->nextFreeBlockNo == 0) {
                t->copyBlockToDBBACB(nextBlock);
                bufMgr.unfixBlock(nextBlock);
                return;
            } else {
                nextFreeBlockNo = t->nextFreeBlockNo;
                bufMgr.unfixBlock(nextBlock);
            }
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


    for (int i = 1; i < bufMgr.blockCounter; i++) {
        try {
            DBBACB fRoot = bufMgr.fixBlock(file, i, LOCK_SHARED);
            TreeBlock *t = (TreeBlock *) fRoot.getDataPtr();
            if (!t->leaf) {
                TreeIntInnerBlock *fRootBlock = (TreeIntInnerBlock *) fRoot.getDataPtr();
                fRootBlock->updatePointers();
                fRootBlock->printAllValues();
            }
            if (t->leaf) {
                TreeIntLeafBlock *treeIntLeafBlock = (TreeIntLeafBlock *) fRoot.getDataPtr();
                treeIntLeafBlock->updatePointers();
                treeIntLeafBlock->printAllValues();
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
void DBMyIndex::removeValueFirstCall(int value, const TID &tid) {
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
IntUndersizedAndValuePair
DBMyIndex::removeValue(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo) {
    std::cout << "REMOVE VALUE " << value << " StartBlock: " << startBlockNo << std::endl;
    DBBACB rootB = bufMgr.fixBlock(file, startBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *treeBlock = (TreeBlock *) rootB.getDataPtr();

    // Falls man den Blattknoten erwischt hat --> Wert löschen
    if (treeBlock->leaf) {
        TreeIntLeafBlock *treeIntLeafBlock = (TreeIntLeafBlock *) rootB.getDataPtr();
        treeIntLeafBlock->updatePointers();
        IntUndersizedAndValuePair r = treeIntLeafBlock->removeTID(value, tid);
        if (parentBlockNo == 0) {
            // Rootknoten ist einziger Knoten und kann deshalb nicht zu wenig Werte enthalten
            r.undersized = false;
        }
        //std::cout << r.undersized << std::endl;
        treeIntLeafBlock->copyBlockToDBBACB(rootB);
        bufMgr.unfixBlock(rootB);


        return r;
    }

    int position = -1;
    // Falls man den Blattknoten noch nicht erreicht hat --> Neuen rekursiven Aufruf (so lange, bis der Blattknoten erreicht wird)
    if (!treeBlock->leaf) {

        TreeIntInnerBlock *treeIntInnerBlock = (TreeIntInnerBlock *) rootB.getDataPtr();
        treeIntInnerBlock->updatePointers();

        // Prüfen, welchen Block man als nächstes lesen muss, um eine Ebene "näher" an den zu löschenden Wert zu kommen
        IntUndersizedAndValuePair r = IntUndersizedAndValuePair(-1, false);
        for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {


            // Falls der Wert größer ist als der letzte Wert im inneren Knoten --> Letzten Pointer (=BlockNo) des inneren Knoten lesen
            if (value > treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]) {
                r = removeValue(
                        treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter], value, tid,
                        treeIntInnerBlock->blockNo);
                position = treeIntInnerBlock->currentValueCounter;
                break;
            }

            // Alle Werte des inneren Knoten durchgehen und gucken, welcher Pointer (=BlockNo) der richtige für den Wert ist
            if (value <= treeIntInnerBlock->values[i]) {
                r = removeValue(treeIntInnerBlock->blockNos[i], value, tid,
                                treeIntInnerBlock->blockNo);

                position = i;

                // Wenn der zu loeschende Wert einem Wert im inneren Knoten entspricht --> Wert im inneren Knoten aendern
                if (value == treeIntInnerBlock->values[i]) {
                    // TODO: KEINE AHNUNG OB
                    treeIntInnerBlock->values[i] = r.value;
                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    //bufMgr.unfixBlock(rootB);

                    // std::cout << "Shit: ich muss Wert aendern im Blattknoten zu dem Wert" << r.value
                    //    << std::endl;
                }
                break;
            }
        }

        // Wenn der Kinderknoten voll genug ist --> Keine Änderung notwendig + evtl. neuen Wert nach "oben" schieben (falls Wert des Elternknoten geloescht wird)
        if (!r.undersized) {
            return r;
        }

        // Wenn dem Blattknoten ein Wert entfernt wurde und dieser nun zu wenig Werte hat --> undersized==true
        if (r.undersized) {

            //std::cout << " Position von undersized leaf " << position << std::endl;
            int positionOfUndersizedBlock = treeIntInnerBlock->blockNos[position];

            DBBACB undersizedBlock = bufMgr.fixBlock(file, positionOfUndersizedBlock, LOCK_EXCLUSIVE);
            TreeBlock *undersizedTreeBlock = (TreeBlock *) undersizedBlock.getDataPtr();


            // Wenn sich unter dem inneren Knoten kein Blattknoten befindet, sondern wieder ein innerer Knoten
            if (!undersizedTreeBlock->leaf) {


                TreeIntInnerBlock *undersizedTreeIntInnerBlock = (TreeIntInnerBlock *) undersizedBlock.getDataPtr();
                // std::cout << " POSITION!!!!!!!!!!!!!!!!!!11111 " << undersizedTreeIntInnerBlock->blockNo << std::endl;


                // Moeglickeit 1: aus dem linken inneren Knoten ein Value+BlockNo Paar rueberschieben
                if (position > 0) {
                    std::cout << "Vom linken inneren Knoten etwas klauen" << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *leftNeighbourInnerBlock = (TreeIntInnerBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourInnerBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = leftNeighbourInnerBlock->removeBiggestBlockNo();



                    //std::cout << " Ergebnis moeglichkeit 1 Value: " << rIntValueAndTIDPair.parentValue << " " << rIntValueAndTIDPair.successful << std::endl;

                    if (rIntValueAndTIDPair.successful) {
                        int oldValue = treeIntInnerBlock->values[position - 1];
                        treeIntInnerBlock->values[position - 1] = rIntValueAndTIDPair.value;

                        if (position >= treeIntInnerBlock->currentValueCounter) {
                            position--;
                        }


                        r.undersized = false;

                        treeIntInnerBlock->printAllValues();
                        leftNeighbourInnerBlock->printAllValues();
                        undersizedTreeIntInnerBlock->printAllValues();


                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntInnerBlock->insertBlockNo(oldValue, rIntValueAndTIDPair.blockNo, true);
                        undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);

                        leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);
                        return r;

                    }
                    leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                }


                // Moeglickeit 1: aus dem rechten inneren Knoten ein Value+BlockNo Paar rueberschieben
                if (position < (treeIntInnerBlock->currentValueCounter)) {
                    //  std::cout << "Versuche Wert zu klauen von rechts (innere Knoten)" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);


                    TreeIntInnerBlock *rightNeighbourInnerBlock = (TreeIntInnerBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourInnerBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = rightNeighbourInnerBlock->removeSmallestBlockNo();
                    if (rIntValueAndTIDPair.successful) {
                        int oldValue = treeIntInnerBlock->values[position];
                        treeIntInnerBlock->values[position] = rIntValueAndTIDPair.value;


                        std::cout << " Wert geklaut von rechts und Elternknoten angepasst " << rIntValueAndTIDPair.value
                                  << std::endl;


                        rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                        bufMgr.unfixBlock(rightNeighbourBlock);

                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntInnerBlock->insertBlockNo(oldValue, rIntValueAndTIDPair.blockNo, false);
                        undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);

                        r.undersized = false;
                        return r;
                    }

                    rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    bufMgr.unfixBlock(rightNeighbourBlock);

                }


                //std::cout << " Bleibt nur noch Merge der inneren Knoten" << std::endl;

                // Moeglichkeit 3: Falls das Rüberschieben eines Value+BlockNo Paar von links und rechts nicht geklappt hat --> Merge notwendig
                // 3.1: Merge mit linkem Knoten
                if (position > 0) {


                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *leftNeighbourInnerBlock = (TreeIntInnerBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourInnerBlock->updatePointers();

                    std::cout << "Merge mit linkem inneren Knoten " << leftNeighbourInnerBlock->blockNo << " mit "
                              << undersizedTreeIntInnerBlock->blockNo << " und Eltern " << treeIntInnerBlock->blockNo
                              << std::endl;


                    if (position >= treeIntInnerBlock->currentValueCounter) {
                        position--;
                    }
                    undersizedTreeIntInnerBlock->printAllValues();
                    leftNeighbourInnerBlock->printAllValues();

                    for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {
                        if (treeIntInnerBlock->blockNos[i] == leftNeighbourInnerBlock->blockNo) {
                            leftNeighbourInnerBlock->values[leftNeighbourInnerBlock->currentValueCounter] = treeIntInnerBlock->values[i];
                            //leftNeighbourInnerBlock->blockNos[leftNeighbourInnerBlock->currentValueCounter+1] = undersizedTreeIntInnerBlock->blockNos[0];
                            //leftNeighbourInnerBlock->currentValueCounter++;
                            break;
                        }
                    }
                    //leftNeighbourInnerBlock->values[undersizedTreeIntInnerBlock->currentValueCounter] = treeIntInnerBlock->values[position];
                    leftNeighbourInnerBlock->printAllValues();

                    for (int i = 0; i <= undersizedTreeIntInnerBlock->currentValueCounter; i++) {
                        if (i < undersizedTreeIntInnerBlock->currentValueCounter) {
                            leftNeighbourInnerBlock->values[leftNeighbourInnerBlock->currentValueCounter +
                                                            i + 1] = undersizedTreeIntInnerBlock->values[i];
                        }
                        leftNeighbourInnerBlock->blockNos[leftNeighbourInnerBlock->currentValueCounter +
                                                          i + 1] = undersizedTreeIntInnerBlock->blockNos[i];
                        //leftNeighbourInnerBlock->currentValueCounter++;
                    }

                    leftNeighbourInnerBlock->currentValueCounter = leftNeighbourInnerBlock->currentValueCounter + 1 +
                                                                   undersizedTreeIntInnerBlock->currentValueCounter;

                    // std::cout << "NACH MERGE: " << std::endl;
                    leftNeighbourInnerBlock->printAllValues();

                    undersizedTreeIntInnerBlock->currentValueCounter = 0;
                    undersizedTreeIntInnerBlock->nextFreeBlockNo = BlockNo(0);




                    //TODO: loeschen von undersized Knoten, da nicht verwendet
                    treeIntInnerBlock->printAllValues();
                    bool found = false;
                    for (int i = 0; i <= treeIntInnerBlock->currentValueCounter; i++) {
                        if (treeIntInnerBlock->blockNos[i] == undersizedTreeIntInnerBlock->blockNo) {
                            found = true;
                        }
                        if (found) {
                            if (i == treeIntInnerBlock->currentValueCounter) {
                                treeIntInnerBlock->blockNos[i - 1] = treeIntInnerBlock->blockNos[i];
                                break;
                            }
                            treeIntInnerBlock->blockNos[i - 1] = treeIntInnerBlock->blockNos[i];
                            treeIntInnerBlock->values[i - 1] = treeIntInnerBlock->values[i];
                        }
                    };
                    treeIntInnerBlock->printAllValues();
                    for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {
                        if (treeIntInnerBlock->blockNos[i] == undersizedTreeIntInnerBlock->blockNo) {
                            treeIntInnerBlock->blockNos[i] = leftNeighbourInnerBlock->blockNo;
                        }
                    }
                    treeIntInnerBlock->printAllValues();
                    // Nach dem Merge von 2 inneren Knoten muss der Elternknoten angepasst werden
                    //memcpy
                    // (&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position+1],
                    // sizeof(int) * (treeIntInnerBlock->currentValueCounter - position + 1));

                    //memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position+1],
                    //sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position + 1));

                    // treeIntInnerBlock->printAllValues();


                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    //treeIntInnerBlock->printAllValues();

                    //treeIntInnerBlock->blockNos[position] = leftNeighbourInnerBlock->blockNo;
                    // treeIntInnerBlock->printAllValues();
                    //  treeIntInnerBlock->values[position] = leftNeighbourInnerBlock->values[
                    //    leftNeighbourInnerBlock->currentValueCounter - 1];

                    treeIntInnerBlock->printAllValues();
                    undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                    insertFreeBlock(undersizedTreeIntInnerBlock->blockNo);
                    bufMgr.unfixBlock(undersizedBlock);

                    // Falls der Elternknoten == Rootknoten ist (d.h. keinen Elternknoten mehr hat) und keine Werte mehr beinhaltet
                    // Neuen Rootknoten setzen (und alten Rootknoten löschen)
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;
                        BlockNo newRootBlockNo = leftNeighbourInnerBlock->blockNo;
                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                        startBlock->rootBlockNo = newRootBlockNo;

                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);

                        treeIntInnerBlock->currentValueCounter = 0;
                        treeIntInnerBlock->nextFreeBlockNo = BlockNo(0);
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeIntInnerBlock->blockNo);
                        bufMgr.unfixBlock(rootB);


                        leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }


                    leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);


                    treeIntInnerBlock->copyBlockToDBBACB(rootB);


                    double currentValue = treeIntInnerBlock->currentValueCounter;
                    double maxValue = treeIntInnerBlock->maxValueCounter;
                    double divisionResult = (currentValue) / maxValue;

                    bufMgr.unfixBlock(rootB);

                    bufMgr.unfixBlock(leftNeighbourBlock);

                    if (divisionResult > 0.4) {
                        r.undersized = false;
                        return r;
                    }
                    return r;


                }


                // 3.2. Merge mit rechtem Knoten
                if (position < (treeIntInnerBlock->currentValueCounter)) {
                    std::cout << "Merge mit rechtem Knoten" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *rightNeighbourLeafBlock = (TreeIntInnerBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourLeafBlock->updatePointers();

                    //std::cout << "VOR MERGE r: " << std::endl;
                    // undersizedTreeIntInnerBlock->printAllValues();
                    // std::cout << "Merge folgende Knoten " << std::endl;
                    undersizedTreeIntInnerBlock->printAllValues();
                    rightNeighbourLeafBlock->printAllValues();

                    // Aus 2 inneren Knoten 1 inneren Knoten machen, d.h. Werte und BlockNos von einem Knoten zum anderen rüberschieben
                    if (position >= treeIntInnerBlock->currentValueCounter) {
                        position--;
                    }
                    undersizedTreeIntInnerBlock->values[undersizedTreeIntInnerBlock->currentValueCounter] = treeIntInnerBlock->values[position];
                    undersizedTreeIntInnerBlock->printAllValues();


                    for (int i = 0; i <= rightNeighbourLeafBlock->currentValueCounter; i++) {
                        if (i < rightNeighbourLeafBlock->currentValueCounter) {
                            undersizedTreeIntInnerBlock->values[undersizedTreeIntInnerBlock->currentValueCounter +
                                                                i + 1] = rightNeighbourLeafBlock->values[i];
                        }
                        undersizedTreeIntInnerBlock->blockNos[undersizedTreeIntInnerBlock->currentValueCounter +
                                                              i + 1] = rightNeighbourLeafBlock->blockNos[i];
                        //leftNeighbourInnerBlock->currentValueCounter++;
                    }

                    undersizedTreeIntInnerBlock->currentValueCounter =
                            undersizedTreeIntInnerBlock->currentValueCounter + 1 +
                            rightNeighbourLeafBlock->currentValueCounter;


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

                    undersizedTreeIntInnerBlock->printAllValues();

                    //std::cout << "NACH MERGE: " << std::endl;
                    //undersizedTreeIntInnerBlock->printAllValues();


                    treeIntInnerBlock->printAllValues();

                    for (int i = position; i < treeIntInnerBlock->currentValueCounter; i++) {
                        treeIntInnerBlock->values[i] = treeIntInnerBlock->values[i + 1];
                        treeIntInnerBlock->blockNos[i] = treeIntInnerBlock->blockNos[i + 1];
                    }
                    //treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter]


                    // Nach dem Merge von 2 inneren Knoten muss der Elternknoten angepasst werden
                    //  memcpy(&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position + 1],
                    //  sizeof(int) * (treeIntInnerBlock->currentValueCounter - position));
                    // memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position + 1],
                    // sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position));


                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position] = undersizedTreeIntInnerBlock->blockNo;
                    //treeIntInnerBlock->values[position] = undersizedTreeIntInnerBlock->values[
                    // undersizedTreeIntInnerBlock->currentValueCounter - 1];

                    //treeIntInnerBlock->printAllValues();

                    rightNeighbourLeafBlock->currentValueCounter = 0;
                    rightNeighbourLeafBlock->nextFreeBlockNo = BlockNo(0);
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    insertFreeBlock(rightNeighbourLeafBlock->blockNo);


                    undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                    bufMgr.unfixBlock(undersizedBlock);
                    bufMgr.unfixBlock(rightNeighbourBlock);



                    // Falls der Elternknoten == Rootknoten ist (d.h. keinen Elternknoten mehr hat) und keine Werte mehr beinhaltet
                    // Neuen Rootknoten setzen (und alten Rootknoten löschen)
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig!!!!" << std::endl;
                        BlockNo newRootBlockNo = undersizedTreeIntInnerBlock->blockNo;
                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                        treeIntInnerBlock->currentValueCounter = 0;
                        treeIntInnerBlock->nextFreeBlockNo = BlockNo(0);
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeIntInnerBlock->blockNo);


                        startBlock->rootBlockNo = newRootBlockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);
                        bufMgr.unfixBlock(rootB);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }
                    treeIntInnerBlock->copyBlockToDBBACB(rootB);


                    double currentValue = treeIntInnerBlock->currentValueCounter;
                    double maxValue = treeIntInnerBlock->maxValueCounter;
                    double divisionResult = (currentValue) / maxValue;

                    bufMgr.unfixBlock(rootB);

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

                TreeIntLeafBlock *undersizedTreeIntLeafBlock = (TreeIntLeafBlock *) undersizedBlock.getDataPtr();
                undersizedTreeIntLeafBlock->updatePointers();

                //std::cout << "position " << position << std::endl;

                // Moeglickeit 1: aus dem linken Blatt ein Value+TID Paar rueberschieben
                if (position > 0) {
                    //  std::cout << "moeglichkeit 1" << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *leftNeighbourLeafBlock = (TreeIntLeafBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourLeafBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = leftNeighbourLeafBlock->removeBiggestTID();

                    //std::cout << " Ergebnis moeglichkeit 1 Value: " << rIntValueAndTIDPair.parentValue << " " << rIntValueAndTIDPair.successful << std::endl;

                    if (rIntValueAndTIDPair.successful) {
                        //std::cout << "AUS: " << treeIntInnerBlock->values[position-1] << " wird " << rIntValueAndTIDPair.parentValue << std::endl;
                        treeIntInnerBlock->values[position - 1] = rIntValueAndTIDPair.neighbourValue;
                        if (position >= treeIntInnerBlock->currentValueCounter) {
                            position--;
                        }

                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntLeafBlock->insertTID(rIntValueAndTIDPair.value, rIntValueAndTIDPair.tid);
                        undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);

                        leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                        bufMgr.unfixBlock(leftNeighbourBlock);

                        std::cout << " Wert geklaut von links und Elternknoten angepasst "
                                  << rIntValueAndTIDPair.neighbourValue << std::endl;

                        r.undersized = false;
                        return r;

                    }

                    leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                }

                // Moeglichkeit 2: aus dem rechten Blatt ein Value+TID Paar rueberschieben
                if (position < (treeIntInnerBlock->currentValueCounter)) {
                    // std::cout << "zwei" << std::endl;

                    bufMgr.unfixBlock(undersizedBlock);
                    DBBACB undersizedBlock2 = bufMgr.fixBlock(file, positionOfUndersizedBlock, LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *undersizedTreeIntLeafBlock2 = (TreeIntLeafBlock *) undersizedBlock2.getDataPtr();
                    undersizedTreeIntLeafBlock2->updatePointers();

                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    //std::cout << rightNeighbourBlock.getLockMode() << std::endl;
                    TreeIntLeafBlock *rightNeighbourLeafBlock = (TreeIntLeafBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourLeafBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = rightNeighbourLeafBlock->removeSmallestTID();
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);


                    if (rIntValueAndTIDPair.successful) {
                        //   std::cout << "TRY UNFIX: " << rightNeighbourBlock.getBlockNo() << std::endl;


                        undersizedTreeIntLeafBlock2->insertTID(rIntValueAndTIDPair.value, rIntValueAndTIDPair.tid);
                        undersizedTreeIntLeafBlock2->copyBlockToDBBACB(undersizedBlock2);


                        treeIntInnerBlock->values[position] = rIntValueAndTIDPair.value;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);


                        std::cout << " Wert geklaut von rechts und Elternknoten angepasst !"
                                  << rIntValueAndTIDPair.value
                                  << std::endl;

                        r.undersized = false;


                        bufMgr.unfixBlock(rightNeighbourBlock);
                        bufMgr.unfixBlock(rootB);
                        bufMgr.unfixBlock(undersizedBlock2);
                        return r;
                    }
                    bufMgr.unfixBlock(rightNeighbourBlock);


                }

                bufMgr.unfixBlock(undersizedBlock);

                DBBACB undersizedBlock2 = bufMgr.fixBlock(file, positionOfUndersizedBlock, LOCK_EXCLUSIVE);
                TreeIntLeafBlock *undersizedTreeIntLeafBlock2 = (TreeIntLeafBlock *) undersizedBlock2.getDataPtr();
                undersizedTreeIntLeafBlock2->updatePointers();
                // std::cout << undersizedTreeIntLeafBlock->blockNo << "das bekommen " << std::endl;


                // Moeglichkeit 3: Falls das Rüberschieben von einem linken oder rechten Blattknoten nicht funktioniert hat --> Merge notwendig
                // 3.1: Merge mit linkem Knoten
                if (position > 0) {

                    std::cout << " Merge mit linkem Knoten " << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *leftNeighbourLeafBlock = (TreeIntLeafBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourLeafBlock->updatePointers();

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
                    for (int i = 0; i < undersizedTreeIntLeafBlock2->currentValueCounter; i++) {
                        leftNeighbourLeafBlock->values[leftNeighbourLeafBlock->currentValueCounter +
                                                       i] = undersizedTreeIntLeafBlock2->values[i];
                        leftNeighbourLeafBlock->tids[leftNeighbourLeafBlock->currentValueCounter +
                                                     i] = undersizedTreeIntLeafBlock2->tids[i];
                    }


                    leftNeighbourLeafBlock->currentValueCounter = leftNeighbourLeafBlock->currentValueCounter +
                                                                  undersizedTreeIntLeafBlock2->currentValueCounter;

                    //std::cout << "NACH MERGE: " << std::endl;
                    leftNeighbourLeafBlock->printAllValues();
                    // std::cout << startBlockNo << std::endl;


                    undersizedTreeIntLeafBlock2->currentValueCounter = 0;
                    undersizedTreeIntLeafBlock2->copyBlockToDBBACB(undersizedBlock2);
                    //std::cout << treeIntInnerBlock->blockNo << "treeIntInnerBlock " << std::endl;

                    insertFreeBlock(undersizedTreeIntLeafBlock2->blockNo);


                    //TODO: loeschen von undersized Knoten, da nicht verwendet

                    // Elternknoten anpassen, nachdem 2 Blattknoten zusammengeschmolzen wurden
                    // memcpy(&treeIntInnerBlock->values[position - 1], &treeIntInnerBlock->values[position],
                    //     sizeof(int) * (treeIntInnerBlock->currentValueCounter - position + 1));
                    //  memcpy(&treeIntInnerBlock->blockNos[position - 1], &treeIntInnerBlock->blockNos[position],
                    //  sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position + 1));
                    treeIntInnerBlock->printAllValues();
                    //std::cout << "JETZT" << std::endl;
                    // std::cout << "Position" << position - 1 << std::endl;
                    for (int i = position; i <= treeIntInnerBlock->currentValueCounter; i++) {
                        treeIntInnerBlock->values[i - 1] = treeIntInnerBlock->values[i];
                        treeIntInnerBlock->blockNos[i - 1] = treeIntInnerBlock->blockNos[i];
                    }

                    treeIntInnerBlock->printAllValues();

                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position - 1] = leftNeighbourLeafBlock->blockNo;
                    //treeIntInnerBlock->values[position - 1] = leftNeighbourLeafBlock->values[
                    //  leftNeighbourLeafBlock->currentValueCounter - 1];

                    // treeIntInnerBlock->printAllValues();

                    // Falls der Elternknoten der Rootknoten ist und keine Werte mehr beinhaltet --> Neuen Rootknoten setzen und alten löschen
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;
                        BlockNo newRootBlockNo = leftNeighbourLeafBlock->blockNo;


                        treeIntInnerBlock->currentValueCounter = 0;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeIntInnerBlock->blockNo);
                        bufMgr.unfixBlock(rootB);

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = newRootBlockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }

                    double currentValue = treeIntInnerBlock->currentValueCounter;
                    double maxValue = treeIntInnerBlock->maxValueCounter;

                    //std::cout << "HIER 3" << std::endl;
                    leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    //std::cout << "HIER 3-1" << std::endl;
                    // undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);

                    //std::cout << "HIER 3-2" << std::endl;
                    treeIntInnerBlock->copyBlockToDBBACB(rootB);


                    // std::cout << "HIER 4" << std::endl;

                    bufMgr.unfixBlock(undersizedBlock2);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                    bufMgr.unfixBlock(rootB);

                    double divisionResult = (currentValue) / maxValue;


                    if (divisionResult > 0.4) {
                        r.undersized = false;
                        return r;
                    }
                    return r;


                }


                // 3.2. Merge mit rechtem Knoten
                if (position < (treeIntInnerBlock->currentValueCounter)) {
                    std::cout << "Merge mit rechtem Knoten" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *rightNeighbourLeafBlock = (TreeIntLeafBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourLeafBlock->updatePointers();

                    //std::cout << "VOR MERGE: " << std::endl;
                    //undersizedTreeIntLeafBlock->printAllValues();


                    // 2 Blattknoten zu 1 Blattknoten verschmelzen
                    if (position >= treeIntInnerBlock->currentValueCounter) {
                        position--;
                    }

                    for (int i = 0; i < rightNeighbourLeafBlock->currentValueCounter; i++) {
                        undersizedTreeIntLeafBlock2->values[undersizedTreeIntLeafBlock2->currentValueCounter +
                                                            i] = rightNeighbourLeafBlock->values[i];
                        undersizedTreeIntLeafBlock2->tids[undersizedTreeIntLeafBlock2->currentValueCounter +
                                                          i] = rightNeighbourLeafBlock->tids[i];
                    }
/*
                    memcpy(&undersizedTreeIntLeafBlock2->values[undersizedTreeIntLeafBlock2->currentValueCounter],
                           &rightNeighbourLeafBlock->values[0],
                           sizeof(int) * rightNeighbourLeafBlock->currentValueCounter);

                    memcpy(&undersizedTreeIntLeafBlock->tids[undersizedTreeIntLeafBlock2->currentValueCounter],
                           &rightNeighbourLeafBlock->tids[0],
                           sizeof(TID) * rightNeighbourLeafBlock->currentValueCounter);
                           */

                    undersizedTreeIntLeafBlock2->currentValueCounter =
                            undersizedTreeIntLeafBlock2->currentValueCounter +
                            rightNeighbourLeafBlock->currentValueCounter;

                    //std::cout << "NACH MERGE: " << std::endl;
                    //  undersizedTreeIntLeafBlock2->printAllValues();


                    treeIntInnerBlock->printAllValues();

                    /*
                    // Nach dem Merge von 2 Blattknoten muss der Elternknoten angepasst werden
                    memcpy(&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position + 1],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - position));
                    memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position + 1],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position));
                           */


                    for (int i = position; i < treeIntInnerBlock->currentValueCounter; i++) {
                        treeIntInnerBlock->values[i] = treeIntInnerBlock->values[i + 1];
                        treeIntInnerBlock->blockNos[i] = treeIntInnerBlock->blockNos[i + 1];
                    }


                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position] = undersizedTreeIntLeafBlock2->blockNo;
                    treeIntInnerBlock->values[position] = undersizedTreeIntLeafBlock2->values[
                            undersizedTreeIntLeafBlock2->currentValueCounter - 1];

                    // treeIntInnerBlock->printAllValues();
                    // std::cout << "Fertig mit merge" << std::endl;

                    rightNeighbourLeafBlock->currentValueCounter = 0;
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    insertFreeBlock(rightNeighbourLeafBlock->blockNo);


                    undersizedTreeIntLeafBlock2->copyBlockToDBBACB(undersizedBlock2);
                    bufMgr.unfixBlock(undersizedBlock2);
                    bufMgr.unfixBlock(rightNeighbourBlock);





                    // Falls der Elternknoten der Rootknoten ist und keine Werte mehr beinhaltet --> Neuen Rootknoten setzen + alten löschen
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig :O" << std::endl;
                        BlockNo newRootBlockNo = undersizedTreeIntLeafBlock2->blockNo;
                        // std::cout << "VERSUCHE UNFIX:" << rootB.getBlockNo() << std::endl;

                        treeIntInnerBlock->currentValueCounter = 0;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        insertFreeBlock(treeIntInnerBlock->blockNo);

                        bufMgr.unfixBlock(rootB);

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = newRootBlockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }
                    double currentValue = treeIntInnerBlock->currentValueCounter;
                    double maxValue = treeIntInnerBlock->maxValueCounter;
                    double divisionResult = (currentValue) / maxValue;

                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);

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
void DBMyIndex::insertValueFirstCall(int value, const TID &tid) {
    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
    bufMgr.unfixBlock(metaB);
    insertValue(startBlock->rootBlockNo, value, tid, 0);
}

// Diese Funktion wird rekursiv aufgerufen, dabei wird die startBlockNo angegeben (anfangs der Rootknoten)
// Ziel ist es, die Funktion so lange rekursiv aufzufrufen, bis man einen Blattknoten erreicht hat
ReturnIntInsertValue
DBMyIndex::insertValue(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo) {
    std::cout << "INSERT VALUE " << value << " StartBlock: " << startBlockNo << std::endl;


    DBBACB rootB = bufMgr.fixBlock(file, startBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *treeBlock = (TreeBlock *) rootB.getDataPtr();

    // Falls der Block ein Blattknoten ist --> Wert einfügen
    if (treeBlock->leaf) {
        TreeIntLeafBlock *treeIntLeafBlock = (TreeIntLeafBlock *) rootB.getDataPtr();
        treeIntLeafBlock->updatePointers();
        bool split = treeIntLeafBlock->insertTID(value, tid);

        if (!split) {
            std::cout << "kein Split notwendig" << std::endl;
            treeIntLeafBlock->copyBlockToDBBACB(rootB);
            bufMgr.unfixBlock(rootB);

            return ReturnIntInsertValue(0, 0, 0);
        }


        // Der Blattknoten, in dem man den neuen Wert eingefügt hat, ist zu voll --> Split des Blattknoten notwendig
        if (split) {
            std::cout << " SPLITEN" << std::endl;


            // Neuer Blattknoten, auf dem Haelfte der Werte uebertragen werden, die beim alten Blattknoten geloescht werden
            DBBACB newLeafBlock = fixNewBlock();
            TreeIntLeafBlock newTreeIntLeafBlock = TreeIntLeafBlock(newLeafBlock.getBlockNo());
            int offset = std::ceil((treeIntLeafBlock->maxValueCounter) / 2);

            for (int i = 0; i < treeIntLeafBlock->maxValueCounter - offset; i++) {
                newTreeIntLeafBlock.values[i] = treeIntLeafBlock->values[offset + i];
                newTreeIntLeafBlock.tids[i] = treeIntLeafBlock->tids[offset + i];
            }

            /*
            memcpy((void *) newTreeIntLeafBlock.values, &(treeIntLeafBlock->values[offset]),
                   sizeof(int) * (treeIntLeafBlock->maxValueCounter - offset));
            memcpy((void *) newTreeIntLeafBlock.tids, &(treeIntLeafBlock->tids[offset]),
                   sizeof(tid) * (treeIntLeafBlock->maxValueCounter - offset));
                   */
            newTreeIntLeafBlock.currentValueCounter = treeIntLeafBlock->maxValueCounter - offset;
            treeIntLeafBlock->currentValueCounter = offset;

            BlockNo left = treeIntLeafBlock->blockNo;
            BlockNo right = newTreeIntLeafBlock.blockNo;
            int newValue = treeIntLeafBlock->values[treeIntLeafBlock->currentValueCounter - 1];

            // Falls ein Split gemacht wurde und es keinen Elternknoten gibt --> Elternknoten erstellen + diesen als Root markieren
            if (parentBlockNo == 0) {
                DBBACB newInnerBlock = fixNewBlock();
                TreeIntInnerBlock newTreeIntInnerBlock = TreeIntInnerBlock(newInnerBlock.getBlockNo());

                DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                // Neuer Elternknoten und diesem Pointer + Werte uebergeben

                newTreeIntInnerBlock.insertBlockNo(left, newValue, right);

                startBlock->rootBlockNo = newTreeIntInnerBlock.blockNo;

                startBlock->copyBlockToDBBACB(metaB);
                newTreeIntInnerBlock.copyBlockToDBBACB(newInnerBlock);
                newTreeIntLeafBlock.copyBlockToDBBACB(newLeafBlock);
                treeIntLeafBlock->copyBlockToDBBACB(rootB);


                std::cout << " Meta Block anpassen" << std::endl;
                bufMgr.unfixBlock(newInnerBlock);
                bufMgr.unfixBlock(metaB);
                bufMgr.unfixBlock(newLeafBlock);
                bufMgr.unfixBlock(rootB);

                return ReturnIntInsertValue(0, 0, 0);
            }

            // Andernfalls: Es gibt Elternknoden --> Diesem Pointer + Value + Pointer uebergeben
            // Elternknoten fügt Pointer+Value+Pointer hinzu (ggf. kann dort wieder ein Split entstehen)
            std::cout << " Eltern knoten muss sich um einfuegen kuemmern" << std::endl;

            newTreeIntLeafBlock.copyBlockToDBBACB(newLeafBlock);
            treeIntLeafBlock->copyBlockToDBBACB(rootB);
            bufMgr.unfixBlock(newLeafBlock);
            bufMgr.unfixBlock(rootB);

            return ReturnIntInsertValue(left, newValue, right);

        }

    }
    // Falls der aufgerufene Block kein Blattknoten ist --> Passende BlockNo raussuchen und Funktion rekursiv aufrufen
    if (!treeBlock->leaf) {

        TreeIntInnerBlock *treeIntInnerBlock = (TreeIntInnerBlock *) rootB.getDataPtr();
        treeIntInnerBlock->updatePointers();

        ReturnIntInsertValue r = ReturnIntInsertValue(0, 0, 0);
        for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {
            // Falls der einzufügende Wert größer ist als der Wert ganz rechts im inneren Knoten --> Letzte BlockNo aufrufen
            if (value <= treeIntInnerBlock->values[i]) {
                r = insertValue(treeIntInnerBlock->blockNos[i], value, tid, treeIntInnerBlock->blockNo);
                break;
            }
            // Nach der richtigen BlockNo für den einzufügenden Wert suchen
            if (value > treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]) {
                r = insertValue(treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter], value, tid,
                                treeIntInnerBlock->blockNo);
                break;
            }
        }

        // Wenn der untere Knoten ein BlockNo + Value + BlockNo nach oben gibt, dann muss der Wert + 2 Pointer (=BlockNos) zum inneren Konto hinzugefügt werden
        if (!r.blockNoRight == 0 && !r.blockNoLeft == 0) {
            // Neuen Wert hinzufugen, da Blattknoten gesplitet wurde

            treeIntInnerBlock->printAllValues();

            // Prüfen, wo man BlockNo + Value + BlockNo hinzufügen soll
            for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {
                if (r.value > treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]) {
                    // std::cout << "Fall 1" << std::endl;
                    treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter] = r.value;
                    treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter] = r.blockNoLeft;
                    treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter + 1] = r.blockNoRight;
                    treeIntInnerBlock->printAllValues();
                    // std::cout << "break1";
                    break;
                }

                if (r.value <= treeIntInnerBlock->values[i]) {
                    //std::cout << "Fall 2" << std::endl;
                    treeIntInnerBlock->printAllValues();
                    for (int j = treeIntInnerBlock->currentValueCounter + 1; j > i; j--) {
                        treeIntInnerBlock->blockNos[j] = treeIntInnerBlock->blockNos[j - 1];
                        if (j < treeIntInnerBlock->currentValueCounter + 1) {
                            treeIntInnerBlock->values[j] = treeIntInnerBlock->values[j - 1];
                        }
                    }

/*
                    memcpy(&treeIntInnerBlock->values[i + 1], &treeIntInnerBlock->values[i],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - i));
                    memcpy(&treeIntInnerBlock->blockNos[i + 2], &treeIntInnerBlock->blockNos[i + 1],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - i));
*/
                    treeIntInnerBlock->blockNos[i] = r.blockNoLeft;
                    treeIntInnerBlock->blockNos[i + 1] = r.blockNoRight;
                    treeIntInnerBlock->values[i] = r.value;
                    treeIntInnerBlock->printAllValues();
                    //std::cout << "break2" << std::endl;

                    break;
                }
            }
            treeIntInnerBlock->currentValueCounter++;

            //std::cout << "1" << std::endl;
            // treeIntInnerBlock->printAllValues();

            //std::cout << " Elternknoten war fleisig und hat neuen Knoten eingeuefgt" << std::endl;

            // Nach dem Einfügen der BlockNo + Value + BlockNo muss mit Pech der Elternknoten gesplitet werden
            if (treeIntInnerBlock->currentValueCounter >= treeIntInnerBlock->maxValueCounter) {
                // Split des Elternknoten
                std::cout << "Elternknoten muss gesplitet werden" << std::endl;


                DBBACB newInnerBlock = fixNewBlock();
                TreeIntInnerBlock newTreeIntInnerBlock = TreeIntInnerBlock(newInnerBlock.getBlockNo());

                int offset = std::ceil((treeIntInnerBlock->maxValueCounter) / 2);

                // treeIntInnerBlock->printAllValues();




                // Mach aus 1 inneren Knoten 2 innere Knoten und kopiere ein Teil der Werte + BlockNos

                for (int i = 0; i <= treeIntInnerBlock->maxValueCounter - offset; i++) {
                    newTreeIntInnerBlock.values[i] = treeIntInnerBlock->values[offset + i];
                    newTreeIntInnerBlock.blockNos[i] = treeIntInnerBlock->blockNos[offset + i];
                }

                /*
                memcpy((void *) newTreeIntInnerBlock.values, &(treeIntInnerBlock->values[offset]),
                       sizeof(int) * (treeIntInnerBlock->maxValueCounter - offset));

                memcpy((void *) newTreeIntInnerBlock.blockNos, &(treeIntInnerBlock->blockNos[offset]),
                       sizeof(BlockNo) * (treeIntInnerBlock->maxValueCounter - offset + 1));
*/

                newTreeIntInnerBlock.currentValueCounter = treeIntInnerBlock->maxValueCounter - offset;
                treeIntInnerBlock->currentValueCounter = offset - 1;

                int newParentValue = treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter];
                int leftBlockNo = treeIntInnerBlock->blockNo;
                int rightBlockNo = newTreeIntInnerBlock.blockNo;

                newTreeIntInnerBlock.printAllValues();
                treeIntInnerBlock->printAllValues();

                newTreeIntInnerBlock.copyBlockToDBBACB(newInnerBlock);
                bufMgr.unfixBlock(newInnerBlock);

                treeIntInnerBlock->copyBlockToDBBACB(rootB);
                bufMgr.unfixBlock(rootB);

                // Falls es keinen Eltern-Elternknoten gibt --> Neuen Knoten erstellen + diesen als Root markieren
                if (parentBlockNo == 0) {

                    // Neuen Root Knoten erstellen und Werte uebertragen (Pointer, Value, Pointer)
                    DBBACB newRootBlock = fixNewBlock();
                    TreeIntInnerBlock newTreeRootBlock = TreeIntInnerBlock(newRootBlock.getBlockNo());
                    newTreeRootBlock.values[0] = newParentValue;
                    newTreeRootBlock.blockNos[0] = leftBlockNo;
                    newTreeRootBlock.blockNos[1] = rightBlockNo;
                    newTreeRootBlock.currentValueCounter++;

                    newTreeRootBlock.copyBlockToDBBACB(newRootBlock);
                    bufMgr.unfixBlock(newRootBlock);

                    newTreeRootBlock.printAllValues();

                    std::cout << " Meta Daten anpassen, da neuer Root Block" << std::endl;

                    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                    startBlock->rootBlockNo = newTreeRootBlock.blockNo;
                    startBlock->copyBlockToDBBACB(metaB);
                    bufMgr.unfixBlock(metaB);


                    return ReturnIntInsertValue(0, 0, 0);

                }
                // Falls es noch Eltern-Elternknoten gibt --> Diesem Pointer + Value + Pointer übergeben
                return ReturnIntInsertValue(leftBlockNo, newParentValue, rightBlockNo);

            } else {
                treeIntInnerBlock->copyBlockToDBBACB(rootB);
                bufMgr.unfixBlock(rootB);
                return ReturnIntInsertValue(0, 0, 0);
                // Elternknoten hat neuen Wert bekommen, ist aber nicht voll -> Kein Split bei Eltern-Elternknoten notwendig
            }
        } else {
            // Blattknoten hat kein Split gemacht, also muss Elternknoten auch nicht gesplitet werden

            //treeIntInnerBlock->copyBlockToDBBACB(rootB);
            bufMgr.unfixBlock(rootB);
            return ReturnIntInsertValue(0, 0, 0);
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
    uint varCharSize = 20 * sizeof(char);

    uint intPointerSize = sizeof(int *);
    uint doublePointerSize = sizeof(double *);
    uint charPointerSize = sizeof(char *);
    uint blockNoPointerSize = sizeof(BlockNo *);
    uint tidPointerSize = sizeof(TID *);

    uint basicSize = 4 * boolSize + blockNoSize + 2 * intSize;

    if (leaf) {
        if (AttrTypeEnum::INT == attrType) {
            int returnValue = std::floor(
                    (blockSize - basicSize - tidPointerSize - intPointerSize) / (intSize + tidSize));
            //return 10;
            if (returnValue % 2 == 0) {
                return returnValue - 2;
            } else {
                return returnValue - 3;
            }

            //return std::floor((blockSize - basicSize - tidPointerSize - intPointerSize) / (intSize + tidSize));
        }
        if (AttrTypeEnum::DOUBLE == attrType) {
            return std::floor(
                    (blockSize - basicSize - tidPointerSize - doublePointerSize) / (doubleSize + tidSize));
        }
        if (AttrTypeEnum::VCHAR == attrType) {
            return std::floor((blockSize - basicSize - tidPointerSize - charPointerSize) / (varCharSize + tidSize));
        }
    } else {
        if (AttrTypeEnum::INT == attrType) {
            int returnValue = std::floor((blockSize - basicSize - blockNoSize - intPointerSize - blockNoPointerSize) /
                                         (intSize + blockNoSize));
            //return 10;
            if (returnValue % 2 == 0) {
                return returnValue - 2;
            } else {
                return returnValue - 3;
            }
            //  return 50;
            //return std::floor((blockSize - basicSize - blockNoSize - intPointerSize - blockNoPointerSize) /
            // (intSize + blockNoSize));
        }
        if (AttrTypeEnum::DOUBLE == attrType) {
            return std::floor((blockSize - basicSize - blockNoSize - doublePointerSize - blockNoPointerSize) /
                              (doubleSize + blockNoSize));
        }
        if (AttrTypeEnum::VCHAR == attrType) {
            return std::floor((blockSize - basicSize - blockNoSize - charPointerSize - blockNoPointerSize) /
                              (varCharSize + blockNoSize));
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
    LOG4CXX_DEBUG(logger, "val:\n" + val.toString("\t"));

    std::cout << " AM SUCHEN" << std::endl;
    DBIntType *t = (DBIntType *) &val;

    findTIDsFirstCall(t->getVal(), tids);
    std::cout << " find " << t->getVal() << " and result " << tids.back().toString() << std::endl;
   // printAllBlocks();
}

void DBMyIndex::findTIDsFirstCall(int value, DBListTID &tids) {
    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
    bufMgr.unfixBlock(metaB);
    //printAllBlocks();
    findTIDs(startBlock->rootBlockNo, value, tids, 0);
}

void
DBMyIndex::findTIDs(BlockNo startBlockNo, int value, DBListTID &tids, BlockNo parentBlockNo) {
    std::cout << "FIND VALUE " << value << " StartBlock: " << startBlockNo << std::endl;


    DBBACB rootB = bufMgr.fixBlock(file, startBlockNo, LOCK_EXCLUSIVE);
    TreeBlock *treeBlock = (TreeBlock *) rootB.getDataPtr();

    if (treeBlock->leaf) {

        TreeIntLeafBlock *treeIntLeafBlock = (TreeIntLeafBlock *) rootB.getDataPtr();
        treeIntLeafBlock->updatePointers();
        for (int i = 0; i < treeIntLeafBlock->currentValueCounter; i++) {
            if (treeIntLeafBlock->values[i] == value) {
                tids.push_back(treeIntLeafBlock->tids[i]);
            }
        }
        bufMgr.unfixBlock(rootB);
        return;
    }

    if (!treeBlock->leaf) {

        TreeIntInnerBlock *treeIntInnerBlock = (TreeIntInnerBlock *) rootB.getDataPtr();
        treeIntInnerBlock->updatePointers();

        ReturnIntInsertValue r = ReturnIntInsertValue(0, 0, 0);
        for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {
            if (value <= treeIntInnerBlock->values[i]) {
                findTIDs(treeIntInnerBlock->blockNos[i], value, tids, treeIntInnerBlock->blockNo);
                bufMgr.unfixBlock(rootB);
                return;
            }

            if (value > treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]) {
                findTIDs(treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter], value, tids,
                         treeIntInnerBlock->blockNo);
                bufMgr.unfixBlock(rootB);
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
    DBIntType *t = (DBIntType *) &val;
    std::cout << " insert " << t->getVal() << " und tid " << tid.toString() << std::endl;
    insertValueFirstCall(t->getVal(), tid);
    // printAllBlocks();


}

/**
 * Entfernt alle Tupel aus der Liste der tids.
 * Um schneller auf der richtigen Seite mit dem Entfernen anfangen zu koennen,
 * wird zum Suchen auch noch der zu loeschende value uebergeben
 */
void DBMyIndex::remove(const DBAttrType &val, const list<TID> &tid) {
    LOG4CXX_INFO(logger, "remove()");
    LOG4CXX_DEBUG(logger, "val:\n" + val.toString("\t"));

    DBIntType *t = (DBIntType *) &val;
    std::cout << " remove " << t->getVal() << std::endl;
    removeValueFirstCall(t->getVal(), tid.back());


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
    memcpy(d.getDataPtr(), (void *) &this->blockNo, sizeof(BlockNo) + sizeof(rootBlockNo) + sizeof(nextFreeBlock));
    d.setModified();
}

void TreeBlock::copyBlockToDBBACB(DBBACB d) {
    memcpy(d.getDataPtr(), (void *) &this->leaf, 4 * sizeof(bool) + 2 * sizeof(BlockNo));
    d.setModified();
}

void TreeIntInnerBlock::copyBlockToDBBACB(DBBACB d) {
    //std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = 4 * sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);

    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *), (void *) this->values,
           (this->maxValueCounter) * sizeof(int));

    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *) +
           this->maxValueCounter * sizeof(int),
           (void *) this->blockNos, (this->maxValueCounter + 1) * sizeof(BlockNo));
    d.setModified();
}

void TreeIntInnerBlock::updatePointers() {
    int basicSize = 4 * sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    values = (int *) (&this->leaf + basicSize + sizeof(int *) + sizeof(BlockNo *));
    blockNos = (BlockNo *) (&this->leaf + basicSize + sizeof(int *) + sizeof(BlockNo *) +
                            (this->maxValueCounter) * sizeof(int));
}

int TreeIntInnerBlock::removeBlockNo(BlockNo blockNo) {
    for (int i = 0; i < currentValueCounter + 1; i++) {

    }
}

void TreeIntInnerBlock::insertBlockNo(BlockNo blockNoLeft, int value, BlockNo blockNoRight) {
    this->values[0] = value;
    this->blockNos[0] = blockNoLeft;
    this->blockNos[1] = blockNoRight;
    this->currentValueCounter++;
}


bool TreeIntInnerBlock::insertBlockNo(int value, BlockNo blockNo, bool fromLeft) {
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->blockNos[currentValueCounter + 1] = blockNo;
            this->currentValueCounter++;
            std::cout << this->values[currentValueCounter - 1] << std::endl;
            std::cout << "am ende der Liste einfuegen" << std::endl;
        } else {
            std::cout << " FALL 2" << std::endl;
            // Mitten in der Liste hinzufügen
            this->printAllValues();
            std::cout << "Einfuegen von value" << value << std::endl;
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {

                    for (int j = currentValueCounter + 1; j > i; j--) {
                        if (j < currentValueCounter + 1) {
                            values[j] = values[j - 1];
                        }
                        blockNos[j] = blockNos[j - 1];
                    }


                    //memcpy(&values[i + 1], &values[i], (currentValueCounter - i + 1) * sizeof(int));
                    //memcpy(&blockNos[i + 1], &blockNos[i], (currentValueCounter - i + 1) * sizeof(BlockNo));
                    values[i] = value;
                    blockNos[i] = blockNo;
                    this->currentValueCounter++;
                    break;
                }

            }
            this->printAllValues();
        }
    } else {
        // Am Anfang der Liste hinzufügen (wenn nur ein BlockPointer vorhanden ist)
        std::cout << "Liste Leer" << std::endl;
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


void TreeIntLeafBlock::copyBlockToDBBACB(DBBACB d) {
    // std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = 4 * sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);
    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(int));
    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *) +
           this->maxValueCounter * sizeof(int),
           (void *) this->tids, this->maxValueCounter * sizeof(TID));
    d.setModified();
}

void TreeIntLeafBlock::updatePointers() {
    int basicSize = 4 * sizeof(bool) + 2 * sizeof(BlockNo) + 2 * sizeof(int);
    values = (int *) (&this->leaf + basicSize + sizeof(int *) + sizeof(BlockNo *));
    tids = (TID *) (&this->leaf + basicSize + sizeof(int *) + sizeof(TID *) +
                    (this->maxValueCounter) * sizeof(int));
}

bool TreeIntLeafBlock::insertTID(int value, TID tid) {
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->tids[currentValueCounter] = tid;
            this->currentValueCounter++;
        } else {
            // Mitten in der Liste hinzufügen
            bool positionFound = false;
            int oldValue;
            TID oldTID;

            int oldValue2;
            TID oldTID2;

            for (int i = 0; i < this->currentValueCounter; i++) {
                if (positionFound) {

                    oldValue2 = values[i];
                    oldTID2 = tids[i];

                    values[i] = oldValue;
                    tids[i] = oldTID;

                    oldValue = oldValue2;
                    oldTID = oldTID2;


                }
                if (this->values[i] >= value & !positionFound) {
                    //memcpy(&values[i + 1], &values[i], (currentValueCounter - i) * sizeof(int));
                    //memcpy(&tids[i + 1], &tids[i], (currentValueCounter - i) * sizeof(TID));


                    oldValue = values[i];
                    oldTID = tids[i];

                    values[i] = value;
                    tids[i] = tid;
                    this->currentValueCounter++;
                    positionFound = true;
                    //break;
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

IntUndersizedAndValuePair TreeIntLeafBlock::removeTID(int value, TID tid) {
    // Am Ende der Liste löschen
    if (value == this->values[currentValueCounter - 1]) {
        this->currentValueCounter--;
    } else {
        bool found = false;
        // Mitten in der Liste löschen
        for (int i = 0; i < this->currentValueCounter; i++) {
            if (this->values[i] == value) {
                // memcpy(&values[i], &values[i + 1], (currentValueCounter - i) * sizeof(int));
                // memcpy(&tids[i], &tids[i + 1], (currentValueCounter - i) * sizeof(TID));
                // this->currentValueCounter--;
                //break;
                found = true;
                this->currentValueCounter--;
            }
            if (found) {
                values[i] = values[i + 1];
                tids[i] = tids[i + 1];

            }
        }
        if (found) {

        }
    }
    std::cout << "this->values(0) " << this->values[0] << std::endl;

    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue) / maxValue;

    std::cout << "BlockNo: " << this->blockNo << " und " << this->currentValueCounter << " ?????????????" << std::endl;

    if (divisionResult >= 0.5) {
        IntUndersizedAndValuePair r = IntUndersizedAndValuePair(this->values[this->currentValueCounter - 1], false);
        return r;
    }
    IntUndersizedAndValuePair r = IntUndersizedAndValuePair(this->values[this->currentValueCounter - 1], true);
    return r;
}


IntValueAndTIDPair TreeIntInnerBlock::removeSmallestBlockNo() {
    this->printAllValues();
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {

        IntValueAndTIDPair returnPair = IntValueAndTIDPair(this->values[0], this->blockNos[0], true);

        for (int i = 0; i < this->currentValueCounter; i++) {
            this->values[i] = this->values[i + 1];
            this->blockNos[i] = this->blockNos[i + 1];
        }

        //memcpy(&values[0], &values[1], (this->currentValueCounter) * sizeof(int));
        //memcpy(&blockNos[0], &blockNos[1], (this->currentValueCounter) * sizeof(BlockNo));
        this->currentValueCounter--;

        this->printAllValues();
        return returnPair;
    }
    return IntValueAndTIDPair(0, BlockNo(), false);
}

IntValueAndTIDPair TreeIntInnerBlock::removeBiggestBlockNo() {
    // Nur wenn innerer Knoten genuegend Values hat, kann er ein Paar (value, BlockNo) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        IntValueAndTIDPair returnPair = IntValueAndTIDPair(this->values[this->currentValueCounter - 1],
                                                           this->blockNos[this->currentValueCounter], true,
                                                           this->values[this->currentValueCounter - 2]);
        this->currentValueCounter--;
        return returnPair;
    }
    return IntValueAndTIDPair(0, BlockNo(), false);
}


IntValueAndTIDPair TreeIntLeafBlock::removeSmallestTID() {
    this->printAllValues();
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        std::cout << "Test" << std::endl;
        IntValueAndTIDPair returnPair = IntValueAndTIDPair(this->values[0], this->tids[0], true);
        for (int i = 0; i < (this->currentValueCounter - 1); i++) {
            this->values[i] = this->values[i + 1];
            this->tids[i] = this->tids[i + 1];
        }
        this->tids[this->currentValueCounter - 1] = this->tids[this->currentValueCounter];


        // memcpy(&values[0], &values[1], (this->currentValueCounter) * sizeof(int));
        // memcpy(&tids[0], &tids[1], (this->currentValueCounter) * sizeof(TID));
        this->currentValueCounter--;

        this->printAllValues();
        return returnPair;
    }
    return IntValueAndTIDPair(0, TID(), false);
}

IntValueAndTIDPair TreeIntLeafBlock::removeBiggestTID() {
    // Nur wenn Blattknoten genuegend Values hat, kann er ein Paar (value, tid) abgeben --> Merge
    double currentValue = (double) this->currentValueCounter;
    double maxValue = (double) this->maxValueCounter;
    double divisionResult = (currentValue - 1) / maxValue;
    if (divisionResult >= 0.5) {
        IntValueAndTIDPair returnPair = IntValueAndTIDPair(this->values[this->currentValueCounter - 1],
                                                           this->tids[this->currentValueCounter - 1], true,
                                                           this->values[this->currentValueCounter - 2]);
        this->currentValueCounter--;
        return returnPair;
    }
    return IntValueAndTIDPair(0, TID(), false);
}


void TreeIntLeafBlock::printAllValues() {
    std::cout << "Block No Leaf: " << this->blockNo << " -> ";
    for (int i = 0; i < this->currentValueCounter; i++) {
        std::cout << this->values[i]<<":"<<this->tids[i].toString() << " ";
        //std::cout << this->values[i] << " ";
    }
    std::cout << endl;
}



