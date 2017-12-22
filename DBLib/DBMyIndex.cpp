#include <hubDB/DBMyIndex.h>
#include <hubDB/DBException.h>
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

    std::cout << "hallo" << std::endl;
    int numberOfBlocks = bufMgr.getBlockCnt(file);
    std::cout << "Number of Blocks:" << numberOfBlocks << std::endl;

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

    // TestZwecke
    /*
    insertValueFirstCall(81, TID());
    printAllBlocks();
    insertValueFirstCall(17, TID());
    printAllBlocks();
    insertValueFirstCall(19, TID());
    printAllBlocks();
    insertValueFirstCall(30, TID());
    printAllBlocks();
    insertValueFirstCall(41, TID());
    printAllBlocks();
    insertValueFirstCall(12, TID());
    printAllBlocks();
    insertValueFirstCall(16, TID());
    printAllBlocks();
    insertValueFirstCall(14, TID());
    printAllBlocks();
    insertValueFirstCall(1, TID());
    printAllBlocks();
    insertValueFirstCall(8, TID());
    printAllBlocks();
    insertValueFirstCall(45, TID());
    printAllBlocks();

    insertValueFirstCall(43, TID());
    printAllBlocks();

    insertValueFirstCall(42, TID());
    printAllBlocks();

    insertValueFirstCall(0, TID());
    printAllBlocks();

    insertValueFirstCall(3, TID());
    printAllBlocks();

    removeValueFirstCall(45, TID());
    printAllBlocks();
     */


}

DBMyIndex::~DBMyIndex() {

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

        treeIntLeafBlock->printAllValues();

        std::cout << "Geloescht value: " << value << " und undersized " << r.undersized << std::endl;
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
            std::cout << "i durchgang " << i << std::endl;

            // Falls der Wert größer ist als der letzte Wert im inneren Knoten --> Letzten Pointer (=BlockNo) des inneren Knoten lesen
            if (value > treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]) {
                r = removeValue(
                        treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter], value, tid,
                        treeIntInnerBlock->blockNo);
                position = treeIntInnerBlock->currentValueCounter;
                std::cout << " hier drinnen " << std::endl;
                break;
            }

            // Alle Werte des inneren Knoten durchgehen und gucken, welcher Pointer (=BlockNo) der richtige für den Wert ist
            if (value <= treeIntInnerBlock->values[i]) {
                r = removeValue(treeIntInnerBlock->blockNos[i], value, tid,
                                treeIntInnerBlock->blockNo);
                std::cout << "Knoten darunter undersized: " << r.undersized << " und kleinster Value der Liste: "
                          << r.value << std::endl;
                position = i;

                // Wenn der zu loeschende Wert einem Wert im inneren Knoten entspricht --> Wert im inneren Knoten aendern
                if (value == treeIntInnerBlock->values[i]) {
                    treeIntInnerBlock->values[i] = r.value;
                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    //bufMgr.unfixBlock(rootB);

                    std::cout << "Shit: ich muss Wert aendern im Blattknoten zu dem Wert" << r.value
                              << std::endl;
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

            std::cout << " Position von undersized leaf " << position << std::endl;

            DBBACB undersizedBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position], LOCK_EXCLUSIVE);
            TreeBlock *undersizedTreeBlock = (TreeBlock *) undersizedBlock.getDataPtr();


            // Wenn sich unter dem inneren Knoten kein Blattknoten befindet, sondern wieder ein innerer Knoten
            if (!undersizedTreeBlock->leaf) {


                TreeIntInnerBlock *undersizedTreeIntInnerBlock = (TreeIntInnerBlock *) undersizedBlock.getDataPtr();


                // Moeglickeit 1: aus dem linken inneren Knoten ein Value+BlockNo Paar rueberschieben
                if (position > 0) {
                    std::cout << "Vom linken inneren Knoten etwas klauen" << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *leftNeighbourInnerBlock = (TreeIntInnerBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourInnerBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = leftNeighbourInnerBlock->removeBiggestBlockNo();

                    std::cout << "Versuche Wert zu klauen von links (innere Knoten)" << std::endl;

                    //std::cout << " Ergebnis moeglichkeit 1 Value: " << rIntValueAndTIDPair.parentValue << " " << rIntValueAndTIDPair.successful << std::endl;

                    if (rIntValueAndTIDPair.successful) {

                        if (position >= treeIntInnerBlock->currentValueCounter) {
                            position--;
                        }
                        int oldValue = treeIntInnerBlock->values[position];
                        treeIntInnerBlock->values[position] = rIntValueAndTIDPair.value;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntInnerBlock->insertBlockNo(oldValue, rIntValueAndTIDPair.blockNo);
                        undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);

                        std::cout << " Wert geklaut von links und Elternknoten angepasst "
                                  << rIntValueAndTIDPair.neighbourValue << std::endl;

                        r.undersized = false;
                        return r;

                    }
                    leftNeighbourInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);
                }
                std::cout << "Versuche Wert zu klauen von rechts (innere Knoten)" << std::endl;

                // Moeglickeit 1: aus dem rechten inneren Knoten ein Value+BlockNo Paar rueberschieben
                if (position < (treeIntInnerBlock->currentValueCounter)) {

                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *rightNeighbourInnerBlock = (TreeIntInnerBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourInnerBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = rightNeighbourInnerBlock->removeSmallestBlockNo();
                    if (rIntValueAndTIDPair.successful) {
                        int oldValue = treeIntInnerBlock->values[position];
                        treeIntInnerBlock->values[position] = rIntValueAndTIDPair.value;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntInnerBlock->insertBlockNo(oldValue, rIntValueAndTIDPair.blockNo);
                        undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);


                        std::cout << " Wert geklaut von rechts und Elternknoten angepasst " << rIntValueAndTIDPair.value
                                  << std::endl;
                        std::cout << " Neuer Pointer fuer zu kleinen Knoten " << rIntValueAndTIDPair.blockNo
                                  << std::endl;
                        rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                        bufMgr.unfixBlock(rightNeighbourBlock);
                        r.undersized = false;
                        return r;
                    }
                    rightNeighbourInnerBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    bufMgr.unfixBlock(rightNeighbourBlock);

                }


                std::cout << " Bleibt nur noch Merge der inneren Knoten" << std::endl;

                // Moeglichkeit 3: Falls das Rüberschieben eines Value+BlockNo Paar von links und rechts nicht geklappt hat --> Merge notwendig
                // 3.1: Merge mit linkem Knoten
                if (position > 0) {

                    std::cout << " Merge mit linkem Knoten " << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *leftNeighbourInnerBlock = (TreeIntInnerBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourInnerBlock->updatePointers();

                    std::cout << "VOR MERGE: " << std::endl;
                    leftNeighbourInnerBlock->printAllValues();
                    treeIntInnerBlock->printAllValues();


                    // Aus 2 inneren Knoten 1 inneren Knoten machen, d.h. Werte und BlockNos von einem Knoten zum anderen rüberschieben
                    memcpy(&leftNeighbourInnerBlock->values[leftNeighbourInnerBlock->currentValueCounter],
                           &undersizedTreeIntInnerBlock->values[0],
                           sizeof(int) * undersizedTreeIntInnerBlock->currentValueCounter);

                    memcpy(&leftNeighbourInnerBlock->blockNos[leftNeighbourInnerBlock->currentValueCounter],
                           &undersizedTreeIntInnerBlock->blockNos[0],
                           sizeof(BlockNo) * (undersizedTreeIntInnerBlock->currentValueCounter + 1));


                    leftNeighbourInnerBlock->currentValueCounter = leftNeighbourInnerBlock->currentValueCounter +
                                                                   undersizedTreeIntInnerBlock->currentValueCounter;

                    std::cout << "NACH MERGE: " << std::endl;
                    leftNeighbourInnerBlock->printAllValues();

                    undersizedTreeIntInnerBlock->currentValueCounter = 0;



                    //TODO: loeschen von undersized Knoten, da nicht verwendet

                    // Nach dem Merge von 2 inneren Knoten muss der Elternknoten angepasst werden
                    memcpy(&treeIntInnerBlock->values[position - 1], &treeIntInnerBlock->values[position],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - position + 1));
                    memcpy(&treeIntInnerBlock->blockNos[position - 1], &treeIntInnerBlock->blockNos[position],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position + 1));

                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position - 1] = leftNeighbourInnerBlock->blockNo;
                    treeIntInnerBlock->values[position - 1] = leftNeighbourInnerBlock->values[
                            leftNeighbourInnerBlock->currentValueCounter - 1];

                    treeIntInnerBlock->printAllValues();

                    // Falls der Elternknoten == Rootknoten ist (d.h. keinen Elternknoten mehr hat) und keine Werte mehr beinhaltet
                    // Neuen Rootknoten setzen (und alten Rootknoten löschen)
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = leftNeighbourInnerBlock->blockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);

                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }

                    undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                    bufMgr.unfixBlock(undersizedBlock);

                    undersizedTreeIntInnerBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);

                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);


                    return r;

                }



                // 3.2. Merge mit rechtem Knoten
                if (position < (treeIntInnerBlock->currentValueCounter)) {
                    std::cout << "Merge mit rechtem Knoten" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    TreeIntInnerBlock *rightNeighbourLeafBlock = (TreeIntInnerBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourLeafBlock->updatePointers();

                    std::cout << "VOR MERGE: " << std::endl;
                    undersizedTreeIntInnerBlock->printAllValues();

                    // Aus 2 inneren Knoten 1 inneren Knoten machen, d.h. Werte und BlockNos von einem Knoten zum anderen rüberschieben

                    memcpy(&undersizedTreeIntInnerBlock->values[undersizedTreeIntInnerBlock->currentValueCounter],
                           &rightNeighbourLeafBlock->values[0],
                           sizeof(int) * rightNeighbourLeafBlock->currentValueCounter);

                    memcpy(&undersizedTreeIntInnerBlock->blockNos[undersizedTreeIntInnerBlock->currentValueCounter],
                           &rightNeighbourLeafBlock->blockNos[0],
                           sizeof(BlockNo) * (rightNeighbourLeafBlock->currentValueCounter + 1));

                    undersizedTreeIntInnerBlock->currentValueCounter =
                            undersizedTreeIntInnerBlock->currentValueCounter +
                            rightNeighbourLeafBlock->currentValueCounter;

                    std::cout << "NACH MERGE: " << std::endl;
                    undersizedTreeIntInnerBlock->printAllValues();


                    treeIntInnerBlock->printAllValues();
                    // Nach dem Merge von 2 inneren Knoten muss der Elternknoten angepasst werden
                    memcpy(&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position + 1],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - position));
                    memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position + 1],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position));


                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position] = undersizedTreeIntInnerBlock->blockNo;
                    treeIntInnerBlock->values[position] = undersizedTreeIntInnerBlock->values[
                            undersizedTreeIntInnerBlock->currentValueCounter - 1];

                    treeIntInnerBlock->printAllValues();

                    rightNeighbourLeafBlock->currentValueCounter = 0;
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    bufMgr.unfixBlock(rightNeighbourBlock);

                    undersizedTreeIntInnerBlock->copyBlockToDBBACB(undersizedBlock);
                    bufMgr.unfixBlock(undersizedBlock);

                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);

                    // Falls der Elternknoten == Rootknoten ist (d.h. keinen Elternknoten mehr hat) und keine Werte mehr beinhaltet
                    // Neuen Rootknoten setzen (und alten Rootknoten löschen)
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig!!!!" << std::endl;

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = undersizedTreeIntInnerBlock->blockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
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
                    std::cout << "moeglichkeit 1" << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *leftNeighbourLeafBlock = (TreeIntLeafBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourLeafBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = leftNeighbourLeafBlock->removeBiggestTID();

                    //std::cout << " Ergebnis moeglichkeit 1 Value: " << rIntValueAndTIDPair.parentValue << " " << rIntValueAndTIDPair.successful << std::endl;

                    if (rIntValueAndTIDPair.successful) {
                        //std::cout << "AUS: " << treeIntInnerBlock->values[position-1] << " wird " << rIntValueAndTIDPair.parentValue << std::endl;
                        if (position >= treeIntInnerBlock->currentValueCounter) {
                            position--;
                        }
                        treeIntInnerBlock->values[position] = rIntValueAndTIDPair.neighbourValue;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntLeafBlock->insertTID(rIntValueAndTIDPair.value, rIntValueAndTIDPair.tid);
                        undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);

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
                    std::cout << "zwei" << std::endl;

                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *rightNeighbourLeafBlock = (TreeIntLeafBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourLeafBlock->updatePointers();
                    IntValueAndTIDPair rIntValueAndTIDPair = rightNeighbourLeafBlock->removeSmallestTID();
                    if (rIntValueAndTIDPair.successful) {
                        treeIntInnerBlock->values[position] = rIntValueAndTIDPair.value;
                        treeIntInnerBlock->copyBlockToDBBACB(rootB);
                        bufMgr.unfixBlock(rootB);

                        undersizedTreeIntLeafBlock->insertTID(rIntValueAndTIDPair.value, rIntValueAndTIDPair.tid);
                        undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);
                        bufMgr.unfixBlock(undersizedBlock);


                        std::cout << " Wert geklaut von rechts und Elternknoten angepasst " << rIntValueAndTIDPair.value
                                  << std::endl;
                        rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);
                        bufMgr.unfixBlock(rightNeighbourBlock);
                        r.undersized = false;
                        return r;
                    }


                }


                // Moeglichkeit 3: Falls das Rüberschieben von einem linken oder rechten Blattknoten nicht funktioniert hat --> Merge notwendig
                // 3.1: Merge mit linkem Knoten
                if (position > 0) {

                    std::cout << " Merge mit linkem Knoten " << std::endl;
                    DBBACB leftNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position - 1],
                                                                LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *leftNeighbourLeafBlock = (TreeIntLeafBlock *) leftNeighbourBlock.getDataPtr();
                    leftNeighbourLeafBlock->updatePointers();

                    std::cout << "VOR MERGE: " << std::endl;
                    leftNeighbourLeafBlock->printAllValues();
                    treeIntInnerBlock->printAllValues();

                    // 2 Blattknoten zu 1 Blattknoten verschmelzen
                    memcpy(&leftNeighbourLeafBlock->values[leftNeighbourLeafBlock->currentValueCounter],
                           &undersizedTreeIntLeafBlock->values[0],
                           sizeof(int) * undersizedTreeIntLeafBlock->currentValueCounter);

                    memcpy(&leftNeighbourLeafBlock->tids[leftNeighbourLeafBlock->currentValueCounter],
                           &undersizedTreeIntLeafBlock->tids[0],
                           sizeof(TID) * undersizedTreeIntLeafBlock->currentValueCounter);


                    leftNeighbourLeafBlock->currentValueCounter = leftNeighbourLeafBlock->currentValueCounter +
                                                                  undersizedTreeIntLeafBlock->currentValueCounter;

                    std::cout << "NACH MERGE: " << std::endl;
                    leftNeighbourLeafBlock->printAllValues();

                    undersizedTreeIntLeafBlock->currentValueCounter = 0;



                    //TODO: loeschen von undersized Knoten, da nicht verwendet

                    // Elternknoten anpassen, nachdem 2 Blattknoten zusammengeschmolzen wurden
                    memcpy(&treeIntInnerBlock->values[position - 1], &treeIntInnerBlock->values[position],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - position + 1));
                    memcpy(&treeIntInnerBlock->blockNos[position - 1], &treeIntInnerBlock->blockNos[position],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position + 1));

                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position - 1] = leftNeighbourLeafBlock->blockNo;
                    treeIntInnerBlock->values[position - 1] = leftNeighbourLeafBlock->values[
                            leftNeighbourLeafBlock->currentValueCounter - 1];

                    treeIntInnerBlock->printAllValues();

                    // Falls der Elternknoten der Rootknoten ist und keine Werte mehr beinhaltet --> Neuen Rootknoten setzen und alten löschen
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = leftNeighbourLeafBlock->blockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);

                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
                    }

                    undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);
                    bufMgr.unfixBlock(undersizedBlock);

                    leftNeighbourLeafBlock->copyBlockToDBBACB(leftNeighbourBlock);
                    bufMgr.unfixBlock(leftNeighbourBlock);

                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);


                    return r;

                }


                // 3.2. Merge mit rechtem Knoten
                if (position < (treeIntInnerBlock->currentValueCounter)) {
                    std::cout << "Merge mit rechtem Knoten" << std::endl;
                    DBBACB rightNeighbourBlock = bufMgr.fixBlock(file, treeIntInnerBlock->blockNos[position + 1],
                                                                 LOCK_EXCLUSIVE);
                    TreeIntLeafBlock *rightNeighbourLeafBlock = (TreeIntLeafBlock *) rightNeighbourBlock.getDataPtr();
                    rightNeighbourLeafBlock->updatePointers();

                    std::cout << "VOR MERGE: " << std::endl;
                    undersizedTreeIntLeafBlock->printAllValues();


                    // 2 Blattknoten zu 1 Blattknoten verschmelzen
                    memcpy(&undersizedTreeIntLeafBlock->values[undersizedTreeIntLeafBlock->currentValueCounter],
                           &rightNeighbourLeafBlock->values[0],
                           sizeof(int) * rightNeighbourLeafBlock->currentValueCounter);

                    memcpy(&undersizedTreeIntLeafBlock->tids[undersizedTreeIntLeafBlock->currentValueCounter],
                           &rightNeighbourLeafBlock->tids[0],
                           sizeof(TID) * rightNeighbourLeafBlock->currentValueCounter);

                    undersizedTreeIntLeafBlock->currentValueCounter = undersizedTreeIntLeafBlock->currentValueCounter +
                                                                      rightNeighbourLeafBlock->currentValueCounter;

                    std::cout << "NACH MERGE: " << std::endl;
                    undersizedTreeIntLeafBlock->printAllValues();


                    treeIntInnerBlock->printAllValues();

                    // Nach dem Merge von 2 Blattknoten muss der Elternknoten angepasst werden
                    memcpy(&treeIntInnerBlock->values[position], &treeIntInnerBlock->values[position + 1],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - position));
                    memcpy(&treeIntInnerBlock->blockNos[position], &treeIntInnerBlock->blockNos[position + 1],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - position));
                    treeIntInnerBlock->currentValueCounter = treeIntInnerBlock->currentValueCounter - 1;
                    treeIntInnerBlock->blockNos[position] = undersizedTreeIntLeafBlock->blockNo;
                    treeIntInnerBlock->values[position] = undersizedTreeIntLeafBlock->values[
                            undersizedTreeIntLeafBlock->currentValueCounter - 1];

                    treeIntInnerBlock->printAllValues();

                    rightNeighbourLeafBlock->currentValueCounter = 0;
                    rightNeighbourLeafBlock->copyBlockToDBBACB(rightNeighbourBlock);
                    bufMgr.unfixBlock(rightNeighbourBlock);

                    undersizedTreeIntLeafBlock->copyBlockToDBBACB(undersizedBlock);
                    bufMgr.unfixBlock(undersizedBlock);

                    treeIntInnerBlock->copyBlockToDBBACB(rootB);
                    bufMgr.unfixBlock(rootB);

                    // Falls der Elternknoten der Rootknoten ist und keine Werte mehr beinhaltet --> Neuen Rootknoten setzen + alten löschen
                    if (treeIntInnerBlock->currentValueCounter == 0 && parentBlockNo == 0) {
                        std::cout << " Root Knoten loeschen, da ueberflussig" << std::endl;

                        DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                        TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
                        startBlock->rootBlockNo = undersizedTreeIntLeafBlock->blockNo;
                        startBlock->copyBlockToDBBACB(metaB);
                        bufMgr.unfixBlock(metaB);


                        return r;

                        //TODO: loeschen von inneren Knoten, da nicht verwendet
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
            DBBACB newLeafBlock = bufMgr.fixNewBlock(file);
            TreeIntLeafBlock newTreeIntLeafBlock = TreeIntLeafBlock(newLeafBlock.getBlockNo());
            int offset = std::ceil((treeIntLeafBlock->maxValueCounter) / 2);
            memcpy((void *) newTreeIntLeafBlock.values, &(treeIntLeafBlock->values[offset]),
                   sizeof(int) * (treeIntLeafBlock->maxValueCounter - offset));
            memcpy((void *) newTreeIntLeafBlock.tids, &(treeIntLeafBlock->tids[offset]),
                   sizeof(tid) * (treeIntLeafBlock->maxValueCounter - offset));
            newTreeIntLeafBlock.currentValueCounter = treeIntLeafBlock->maxValueCounter - offset;
            treeIntLeafBlock->currentValueCounter = offset;

            BlockNo left = treeIntLeafBlock->blockNo;
            BlockNo right = newTreeIntLeafBlock.blockNo;
            int newValue = treeIntLeafBlock->values[treeIntLeafBlock->currentValueCounter - 1];

            // Falls ein Split gemacht wurde und es keinen Elternknoten gibt --> Elternknoten erstellen + diesen als Root markieren
            if (parentBlockNo == 0) {
                DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_EXCLUSIVE);
                TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();

                // Neuer Elternknoten und diesem Pointer + Werte uebergeben
                DBBACB newInnerBlock = bufMgr.fixNewBlock(file);
                TreeIntInnerBlock newTreeIntInnerBlock = TreeIntInnerBlock(newInnerBlock.getBlockNo());
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

            // Prüfen, wo man BlockNo + Value + BlockNo hinzufügen soll
            for (int i = 0; i < treeIntInnerBlock->currentValueCounter; i++) {
                if (r.value > treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter - 1]) {
                    // std::cout << "Fall 1" << std::endl;
                    treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter] = r.value;
                    treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter] = r.blockNoLeft;
                    treeIntInnerBlock->blockNos[treeIntInnerBlock->currentValueCounter + 1] = r.blockNoRight;
                    break;
                }

                if (r.value <= treeIntInnerBlock->values[i]) {
                    //std::cout << "Fall 2" << std::endl;
                    memcpy(&treeIntInnerBlock->values[i + 1], &treeIntInnerBlock->values[i],
                           sizeof(int) * (treeIntInnerBlock->currentValueCounter - i));
                    memcpy(&treeIntInnerBlock->blockNos[i + 2], &treeIntInnerBlock->blockNos[i + 1],
                           sizeof(BlockNo) * (treeIntInnerBlock->currentValueCounter - i));
                    treeIntInnerBlock->blockNos[i] = r.blockNoLeft;
                    treeIntInnerBlock->blockNos[i + 1] = r.blockNoRight;
                    treeIntInnerBlock->values[i] = r.value;
                    break;
                }
            }
            treeIntInnerBlock->currentValueCounter++;

            //std::cout << " Elternknoten war fleisig und hat neuen Knoten eingeuefgt" << std::endl;

            // Nach dem Einfügen der BlockNo + Value + BlockNo muss mit Pech der Elternknoten gesplitet werden
            if (treeIntInnerBlock->currentValueCounter >= treeIntInnerBlock->maxValueCounter) {
                // Split des Elternknoten
                std::cout << "Elternknoten muss gesplitet werden" << std::endl;


                DBBACB newInnerBlock = bufMgr.fixNewBlock(file);
                TreeIntInnerBlock newTreeIntInnerBlock = TreeIntInnerBlock(newInnerBlock.getBlockNo());

                int offset = std::ceil((treeIntInnerBlock->maxValueCounter) / 2);

                treeIntInnerBlock->printAllValues();


                // Mach aus 1 inneren Knoten 2 innere Knoten und kopiere ein Teil der Werte + BlockNos
                memcpy((void *) newTreeIntInnerBlock.values, &(treeIntInnerBlock->values[offset]),
                       sizeof(int) * (treeIntInnerBlock->maxValueCounter - offset));

                memcpy((void *) newTreeIntInnerBlock.blockNos, &(treeIntInnerBlock->blockNos[offset]),
                       sizeof(BlockNo) * (treeIntInnerBlock->maxValueCounter - offset + 1));


                newTreeIntInnerBlock.currentValueCounter = treeIntInnerBlock->maxValueCounter - offset;
                treeIntInnerBlock->currentValueCounter = offset - 1;

                int newParentValue = treeIntInnerBlock->values[treeIntInnerBlock->currentValueCounter];
                int leftBlockNo = treeIntInnerBlock->blockNo;
                int rightBlockNo = newTreeIntInnerBlock.blockNo;

                newTreeIntInnerBlock.copyBlockToDBBACB(newInnerBlock);
                bufMgr.unfixBlock(newInnerBlock);

                treeIntInnerBlock->copyBlockToDBBACB(rootB);
                bufMgr.unfixBlock(rootB);

                // Falls es keinen Eltern-Elternknoten gibt --> Neuen Knoten erstellen + diesen als Root markieren
                if (parentBlockNo == 0) {

                    // Neuen Root Knoten erstellen und Werte uebertragen (Pointer, Value, Pointer)
                    DBBACB newRootBlock = bufMgr.fixNewBlock(file);
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
            return 4;
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
            return 4;
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

    DBIntType *t = (DBIntType *) &val;
    std::cout << " find " << t->getVal() << std::endl;
    findTIDsFirstCall(t->getVal(), tids);
    printAllBlocks();
}

void DBMyIndex::findTIDsFirstCall(int value, DBListTID &tids) {
    DBBACB metaB = bufMgr.fixBlock(file, 0, LOCK_SHARED);
    TreeStartBlock *startBlock = (TreeStartBlock *) metaB.getDataPtr();
    bufMgr.unfixBlock(metaB);
    printAllBlocks();
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
    printAllBlocks();


}

/**
 * Entfernt alle Tupel aus der Liste der tids.
 * Um schneller auf der richtigen Seite mit dem Entfernen anfangen zu koennen,
 * wird zum Suchen auch noch der zu loeschende value uebergeben
 */
void DBMyIndex::remove(const DBAttrType &val, const list<TID> &tid) {
    LOG4CXX_INFO(logger, "remove()");
    LOG4CXX_DEBUG(logger, "val:\n" + val.toString("\t"));


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
    memcpy(d.getDataPtr(), (void *) &this->blockNo, sizeof(BlockNo) + sizeof(rootBlockNo));
    d.setModified();
}

void TreeIntInnerBlock::copyBlockToDBBACB(DBBACB d) {
    //std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = 4 * sizeof(bool) + sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);

    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(int));

    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *) +
           this->maxValueCounter * sizeof(int),
           (void *) this->blockNos, (this->maxValueCounter + 1) * sizeof(BlockNo));
    d.setModified();
}

void TreeIntInnerBlock::updatePointers() {
    int basicSize = 4 * sizeof(bool) + sizeof(BlockNo) + 2 * sizeof(int);
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


bool TreeIntInnerBlock::insertBlockNo(int value, BlockNo blockNo) {
    if (this->currentValueCounter > 0) {
        // Am Ende der Liste hinzufügen
        if (value >= this->values[currentValueCounter - 1]) {
            this->values[currentValueCounter] = value;
            this->blockNos[currentValueCounter + 1] = blockNo;
            this->currentValueCounter++;
            std::cout << this->values[currentValueCounter - 1] << std::endl;
            std::cout << "am ende der Liste einfuegen" << std::endl;
        } else {
            // Mitten in der Liste hinzufügen
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memcpy(&values[i + 1], &values[i], (currentValueCounter - i + 1) * sizeof(int));
                    memcpy(&blockNos[i + 1], &blockNos[i], (currentValueCounter - i + 1) * sizeof(BlockNo));
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


void TreeIntLeafBlock::copyBlockToDBBACB(DBBACB d) {
    // std::cout << "BlockNo: " << this->blockNo << " and DBBACB " << d.getBlockNo() << std::endl;
    int basicSize = 4 * sizeof(bool) + sizeof(BlockNo) + 2 * sizeof(int);
    memcpy(d.getDataPtr(), (void *) &this->leaf, basicSize);
    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *), (void *) this->values,
           this->maxValueCounter * sizeof(int));
    memcpy(d.getDataPtr() + basicSize + sizeof(int *) + sizeof(BlockNo *) +
           this->maxValueCounter * sizeof(int),
           (void *) this->tids, this->maxValueCounter * sizeof(TID));
    d.setModified();
}

void TreeIntLeafBlock::updatePointers() {
    int basicSize = 4 * sizeof(bool) + sizeof(BlockNo) + 2 * sizeof(int);
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
            for (int i = 0; i < this->currentValueCounter; i++) {
                if (this->values[i] >= value) {
                    memcpy(&values[i + 1], &values[i], (currentValueCounter - i) * sizeof(int));
                    memcpy(&tids[i + 1], &tids[i], (currentValueCounter - i) * sizeof(TID));
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

IntUndersizedAndValuePair TreeIntLeafBlock::removeTID(int value, TID tid) {
    // Am Ende der Liste löschen
    if (value == this->values[currentValueCounter - 1]) {
        this->currentValueCounter--;
    } else {
        // Mitten in der Liste löschen
        for (int i = 0; i < this->currentValueCounter; i++) {
            if (this->values[i] == value) {
                memcpy(&values[i], &values[i + 1], (currentValueCounter - i) * sizeof(int));
                memcpy(&tids[i], &tids[i + 1], (currentValueCounter - i) * sizeof(TID));
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
        IntUndersizedAndValuePair r = IntUndersizedAndValuePair(this->values[0], false);
        return r;
    }
    IntUndersizedAndValuePair r = IntUndersizedAndValuePair(this->values[0], true);
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
        memcpy(&values[0], &values[1], (this->currentValueCounter) * sizeof(int));
        memcpy(&blockNos[0], &blockNos[1], (this->currentValueCounter) * sizeof(BlockNo));
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
        IntValueAndTIDPair returnPair = IntValueAndTIDPair(this->values[0], this->tids[0], true);
        memcpy(&values[0], &values[1], (this->currentValueCounter) * sizeof(int));
        memcpy(&tids[0], &tids[1], (this->currentValueCounter) * sizeof(TID));
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
        //std::cout << this->values[i]<<":"<<this->tids[i].toString() << " ";
        std::cout << this->values[i] << " ";
    }
    std::cout << endl;
}



