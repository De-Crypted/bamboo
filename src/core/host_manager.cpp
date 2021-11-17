#include "host_manager.hpp"
#include "helpers.hpp"
#include "api.hpp"
#include "constants.hpp"
#include "logger.hpp"
#include "../external/http.hpp"
#include <iostream>
using namespace std;

HostManager::HostManager(json config) {
    for(auto h : config["hostSources"]) {
        this->hostSources.push_back(h);
    }
    this->refreshHostList();
}

HostManager::HostManager() {

}

void HostManager::refreshHostList() {
    json hostList;
    try {
        for (int i = 0; i < this->hostSources.size(); i++) {
            bool foundHost = false;
            try {
                string hostUrl = this->hostSources[i];
                http::Request request{hostUrl};
                const auto response = request.send("GET","",{},std::chrono::milliseconds{TIMEOUT_MS});
                hostList = json::parse(std::string{response.body.begin(), response.body.end()});
                foundHost = true;
                break;
            } catch (...) {
                continue;
            }
            if (!foundHost) throw std::runtime_error("Could not fetch host directory.");
        }

        // if our node is in the host list, remove ourselves:
        string myUrl = "http://" + exec("host $(curl -s http://checkip.amazonaws.com) | tail -c 51 | head -c 49") + ":3000";
        this->hosts.clear();
        for(auto host : hostList) {
            if (host != myUrl) {
                Logger::logStatus("Adding host: " + string(host));
                this->hosts.push_back(string(host));
            }
        }
    } catch (std::exception &e) {
        Logger::logError("HostManager::refreshHostList", string(e.what()));
    }
}

vector<string> HostManager::getHosts() {
    return this->hosts;
}

size_t HostManager::size() {
    return this->hosts.size();
}

std::pair<string, int> HostManager::getLongestChainHost() {
    // TODO: make this asynchronous
    string bestHost = "";
    int bestCount = 0;
    for(auto host : this->hosts) {
        try {
            int curr = getCurrentBlockCount(host);
            if (curr > bestCount) {
                bestCount = curr;
                bestHost = host;
            }
        } catch (std::exception & e) {
            continue;
        }
    }
    if (bestHost == "") throw std::runtime_error("Could not get chain length from any host");
    return std::pair<string, int>(bestHost, bestCount);
}