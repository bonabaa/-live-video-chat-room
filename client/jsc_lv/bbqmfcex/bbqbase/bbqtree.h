
#ifndef _BBQ_TREE_
#define _BBQ_TREE_

#include "sfidmsg.h"
#include "useridlist.h"
#include "bytepack.h"

#include <map>

#define		TREE_MAX_DEPTH	8

// #include <list>
// typedef std::list<unsigned int> uint_list;

typedef uint_list		BBQTreeNode;
typedef std::map<uint32, BBQTreeNode *> BBQTreeNodeMap;

class BBQTrees
{
public:
	uint_list		m_listRootValues;	// root nodes, might be more trees maintains in this class
	BBQTreeNodeMap	m_mapTreeNodes;		// node value as the key, only contain those nodes that have sub nodes

	BBQTrees();
	~BBQTrees();

	void RemoveAll();
	BBQTreeNode * FindNode( uint32 nValue ); // if not found, return NULL;

	bool IsEmpty( void );
	bool Exists( uint32 nValue );

	// pack/unpack for network transfering
	friend PBytePack & operator<< ( PBytePack & pack, BBQTrees & tree );
	friend PBytePack & operator>> ( PBytePack & pack, BBQTrees & tree );

	// for debug only
	void PrintSubTree( PString & str, uint32 nValue, int depth = 0 );
	void Print( PString & str );
	void PrintSubTreeXML( PString & str, uint32 nValue, int depth  ,std::map<uint32,uint32>&  ids,bool bIncludeAlias);
	void PrintXML( PString & str ,std::map<uint32,uint32>&  ids, bool bIncludeAlias);
};

PBytePack & operator<< ( PBytePack & pack, uint_list & idList );
PBytePack & operator>> ( PBytePack & pack, uint_list & idList );

#endif // _BBQ_TREE_
