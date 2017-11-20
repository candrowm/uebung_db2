#ifndef DBRANDOMBUFFERMGR_H_
#define DBRANDOMBUFFERMGR_H_

#include <hubDB/DBBufferMgr.h>

namespace HubDB{
	namespace Manager{
		class DBRandomBufferMgr : public DBBufferMgr
		{
		
		public:
			DBRandomBufferMgr (bool doThreading,int bufferBlock = STD_BUFFER_BLOCKS);
 			~DBRandomBufferMgr ();
			string toString(string linePrefix="") const;

			void unfixBlock(DBBCB & bcb);
			void flushBlock(DBBACB & bacb);

			static int registerClass();

		protected:
			bool isBlockOfFileOpen(DBFile & file) const;
			void closeAllOpenBlocks(DBFile & file);
			DBBCB * fixBlock(DBFile & file,BlockNo blockNo,DBBCBLockMode mode,bool read);
			int findBlock(DBFile & file,BlockNo blockNo);
			int findBlock(DBBCB * bcb);


			void setBit(int i){ bitMap[i/32] |= (1<<(i%32));}
			void unsetBit(int i){ bitMap[i/32] &= (~(1<<(i%32)));}
			int getBit(int i)const { return (bitMap[i/32] & (1<<(i%32))) != 0 ? 1 : 0;}
			
		private:
			DBBCB ** bcbList;
			int * bitMap;
			int mapSize;
  			static LoggerPtr logger;
		};
	}
}

#endif /*DBRANDOMBUFFERMGR_H_*/
