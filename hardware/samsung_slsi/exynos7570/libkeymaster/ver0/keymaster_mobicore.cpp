/*
 * Copyright (C) 2012 Samsung Electronics Co., LTD
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <string.h>
#include <stdint.h>

#include <hardware/hardware.h>
#include <hardware/keymaster0.h>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/x509.h>

#include <UniquePtr.h>

#define LOG_TAG "ExynosKeyMaster"
#include <cutils/log.h>

#include <tlcTeeKeymaster_if.h>

#define KEY_BUFFER_SIZE		4096
#define PUBKEY_BUFFER_SIZE	1204
#define RSA_SIG_MAX_SIZE	(4096 >> 3)
#define DSA_SIG_MAX_SIZE	((256 >> 3) * 2)
#define ECDSA_SIG_MAX_SIZE	(((521 >> 3) + 1) * 2)

struct BIGNUM_Delete {
    void operator()(BIGNUM* p) const {
        BN_free(p);
    }
};
typedef UniquePtr<BIGNUM, BIGNUM_Delete> Unique_BIGNUM;

struct EVP_PKEY_Delete {
    void operator()(EVP_PKEY* p) const {
        EVP_PKEY_free(p);
    }
};
typedef UniquePtr<EVP_PKEY, EVP_PKEY_Delete> Unique_EVP_PKEY;

struct PKCS8_PRIV_KEY_INFO_Delete {
    void operator()(PKCS8_PRIV_KEY_INFO* p) const {
        PKCS8_PRIV_KEY_INFO_free(p);
    }
};
typedef UniquePtr<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_Delete> Unique_PKCS8_PRIV_KEY_INFO;

struct RSA_Delete {
    void operator()(RSA* p) const {
        RSA_free(p);
    }
};
typedef UniquePtr<RSA, RSA_Delete> Unique_RSA;

struct DSA_Delete {
    void operator()(DSA* p) const {
        DSA_free(p);
    }
};
typedef UniquePtr<DSA, DSA_Delete> Unique_DSA;

struct EC_KEY_Delete {
    void operator()(EC_KEY* p) const {
        EC_KEY_free(p);
    }
};
typedef UniquePtr<EC_KEY, EC_KEY_Delete> Unique_EC_KEY;

struct EC_POINT_Delete {
    void operator()(EC_POINT* p) const {
        EC_POINT_free(p);
    }
};
typedef UniquePtr<EC_POINT, EC_POINT_Delete> Unique_EC_POPINT;

struct EC_GROUP_Delete {
    void operator()(EC_GROUP* p) const {
        EC_GROUP_free(p);
    }
};
typedef UniquePtr<EC_GROUP, EC_GROUP_Delete> Unique_EC_GROUP;

struct Malloc_Free {
    void operator()(void* p) const {
	free(p);
    }
};

typedef UniquePtr<keymaster0_device_t> Unique_keymaster0_device_t;

/**
 * Many OpenSSL APIs take ownership of an argument on success but don't free the argument
 * on failure. This means we need to tell our scoped pointers when we've transferred ownership,
 * without triggering a warning by not using the result of release().
 */
#define OWNERSHIP_TRANSFERRED(obj) \
    typeof (obj.release()) _dummy __attribute__((unused)) = obj.release()

/*
 * Checks this thread's error queue and logs if necessary.
 */
static void logOpenSSLError(const char* location) {
    int error = ERR_get_error();

    if (error != 0) {
        char message[256];
        ERR_error_string_n(error, message, sizeof(message));
        ALOGE("OpenSSL error in %s %d: %s", location, error, message);
    }

    ERR_clear_error();
    ERR_remove_state(0);
}

static int km_generate_rsa_keypair(const keymaster_rsa_keygen_params_t *rsa_params,
				uint8_t *keyData, size_t *keyLength) {
    teeResult_t ret = TEE_ERR_NONE;

    ret = TEE_RSAGenerateKeyPair(TEE_KEYPAIR_RSACRT, keyData, KEY_BUFFER_SIZE,
				rsa_params->modulus_size,
				(uint32_t)rsa_params->public_exponent,
				(uint32_t *)keyLength);
    if (ret != TEE_ERR_NONE)
        ALOGE("TEE_RSAGenerateKeyPair() is failed: %d", ret);

    return ret;
}

static int km_generate_dsa_keypair(const keymaster_dsa_keygen_params_t *dsa_params,
				uint8_t *keyData, size_t *keyLength) {
    teeDsaParams_t tee_dsa_params;
    teeResult_t ret = TEE_ERR_NONE;

    if (dsa_params->generator_len == 0 ||
            dsa_params->prime_p_len == 0 ||
            dsa_params->prime_q_len == 0 ||
            dsa_params->generator == NULL||
            dsa_params->prime_p == NULL ||
            dsa_params->prime_q == NULL) {
	Unique_DSA dsa(DSA_new());

	if (dsa.get() == NULL) {
	    logOpenSSLError("openssl_verify_data");
	    return -1;
	}

	if (DSA_generate_parameters_ex(dsa.get(), dsa_params->key_size, NULL, 0,
				NULL, NULL, NULL) != 1) {
	    logOpenSSLError("generate_dsa_keypair");
	    return -1;
	}

	UniquePtr<uint8_t, Malloc_Free> prime_p(static_cast<uint8_t*>(malloc(BN_num_bytes(dsa->p))));
	if (prime_p.get() == NULL) {
	    ALOGE("Could not allocate memory for prime p");
	    return -1;
	}

	UniquePtr<uint8_t, Malloc_Free> prime_q(static_cast<uint8_t*>(malloc(BN_num_bytes(dsa->q))));
	if (prime_q.get() == NULL) {
	    ALOGE("Could not allocate memory for prime q");
	    return -1;
	}

	UniquePtr<uint8_t, Malloc_Free> generator(static_cast<uint8_t*>(malloc(BN_num_bytes(dsa->g))));
	if (generator.get() == NULL) {
	    ALOGE("Could not allocate memory for generator");
	    return -1;
	}

	tee_dsa_params.pLen = BN_bn2bin(dsa->p, prime_p.get());
	tee_dsa_params.qLen = BN_bn2bin(dsa->q, prime_q.get());
	tee_dsa_params.gLen = BN_bn2bin(dsa->g, generator.get());
	tee_dsa_params.xLen = tee_dsa_params.qLen;
	tee_dsa_params.yLen = tee_dsa_params.pLen;
	tee_dsa_params.p = (uint8_t*)prime_p.get();
	tee_dsa_params.q = (uint8_t*)prime_q.get();
	tee_dsa_params.g = (uint8_t*)generator.get();

	ret = TEE_DSAGenerateKeyPair(keyData, KEY_BUFFER_SIZE, &tee_dsa_params,
				(uint32_t *)keyLength);
	if (ret != TEE_ERR_NONE) {
            ALOGE("TEE_DSAGenerateKeyPair() is failed: %d", ret);
	    return -1;
	}
    } else {
	/* binder gives us read-only mappings we can't use with mobicore */
	void *tmpP = malloc(dsa_params->prime_p_len);
	void *tmpQ = malloc(dsa_params->prime_q_len);
	void *tmpG = malloc(dsa_params->generator_len);
	uint32_t i;

	tee_dsa_params.p = (uint8_t*)tmpP;
	tee_dsa_params.q = (uint8_t*)tmpQ;
	tee_dsa_params.g = (uint8_t*)tmpG;

	/* decide p length */
	for (i = 0; i < dsa_params->prime_p_len; i++)
		if (*(dsa_params->prime_p + i) != 0)
			break;
	tee_dsa_params.pLen = dsa_params->prime_p_len - i;
	memcpy(tmpP, dsa_params->prime_p + i, tee_dsa_params.pLen);

	/* decide q length */
	for (i = 0; i < dsa_params->prime_q_len; i++)
		if (*(dsa_params->prime_q + i) != 0)
			break;
	tee_dsa_params.qLen = dsa_params->prime_q_len - i;
	memcpy(tmpQ, dsa_params->prime_q + i, tee_dsa_params.qLen);

	/* decide g length */
	for (i = 0; i < dsa_params->generator_len; i++)
		if (*(dsa_params->generator + i) != 0)
			break;
	tee_dsa_params.gLen = dsa_params->generator_len - i;
	memcpy(tmpG, dsa_params->generator + i, tee_dsa_params.gLen);

	tee_dsa_params.xLen = tee_dsa_params.qLen;
	tee_dsa_params.yLen = tee_dsa_params.pLen;

	ret = TEE_DSAGenerateKeyPair(keyData, KEY_BUFFER_SIZE, &tee_dsa_params,
				(uint32_t *)keyLength);
	free(tmpP);
	free(tmpQ);
	free(tmpG);
	if (ret != TEE_ERR_NONE) {
	    ALOGE("TEE_DSAGenerateKeyPair() is failed: %d", ret);
	    return -1;
	}
    }

    return ret;
}

static int km_generate_ec_keypair(const keymaster_ec_keygen_params_t *ec_params,
				uint8_t *keyData, size_t *keyLength) {
    teeEccCurveType_t curve;
    teeResult_t ret = TEE_ERR_NONE;

    switch (ec_params->field_size) {
    case 192:
	curve = TEE_ECC_CURVE_NIST_P192;
	break;
    case 224:
	curve = TEE_ECC_CURVE_NIST_P224;
	break;
    case 256:
	curve = TEE_ECC_CURVE_NIST_P256;
	break;
    case 384:
	curve = TEE_ECC_CURVE_NIST_P384;
	break;
    case 521:
	curve = TEE_ECC_CURVE_NIST_P521;
	break;
    default:
	ALOGE("curve(%d) is not valid\n", ec_params->field_size);
	return -1;
    }

    ret = TEE_ECDSAGenerateKeyPair(keyData, KEY_BUFFER_SIZE, curve,
				(uint32_t *)keyLength);
    if (ret != TEE_ERR_NONE)
        ALOGE("TEE_ECDSAGenerateKeyPair() is failed: %d", ret);

    return ret;
}

static int exynos_km_generate_keypair(const keymaster0_device_t*,
				const keymaster_keypair_t key_type,
				const void* key_params, uint8_t** keyBlob,
				size_t* keyBlobLength) {
    /* keyBlobLength will be casted as uint32_t to call TAC API, so in 64bit
     * architecture, upper 32bit can have unexpected value. To prevent it,
     * it's modified to use temporary variable. */
    size_t tmp_blob_length = 0;

    if (key_params == NULL) {
	ALOGE("key_params = NULL");
	return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> keyDataPtr(reinterpret_cast<uint8_t*>(malloc(KEY_BUFFER_SIZE)));
    if (keyDataPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    } else if (keyBlobLength == NULL) {
        ALOGE("keyBlobLength = NULL");
	return -1;
    }

    if (key_type == TYPE_RSA) {
	const keymaster_rsa_keygen_params_t * rsa_params =
		(const keymaster_rsa_keygen_params_t*) key_params;
	if(km_generate_rsa_keypair(rsa_params, keyDataPtr.get(), &tmp_blob_length))
	    return -1;
    } else if (key_type == TYPE_DSA) {
	const keymaster_dsa_keygen_params_t * dsa_params =
		(const keymaster_dsa_keygen_params_t*) key_params;
	if(km_generate_dsa_keypair(dsa_params, keyDataPtr.get(), &tmp_blob_length))
	    return -1;
    } else if (key_type == TYPE_EC) {
	const keymaster_ec_keygen_params_t * ec_params =
		(const keymaster_ec_keygen_params_t*) key_params;
	if(km_generate_ec_keypair(ec_params, keyDataPtr.get(), &tmp_blob_length))
	    return -1;
    } else {
        ALOGE("Invalid key type (%d)\n", key_type);
        return -1;
    }

    *keyBlob = keyDataPtr.release();
    *keyBlobLength = tmp_blob_length;

    return 0;
}

static int km_import_rsa_keypair(EVP_PKEY *pkey, uint8_t *kbuf, uint32_t *key_len) {
    teeKeyMeta_t meta;
    teeRsaKeyMeta_t rsa_meta;
    uint32_t len = *key_len;
    BIGNUM *tmp = NULL;
    BN_CTX *ctx = NULL;

    /* change key format */
    Unique_RSA rsa(EVP_PKEY_get1_RSA(pkey));
    if (rsa.get() == NULL) {
        logOpenSSLError("get rsa key format");
	return -1;
    }

    if (BN_cmp(rsa->p, rsa->q) < 0) {
        /* p <-> q */
        tmp = rsa->p;
        rsa->p = rsa->q;
        rsa->q = tmp;
        /* dp <-> dq */
        tmp = rsa->dmp1;
        rsa->dmp1 = rsa->dmq1;
        rsa->dmq1 = tmp;
        /* calulate inverse of q mod p */
        ctx = BN_CTX_new();
        if (!BN_mod_inverse(rsa->iqmp, rsa->q, rsa->p, ctx)) {
            ALOGE("Calculating inverse of q mod p is failed\n");
            BN_CTX_free(ctx);
            return -1;
        }
        BN_CTX_free(ctx);
    }

    meta.keytype = TEE_KEYTYPE_RSA;
    len += sizeof(meta);

    rsa_meta.lenpubmod = BN_bn2bin(rsa->n, kbuf + len);

    len += rsa_meta.lenpubmod;
    if (rsa_meta.lenpubmod == (512 >> 3))
        rsa_meta.keysize = TEE_RSA_KEY_SIZE_512;
    else if (rsa_meta.lenpubmod == (1024 >> 3))
        rsa_meta.keysize = TEE_RSA_KEY_SIZE_1024;
    else if (rsa_meta.lenpubmod == (2048 >> 3))
        rsa_meta.keysize = TEE_RSA_KEY_SIZE_2048;
    else if (rsa_meta.lenpubmod == (3072 >> 3))
        rsa_meta.keysize = TEE_RSA_KEY_SIZE_3072;
    else if (rsa_meta.lenpubmod == (4096 >> 3))
        rsa_meta.keysize = TEE_RSA_KEY_SIZE_4096;
    else {
        ALOGE("key size(%d) is not supported\n", rsa_meta.lenpubmod << 3);
        return -1;
    }

    rsa_meta.lenpubexp = BN_bn2bin(rsa->e, kbuf + len);
    len += rsa_meta.lenpubexp;

    if ((rsa->p != NULL) && (rsa->q != NULL) && (rsa->dmp1 != NULL) &&
	(rsa->dmq1 != NULL) && (rsa->iqmp != NULL)) {
	rsa_meta.type = TEE_KEYPAIR_RSACRT;
	rsa_meta.rsacrtpriv.lenp = BN_bn2bin(rsa->p, kbuf + len);
	len += rsa_meta.rsacrtpriv.lenp;
	rsa_meta.rsacrtpriv.lenq = BN_bn2bin(rsa->q, kbuf + len);
	len += rsa_meta.rsacrtpriv.lenq;
	rsa_meta.rsacrtpriv.lendp = BN_bn2bin(rsa->dmp1, kbuf + len);
	len += rsa_meta.rsacrtpriv.lendp;
	rsa_meta.rsacrtpriv.lendq = BN_bn2bin(rsa->dmq1, kbuf + len);
	len += rsa_meta.rsacrtpriv.lendq;
	rsa_meta.rsacrtpriv.lenqinv = BN_bn2bin(rsa->iqmp, kbuf + len);
	len += rsa_meta.rsacrtpriv.lenqinv;
    } else {
	rsa_meta.type = TEE_KEYPAIR_RSA;
	rsa_meta.rsapriv.lenpriexp = BN_bn2bin(rsa->d, kbuf + len);
	len += rsa_meta.rsapriv.lenpriexp;
    }

    memcpy(&meta.rsakey, &rsa_meta, sizeof(rsa_meta));
    memcpy(kbuf, &meta, sizeof(meta));

    *key_len = len;

    return 0;
}

static int km_import_dsa_keypair(EVP_PKEY *pkey, uint8_t *kbuf, uint32_t *key_len) {
    teeKeyMeta_t meta;
    uint32_t len = *key_len;

    /* change key format */
    Unique_DSA dsa(EVP_PKEY_get1_DSA(pkey));
    if (dsa.get() == NULL) {
	logOpenSSLError("get dsa key format");
	return -1;
    }

    meta.keytype = TEE_KEYTYPE_DSA;
    len += sizeof(meta);

    meta.dsakey.pLen = BN_bn2bin(dsa->p, kbuf + len);

    len += meta.dsakey.pLen;
    meta.dsakey.qLen = BN_bn2bin(dsa->q, kbuf + len);

    len += meta.dsakey.qLen;
    meta.dsakey.gLen = BN_bn2bin(dsa->g, kbuf + len);

    len += meta.dsakey.gLen;
    meta.dsakey.yLen = BN_bn2bin(dsa->pub_key, kbuf + len);

    len += meta.dsakey.yLen;
    meta.dsakey.xLen = BN_bn2bin(dsa->priv_key, kbuf + len);

    len += meta.dsakey.xLen;

    memcpy(kbuf, &meta, sizeof(meta));

    *key_len = len;

    return 0;
}

static int km_import_ec_keypair(EVP_PKEY *pkey, uint8_t *kbuf, uint32_t *key_len) {
    teeKeyMeta_t meta;
    uint32_t len = *key_len;
    int ret;

    /* change key format */
    Unique_EC_KEY eckey(EVP_PKEY_get1_EC_KEY(pkey));
    if (eckey.get() == NULL) {
        logOpenSSLError("get ec key format");
	return -1;
    }

    const EC_GROUP *group = EC_KEY_get0_group(eckey.get());
    const EC_POINT *pub_key = EC_KEY_get0_public_key(eckey.get());
    const BIGNUM *priv_key = EC_KEY_get0_private_key(eckey.get());
    Unique_BIGNUM affine_x(BN_new());
    Unique_BIGNUM affine_y(BN_new());

    /* transform to affine coordinate */
    ret = EC_POINT_get_affine_coordinates_GFp(group, pub_key, affine_x.get(),
					affine_y.get(), NULL);
    if (!ret) {
	logOpenSSLError("get affine coordinates");
	return -1;
    }

    meta.keytype = TEE_KEYTYPE_ECDSA;
    len += sizeof(meta);

    /* Add to compare with nist prime */

    switch (EC_GROUP_get_curve_name(group)) {
    case NID_X9_62_prime192v1:
	meta.ecdsakey.curve = TEE_ECC_CURVE_NIST_P192;
	meta.ecdsakey.curveLen = 192 >> 3;
	break;
    case NID_secp224r1:
	meta.ecdsakey.curve = TEE_ECC_CURVE_NIST_P224;
	meta.ecdsakey.curveLen = 224 >> 3;
	break;
    case NID_X9_62_prime256v1:
	meta.ecdsakey.curve = TEE_ECC_CURVE_NIST_P256;
	meta.ecdsakey.curveLen = 256 >> 3;
	break;
    case NID_secp384r1:
	meta.ecdsakey.curve = TEE_ECC_CURVE_NIST_P384;
	meta.ecdsakey.curveLen = 384 >> 3;
	break;
    case NID_secp521r1:
	meta.ecdsakey.curve = TEE_ECC_CURVE_NIST_P521;
	meta.ecdsakey.curveLen = (521 >> 3) + 1;
	break;
    default:
	ALOGE("curve(%d) is not valid\n", EC_GROUP_get_curve_name(group));
	return -1;
    }

    len += BN_bn2bin(affine_x.get(), kbuf + len);
    len += BN_bn2bin(affine_y.get(), kbuf + len);
    if (len != (sizeof(meta) + 2 * meta.ecdsakey.curveLen)) {
	ALOGE("got invalid public key(len: 0x%x)\n", len);
	return -1;
    }

    len += BN_bn2bin(priv_key, kbuf + len);
    if (len != (sizeof(meta) + 3 * meta.ecdsakey.curveLen)) {
	ALOGE("got invalid private key");
	return -1;
    }

    memcpy(kbuf, &meta, sizeof(meta));

    *key_len = len;

    return 0;
}

static int exynos_km_import_keypair(const keymaster0_device_t*,
				const uint8_t* key, const size_t key_length,
				uint8_t** key_blob, size_t* key_blob_length) {
    uint8_t kbuf[KEY_BUFFER_SIZE];
    uint32_t key_len = 0;
    teeResult_t ret = TEE_ERR_NONE;

    if (key == NULL) {
        ALOGE("input key == NULL");
        return -1;
    } else if (key_blob == NULL || key_blob_length == NULL) {
        ALOGE("output key blob or length == NULL");
        return -1;
    }

    /* decoding */
    Unique_PKCS8_PRIV_KEY_INFO pkcs8(d2i_PKCS8_PRIV_KEY_INFO(NULL,
						&key, key_length));
    if (pkcs8.get() == NULL) {
        logOpenSSLError("pkcs4.get");
        return -1;
    }

    /* assign to EVP */
    Unique_EVP_PKEY pkey(EVP_PKCS82PKEY(pkcs8.get()));
    if (pkey.get() == NULL) {
        logOpenSSLError("pkey.get");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(pkcs8);

    int type = EVP_PKEY_type(pkey->type);
    if (type == EVP_PKEY_RSA) {
	if (km_import_rsa_keypair(pkey.get(), kbuf, &key_len))
	    return -1;
    } else if (type == EVP_PKEY_DSA) {
	if (km_import_dsa_keypair(pkey.get(), kbuf, &key_len))
	    return -1;
    } else if (type == EVP_PKEY_EC) {
	if (km_import_ec_keypair(pkey.get(), kbuf, &key_len))
	    return -1;
    } else {
	ALOGE("Unsupported key type");
	return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> outPtr(reinterpret_cast<uint8_t*>(malloc(KEY_BUFFER_SIZE)));
    if (outPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    *key_blob_length = KEY_BUFFER_SIZE;

    ret = TEE_KeyImport(kbuf, key_len, outPtr.get(), (uint32_t *)key_blob_length);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_KeyImport() is failed: %d", ret);
        return -1;
    }

    *key_blob = outPtr.release();

    return ret;
}

static int km_get_rsa_keypair_public(uint8_t *pubkey, EVP_PKEY *pkey) {
    teePubKeyMeta_t *meta;
    uint8_t *modulus;
    uint8_t *pubexp;
    size_t mod_len;
    size_t exp_len;

    meta = (teePubKeyMeta_t*)pubkey;
    modulus = pubkey + sizeof(teePubKeyMeta_t);
    mod_len = meta->rsakey.lenpubmod;
    pubexp = pubkey + sizeof(teePubKeyMeta_t) + mod_len;
    exp_len = meta->rsakey.lenpubexp;

    Unique_BIGNUM bn_mod(BN_new());
    if (bn_mod.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_exp(BN_new());
    if (bn_exp.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    BN_bin2bn(modulus, mod_len, bn_mod.get());
    BN_bin2bn(pubexp, exp_len, bn_exp.get());

    /* assign to RSA */
    Unique_RSA rsa(RSA_new());
    if (rsa.get() == NULL) {
        logOpenSSLError("rsa.get");
        return -1;
    }

    RSA* rsa_tmp = rsa.get();

    rsa_tmp->n = bn_mod.release();
    rsa_tmp->e = bn_exp.release();

    /* assign to EVP */
    if (EVP_PKEY_assign_RSA(pkey, rsa.get()) == 0) {
        logOpenSSLError("assing RSA to EVP_PKEY");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(rsa);

    return 0;
}

static int km_get_dsa_keypair_public(uint8_t *pubkey, EVP_PKEY *pkey) {
    teePubKeyMeta_t *meta;
    uint8_t *p, *q, *g, *y;
    size_t p_len, q_len, g_len, y_len;

    meta = (teePubKeyMeta_t*)pubkey;
    p = pubkey + sizeof(teePubKeyMeta_t);
    p_len = meta->dsakey.pLen;
    q = pubkey + sizeof(teePubKeyMeta_t) + p_len;
    q_len = meta->dsakey.qLen;
    g = pubkey + sizeof(teePubKeyMeta_t) + p_len + q_len;
    g_len = meta->dsakey.gLen;
    y = pubkey + sizeof(teePubKeyMeta_t) + p_len + q_len + g_len;
    y_len = meta->dsakey.yLen;

    Unique_BIGNUM bn_p(BN_new());
    if (bn_p.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_q(BN_new());
    if (bn_q.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_g(BN_new());
    if (bn_g.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_y(BN_new());
    if (bn_y.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    BN_bin2bn(p, p_len, bn_p.get());
    BN_bin2bn(q, q_len, bn_q.get());
    BN_bin2bn(g, g_len, bn_g.get());
    BN_bin2bn(y, y_len, bn_y.get());

    Unique_DSA dsa(DSA_new());
    if (dsa.get() == NULL) {
        logOpenSSLError("dsa.get");
        return -1;
    }

    DSA* dsa_tmp = dsa.get();

    dsa_tmp->p = bn_p.release();
    dsa_tmp->q = bn_q.release();
    dsa_tmp->g = bn_g.release();
    dsa_tmp->pub_key = bn_y.release();

    /* assign to EVP */
    if (EVP_PKEY_assign_DSA(pkey, dsa.get()) == 0) {
        logOpenSSLError("assing DSA to EVP_PKEY");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(dsa);

    return 0;
}

static int km_get_ecdsa_keypair_public(uint8_t *pubkey, EVP_PKEY *pkey) {
    teePubKeyMeta_t *meta;
    uint8_t *x, *y;
    size_t x_len, y_len, curve_len;
    int curve;
    int ret;

    meta = (teePubKeyMeta_t*)pubkey;
    switch (meta->ecdsakey.curve) {
    case TEE_ECC_CURVE_NIST_P192:
	curve = NID_X9_62_prime192v1;
	curve_len = 192 >> 3;
	break;
    case TEE_ECC_CURVE_NIST_P224:
	curve = NID_secp224r1;
	curve_len = 224 >> 3;
	break;
    case TEE_ECC_CURVE_NIST_P256:
	curve = NID_X9_62_prime256v1;
	curve_len = 256 >> 3;
	break;
    case TEE_ECC_CURVE_NIST_P384:
	curve = NID_secp384r1;
	curve_len = 384 >> 3;
	break;
    case TEE_ECC_CURVE_NIST_P521:
	curve = NID_secp521r1;
	curve_len = (521 >> 3) + 1;
	break;
    default:
	ALOGE("curve(%d) is not valid\n", meta->ecdsakey.curve);
	return -1;
    }

    EC_GROUP *group;

    /* Get group with curve */
    group = EC_GROUP_new_by_curve_name(curve);
    if (!group) {
        ALOGE("fail to get group");
	return -1;
    }

    x = pubkey + sizeof(teePubKeyMeta_t);
    y = pubkey + sizeof(teePubKeyMeta_t) + curve_len;

    Unique_BIGNUM bn_x(BN_new());
    if (bn_x.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_y(BN_new());
    if (bn_y.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    BN_bin2bn(x, curve_len, bn_x.get());
    BN_bin2bn(y, curve_len, bn_y.get());

    Unique_EC_KEY eckey(EC_KEY_new());
    if (eckey.get() == NULL) {
	logOpenSSLError("eckey.get");
        return -1;
    }

    ret = EC_KEY_set_group(eckey.get(), group);
    if (!ret) {
	logOpenSSLError("set group");
	return -1;
    }

    ret = EC_KEY_set_public_key_affine_coordinates(eckey.get(), bn_x.release(), bn_y.release());
    if (!ret) {
	logOpenSSLError("set affine coordinates");
	return -1;
    }

    /* assign to EVP */
    if (EVP_PKEY_assign_EC_KEY(pkey, eckey.get()) == 0) {
        logOpenSSLError("assing EC_KEY to EVP_PKEY");
        return -1;
    }
    OWNERSHIP_TRANSFERRED(eckey);

    return 0;
}

static int exynos_km_get_keypair_public(const struct keymaster0_device*,
				const uint8_t* key_blob, const size_t key_blob_length,
				uint8_t** x509_data, size_t* x509_data_length) {
    teeResult_t ret = TEE_ERR_NONE;

    Unique_EVP_PKEY pkey(EVP_PKEY_new());
    if (pkey.get() == NULL) {
        logOpenSSLError("allocate EVP_PKEY");
        return -1;
    }

    if (x509_data == NULL || x509_data_length == NULL) {
        ALOGE("output public key buffer == NULL");
        return -1;
    }

    size_t pubkey_len = sizeof(teePubKeyMeta_t) + PUBKEY_BUFFER_SIZE;

    UniquePtr<uint8_t, Malloc_Free> pubkey(reinterpret_cast<uint8_t*>(malloc(pubkey_len)));
    if (pubkey.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    ret = TEE_GetPubKey(key_blob, (uint32_t)key_blob_length, pubkey.get(), (uint32_t *)&pubkey_len);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_GetPubKey() is failed: %d", ret);
        return -1;
    }

    teePubKeyMeta_t *meta = (teePubKeyMeta_t*)pubkey.get();
    switch (meta->keytype) {
    case TEE_KEYTYPE_RSA:
	if (km_get_rsa_keypair_public(pubkey.get(), pkey.get()))
	    return -1;
	break;
    case TEE_KEYTYPE_DSA:
	if (km_get_dsa_keypair_public(pubkey.get(), pkey.get()))
	    return -1;
	break;
    case TEE_KEYTYPE_ECDSA:
	if (km_get_ecdsa_keypair_public(pubkey.get(), pkey.get()))
	    return -1;
	break;
    default:
	ALOGE("Unsupported key type");
	return -1;
    }

    /* change to x.509 format */
    int len = i2d_PUBKEY(pkey.get(), NULL);
    if (len <= 0) {
        logOpenSSLError("i2d_PUBKEY");
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> key(static_cast<uint8_t*>(malloc(len)));
    if (key.get() == NULL) {
        ALOGE("Could not allocate memory for public key data");
        return -1;
    }

    unsigned char* tmp = reinterpret_cast<unsigned char*>(key.get());
    if (i2d_PUBKEY(pkey.get(), &tmp) != len) {
        logOpenSSLError("Compare results");
        return -1;
    }

    *x509_data_length = len;
    *x509_data = key.release();

    return ret;
}

static int km_rsa_sign_data(const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* data,
			const size_t dataLength, uint8_t** signedData,
			size_t* signedDataLength) {
    teeResult_t ret = TEE_ERR_NONE;

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGE("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> signedDataPtr(reinterpret_cast<uint8_t*>(malloc(RSA_SIG_MAX_SIZE)));
    if (signedDataPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    *signedDataLength = RSA_SIG_MAX_SIZE;

    /* binder gives us read-only mappings we can't use with mobicore */
    void *tmpData = malloc(dataLength);
    memcpy(tmpData, data, dataLength);
    ret = TEE_RSASign(keyBlob, (uint32_t)keyBlobLength, (const uint8_t *)tmpData,
		dataLength, signedDataPtr.get(), (uint32_t *)signedDataLength,
		TEE_RSA_NODIGEST_NOPADDING);
    free(tmpData);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSASign() is failed: %d", ret);
        return -1;
    }

    *signedData = signedDataPtr.release();

    return ret;
}

static int km_dsa_sign_data(const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* data,
			const size_t dataLength, uint8_t** signedData,
			size_t* signedDataLength) {
    teeResult_t ret = TEE_ERR_NONE;

    keymaster_dsa_sign_params_t* sign_params = (keymaster_dsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free>
    signedDataPtr(reinterpret_cast<uint8_t*>(malloc(DSA_SIG_MAX_SIZE)));
    if (signedDataPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }
    uint8_t *tmp_sig = signedDataPtr.get();

    *signedDataLength = DSA_SIG_MAX_SIZE;

    /* binder gives us read-only mappings we can't use with mobicore */
    void *tmpData = malloc(dataLength);
    memcpy(tmpData, data, dataLength);
    ret = TEE_DSASign(keyBlob, keyBlobLength, (const uint8_t *)tmpData,
		dataLength, signedDataPtr.get(), (uint32_t *)signedDataLength);
    free(tmpData);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_DSASign() is failed: %d", ret);
        return -1;
    }

    /* DER encode signature */
    unsigned int slen = *signedDataLength/2;

    Unique_BIGNUM bn_sig_r(BN_new());
    if (bn_sig_r.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    Unique_BIGNUM bn_sig_s(BN_new());
    if (bn_sig_s.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    DSA_SIG *s = DSA_SIG_new();
    if (!s) {
       ALOGE("memory allocation is failed");
       return -1;
    }

    s->r = bn_sig_r.get();
    s->s = bn_sig_s.get();

    BN_bin2bn(tmp_sig, slen, s->r);
    BN_bin2bn(tmp_sig + slen, slen, s->s);

    UniquePtr<uint8_t, Malloc_Free> encodedSig(reinterpret_cast<uint8_t*>(malloc(DSA_SIG_MAX_SIZE)));
    if (encodedSig.get() == NULL) {
        ALOGE("memory allocation is failed");
        DSA_SIG_free(s);
        return -1;
    }
    uint8_t *tmp = encodedSig.get();

    *signedDataLength = i2d_DSA_SIG(s, &tmp);

    *signedData = encodedSig.release();

    DSA_SIG_free(s);

    return ret;
}

static int km_ecdsa_sign_data(const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* data,
			const size_t dataLength, uint8_t** signedData,
			size_t* signedDataLength) {
    teeResult_t ret = TEE_ERR_NONE;

    keymaster_ec_sign_params_t* sign_params = (keymaster_ec_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free>
    signedDataPtr(reinterpret_cast<uint8_t*>(malloc(ECDSA_SIG_MAX_SIZE)));
    if (signedDataPtr.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }
    uint8_t *tmp_sig = signedDataPtr.get();

    *signedDataLength = ECDSA_SIG_MAX_SIZE;

    /* binder gives us read-only mappings we can't use with mobicore */
    void *tmpData = malloc(dataLength);
    memcpy(tmpData, data, dataLength);
    ret = TEE_ECDSASign(keyBlob, (uint32_t)keyBlobLength, (const uint8_t *)tmpData,
			dataLength, signedDataPtr.get(), (uint32_t *)signedDataLength);
    free(tmpData);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_ECDSASign() is failed: %d", ret);
        return -1;
    }

    /* DER encode signature */
    unsigned int curve = *signedDataLength/2;
    ECDSA_SIG *s = ECDSA_SIG_new();
    if (!s) {
       ALOGE("memory allocation is failed");
       return -1;
    }

    BN_bin2bn(tmp_sig, curve, s->r);
    BN_bin2bn(tmp_sig + curve, curve, s->s);

    UniquePtr<uint8_t, Malloc_Free> encodedSig(reinterpret_cast<uint8_t*>(malloc(ECDSA_SIG_MAX_SIZE)));
    if (encodedSig.get() == NULL) {
        ALOGE("memory allocation is failed");
        ECDSA_SIG_free(s);
        return -1;
    }
    uint8_t *tmp = encodedSig.get();

    *signedDataLength = i2d_ECDSA_SIG(s, &tmp);

    *signedData = encodedSig.release();

    ECDSA_SIG_free(s);

    return ret;
}

static int exynos_km_sign_data(const keymaster0_device_t*, const void* params,
		        const uint8_t* keyBlob, const size_t keyBlobLength,
		        const uint8_t* data, const size_t dataLength,
		        uint8_t** signedData, size_t* signedDataLength) {
    teeResult_t ret = TEE_ERR_NONE;
    teeKeyMeta_t meta;

    if (data == NULL) {
        ALOGE("input data to sign == NULL");
        return -1;
    } else if (signedData == NULL || signedDataLength == NULL) {
        ALOGE("output signature buffer == NULL");
        return -1;
    }

    /* find the algorithm */
    ret = TEE_GetKeyInfo(keyBlob, keyBlobLength, &meta);
    if (ret != TEE_ERR_NONE) {
	ALOGE("TEE_GetKeyInfo() is failed: %d", ret);
	return -1;
    }

    switch (meta.keytype) {
    case TEE_KEYTYPE_RSA:
	if (km_rsa_sign_data(params, keyBlob, keyBlobLength, data,
			dataLength, signedData, signedDataLength))
	    return -1;
	break;
    case TEE_KEYTYPE_DSA:
	if (km_dsa_sign_data(params, keyBlob, keyBlobLength, data,
			dataLength, signedData, signedDataLength))
	    return -1;
	break;
    case TEE_KEYTYPE_ECDSA:
	if (km_ecdsa_sign_data(params, keyBlob, keyBlobLength, data,
			dataLength, signedData, signedDataLength))
	    return -1;
	break;
    default:
	ALOGE("Invalid key type");
	return -1;
    }

    return ret;
}

static int km_rsa_verify_data(const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* signedData,
			const size_t signedDataLength, const uint8_t* signature,
			const size_t signatureLength, bool* result) {
    teeResult_t ret = TEE_ERR_NONE;

    keymaster_rsa_sign_params_t* sign_params = (keymaster_rsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    } else if (sign_params->padding_type != PADDING_NONE) {
        ALOGE("Cannot handle padding type %d", sign_params->padding_type);
        return -1;
    } else if (signatureLength != signedDataLength) {
        ALOGE("signed data length must be signature length");
        return -1;
    }

    /* binder gives us read-only mappings we can't use with mobicore */
    void *tmpSignedData = malloc(signedDataLength);
    void *tmpSig = malloc(signatureLength);

    memcpy(tmpSignedData, signedData, signedDataLength);
    memcpy(tmpSig, signature, signatureLength);

    ret = TEE_RSAVerify(keyBlob, keyBlobLength, (const uint8_t*)tmpSignedData, signedDataLength, (const uint8_t *)tmpSig,
			signatureLength, TEE_RSA_NODIGEST_NOPADDING, result);

    free(tmpSignedData);
    free(tmpSig);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSAVerify() is failed: %d", ret);
        return -1;
    }

    return ret;
}

static int km_dsa_verify_data(const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* signedData,
			const size_t signedDataLength, const uint8_t* signature,
			const size_t signatureLength, bool* result) {
    teeResult_t ret = TEE_ERR_NONE;

    keymaster_dsa_sign_params_t* sign_params = (keymaster_dsa_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> tmpData(reinterpret_cast<uint8_t*>(malloc(signedDataLength)));
    if (tmpData.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }
    memcpy(tmpData.get(), signedData, signedDataLength);

    UniquePtr<uint8_t, Malloc_Free> encSig(reinterpret_cast<uint8_t*>(malloc(signatureLength)));
    if (encSig.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }
    memcpy(encSig.get(), signature, signatureLength);

    /* Decodes a DER encoded signature */
    DSA_SIG *s = DSA_SIG_new();
    if (!s) {
	ALOGE("memory allocation is failed");
	return -1;
    }

    uint8_t *tmpEncSig = encSig.get();
    if (!d2i_DSA_SIG(&s, (const unsigned char**)&tmpEncSig, signatureLength)) {
	ALOGE("failed to decodes a signature");
	return -1;
    }

    size_t rlen, slen;
    UniquePtr<uint8_t, Malloc_Free> sig(reinterpret_cast<uint8_t*>(malloc(signatureLength)));
    if (sig.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    rlen = BN_bn2bin(s->r, sig.get());
    slen = BN_bn2bin(s->s, sig.get() + rlen);

    ret = TEE_DSAVerify(keyBlob, keyBlobLength, (const uint8_t*)tmpData.get(),
			signedDataLength, (const uint8_t *)sig.get(),
			rlen + slen, result);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSAVerify() is failed: %d", ret);
        return -1;
    }

    return ret;
}

static int km_ecdsa_verify_data(const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* signedData,
			const size_t signedDataLength, const uint8_t* signature,
			const size_t signatureLength, bool* result) {
    teeResult_t ret = TEE_ERR_NONE;

    keymaster_ec_sign_params_t* sign_params = (keymaster_ec_sign_params_t*) params;
    if (sign_params->digest_type != DIGEST_NONE) {
        ALOGE("Cannot handle digest type %d", sign_params->digest_type);
        return -1;
    }

    UniquePtr<uint8_t, Malloc_Free> tmpData(reinterpret_cast<uint8_t*>(malloc(signedDataLength)));
    if (tmpData.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }
    memcpy(tmpData.get(), signedData, signedDataLength);

    UniquePtr<uint8_t, Malloc_Free> encSig(reinterpret_cast<uint8_t*>(malloc(signatureLength)));
    if (encSig.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }
    memcpy(encSig.get(), signature, signatureLength);

    /* Decodes a DER encoded signature */
    ECDSA_SIG *s = ECDSA_SIG_new();
    if (!s) {
	ALOGE("memory allocation is failed");
	return -1;
    }

    uint8_t *tmpEncSig = encSig.get();
    if (!d2i_ECDSA_SIG(&s, (const unsigned char**)&tmpEncSig, signatureLength)) {
	ALOGE("failed to decodes a signature");
	return -1;
    }

    size_t rlen, slen;
    UniquePtr<uint8_t, Malloc_Free> sig(reinterpret_cast<uint8_t*>(malloc(signatureLength)));
    if (sig.get() == NULL) {
        ALOGE("memory allocation is failed");
        return -1;
    }

    rlen = BN_bn2bin(s->r, sig.get());
    slen = BN_bn2bin(s->s, sig.get() + rlen);

    ret = TEE_ECDSAVerify(keyBlob, keyBlobLength, (const uint8_t*)tmpData.get(),
			signedDataLength, (const uint8_t *)sig.get(),
			rlen + slen, result);
    if (ret != TEE_ERR_NONE) {
        ALOGE("TEE_RSAVerify() is failed: %d", ret);
        return -1;
    }

   return ret;
}

static int exynos_km_verify_data(const keymaster0_device_t*,
			const void* params, const uint8_t* keyBlob,
			const size_t keyBlobLength, const uint8_t* signedData,
			const size_t signedDataLength, const uint8_t* signature,
			const size_t signatureLength) {
    bool result;
    teeResult_t ret = TEE_ERR_NONE;
    teeKeyMeta_t meta;

    if (signedData == NULL || signature == NULL) {
        ALOGE("data or signature buffers == NULL");
        return -1;
    }

    /* find the algorithm */
    ret = TEE_GetKeyInfo(keyBlob, keyBlobLength, &meta);
    if (ret != TEE_ERR_NONE) {
	ALOGE("TEE_GetKeyInfo() is failed: %d", ret);
	return -1;
    }

    switch (meta.keytype) {
    case TEE_KEYTYPE_RSA:
	if (km_rsa_verify_data(params, keyBlob, keyBlobLength,
			signedData, signedDataLength, signature,
			signatureLength, &result))
	    return -1;
	break;
    case TEE_KEYTYPE_DSA:
	if (km_dsa_verify_data(params, keyBlob, keyBlobLength,
			signedData, signedDataLength, signature,
			signatureLength, &result))
	    return -1;
	break;
    case TEE_KEYTYPE_ECDSA:
	if (km_ecdsa_verify_data(params, keyBlob, keyBlobLength,
			signedData, signedDataLength, signature,
			signatureLength, &result))
	    return -1;
	break;
    default:
	ALOGE("Invalid key type");
	return -1;
    }

    return (result == true) ? 0 : -1;
}

/* Close an opened Exynos KM instance */
static int exynos_km_close(hw_device_t *dev) {
    free(dev);
    return 0;
}

/*
 * Generic device handling
 */
static int exynos_km_open(const hw_module_t* module, const char* name,
        hw_device_t** device) {
    if (strcmp(name, KEYSTORE_KEYMASTER) != 0)
        return -EINVAL;

    Unique_keymaster0_device_t dev(new keymaster0_device_t);
    if (dev.get() == NULL)
        return -ENOMEM;

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 1;
    dev->common.module = (struct hw_module_t*) module;
    dev->common.close = exynos_km_close;

#if defined(KEYMASTER_VER0_3)
    dev->flags = KEYMASTER_BLOBS_ARE_STANDALONE;
#else
    dev->flags = 0;
#endif

    dev->generate_keypair = exynos_km_generate_keypair;
    dev->import_keypair = exynos_km_import_keypair;
    dev->get_keypair_public = exynos_km_get_keypair_public;
    dev->delete_keypair = NULL;
    dev->delete_all = NULL;
    dev->sign_data = exynos_km_sign_data;
    dev->verify_data = exynos_km_verify_data;

    ERR_load_crypto_strings();
    ERR_load_BIO_strings();

    *device = reinterpret_cast<hw_device_t*>(dev.release());

    return 0;
}

static struct hw_module_methods_t keystore_module_methods = {
    open: exynos_km_open,
};

struct keystore_module HAL_MODULE_INFO_SYM
__attribute__ ((visibility ("default"))) = {
    common: {
        tag: HARDWARE_MODULE_TAG,
#if defined(KEYMASTER_VER0_3)
        version_major: 3,
#else
        version_major: 2,
#endif
        version_minor: 0,
        id: KEYSTORE_HARDWARE_MODULE_ID,
        name: "Keymaster Exynos HAL",
        author: "Samsung S.LSI",
        methods: &keystore_module_methods,
        dso: 0,
        reserved: {},
    },
};
