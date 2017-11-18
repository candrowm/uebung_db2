#include <hubDB/DBRandomBufferMgr.h>
#include <hubDB/DBException.h>
#include <hubDB/DBMonitorMgr.h>

using namespace HubDB::Manager;
using namespace HubDB::Exception;

LoggerPtr DBRandomBufferMgr::logger(Logger::getLogger("HubDB.Buffer.DBRandomBufferMgr"));
int rBMgr = DBRandomBufferMgr::registerClass();

extern "C" void * createDBRandomBufferMgr(int nArgs,va_list & ap);

DBRandomBufferMgr::DBRandomBufferMgr (bool doThreading,int cnt):
	DBBufferMgr(doThreading,cnt),
	bcbList(NULL)
{
    if(logger!=NULL)
        LOG4CXX_INFO(logger,"DBRandomBufferMgr()");

    bcbList = new DBBCB*[maxBlockCnt];
    for (int i = 0; i < maxBlockCnt; i++) {
        bcbList[i] = NULL;
    }

    mapSize = cnt / 32 + 1 ;
    bitMap = new int[mapSize];
	
    for(int i=0;i<mapSize;++i){
      bitMap[i] = 0;
    }

    for(int i=0;i<cnt;++i){
      setBit(i);
    }

    if(logger!=NULL)
        LOG4CXX_DEBUG(logger,"this:\n"+toString("\t"));
}

DBRandomBufferMgr::~DBRandomBufferMgr ()
{
    LOG4CXX_INFO(logger,"~DBRandomBufferMgr()");
	LOG4CXX_DEBUG(logger,"this:\n"+toString("\t"));    
	if(bcbList!=NULL){
		for(int i=0;i<maxBlockCnt;++i){
			if(bcbList[i]!=NULL){
				try{					
					flushBCBBlock(*bcbList[i]);
				}catch(DBException & e){}					
				delete bcbList[i];
			}
		}
		delete [] bcbList;
		delete [] bitMap;
	}
}

string DBRandomBufferMgr::toString(string linePrefix) const
{
	stringstream ss;
	ss << linePrefix << "[DBRandomBufferMgr]"<<endl;
	ss << DBBufferMgr::toString(linePrefix+"\t");
	lock();

	uint i,sum =0;
	for(i=0;i<maxBlockCnt;++i){
	  if(getBit(i)==1)
	    ++sum;
	}
	ss << linePrefix << "unfixedPages( size: " << sum <<" ):" <<endl;
	for(i=0;i<maxBlockCnt;++i){
	  if(getBit(i)==1)
	    ss << linePrefix << i << endl;
	}

	ss << linePrefix << "bcbList( size: " << maxBlockCnt <<" ):" <<endl;
	for(int i=0;i<maxBlockCnt;++i){
		ss << linePrefix << "bcbList["<<i<<"]:";
		if(bcbList[i] == NULL)
			ss << "NULL" << endl;
		else
			ss << endl << bcbList[i]->toString(linePrefix+"\t");
	}
	ss << linePrefix << "-------------------" <<endl;
	unlock();
	return ss.str();
}

int DBRandomBufferMgr::registerClass(){
	setClassForName("DBRandomBufferMgr",createDBRandomBufferMgr);
	return 0;
}

DBBCB * DBRandomBufferMgr::fixBlock(DBFile & file,BlockNo blockNo,DBBCBLockMode mode,bool read)
{
    LOG4CXX_INFO(logger,"fixBlock()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger,"blockNo: " + TO_STR(blockNo));
    LOG4CXX_DEBUG(logger,"mode: " + DBBCB::LockMode2String(mode));
    LOG4CXX_DEBUG(logger,"read: " + TO_STR(read));
    LOG4CXX_DEBUG(logger,"this:\n" + toString("\t"));
    		
	int i = findBlock(file,blockNo);
	
	LOG4CXX_DEBUG(logger,"i:" + TO_STR(i));
	
	if(i == -1){
	  for(i=0;i<maxBlockCnt;++i){
	    if(getBit(i)==1)
	      break;
	  }
	  if(i==maxBlockCnt)
	    throw DBBufferMgrException("no more free pages");

	  if(bcbList[i]!=NULL){
	    if(bcbList[i]->getDirty()==false)
	      flushBCBBlock(*bcbList[i]);
	    delete bcbList[i];
	  }
	  bcbList[i] = new DBBCB(file,blockNo);
	  if(read == true)
	    fileMgr.readFileBlock(bcbList[i]->getFileBlock());
	}
		
	DBBCB *rc = bcbList[i];
    if(rc->grantAccess(mode)==false){
    	rc = NULL;
    }else{
      unsetBit(i);
    }
    
    LOG4CXX_DEBUG(logger,"rc: " + TO_STR(rc));
	return rc;
}

void DBRandomBufferMgr::unfixBlock(DBBCB & bcb)
{
    LOG4CXX_INFO(logger,"unfixBlock()");
    LOG4CXX_DEBUG(logger,"bcb:\n" + bcb.toString("\t"));
    LOG4CXX_DEBUG(logger,"this:\n" + toString("\t"));
	bcb.unlock();
	int i = findBlock(&bcb);
	if(bcb.getDirty()==true){
		delete bcbList[i];
		bcbList[i]=NULL;
		setBit(i);
	}else if(bcb.isUnlocked()==true){
	  setBit(i);
	}
}

bool DBRandomBufferMgr::isBlockOfFileOpen(DBFile & file) const
{
    LOG4CXX_INFO(logger,"isBlockOfFileOpen()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger,"this:\n" + toString("\t"));
	for(int i=0;i<maxBlockCnt;++i){
		if(bcbList[i]!=NULL && bcbList[i]->getFileBlock() == file){
			LOG4CXX_DEBUG(logger,"rc: true");
			return true;
		}
	}
	LOG4CXX_DEBUG(logger,"rc: false");
	return false;
}

void DBRandomBufferMgr::closeAllOpenBlocks(DBFile & file)
{
    LOG4CXX_INFO(logger,"closeAllOpenBlocks()");
    LOG4CXX_DEBUG(logger,"file:\n" + file.toString("\t"));
    LOG4CXX_DEBUG(logger,"this:\n" + toString("\t"));
	for(int i=0;i<maxBlockCnt;++i){
		if(bcbList[i]!=NULL && bcbList[i]->getFileBlock() == file){
			if(bcbList[i]->isUnlocked() == false)
				throw DBBufferMgrException("can not close fileblock because it is still lock");
			flushBCBBlock(*bcbList[i]);
			delete bcbList[i];
			bcbList[i] = NULL;
			setBit(i);
		}
	}
}

int DBRandomBufferMgr::findBlock(DBFile & file,BlockNo blockNo)
{
    LOG4CXX_INFO(logger,"findBlock()");
	int pos = -1;
	for(int i=0;pos == -1 && i<maxBlockCnt;++i){
		if(bcbList[i]!=NULL && 
		   bcbList[i]->getFileBlock() == file &&
		   bcbList[i]->getFileBlock().getBlockNo() == blockNo)
		   	pos = i;
	}
    LOG4CXX_DEBUG(logger,"pos: " + TO_STR(pos));
	return pos;
}

int DBRandomBufferMgr::findBlock(DBBCB * bcb)
{
    LOG4CXX_INFO(logger,"findBlock()");
	int pos = -1;
	for(int i=0;pos == -1 && i<maxBlockCnt;++i){
		if(bcbList[i]==bcb)
			pos = i;
	}
    LOG4CXX_DEBUG(logger,"pos: " + TO_STR(pos));
	return pos;
}

extern "C" void * createDBRandomBufferMgr(int nArgs,va_list & ap){
	DBRandomBufferMgr * b = NULL;
	bool t;
	uint c;
	switch(nArgs){
	case 1:
		t = va_arg(ap,int);
		b = new DBRandomBufferMgr(t);
		break;
	case 2:
		t = va_arg(ap,int);
		c = va_arg(ap,int);
		b = new DBRandomBufferMgr(t,c);
		break;
	default:
		throw DBException("Invalid number of arguments");
	}
	return b;
}
