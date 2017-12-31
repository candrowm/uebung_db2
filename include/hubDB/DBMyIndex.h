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
                int blockArt = (*getBlockArtPointer(block));
                return blockArt == MetaBlock;
            }

            bool isLeafBlock(DBBACB &block) {
                int blockArt = (*getBlockArtPointer(block));
                return blockArt == LeafNode;
            }

            bool isInnerBlock(DBBACB &block) {
                int blockArt = (*getBlockArtPointer(block));
                return blockArt == InnerNode;
            }

            void setBlockToMetaBlock(DBBACB &block) {
                int *blockArtPointer = getBlockArtPointer(block);
                *blockArtPointer = MetaBlock;
            }

            void setBlockToLeafBlock(DBBACB &block) {
                int *blockArtPointer = getBlockArtPointer(block);
                *blockArtPointer = LeafNode;
            }

            void setBlockToInnerBlock(DBBACB &block) {
                int *blockArtPointer = getBlockArtPointer(block);
                *blockArtPointer = InnerNode;
            }

            bool isNextFreeBlockExist(DBBACB &block) {
                return getNextFreeBlockNo(block) != 0;
            }

            BlockNo getNextFreeBlockNo(DBBACB &block) {
                return *(getNextFreeBlockNoPointer(block));
            }

            void setNextFreeBlockNoFieldTo(DBBACB &block, BlockNo nextFreeBlockNo) {
                *(getNextFreeBlockNoPointer(block)) = nextFreeBlockNo;
            }

        protected:
            int *getBlockArtPointer(DBBACB &block) const { return reinterpret_cast<int *>(block.getDataPtr()); }

            BlockNo *getNextFreeBlockNoPointer(DBBACB &block) const {
                int *pointerToNextFreeBlockNoField = getBlockArtPointer(block) + 1;
                return reinterpret_cast<BlockNo *>(pointerToNextFreeBlockNoField);
            }
        };

        class MetaBlockView : public BlockView {
        public:
            BlockNo getRootNodeBlockNo(DBBACB &metaBlock) {
                return *(getTreeRootNodeBlockNoPointer(metaBlock));
            }

            void setRootNodeBlockNo(DBBACB &metaBlock, BlockNo blockNo) {
                *(getTreeRootNodeBlockNoPointer(metaBlock)) = blockNo;
            }


            bool isBTreeEmpty(DBBACB &metaBlock) {
                return *(getTreeRootNodeBlockNoPointer(metaBlock)) == treeEmptyValue;
            }

            void setTreeToEmpty(DBBACB &metaBlock) {
                setRootNodeBlockNo(metaBlock, treeEmptyValue);
            }

            void initializeIndex(DBBACB &firstBlock) {
                setBlockToMetaBlock(firstBlock);
                setNextFreeBlockNoFieldTo(firstBlock, treeEmptyValue);
                setRootNodeBlockNo(firstBlock, treeEmptyValue);
                setTreeToEmpty(firstBlock);
                firstBlock.setModified();
            }

        private:
            BlockNo *getTreeRootNodeBlockNoPointer(DBBACB &metaBlock) const {
                return getNextFreeBlockNoPointer(metaBlock) + 1;
            }

            int treeEmptyValue = 0;
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
