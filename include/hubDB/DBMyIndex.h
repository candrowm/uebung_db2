#ifndef DBMYINDEX_H_
#define DBMYINDEX_H_

#include <hubDB/DBIndex.h>
#include <cmath>

namespace HubDB {
    namespace Index {
        
        struct ReturnInsertValue {
            BlockNo blockNoLeft;
            DBIntType value;
            BlockNo blockNoRight;

            ReturnInsertValue(BlockNo leftBlockNo, DBIntType val, BlockNo rightBlockNo) : value(val) {
                blockNoLeft = leftBlockNo;
                //value = val;
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

            //virtual void printAllBlocks();
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
        
        /*      INNER BLOCK     */
        class TreeInnerBlock : public TreeBlock{
        public:
            TreeInnerBlock(BlockNo blockNo) : TreeBlock(blockNo){}
            virtual ~TreeInnerBlock();
            virtual void copyBlockToDBBACB(DBBACB d) = 0;
            virtual void updatePointers() = 0;
            virtual void printAllValues() = 0;
            //virtual bool insertBlockNo(const DBAttrType &val, BlockNo blockNo);
            virtual bool insertBlockNo(BlockNo blockNoLeft, const DBAttrType &val , BlockNo BlockNoRight, bool root) = 0;
            virtual DBAttrType * getValue(int index) = 0;
            virtual void setValue(int index, const DBAttrType &val) = 0;
            virtual BlockNo getBlockNo(int index) = 0;
            virtual void setBlockNo(int index, BlockNo blockNo) = 0;
            virtual int compare(int index, const DBAttrType &val) = 0 ;
            virtual TreeInnerBlock * splitBlock(BlockNo blockNo) = 0;
    
        };

        /*      INT BLOCK      */
        class TreeIntInnerBlock : public TreeInnerBlock {
        public:
            int *values;
            BlockNo *blockNos;


        public:
            TreeIntInnerBlock(BlockNo blockNo) : TreeInnerBlock(blockNo) {
                this->intBlock = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::INT, false);
                this->values = new int[maxValueCounter];
                this->blockNos = new BlockNo[maxValueCounter+1];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    blockNos[i] = i;
                }
            };
            
            int compare(int index, const DBAttrType &val);
            
            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

            void insertValue(int value);

            int removeBlockNo(BlockNo blockNo);

            void printAllValues();

            bool insertBlockNo(int value, BlockNo blockNo);

            void insertBlockNo(BlockNo blockNoLeft, int value, BlockNo BlockNoRight);
            bool insertBlockNo(BlockNo blockNoLeft, const DBAttrType &val, BlockNo blockNoRight, bool root);

            IntValueAndTIDPair removeSmallestBlockNo();

            IntValueAndTIDPair removeBiggestBlockNo();
            
            DBAttrType * getValue(int index);
            void setValue(int index, const DBAttrType &val);
            BlockNo getBlockNo(int index);
            void setBlockNo(int index, BlockNo blockNo);
            
            TreeInnerBlock * splitBlock(BlockNo blockNo);
        };
        
        /*      LEAF BLOCK      */
        class TreeLeafBlock : public TreeBlock{
        public:
            TreeLeafBlock(BlockNo blockNo) : TreeBlock(blockNo) {}
            virtual ~TreeLeafBlock();
            
            virtual void copyBlockToDBBACB(DBBACB d) = 0;
            virtual void updatePointers() = 0;
            virtual void printAllValues() = 0;
    
            //virtual void insertTID(TID tid) = 0;
            virtual bool insertTID(const DBAttrType &val, TID tid) = 0;
            
            virtual DBAttrType * getValue(int index) = 0;
            virtual void setValue(int index, const DBAttrType &val) = 0;
            virtual TID getTID(int index) = 0;
            virtual void setTID(int index, TID tid) = 0;
            virtual int compare(int index, const DBAttrType &val) = 0;
            virtual TreeLeafBlock * splitBlock(BlockNo blockno) = 0;
            
            
        };
        
        class TreeIntLeafBlock : public TreeLeafBlock {
        public:
            int *values;
            TID *tids;

        public:
            TreeIntLeafBlock(BlockNo blockNo) : TreeLeafBlock(blockNo) {
                this->intBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::INT, true);
                this->values = new int[maxValueCounter];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = 7;
                    tids[i] = TID();
                }
            };
            
            int compare(int index, const DBAttrType &val);

            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

            //void insertTID(TID tid);

            bool insertTID(const DBAttrType &val, TID tid);

            void printAllValues();


            IntUndersizedAndValuePair removeTID(int value, TID tid);

            IntValueAndTIDPair removeSmallestTID(int value, TID tid);

            IntValueAndTIDPair removeBiggestTID(int value, TID tid);

            IntValueAndTIDPair removeBiggestTID();

            IntValueAndTIDPair removeSmallestTID();
            
            DBAttrType * getValue(int index);
            void setValue(int index, const DBAttrType &val);
            TID getTID(int index);
            void setTID(int index, TID tid);
            TreeLeafBlock * splitBlock(BlockNo blockNo);
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
            
            TreeInnerBlock * getInnerBlockFromDBBACB(DBBACB d);
            TreeInnerBlock * splitInnerBlock(TreeInnerBlock *treeInnerBlock, BlockNo blockNo);
            TreeLeafBlock * getLeafBlockFromDBBACB(DBBACB d);
            TreeLeafBlock * splitLeafBlock(TreeLeafBlock *treeLeafBlock, BlockNo blockNo);

            static int registerClass();
            
            TreeInnerBlock * createNewRoot(BlockNo blockNo);

        private:

            static LoggerPtr logger;
            static const BlockNo rootBlockNo;
            uint calculateMaxCounter(AttrTypeEnum attrType, bool leaf);

            void insertValue(const DBAttrType &val, const TID &tid);

            void insertValue(const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);


            ReturnInsertValue insertValue(BlockNo startBlockNo, const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);

            void printAllBlocks();

            void insertValueFirstCall(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void insertValueFirstCall(int value, const TID &tid, BlockNo parentBlockNo);

            void insertValueFirstCall(int value, const TID &tid);

            void removeValueFirstCall(int value, const TID &tid);

            IntUndersizedAndValuePair removeValue(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void findTIDS(const DBAttrType &val, DBListTID &tids);

            void findTIDs(BlockNo startBlockNo, const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);

            void findTIDs(BlockNo startBlockNo, const DBAttrType &val, DBListTID &tids, BlockNo parentBlockNo);

            void findTIDsFirstCall(const DBAttrType &val, DBListTID &tids);
        };
    }
}


#endif /*DBMYINDEX_H_*/
