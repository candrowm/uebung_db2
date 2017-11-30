#ifndef HUBDB_DBMYBUFFERMGR_H
#define HUBDB_DBMYBUFFERMGR_H

#include <hubDB/DBBufferMgr.h>

using std::vector;


namespace HubDB {
    namespace Manager {
        class DBMyBufferMgr : public DBBufferMgr {
        public:
            DBMyBufferMgr(bool doThreading, int bufferBlock = STD_BUFFER_BLOCKS);

            ~DBMyBufferMgr() override;

            string toString(string linePrefix = "") const override;

            void unfixBlock(DBBCB &bcb) override;

            void flushBlock(DBBACB &bacb);

            static int registerClass();

        protected:
            bool isBlockOfFileOpen(DBFile &file) const override;

            void closeAllOpenBlocks(DBFile &file) override;

            DBBCB *fixBlock(DBFile &file, BlockNo blockNo, DBBCBLockMode mode, bool read) override;

            int findFramePostionsOfBlock(DBFile &file, BlockNo blockNo);

            int findFramePostionsOfBlock(DBBCB *bcb);


        private:
            DBBCB **bcbList;
            vector<int> unfixedFramesPositionList;
            static LoggerPtr logger;

            int NOT_FOUND_POSITION = -1;

            void removeFixedFrameFromUnfixedFramePositionList(int framePositionOfFixedBlock);

            int findAppropriateReplacementPosition() const;
        };
    }
}

#endif //HUBDB_DBMYBUFFERMGR_H
