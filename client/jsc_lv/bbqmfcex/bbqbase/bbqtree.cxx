
#include "bbqbase.h"
#include "bbqtree.h"

// --------------------------------------------------------------------------
// uint_list

PBytePack & operator<< ( PBytePack & pack, uint_list & idList )
{
	unsigned int n = idList.size();
	pack << n;

	if( n > 0 ) {
		uint32 * values = new uint32[ n ]; 
		if( values ) {
			unsigned int i = 0;
			for( uint_list::iterator it = idList.begin(), et = idList.end(); it != et; it ++, i ++ ) {
				values[ i ] = * it;
			}
			pack.Pack( (const void *) values, sizeof(uint32) * n );
			delete[] values; 
		}
	}

	return pack;
}

PBytePack & operator>> ( PBytePack & pack, uint_list & idList )
{
	unsigned int n = 0;
	pack >> n;

	if( n > 0 ) {
		uint32 * values = new uint32[ n ]; 
		if( values ) {
			int m = pack.Unpack( values, sizeof(uint32) * n ); // m == sizeof(uint32) * n
			for( int i=0; i<n; i++ ) idList.push_back( values[i] );
			delete[] values; 
		}
	}

	return pack;
}

// ----------------------------------------------------------------------------
// BBQTrees
PBytePack & operator<< ( PBytePack & pack, BBQTrees & trees )
{
	pack << trees.m_listRootValues;

	unsigned int n = trees.m_mapTreeNodes.size();
	pack << n;
	if( n > 0 ) {
		for( BBQTreeNodeMap::iterator it = trees.m_mapTreeNodes.begin(), et = trees.m_mapTreeNodes.end(); it != et; it ++ ) {
			pack << (it->first);
			pack << ( * (it->second) );
		}
	}
	return pack;
}

PBytePack & operator>> ( PBytePack & pack, BBQTrees & trees )
{
	pack >> trees.m_listRootValues;

	unsigned int n = 0, i = 0;
	pack >> n;
	if( n > 0 ) {
		for( i=0; i<n; i++ ) {
			uint32 value = 0;
			BBQTreeNode * pNode = new BBQTreeNode; 
			if( pNode ) {
				pack >> value;
				pack >> (* pNode);
				if( trees.m_mapTreeNodes.find( value ) != trees.m_mapTreeNodes.end() ) {
					delete pNode; 
				} else {
					trees.m_mapTreeNodes[ value ] = pNode;
				}
			}
		}
	}

	return pack;
}

BBQTrees::BBQTrees()
{
}

BBQTrees::~BBQTrees()
{
	RemoveAll();
}

void BBQTrees::RemoveAll()
{
	for( BBQTreeNodeMap::iterator it = m_mapTreeNodes.begin(), et = m_mapTreeNodes.end(); it != et; /*it ++, already call erase() */ ) {
		BBQTreeNode * p = it->second;;

		STL_ERASE( m_mapTreeNodes, BBQTreeNodeMap::iterator, it );
		delete p; 
	}
	m_listRootValues.clear();
}

bool BBQTrees::IsEmpty( void )
{
	return m_listRootValues.empty();
}

bool BBQTrees::Exists( uint32 nValue )
{
	{
		for( uint_list::iterator it = m_listRootValues.begin(), eit = m_listRootValues.end(); it != eit; it ++ ) {
			if( nValue == * it ) return true;
		}
	}

	{
		for( BBQTreeNodeMap::iterator it = m_mapTreeNodes.begin(), eit = m_mapTreeNodes.end(); it != eit; it ++ ) {
			uint_list * pList = it->second;
			for( uint_list::iterator i = pList->begin(), e = pList->end(); i != e; i ++ ) {
				if( nValue ==  * i ) return true;
			}
		}
	}

	return false;
}

BBQTreeNode * BBQTrees::FindNode( uint32 nValue )
{
	BBQTreeNodeMap::iterator it = m_mapTreeNodes.find( nValue );
	return ( it != m_mapTreeNodes.end() ) ? ( it->second ) : NULL;
}

// for debug only
void BBQTrees::PrintSubTree( PString & str, uint32 nValue, int depth )
{
	if( depth >= TREE_MAX_DEPTH ) return;

	uint_list * pList = FindNode( nValue );
	if( ! pList ) return;

	for( uint_list::iterator it = pList->begin(), et = pList->end(); it != et; it ++ ) {
		for( int i=0; i<depth; i++ ) str += (i>0) ? "|    " : "     ";
		str += PString( PString::Printf, "|_ %d\n", * it );
		PrintSubTree( str, * it, depth +1 );
	}
}
 void BBQTrees::Print( PString & str )
{
	for( uint_list::iterator it = m_listRootValues.begin(), et = m_listRootValues.end(); it != et; it ++ ) {
		str += PString( PString::Printf, "- %d\n", * it );
		PrintSubTree( str, * it, 1 );
	}		
}
extern PString g_GetAliasName(uint32 uid, bool);
void BBQTrees::PrintSubTreeXML( PString & str, uint32 nValue, int depth ,std::map<uint32,uint32>&  ids,bool bIncludeAlias)
{
	if( depth >= TREE_MAX_DEPTH ) return;

	uint_list * pList = FindNode( nValue );
	if( ! pList ) return;

	for( uint_list::iterator it = pList->begin(), et = pList->end(); it != et; it ++ ) {
		//for( int i=0; i<depth; i++ ) str += (i>0) ? "|    " : "     ";
    if (bIncludeAlias == false)
		  str += PString( PString::Printf, "<entry uid=\"%d\">", * it   );
    else
		  str += PString( PString::Printf, "<entry uid=\"%d\" alias=\"%s\">", * it , (const char*) g_GetAliasName(* it, false) );
    ids[ * it]= * it;
		PrintSubTreeXML( str, * it, depth +1 , ids, bIncludeAlias);
    str+= "</entry>";
	}
}
void BBQTrees::PrintXML( PString & str ,std::map<uint32,uint32>&  ids,bool bIncludeAlias)
{
  str=(const char*) "<?xml version=\"1.0\" encoding=\"UTF-8\"?><root>";
	for( uint_list::iterator it = m_listRootValues.begin(), et = m_listRootValues.end(); it != et; it ++ ) {
    ids[ * it]= * it;
    if (bIncludeAlias == false)
		  str += PString( PString::Printf, "<entry uid=\"%d\"  >", * it  );
    else
		  str += PString( PString::Printf, "<entry uid=\"%d\" alias=\"%s\">", * it, (const char*) g_GetAliasName(* it,false) );

		PrintSubTreeXML( str, * it, 1 , ids, bIncludeAlias);
    str+= "</entry>";
	}		
  str+="</root>";
}
