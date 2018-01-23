#ifndef DBMYINDEX_H_
#define DBMYINDEX_H_

#include <hubDB/DBIndex.h>
#include <cmath>

namespace HubDB {
    namespace Index {
        
        struct ReturnInsertValue {
            BlockNo blockNoLeft;
            DBIntType int_value;
            DBDoubleType double_value;
            DBVCharType vchar_value;
            BlockNo blockNoRight;

            ReturnInsertValue(BlockNo leftBlockNo, const DBAttrType &val, BlockNo rightBlockNo, AttrTypeEnum attrType):
                    int_value(DBIntType(0)), double_value(DBDoubleType(0.0)), vchar_value(DBVCharType("0")){
                blockNoLeft = leftBlockNo;
                blockNoRight = rightBlockNo;
                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int_value = DBIntType(value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double_value = DBDoubleType(value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        break;
                    }
                }
            }

            DBAttrType& getValue(AttrTypeEnum attrType){
                switch(attrType) {
                    case INT: {
                        return int_value;
                        break;
                    }
                    case DOUBLE:{
                        return double_value;
                        break;
                    }
                    case VCHAR:{
                        return vchar_value;
                        break;
                    }
                }
            }
        };

        struct ValueAndTIDPair {
            DBIntType int_value = DBIntType(0);
            DBDoubleType double_value = DBDoubleType(0.0);
            DBVCharType vchar_value = DBVCharType("0");
            TID tid;
            bool successful;
            DBIntType int_neighbourValue = DBIntType(0);
            DBDoubleType double_neighbourValue = DBDoubleType(0.0);
            DBVCharType vchar_neighbourValue = DBVCharType("0");
            BlockNo blockNo;


            ValueAndTIDPair(const DBAttrType &val, TID t, AttrTypeEnum attrType){
                tid = t;
                successful = true;

                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int_value = DBIntType(value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double_value = DBDoubleType(value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        break;
                    }
                }
            }

            ValueAndTIDPair(const DBAttrType &val, TID t, bool success, AttrTypeEnum attrType){
                tid = t;
                successful = success;
                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int_value = DBIntType(value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double_value = DBDoubleType(value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        break;
                    }
                }
            }

            ValueAndTIDPair(const DBAttrType &val, TID t, bool success, const DBAttrType &neighbourVal, AttrTypeEnum attrType){
                tid = t;
                successful = success;
                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int n_value = ((DBIntType *) &neighbourVal)->getVal();
                        int_value = DBIntType(value);
                        int_neighbourValue = DBIntType(n_value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double n_value = ((DBDoubleType *) &neighbourVal)->getVal();
                        double_value = DBDoubleType(value);
                        double_neighbourValue = DBDoubleType(n_value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        string n_value = ((DBVCharType *) &neighbourVal)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        vchar_neighbourValue = DBVCharType(n_value.c_str());
                        break;
                    }
                }
            }

            ValueAndTIDPair(const DBAttrType &val, BlockNo blockN, bool success, AttrTypeEnum attrType){
                blockNo = blockN;
                successful = success;
                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int_value = DBIntType(value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double_value = DBDoubleType(value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        break;
                    }
                }
            }

            ValueAndTIDPair(const DBAttrType &val, BlockNo blockN,
                               bool success,  const DBAttrType &neighbourVal, AttrTypeEnum attrType){
                blockNo = blockN;
                successful = success;
                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int n_value = ((DBIntType *) &neighbourVal)->getVal();
                        int_value = DBIntType(value);
                        int_neighbourValue = DBIntType(n_value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double n_value = ((DBDoubleType *) &neighbourVal)->getVal();
                        double_value = DBDoubleType(value);
                        double_neighbourValue = DBDoubleType(n_value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        string n_value = ((DBVCharType *) &val)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        vchar_neighbourValue = DBVCharType(n_value.c_str());
                        break;
                    }
                }
            }

            DBAttrType& getValue(AttrTypeEnum attrType){
                switch(attrType) {
                    case INT: {
                        return int_value;
                        break;
                    }
                    case DOUBLE:{
                        return double_value;
                        break;
                    }
                    case VCHAR:{
                        return vchar_value;
                        break;
                    }
                }
            }

            DBAttrType& getNeighbourValue(AttrTypeEnum attrType){
                switch(attrType) {
                    case INT: {
                        return int_neighbourValue;
                        break;
                    }
                    case DOUBLE:{
                        return double_neighbourValue;
                        break;
                    }
                    case VCHAR:{
                        return vchar_neighbourValue;
                        break;
                    }
                }
            }
        };

        struct UndersizedAndValuePair {
            bool undersized;
            DBIntType int_value = DBIntType(0);
            DBDoubleType double_value = DBDoubleType(0.0);
            DBVCharType vchar_value = DBVCharType("0");

            UndersizedAndValuePair(const DBAttrType &val, bool undersize, AttrTypeEnum attrType){
                undersized = undersize;
                switch(attrType) {
                    case INT: {
                        int value = ((DBIntType *) &val)->getVal();
                        int_value = DBIntType(value);
                        break;
                    }
                    case DOUBLE: {
                        double value = ((DBDoubleType *) &val)->getVal();
                        double_value = DBDoubleType(value);
                        break;
                    }
                    case VCHAR: {
                        string value = ((DBVCharType *) &val)->getVal();
                        vchar_value = DBVCharType(value.c_str());
                        break;
                    }
                }
            }

            DBAttrType& getValue(AttrTypeEnum attrType){
                switch(attrType) {
                    case INT: {
                        return int_value;
                        break;
                    }
                    case DOUBLE:{
                        return double_value;
                        break;
                    }
                    case VCHAR:{
                        return vchar_value;
                        break;
                    }
                }
            }
        };

        class TreeBlock {
        public:
            bool leaf;
            //bool intBlock;
            //bool doubleBlock;
            //bool varCharBlock;
            BlockNo blockNo;
            BlockNo nextFreeBlockNo;
            int maxValueCounter;
            int currentValueCounter;
        public:
            TreeBlock(BlockNo blockNo) {
                this->leaf = false;
                //this->intBlock = false;
                //this->doubleBlock = false;
                //this->varCharBlock = false;
                this->blockNo = blockNo;
                this->currentValueCounter = 0;
                this->nextFreeBlockNo = BlockNo(0);
            }

            TreeBlock() {}

            uint calculateMaxCounter(AttrTypeEnum attrType, bool leaf);
            //virtual void copyBlockToDBBACB(DBBACB d) = 0;

            //virtual void printAllBlocks();
        };

        /*      START BLOCK      */
        class TreeStartBlock{
        public:
            BlockNo blockNo;
            BlockNo rootBlockNo;
            BlockNo nextFreeBlock;
            TreeStartBlock(){
                this->blockNo = BlockNo(0);
                this->nextFreeBlock = BlockNo(0);
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
            virtual bool insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool fromLeft) = 0;
            virtual DBAttrType * getValue(int index) = 0;
            virtual void setValue(int index, const DBAttrType &val) = 0;
            virtual BlockNo getBlockNo(int index) = 0;
            virtual void setBlockNo(int index, BlockNo blockNo) = 0;
            virtual int compare(int index, const DBAttrType &val) = 0 ;
            virtual TreeInnerBlock * splitBlock(BlockNo blockNo) = 0;
            virtual ValueAndTIDPair removeSmallestBlockNo() = 0;
            virtual ValueAndTIDPair removeBiggestBlockNo() = 0;
    
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

            virtual UndersizedAndValuePair removeTID(const DBAttrType &val, TID tid) = 0;

            virtual ValueAndTIDPair removeSmallestTID() = 0;
            virtual ValueAndTIDPair removeBiggestTID() = 0;
            
            
        };

        /*      INT INNER BLOCK      */
        class TreeIntInnerBlock : public TreeInnerBlock {
        public:
            int *values;
            BlockNo *blockNos;


        public:
            TreeIntInnerBlock(BlockNo blockNo) : TreeInnerBlock(blockNo) {
                //this->intBlock = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::INT, false);
                this->values = new int[maxValueCounter];
                this->blockNos = new BlockNo[maxValueCounter+1];
                this->nextFreeBlockNo = BlockNo(0);
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    blockNos[i] = i;
                }
            };
            ~TreeIntInnerBlock(){
                delete[] values;
                delete[] blockNos;
            }
            
            int compare(int index, const DBAttrType &val);
            
            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

            //void insertValue(int value);

            int removeBlockNo(BlockNo blockNo);

            void printAllValues();

            bool insertBlockNo(int value, BlockNo blockNo);

            //void insertBlockNo(BlockNo blockNoLeft, int value, BlockNo BlockNoRight);
            bool insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool empty);
            bool insertBlockNo(BlockNo blockNoLeft, const DBAttrType &val, BlockNo blockNoRight, bool root);

            ValueAndTIDPair removeSmallestBlockNo();

            ValueAndTIDPair removeBiggestBlockNo();
            
            DBAttrType * getValue(int index);
            void setValue(int index, const DBAttrType &val);
            BlockNo getBlockNo(int index);
            void setBlockNo(int index, BlockNo blockNo);
            
            TreeInnerBlock * splitBlock(BlockNo blockNo);
        };

        
        /*      INT LEAF BLOCK      */
        class TreeIntLeafBlock : public TreeLeafBlock {
        public:
            int *values;
            TID *tids;

        public:
            TreeIntLeafBlock(BlockNo blockNo) : TreeLeafBlock(blockNo) {
                //this->intBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::INT, true);
                this->nextFreeBlockNo = BlockNo(0);
                this->values = new int[maxValueCounter];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = 0;
                    tids[i] = TID();
                }
            };
            ~TreeIntLeafBlock(){
                delete[] values;
                delete[] tids;
            }
            
            int compare(int index, const DBAttrType &val);

            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();

            //void insertTID(TID tid);

            bool insertTID(const DBAttrType &val, TID tid);

            void printAllValues();


            UndersizedAndValuePair removeTID(const DBAttrType &val, TID tid);

            //ValueAndTIDPair removeSmallestTID(int value, TID tid);

            //ValueAndTIDPair removeBiggestTID(int value, TID tid);

            ValueAndTIDPair removeBiggestTID();

            ValueAndTIDPair removeSmallestTID();
            
            DBAttrType * getValue(int index);
            void setValue(int index, const DBAttrType &val);
            TID getTID(int index);
            void setTID(int index, TID tid);
            TreeLeafBlock *splitBlock(BlockNo blockNo);
        };

        
        /*      DOUBLE INNER BLOCK      */
        
        class TreeDoubleInnerBlock : public TreeInnerBlock {
        public:
            double *values;
            BlockNo *blockNos;

        public:
            TreeDoubleInnerBlock(BlockNo blockNo) : TreeInnerBlock(blockNo) {
                //this->doubleBlock = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::DOUBLE, false);
                this->nextFreeBlockNo = BlockNo(0);
                this->values = new double[maxValueCounter];
                this->blockNos = new BlockNo[maxValueCounter+1];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    blockNos[i] = i;
                }
            };
            ~TreeDoubleInnerBlock(){
                delete[] values;
                delete[] blockNos;
            }
    
    
            int compare(int index, const DBAttrType &val);
            DBAttrType * getValue(int index);
            void setValue(int index, const DBAttrType &val);
    
    
            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();
    
            //void insertValue(int value);
    
            int removeBlockNo(BlockNo blockNo);
    
            void printAllValues();
    
            //bool insertBlockNo(double *value, BlockNo blockNo);
            bool insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool fromLeft);
            bool insertBlockNo(BlockNo blockNoLeft, const DBAttrType &val, BlockNo blockNoRight, bool root);
    
            ValueAndTIDPair removeSmallestBlockNo();
            ValueAndTIDPair removeBiggestBlockNo();
    
    
            BlockNo getBlockNo(int index);
            void setBlockNo(int index, BlockNo blockNo);
    
            TreeInnerBlock * splitBlock(BlockNo blockNo);
            
        };

        class TreeDoubleLeafBlock : public TreeLeafBlock {
        public:
            double *values;
            TID *tids;

        public:
            TreeDoubleLeafBlock(BlockNo blockNo) : TreeLeafBlock(blockNo) {
                //this->doubleBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::DOUBLE, true);
                this->nextFreeBlockNo = BlockNo(0);
                this->values = new double[maxValueCounter];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = i;
                    tids[i] = TID();
                }
            };
            ~TreeDoubleLeafBlock(){
                delete[] values;
                delete[] tids;
            }
    
            DBAttrType * getValue(int index);
            void setValue(int index, const DBAttrType &val);
            TID getTID(int index);
            void setTID(int index, TID tid);
    
    
            int compare(int index, const DBAttrType &val);
    
            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
    
            void updatePointers();
    
            bool insertTID(const DBAttrType &val, TID tid);
    
            TreeLeafBlock * splitBlock(BlockNo blockNo);
    
            void printAllValues();
    
            UndersizedAndValuePair removeTID(const DBAttrType &val, TID tid);
            ValueAndTIDPair removeBiggestTID();
            ValueAndTIDPair removeSmallestTID();
        };
        


        /*      VARCHAR INNER BLOCK      */

        
        class TreeVarCharInnerBlock : public TreeInnerBlock {
        public:
            char *values;
            BlockNo *blockNos;

        public:
            TreeVarCharInnerBlock(BlockNo blockNo) : TreeInnerBlock(blockNo) {
                //this->varCharBlock= true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::VCHAR, false);
                this->nextFreeBlockNo = BlockNo(0);
                this->values = new char[maxValueCounter*(MAX_STR_LEN+1)];
                this->blockNos = new BlockNo[maxValueCounter+1];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = 'a';
                    blockNos[i] = i;
                }
            };
            ~TreeVarCharInnerBlock(){
                delete[] values;
                delete[] blockNos;
            }
            
            int compare(int index, const DBAttrType &val);
            DBAttrType *getValue(int index);
            void setValue(int index, const DBAttrType &val);
            
            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            void updatePointers();
            
            
            int removeBlockNo(BlockNo blockNo);  
            void printAllValues();
            
            bool insertBlockNo(const DBAttrType &val, BlockNo blockNo, bool fromLeft);
            bool insertBlockNo(BlockNo blockNoLeft, const DBAttrType &val, BlockNo blockNoRight, bool root);
            
            ValueAndTIDPair removeSmallestBlockNo();
            ValueAndTIDPair removeBiggestBlockNo();

            BlockNo getBlockNo(int index);
            void setBlockNo(int index, BlockNo blockNo);
            
            TreeInnerBlock * splitBlock(BlockNo blockNo);
            
        };

        class TreeVarCharLeafBlock : public TreeLeafBlock {
        public:
            char *values;
            TID *tids;

        public:
            TreeVarCharLeafBlock(BlockNo blockNo) : TreeLeafBlock(blockNo) {
                //this->varCharBlock = true;
                this->leaf = true;
                this->maxValueCounter = TreeBlock::calculateMaxCounter(AttrTypeEnum::VCHAR, true);
                this->nextFreeBlockNo = BlockNo(0);
                this->values = new char[maxValueCounter * (MAX_STR_LEN+1)];
                this->tids = new TID[maxValueCounter];
                for (int i = 0; i < maxValueCounter; i++) {
                    values[i] = 'a';
                    tids[i] = TID();
                }
            };
            ~TreeVarCharLeafBlock(){
                delete[] values;
                delete[] tids;
            }
    
            DBAttrType *getValue(int index);
            void setValue(int index, const DBAttrType &val);
            TID getTID(int index);
            void setTID(int index, TID tid);
            
            
            int compare(int index, const DBAttrType &val);
            
            void copyDBBACBToBlock(DBBACB d);
            void copyBlockToDBBACB(DBBACB d);
            
            void updatePointers();
            
            bool insertTID(const DBAttrType &val, TID tid);
    
            TreeLeafBlock * splitBlock(BlockNo blockNo);
    
            void printAllValues();
            
            UndersizedAndValuePair removeTID(const DBAttrType &val, TID tid);
            ValueAndTIDPair removeBiggestTID();
            ValueAndTIDPair removeSmallestTID();            
                
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

            bool isIndexNonUniqueAble() { return false; };

            void unfixBACBs(bool dirty);
            
            TreeInnerBlock * getInnerBlockFromDBBACB(DBBACB d);
            TreeLeafBlock * getLeafBlockFromDBBACB(DBBACB d);
            //ReturnInsertValue * getReturnInsertValue(BlockNo leftBlockNo, DBAttrType *val, BlockNo rightBlockNo);


            static int registerClass();
            
            TreeInnerBlock * createNewRoot(BlockNo blockNo);
            TreeLeafBlock * createInitialRoot(BlockNo blockNo);
            void printAllBlocks();
            
        private:

            static LoggerPtr logger;
            static const BlockNo rootBlockNo;
            uint calculateMaxCounter(AttrTypeEnum attrType, bool leaf);

            void insertValue(const DBAttrType &val, const TID &tid);

            void insertValue(const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);


            ReturnInsertValue insertValue(BlockNo startBlockNo, const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);

            //void printAllBlocks();

            void insertValueFirstCall(BlockNo startBlockNo, int value, const TID &tid, BlockNo parentBlockNo);

            void insertValueFirstCall(int value, const TID &tid, BlockNo parentBlockNo);

            void insertValueFirstCall(const DBAttrType &value, const TID &tid);

            void removeValueFirstCall(const DBAttrType &value, const TID &tid);
            //void removeValueFirstCall(const DBAttrType &value, const TID &tid);

            UndersizedAndValuePair removeValue(BlockNo startBlockNo, const DBAttrType &value, const TID &tid, BlockNo parentBlockNo);

            void findTIDS(const DBAttrType &val, DBListTID &tids);

            void findTIDs(BlockNo startBlockNo, const DBAttrType &val, const TID &tid, BlockNo parentBlockNo);

            void findTIDs(BlockNo startBlockNo, const DBAttrType &val, DBListTID &tids, BlockNo parentBlockNo);

            void findTIDsFirstCall(const DBAttrType &val, DBListTID &tids);

            DBBACB fixNewBlock();

            void insertFreeBlock(BlockNo blockNo);

            BlockNo findFreeBlock();

            void printFreeBlock();

            void printFreeBlocks();

            BlockNo getFreeBlock();
        };
    }
}


#endif /*DBMYINDEX_H_*/
