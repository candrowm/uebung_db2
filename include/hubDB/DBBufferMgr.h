#ifndef DBBUFFERMGR_H_
#define DBBUFFERMGR_H_

#include <hubDB/DBManager.h>
#include <hubDB/DBFileMgr.h>
#include <hubDB/DBBCB.h>
#include <hubDB/DBBACB.h>

using namespace HubDB::File;
using namespace HubDB::Buffer;

namespace HubDB{
	namespace Manager{
		class DBBufferMgr : public DBManager
		{
		public:
			DBBufferMgr (bool doThreading,int bufferBlock = STD_BUFFER_BLOCKS);
 			virtual ~DBBufferMgr ( );
			virtual string toString(string linePrefix="") const;

			void createFile(const string & name){fileMgr.createFile(name);};
			void createDirectory(const string & name){fileMgr.createDirectory(name);};

			void dropFile(const string & name);
			void dropDirectory(const string & name){fileMgr.dropDirectory(name);};
			
			DBFile & openFile(const string & name){return fileMgr.openFile(name);};
			void closeFile(DBFile & file);

			uint getBlockCnt(DBFile & file){return fileMgr.getBlockCnt(file);};
			void setBlockCnt(DBFile & file,uint cnt){fileMgr.setBlockCnt(file,cnt);};

			DBBACB fixBlock(DBFile & file,BlockNo blockNo,DBBCBLockMode mode);
			DBBACB fixNewBlock(DBFile & file);
			DBBACB fixEmptyBlock(DBFile & file,BlockNo blockNo);

			void unfixBlock(const DBBACB & bacb);
			void flushBlock(DBBACB & bacb);
			DBBACB upgradeToExclusive(const DBBACB & bacb);
			const DBBACB downgradeToShared(const DBBACB & bacb);

			int blockCounter = 0;

		protected:
			virtual bool isBlockOfFileOpen(DBFile & file) const = 0;
			virtual void closeAllOpenBlocks(DBFile & file) = 0;
			virtual DBBCB * fixBlock(DBFile & file,BlockNo blockNo,DBBCBLockMode mode,bool read)=0;
			virtual void unfixBlock(DBBCB & bcb) = 0;
			
			void flushBCBBlock(DBBCB & bcb);
            void waitForLock();
            void emitSignal();

			DBFileMgr fileMgr;
			int maxBlockCnt;
			
		private:
  			static LoggerPtr logger;
            pthread_cond_t cond;
		};
    }
    namespace Exception{
        
        class DBBufferMgrException : public DBException{
        public:
            DBBufferMgrException(const std::string& msg);
            DBBufferMgrException(const DBBufferMgrException&);
            DBBufferMgrException& operator=(const DBBufferMgrException&);
        };
	}
}

#endif /*DBBUFFERMGR_H_*/
