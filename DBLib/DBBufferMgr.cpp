#include <hubDB/DBBufferMgr.h>
#include <hubDB/DBException.h>
#include <hubDB/DBMonitorMgr.h>

using namespace HubDB::Manager;
using namespace HubDB::Exception;

LoggerPtr DBBufferMgr::logger(Logger::getLogger("HubDB.Buffer.DBBufferMgr"));

DBBufferMgr::DBBufferMgr(bool threading,int blocks):
	DBManager(threading),
	fileMgr(threading),
	maxBlockCnt(blocks)
{
   	if(logger!=NULL)
       	LOG4CXX_INFO(logger,"DBBufferMgr()");
	if(doThreading()){
    	int err=0;
    	if((err=pthread_cond_init(&cond,NULL))!=0){
        	if(logger!=NULL)
            	LOG4CXX_ERROR(logger,"pthread_cond_init()="+TO_STR(err));
        	throw DBMutexException(DBMutex::getMsg(err));
    	}
	}
   	if(logger!=NULL)
	   	LOG4CXX_DEBUG(logger,"this:\n"+toString("\t"));
}

DBBufferMgr::~DBBufferMgr()
{
   	LOG4CXX_INFO(logger,"~DBBufferMgr()");
   	LOG4CXX_DEBUG(logger,"this:\n"+toString("\t"));
	if(doThreading()){
    	int err=0;
    	if((err=pthread_cond_destroy(&cond))!=0){
        	if(logger!=NULL)
	            LOG4CXX_ERROR(logger,"pthread_cond_destroy()="+TO_STR(err));
    	    throw DBMutexException(DBMutex::getMsg(err));
    	}
	}
}

string DBBufferMgr::toString(string linePrefix) const
{
	stringstream ss;
	lock();
	ss << linePrefix << "[DBBufferMgr]"<<endl;
	ss << DBManager::toString(linePrefix+"\t");
    ss << linePrefix << "maxBlockCnt: " << maxBlockCnt << endl;
	ss << linePrefix << "fileMgr:\n" << fileMgr.toString(linePrefix+"\t");
	ss << linePrefix << "-------------" << endl;
	unlock();
	return ss.str();
}

void DBBufferMgr::dropFile(const string & fileName)
{
    LOG4CXX_INFO(logger,"dropFile()");
    LOG4CXX_DEBUG(logger,"fileName:"+fileName);
	lock();
    try{
        DBFile * file = fileMgr.getOpenFile(fileName);
        if(file!=NULL){
		  closeAllOpenBlocks(*file);
		  fileMgr.closeFile(*file);
        }
		fileMgr.dropFile(fileName);
	}catch(DBException e){
        unlock();
        throw e;
	}
	unlock();
}

void DBBufferMgr::closeFile(DBFile & file)
{
    LOG4CXX_INFO(logger,"dropFile()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
	lock();
    try{
	   if(isBlockOfFileOpen(file)==false){
		  fileMgr.closeFile(file);
	   }
    }catch(DBException e){
        unlock();
        throw e;
    }
	unlock();
}

void DBBufferMgr::flushBlock(DBBACB & bacb)
{
    LOG4CXX_INFO(logger,"flushBlock()");
    LOG4CXX_DEBUG(logger,"bacb:\n" + bacb.toString("\t"));
	MONITOR_FUNC;
	if(bacb.getDirty() == true)
		throw DBBufferMgrException("try to flush dirty page");
	flushBCBBlock(bacb.getBCB());
}

void DBBufferMgr::flushBCBBlock(DBBCB & bcb)
{
    LOG4CXX_INFO(logger,"flushBCBBlock()");
    LOG4CXX_DEBUG(logger,"bcb:\n" + bcb.toString("\t"));
	MONITOR_FUNC;
	lock();
    try{
	   if(bcb.getModified() == true && bcb.getDirty() == false){
	   		fileMgr.writeFileBlock(bcb.getFileBlock());
	   		bcb.unsetModified();
	   }
    }catch(DBException e){
        unlock();
        throw e;
    }
	unlock();
}

DBBACB DBBufferMgr::upgradeToExclusive(const DBBACB & bacb)
{
    LOG4CXX_INFO(logger,"upgradeToExclusive()");
    LOG4CXX_DEBUG(logger,"bacb:\n" + bacb.toString("\t"));
    lock();
	DBBCB & bcb = bacb.getBCB();
    try{
        while(bcb.upgradeLock()==false)
            waitForLock();
    }catch(DBException e){
        unlock();
        throw e;
    }
    unlock();
    return bcb;
}

const DBBACB DBBufferMgr::downgradeToShared(const DBBACB & bacb)
{
    LOG4CXX_INFO(logger,"downgradeToShared()");
    LOG4CXX_DEBUG(logger,"bacb:\n" + bacb.toString("\t"));
    lock();
	DBBCB & bcb = bacb.getBCB();
    try{
        while(bcb.downgradeLock()==false)
            waitForLock();
    }catch(DBException e){
        unlock();
        throw e;
    }
    unlock();
    return bcb;
}

DBBACB DBBufferMgr::fixBlock(DBFile & file,BlockNo blockNo,DBBCBLockMode mode)
{
    LOG4CXX_INFO(logger,"fixBlock()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger,"blockNo: " + TO_STR(blockNo));
    LOG4CXX_DEBUG(logger,"mode: " + DBBCB::LockMode2String(mode));
    lock();
    DBBCB * bcb = NULL;
    try{
	    while((bcb= fixBlock(file,blockNo,mode,true))==NULL)
    	    waitForLock();
    }catch(DBException e){
        unlock();
        throw e;
    }
    unlock();
    return DBBACB(*bcb);
}

DBBACB DBBufferMgr::fixNewBlock(DBFile & file)
{
    LOG4CXX_INFO(logger,"fixNewBlock()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
    lock();
    DBBCB * bcb = NULL;
    try{
    	BlockNo b = fileMgr.getBlockCnt(file);
    	fileMgr.setBlockCnt(file,b+1);
    	while((bcb= fixBlock(file,b,LOCK_EXCLUSIVE,false))==NULL)
        	waitForLock();
    	memset(bcb->getDataPtr(),0,DBFileBlock::getBlockSize());
        this->blockCounter++;
    }catch(DBException e){
        unlock();
        throw e;
    }
    unlock();
    return *bcb;
}

DBBACB DBBufferMgr::fixEmptyBlock(DBFile & file,BlockNo blockNo)
{
    LOG4CXX_INFO(logger,"fixNewBlock()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger,"blockNo: " + TO_STR(blockNo));
    lock();
    DBBCB * bcb = NULL;
    try{
    	while((bcb= fixBlock(file,blockNo,LOCK_EXCLUSIVE,false))==NULL)
        	waitForLock();
    	memset(bcb->getDataPtr(),0,DBFileBlock::getBlockSize());
    }catch(DBException e){
        unlock();
        throw e;
    }
    unlock();
    return *bcb;
}

void DBBufferMgr::unfixBlock(const DBBACB & bacb)
{
    LOG4CXX_INFO(logger,"unfixBlock()");
    LOG4CXX_DEBUG(logger,"bacb:\n"+ bacb.toString("\t"));
	lock();
	DBBCB & bcb = bacb.getBCB();
	try{
		unfixBlock(bcb);
	    emitSignal();			
	}catch(DBException e){
        unlock();
        throw e;
    }
	unlock();
}

void DBBufferMgr::waitForLock()
{
    LOG4CXX_INFO(logger,"waitForLock()");
    LOG4CXX_DEBUG(logger,"threading: "+ TO_STR(doThreading()));
    if(doThreading())
        pthread_cond_wait(&cond, getMutex().getPthread_mutex());
};

void DBBufferMgr::emitSignal()
{
    LOG4CXX_INFO(logger,"emitSignal()");
    LOG4CXX_DEBUG(logger,"threading: "+ TO_STR(doThreading()));
    if(doThreading()) pthread_cond_signal(&cond);
};

DBBufferMgrException::DBBufferMgrException(const std::string& msg1)
     : DBException(msg1)
{
}

DBBufferMgrException::DBBufferMgrException(const DBBufferMgrException& src)
      : DBException(src)
{
}

DBBufferMgrException& DBBufferMgrException::operator=(const DBBufferMgrException& src)
{
      DBException::operator=(src);
      return *this;
}
