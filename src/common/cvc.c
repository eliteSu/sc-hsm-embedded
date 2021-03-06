/**
 * SmartCard-HSM PKCS#11 Module
 *
 * Copyright (c) 2013, CardContact Systems GmbH, Minden, Germany
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of CardContact Systems GmbH nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CardContact Systems GmbH BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @file    cvc.c
 * @author  Andreas Schwier
 * @brief   Encoding and decoding of card verifiable certificates
 */

#include "cvc.h"
#include "asn1.h"


#ifdef DEBUG
extern void debug(char *, ...);
#endif


static struct ec_curve curves[] = {
		{
				{ (unsigned char *) "\x2A\x86\x48\xCE\x3D\x03\x01\x01", 8},	// secp192r1 aka prime192r1
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 24},
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFC", 24},
				{ (unsigned char *) "\x64\x21\x05\x19\xE5\x9C\x80\xE7\x0F\xA7\xE9\xAB\x72\x24\x30\x49\xFE\xB8\xDE\xEC\xC1\x46\xB9\xB1", 24},
				{ (unsigned char *) "\x04\x18\x8D\xA8\x0E\xB0\x30\x90\xF6\x7C\xBF\x20\xEB\x43\xA1\x88\x00\xF4\xFF\x0A\xFD\x82\xFF\x10\x12\x07\x19\x2B\x95\xFF\xC8\xDA\x78\x63\x10\x11\xED\x6B\x24\xCD\xD5\x73\xF9\x77\xA1\x1E\x79\x48\x11", 49},
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x99\xDE\xF8\x36\x14\x6B\xC9\xB1\xB4\xD2\x28\x31", 24},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2A\x86\x48\xCE\x3D\x03\x01\x07", 8},	// secp256r1 aka prime256r1
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 32},
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFC", 32},
				{ (unsigned char *) "\x5A\xC6\x35\xD8\xAA\x3A\x93\xE7\xB3\xEB\xBD\x55\x76\x98\x86\xBC\x65\x1D\x06\xB0\xCC\x53\xB0\xF6\x3B\xCE\x3C\x3E\x27\xD2\x60\x4B", 32},
				{ (unsigned char *) "\x04\x6B\x17\xD1\xF2\xE1\x2C\x42\x47\xF8\xBC\xE6\xE5\x63\xA4\x40\xF2\x77\x03\x7D\x81\x2D\xEB\x33\xA0\xF4\xA1\x39\x45\xD8\x98\xC2\x96\x4F\xE3\x42\xE2\xFE\x1A\x7F\x9B\x8E\xE7\xEB\x4A\x7C\x0F\x9E\x16\x2B\xCE\x33\x57\x6B\x31\x5E\xCE\xCB\xB6\x40\x68\x37\xBF\x51\xF5", 65},
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\x00\x00\x00\x00\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xBC\xE6\xFA\xAD\xA7\x17\x9E\x84\xF3\xB9\xCA\xC2\xFC\x63\x25\x51", 32},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2B\x24\x03\x03\x02\x08\x01\x01\x03", 9},	// brainpoolP192r1
				{ (unsigned char *) "\xC3\x02\xF4\x1D\x93\x2A\x36\xCD\xA7\xA3\x46\x30\x93\xD1\x8D\xB7\x8F\xCE\x47\x6D\xE1\xA8\x62\x97", 24},
				{ (unsigned char *) "\x6A\x91\x17\x40\x76\xB1\xE0\xE1\x9C\x39\xC0\x31\xFE\x86\x85\xC1\xCA\xE0\x40\xE5\xC6\x9A\x28\xEF", 24},
				{ (unsigned char *) "\x46\x9A\x28\xEF\x7C\x28\xCC\xA3\xDC\x72\x1D\x04\x4F\x44\x96\xBC\xCA\x7E\xF4\x14\x6F\xBF\x25\xC9", 24},
				{ (unsigned char *) "\x04\xC0\xA0\x64\x7E\xAA\xB6\xA4\x87\x53\xB0\x33\xC5\x6C\xB0\xF0\x90\x0A\x2F\x5C\x48\x53\x37\x5F\xD6\x14\xB6\x90\x86\x6A\xBD\x5B\xB8\x8B\x5F\x48\x28\xC1\x49\x00\x02\xE6\x77\x3F\xA2\xFA\x29\x9B\x8F", 49},
				{ (unsigned char *) "\xC3\x02\xF4\x1D\x93\x2A\x36\xCD\xA7\xA3\x46\x2F\x9E\x9E\x91\x6B\x5B\xE8\xF1\x02\x9A\xC4\xAC\xC1", 24},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2B\x24\x03\x03\x02\x08\x01\x01\x05", 9},	// brainpoolP224r1
				{ (unsigned char *) "\xD7\xC1\x34\xAA\x26\x43\x66\x86\x2A\x18\x30\x25\x75\xD1\xD7\x87\xB0\x9F\x07\x57\x97\xDA\x89\xF5\x7E\xC8\xC0\xFF", 28},
				{ (unsigned char *) "\x68\xA5\xE6\x2C\xA9\xCE\x6C\x1C\x29\x98\x03\xA6\xC1\x53\x0B\x51\x4E\x18\x2A\xD8\xB0\x04\x2A\x59\xCA\xD2\x9F\x43", 28},
				{ (unsigned char *) "\x25\x80\xF6\x3C\xCF\xE4\x41\x38\x87\x07\x13\xB1\xA9\x23\x69\xE3\x3E\x21\x35\xD2\x66\xDB\xB3\x72\x38\x6C\x40\x0B", 28},
				{ (unsigned char *) "\x04\x0D\x90\x29\xAD\x2C\x7E\x5C\xF4\x34\x08\x23\xB2\xA8\x7D\xC6\x8C\x9E\x4C\xE3\x17\x4C\x1E\x6E\xFD\xEE\x12\xC0\x7D\x58\xAA\x56\xF7\x72\xC0\x72\x6F\x24\xC6\xB8\x9E\x4E\xCD\xAC\x24\x35\x4B\x9E\x99\xCA\xA3\xF6\xD3\x76\x14\x02\xCD", 57},
				{ (unsigned char *) "\xD7\xC1\x34\xAA\x26\x43\x66\x86\x2A\x18\x30\x25\x75\xD0\xFB\x98\xD1\x16\xBC\x4B\x6D\xDE\xBC\xA3\xA5\xA7\x93\x9F", 28},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2B\x24\x03\x03\x02\x08\x01\x01\x07", 9},	// brainpoolP256r1
				{ (unsigned char *) "\xA9\xFB\x57\xDB\xA1\xEE\xA9\xBC\x3E\x66\x0A\x90\x9D\x83\x8D\x72\x6E\x3B\xF6\x23\xD5\x26\x20\x28\x20\x13\x48\x1D\x1F\x6E\x53\x77", 32},
				{ (unsigned char *) "\x7D\x5A\x09\x75\xFC\x2C\x30\x57\xEE\xF6\x75\x30\x41\x7A\xFF\xE7\xFB\x80\x55\xC1\x26\xDC\x5C\x6C\xE9\x4A\x4B\x44\xF3\x30\xB5\xD9", 32},
				{ (unsigned char *) "\x26\xDC\x5C\x6C\xE9\x4A\x4B\x44\xF3\x30\xB5\xD9\xBB\xD7\x7C\xBF\x95\x84\x16\x29\x5C\xF7\xE1\xCE\x6B\xCC\xDC\x18\xFF\x8C\x07\xB6", 32},
				{ (unsigned char *) "\x04\x8B\xD2\xAE\xB9\xCB\x7E\x57\xCB\x2C\x4B\x48\x2F\xFC\x81\xB7\xAF\xB9\xDE\x27\xE1\xE3\xBD\x23\xC2\x3A\x44\x53\xBD\x9A\xCE\x32\x62\x54\x7E\xF8\x35\xC3\xDA\xC4\xFD\x97\xF8\x46\x1A\x14\x61\x1D\xC9\xC2\x77\x45\x13\x2D\xED\x8E\x54\x5C\x1D\x54\xC7\x2F\x04\x69\x97", 65},
				{ (unsigned char *) "\xA9\xFB\x57\xDB\xA1\xEE\xA9\xBC\x3E\x66\x0A\x90\x9D\x83\x8D\x71\x8C\x39\x7A\xA3\xB5\x61\xA6\xF7\x90\x1E\x0E\x82\x97\x48\x56\xA7", 32},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2B\x24\x03\x03\x02\x08\x01\x01\x09", 9},	// brainpoolP320r1
				{ (unsigned char *) "\xD3\x5E\x47\x20\x36\xBC\x4F\xB7\xE1\x3C\x78\x5E\xD2\x01\xE0\x65\xF9\x8F\xCF\xA6\xF6\xF4\x0D\xEF\x4F\x92\xB9\xEC\x78\x93\xEC\x28\xFC\xD4\x12\xB1\xF1\xB3\x2E\x27", 40},
				{ (unsigned char *) "\x3E\xE3\x0B\x56\x8F\xBA\xB0\xF8\x83\xCC\xEB\xD4\x6D\x3F\x3B\xB8\xA2\xA7\x35\x13\xF5\xEB\x79\xDA\x66\x19\x0E\xB0\x85\xFF\xA9\xF4\x92\xF3\x75\xA9\x7D\x86\x0E\xB4", 40},
				{ (unsigned char *) "\x52\x08\x83\x94\x9D\xFD\xBC\x42\xD3\xAD\x19\x86\x40\x68\x8A\x6F\xE1\x3F\x41\x34\x95\x54\xB4\x9A\xCC\x31\xDC\xCD\x88\x45\x39\x81\x6F\x5E\xB4\xAC\x8F\xB1\xF1\xA6", 40},
				{ (unsigned char *) "\x04\x43\xBD\x7E\x9A\xFB\x53\xD8\xB8\x52\x89\xBC\xC4\x8E\xE5\xBF\xE6\xF2\x01\x37\xD1\x0A\x08\x7E\xB6\xE7\x87\x1E\x2A\x10\xA5\x99\xC7\x10\xAF\x8D\x0D\x39\xE2\x06\x11\x14\xFD\xD0\x55\x45\xEC\x1C\xC8\xAB\x40\x93\x24\x7F\x77\x27\x5E\x07\x43\xFF\xED\x11\x71\x82\xEA\xA9\xC7\x78\x77\xAA\xAC\x6A\xC7\xD3\x52\x45\xD1\x69\x2E\x8E\xE1", 81},
				{ (unsigned char *) "\xD3\x5E\x47\x20\x36\xBC\x4F\xB7\xE1\x3C\x78\x5E\xD2\x01\xE0\x65\xF9\x8F\xCF\xA5\xB6\x8F\x12\xA3\x2D\x48\x2E\xC7\xEE\x86\x58\xE9\x86\x91\x55\x5B\x44\xC5\x93\x11", 40},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2B\x81\x04\x00\x1F", 5},	// secp192k1
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\xFF\xFF\xEE\x37", 24},
				{ (unsigned char *) "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 24},
				{ (unsigned char *) "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x03", 24},
				{ (unsigned char *) "\x04\xDB\x4F\xF1\x0E\xC0\x57\xE9\xAE\x26\xB0\x7D\x02\x80\xB7\xF4\x34\x1D\xA5\xD1\xB1\xEA\xE0\x6C\x7D\x9B\x2F\x2F\x6D\x9C\x56\x28\xA7\x84\x41\x63\xD0\x15\xBE\x86\x34\x40\x82\xAA\x88\xD9\x5E\x2F\x9D", 49},
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\x26\xF2\xFC\x17\x0F\x69\x46\x6A\x74\xDE\xFD\x8D", 24},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ (unsigned char *) "\x2B\x81\x04\x00\x0A", 5},	// secp256k1
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\xFF\xFF\xFC\x2F", 32},
				{ (unsigned char *) "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 32},
				{ (unsigned char *) "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x07", 32},
				{ (unsigned char *) "\x04\x79\xBE\x66\x7E\xF9\xDC\xBB\xAC\x55\xA0\x62\x95\xCE\x87\x0B\x07\x02\x9B\xFC\xDB\x2D\xCE\x28\xD9\x59\xF2\x81\x5B\x16\xF8\x17\x98\x48\x3A\xDA\x77\x26\xA3\xC4\x65\x5D\xA4\xFB\xFC\x0E\x11\x08\xA8\xFD\x17\xB4\x48\xA6\x85\x54\x19\x9C\x47\xD0\x8F\xFB\x10\xD4\xB8", 65},
				{ (unsigned char *) "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFE\xBA\xAE\xDC\xE6\xAF\x48\xA0\x3B\xBF\xD2\x5E\x8C\xD0\x36\x41\x41", 32},
				{ (unsigned char *) "\x01", 1}
		},
		{
				{ NULL, 0},
				{ NULL, 0},
				{ NULL, 0},
				{ NULL, 0},
				{ NULL, 0},
				{ NULL, 0},
				{ NULL, 0}
		}
};



struct ec_curve *cvcGetCurveForOID(bytestring oid)
{
	struct ec_curve *c;

	c = curves;
	while ((c->oid.val != NULL) && bsCompare((bytestring)&c->oid, oid)) {
		c++;
	}
	if (c->oid.val == NULL) {
		return NULL;
	}

	return c;
}



int cvcDetermineCurveOID(struct cvc *cvc, bytestring *oid)
{
	struct ec_curve *c;

	if (cvc->primeOrModulus.val == NULL) {
		return -1;
	}
	c = curves;
	while ((c->oid.val != NULL) && bsCompare((bytestring)&c->prime, &cvc->primeOrModulus)) {
		c++;
	}
	if (c->oid.val == NULL) {
		return -1;
	}

	*oid = &c->oid;
	return 0;

}



int cvcDetermineCurveFromECParam(unsigned char *ecparam, size_t ecparamlen, struct ec_curve *curve)
{
	int vallen, tag, length, childrenlen;
	unsigned char *po, *val, *children;

	memset(curve, 0, sizeof(struct ec_curve));

	po = ecparam;
	length = (int)ecparamlen;

	if (!asn1Next(&po, &length, &tag, &childrenlen, &children)) {
#ifdef DEBUG
		debug("Error decoding ecParameter");
#endif
		return -1;
	}

	if (tag != ASN1_SEQUENCE) {
#ifdef DEBUG
		debug("ecParameter not a SEQUENCE");
#endif
		return -1;
	}

	po = children;
	length = childrenlen;

	// version
	if (!asn1Next(&po, &length, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding version");
#endif
		return -1;
	}

	if ((tag != ASN1_INTEGER) || (vallen != 1) || (*val != 0x01)) {
#ifdef DEBUG
		debug("version not INTEGER, length = 1 or value = 1");
#endif
return -1;
	}

	// fieldID
	if (!asn1Next(&po, &length, &tag, &childrenlen, &children)) {
#ifdef DEBUG
		debug("Error decoding fieldID");
#endif
		return -1;
	}

	if (tag != ASN1_SEQUENCE) {
#ifdef DEBUG
		debug("fieldID not a SEQUENCE");
#endif
		return -1;
	}

	// fieldType
	if (!asn1Next(&children, &childrenlen, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding fieldType");
#endif
		return -1;
	}

	if ((tag != ASN1_OBJECT_IDENTIFIER) || (vallen != 7) || (*(val + 6) != 0x01)) {
#ifdef DEBUG
		debug("fieldType not OBJECT IDENTIFIER, length = 7 or value = prime-field");
#endif
		return -1;
	}

	// prime
	if (!asn1Next(&children, &childrenlen, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding prime");
#endif
		return -1;
	}

	if (tag != ASN1_INTEGER) {
#ifdef DEBUG
		debug("prime not INTEGER");
#endif
		return -1;
	}

	if (*val == 0) {
		val++;
		vallen--;
	}

	curve->prime.val = val;
	curve->prime.len = vallen;

	// curve
	if (!asn1Next(&po, &length, &tag, &childrenlen, &children)) {
#ifdef DEBUG
		debug("Error decoding curve");
#endif
		return -1;
	}

	if (tag != ASN1_SEQUENCE) {
#ifdef DEBUG
		debug("curve not a SEQUENCE");
#endif
		return -1;
	}

	// a
	if (!asn1Next(&children, &childrenlen, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding curve parameter a");
#endif
		return -1;
	}

	if (tag != ASN1_OCTET_STRING) {
#ifdef DEBUG
		debug("parameter a not OCTET STRING");
#endif
		return -1;
	}

	curve->coefficientA.val = val;
	curve->coefficientA.len = vallen;

	// b
	if (!asn1Next(&children, &childrenlen, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding curve parameter b");
#endif
		return -1;
	}

	if (tag != ASN1_OCTET_STRING) {
#ifdef DEBUG
		debug("parameter b not OCTET STRING");
#endif
		return -1;
	}

	curve->coefficientB.val = val;
	curve->coefficientB.len = vallen;

	// base
	if (!asn1Next(&po, &length, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding base");
#endif
		return -1;
	}

	if ((tag != ASN1_OCTET_STRING) || (*val != 0x04)) {
#ifdef DEBUG
		debug("parameter base not OCTET STRING or not uncompressed format");
#endif
		return -1;
	}

	curve->basePointG.val = val;
	curve->basePointG.len = vallen;

	// order
	if (!asn1Next(&po, &length, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding order");
#endif
		return -1;
	}

	if (tag != ASN1_INTEGER) {
#ifdef DEBUG
		debug("parameter order not INTEGER");
#endif
		return -1;
	}

	if (*val == 0) {
		val++;
		vallen--;
	}

	curve->order.val = val;
	curve->order.len = vallen;

	// cofactor
	if (!asn1Next(&po, &length, &tag, &vallen, &val)) {
#ifdef DEBUG
		debug("Error decoding cofactor");
#endif
		return -1;
	}

	if (tag != ASN1_INTEGER) {
#ifdef DEBUG
		debug("parameter cofactor not INTEGER");
#endif
		return -1;
	}

	if (*val == 0) {
		val++;
		vallen--;
	}

	curve->coFactor.val = val;
	curve->coFactor.len = vallen;

	return 0;
}



int cvcDecode(unsigned char *cert, size_t certlen, struct cvc *cvc)
{
	int rc, rlen, tag, outertag, length, childrenlen;
	unsigned char *po, *ppo, *val, *children;

	memset(cvc, 0, sizeof(struct cvc));

	rc = (int)asn1Validate(cert, certlen);

	if (rc != 0) {
		return -1;
	}

	po = cert;
	if (!asn1Next(&cert, (int *)&certlen, &outertag, &childrenlen, &children)) {
		return -1;
	}

	certlen = cert - po;

	if (outertag == 0x67) {			// CVC Request
		po = children;
		rlen = childrenlen;

		if (!asn1Next(&po, &rlen, &outertag, &childrenlen, &children)) {	// Decode later
			return -1;
		}

		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->outer_car.len, &cvc->outer_car.val) || (tag != 0x42)) {
			return -1;
		}

		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->outerSignature.len, &cvc->outerSignature.val) || (tag != 0x5F37)) {
			return -1;
		}

		if (rlen > 0) {
			return -1;
		}
	}

	if (outertag != 0x7F21) {
		return -1;
	}

	po = children;
	rlen = childrenlen;

	if (!asn1Next(&po, &rlen, &tag, &childrenlen, &children) || (tag != 0x7F4E)) {		// Decode later
		return -1;
	}

	if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->signature.len, &cvc->signature.val) || (tag != 0x5F37)) {
		return -1;
	}

	if (rlen > 0) {
		return -1;
	}

	po = children;
	rlen = childrenlen;

	if (!asn1Next(&po, &rlen, &tag, &length, &val) || (tag != 0x5F29) || (*val != 0)) {
		return -1;
	}

	ppo = po;
	if (asn1Tag(&ppo) == 0x42) {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->car.len, &cvc->car.val)) {
			return -1;
		}
	}

	if (!asn1Next(&po, &rlen, &tag, &childrenlen, &children) || (tag != 0x7F49)) {
		return -1;
	}

	if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->chr.len, &cvc->chr.val) || (tag != 0x5F20)) {
		return -1;
	}

	ppo = po;
	if ((rlen > 0) && (asn1Tag(&ppo) == 0x7F4C)) {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->chat.len, &cvc->chat.val)) {
			return -1;
		}
	}

	ppo = po;
	if ((rlen > 0) && (asn1Tag(&ppo) == 0x5F25)) {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->ced.len, &cvc->ced.val)) {
			return -1;
		}
	}

	ppo = po;
	if ((rlen > 0) && (asn1Tag(&ppo) == 0x5F24)) {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->cxd.len, &cvc->cxd.val)) {
			return -1;
		}
	}

	if (rlen > 0) {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->extensions.len, &cvc->extensions.val) || (tag != 0x65)) {
			return -1;
		}
		if (rlen > 0) {
			return -1;
		}
	}

	po = children;
	rlen = childrenlen;

	if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->pukoid.len, &cvc->pukoid.val) || (tag != 0x06)) {
		return -1;
	}

	ppo = po;
	if ((rlen > 0) && (asn1Tag(&ppo) == 0x86)) {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->publicPoint.len, &cvc->publicPoint.val) || (tag != 0x86)) {
			return -1;
		}
	} else {
		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->primeOrModulus.len, &cvc->primeOrModulus.val) || (tag != 0x81)) {
			return -1;
		}

		if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->coefficientAorExponent.len, &cvc->coefficientAorExponent.val) || (tag != 0x82)) {
			return -1;
		}

		if (rlen > 0) {
			if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->coefficientB.len, &cvc->coefficientB.val) || (tag != 0x83)) {
				return -1;
			}

			if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->basePointG.len, &cvc->basePointG.val) || (tag != 0x84)) {
				return -1;
			}

			if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->order.len, &cvc->order.val) || (tag != 0x85)) {
				return -1;
			}

			if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->publicPoint.len, &cvc->publicPoint.val) || (tag != 0x86)) {
				return -1;
			}

			if (!asn1Next(&po, &rlen, &tag, (int *)&cvc->cofactor.len, &cvc->cofactor.val) || (tag != 0x87)) {
				return -1;
			}
		}
	}

	return (int)certlen;
}



/**
 * Wrap a ECDSA signature with fixed length components R and S two ASN1 INTEGER objects combined to an ASN1 SEQUENCE
 *
 * @param signature the pointer to the plain signature
 * @param signatueLen the length of the plain signature
 * @param wrappedSig the buffer receiving the wrapped signature
 * @param bufflen on input the maximum size of the buffer on output the actual length
 * @return -1 if conversion failed
 */
int cvcWrapECDSASignature(unsigned char *signature, int signatureLen, unsigned char *wrappedSig, int *bufflen)
{
	struct bytebuffer_s bb = { wrappedSig, 0, *bufflen };
	struct bytestring_s str = { NULL, 0 };

	str.val = signature;
	str.len = signatureLen >> 1;
	asn1AppendUnsignedBigInteger(&bb, ASN1_INTEGER, &str);

	str.val = signature + str.len;
	asn1AppendUnsignedBigInteger(&bb, ASN1_INTEGER, &str);

	asn1EncapBuffer(ASN1_SEQUENCE, &bb, 0);

	if (bbHasFailed(&bb)) {
		return -1;
	}

	*bufflen = (int)bb.len;
	return 0;
}

