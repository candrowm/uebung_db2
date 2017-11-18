#ifndef DBBCB_H_
#define DBBCB_H_

#include <hubDB/DBFileBlock.h>

using namespace HubDB::File;

namespace HubDB{
	namespace Buffer{
        enum DBBCBLockMode{
            LOCK_FREE = 0,
            LOCK_SHARED = 1,
            LOCK_INTWRITE = 2,
            LOCK_EXCLUSIVE = 4
        };
        
		class DBBCB
		{
		public:
                      
			DBBCB(DBFile & file,const BlockNo blockNum);
		    virtual ~DBBCB();
			virtual string toString(string linePrefix="") const;
			char * getDataPtr() { return fileBlock.getDataPtr();};
			const char * getDataPtr() const { return fileBlock.getDataPtr();};
			void setModified();
			void unsetModified(){modified = false;};
			bool getModified()const {return modified;};
			void setDirty();
			bool getDirty()const {return dirty;};
			DBFileBlock & getFileBlock(){return fileBlock;};
			bool grantSharedAccess(){return grantAccess(LOCK_SHARED);};
			bool grantAccess(DBBCBLockMode mode);
			bool grantExclusiveAccess(){return grantAccess(LOCK_EXCLUSIVE);};
			bool grantIntensionWriteAccess(){return grantAccess(LOCK_INTWRITE);};
            bool upgradeLock();
            bool downgradeLock();
			void unlock();
            bool isUnlocked()const{ return mode==LOCK_FREE ? true : false;};
            bool isExclusive()const{ return mode==LOCK_EXCLUSIVE ? true : false;};
            static string LockMode2String(DBBCBLockMode mode);
            DBBCBLockMode getLockMode4Thread()const;

        protected:
            DBFileBlock fileBlock;
            bool modified;  
            bool dirty;
            DBBCBLockMode mode;
            map<pthread_t,DBBCBLockMode> threadIdsToMode;
		private:
  			static LoggerPtr logger;
		};
    }
    namespace Exception{
        class DBBCBException : public DBException{
        public:
            DBBCBException(const std::string& msg);
            DBBCBException(const DBBCBException&);
            DBBCBException& operator=(const DBBCBException&);
        };
	}
}

#endif /*DBBCB_H_*/
