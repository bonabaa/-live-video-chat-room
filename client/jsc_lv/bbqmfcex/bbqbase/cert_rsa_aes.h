
#ifndef _CERT_AES_RSA_H_
#define _CERT_AES_RSA_H_

// must init openssl functions
extern void cert_rsa_aes_init( void );

extern void cert_rsa_aes_cleanup( void );

extern char * ReadTextFromFile( const char * pzFilePath, int maxlen );

// convert binary data block to hex ascii string
extern int bin_to_ascii( const unsigned char * data, int length, unsigned char * out );
// convert hex ascii string to binary data block
extern int ascii_to_bin( const char * ascii, unsigned char * out );

// convert pin into encrypted form
extern PString AES_encrypt_pin( const char * pin_string );
// convert encrypted pin into plain form
extern PString AES_decrypt_pin( const char * enc_string );

// encrypt data into out
// Input: data, length, key, iv. Key and iv are 128 or 256 bit. iv need not be secret, but must be unique.
// Output: out
extern bool AES_encrypt(const unsigned char * data, int length, const unsigned char * key, const unsigned char * iv, int keylen, unsigned char * out);

// decrypt data into out.
// Input: data, length, key, iv. Key and iv are 128 or 256 bit. iv need not be secret, but must be unique.
// Output: out
extern bool AES_decrypt(const unsigned char * data, int length, const unsigned char * key, const unsigned char * iv, int keylen, unsigned char * out);

// encrypt use public key.
// return outdata length. return -1 on error.
extern int PKI_rsa_public_encrypt( const void * cert, const char * indata, int inlen, char * outdata);

// decrypt with private key.
// return data length, return -1 on error.
extern int PKI_rsa_private_decrypt( const void * prikey, const char * indata, int inlen, char * outdata);

// convert PEM format cert to X509 object
extern void * PKI_PEM_to_X509( const char * buf, int len );

// convert PEM format private key to key object
extern void * PKI_PEM_to_private_key( const char * buf, int len, const char * pass );

// verify x509 and private key match or not
extern bool PKI_check_X509_privkey( void * cert, void * privkey );

// convert x509 to human readable text, call free() after use
extern char * X509_to_text( void * x509 );

// free the resource 
extern void PKI_free_cert( void * cert );

extern void PKI_free_key( void * privkey );
// encrypt with public key.
// return data length, return -1 on error.
extern int PKI_rsa_private_encrypt( const void * prikey, const char * indata, int inlen, char * outdata);
// decrypt with public key.
// return data length, return -1 on error.
extern int PKI_rsa_public_decrypt( const void * cert, const char * indata, int inlen, char * outdata);
// base64 encode
extern int PKI_encode(const unsigned char * in, int inl, unsigned char * out);
// base64 decode
extern int PKI_decode(const unsigned char * in, int inl, unsigned char * out);
//extern PString smsencrypt(LPCTSTR lpszString, LPCTSTR lpszKey, LPCTSTR lpszSeed1, LPCTSTR lpszSeed2);
//extern PString smsdecrypt(LPCTSTR lpszString, LPCTSTR lpszKey, LPCTSTR lpszSeed1, LPCTSTR lpszSeed2);

#endif // _CERT_AES_RSA_H_
