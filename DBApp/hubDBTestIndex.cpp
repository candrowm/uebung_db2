#include <hubDB/DBException.h>
#include <hubDB/DBServer.h>

using namespace HubDB::Server;
using namespace HubDB::Exception;

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <hubDB/DBMyIndex.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

const int metaBlockNo = 0;

void testIndexInitialization(const std::string &indexName) {
    //setup
    LoggerPtr logger(Logger::getLogger("HubDB.TestIndex.initializationtest"));

    HubDB::Index::MetaBlockView metaBlockView;

    DBBufferMgr &bufMgr = *(DBBufferMgr *) getClassForName("DBRandomBufferMgr", 1, true);
    const char *fileName = "file1.indexInitializationTest";
    try {
        bufMgr.dropFile(fileName);
    }catch (DBException e) {
        LOG4CXX_INFO(logger, "Setup hat versucht ein nicht existierendes File zu loeschen - Alles gut!!!");
    }

    bufMgr.createFile(fileName);
    DBFile &file1 = bufMgr.openFile(fileName);

    int indexIsUnique = 1;
    DBIndex &index = *(DBIndex *) getClassForName(indexName, 5, &bufMgr, &file1, AttrTypeEnum::INT, ModType::WRITE,
                                                  indexIsUnique);
    LOG4CXX_INFO(logger, "Begin testing...");
    //TEST----------------------------------------------------------------
    index.initializeIndex();
    {
        DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);
        bufMgr.unfixBlock(metaBlock);
        bufMgr.flushBlock(metaBlock);
        bufMgr.closeFile(file1);
        bufMgr.openFile(fileName);
    }

    DBBACB metaBlock = bufMgr.fixBlock(file1, metaBlockNo, DBBCBLockMode::LOCK_SHARED);

    if (!metaBlockView.isMetaBlock(metaBlock)) {
        LOG4CXX_WARN(logger, "angelegter Metablock wird nicht als Metablock angelegt");
        return;
    }
    if(metaBlockView.getNextFreeBlockNo(metaBlock) != 0) {
        LOG4CXX_WARN(logger, "NextFreeBlock nicht richtig gesetzt! Der gesetzt Wert ist: "
                             + std::to_string(metaBlockView.getNextFreeBlockNo(metaBlock)));
        return;
    }
    if(metaBlockView.getRootNodeBlockNo(metaBlock) != 0) {
        LOG4CXX_WARN(logger, "RootNodeBlockNo ist nicht richtig gesetzt! Der gesetzt Wert ist: "
                             + std::to_string(metaBlockView.getRootNodeBlockNo(metaBlock)));
        return;
    }
    if(!metaBlockView.isBTreeEmpty(metaBlock)) {
        LOG4CXX_WARN(logger, "Baum ist nicht leer, der sollte das aber sein!");
        return;
    }

    bufMgr.unfixBlock(metaBlock);

    //CLEANUP-------------------------------------------------------------
    bufMgr.closeFile(file1);
    bufMgr.dropFile(fileName);
    delete &index;
    delete &bufMgr;
    LOG4CXX_INFO(logger, "Test finished succesfull");
}

int main(int argc, char *argv[]) {
    int rc = EXIT_SUCCESS;
    try {
        File f("../log4c++.hubDB.properties.testIndex");
        PropertyConfigurator::configure(f);

        testIndexInitialization("DBMyIndex");
    } catch (Exception &) {
        rc = EXIT_FAILURE;
    }

    return rc;
}
