#ifndef DBSEQINDEX_H_
#define DBSEQINDEX_H_

#include <hubDB/DBIndex.h>
#include <hubDB/DBException.h>
#include <unordered_map>

using namespace HubDB::Exception;

namespace HubDB {
    namespace Index {

        enum BlockViewId {
            MetaBlock = 0, InnerNode = 1, LeafNode = 2
        };

        class BlockView {
        public:
            bool isMetaBlock(DBBACB &block) {
                int blockArt = (*reinterpret_cast<int *>(block.getDataPtr()));
                return blockArt == MetaBlock;
            }

            bool isLeafBlock(DBBACB &block) {
                int blockArt = (*reinterpret_cast<int *>(block.getDataPtr()));
                return blockArt == LeafNode;
            }

            bool isInnerBlock(DBBACB &block) {
                int blockArt = (*reinterpret_cast<int *>(block.getDataPtr()));
                return blockArt == InnerNode;
            }

            void setBlockToMetaBlock(DBBACB &block) {
                int *blockArtPointer = reinterpret_cast<int *>(block.getDataPtr());
                *blockArtPointer = MetaBlock;
            }

            //Result == 0 bedeutet, dass es keinen weiteren freien Block gibt
            BlockNo getNextFreeBlockNo(DBBACB &block) {
                int *pointerToNextFreeBlockNoField = reinterpret_cast<int *>(block.getDataPtr()) + 1;
                return *(reinterpret_cast<BlockNo *>(pointerToNextFreeBlockNoField));
            }

            void setNextFreeBlockNoFieldTo(DBBACB &block, BlockNo nextFreeBlockNo) {
                int *nextFreeBlockNoFieldPointer = reinterpret_cast<int *>(block.getDataPtr()) + 1;
                *(reinterpret_cast<BlockNo *>(nextFreeBlockNoFieldPointer)) = nextFreeBlockNo;
            }
        };

        class MetaBlockView : public BlockView {
        public:
            BlockNo getRootNodeBlockNo(DBBACB &metaBlock) {
                int *pointerToRootNodeIndicatorField = reinterpret_cast<int *>(metaBlock.getDataPtr()) + 2;
                return *(reinterpret_cast<BlockNo *>(pointerToRootNodeIndicatorField));
            }

            bool isBTreeEmpty(DBBACB &metaBlock) {
                int *pointerToTreeEmptinessField = reinterpret_cast<int *>(metaBlock.getDataPtr()) + 3;
                return *pointerToTreeEmptinessField == treeIsEmptyValue;
            }

            void setRootNodeBlockNo(DBBACB &metaBlock, BlockNo blockNo) {
                int *pointerToRootNodeIndicatorField = reinterpret_cast<int *>(metaBlock.getDataPtr()) + 2;
                *(reinterpret_cast<BlockNo *>(pointerToRootNodeIndicatorField)) = blockNo;
            }

            void setTreeEmptiness(DBBACB &metaBlock, bool treeIsEmpty) {
                int *pointerToTreeEmptinessField = reinterpret_cast<int *>(metaBlock.getDataPtr()) + 3;
                if (treeIsEmpty) {
                    *pointerToTreeEmptinessField = treeIsEmptyValue;
                } else {
                    *pointerToTreeEmptinessField = 0;
                }
            }

            void initializeIndex(DBBACB &firstBlock) {
                setBlockToMetaBlock(firstBlock);
                setNextFreeBlockNoFieldTo(firstBlock, 0);
                setRootNodeBlockNo(firstBlock, 0);
                setTreeEmptiness(firstBlock, true);
                firstBlock.setModified();
            }

        private:
            int treeIsEmptyValue = 1;
        };

        class NodeBlockView : public BlockView {
        public:
            NodeBlockView(AttrTypeEnum attrType) : attrType(attrType) {};
        private:
            AttrTypeEnum attrType;
        };


        class DBMyIndex : public DBIndex {
        public:
            DBMyIndex(DBBufferMgr &bufferMgr, DBFile &file, AttrTypeEnum attrType, ModType mode, bool unique);

            void initializeIndex() override;

            void find(const DBAttrType &val, DBListTID &tids) override;

            void insert(const DBAttrType &val, const TID &tid) override;

            void remove(const DBAttrType &val, const DBListTID &tid) override;

            bool isIndexNonUniqueAble() override { return false; };

            static int registerClass();

            string toString(string linePrefix) const override;

        private:
            static LoggerPtr logger;
            static const BlockNo rootBlockNo = 0;

            MetaBlockView metaBlockView;
            NodeBlockView nodeBlockView;

            unordered_map<BlockNo, DBBACB &> usedBlocks;
        };
    }
}


#endif /*DBSEQINDEX_H_*/
