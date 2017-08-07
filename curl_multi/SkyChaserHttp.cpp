#include "StdAfx.h"
#include "SkyChaserHttp.h"
#include <sys/timeb.h>
#include <fstream>
using std::fstream;
#include <WinSock2.h>

#pragma comment ( lib, "ws2_32.lib" )
#pragma comment ( lib, "wldap32.lib" )

#define ZLIB_WINAPI
#define CURL_STATICLIB
#include "libcurl/curl.h"
#include "libcurl/zlib/zlib.h"
#if _DEBUG
#pragma comment( lib, "libcurl/lib/zlibd.lib")
#pragma comment( lib, "libcurl/lib/libcurld.lib")
#pragma comment( lib, "libcurl/lib/libeay32d.lib" )
#pragma comment( lib, "libcurl/lib/ssleay32d.lib" )
#else
#pragma comment( lib, "libcurl/lib/zlib.lib")
#pragma comment( lib, "libcurl/lib/libcurl.lib")
#pragma comment( lib, "libcurl/lib/libeay32.lib" )
#pragma comment( lib, "libcurl/lib/ssleay32.lib" )
#endif


CSkyChaserHttp::CSkyChaserHttp( bool isMuliti ) :
	 m_hCurl( NULL )	
	,m_header( NULL )
  , m_isMulti(isMuliti)
{
	m_pCookieEx = new std::map<std::string, std::string>;

	CURLcode pCode;
	pCode = curl_global_init( CURL_GLOBAL_ALL );
	if ( pCode != CURLE_OK )
	{
		OutputDebugStringW( L"CSkyChaserHttp: curl_global_init Failed...\r\n" );
		return;
	}

	m_hCurl = curl_easy_init();
	if ( !m_hCurl )
	{
		OutputDebugStringW( L"CSkyChaserHttp:curl_easy_init Failed...\r\n" );
		return;
	}

	curl_easy_setopt( m_hCurl, CURLOPT_ACCEPT_ENCODING, "gzip"); 
	curl_easy_setopt( m_hCurl, CURLOPT_FOLLOWLOCATION,1 );

	curl_easy_setopt( m_hCurl, CURLOPT_SSL_VERIFYPEER, false );
	curl_easy_setopt( m_hCurl, CURLOPT_SSL_VERIFYHOST, false );
}

CSkyChaserHttp::~CSkyChaserHttp(void)
{
	delete m_pCookieEx;
	m_pCookieEx = NULL;

	sc_cleanHeader();

	if ( m_hCurl )
		curl_easy_cleanup( m_hCurl );

	m_hCurl = NULL;
	curl_global_cleanup();
}


void CSkyChaserHttp::sc_setCookieFile( const char * sFilename )
{
	if ( !m_hCurl )
		return;

	m_szCookie = (char*)sFilename;
	curl_easy_setopt( m_hCurl, CURLOPT_COOKIEJAR, m_szCookie );
	curl_easy_setopt( m_hCurl, CURLOPT_COOKIEFILE, m_szCookie );
}

CURLcode CSkyChaserHttp::sc_get( const char * sURL, const char * sFileName, bool bCaseRepHdr )
{
	if ( !m_hCurl )
		return CURLE_FAILED_INIT;

	CURLcode retCode = CURLE_FAILED_INIT;

	FILE * pFile = NULL;
	if ( sFileName )
	{
		errno_t err = fopen_s( &pFile, sFileName, "wb" );
		if ( err != 0 )
		{
			OutputDebugStringW( L"CSkyChaserHttp:Open File Failed...\r\n" );
			return retCode;	
		}
	}
	if ( m_header )
		curl_easy_setopt( m_hCurl, CURLOPT_HTTPHEADER, m_header );

	if ( sFileName )
	{
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackWriteFile );
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, (void*)pFile );
	}
	else
	{
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackNotCase );
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, NULL );
	}

	curl_easy_setopt(m_hCurl, CURLOPT_HEADER, bCaseRepHdr );
	curl_easy_setopt(m_hCurl, CURLOPT_URL, sURL);
	curl_easy_setopt(m_hCurl, CURLOPT_POST, 0);
	curl_easy_setopt(m_hCurl, CURLOPT_HTTPGET, 1);
	_checkExCookies();
  
  if(!m_isMulti)
  {
	  retCode = curl_easy_perform( m_hCurl ); 
	  if ( sFileName )
	  	fclose( pFile );
  }

	return retCode;
}

CURLcode CSkyChaserHttp::sc_get( const char * sURL, std::string & sRet, bool bCaseRepHdr )
{
	if ( !m_hCurl )
		return CURLE_FAILED_INIT;

	sRet.clear();
	CURLcode retCode = CURLE_FAILED_INIT;
	curl_easy_setopt(m_hCurl, CURLOPT_HEADER, bCaseRepHdr );
	curl_easy_setopt(m_hCurl, CURLOPT_URL, sURL);
	curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackWriteString );
	curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, &sRet );
	curl_easy_setopt(m_hCurl, CURLOPT_POST, 0);
	curl_easy_setopt(m_hCurl, CURLOPT_HTTPGET, 1);
	if ( m_header )
		curl_easy_setopt(m_hCurl, CURLOPT_HTTPHEADER, m_header);

	_checkExCookies();
  if(!m_isMulti)
  {
    retCode = curl_easy_perform(m_hCurl);
  }
	return retCode;
}


CURLcode CSkyChaserHttp::sc_post( const char * sURL, const char * sData, const wchar_t * sFileName, bool bCaseRepHdr )
{
	if ( !m_hCurl )
		return CURLE_FAILED_INIT;

	CURLcode retCode = CURLE_FAILED_INIT;

	FILE * pFile = NULL;
	if ( sFileName )
	{
		errno_t err = _wfopen_s( &pFile, sFileName, L"wb" );
		if ( err != 0 )
		{
			OutputDebugStringW( L"CSkyChaserHttp:Open File Failed...\r\n" );
			return retCode;	
		}
	}
	if ( m_header )
		curl_easy_setopt( m_hCurl, CURLOPT_HTTPHEADER, m_header );
	if ( sFileName )
	{
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackWriteFile );
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, (void*)pFile );
	}
	else
	{
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackNotCase );
		curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, NULL );
	}
	curl_easy_setopt(m_hCurl, CURLOPT_HEADER, bCaseRepHdr );
	curl_easy_setopt(m_hCurl, CURLOPT_POSTFIELDS, sData);
	curl_easy_setopt(m_hCurl, CURLOPT_URL, sURL);
	curl_easy_setopt(m_hCurl, CURLOPT_POST, 1);
	curl_easy_setopt(m_hCurl, CURLOPT_HTTPGET, 0);
	_checkExCookies();
  if(!m_isMulti)
  {
    retCode = curl_easy_perform(m_hCurl);
    if(sFileName)
      fclose(pFile);
  }
	return retCode;
}

CURLcode CSkyChaserHttp::sc_post( const char * sURL, const char * sData, std::string & sRet, bool bCaseRepHdr )
{
	if ( !m_hCurl )
		return CURLE_FAILED_INIT;

	sRet.clear();
	CURLcode retCode = CURLE_FAILED_INIT;
	curl_easy_setopt(m_hCurl, CURLOPT_HEADER, bCaseRepHdr );
	curl_easy_setopt(m_hCurl, CURLOPT_POSTFIELDS, sData);
	curl_easy_setopt(m_hCurl, CURLOPT_URL, sURL);
	curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackWriteString );
	curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, &sRet );
	curl_easy_setopt(m_hCurl, CURLOPT_POST, 1);
	curl_easy_setopt(m_hCurl, CURLOPT_HTTPGET, 0);
	if ( m_header )
		curl_easy_setopt(m_hCurl, CURLOPT_HTTPHEADER, m_header);

	_checkExCookies();
  
  if(!m_isMulti)
  {
    retCode = curl_easy_perform(m_hCurl);
  }
	return retCode;
}

CURLcode CSkyChaserHttp::sc_post( const char * sURL, const char * sFieldName, const char * sFileName, std::string & sRet, bool bCaseRepHdr )
{
	sRet.clear();
	
	struct curl_httppost* post = NULL;
	struct curl_httppost* last = NULL;
	/* Add file/contenttype section */
	curl_formadd( &post, &last, CURLFORM_COPYNAME, sFieldName, CURLFORM_FILE, sFileName, CURLFORM_END);

	/* Set the form info */
	curl_easy_setopt( m_hCurl, CURLOPT_URL, sURL );
	curl_easy_setopt( m_hCurl, CURLOPT_HEADER, bCaseRepHdr );
	curl_easy_setopt( m_hCurl, CURLOPT_HTTPPOST, post );
	curl_easy_setopt( m_hCurl, CURLOPT_WRITEFUNCTION, _callBackWriteString );
	curl_easy_setopt( m_hCurl, CURLOPT_WRITEDATA, &sRet );
	if ( m_header )
		curl_easy_setopt(m_hCurl, CURLOPT_HTTPHEADER, m_header);

	CURLcode retCode = CURLE_FAILED_INIT;
  if(!m_isMulti)
  {
    retCode = curl_easy_perform(m_hCurl);
  }
	return retCode;
}

CURLcode CSkyChaserHttp::sc_put( const char * sURL, const char * sData, std::string & sRet, bool bCaseRepHdr )
{
	if ( !m_hCurl )
		return CURLE_FAILED_INIT;

	sRet.clear();
	CURLcode retCode = CURLE_FAILED_INIT;
	curl_easy_setopt(m_hCurl, CURLOPT_HEADER, bCaseRepHdr );
	curl_easy_setopt(m_hCurl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(m_hCurl, CURLOPT_POSTFIELDS, sData);
	curl_easy_setopt(m_hCurl, CURLOPT_POSTFIELDSIZE, strlen(sData) );
	curl_easy_setopt(m_hCurl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(m_hCurl, CURLOPT_URL, sURL);
	curl_easy_setopt(m_hCurl, CURLOPT_WRITEFUNCTION, _callBackWriteString );
	curl_easy_setopt(m_hCurl, CURLOPT_WRITEDATA, &sRet );
	if ( m_header )
		curl_easy_setopt(m_hCurl, CURLOPT_HTTPHEADER, m_header);

	_checkExCookies();
	retCode = curl_easy_perform( m_hCurl );  

	curl_easy_setopt( m_hCurl, CURLOPT_CUSTOMREQUEST, NULL );
	curl_easy_setopt(m_hCurl, CURLOPT_POSTFIELDSIZE, -1 );
	return retCode;
}

void CSkyChaserHttp::sc_cleanHeader()
{
	if ( m_header )
		curl_slist_free_all( m_header );

	if ( m_hCurl )
		curl_easy_setopt( m_hCurl, CURLOPT_HTTPHEADER, NULL );

	m_header = NULL;
}

void CSkyChaserHttp::sc_appendHeader( const char * sHeader )
{
	m_header = curl_slist_append( m_header, sHeader );
}

std::string CSkyChaserHttp::sc_getCookie( const char * sCookieName )
{
	std::string sRet("");
	struct curl_slist * cookies = NULL;
	if ( !m_hCurl )
		return sRet;

	CURLcode ret = curl_easy_getinfo( m_hCurl, CURLINFO_COOKIELIST, &cookies );
	if ( ret != CURLE_OK )
		return sRet;

	int nPos;
	while ( cookies ) 
	{  
		sRet = cookies->data;
		nPos = sRet.find( sCookieName, 0 );
		if ( nPos != std::string::npos )
		{
			nPos += strlen( sCookieName ) + 1;
			sRet = sRet.substr( nPos, sRet.length()-nPos );
			break;
		}
		else
			sRet.clear();

		cookies = cookies->next;  
	} 

	curl_slist_free_all( cookies );
	return sRet;
}

std::string CSkyChaserHttp::sc_getCookie()
{
	std::string sRet("");
	struct curl_slist * cookies = NULL;
	if ( !m_hCurl )
		return sRet;

	CURLcode ret = curl_easy_getinfo( m_hCurl, CURLINFO_COOKIELIST, &cookies );
	if ( ret != CURLE_OK )
		return sRet;

	//int nPos;
	while ( cookies ) 
	{  
		sRet = cookies->data;
		//nPos = sRet.find( sCookieName, 0 );
		// 		if ( nPos != std::string::npos )
		// 		{
		// 			nPos += strlen( sCookieName ) + 1;
		// 			sRet = sRet.substr( nPos, sRet.length()-nPos );
		// 			break;
		// 		}
		// 		else
		// 			sRet.clear();

		cookies = cookies->next;  
	} 

	curl_slist_free_all( cookies );
	return sRet;
}

size_t CSkyChaserHttp::_callBackWriteFile( void * buffer, size_t size, size_t nmemb, void * usrFp )
{
	size_t xSize = size * nmemb;
	FILE * pHFile = (FILE*)usrFp;
	if ( !pHFile )
		return xSize;

	fwrite( buffer, xSize, 1, pHFile );
	return xSize;
}

size_t CSkyChaserHttp::_callBackNotCase(void * buffer, size_t size, size_t nmemb, void * usrFp)
{
	return size * nmemb;
}

size_t CSkyChaserHttp::_callBackWriteString(void * buffer, size_t size, size_t nmemb, void * usrFp)
{
	size_t xSize = size*nmemb;
	std::string * pStr = (std::string*)usrFp;
	if ( pStr )
		pStr->append((char*)buffer, xSize);

	return xSize;
}

// 设置代理
void CSkyChaserHttp::sc_setProxy( const char * sProxy, char * sPass, curl_proxytype ntype )
{
	if ( !m_hCurl )
		return;

	curl_easy_setopt( m_hCurl, CURLOPT_PROXY, sProxy);
	curl_easy_setopt(m_hCurl, CURLOPT_PROXYTYPE, ntype);
	if ( sPass )
		curl_easy_setopt(m_hCurl, CURLOPT_PROXYPASSWORD, sPass);
}


bool CSkyChaserHttp::sc_readTextFile( const wchar_t * sFileName, string & sRet )
{
	sRet.clear();

	FILE * pFile = NULL;
	errno_t err = _wfopen_s( &pFile, sFileName, L"rb" );
	if ( err != 0 )
		return false;

	char sBuf[2048];
	while ( !feof( pFile ) )
	{
		memset( sBuf, 0, 2048 );
		fread( sBuf, 2048, 1, pFile);
		sRet.append( sBuf );
	} 

	fclose( pFile );
	return true;
}


std::string CSkyChaserHttp::sc_getTimeStamp( int nOffest )
{
	struct timeb f;  
	ftime( &f );  
	__int64 b = f.time * 1000 + f.millitm + nOffest;

	char sBuf[80] = {0};
	sprintf_s(sBuf, 80, "%I64d", b);

	std::string str = sBuf;
	return str;
}

// 设置最大等待时间
void CSkyChaserHttp::sc_setTimeOut( int nTimeOutMillisecond )
{
	if ( !m_hCurl )
		return;

	curl_easy_setopt( m_hCurl, CURLOPT_TIMEOUT_MS, nTimeOutMillisecond );
	curl_easy_setopt( m_hCurl, CURLOPT_CONNECTTIMEOUT_MS, nTimeOutMillisecond );
}

void CSkyChaserHttp::sc_delCookieFile(void)
{
	if ( m_szCookie )
		DeleteFileA( m_szCookie );
}

void CSkyChaserHttp::sc_setCookie( const char * sName, const char * sValue )
{
	if ( !m_hCurl )
		return;

	(*m_pCookieEx)[ sName ] = sValue;
}

void CSkyChaserHttp::_checkExCookies()
{
	string sTmp("");
	map<string, string>::iterator itr = m_pCookieEx->begin(), ite = m_pCookieEx->end();
	for ( ; itr != ite; itr++ )
		sTmp += itr->first + "=" + itr->second + ";";

	if ( sTmp.empty() )
		return;

	sTmp = sTmp.substr( 0, sTmp.length() - 2 );
	curl_easy_setopt( m_hCurl, CURLOPT_COOKIE, sTmp.c_str() );
}

void CSkyChaserHttp::sc_cleanExCookie()
{
	m_pCookieEx->clear();
}

bool CSkyChaserHttp::sc_writeLog( const wchar_t * sFile, const char * sText )
{
	FILE * pFile = NULL;
	errno_t err = _wfopen_s( &pFile, sFile, L"ab+" );
	if ( err != 0 )
		return false;

	fwrite( sText, strlen(sText), 1, pFile );
	fwrite( "\r\n", strlen( "\r\n" ), 1, pFile );
	fclose( pFile );
	return true;
}


int CSkyChaserHttp::sc_getMidString( string src, string & dst, const char * sLeft, const char * sRight, int nStart )
{
	dst = "";
	int nPosl = src.find( sLeft, nStart );
	if ( nPosl < 0 )
		return -1;

	nPosl += strlen( sLeft );
	int nPosr = src.find( sRight, nPosl );
	if ( nPosr < 0 )
		return -1;

	int nCount = nPosr - nPosl;
	if ( nCount < 0 )
		return -2;

	dst = src.substr( nPosl, nCount );
	return ( nPosr + strlen(  sRight ) );
}

int CSkyChaserHttp::sc_split( string str, string pattern, vector<string> & xRet )
{
	xRet.clear();
	string::size_type pos;
	str += pattern;
	int sizePattern = pattern.size() - 1;
	string::size_type size = str.size();
	for ( UINT i=0; i<size; i++ )
	{
		pos = str.find( pattern, i );
		if ( pos < size )
		{
			std::string s = str.substr( i, pos-i );
			xRet.push_back( s );
			i = pos + sizePattern;
		}
	}

	return xRet.size();
}

void CSkyChaserHttp::sc_replace( string & str, const char * sRp, const char * sTo )
{
	string::size_type pos = 0;
	string::size_type srcLen = strlen( sRp );
	string::size_type desLen = strlen( sTo );
	pos = str.find( sRp, pos );   
	while ( (pos != string::npos) )  
	{  
		str.replace( pos, srcLen, sTo );  
		pos = str.find( sRp, ( pos + desLen ) );  
	}  
}
void CSkyChaserHttp::sc_utf8ToAnsi( string & str )
{
	if ( str.empty() )
		return;

	DWORD dwLen = MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, NULL, 0 );
	wchar_t * strUnicode = new wchar_t[dwLen];
	if ( strUnicode == NULL )
		return;

	memset( strUnicode, 0, dwLen );
	MultiByteToWideChar( CP_UTF8, 0, str.c_str(), -1, strUnicode, dwLen );

	DWORD dwAnsiLen = WideCharToMultiByte( CP_ACP, 0, strUnicode, -1, NULL, 0, NULL, NULL );
	char * strAnsi = new char[dwAnsiLen];
	if (  strAnsi == NULL )
	{
		delete [] strUnicode;
		return;
	}
	memset( strAnsi, 0, dwAnsiLen );
	WideCharToMultiByte(CP_ACP, 0, strUnicode, -1, strAnsi, dwAnsiLen, NULL, NULL);

	str = strAnsi;

	delete [] strAnsi;
	delete [] strUnicode;
}

void CSkyChaserHttp::sc_ansiToUtf8( string & str )
{
	if ( str.empty() )
		return;

	DWORD dwLen = MultiByteToWideChar( CP_ACP, 0, str.c_str(), -1, NULL, 0 );
	wchar_t * strUnicode = new wchar_t[dwLen];
	if ( strUnicode == NULL )
		return;

	memset( strUnicode, 0, dwLen );
	MultiByteToWideChar( CP_ACP, 0, str.c_str(), -1, strUnicode, dwLen );

	DWORD dwUtf8Len = WideCharToMultiByte( CP_UTF8, 0, strUnicode, -1, NULL, 0, NULL, NULL );
	char * strUtf8 = new char[dwUtf8Len];
	if (  strUtf8 == NULL )
	{
		delete [] strUnicode;
		return;
	}
	memset( strUtf8, 0, dwUtf8Len );
	WideCharToMultiByte( CP_UTF8, 0, strUnicode, -1, strUtf8, dwUtf8Len, NULL, NULL );
	str = strUtf8;

	delete [] strUtf8;
	delete [] strUnicode;
}

void CSkyChaserHttp::sc_ansiToUnicode( const string & str, wstring & sUnicode )
{
	if ( str.empty() )
		return;

	DWORD dwLen = MultiByteToWideChar( CP_ACP, 0, str.c_str(), -1, NULL, 0 );
	wchar_t * strUnicode = new wchar_t[dwLen];
	if ( strUnicode == NULL )
		return;

	memset( strUnicode, 0, dwLen );
	MultiByteToWideChar( CP_ACP, 0, str.c_str(), -1, strUnicode, dwLen );
	sUnicode = strUnicode;
	delete [] strUnicode;
}

void CSkyChaserHttp::sc_unicodeToAnsi( const wstring & sUnicode, string & str )
{
	if ( sUnicode.empty() )
		return;

	DWORD dwAnsiLen = WideCharToMultiByte( CP_OEMCP, 0, sUnicode.c_str(), -1, NULL, 0, NULL, NULL );
	char * strAnsi = new char[dwAnsiLen];
	if (  strAnsi == NULL )
		return;
	memset( strAnsi, 0, dwAnsiLen );
	WideCharToMultiByte( CP_OEMCP, 0, sUnicode.c_str(), -1, strAnsi, dwAnsiLen, NULL, NULL );
	str = strAnsi;
	delete [] strAnsi;
}

void CSkyChaserHttp::sc_utf8ToUnicode( const string & sUtf8, wstring & sUnicode )
{
	string tmp( sUtf8 );
	CSkyChaserHttp::sc_utf8ToAnsi( tmp );
	CSkyChaserHttp::sc_ansiToUnicode( tmp, sUnicode );
}

void CSkyChaserHttp::sc_unicodeToUtf8( const wstring & sUnicode, string & sUtf8 )
{
	CSkyChaserHttp::sc_unicodeToAnsi( sUnicode, sUtf8 );
	CSkyChaserHttp::sc_ansiToUtf8( sUtf8 );
}

bool CSkyChaserHttp::sc_urlEncodeAnsi( string & strAnsiData, bool bUpperCase )
{
	string strEncode("");
	char baseChar = bUpperCase ? 'A' : 'a';
	unsigned char c;
	unsigned char c2;
	int cbDest = 0;
	unsigned char *pSrc   = (unsigned char*)strAnsiData.c_str();
	while( *pSrc )
	{
		c = *pSrc;
		if(isalpha(c) || isdigit(c) || c == '-' || c == '.' || c == '~')
		{
			strEncode += c;
			++cbDest;
		}
		else if(c == ' ')
		{
			strEncode += '+';
			++cbDest;
		}
		else
		{
			strEncode += '%';
			c2 = (c >= 0xA0) ? ((c >> 4) - 10 + baseChar) : ((c >> 4) + '0');
			strEncode += c2;
			c2 = ((c & 0xF) >= 0xA)? ((c & 0xF) - 10 + baseChar) : ((c & 0xF) + '0');
			strEncode += c2;
			cbDest += 3;
		}
		++pSrc;
	}

	strAnsiData = strEncode;
	return true;
}

bool CSkyChaserHttp::sc_urlEncodeUtf8( string & strUtf8Data, bool bUpperCase )
{
	string strEncode("");
	char baseChar = bUpperCase ? 'A' : 'a';
	unsigned char c;
	unsigned char c2;
	int cbDest = 0;
	unsigned char *pSrc   = (unsigned char*)strUtf8Data.c_str();
	while( *pSrc )
	{
		c = *pSrc;
		if(isalpha(c) || isdigit(c) || c == '-' || c == '.' || c == '~')
		{
			strEncode += c;
			++cbDest;
		}
		else if(c == ' ')
		{
			strEncode += '+';
			++cbDest;
		}
		else
		{
			strEncode += '%';
			c2 = (c >= 0xA0) ? ((c >> 4) - 10 + baseChar) : ((c >> 4) + '0');
			strEncode += c2;
			c2 = ((c & 0xF) >= 0xA)? ((c & 0xF) - 10 + baseChar) : ((c & 0xF) + '0');
			strEncode += c2;
			cbDest += 3;
		}
		++pSrc;
	}

	strUtf8Data = strEncode;
	return true;
}


bool CSkyChaserHttp::sc_urlDecodeUtf8( string & strUtf8Data )
{
	string strDecode( "" );

	int cbDest = 0; //累加
	unsigned char c;
	unsigned char * pSrc = (unsigned char*)strUtf8Data.c_str();
	while ( *pSrc )
	{
		if ( *pSrc == '%' )
		{
			c = 0;
			//高位
			if(pSrc[1] >= 'A' && pSrc[1] <= 'F')
				c += (pSrc[1] - 'A' + 10) * 0x10;
			else if(pSrc[1] >= 'a' && pSrc[1] <= 'f')
				c += (pSrc[1] - 'a' + 10) * 0x10;
			else
				c += (pSrc[1] - '0') * 0x10;

			//低位
			if ( pSrc[2] >= 'A' && pSrc[2] <= 'F' )
				c += ( pSrc[2] - 'A' + 10 );
			else if( pSrc[2] >= 'a' && pSrc[2] <= 'f' )
				c += ( pSrc[2] - 'a' + 10 );
			else
				c += ( pSrc[2] - '0' );
			pSrc += 3;
		}
		else if ( *pSrc == '+' )
		{
			c = ' ';
			++pSrc;
		}
		else
		{
			c = *pSrc;
			++pSrc;
		}

		strDecode += c;
		++cbDest;
	}

	strUtf8Data = strDecode;
	return true;
}

bool CSkyChaserHttp::sc_urlDecodeAnsi( string & strAnsiData )
{
	string strDecode("");

	int cbDest = 0; //累加
	unsigned char c;
	unsigned char * pSrc = (unsigned char*)strAnsiData.c_str();
	while ( *pSrc )
	{
		if ( *pSrc == '%' )
		{
			c = 0;
			//高位
			if(pSrc[1] >= 'A' && pSrc[1] <= 'F')
				c += (pSrc[1] - 'A' + 10) * 0x10;
			else if(pSrc[1] >= 'a' && pSrc[1] <= 'f')
				c += (pSrc[1] - 'a' + 10) * 0x10;
			else
				c += (pSrc[1] - '0') * 0x10;

			//低位
			if ( pSrc[2] >= 'A' && pSrc[2] <= 'F' )
				c += ( pSrc[2] - 'A' + 10 );
			else if( pSrc[2] >= 'a' && pSrc[2] <= 'f' )
				c += ( pSrc[2] - 'a' + 10 );
			else
				c += ( pSrc[2] - '0' );
			pSrc += 3;
		}
		else if ( *pSrc == '+' )
		{
			c = ' ';
			++pSrc;
		}
		else
		{
			c = *pSrc;
			++pSrc;
		}

		strDecode += c;
		++cbDest;
	}

	strAnsiData = strDecode;
	return true;
}
string CSkyChaserHttp::_base64DecodeRaw( const char *b, const char *e )
{
	static BYTE dec64[] =
	{
		/* 0x */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 1x */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 2x */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3E, 0xFF, 0xFF, 0xFF, 0x3F,
		/* 3x */0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 4x */0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
		/* 5x */0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 6x */0xFF, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
		/* 7x */0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 8x */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* 9x */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Ax */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Bx */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Cx */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Dx */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Ex */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		/* Fx */0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};
	string out;
	BYTE c[4];
	int pos = 0;
	for(; b < e; b++)
		if((BYTE)*b > ' ') {
			BYTE ch = dec64[(BYTE)*b];
			if(ch & 0xC0)
				break;
			c[pos++] = ch;
			if(pos == 4) {
				out.append(1, (c[0] << 2) | (c[1] >> 4));
				out.append(1, (c[1] << 4) | (c[2] >> 2));
				out.append(1, (c[2] << 6) | (c[3] >> 0));
				pos = 0;
			}
		}
		if(pos >= 2) {
			out.append(1, (c[0] << 2) | (c[1] >> 4));
			if(pos >= 3) {
				out.append(1, (c[1] << 4) | (c[2] >> 2));
				if(pos >= 4)
					out.append(1, (c[2] << 6) | (c[3] >> 0));
			}
		}
		return out;
}

string CSkyChaserHttp::_base64EncodeRaw( const char *b, const char *e )
{
	static const char encoder[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
	if(b == e)
		return NULL;
	int out = (int(e - b) + 2) / 3 * 4;
	int rem = int(e - b) % 3;
	e -= rem;

	//	StringBuffer s(out);
	char* s = (char*)malloc(out);
	memset(s, 0, out);


	char *p = s;
	while(b < e)
	{
		p[0] = encoder[(b[0] >> 2) & 0x3F];
		p[1] = encoder[((b[0] << 4) & 0x30) | ((b[1] >> 4) & 0x0F)];
		p[2] = encoder[((b[1] << 2) & 0x3C) | ((b[2] >> 6) & 0x03)];
		p[3] = encoder[b[2] & 0x3F];
		b += 3;
		p += 4;
	}
	if(rem == 1)
	{
		p[0] = encoder[(b[0] >> 2) & 0x3F];
		p[1] = encoder[(b[0] << 4) & 0x30];
		p[2] = p[3] = '=';
	}
	else if(rem == 2)
	{
		p[0] = encoder[(b[0] >> 2) & 0x3F];
		p[1] = encoder[((b[0] << 4) & 0x30) | ((b[1] >> 4) & 0x0F)];
		p[2] = encoder[(b[1] << 2) & 0x3C];
		p[3] = '=';
	}

	string str(s,out);
	free(s);

	return str;
}

string CSkyChaserHttp::sc_base64Decode( const char *s, int len )
{
	if ( s == NULL )
		return "";
	return _base64DecodeRaw( s, s + len );
}

string CSkyChaserHttp::sc_base64Encode( const char *b, int len )
{
	if ( b == NULL )
		return "";
	if ( len == 0 )
		return "";

	return _base64EncodeRaw( b, b + len );
}