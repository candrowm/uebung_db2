#ifndef DBSIMPLEQUERYMGR_H_
#define DBSIMPLEQUERYMGR_H_

#include <hubDB/DBQueryMgr.h>

namespace HubDB {
    namespace Manager {
        class DBMyQueryMgr : public DBQueryMgr {
        public:
            DBMyQueryMgr(DBServerSocket &socket, DBSysCatMgr &sysCatMgr);
            string toString(string linePrefix) const;

            void selectTuple(DBTable * table,DBListPredicate & where,DBListTuple & tuple);
            void selectJoinTuple(DBTable * table[2],uint attrJoinPos[2],DBListPredicate where[2],DBListJoinTuple & tuples);

            static int registerClass();

        private:
            static LoggerPtr logger;

            void nestedLoopJoinNoIndex(DBTable * table[2],uint attrJoinPos[2],DBListPredicate where[2],DBListJoinTuple & tuples);

            void getIndexInformation(DBTable * table[2],uint attrJoinPos[2], bool (&hasIndex)[2]);
        };
    }
}

#endif
