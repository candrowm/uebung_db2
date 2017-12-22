#ifndef DBMYINDEX_H_
#define DBMYINDEX_H_

#include <hubDB/DBIndex.h>
#include <cmath>

namespace HubDB {
    namespace Index {

        struct ReturnIntInsertValue {
            BlockNo blockNoLeft;
            int value;
            BlockNo blockNoRight;

            ReturnIntInsertValue(BlockNo leftBlockNo, int val, BlockNo rightBlockNo) {
                blockNoLeft = leftBlockNo;
                value = val;
                blockNoRight = rightBlockNo;
            }
        };

        struct IntValueAndTIDPair {
            int value;
            TID tid;
            bool successful;
            int neighbourValue;
            BlockNo blockNo;


            IntValueAndTIDPair(int val, TID t){
                value = val;
                tid = t;
                successful = true;
            }

            IntValueAndTIDPair(int val, TID t, bool success){
                value = val;
                tid = t;
                successful = success;
            }

            IntValueAndTIDPair(int val, TID t, bool success, int neighbourVal){
                value = val;
                tid = t;
                successful = success;
                neighbourValue = neighbourVal;
            }

            IntValueAndTIDPair(int val, BlockNo blockN, bool success){
                value = val;
                blockNo = blockN;
                successful = success;
            }

            IntValueAndTIDPair(int val, BlockNo blockN, bool success,  int neighbourVal){
                value = val;
                blockNo = blockN;
                successful = success;
                neighbourValue = neighbourVal;
            }
        };

        struct IntUndersizedAndValuePair {
            bool undersized;
            int value;

            IntUndersizedAndValuePair(int val, bool undersize){
                undersized = undersize;
                value = val;
            }
        };

        class TreeBlock {
        public:
            bool leaf;
            bool intBlock;
            bool doubleBlock;
            bool varCharBlock;
            BlockNo blockNo;
            int maxValueCounter;
            int currentValueCounter;
        public:
            TreeBlock(BlockNo blockNo) {
                this->leaf = false;
                this->intBlock = false;
                this->doubleBlock = false;
                this->varCharBlock = false;
                this->blockNo = blockNo;
                this->currentValueCounter = 0;
            }

            TreeBlock() {}

            uint calculateMaxCounter(AttrTypeEnum attrType, bool leaf);

            void printAllBlocks();
        };

        /*      START BLOCK      */
        class TreeStartBlock{
        public:
            BlockNo blockNo;
            BlockNo rootBlockNo;
            TreeStartBlock(){
                this->blockNo = BlockNo(0);
            }
            void copyBlockToDBBACB(DBBACB d);
        };

        /*      INT BLOCK      */
        class TreeIntInnerBlock : public TreeBlock {
        public:
            int *values;
            BlockNo *blockNos;


        public:
            TreeIntInnerBlock(BlockNo blockNo) : TreeBlock(blockNo) {
                this->intBlock = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::INT, false);
                this->values = new int[maxValueCounter];
                this->blockNos = new BlockNo[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    blockNos[i] = i;
                }
            };

            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

            void insertValue(int value);

            int removeBlockNo(BlockNo blockNo);

            void printAllValues();

            bool insertBlockNo(int value, BlockNo blockNo);

            void insertBlockNo(BlockNo blockNoLeft, int value, BlockNo BlockNoRight);

            IntValueAndTIDPair removeSmallestBlockNo();

            IntValueAndTIDPair removeBiggestBlockNo();
        };
        class TreeIntLeafBlock : public TreeBlock {
        public:
            int *values;
            TID *tids;

        public:
            TreeIntLeafBlock(BlockNo blockNo) : TreeBlock(blockNo) {
                this->intBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::INT, true);
                this->values = new int[maxValueCounter];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                   // values[i] = i;
                    tids[i] = TID();
                }
            };

            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

            void insertTID(TID tid);

            bool insertTID(int value, TID tid);

            void printAllValues();


            IntUndersizedAndValuePair removeTID(int value, TID tid);

            IntValueAndTIDPair removeSmallestTID(int value, TID tid);

            IntValueAndTIDPair removeBiggestTID(int value, TID tid);

            IntValueAndTIDPair removeBiggestTID();

            IntValueAndTIDPair removeSmallestTID();
        };


        /*      DOUBLE BLOCK      */
        class TreeDoubleInnerBlock : public TreeBlock {
        public:
            double *values;
            BlockNo *blockNos;

        public:
            TreeDoubleInnerBlock(BlockNo blockNo) : TreeBlock(blockNo) {
                this->doubleBlock = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::DOUBLE, false);
                this->values = new double[maxValueCounter];
                this->blockNos = new BlockNo[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    blockNos[i] = i;
                }
            };

            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

        };

        class TreeDoubleLeafBlock : public TreeBlock {
        public:
            double *values;
            TID *tids;

        public:
            TreeDoubleLeafBlock(BlockNo blockNo) : TreeBlock(blockNo) {
                this->doubleBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::DOUBLE, true);
                this->values = new double[maxValueCounter];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    tids[i] = TID();
                }
            };

            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();
        };

        /*      VARCHAR BLOCK      */
        class TreeVarCharInnerBlock : public TreeBlock {
        public:
            char *values;
            BlockNo *blockNos;

        public:
            TreeVarCharInnerBlock(BlockNo blockNo) : TreeBlock(blockNo) {
                this->varCharBlock= true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::VCHAR, false);
                this->values = new char[maxValueCounter*20];
                this->blockNos = new BlockNo[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = 'a';
                    blockNos[i] = i;
                }
            };

            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

        };

        class TreeVarCharLeafBlock : public TreeBlock {
        public:
            char *values;
            TID *tids;

        public:
            TreeVarCharLeafBlock(BlockNo blockNo) : TreeBlock(blockNo) {
                this->varCharBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::VCHAR, true);
                this->values = new char[maxValueCounter * 20];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = 'a';
                    tids[i] = TID();
                }
            };

            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();
        };




        class DBMyIndex : public DBIndex {

        public:
            DBMyIndex(DBBufferMgr &bufferMgr, DBFile &file, enum AttrTypeEnum attrType, ModType mode, bool unique);

            ~DBMyIndex();

            string toString(string linePrefix = "") const;

            void initializeIndex();

            void find(const DBAttrType &val, DBListTID &tids);

            void insert(const DBAttrType &val, const TID &tid);

            void remove(const DBAttrType &val, const DBListTID &tid);

            bool isIndexNonUniqueAble() { return true; };

            void unfixBACBs(bool dirty);

            static int registerClass();

        private:

            static LoggerPtr logger;
            static const BlockNo rootBlockNo;
            uint calculateMaxCounter(AttrTypeEnum attrType, bool leaf);

            void insertValue(const DBAttrType &val, const TID &tid);

            void insertValue(const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);


            ReturnIntInsertValue insertValue(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void printAllBlocks();

            void insertValueFirstCall(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void insertValueFirstCall(int value, const TID &tid, BlockNo parentBlockNo);

            void insertValueFirstCall(int value, const TID &tid);

            void removeValueFirstCall(int value, const TID &tid);

            IntUndersizedAndValuePair removeValue(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void findTIDS(int val, DBListTID &tids);

            void findTIDs(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void findTIDs(BlockNo startBlockNo, int value, DBListTID &tids, BlockNo parentBlockNo);

            void findTIDsFirstCall(int value, DBListTID &tids);
        };
    }
}


#endif /*DBMYINDEX_H_*/
