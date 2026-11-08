#include "JsonProtocol.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: cyberpath_engine <request.json> <response.json>\n";
        return 2;
    }

    const string requestPath = argv[1];
    const string responsePath = argv[2];
    cyberpath::Json response;
    try {
        ifstream requestFile(requestPath);
        if (!requestFile) {
            response = {{"schema_version", 1}, {"success", false},
                        {"algorithm", nullptr},
                        {"error", {{"code", "REQUEST_FILE_ERROR"},
                                   {"message", "Could not open request file."}}}};
        } else {
            try {
                const cyberpath::Json request = cyberpath::Json::parse(requestFile);
                response = cyberpath::handleJsonRequest(request);
            } catch (const cyberpath::Json::parse_error& error) {
                response = {{"schema_version", 1}, {"success", false},
                            {"algorithm", nullptr},
                            {"error", {{"code", "INVALID_JSON"},
                                       {"message", error.what()}}}};
            }
        }

        ofstream responseFile(responsePath, ios::trunc);
        if (!responseFile) {
            cerr << "Could not open response file: " << responsePath << '\n';
            return 2;
        }
        responseFile << response.dump(2) << '\n';
        responseFile.close();
        if (!responseFile) {
            cerr << "Could not write response file: " << responsePath << '\n';
            return 2;
        }
        if (!response.value("success", false)) {
            cerr << response.at("error").at("message").get<string>() << '\n';
            return 1;
        }
        return 0;
    } catch (const exception& error) {
        cerr << "Engine error: " << error.what() << '\n';
        return 2;
    }
}
