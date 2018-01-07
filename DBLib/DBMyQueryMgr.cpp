#include <hubDB/DBMyQueryMgr.h>
#include <hubDB/DBTable.h>
#include <hubDB/DBIndex.h>
#include <hubDB/DBException.h>

using namespace HubDB::Exception;
using namespace HubDB::Manager;
using namespace HubDB::Table;
using namespace HubDB::Index;

LoggerPtr DBMyQueryMgr::logger(Logger::getLogger("HubDB.Query.DBMyQueryMgr"));

int rMyQMgr = DBMyQueryMgr::registerClass();

extern "C" void *createDBMyQueryMgr(int nArgs, va_list &ap);

int DBMyQueryMgr::registerClass() {
    setClassForName("DBMyQueryMgr", createDBMyQueryMgr);
    return 0;
}

extern "C" void *createDBMyQueryMgr(int nArgs, va_list &ap) {
    if (nArgs != 2)
        throw DBException("Invalid number of arguments");
    DBServerSocket *socket = va_arg(ap, DBServerSocket *);
    DBSysCatMgr *sysCat = va_arg(ap, DBSysCatMgr *);
    return new DBMyQueryMgr(*socket, *sysCat);
}

DBMyQueryMgr::DBMyQueryMgr(DBServerSocket &socket, DBSysCatMgr &sysCatMgr) :
        DBQueryMgr(socket, sysCatMgr) {
    if (logger != NULL) LOG4CXX_INFO(logger, "DBMyQueryMgr()");
    std::cout << "MyQueryManager activated!" << std::endl;
}

string DBMyQueryMgr::toString(string linePrefix) const {
    stringstream ss;
    ss << linePrefix << "[DBMyQueryManager]" << endl;
    ss << linePrefix << "----------------" << endl;
    return ss.str();
}

void DBMyQueryMgr::selectJoinTuple(DBTable *table[2], uint attrJoinPos[2], DBListPredicate where[2],
                                   DBListJoinTuple &tuples) {
    bool hasIndex[2];
    getIndexInformation(table, attrJoinPos, hasIndex);

    if (hasIndex[0] && hasIndex[1]) {
        LOG4CXX_INFO(logger, "Index on both join attributes");
        bool leftRelationIsOuterRelation = true;

        if (leftTableHasMorePages(table)) {
            leftRelationIsOuterRelation = false;
        }

        indexNestedLoopJoin(table, attrJoinPos, where, tuples, leftRelationIsOuterRelation);
    } else if (hasIndex[0]) {
        LOG4CXX_INFO(logger, "Index on left join attribute");
        indexNestedLoopJoin(table, attrJoinPos, where, tuples, false);
    } else if (hasIndex[1]) {
        LOG4CXX_INFO(logger, "Index on right join attribute");
        indexNestedLoopJoin(table, attrJoinPos, where, tuples, true);
    } else {
        LOG4CXX_INFO(logger, "No Index on both join attributes");
        nestedLoopJoinNoIndex(table, attrJoinPos, where, tuples);
    }
}

void DBMyQueryMgr::indexNestedLoopJoin(DBTable *table[2], uint attrJoinPos[2], DBListPredicate where[2],
                                       DBListJoinTuple &tuples, bool leftRelationIsOuterRelation) {
    LOG4CXX_INFO(logger, "indexNestedLoopJoin()");
    DBListTuple outerRelationTuples;
    int outerRelationPos = getOuterRelationPosition(leftRelationIsOuterRelation);
    int innerRelationPos = getInnerRelationPosition(leftRelationIsOuterRelation);

    selectTuple(table[outerRelationPos], where[outerRelationPos], outerRelationTuples);

    uint attrJoinPosOfOuter = attrJoinPos[outerRelationPos];
    uint attrJoinPosOfInner = attrJoinPos[innerRelationPos];

    const DBRelDef &innerRelDef = table[innerRelationPos]->getRelDef();

    QualifiedName qname;
    strcpy(qname.relationName, innerRelDef.relationName().c_str());
    strcpy(qname.attributeName, innerRelDef.attrDef(attrJoinPosOfInner).attrName().c_str());

    DBListTuple::iterator outerTupelIterator = outerRelationTuples.begin();
    while (outerTupelIterator != outerRelationTuples.end()) {
        DBTuple &outerTupel = (*outerTupelIterator);
        const DBAttrType &outerAttrVal = outerTupel.getAttrVal(attrJoinPosOfOuter);

        DBListTID innerTidListTmp;
        DBIndex *index = NULL;
        try {
            index = sysCatMgr.openIndex(connectDB, qname, READ);
            index->find(outerAttrVal, innerTidListTmp);
            innerTidListTmp.sort();
            delete index;
        } catch (DBException e) {
            if (index != NULL)
                delete index;
            throw e;
        }

        DBListTuple innerTupelListTmp;
        table[innerRelationPos]->readTIDs(innerTidListTmp, innerTupelListTmp);

        DBListTuple::iterator innerTupelIterator = innerTupelListTmp.begin();
        while (innerTupelIterator != innerTupelListTmp.end()) {
            DBTuple &innerTupel = (*innerTupelIterator);

            if (isTupelPassingWhereCondition(innerTupel, attrJoinPosOfInner, where[innerRelationPos])) {
                pair<DBTuple, DBTuple> joinPair;
                DBTuple left;
                DBTuple right;

                if (outerRelationPos == 0) {
                    left = outerTupel;
                    right = innerTupel;
                } else {
                    left = innerTupel;
                    right = outerTupel;
                }

                joinPair.first = left;
                joinPair.second = right;
                tuples.push_back(joinPair);
            }
            innerTupelIterator++;
        }
        outerTupelIterator++;
    }
}

int DBMyQueryMgr::getOuterRelationPosition(bool leftRelationIsOuterRelation) const {
    int outerRelationPos = 0;

    if (!leftRelationIsOuterRelation) {
        outerRelationPos = 1;
    }
    return outerRelationPos;
}

int DBMyQueryMgr::getInnerRelationPosition(bool leftRelationIsOuterRelation) const {
    if (getOuterRelationPosition(leftRelationIsOuterRelation) == 1) {
        return 0;
    }
    return 1;
}


//Algorithmus aus SimpleQueryManager
void DBMyQueryMgr::nestedLoopJoinNoIndex(DBTable *table[2],
                                         uint attrJoinPos[2],
                                         DBListPredicate where[2],
                                         DBListJoinTuple &tuples) {
    LOG4CXX_INFO(logger, "nestedLoopJoin()");

    DBListTuple l[2];
    for (uint i = 0; i < 2; ++i) {
        selectTuple(table[i], where[i], l[i]);
    }
    DBListTuple::iterator i = l[0].begin();
    while (i != l[0].end()) {
        DBTuple &left = (*i);

        DBListTuple::iterator u = l[1].begin();
        while (u != l[1].end()) {
            DBTuple &right = (*u);

            if (left.getAttrVal(attrJoinPos[0]) == right.getAttrVal(attrJoinPos[1])) {
                LOG4CXX_DEBUG(logger, "left:\n" + left.toString("\t"));
                LOG4CXX_DEBUG(logger, "right:\n" + right.toString("\t"));
                pair<DBTuple, DBTuple> p;
                p.first = left;
                p.second = right;
                tuples.push_back(p);
            }
            ++u;
        }
        ++i;
    }
}

bool DBMyQueryMgr::leftTableHasMorePages(DBTable *const *table) const {
    return table[0]->getPageCnt() >= table[1]->getPageCnt();
}

void DBMyQueryMgr::getIndexInformation(DBTable *table[2], uint attrJoinPos[2], bool (&hasIndex)[2]) {
    hasIndex[0] = false;
    hasIndex[1] = false;

    for (int i = 0; i < 2; i++) {
        const DBRelDef &relDef = table[i]->getRelDef();
        const DBAttrDef &attrDef = relDef.attrDef(attrJoinPos[i]);
        if (attrDef.isIndexed()) {
            hasIndex[i] = true;
        }
    }
}

//Algorithmus aus SimpleQueryManager
void DBMyQueryMgr::selectTuple(DBTable *table, DBListPredicate &where, DBListTuple &tuple) {
    LOG4CXX_INFO(logger, "selectTuple()");
    LOG4CXX_DEBUG(logger, "table:\n" + table->toString("\t"));
    LOG4CXX_DEBUG(logger, "where: " + TO_STR(where));

    DBListTuple l;
    TID t;
    t.page = 0;
    t.slot = 0;
    list<uint> posList;//gibt fuer jeden Praedikat die Position des Attributes innerhalb der Relationsdefinition an
    list<bool> checkList;//gibt fuer jeden Praedikat an, ob der Attribut indeziert ist oder nicht. kein Index --> true
    DBListTID tidList;//TIDs der Tupel, die bei einem indeziertem Praedikat existieren
    const DBRelDef &def = table->getRelDef(); //Definition der Struktur der Relation z.B. Relationsname, Attributname, ...
    QualifiedName qname;//Relationsname und Attributname jedes Praedikates
    bool indexUsed = false;

    strcpy(qname.relationName, def.relationName().c_str());

    DBListPredicate::iterator u = where.begin();
    while (u != where.end()) {
        DBPredicate &p = *u;//Praedikat, der auf Gleichheit prueft z.B. Relationsname.Attributname = Wert

        if (strcmp(def.relationName().c_str(), p.name().relationName) != 0)
            throw DBQueryMgrException("Predicate missmatch");
        DBAttrDef adef = def.attrDef(
                p.name().attributeName);//Definition des Attributes eines Praedikates, aber nicht der Attributwert selbst!

        if (adef.isIndexed() == true) {
            checkList.push_back(false);
            strcpy(qname.attributeName, adef.attrName().c_str());
            DBListTID tidListTmp;
            DBIndex *index = NULL;
            try {
                index = sysCatMgr.openIndex(connectDB, qname, READ);
                if (indexUsed == true) {
                    index->find(p.val(), tidListTmp);
                    tidListTmp.sort();
                } else {
                    index->find(p.val(), tidList);//findet TID fuer den Vergleichswert des Praedikates
                    tidList.sort();
                }
                delete index;
            } catch (DBException e) {
                if (index != NULL)
                    delete index;
                throw e;
            }
            if (indexUsed == true) {
                DBListTID tidListNew;
                set_intersection(tidList.begin(), tidList.end(), tidListTmp.begin(), tidListTmp.end(),
                                 std::inserter(tidListNew, tidListNew.begin()));
                tidList = tidListNew;
            } else {
                indexUsed = true;
            }
            LOG4CXX_DEBUG(logger, "tidList: " + TO_STR(tidList));
            if (tidList.size() == 0)
                break;
        } else {
            checkList.push_back(true);
        }
        posList.push_back(adef.attrPos());
        ++u;
    }

    do {
        l.clear();
        if (indexUsed == true) {
            table->readTIDs(tidList, l);
        } else {
            t = table->readSeqFromTID(t, 100, l);
        }
        LOG4CXX_DEBUG(logger, "read " + TO_STR(l.size()) + " tuples");
        DBListTuple::iterator i = l.begin();
        while (l.end() != i) {
            bool match = true;
            u = where.begin();
            list<uint>::iterator ii = posList.begin();
            list<bool>::iterator ic = checkList.begin();
            while (match == true && u != where.end()) {
                DBPredicate &p = *u;
                if (*ic == true &&
                    !(p.val() == (*i).getAttrVal(*ii)))
                    match = false;
                ++ii;
                ++ic;
                ++u;
            }
            if (match == true) {
                LOG4CXX_DEBUG(logger, "tuple: " + (*i).toString("\t"));
                tuple.push_back(*i);
            }
            ++i;
        }
    } while (l.size() == 100 && indexUsed == false);
    LOG4CXX_DEBUG(logger, "return");
}

bool DBMyQueryMgr::isTupelPassingWhereCondition(DBTuple &tuple, uint positionOfAttr, DBListPredicate where) {
    bool match = true;
    DBListPredicate::iterator predicateIterator = where.begin();

    while (match == true && predicateIterator != where.end()) {
        DBPredicate &predicate = *predicateIterator;

        if (!(predicate.val() == tuple.getAttrVal(positionOfAttr))) {
            match = false;
        }
        ++predicateIterator;
    }
    return match;
}
