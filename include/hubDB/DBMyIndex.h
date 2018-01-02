#ifndef DBSEQINDEX_H_
#define DBSEQINDEX_H_

#include <hubDB/DBIndex.h>
#include <hubDB/DBException.h>
#include <hubDB/DBTypes.h>
#include <unordered_map>

using namespace HubDB::Exception;

namespace HubDB {
    namespace Index {

        static int maxKeysPerNodeInTestMode = 3;
        static bool isInTestMode = true;
        static const BlockNo metaBlockNo = 0;

        enum BlockViewId {
            MetaBlock = 0, InnerNode = 1, LeafNode = 2
        };

        union NodeKey {
            int intValue;
            double doubleValue;
            char vchar[MAX_STR_LEN + 1];
        };

        union NodeValue {
            BlockNo blockNo;
            TID tid;
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
                block.setModified();
            }

            void setBlockToLeafBlock(DBBACB &block) {
                int *blockArtPointer = getBlockArtPointer(block);
                *blockArtPointer = LeafNode;
                block.setModified();
            }

            void setBlockToInnerBlock(DBBACB &block) {
                int *blockArtPointer = getBlockArtPointer(block);
                *blockArtPointer = InnerNode;
                block.setModified();
            }

            bool isNextFreeBlockExist(DBBACB &block) {
                return getNextFreeBlockNo(block) != 0;
            }

            BlockNo getNextFreeBlockNo(DBBACB &block) {
                return *(getNextFreeBlockNoPointer(block));
            }

            void setNextFreeBlockNoFieldTo(DBBACB &block, BlockNo nextFreeBlockNo) {
                *(getNextFreeBlockNoPointer(block)) = nextFreeBlockNo;
                block.setModified();
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
                metaBlock.setModified();
            }


            bool isBTreeEmpty(DBBACB &metaBlock) {
                return *(getTreeRootNodeBlockNoPointer(metaBlock)) == treeEmptyValue;
            }

            void setTreeToEmpty(DBBACB &metaBlock) {
                setRootNodeBlockNo(metaBlock, treeEmptyValue);
                metaBlock.setModified();
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

            BlockNo getParentNodeBlockNo(DBBACB &nodeBlock) {
                return *getParentNodeBlockNoPointer(nodeBlock);
            }

            void setParentNodeBlockNo(DBBACB &nodeBlock, BlockNo parentBlockNo) {
                *getParentNodeBlockNoPointer(nodeBlock) = parentBlockNo;
                nodeBlock.setModified();
            }

            int getNumberOfKeysExistingInNode(DBBACB &nodeBlock) {
                return *getNumberOfValuesExistingInNodePointer(nodeBlock);
            }

            int getNumberOfValuesExistingInNode(DBBACB &nodeBlock) {
                return getNumberOfKeysExistingInNode(nodeBlock) + 1;
            }

            void setNumberOfKeysExistingInNode(DBBACB &nodeBlock, uint numberOfValuesExistingInNode) {
                *getNumberOfValuesExistingInNodePointer(nodeBlock) = numberOfValuesExistingInNode;
                nodeBlock.setModified();
            }

            void increaseNumberOfKeysExistingInNode(DBBACB &nodeBlock) {
                *getNumberOfValuesExistingInNodePointer(nodeBlock) += 1;
                nodeBlock.setModified();
            }

            void decreaseNumberOfKeysExistingInNode(DBBACB &nodeBlock) {
                *getNumberOfValuesExistingInNodePointer(nodeBlock) -= 1;
                nodeBlock.setModified();
            }


            NodeKey *getKeyArraySorted(DBBACB &nodeBlock) {
                return reinterpret_cast<NodeKey *>(getNumberOfValuesExistingInNodePointer(nodeBlock) + 1);
            }

            NodeValue *getValueArrayAlignedToKeyArray(DBBACB &nodeBlock) {
                return reinterpret_cast<NodeValue *>(getKeyArraySorted(nodeBlock) + getMaxKeysPerNode());
            }

            bool isRootNode(DBBACB &nodeBlock) {
                return getParentNodeBlockNo(nodeBlock) == metaBlockNo;
            }

            int getMaxKeysPerNode(DBBACB &nodeBlock) {
                int maxKeysPerNode = getMaxKeysPerNode();

                if (isInTestMode) {
                    if (maxKeysPerNodeInTestMode > maxKeysPerNode) {
                        throw DBException("In Testmodus kann die angegebene Knotenkapazitaet nicht erreicht werden!");
                    }
                    return maxKeysPerNodeInTestMode;
                }
                return maxKeysPerNode;
            }

            void initializeRootBlock(DBBACB &nodeBlock) {
                setBlockToLeafBlock(nodeBlock);
                setNextFreeBlockNoFieldTo(nodeBlock, 0);
                setParentNodeBlockNo(nodeBlock, metaBlockNo);
                setNumberOfKeysExistingInNode(nodeBlock, 0);
            }


            void addValueToLeafNodeWithoutSplit(DBBACB nodeBlock, const DBAttrType &val, const TID &tid) {
                NodeKey *pNodeKey = getKeyArraySorted(nodeBlock);
                NodeValue *pNodeValue = getValueArrayAlignedToKeyArray(nodeBlock);

                int numberOfExistingKeys = getNumberOfKeysExistingInNode(nodeBlock);
                if (numberOfExistingKeys == 0) {
                    *pNodeKey = mapValToNodeKey(val);
                    pNodeValue->tid = tid;
                } else {
                    vector<NodeKey> keysVector;
                    vector<NodeValue> valuesVector;
                    keysVector.assign(pNodeKey, pNodeKey + numberOfExistingKeys);

                    for (int i = 0; i < numberOfExistingKeys; i++) {
                        valuesVector.push_back(*(pNodeValue + i));
                    }

                    for (int i = 0; i < keysVector.size(); i++) {
                        if (isValueLessThan(val, keysVector[i])) {
                            //vorne hinzufuegen
                            keysVector.insert(keysVector.begin() + i, mapValToNodeKey(val));
                            insertNodeValueTIDToVector(valuesVector, i, tid);
                            break;
                        } else if (i <= (keysVector.size() - 2)) {
                            if (isValueBetween(val, keysVector[i], keysVector[i + 1])) {
                                //zwischen hinzufuegen
                                keysVector.insert(keysVector.begin() + (i + 1), mapValToNodeKey(val));
                                insertNodeValueTIDToVector(valuesVector, i+1, tid);
                                break;
                            }
                        } else if(i == keysVector.size()-1) {
                            //hinten hinzufuegen
                            keysVector.insert(keysVector.end(), mapValToNodeKey(val));
                            insertNodeValueTIDToVector(valuesVector, static_cast<int>(valuesVector.size()), tid);
                            break;
                        }
                    }

                    memcpy(pNodeKey, keysVector.data(), sizeof(NodeKey) * keysVector.size());
                    memcpy(pNodeValue, valuesVector.data(), sizeof(NodeValue) * valuesVector.size());
                }
                increaseNumberOfKeysExistingInNode(nodeBlock);
                nodeBlock.setModified();
            }

            NodeValuesAsVectors getOverflowedNode(DBBACB nodeBlock, const DBAttrType &val, const TID &tid) {
                NodeKey *pNodeKey = getKeyArraySorted(nodeBlock);
                NodeValue *pNodeValue = getValueArrayAlignedToKeyArray(nodeBlock);

                int numberOfExistingKeys = getNumberOfKeysExistingInNode(nodeBlock);
                vector<NodeKey> keysVector;
                vector<NodeValue> valuesVector;
                keysVector.assign(pNodeKey, pNodeKey + numberOfExistingKeys);

                for (int i = 0; i < numberOfExistingKeys; i++) {
                    valuesVector.push_back(*(pNodeValue + i));
                }

                for (int i = 0; i < keysVector.size(); i++) {
                    if (isValueLessThan(val, keysVector[i])) {
                        //vorne hinzufuegen
                        keysVector.insert(keysVector.begin() + i, mapValToNodeKey(val));
                        insertNodeValueTIDToVector(valuesVector, i, tid);
                        break;
                    } else if (i <= (keysVector.size() - 2)) {
                        if (isValueBetween(val, keysVector[i], keysVector[i + 1])) {
                            //zwischen hinzufuegen
                            keysVector.insert(keysVector.begin() + (i + 1), mapValToNodeKey(val));
                            insertNodeValueTIDToVector(valuesVector, i+1, tid);
                            break;
                        }
                    } else if(i == keysVector.size()-1) {
                        //hinten hinzufuegen
                        keysVector.insert(keysVector.end(), mapValToNodeKey(val));
                        insertNodeValueTIDToVector(valuesVector, static_cast<int>(valuesVector.size()), tid);
                        break;
                    }
                }

                NodeValuesAsVectors result;
                result.keysVector = keysVector;
                result.valuesVector = valuesVector;

                return result;
            }

            void insertNodeValueTIDToVector(vector<NodeValue> &valuesVector, int positionToInsertAt, TID tid) {
                NodeValue dummy = NodeValue{};
                valuesVector.push_back(dummy);
                for (int j = positionToInsertAt; j < valuesVector.size()-1; j++) {
                    valuesVector[j + 1].tid = valuesVector[j].tid;
                }
                valuesVector.at(static_cast<unsigned long>(positionToInsertAt)).tid = tid;
            }

            NodeValue mapTidToNodeValue(const TID &tid) {
                NodeValue result{};
                result.tid.page = tid.page;
                result.tid.slot = tid.slot;
                return result;
            }

            NodeKey mapValToNodeKey(const DBAttrType &value) {
                NodeKey result{};
                AttrTypeEnum type = value.type();
                if (type == AttrTypeEnum::INT) {
                    const DBIntType &intType = dynamic_cast<const DBIntType &>(value);
                    result.intValue = intType.getVal();
                    return result;
                } else if (type == AttrTypeEnum::DOUBLE) {
                    const DBDoubleType &doubleType = dynamic_cast<const DBDoubleType &>(value);
                    result.doubleValue = doubleType.getVal();
                    return result;
                } else if (type == AttrTypeEnum::VCHAR) {
                    const DBVCharType &vcharType = dynamic_cast<const DBVCharType &>(value);
                    strcpy(result.vchar, vcharType.getVal().c_str());
                    return result;
                }
                throw DBException("AttrType: NONE wird nicht unterstuetzt!");
            }

            bool isValueLessThan(const DBAttrType &value, NodeKey &nodeEntry) {
                AttrTypeEnum type = value.type();
                if (type == AttrTypeEnum::INT) {
                    const DBIntType &intType = dynamic_cast<const DBIntType &>(value);
                    return intType.getVal() < nodeEntry.intValue;
                } else if (type == AttrTypeEnum::DOUBLE) {
                    const DBDoubleType &doubleType = dynamic_cast<const DBDoubleType &>(value);
                    return doubleType.getVal() < nodeEntry.doubleValue;
                } else if (type == AttrTypeEnum::VCHAR) {
                    const DBVCharType &vcharType = dynamic_cast<const DBVCharType &>(value);
                    return vcharType.getVal().compare(nodeEntry.vchar) < 0;
                }
                throw DBException("AttrType: NONE wird nicht unterstuetzt!");
            }

            bool isValueBetween(const DBAttrType &value, NodeKey &leftNodeEntry, NodeKey &rightNodeEntry) {
                AttrTypeEnum type = value.type();
                if (type == AttrTypeEnum::INT) {
                    const DBIntType &intType = dynamic_cast<const DBIntType &>(value);
                    return leftNodeEntry.intValue < intType.getVal() && intType.getVal() < rightNodeEntry.intValue;
                } else if (type == AttrTypeEnum::DOUBLE) {
                    const DBDoubleType &doubleType = dynamic_cast<const DBDoubleType &>(value);
                    return leftNodeEntry.doubleValue < doubleType.getVal() &&
                           doubleType.getVal() < rightNodeEntry.doubleValue;
                } else if (type == AttrTypeEnum::VCHAR) {
                    const DBVCharType &vcharType = dynamic_cast<const DBVCharType &>(value);
                    return (vcharType.getVal().compare(leftNodeEntry.vchar) > 0) &&
                           (vcharType.getVal().compare(rightNodeEntry.vchar) < 0);
                }
                throw DBException("AttrType: NONE wird nicht unterstuetzt!");
            }

            bool isValueEquals(const DBAttrType &value, NodeKey &nodeEntry) {
                AttrTypeEnum type = value.type();
                if (type == AttrTypeEnum::INT) {
                    const DBIntType &intType = dynamic_cast<const DBIntType &>(value);
                    return intType.getVal() == nodeEntry.intValue;
                } else if (type == AttrTypeEnum::DOUBLE) {
                    const DBDoubleType &doubleType = dynamic_cast<const DBDoubleType &>(value);
                    return doubleType.getVal() == nodeEntry.doubleValue;
                } else if (type == AttrTypeEnum::VCHAR) {
                    const DBVCharType &vcharType = dynamic_cast<const DBVCharType &>(value);
                    return vcharType.getVal().compare(nodeEntry.vchar) == 0;
                }
                throw DBException("AttrType: NONE wird nicht unterstuetzt!");
            }

            bool containsValue(DBBACB &leafBlock, const DBAttrType &value) {
                for (int i = 0; i < getNumberOfKeysExistingInNode(leafBlock); i++) {
                    NodeKey *pNodeKey = getKeyArraySorted(leafBlock);
                    if(isValueEquals(value, *(pNodeKey+i))) {
                        return true;
                    }
                }
                return false;
            }

            TID getTIDFor(DBBACB &leafBlock, const DBAttrType &value) {
                TID result{};
                int positionOfResult = -1;
                for (int i = 0; i < getNumberOfKeysExistingInNode(leafBlock); i++) {
                    NodeKey *pNodeKey = getKeyArraySorted(leafBlock);
                    if(isValueEquals(value, *(pNodeKey+i))) {
                        positionOfResult = i;
                        break;
                    }
                }
                if (positionOfResult == -1) {
                    throw DBException("TID ist nicht vorhanden! Pruefe zuerst, ob TID vorhanden ist!!!!");
                }

                NodeValue *pNodeValue = getValueArrayAlignedToKeyArray(leafBlock);
                result.page = (pNodeValue + positionOfResult)->tid.page;
                result.slot = (pNodeValue + positionOfResult)->tid.slot;

                return result;
            }

            vector<BlockNo> getChildrenBlockNo(DBBACB nodeBlock) {
                vector<BlockNo> result = vector<BlockNo>();
                NodeValue *pNodeValue = getValueArrayAlignedToKeyArray(nodeBlock);
                for (int i = 0; i < getNumberOfValuesExistingInNode(nodeBlock); i++) {
                    result.push_back((pNodeValue+i)->blockNo);
                }

                return result;
            }

            BlockNo getChildNodeBlockNoForValue(DBBACB &innerNode, const DBAttrType &value) {
                NodeKey *pNodeKey = getKeyArraySorted(innerNode);
                const vector<BlockNo> &childrenBlockNo = getChildrenBlockNo(innerNode);
                for (int i = 0; i < getNumberOfKeysExistingInNode(innerNode); i++) {
                    if (isValueLessThan(value, *(pNodeKey + i))) {
                        return childrenBlockNo[i];
                    }
                }
                return childrenBlockNo[childrenBlockNo.size()-1];
            }

            

        private:
            int getMaxKeysPerNode() {
                int sizeOfBlockInBytes = sizeof(char) * STD_BLOCKSIZE;

                //das wird subtrahiert: blockart + (nextfreeblockNo + parentBlockNo + numberOfValues)
                int bytesAvailableForKeysAndValues = sizeOfBlockInBytes - sizeof(int) - 3 * sizeof(BlockNo);

                //folgende Ungleichung wurde umgestellt: bytesAvailableForKeysAndValues >=
                // maxNumberOfKeys*sizeof(NodeKey) + (maxNumberOfKeys+1)*sizeof(NodeValue)
                float maxNumberOfKeys = (float) (bytesAvailableForKeysAndValues - sizeof(NodeValue))
                                        / (float) (sizeof(NodeKey) + sizeof(NodeValue));
                return (int) maxNumberOfKeys;
            }

            BlockNo *getParentNodeBlockNoPointer(DBBACB &nodeBlock) {
                return getNextFreeBlockNoPointer(nodeBlock) + 1;
            }

            uint *getNumberOfValuesExistingInNodePointer(DBBACB &nodeBlock) {
                return getParentNodeBlockNoPointer(nodeBlock) + 1;
            }

            AttrTypeEnum attrType;
        };

        struct NodeValuesAsVectors {
            vector<NodeKey> keysVector;
            vector<NodeValue> valuesVector;
        };


        class DBMyIndex : public DBIndex {
        public:
            DBMyIndex(DBBufferMgr &bufferMgr, DBFile &file, AttrTypeEnum attrType, ModType mode, bool unique);

            void initializeIndex() override;

            void find(const DBAttrType &val, DBListTID &tids) override;

            void insert(const DBAttrType &val, const TID &tid) override;

            void remove(const DBAttrType &val, const DBListTID &tid) override;

            bool isIndexNonUniqueAble() override { return false; };

            void setTestMode(int maxKeysPerNode) {
                maxKeysPerNodeInTestMode = maxKeysPerNode;
                isInTestMode = true;
            }

            static int registerClass();

            string toString(string linePrefix) const override;


        private:
            static LoggerPtr logger;


            MetaBlockView metaBlockView;
            NodeBlockView nodeBlockView;

            unordered_map<BlockNo, DBBACB &> usedBlocks;

            DBBACB getRootBlockExclusively(DBBACB dbbacb);

            bool isValueLessThan(const DBAttrType &value, NodeKey &nodeEntry);

            bool isValueBetween(const DBAttrType &value, NodeKey &leftNodeEntry, NodeKey &rightNodeEntry);

            NodeKey mapValToNodeKey(const DBAttrType &val);

            DBBACB findLeafForInsert(const DBAttrType &val, DBBACB &metaBlock);
        };
    }
}


#endif /*DBSEQINDEX_H_*/
