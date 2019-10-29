//
// Created by kladko on 10/1/19.
//

#include <stdlib.h>
#include <../tgmp-build/include/sgx_tgmp.h>
#include <stdbool.h>
#include "domain_parameters.h"
#include "curves.h"
#include "point.h"
#include "numbertheory.h"

#include <stdint.h>
#include "BLSEnclave.h"
#include <string.h>

void gen_session_key(char *skey_str, char* pb_keyB, char* common_key){

    char* pb_keyB_x = (char*)malloc(65);
    strncpy(pb_keyB_x, pb_keyB, 64);

    char* pb_keyB_y = (char*)malloc(65);
    strncpy(pb_keyB_y, pb_keyB + 64, 64);

    domain_parameters curve = domain_parameters_init();
    domain_parameters_load_curve(curve, secp256k1);

    mpz_t skey;
    mpz_init(skey);
    mpz_set_str(skey, skey_str, 16);

    point pub_keyB = point_init();
    point_set_hex(pub_keyB, pb_keyB_x, pb_keyB_y);

    point session_key = point_init();
    point_multiplication(session_key, skey, pub_keyB, curve);

    char arr_x[mpz_sizeinbase (session_key->x, 16) + 2];
    char* x = mpz_get_str(arr_x, 16, session_key->x);
    //strncpy(common_key, arr_x, 64);
    int n_zeroes = 64 - strlen(arr_x);
    for ( int i = 0; i < n_zeroes; i++){
      common_key[i] = '0';
    }
    strncpy(common_key + n_zeroes, arr_x, strlen(arr_x));

    mpz_clear(skey);
    point_clear(pub_keyB);
    domain_parameters_clear(curve);
    free(pb_keyB_x);
    free(pb_keyB_y);
}

void session_key_recover(const char *skey_str, const char* sshare, char* common_key){

    char* pb_keyB_x = (char*)malloc(65);
    strncpy(pb_keyB_x, sshare + 64, 64);

    char* pb_keyB_y = (char*)malloc(65);
    strncpy(pb_keyB_y, sshare + 128, 64);

    domain_parameters curve = domain_parameters_init();
    domain_parameters_load_curve(curve, secp256k1);

    mpz_t skey;
    mpz_init(skey);
    mpz_set_str(skey, skey_str, 16);

    point pub_keyB = point_init();
    point_set_hex(pub_keyB, pb_keyB_x, pb_keyB_y);

    point session_key = point_init();
    point_multiplication(session_key, skey, pub_keyB, curve);

    char arr_x[mpz_sizeinbase (session_key->x, 16) + 2];
    char* x = mpz_get_str(arr_x, 16, session_key->x);
    //strncpy(common_key, arr_x, 64);
    int n_zeroes = 64 - strlen(arr_x);
    for ( int i = 0; i < n_zeroes; i++){
        common_key[i] = '0';
    }
    strncpy(common_key + n_zeroes, arr_x, strlen(arr_x));
    //strncpy(common_key , pb_keyB_x, 64);

    mpz_clear(skey);
    point_clear(pub_keyB);
    domain_parameters_clear(curve);
    free(pb_keyB_x);
    free(pb_keyB_y);
}

void xor_encrypt(char* key, char* message, char* cypher){

   uint8_t cypher_bin[33];

   //uint8_t key_bin[33];
   uint8_t* key_bin = (uint8_t*)malloc(33);
   uint64_t key_length;
   hex2carray(key, &key_length, key_bin);

   uint64_t msg_length;
   uint8_t msg_bin[33];//[ECDSA_BIN_LEN];
   hex2carray(message, &msg_length, msg_bin);


   for (int i = 0; i < 32; i++){
     cypher_bin[i] = msg_bin[i] ^ key_bin[i];
   }

   carray2Hex(cypher_bin, 32, cypher);

   free(key_bin);
}

void xor_decrypt(char* key, char* cypher, char* message){

    uint8_t msg_bin[33];

    //uint8_t key_bin[33];
    uint8_t* key_bin = (uint8_t*)malloc(33);
    uint64_t key_length;
    hex2carray(key, &key_length, key_bin);

    uint64_t cypher_length;
    uint8_t cypher_bin[33];//[ECDSA_BIN_LEN];
    hex2carray(cypher, &cypher_length, cypher_bin);


    for (int i = 0; i < 32; i++){
        msg_bin[i] = cypher_bin[i] ^ key_bin[i];
    }

    carray2Hex(msg_bin, 32, message);

    free(key_bin);
}
