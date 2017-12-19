
#include "bbqbase.h"

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>

//#define USE_WINCRYPT

#ifdef _WIN32
#include <wincrypt.h>
#if P_SSL
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif
#ifndef OPENSSL_NO_RSA
#include <openssl/rsa.h>
#endif

#endif
#else
#define P_SSL 1
#if P_SSL
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif
#ifndef OPENSSL_NO_RSA
#include <openssl/rsa.h>
#endif

#endif
#endif




typedef const char* LPCTSTR;

//#include <ptclib/mime.h>

// must init openssl functions
void cert_rsa_aes_init( void )
{
	static int inited = 0;
	if( ! inited ) {
#if P_SSL
		OPENSSL_add_all_algorithms_noconf();
#endif
		inited = 1;
	}
}

void cert_rsa_aes_cleanup( void )
{
#if P_SSL
	ENGINE_cleanup();
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
	ERR_remove_state(0);
	EVP_cleanup();
#endif
}

// malloc, need free(str)
char * ReadTextFromFile( const char * pzFilePath, int maxlen )
{
	char * str = NULL;

	FILE * fp = fopen( pzFilePath, "rb" );
	if( fp ) {
		fseek( fp, 0, SEEK_END );
		int nLen = ftell( fp );

		nLen = min( nLen, maxlen );
		str = (char *) malloc( nLen +1 );
		if( str ) {
			fseek( fp, 0, SEEK_SET );
			int nRead = fread( str, sizeof(char), nLen, fp );
			if( nRead >=0 ) str[ nRead ] = 0;
		}

		fclose( fp );
	}

	return str;
}


#define HEX_CHAR(x)  (((x)>9)?('A'+((x)-10)):((x)+'0'))
#define VALUE_HEX(x) (((x)>'9')?(((x)>'Z')?(10+((x)-'a')):(10+((x)-'A'))):((x)-'0')) 

int bin_to_ascii( const unsigned char * data, int length, unsigned char * out )
{
	const unsigned char * p = data;
	unsigned char * q = out;
	unsigned char hi, low;
	int i;
	for( i=0; i<length; i++ ) {
		hi = ((*p) & 0xf0) >> 4;
		low = (*p) & 0x0f;
		p ++;
		* q = HEX_CHAR(hi); q ++;
		* q = HEX_CHAR(low); q ++;
	}
	* q = '\0';

	return i;
}

int ascii_to_bin( const char * ascii, unsigned char * out )
{
	int length = strlen( ascii ) >> 1;
	const unsigned char * p = (const unsigned char *) ascii;
	unsigned char * q = out;
	unsigned char hi, low;
	int i;
	for( i=0; i<length; i++ ) {
		if( *p > 'z' ) {
			return 0; // bad string
		}
		hi = VALUE_HEX(* p); p++;
		if( *p > 'z' ) {
			return 0; // bad string
		}
		low = VALUE_HEX(* p); p++;
		* q = ((hi & 0x0f) << 4) | (low & 0x0f);
		q ++;
	}
	return i;
}

//---------------- AES encryption & decryption ---------------------
// encrypt data into out
// Input: data, length, key, iv. Key and iv are 128 or 256 bit. iv need not be secret, but must be unique.
// Output: out
bool AES_encrypt(const unsigned char * data, int length, const unsigned char * key, const unsigned char * iv, int keylen, unsigned char * out)
{
	if( length == 0 ) return true;
#if P_SSL
	EVP_CIPHER_CTX		ctx;
	int					outlength;

	EVP_CIPHER_CTX_init(&ctx);

	if( keylen == 128 ) {
		if ( EVP_EncryptInit(&ctx, EVP_aes_128_cfb(), key, iv) != 1 ||
			EVP_EncryptUpdate(&ctx, out, &outlength, data, length) != 1 ||
			EVP_EncryptFinal(&ctx, out, &outlength) != 1 )
			return false;
	} else if ( keylen == 256 ) {
		if ( EVP_EncryptInit(&ctx, EVP_aes_256_cfb(), key, iv) != 1 ||
			EVP_EncryptUpdate(&ctx, out, &outlength, data, length) != 1 ||
			EVP_EncryptFinal(&ctx, out, &outlength) != 1 )
			return false;
	} else 
		return false;

	EVP_CIPHER_CTX_cleanup(&ctx);
	return true;
#else
	return false;
#endif
}

// decrypt data into out.
// Input: data, length, key, iv. Key and iv are 128 or 256 bit. iv need not be secret, but must be unique.
// Output: out
bool AES_decrypt(const unsigned char * data, int length, const unsigned char * key, const unsigned char * iv, int keylen, unsigned char * out)
{
	if( length == 0 ) return true;
#if P_SSL
	EVP_CIPHER_CTX		ctx;
	int					outlength;

	EVP_CIPHER_CTX_init(&ctx);

	if( keylen == 128 ) {
		if ( EVP_DecryptInit(&ctx, EVP_aes_128_cfb(), key, iv) != 1 ||
			EVP_DecryptUpdate(&ctx, out, &outlength, data, length) != 1 ||
			EVP_DecryptFinal(&ctx, out, &outlength) != 1 )
			return false;
	} else if ( keylen == 256 ) {
		if ( EVP_DecryptInit(&ctx, EVP_aes_256_cfb(), key, iv) != 1 ||
			EVP_DecryptUpdate(&ctx, out, &outlength, data, length) != 1 ||
			EVP_DecryptFinal(&ctx, out, &outlength) != 1 )
			return false;
	} else return false;

	EVP_CIPHER_CTX_cleanup(&ctx);
	return true;
#else
	return false;
#endif
}

//---------------- RSA ---------------------
// encrypt use public key.
// return outdata length. return -1 on error.
int PKI_rsa_public_encrypt( const void * cert, const char * indata, int inlen, char * outdata)
{
	int eout = -1;

#if P_SSL
	EVP_PKEY * pkey = X509_get_pubkey( (X509 *) cert);
	if (pkey == NULL)
		return -1;

	RSA * rsa = EVP_PKEY_get1_RSA(pkey);
	eout = RSA_public_encrypt(inlen, (const unsigned char *)indata, (unsigned char *)outdata, rsa, RSA_PKCS1_OAEP_PADDING);
	RSA_free(rsa);
	EVP_PKEY_free(pkey);
#endif

	return eout;
}

// decrypt with public key.
// return data length, return -1 on error.
int PKI_rsa_public_decrypt( const void * cert, const char * indata, int inlen, char * outdata)
{
	int dout = -1;
  
#if P_SSL
	EVP_PKEY * pkey = X509_get_pubkey( (X509 *) cert);
	if (pkey == NULL)
		return -1;
 
	RSA * rsa = EVP_PKEY_get1_RSA(pkey);
	
	//the outdata length must more than RSA_size(rsa) -11 
	dout = RSA_public_decrypt(inlen, (const unsigned char *)indata, (unsigned char *)outdata, rsa, RSA_PKCS1_PADDING);
	RSA_free(rsa);
	EVP_PKEY_free(pkey);
#endif

	return dout;
}

// decrypt with private key.
// return data length, return -1 on error.
int PKI_rsa_private_decrypt( const void * prikey, const char * indata, int inlen, char * outdata)
{
	int dout = -1;

#if P_SSL
	RSA * privatersa = EVP_PKEY_get1_RSA( (EVP_PKEY *) prikey );
	dout = RSA_private_decrypt(inlen, (const unsigned char *)indata, (unsigned char *)outdata, privatersa, RSA_PKCS1_OAEP_PADDING);
	RSA_free(privatersa);
#endif

	return dout;
}
// encrypt with private key.
// return data length, return -1 on error.
int PKI_rsa_private_encrypt(const  void * prikey, const char * indata, int inlen, char * outdata)
{
	int dout = -1;

#if P_SSL
	RSA * privatersa = EVP_PKEY_get1_RSA( (EVP_PKEY *) prikey );
	//note: if the inlen more than RSASIZE , the inlen equal to RSASIZE-11
	dout = RSA_private_encrypt(inlen, (const unsigned char *)indata, (unsigned char *)outdata, privatersa, RSA_PKCS1_PADDING);
	
	RSA_free(privatersa);
#endif

	return dout;
}

//---------------- load PKI cert & key ---------------------

void * PKI_PEM_to_X509( const char * buf, int len )
{
#if P_SSL
	BIO * bio = BIO_new_mem_buf((void*)buf,len);
	X509 * cert = PEM_read_bio_X509(bio,NULL,NULL, NULL);
	//BIO_set_close(bio, BIO_CLOSE);
	BIO_free(bio);
	return (void *)cert;
#else
	return NULL;
#endif
}

void * PKI_PEM_to_private_key( const char * buf, int len, const char * pass )
{
#if P_SSL
	//PW_CB_DATA cb_data;
	//cb_data.password = pass;
	//cb_data.prompt_info = "PrivateKey";
	BIO * bio = BIO_new_mem_buf((void*)buf,len);
	EVP_PKEY * privkey = (EVP_PKEY *) PEM_read_bio_PrivateKey( bio, NULL, NULL, (void*) pass );
	//BIO_set_close(bio, BIO_CLOSE);
	BIO_free(bio);
	return (void *) privkey;
#else
	return NULL;
#endif

}

char * X509_to_text( void * x509 )
{
	char * text = NULL;
#if P_SSL

	BIO *mem = BIO_new(BIO_s_mem());
	if( X509_print( mem, (X509 *) x509 ) ) {
		BUF_MEM *bptr;
		BIO_get_mem_ptr(mem, &bptr);
		text = (char *) malloc( bptr->length + 1 );
		if( text ) {
			memcpy( text, bptr->data, bptr->length );
			text[ bptr->length ] = '\0';
		}
	}
	BIO_free(mem);
#endif
	return text;
}

bool PKI_check_X509_privkey( void * cert, void * privkey )
{
#if P_SSL
	if( ! X509_check_private_key( (X509 *) cert, (EVP_PKEY *) privkey ) ) 
		return false;
	
	return true;
#else
	return false;
#endif
}

void PKI_free_cert( void * cert )
{
#if P_SSL
	if( cert ) X509_free( (X509 *) cert );
#endif
}

void PKI_free_key( void * privkey )
{
#if P_SSL
	if( privkey ) EVP_PKEY_free( (EVP_PKEY *) privkey );
#endif
}

static const char STR_KEYKEY[] = "3.141592653589793238462643383279";
static const char STR_KEY_IV[] = "12345678901234567890123456789012";

PString AES_encrypt_pin( const char * pin_string )

{
	char enc_string[ 256 ] = "";
	int n = strlen(pin_string);
	if( n > 64 ) return false;

	unsigned char enc[ 256 ];
	if( AES_encrypt( (unsigned char *)pin_string, n, (const unsigned char *) STR_KEYKEY, (const unsigned char *)  STR_KEY_IV, 256, & enc[8] ) ) {
		for( int i=0; i<8; i++ ) { 
			unsigned char c = 0x13;
			for( int j=8+n-1; j>=i; j-- ) {
				c += enc[8+j]; 
			}
			enc[i] = c;
		};
		n = bin_to_ascii( enc, 8 + strlen(pin_string), (unsigned char *) enc_string );
	}
	return PString( enc_string );
}

PString AES_decrypt_pin( const char * enc_string )
{
	char pin_string[ 256 ] = "";
	int n = strlen( enc_string );
	if( (n < 256) && (n>8) ) {
		unsigned char enc[ 256 ];
		n = ascii_to_bin( enc_string, enc ) - 8;
		if( n > 0 ) {
			if( AES_decrypt( & enc[8], n, (const unsigned char *) STR_KEYKEY, (const unsigned char *)  STR_KEY_IV, 256, (unsigned char *) pin_string ) ) {
				pin_string[ n ] = '\0';
			}
		}
	}
	return PString( pin_string );
}

//#define PEM_read_PrivateKey(fp,x,cb,u) \
//	(EVP_PKEY *)PEM_ASN1_read( (char *(*)())d2i_PrivateKey,PEM_STRING_EVP_PKEY,fp,(char **)x,cb,u)


//
// test program
//
/*
int main( int argc, char* argv[] )
{
	BIO *bio_err = NULL;

	ENGINE *e = NULL;
	EVP_PKEY *pkey = NULL;
	X509 *x509 = NULL, *x509p = NULL;

	const char *keyfile_key = "secret";
	const char *keyfile = "ca_privkey.pem";
	const char *certfile = "ca_cert.pem";

	if (!X509_check_private_key(x509,pkey))
	{
		BIO_printf(bio_err,"CA certificate and CA private key do not match\n");
		goto err;
	}

err:
	if(pkey) EVP_PKEY_free(pkey);
	if(x509) X509_free(x509);

	return 0;
}
*/

//---------------- base64 encode ---------------------
// encode data into out
// Input: in, inl ,out ,out_max
// Output: out
int PKI_encode(const unsigned char * in, int inl, unsigned char * out)
{
	if (!in || !out || inl <=0)
		return -2;
#if P_SSL
		EVP_ENCODE_CTX     ectx;	
		int total=0, outl=0;
		EVP_EncodeInit(&ectx);
		EVP_EncodeUpdate(&ectx, out, &outl, (unsigned char*)in, inl);
		total+=outl;
		EVP_EncodeFinal(&ectx, out+total, &outl);
		total+=outl;
		
		//printf("%s\n[%d]",out, total);

	return total;
#else
	return -1;
#endif
}

//---------------- base64 decode ---------------------
// decode data into out
// Input: in, inl ,out ,out_max
// Output: out
int PKI_decode(const unsigned char * in, int inl, unsigned char * out)
{
	if (!in || !out || inl <=0)
		return -2;
#if P_SSL
		int total=0, outl=0;
		EVP_ENCODE_CTX     dctx;	
		EVP_DecodeInit(&dctx);
		if(EVP_DecodeUpdate(&dctx,out, &outl,(unsigned char*)in, inl) < 0)
		{
			printf("EVP_DecodeUpdate err!\n");
			return -1;
		}		
		total+=outl;
		if (EVP_DecodeFinal(&dctx, out + total, &outl)<0)
		{
			printf("EVP_DecodeFinal err!\n");
			return -1;
		}
		total+=outl;

		//printf("%s\n[%d]",out, total);

	return total;
#else
	return -1;
#endif
}

bool VerifyCert(void* peerCertificate)
{
#if P_SSL
	bool bRet = false;
	int i = 0;
	X509_STORE *store = NULL;
	X509_LOOKUP *lookup = NULL;
	X509_STORE_CTX * csc = NULL;

	if(!(store = X509_STORE_new())) goto end;
	lookup = X509_STORE_add_lookup(store,X509_LOOKUP_file());
	if (lookup == NULL) goto end;
	X509_LOOKUP_init(lookup);
	X509_LOOKUP_load_file(lookup,NULL,X509_FILETYPE_DEFAULT);
	X509_LOOKUP_shutdown(lookup);

	lookup=X509_STORE_add_lookup(store,X509_LOOKUP_hash_dir());
	if (lookup == NULL) goto end;
	X509_LOOKUP_init(lookup);
	X509_LOOKUP_add_dir(lookup,NULL,X509_FILETYPE_DEFAULT);
	X509_LOOKUP_shutdown(lookup);

	csc = X509_STORE_CTX_new();
	if(!X509_STORE_CTX_init(csc, store ,(X509*)peerCertificate,NULL))
		goto end;

	//开始校�?
	X509_STORE_set_flags(store,X509_V_FLAG_USE_CHECK_TIME | X509_V_FLAG_CRL_CHECK_ALL);

	i = X509_verify_cert(csc);

	PTRACE(0, "error %d at %d depth lookup:%s\n" << csc->error << 
			csc->error_depth <<
			X509_verify_cert_error_string(csc->error));

	bRet = (i != 0);

	ERR_clear_error();
end:
	if( csc != NULL ) X509_STORE_CTX_free(csc);
	if( store != NULL ) X509_STORE_free(store);
	return bRet;
#else
	return true;
#endif
}

bool VerifyCertHostName(void* peerCertificate, const char* hostname)
{
	VerifyCert(peerCertificate);
#if P_SSL
	char commonName [512];
	X509_NAME * name = X509_get_subject_name((X509*)peerCertificate);
	X509_NAME_get_text_by_NID(name, NID_commonName, commonName, 512);

	/* More in-depth checks of the common name can be used if necessary */

	if(strcmp(commonName, hostname) != 0)
	{
		/* Handle a suspect certificate here */
		return false;
	}
	
	
	return true;
#else
	return true;
#endif
}

void SaveCertToFile(const char* pszFile, void* pszContent)
{
	FILE * fp = fopen( pszFile, "wb+" );
	if( fp ) {
#if P_SSL
		i2d_X509_fp(fp,  (X509*)pszContent);
#endif
		fclose( fp );
	}
}

#ifdef USE_WINCRYPT

DWORD VerifyServerCertificate(
    PCCERT_CONTEXT  pServerCert,
    PSTR            pszServerName,
    DWORD           dwCertFlags)
{
    HTTPSPolicyCallbackData  polHttps;
    CERT_CHAIN_POLICY_PARA   PolicyPara;
    CERT_CHAIN_POLICY_STATUS PolicyStatus;
    CERT_CHAIN_PARA          ChainPara;
    PCCERT_CHAIN_CONTEXT     pChainContext = NULL;

    LPSTR rgszUsages[] = {  szOID_PKIX_KP_SERVER_AUTH,
                            szOID_SERVER_GATED_CRYPTO,
                            szOID_SGC_NETSCAPE };
    DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);

    PWSTR   pwszServerName = NULL;
    DWORD   cchServerName;
    DWORD   Status;

    if(pServerCert == NULL)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }


    //
    // Convert server name to unicode.
    //

    if(pszServerName == NULL || strlen(pszServerName) == 0)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }

    cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, NULL, 0);
    pwszServerName = (PWSTR)LocalAlloc(LMEM_FIXED, cchServerName * sizeof(WCHAR));
    if(pwszServerName == NULL)
    {
        Status = SEC_E_INSUFFICIENT_MEMORY;
        goto cleanup;
    }
    cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, pwszServerName, cchServerName);
    if(cchServerName == 0)
    {
        Status = SEC_E_WRONG_PRINCIPAL;
        goto cleanup;
    }


    //
    // Build certificate chain.
    //

    ZeroMemory(&ChainPara, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier     = cUsages;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

    if(!CertGetCertificateChain(
                            NULL,
                            pServerCert,
                            NULL,
                            pServerCert->hCertStore,
                            &ChainPara,
                            0,
                            NULL,
                            &pChainContext))
    {
        Status = GetLastError();
        printf("Error 0x%x returned by CertGetCertificateChain!\n", Status);
        goto cleanup;
    }


    //
    // Validate certificate chain.
    // 

    ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
    polHttps.cbStruct           = sizeof(HTTPSPolicyCallbackData);
    polHttps.dwAuthType         = AUTHTYPE_SERVER;
    polHttps.fdwChecks          = dwCertFlags;
    polHttps.pwszServerName     = pwszServerName;

    memset(&PolicyPara, 0, sizeof(PolicyPara));
    PolicyPara.cbSize            = sizeof(PolicyPara);
    PolicyPara.pvExtraPolicyPara = &polHttps;

    memset(&PolicyStatus, 0, sizeof(PolicyStatus));
    PolicyStatus.cbSize = sizeof(PolicyStatus);

    if(!CertVerifyCertificateChainPolicy(
                            CERT_CHAIN_POLICY_SSL,
                            pChainContext,
                            &PolicyPara,
                            &PolicyStatus))
    {
        Status = GetLastError();
        printf("Error 0x%x returned by CertVerifyCertificateChainPolicy!\n", Status);
        goto cleanup;
    }

    if(PolicyStatus.dwError)
    {
        Status = PolicyStatus.dwError;
        //DisplayWinVerifyTrustError(Status); 
        goto cleanup;
    }


    Status = SEC_E_OK;

	printf("CertVerifyCertificateChainPolicy ok. \n");

cleanup:

    if(pChainContext)
    {
        CertFreeCertificateChain(pChainContext);
    }

    if(pwszServerName)
    {
        LocalFree(pwszServerName);
    }

    return Status;
}

#endif



static PBYTEArray encrypt(const PBYTEArray& byteArray, LPCTSTR lpszKey)
{
	PBYTEArray strResult;
	BYTE * out = strResult.GetPointer(byteArray.GetSize());
	for( int i= 0; i < byteArray.GetSize(); i++) {
		char c1 = byteArray[i];
		int nIndex = i % strlen(lpszKey) - 1;
		if( nIndex < 0 ) {
			nIndex = strlen(lpszKey) + nIndex;
		}
		char c2 = lpszKey[ nIndex];
		char c = c1 + c2;
		strResult[i] = c;
	}
	return strResult;
}

static PBYTEArray decrypt(const PBYTEArray& byteArray, LPCTSTR lpszKey)
{
	PBYTEArray strResult;
	BYTE * out = strResult.GetPointer(byteArray.GetSize());
	for( int i= 0; i < byteArray.GetSize(); i++) {
		char c1 = byteArray[i];
		int nIndex = i % strlen(lpszKey) - 1;
		if( nIndex < 0 ) {
			nIndex = strlen(lpszKey) + nIndex;
		}
		char c2 = lpszKey[ nIndex ];
		char c = c1 - c2;
		strResult[i] = c;
	}
	return strResult;
}

PString smsencrypt(LPCTSTR lpszString, LPCTSTR lpszKey, LPCTSTR lpszSeed1, LPCTSTR lpszSeed2)
{
/*#ifdef _WIN32
	PBYTEArray strResult((BYTE*)lpszString, strlen(lpszString));
	strResult = encrypt(strResult, lpszSeed1);
	strResult = encrypt(strResult, lpszKey);
	strResult = encrypt(strResult, lpszSeed2);
	PString strRet = PBase64::Encode(strResult);
	strRet.Replace("\r", "", TRUE);
	strRet.Replace("\n", "", TRUE);
	return strRet;
#else*/
	return "";
//#endif
}

PString smsdecrypt(LPCTSTR lpszString, LPCTSTR lpszKey, LPCTSTR lpszSeed1, LPCTSTR lpszSeed2)
{
/*#ifdef _WIN32
	PBYTEArray strResult;
	PBase64::Decode(lpszString, strResult);
	strResult = decrypt(strResult, lpszSeed2);
	strResult = decrypt(strResult, lpszKey);
	strResult = decrypt(strResult, lpszSeed1);
	return PString((const char *)(const BYTE *)strResult, strResult.GetSize());
#else*/
	return "";
//#endif
}


int VerifyServerCert(void* peerCertificate, const char* hostname)
{
#if P_SSL

#ifdef USE_WINCRYPT
	BYTE* pszContent = NULL;//OPENSSL_malloc(nLen);
	int nLen = i2d_X509((X509*)peerCertificate, &pszContent);
	PCCERT_CONTEXT  pTargetCert = CertCreateCertificateContext(X509_ASN_ENCODING, (BYTE*)pszContent, nLen);
	OPENSSL_free(pszContent);
	if( pTargetCert != NULL ) {
		int nStatus = VerifyServerCertificate(pTargetCert, (PSTR)hostname, 0);
		CertFreeCertificateContext(pTargetCert);
		return nStatus;
	} else {
		return ERROR_INVALID_HANDLE;
	}
#else
	return 0;
#endif

#else
	return 0;
#endif
}

