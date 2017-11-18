#ifndef DBINDEX_H_
#define DBINDEX_H_

#include <hubDB/DBTypes.h>
#include <hubDB/DBBufferMgr.h>

using namespace HubDB::Types;
using namespace HubDB::Manager;

namespace HubDB{
    namespace Exception{
        class DBIndexException : public DBRuntimeException
        {
            public:
                DBIndexException(const std::string& msg);
                DBIndexException(const DBIndexException&);
                DBIndexException& operator=(const DBIndexException&);
        };
        class DBIndexUniqueKeyException : public DBIndexException
        {
            public:
                DBIndexUniqueKeyException(const std::string& msg);
                DBIndexUniqueKeyException(const DBIndexUniqueKeyException&);
                DBIndexUniqueKeyException& operator=(const DBIndexUniqueKeyException&);
        };
	}
	namespace Index{
		class DBIndex{
			public:
				DBIndex(DBBufferMgr & bufferMgr,
						DBFile & file,
						enum AttrTypeEnum attrType,
						ModType mode,
						bool unique):
					bufMgr(bufferMgr),
					file(file),
					attrType(attrType),
					mode(mode),
					unique(unique)
					{
                        attrTypeSize=DBAttrType::getSize4Type(attrType);
                        if(logger!=NULL){
                            LOG4CXX_INFO(logger,"DBIndex()");
                            LOG4CXX_DEBUG(logger,"this:\n"+this->toString("\t"));
                        }
					};
				virtual ~DBIndex(){ LOG4CXX_INFO(logger,"~DBIndex()"); LOG4CXX_DEBUG(logger,"this:\n"+toString("\t"));};
				virtual string toString(string linePrefix="") const;
				
				virtual void initializeIndex() = 0;
				virtual void find(const DBAttrType & val,DBListTID & tids) = 0;
				virtual void insert(const DBAttrType & val,const TID & tid) = 0;
				virtual void remove(const DBAttrType & val,const DBListTID & tid) = 0;
				virtual bool isIndexNonUniqueAble()= 0;
				bool isUnique()const{ return unique; };
				
			protected:
                DBBufferMgr & bufMgr;
				DBFile & file;
				enum AttrTypeEnum attrType;
                size_t attrTypeSize;
				ModType mode;
				bool unique;
			private:				
	  			static LoggerPtr logger;
		};
	}
}


#endif /*DBINDEX_H_*/
