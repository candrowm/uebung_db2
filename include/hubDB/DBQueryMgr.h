#ifndef DBQUERYMGR_H_
#define DBQUERYMGR_H_

#include <hubDB/DBSysCatMgr.h>
#include <hubDB/DBServerSocket.h>
#include <hubDB/DBTypes.h>

using namespace HubDB::Socket;
using namespace HubDB::Types;

namespace HubDB{
	namespace Manager{
		class DBQueryMgr{
		public:
			DBQueryMgr(DBServerSocket & socket,DBSysCatMgr & sysCatMgr);
  			virtual ~DBQueryMgr();
			string toString(string linePrefix="") const;
			DBServerSocket * getSocket(){ return &socket;};

			void process();

  			void createDB(char* dbName);
  			void dropDB(char* dbName);
  			void connectTo(char* dbName);
  			void disconnect();
  			void listTables();
  			void getSchemaForTable(char * table);
  			void createTable(const RelDefStruct & def);
  			void dropTable(char* table);
  			void createIndex(const QualifiedName & qname,char * type);
  			void dropIndex(const QualifiedName & qname);
  			bool insertInto(char * table,DBTuple * values,bool print=true);
  			void deleteFromTable(char * table,DBListPredicate * where);
            void importTab(char* fileName, char *table);
            void exportTab(char *table,char * fileName);
            void select(DBListQualifiedName * projection,char * relName,DBJoin * join,DBListPredicate * where);
        protected:
            
        	virtual void selectTuple(DBTable * table,DBListPredicate & where,DBListTuple & tuple) = 0;
        	virtual void selectJoinTuple(DBTable * table[2],uint attrJoinPos[2],DBListPredicate where[2],DBListJoinTuple & tuples) = 0;
        	
		protected:
			DBServerSocket & socket;
			DBSysCatMgr & sysCatMgr;
			string connectDB;
			bool isConnected;
			DBBACB * sysCatHdl;
		private:
			static LoggerPtr logger;
		};
	}
    namespace Exception{
        class DBQueryMgrException : public DBRuntimeException
        {
            public:
                DBQueryMgrException(const std::string& msg);
                DBQueryMgrException(const DBQueryMgrException&);
                DBQueryMgrException& operator=(const DBQueryMgrException&);
        };
        class DBQueryMgrNoConnectionException : public DBQueryMgrException
        {
            public:
            	DBQueryMgrNoConnectionException();
                DBQueryMgrNoConnectionException(const DBQueryMgrNoConnectionException&);
                DBQueryMgrNoConnectionException& operator=(const DBQueryMgrNoConnectionException&);
        };
	}
}

#endif /*DBQUERYMGR_H_*/
