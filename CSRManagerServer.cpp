/*
    Copyright (C) 2019-Present SKALE Labs

    This file is part of sgxwallet.

    sgxwallet is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    sgxwallet is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with sgxwallet.  If not, see <https://www.gnu.org/licenses/>.

    @file CSRManager.cpp
    @author Stan Kladko
    @date 2019
*/


#include "CSRManagerServer.h"
#include "SGXException.h"
#include "sgxwallet_common.h"

#include <iostream>
#include <fstream>

#include <jsonrpccpp/server/connectors/httpserver.h>

#include "spdlog/spdlog.h"
#include "common.h"


CSRManagerServer *cs = nullptr;
jsonrpc::HttpServer *hs3 = nullptr;


CSRManagerServer::CSRManagerServer(AbstractServerConnector &connector,
                                   serverVersion_t type) : abstractCSRManagerServer(connector, type) {}


Json::Value getUnsignedCSRsImpl() {
    spdlog::info("Enter getUnsignedCSRsImpl");
    Json::Value result;
    result["status"] = 0;
    result["errorMessage"] = "";

    try {
        vector<string> hashes_vect = LevelDB::getCsrDb()->writeKeysToVector1(MAX_CSR_NUM);
        for (int i = 0; i < (int) hashes_vect.size(); i++) {
            result["hashes"][i] = hashes_vect.at(i);
        }
    } catch (SGXException &_e) {
        cerr << " err str " << _e.errString << endl;
        result["status"] = _e.status;
        result["errorMessage"] = _e.errString;

    }

    return result;
}

Json::Value signByHashImpl(const string &hash, int status) {
    Json::Value result;
    result["errorMessage"] = "";

    try {
        if (!(status == 0 || status == 2)) {
            throw SGXException(-111, "Invalid csr status");
        }

        string csr_db_key = "CSR:HASH:" + hash;
        shared_ptr<string> csr_ptr = LevelDB::getCsrDb()->readString(csr_db_key);
        if (csr_ptr == nullptr) {
            throw SGXException(KEY_SHARE_DOES_NOT_EXIST, "HASH DOES NOT EXIST IN DB");
        }

        if (status == 0) {
            string csr_name = "sgx_data/cert/" + hash + ".csr";
            ofstream outfile(csr_name);
            outfile << *csr_ptr << endl;
            outfile.close();
            if (access(csr_name.c_str(), F_OK) != 0) {
                LevelDB::getCsrDb()->deleteKey(csr_db_key);
                throw SGXException(FILE_NOT_FOUND, "Csr does not exist");
            }

            string signClientCert = "cd sgx_data/cert && ./create_client_cert " + hash;

            if (system(signClientCert.c_str()) == 0) {
                spdlog::info("CLIENT CERTIFICATE IS SUCCESSFULLY GENERATED");
            } else {
                spdlog::info("CLIENT CERTIFICATE GENERATION FAILED");
                LevelDB::getCsrDb()->deleteKey(csr_db_key);
                string status_db_key = "CSR:HASH:" + hash + "STATUS:";
                LevelDB::getCsrStatusDb()->deleteKey(status_db_key);
                LevelDB::getCsrStatusDb()->writeDataUnique(status_db_key, "-1");
                throw SGXException(FAIL_TO_CREATE_CERTIFICATE, "CLIENT CERTIFICATE GENERATION FAILED");
                //exit(-1);
            }
        }

        LevelDB::getCsrDb()->deleteKey(csr_db_key);
        string status_db_key = "CSR:HASH:" + hash + "STATUS:";
        LevelDB::getCsrStatusDb()->deleteKey(status_db_key);
        LevelDB::getCsrStatusDb()->writeDataUnique(status_db_key, to_string(status));

        result["status"] = status;

    } catch (SGXException &_e) {
        cerr << " err str " << _e.errString << endl;
        result["status"] = _e.status;
        result["errorMessage"] = _e.errString;
    }

    return result;
}


Json::Value CSRManagerServer::getUnsignedCSRs() {
    lock_guard<recursive_mutex> lock(m);
    return getUnsignedCSRsImpl();
}

Json::Value CSRManagerServer::signByHash(const string &hash, int status) {
    lock_guard<recursive_mutex> lock(m);
    return signByHashImpl(hash, status);
}

int init_csrmanager_server() {
    hs3 = new jsonrpc::HttpServer(BASE_PORT + 2);
    hs3->BindLocalhost();
    cs = new CSRManagerServer(*hs3, JSONRPC_SERVER_V2); // server (json-rpc 2.0)

    if (!cs->StartListening()) {
        spdlog::info("CSR manager server could not start listening");
        exit(-1);
    } else {
        spdlog::info("CSR manager server started on port {}", BASE_PORT + 2);
    }
    return 0;
};