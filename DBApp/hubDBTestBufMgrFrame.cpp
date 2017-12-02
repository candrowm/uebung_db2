//
// Created by Jörg Bachmann on 28.11.17.
//

#include <hubDB/DBException.h>
#include <hubDB/DBServer.h>
using namespace HubDB::Server;
using namespace HubDB::Exception;

#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

void testBufferManager( const std::string& bufferMgrName ) {
    LoggerPtr logger(Logger::getLogger("HubDB.TestBufferManager" ) );

    DBBufferMgr& bufMgr = *( DBBufferMgr* )getClassForName( bufferMgrName, 1, true );

    try {
        bufMgr.dropFile( "file1.bufferTest" );
        bufMgr.dropFile( "file2.bufferTest" );
    }
    catch ( DBException e ) {}

    LOG4CXX_INFO( logger, "Begin testing..." );

    bufMgr.createFile( "file1.bufferTest" );
    bufMgr.createFile( "file2.bufferTest" );
    DBFile& file1 = bufMgr.openFile( "file1.bufferTest" );
    DBFile& file2 = bufMgr.openFile( "file2.bufferTest" );

    try {
        DBBACB block1_1 = bufMgr.fixNewBlock( file1 );
        DBBACB block1_2 = bufMgr.fixNewBlock( file1 );
        DBBACB block2_1 = bufMgr.fixNewBlock( file2 );

        block1_2.setDirty();

        strcpy( block2_1.getDataPtr(), "Hallo Welt!" );
        // now find the file and have a glimpse ;)
        block2_1.setModified();

        bufMgr.unfixBlock( block1_1 );
        bufMgr.unfixBlock( block1_2 );
        bufMgr.unfixBlock( block2_1 );
    }
    catch( DBException e ) {
        LOG4CXX_INFO( logger, "Wasn't my fault :(" );
    }

    LOG4CXX_INFO( logger, "----- finished... ------------------------------" )

    bufMgr.closeFile( file1 );
    bufMgr.closeFile( file2 );

    // if you had your glimpse, uncomment those lines...
    //bufMgr.dropFile( "file1.bufferTest" );
    //bufMgr.dropFile( "file2.bufferTest" );

    // clean up
    delete &bufMgr;
}

int main( int argc, char *argv[] ) {
    int rc = EXIT_SUCCESS;
    try{
        File f( "log4c++.hubDB.properties.testBufMgr" );
        PropertyConfigurator::configure( f );

        testBufferManager( "DBRandomBufferMgr" );
    } catch( Exception & ) {
        rc = EXIT_FAILURE;
    }

    return rc;
}
