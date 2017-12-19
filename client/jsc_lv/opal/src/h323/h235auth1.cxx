/*
 * h235auth1.cxx
 *
 * H.235 security PDU's
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): Fürbass Franz <franz.fuerbass@infonova.at>
 *
 * $Revision: 22444 $
 * $Author: rjongbloed $
 * $Date: 2009-04-20 23:49:06 +0000 (Mon, 20 Apr 2009) $
 */

#include <ptlib.h>

#include <opal/buildopts.h>
#if OPAL_PTLIB_SSL

#if OPAL_H323

#include <openssl/sha.h>

#include <h323/h235auth.h>
#include <h323/h323pdu.h>


#ifdef _MSC_VER
#pragma comment(lib, P_SSL_LIB1)
#pragma comment(lib, P_SSL_LIB2)
#endif


#define REPLY_BUFFER_SIZE 1024


static const char OID_A[] = "0.0.8.235.0.2.1";
static const char OID_T[] = "0.0.8.235.0.2.5";
static const char OID_U[] = "0.0.8.235.0.2.6";

#define OID_VERSION_OFFSET 5

#define HASH_SIZE 12

static const BYTE SearchPattern[HASH_SIZE] = { // Must be 12 bytes
  't', 'W', 'e', 'l', 'V', 'e', '~', 'b', 'y', 't', 'e', 'S'
};

#ifndef SHA_DIGESTSIZE
#define SHA_DIGESTSIZE  20
#endif

#ifndef SHA_BLOCKSIZE
#define SHA_BLOCKSIZE   64
#endif


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

/* Function to print the digest */
#if 0
static void pr_sha(FILE* fp, char* s, int t)
{
        int     i ;

        fprintf(fp, "0x") ;
        for (i = 0 ; i < t ; i++)
                fprintf(fp, "%02x", s[i]) ;
        fprintf(fp, "0x") ;
}
#endif

static void truncate(unsigned char*    d1,    /* data to be truncated */
                     char*             d2,    /* truncated data */
                     int               len)   /* length in bytes to keep */
{
        int     i ;
        for (i = 0 ; i < len ; i++) d2[i] = d1[i];
}

/* Function to compute the digest */
static void hmac_sha (const unsigned char*    k,      /* secret key */
                      int      lk,              /* length of the key in bytes */
                      const unsigned char*    d,      /* data */
                      int      ld,              /* length of data in bytes */
                      char*    out,             /* output buffer, at least "t" bytes */
                      int      t)
{
        SHA_CTX ictx, octx ;
        unsigned char    isha[SHA_DIGESTSIZE], osha[SHA_DIGESTSIZE] ;
        unsigned char    key[SHA_DIGESTSIZE] ;
        char    buf[SHA_BLOCKSIZE] ;
        int     i ;

        if (lk > SHA_BLOCKSIZE) {

                SHA_CTX         tctx ;

                SHA1_Init(&tctx) ;
                SHA1_Update(&tctx, k, lk) ;
                SHA1_Final(key, &tctx) ;

                k = key ;
                lk = SHA_DIGESTSIZE ;
        }

        /**** Inner Digest ****/

        SHA1_Init(&ictx) ;

        /* Pad the key for inner digest */
        for (i = 0 ; i < lk ; ++i) buf[i] = (char)(k[i] ^ 0x36);
        for (i = lk ; i < SHA_BLOCKSIZE ; ++i) buf[i] = 0x36;

        SHA1_Update(&ictx, buf, SHA_BLOCKSIZE) ;
        SHA1_Update(&ictx, d, ld) ;

        SHA1_Final(isha, &ictx) ;

        /**** Outter Digest ****/

        SHA1_Init(&octx) ;

        /* Pad the key for outter digest */

        for (i = 0 ; i < lk ; ++i) buf[i] = (char)(k[i] ^ 0x5C);
        for (i = lk ; i < SHA_BLOCKSIZE ; ++i) buf[i] = 0x5C;

        SHA1_Update(&octx, buf, SHA_BLOCKSIZE) ;
        SHA1_Update(&octx, isha, SHA_DIGESTSIZE) ;

        SHA1_Final(osha, &octx) ;

        /* truncate and print the results */
        t = t > SHA_DIGESTSIZE ? SHA_DIGESTSIZE : t ;
        truncate(osha, out, t) ;

}


/////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PFactory<H235Authenticator>, H235AuthProcedure1, "H235Procedure1", false);

H235AuthProcedure1::H235AuthProcedure1()
{
}


PObject * H235AuthProcedure1::Clone() const
{
  H235AuthProcedure1 * auth = new H235AuthProcedure1(*this);

  // We do NOT copy these fields in Clone()
  auth->lastRandomSequenceNumber = 0;
  auth->lastTimestamp = 0;

  return auth;
}


const char * H235AuthProcedure1::GetName() const
{
  return "H235AnnexD_Procedure1";
}


H225_CryptoH323Token * H235AuthProcedure1::CreateCryptoToken()
{
  if (!IsActive())
    return NULL;

  H225_CryptoH323Token * cryptoToken = new H225_CryptoH323Token;

  // Create the H.225 crypto token in the H323 crypto token
  cryptoToken->SetTag(H225_CryptoH323Token::e_nestedcryptoToken);
  H235_CryptoToken & nestedCryptoToken = *cryptoToken;

  // We are doing hashed password
  nestedCryptoToken.SetTag(H235_CryptoToken::e_cryptoHashedToken);
  H235_CryptoToken_cryptoHashedToken & cryptoHashedToken = nestedCryptoToken;

  // tokenOID = "A"
  cryptoHashedToken.m_tokenOID = OID_A;
  
  //ClearToken
  H235_ClearToken & clearToken = cryptoHashedToken.m_hashedVals;
  
  // tokenOID = "T"
  clearToken.m_tokenOID  = OID_T;
  
  if (!remoteId) {
    clearToken.IncludeOptionalField(H235_ClearToken::e_generalID);
    clearToken.m_generalID = remoteId;
  }

  if (!localId) {
    clearToken.IncludeOptionalField(H235_ClearToken::e_sendersID);
    clearToken.m_sendersID = localId;
  }
  
  clearToken.IncludeOptionalField(H235_ClearToken::e_timeStamp);
  clearToken.m_timeStamp = (int)PTime().GetTimeInSeconds();

  clearToken.IncludeOptionalField(H235_ClearToken::e_random);
  clearToken.m_random = ++sentRandomSequenceNumber;

  //H235_HASHED
  H235_HASHED<H235_EncodedGeneralToken> & encodedToken = cryptoHashedToken.m_token;
  
  //  algorithmOID = "U"
  encodedToken.m_algorithmOID = OID_U;


  /*******
   * step 1
   *
   * set a pattern for the hash value
   *
   */

  encodedToken.m_hash.SetData(HASH_SIZE*8, SearchPattern);
  return cryptoToken;
}


PBoolean H235AuthProcedure1::Finalise(PBYTEArray & rawPDU)
{
  if (!IsActive())
    return PFalse;

  // Find the pattern

  int foundat = -1;
  for (PINDEX i = 0; i <= rawPDU.GetSize() - HASH_SIZE; i++) {
    if (memcmp(&rawPDU[i], SearchPattern, HASH_SIZE) == 0) { // i'v found it !
      foundat = i;
      break;
    }
  }
  
  if (foundat == -1) {
    //Can't find the search pattern in the ASN1 packet.
    PTRACE(1, "H235RAS\tPDU not prepared for H235AuthProcedure1");
    return PFalse;
  }
  
  // Zero out the search pattern
  memset(&rawPDU[foundat], 0, HASH_SIZE);

 /*******
  * 
  * generate a HMAC-SHA1 key over the hole message
  * and save it in at (step 3) located position.
  * in the asn1 packet.
  */
  
  char key[HASH_SIZE];
 
  /** make a SHA1 hash before send to the hmac_sha1 */
  unsigned char secretkey[20];
  
  SHA1((unsigned char *)password.GetPointer(), password.GetSize()-1, secretkey);

  hmac_sha(secretkey, 20, rawPDU.GetPointer(), rawPDU.GetSize(), key, HASH_SIZE);
  
  memcpy(&rawPDU[foundat], key, HASH_SIZE);
  
  PTRACE(4, "H235RAS\tH235AuthProcedure1 hashing completed: \"" << password << '"');
  return PTrue;
}


static PBoolean CheckOID(const PASN_ObjectId & oid1, const PASN_ObjectId & oid2)
{
  if (oid1.GetSize() != oid2.GetSize())
    return PFalse;

  PINDEX i;
  for (i = 0; i < OID_VERSION_OFFSET; i++) {
    if (oid1[i] != oid2[i])
      return PFalse;
  }

  for (i++; i < oid1.GetSize(); i++) {
    if (oid1[i] != oid2[i])
      return PFalse;
  }

  return PTrue;
}


H235Authenticator::ValidationResult H235AuthProcedure1::ValidateCryptoToken(
                                            const H225_CryptoH323Token & cryptoToken,
                                            const PBYTEArray & rawPDU)
{
  //verify the token is of correct type
  if (cryptoToken.GetTag() != H225_CryptoH323Token::e_nestedcryptoToken) {
    PTRACE(4, "H235\tNo nested crypto token!");
    return e_Absent;
  }
  
  const H235_CryptoToken & crNested = cryptoToken;
  if (crNested.GetTag() != H235_CryptoToken::e_cryptoHashedToken) {
    PTRACE(4, "H235\tNo crypto hash token!");
    return e_Absent;
  }
  
  const H235_CryptoToken_cryptoHashedToken & crHashed = crNested;
  
  //verify the crypto OIDs
  
  // "A" indicates that the whole messages is used for authentication.
  if (!CheckOID(crHashed.m_tokenOID, OID_A)) {
    PTRACE(2, "H235RAS\tH235AuthProcedure1 requires all fields are hashed, got OID " << crHashed.m_tokenOID);
    return e_Absent;
  }
  
  // "T" indicates that the hashed token of the CryptoToken is used for authentication.
  if (!CheckOID(crHashed.m_hashedVals.m_tokenOID, OID_T)) {
    PTRACE(2, "H235RAS\tH235AuthProcedure1 requires ClearToken, got OID " << crHashed.m_hashedVals.m_tokenOID);
    return e_Absent;
  }
  
  // "U" indicates that the HMAC-SHA1-96 alorigthm is used.
  if (!CheckOID(crHashed.m_token.m_algorithmOID, OID_U)) {
    PTRACE(2, "H235RAS\tH235AuthProcedure1 requires HMAC-SHA1-96, got OID " << crHashed.m_token.m_algorithmOID);
    return e_Absent;
  }
  
  //first verify the timestamp
  PTime now;
  int deltaTime = (int)now.GetTimeInSeconds() - crHashed.m_hashedVals.m_timeStamp;
  if (PABS(deltaTime) > timestampGracePeriod) {
    PTRACE(1, "H235RAS\tInvalid timestamp ABS(" << now.GetTimeInSeconds() << '-' 
           << (int)crHashed.m_hashedVals.m_timeStamp << ") > " << timestampGracePeriod);
    //the time has elapsed
    return e_InvalidTime;
  }
  
  //verify the randomnumber
  if (lastTimestamp == crHashed.m_hashedVals.m_timeStamp &&
      lastRandomSequenceNumber == crHashed.m_hashedVals.m_random) {
    //a message with this timespamp and the same random number was already verified
    PTRACE(2, "H235RAS\tConsecutive messages with the same random and timestamp");
    return e_ReplyAttack;
  }
  
  // save the values for the next call
  lastRandomSequenceNumber = crHashed.m_hashedVals.m_random;
  lastTimestamp = crHashed.m_hashedVals.m_timeStamp;
  
  //verify the username
  if (!localId && crHashed.m_tokenOID[OID_VERSION_OFFSET] > 1) {
    if (!crHashed.m_hashedVals.HasOptionalField(H235_ClearToken::e_generalID)) {
      PTRACE(1, "H235RAS\tH235AuthProcedure1 requires general ID.");
      return e_Error;
    }
  
    if (crHashed.m_hashedVals.m_generalID.GetValue() != localId) {
      PTRACE(1, "H235RAS\tGeneral ID is \"" << crHashed.m_hashedVals.m_generalID.GetValue()
             << "\", should be \"" << localId << '"');
      return e_Error;
    }
  }

  if (!remoteId) {
    if (!crHashed.m_hashedVals.HasOptionalField(H235_ClearToken::e_sendersID)) {
      PTRACE(1, "H235RAS\tH235AuthProcedure1 requires senders ID.");
      return e_Error;
    }
  
    if (crHashed.m_hashedVals.m_sendersID.GetValue() != remoteId) {
      PTRACE(1, "H235RAS\tSenders ID is \"" << crHashed.m_hashedVals.m_sendersID.GetValue()
             << "\", should be \"" << remoteId << '"');
      return e_Error;
    }
  }

  
  /****
  * step 1
  * extract the variable hash and save it
  *
  */
  BYTE RV[HASH_SIZE];
  
  if (crHashed.m_token.m_hash.GetSize() != HASH_SIZE*8) {
    PTRACE(1, "H235RAS\tH235AuthProcedure1 requires a hash!");
    return e_Error;
  }
  
  const unsigned char *data = crHashed.m_token.m_hash.GetDataPointer();
  memcpy(RV, data, HASH_SIZE);
  
  unsigned char secretkey[20];
  SHA1((unsigned char *)password.GetPointer(), password.GetSize()-1, secretkey);
    
  
  /****
  * step 4
  * lookup the variable int the orginal ASN1 packet
  * and set it to 0.
  */
  PINDEX foundat = 0;
  bool found = false;
  
  const BYTE * asnPtr = rawPDU;
  PINDEX asnLen = rawPDU.GetSize();
  while (foundat < asnLen - HASH_SIZE) {
    for (PINDEX i = foundat; i <= asnLen - HASH_SIZE; i++) {
      if (memcmp(asnPtr+i, data, HASH_SIZE) == 0) { // i'v found it !
        foundat = i;
        found = true;
        break;
      }
    }
    
    if (!found) {
      if (foundat != 0)
        break;

      PTRACE(1, "H235RAS\tH235AuthProcedure1 could not locate embedded hash!");
      return e_Error;
    }
    
    found = false;
    
    memset((BYTE *)asnPtr+foundat, 0, HASH_SIZE);
    
    /****
    * step 5
    * generate a HMAC-SHA1 key over the hole packet
    *
    */
    
    char key[HASH_SIZE];
    hmac_sha(secretkey, 20, asnPtr, asnLen, key, HASH_SIZE);
    
    
    /****
    * step 6
    * compare the two keys
    *
    */
    if(memcmp(key, RV, HASH_SIZE) == 0) // Keys are the same !! Ok
      return e_OK;

    // Put it back and look for another
    memcpy((BYTE *)asnPtr+foundat, data, HASH_SIZE);
    foundat++;
  }

  PTRACE(2, "H235RAS\tH235AuthProcedure1 hash does not match.");
  return e_BadPassword;
}


PBoolean H235AuthProcedure1::IsCapability(const H235_AuthenticationMechanism & mechansim,
                                      const PASN_ObjectId & algorithmOID)
{
  return mechansim.GetTag() == H235_AuthenticationMechanism::e_pwdHash &&
         algorithmOID.AsString() == OID_U;
}


PBoolean H235AuthProcedure1::SetCapability(H225_ArrayOf_AuthenticationMechanism & mechanisms,
                                      H225_ArrayOf_PASN_ObjectId & algorithmOIDs)
{
  return AddCapability(H235_AuthenticationMechanism::e_pwdHash, OID_U, mechanisms, algorithmOIDs);
}


PBoolean H235AuthProcedure1::UseGkAndEpIdentifiers() const
{
  return PTrue;
}


#endif // OPAL_H323

#endif // #if OPAL_PTLIB_SSL


/////////////////////////////////////////////////////////////////////////////
