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

void DBMyQueryMgr::selectTuple(DBTable * table,DBListPredicate & where,DBListTuple & tuple) {

}

void DBMyQueryMgr::selectJoinTuple(DBTable * table[2],uint attrJoinPos[2],DBListPredicate where[2],DBListJoinTuple & tuples) {

}



