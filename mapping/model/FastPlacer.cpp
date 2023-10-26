#include "FastPlacer.h"
#include <algorithm>
#include <thread>
#include <mutex>
#include <atomic>
using namespace std;

#define cgrame_arch 1
namespace CGRA
{
std::pair<std::unordered_map<std::string, std::string>, FastRouter> FastPlacer::place_matching(const Graph &DFG, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatible, const FastRouter &routerInit, 
                                                        const std::unordered_map<std::string, std::string>  &usedDFG2RRGInitial,  const NetworkAnalyzerLegacy &analyzerInitial, const NOrderValidator &validatorInitial,
                                                        const std::vector<std::string> &order, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatNew)
{
    const size_t Max_Failed_Times = 64;

    // Clear the previous status
    _failedVertices.clear();
    // Data
    FastRouter router(routerInit);
    vector<string> Goorder = order;
    unordered_map<string, string> vertexDFG2RRG = usedDFG2RRGInitial;
    unordered_map<string, string> vertexRRG2DFG;
    unordered_map<string, bool>   travesalVertexDFG;
    unordered_map<string, unordered_set<string>> nodeCompat = compatNew;

    // unordered_map<string, unordered_set<string>> coincompat = compatible;
    stack<vector<pair<string, string>>> stackEdgesToMap;
    stack<unordered_set<string>> stackVerticesToTry;
    for(const auto &dfg2rrg: vertexDFG2RRG){
        vertexRRG2DFG[dfg2rrg.second] = dfg2rrg.first;
    }
    vector<string> contractVertex;
    unordered_map<string, unordered_set<string>> contractCompat;
    unordered_map<string, unordered_set<string>> contractPorts;
    for(const auto &item: compatible){
        if (getPostfix(item.first).empty()) {
                contractVertex.emplace_back(item.first);
                contractCompat.emplace(item);
        }
    }

    for(const auto &vertexDFG: DFG.vertices())
    {
        if(vertexDFG2RRG.find(vertexDFG.first) != vertexDFG2RRG.end()){
            travesalVertexDFG[vertexDFG.first] = true;
        }
        else{
            travesalVertexDFG[vertexDFG.first] = false;
        }
        if(!getPostfix(vertexDFG.first).empty()){
            string prefix = getPrefix(vertexDFG.first);
            if (contractPorts.find(prefix) == contractPorts.end()) {
                contractPorts[prefix] = {};
            }
            contractPorts[prefix].emplace(getPostfix(vertexDFG.first));
        }
    }

    //N-order validation
    NetworkAnalyzerLegacy analyzer(analyzerInitial);
    Graph &RRGAnalyzed = analyzer.RRG();
    NOrderValidator validator(validatorInitial);
    // size_t unplacibleCount = 0;
    // for (const auto &vertexDFG : coarseDFG.vertices()) {
    //     if (!getPostfix(vertexDFG.first).empty() || vertexDFG.first.find("block") != string::npos) {
    //         continue;
    //     }
    //     if (coincompat.find(vertexDFG.first) == coincompat.end()) {
    //         WARN << "FastPlacement: Compatible vertices NOT FOUND: " + vertexDFG.first;
    //         return {unordered_map<string, string>(), router};
    //     }
    //     unordered_set<string> compatibles;
    //     for(const auto &vertexRRG: coincompat[vertexDFG.first]){
    //         clog << "\rFastPlacement: -> Validating " << vertexDFG.first << " : " << vertexRRG << "            ";
    //         if (validator.validateSlow(vertexDFG.first, vertexRRG, 2)) { //NorderValidate
    //             compatibles.insert(vertexRRG);
    //         }
    //     }
    //     clog << vertexDFG.first << ": " << coincompat[vertexDFG.first].size() << " -> " << compatibles.size() << "            ";
    //     coincompat[vertexDFG.first] = compatibles;
    // }
    // if(unplacibleCount > 0){
    //     clog << "VanillaPlacer: FAILED, uncompatible vertex found in first order validation. " << endl;
    //     return {unordered_map<string, string>(), router};
    // }

    size_t furthest = 0;
    size_t coarseIter = 0;
    size_t failureCount = 0;
    string toMap = Goorder[0];

    unordered_set<string> verticesToTry = contractCompat.find(toMap)->second;
    string arch = "cgrame";
    unordered_map<string, unordered_map<string, vector<string>>> config = Utils::readConifgFile("./arch/arch.ini");
    string RRG = config["Global"]["TopRRG"][0];
    if(RRG.find("cgrame") == string::npos){
        arch = "hycube";
    }
    while(coarseIter < Goorder.size()){
        furthest = max(furthest, coarseIter);
        clog << "FastPlacer: New iteration, size of stack: " << vertexDFG2RRG.size() << " / " << travesalVertexDFG.size() << "; furthest: " << furthest << endl;
        bool failed = false;
        string toTry = "";

        auto prune = [&](string toMap, unordered_set<string> &verticesToTry, bool link) {
            unordered_set<string> toDelete;
            // Prepare to delete used RRG vertices
            for(const auto &vertexToTry: verticesToTry){
                if(vertexRRG2DFG.find(vertexToTry) != vertexRRG2DFG.end()){
                    toDelete.emplace(vertexToTry);
                }
            }
            for(const auto &vertexToTry: verticesToTry){
                if(toDelete.find(vertexToTry) != toDelete.end()){
                    continue;
                }
                // Prepare to unconnectable RRG vertice
               if(link){
                    bool available = true;
                    unordered_multimap<string, string> linksToValidate;
                    unordered_set<string> portVertex;
                    for(const auto &port: contractPorts[toMap]){
                        for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                            if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            const string &fromRRG = vertexDFG2RRG[edge.from()];
                            const string &toRRG = vertexToTry + "." + port;
                            linksToValidate.insert({fromRRG, toRRG});
                        }
                        for(const auto &edge: DFG.edgesOut(toMap + "." + port)){
                            if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            const string &toRRG = vertexDFG2RRG[edge.to()];
                            const string &fromRRG = vertexToTry + "." + port;
                            linksToValidate.insert({fromRRG, toRRG});
                        }
                    }
                    for(const auto &link: linksToValidate){
                        bool found = false;
                        for(const auto &edge: _RRGAnalyzed.edgesOut(link.first)){
                            if(edge.to() == link.second){
                                found = true;
                                break;
                            }
                        }
                        if(!found){
                            available = false;
                            break;
                        }
                    }
                    if(!available){
                        toDelete.insert(vertexToTry);
                    }
               }
            }
            //Delete
            for (const auto &vertex : toDelete) {
                verticesToTry.erase(vertex);
            }
        };
        bool link = false;
        prune(toMap, verticesToTry, link);
  
        
        clog << "FastPlacer: -> toMap: " << toMap << "; Candidates After Purge: " << verticesToTry.size() << endl;
        vector<string> verticesToTryRanked(verticesToTry.begin(), verticesToTry.end());
        vector<pair<string, string>> edgesToMap;
        vector<string> edgesSignal;
        unordered_map<string, string> port2rrg;
        unordered_map<string, size_t> distancedNet;
        unordered_map<string, int> pos_dis;
        auto sortPE_hycube = [&](auto &toMap, auto &verticesToTry, auto &verticesToTryRanked) {
            // unordered_map<string, int> pos_dis;
            if (verticesToTry.empty()) {
                failed = false;
                clog << "FastPlacer: Failed. Nothing to try for " << toMap << endl;
            } else {
                //  sort the candidates by sharedNet
                unordered_map<string, size_t> distancedNet;
                unordered_set<string> inPortRRG;
                unordered_set<string> outPortRRG;
                for(const auto &vertexToTry: verticesToTry){
                    inPortRRG.clear();
                    outPortRRG.clear();
                    int try_x;
                    int try_y;
                    if(vertexToTry.find("MEM") != string::npos){
                        string try_pos = split(vertexToTry,".")[1];
                        string try_mem = split(vertexToTry,".")[2];
                        try_x = str2num<int>(split(try_pos,"_")[1]);
                        try_y = str2num<int>(split(try_mem,"EM")[1]);
                    }
                    else{
                        string try_pos = split(vertexToTry,".")[1];
                        try_x = str2num<int>(split(try_pos,"_")[1]);
                        try_y = str2num<int>(split(try_pos,"_")[2]);
                    }
                    for(const auto &port: contractPorts[toMap]){
                        int dis = 0;
                        for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                            if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            if(vertexDFG2RRG[edge.from()].find("MEM") != string::npos){
                                string placed_pos = split(vertexDFG2RRG[edge.from()],".")[1];
                                int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                                string placed_mem = split(vertexDFG2RRG[edge.from()],".")[2];
                                int placed_y = str2num<int>(split(placed_mem,"EM")[1]);
                                dis = manhattan_distance(placed_x, placed_y, 
                                                        try_x, try_y);
                                pos_dis[vertexToTry] += dis;
                            }
                            else{
                                string placed_cycle = split(vertexDFG2RRG[edge.from()],".")[0];
                                string placed_pos = split(vertexDFG2RRG[edge.from()],".")[1];
                                int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                                int placed_y = str2num<int>(split(placed_pos,"_")[2]);
                                dis = manhattan_distance(placed_x, placed_y, 
                                                        try_x, try_y);
                                pos_dis[vertexToTry] += dis;
                            }
                            string core = getFront(vertexDFG2RRG[edge.from()]);
                            if(vertexToTry.find(core) != string::npos){
                            }
                            inPortRRG.emplace(vertexDFG2RRG[edge.from()]);
                        }
                        for(const auto &edge: DFG.edgesOut(toMap + "." + port)){//out->(in)
                            if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            if(vertexDFG2RRG[edge.to()].find("MEM") != string::npos){
                                string placed_pos = split(vertexDFG2RRG[edge.to()],".")[1];
                                int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                                string placed_mem =split(vertexDFG2RRG[edge.to()],".")[2];
                                int placed_y = str2num<int>(split(placed_mem,"EM")[1]);
                                dis = manhattan_distance(placed_x, placed_y, 
                                                            try_x, try_y);
                                pos_dis[vertexToTry] += dis;
                            }
                            else{
                                string placed_cycle = split(vertexDFG2RRG[edge.to()],".")[0];
                                string placed_pos = split(vertexDFG2RRG[edge.to()],".")[1];
                                int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                                int placed_y = str2num<int>(split(placed_pos,"_")[2]);
                                dis = manhattan_distance(placed_x, placed_y, 
                                                            try_x, try_y);
                                pos_dis[vertexToTry] += dis;
                            }
                            outPortRRG.insert(vertexDFG2RRG[edge.to()]);
                        }
                    }
                }
            }
        };
        

        auto sortPE_cgrmae = [&](auto &toMap, auto &verticesToTry, auto &verticesToTryRanked){
            // unordered_map<string, int> pos_dis;
            if (verticesToTry.empty()) {
                failed = false;
                clog << "FastPlacer: Failed. Nothing to try for " << toMap << endl;
            } else {
                //  sort the candidates by sharedNet
                unordered_map<string, size_t> distancedNet;
                unordered_set<string> inPortRRG;
                unordered_set<string> outPortRRG;
                for(const auto &vertexToTry: verticesToTry){
                    inPortRRG.clear();
                    outPortRRG.clear();
                    for(const auto &port: contractPorts[toMap]){
                        int dis = 0;
                        for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                            if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            string placed_cycle = split(vertexDFG2RRG[edge.from()],".")[0];
                            string placed_pos = split(vertexDFG2RRG[edge.from()],".")[1];
                            int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                            int placed_y = str2num<int>(split(placed_pos,"_")[2]);
                            
                            string try_cycle = split(vertexToTry,".")[0];
                            string try_pos = split(vertexToTry,".")[1];
                            int try_x = str2num<int>(split(try_pos,"_")[1]);
                            int try_y = str2num<int>(split(try_pos,"_")[2]);
                            dis = manhattan_distance(placed_cycle.back(), placed_x, placed_y, 
                                                        try_cycle.back(), try_x, try_y);
                            pos_dis[vertexToTry] += dis;
                            inPortRRG.emplace(vertexDFG2RRG[edge.from()]);
                        }
                        for(const auto &edge: DFG.edgesOut(toMap + "." + port)){//out->(in)
                            if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            string placed_cycle = split(vertexDFG2RRG[edge.to()],".")[0];
                            string placed_pos = split(vertexDFG2RRG[edge.to()],".")[1];
                            int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                            int placed_y = str2num<int>(split(placed_pos,"_")[2]);
                            
                            string try_cycle = split(vertexToTry,".")[0];
                            string try_pos = split(vertexToTry,".")[1];
                            int try_x = str2num<int>(split(try_pos,"_")[1]);
                            int try_y = str2num<int>(split(try_pos,"_")[2]);
                            dis = manhattan_distance(placed_cycle.back(), placed_x, placed_y, 
                                                        try_cycle.back(), try_x, try_y);
                            pos_dis[vertexToTry] += dis;
                            outPortRRG.insert(vertexDFG2RRG[edge.to()]);
                        }
                    }
                }

            }   
        };

        if(arch.find("cgrame") != string::npos){
            sortPE_cgrmae(toMap, verticesToTry, verticesToTryRanked);
        }
        else{
            sortPE_hycube(toMap, verticesToTry, verticesToTryRanked);  
        }

        sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return pos_dis[a] < pos_dis[b];});

        // try map vertex
        bool isSuccess = false;
        size_t IterVertextoTry = 0;
        
        while(!isSuccess && IterVertextoTry < verticesToTryRanked.size()){
            // -> Find edges that need to be mapped
            toTry = verticesToTryRanked[IterVertextoTry++];
            clog << "FastPlacer: try to map " << toMap << " to " << toTry << endl;
            edgesToMap.clear();
            edgesSignal.clear();
            if (!isSuccess) {
                vector<string> portVertex = {};
                port2rrg.clear();
                for(const auto &port: contractPorts[toMap]){
                    port2rrg[toMap + "." + port] = toTry + "." + port;
                    if(port.find("in") != string::npos){
                        edgesToMap.emplace_back(pair<string, string>(toTry + "." + port, toTry));
                        edgesSignal.emplace_back(toMap + "." + port);
                    }
                    else{
                        edgesToMap.emplace_back(pair<string, string>(toTry, toTry + "." + port));
                        edgesSignal.emplace_back(toMap);
                    }
                }
                // travesalVertexDFG[toMap] = true;
                // vertexDFG2RRG[toMap] = toTry;
                for(const auto &vertex: port2rrg){
                    for (const auto &edge : DFG.edgesIn(vertex.first)) {
                        if (travesalVertexDFG[edge.from()] ) {
                            edgesToMap.emplace_back(pair<string, string>(vertexDFG2RRG[edge.from()], vertex.second));
                            edgesSignal.emplace_back(edge.from());
                        }
                    }
                    for (const auto &edge : DFG.edgesOut(vertex.first)) {
                        if (travesalVertexDFG[edge.to()]) {
                            edgesToMap.emplace_back(pair<string, string>(vertex.second, vertexDFG2RRG[edge.to()]));
                            edgesSignal.emplace_back(vertex.first);
                        }
                    }
                }
                // travesalVertexDFG[toMap] = false;
                // vertexDFG2RRG.erase(toMap);
                clog << "FastPlacer: edgesToMap: " << edgesToMap.size() << " for " << toTry << endl;
            }
            if(edgesToMap.size() > 0){
                if(arch.find("cgrame") != string::npos){
                    isSuccess = router.norm_route(edgesToMap, edgesSignal);
                }
                else{
                    isSuccess = router.norm_route1(edgesToMap, edgesSignal);
                }
                    
                // if(!isSuccess){
                //     clog << "FastPlacer: Choose a loose route." << endl;
                //     isSuccess = router.route(edgesToMap, edgesSignal);
                // }
            } else {
                isSuccess = true;
            }
        }
        if(!isSuccess){
            failed = true;
        }

        if(!failed){
            vertexDFG2RRG[toMap] = toTry;
            vertexRRG2DFG[toTry] = toMap;
            travesalVertexDFG[toMap]=true;
            for(const auto &port: port2rrg){
                vertexDFG2RRG[port.first] = port.second;
                vertexRRG2DFG[port.second] = port.first;
                travesalVertexDFG[port.first] = true;
            }
            stackVerticesToTry.push(verticesToTry);
            stackEdgesToMap.push(edgesToMap);
            
            if(++coarseIter < Goorder.size()){
                toMap = Goorder[coarseIter];
                verticesToTry = contractCompat.find(toMap)->second;
            } else {
                break;
            }
        } else {
            failureCount++;
            if (_failedVertices.find(toMap) == _failedVertices.end()) {
                _failedVertices[toMap] = 0;
            }
            _failedVertices[toMap]++;
            if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
                clog << "FastPlacer: FAILED. Too many failure. " << endl;
                return {unordered_map<string, string>(), routerInit};
            }
            if(coarseIter == 0){
                clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
                return {unordered_map<string, string>(), routerInit};
            }
            toMap = Goorder[--coarseIter];
            verticesToTry = stackVerticesToTry.top();
            stackVerticesToTry.pop();
            clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
            vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
            stackEdgesToMap.pop();
            router.unroute(edgesToDelte);
            vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
            vertexDFG2RRG.erase(toMap);
            for(const auto &port: port2rrg){
                vertexDFG2RRG.erase(port.first);
                vertexRRG2DFG.erase(port.second);
                travesalVertexDFG[port.first] = false;
            }
            travesalVertexDFG[toMap]=false;
        }
        clog << endl << endl;

    }

    clog << "FastPlacer: finished placing the DFG. Failure count: " << failureCount << "." << endl
         << endl;

    return{vertexDFG2RRG, router};

}


std::pair<std::unordered_map<std::string, std::string>, FastRouter> FastPlacer::place_matching_temp(const Graph &DFG, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatible, const FastRouter &routerInit, 
                                                        const std::unordered_map<std::string, std::string>  &usedDFG2RRGInitial,  const NetworkAnalyzerLegacy &analyzerInitial, const NOrderValidator &validatorInitial,
                                                        const std::vector<std::string> &order, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatNew)
{
    const size_t Max_Failed_Times = 64;

    // Clear the previous status
    _failedVertices.clear();
    // Data
    FastRouter router(routerInit);
    vector<string> Goorder = order;
    unordered_map<string, string> vertexDFG2RRG = usedDFG2RRGInitial;
    unordered_map<string, string> vertexRRG2DFG;
    unordered_map<string, bool>   travesalVertexDFG;
    unordered_map<string, unordered_set<string>> nodeCompat = compatNew;

    // unordered_map<string, unordered_set<string>> coincompat = compatible;
    stack<vector<pair<string, string>>> stackEdgesToMap;
    stack<unordered_set<string>> stackVerticesToTry;
    for(const auto &dfg2rrg: vertexDFG2RRG){
        vertexRRG2DFG[dfg2rrg.second] = dfg2rrg.first;
    }
    vector<string> contractVertex;
    unordered_map<string, unordered_set<string>> contractCompat;
    unordered_map<string, unordered_set<string>> contractPorts;
    for(const auto &item: compatible){
        if (getPostfix(item.first).empty()) {
                contractVertex.emplace_back(item.first);
                contractCompat.emplace(item);
        }
    }

    for(const auto &vertexDFG: DFG.vertices())
    {
        if(vertexDFG2RRG.find(vertexDFG.first) != vertexDFG2RRG.end()){
            travesalVertexDFG[vertexDFG.first] = true;
        }
        else{
            travesalVertexDFG[vertexDFG.first] = false;
        }
        if(!getPostfix(vertexDFG.first).empty()){
            string prefix = getPrefix(vertexDFG.first);
            if (contractPorts.find(prefix) == contractPorts.end()) {
                contractPorts[prefix] = {};
            }
            contractPorts[prefix].emplace(getPostfix(vertexDFG.first));
        }
    }

    //N-order validation
    NetworkAnalyzerLegacy analyzer(analyzerInitial);
    Graph &RRGAnalyzed = analyzer.RRG();
    NOrderValidator validator(validatorInitial);
    // size_t unplacibleCount = 0;
    // for (const auto &vertexDFG : coarseDFG.vertices()) {
    //     if (!getPostfix(vertexDFG.first).empty() || vertexDFG.first.find("block") != string::npos) {
    //         continue;
    //     }
    //     if (coincompat.find(vertexDFG.first) == coincompat.end()) {
    //         WARN << "FastPlacement: Compatible vertices NOT FOUND: " + vertexDFG.first;
    //         return {unordered_map<string, string>(), router};
    //     }
    //     unordered_set<string> compatibles;
    //     for(const auto &vertexRRG: coincompat[vertexDFG.first]){
    //         clog << "\rFastPlacement: -> Validating " << vertexDFG.first << " : " << vertexRRG << "            ";
    //         if (validator.validateSlow(vertexDFG.first, vertexRRG, 2)) { //NorderValidate
    //             compatibles.insert(vertexRRG);
    //         }
    //     }
    //     clog << vertexDFG.first << ": " << coincompat[vertexDFG.first].size() << " -> " << compatibles.size() << "            ";
    //     coincompat[vertexDFG.first] = compatibles;
    // }
    // if(unplacibleCount > 0){
    //     clog << "VanillaPlacer: FAILED, uncompatible vertex found in first order validation. " << endl;
    //     return {unordered_map<string, string>(), router};
    // }

    size_t furthest = 0;
    size_t coarseIter = 0;
    size_t failureCount = 0;
    string toMap = Goorder[0];

    unordered_set<string> verticesToTry = contractCompat.find(toMap)->second;

    while(coarseIter < Goorder.size()){
        furthest = max(furthest, coarseIter);
        clog << "FastPlacer: New iteration, size of stack: " << vertexDFG2RRG.size() << " / " << travesalVertexDFG.size() << "; furthest: " << furthest << endl;
        bool failed = false;
        string toTry = "";

        auto prune = [&](string toMap, unordered_set<string> &verticesToTry) {
            unordered_set<string> toDelete;
            // Prepare to delete used RRG vertices
            for(const auto &vertexToTry: verticesToTry){
                if(vertexRRG2DFG.find(vertexToTry) != vertexRRG2DFG.end()){
                    toDelete.emplace(vertexToTry);
                }
            }
            for(const auto &vertexToTry: verticesToTry){
                if(toDelete.find(vertexToTry) != toDelete.end()){
                    continue;
                }
                // Prepare to unconnectable RRG vertice
                // bool available = true;
                // unordered_multimap<string, string> linksToValidate;
                // unordered_set<string> portVertex;
                // for(const auto &port: contractPorts[toMap]){
                //     for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                //         if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                //             continue;
                //         }
                //         const string &fromRRG = vertexDFG2RRG[edge.from()];
                //         const string &toRRG = vertexToTry + "." + port;
                //         linksToValidate.insert({fromRRG, toRRG});
                //     }
                //     for(const auto &edge: DFG.edgesOut(toMap + "." + port)){
                //         if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                //             continue;
                //         }
                //         const string &toRRG = vertexDFG2RRG[edge.to()];
                //         const string &fromRRG = vertexToTry + "." + port;
                //         linksToValidate.insert({fromRRG, toRRG});
                //     }
                // }
                // for(const auto &link: linksToValidate){
                //     bool found = false;
                //     for(const auto &edge: _RRGAnalyzed.edgesOut(link.first)){
                //         if(edge.to() == link.second){
                //             found = true;
                //             break;
                //         }
                //     }
                //     if(!found){
                //         available = false;
                //         break;
                //     }
                // }
                // if(!available){
                //     toDelete.insert(vertexToTry);
                // }
            }
            //Delete
            for (const auto &vertex : toDelete) {
                verticesToTry.erase(vertex);
            }
        };
        prune(toMap, verticesToTry);
        unordered_set<string> toDelete;
        // Prepare to delete used RRG vertices
        for(const auto &vertexToTry: verticesToTry){
            if(vertexRRG2DFG.find(vertexToTry) != vertexRRG2DFG.end()){
                toDelete.emplace(vertexToTry);
            }
        }
        bool prun_flag = false;
        //const->mem, mem->func, const->func
        // if(1){
        //     if(toMap.find("const") != string::npos){
        //         string toMapPort = toMap + ".out0";
        //         for(const auto &edge: DFG.edgesOut(toMapPort)){
        //             if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
        //                 string tempFU = *nodeCompat[edge.to()].begin();
        //                 if(tempFU.find("mem") != string::npos || tempFU.find("func") != string::npos){
        //                     prun_flag = true;
        //                 }
        //             }
        //         } 
        //     }
        //     else if(toMap.find("load") != string::npos){
        //         string toMapPort = toMap + ".out0";
        //         for(const auto &edge: DFG.edgesOut(toMapPort)){
        //             if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
        //                 string tempFU = *nodeCompat[edge.to()].begin();
        //                 if(tempFU.find("func") != string::npos){
        //                     prun_flag = true;
        //                 }
        //             }
        //         }
        //     }
        //     //假如func已经布好
        //     // else {
        //     //     string toMapPort = toMap + ".out0";
        //     //     for(const auto &edge: DFG.edgesOut(toMapPort)){
        //     //         if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
        //     //             string tempFU = *nodeCompat[edge.from()].begin();
        //     //             if(tempFU.find("func") != string::npos){
        //     //                 prun_flag = true;
        //     //             }
        //     //         }
        //     //     }
        //     // }
        // }
        // cout << prun_flag << endl;

        // bool const_flag = false;
        // if(toMap.find("const") != string::npos){
        //     string const_dfg = toMap + ".out0" ;
        //     for(const auto &edge: DFG.edgesOut(const_dfg)){
        //         if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
        //             if(vertexDFG2RRG[edge.to()].find("func") != string::npos)
        //                 const_flag = true;
        //         }
        //     }
        //     // const_flag = true;
        // }

        //  bool const_flag = false;
        // if(toMap.find("const") != string::npos){
        //     string const_dfg = toMap + ".out0" ;
        //     const_flag = true;
        //     for(const auto &edge: DFG.edgesOut(const_dfg)){
        //         if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
        //             const_flag = ture;
        //         }
        //     }
        // }
        // Prepare to unconnectable RRG vertice
        //同时前面的剪枝部分也是类似的，CGRAME架构不加剪枝，ADRES加剪枝
        for(const auto &vertexToTry: verticesToTry){
            if(toDelete.find(vertexToTry) != toDelete.end()){
                continue;
            }
            // if(prun_flag){
            //     string const_dfg = toMap + ".out0" ;
            //     string pos_rrg ;
            //     for(const auto &edge: DFG.edgesOut(const_dfg)){
            //         if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
            //             pos_rrg = split(vertexDFG2RRG[edge.to()],".")[1];
            //         }
            //     }
            //     if(vertexToTry.find(pos_rrg) == string::npos){
            //         toDelete.emplace(vertexToTry);
            //     }
            // }
            
            // bool available = true;
            // unordered_multimap<string, string> linksToValidate;
            // unordered_set<string> portVertex;
            // for(const auto &port: contractPorts[toMap]){
            //     for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
            //         if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
            //             continue;
            //         }
            //         const string &fromRRG = vertexDFG2RRG[edge.from()];
            //         const string &toRRG = vertexToTry + "." + port;
            //         linksToValidate.insert({fromRRG, toRRG});
            //         cout << "11" << endl;
            //         cout << fromRRG << " "<<  toRRG << endl;
            //     }
            //     for(const auto &edge: DFG.edgesOut(toMap + "." + port)){
            //         if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
            //             continue;
            //         }
            //         const string &toRRG = vertexDFG2RRG[edge.to()];
            //         const string &fromRRG = vertexToTry + "." + port;
            //         linksToValidate.insert({fromRRG, toRRG});
            //         cout << "22" << endl;
            //         cout << fromRRG << " "<<  toRRG << endl;
            //     }
            // }
            
            // for(const auto &link: linksToValidate){
            //     bool found = false;
            //     for(const auto &edge: _RRGAnalyzed.edgesOut(link.first)){
            //         if(edge.to() == link.second){
            //             found = true;
            //             break;
            //         }
            //     }
            //     if(!found){
            //         available = false;
            //         break;
            //     }
            // }
            // if(!available){
            //     toDelete.insert(vertexToTry);
            // }
        }
        //Delete
        for (const auto &vertex : toDelete) {
            verticesToTry.erase(vertex);
        }
        
        clog << "FastPlacer: -> toMap: " << toMap << "; Candidates After Purge: " << verticesToTry.size() << endl;
        vector<string> verticesToTryRanked(verticesToTry.begin(), verticesToTry.end());
        vector<pair<string, string>> edgesToMap;
        vector<string> edgesSignal;
        unordered_set<string> coreVertex;
        unordered_map<string, string> port2rrg;
        //flag -> true: placedRRG is MEM
        // auto computPOS = [&](string &placedRRG, string &vertexToTry, bool &flag) {
        //     if(flag){
        //         string placed_pos = split(vertexDFG2RRG[edge.from()],".")[1];
        //         int placed_x = str2num<int>(split(placed_pos,"_")[1]);
        //         int placed_y ;//= str2num<int>(split(vertexDFG2RRG[edge.from()],".")[2].back());
        //         string try_cycle = split(vertexToTry,".")[0];
        //         string try_pos = split(vertexToTry,".")[1];
        //         cout << "try " << try_cycle << "  "<< try_pos << endl;
        //         cout << try_cycle.back() << endl;
        //         int try_x = str2num<int>(split(try_pos,"_")[1]);
        //         int try_y = str2num<int>(split(try_pos,"_")[2]);
        //         dis = manhattan_distance(placed_x, placed_y, 
        //                                 try_x, try_y);
        //     }
        //     else{
        //         string placed_cycle = split(vertexDFG2RRG[edge.to()],".")[0];
        //         string placed_pos = split(vertexDFG2RRG[edge.to()],".")[1];
        //         cout << "placed " << placed_cycle << "  "<< placed_pos << endl;
        //         cout << placed_cycle.back() << endl;
        //         cout << vertexDFG2RRG[edge.to()] <<endl;
        //         int placed_x = str2num<int>(split(placed_pos,"_")[1]);
        //         int placed_y = str2num<int>(split(placed_pos,"_")[2]);
        //     }
        // };
        //这个地方排序可以写成sort函数，根据不同的有不同的匹配，比如说ADRES架构，是根据CORE来决定的，然后CGRAME架构是根据位置来决定的。
        //sort函数的入口有不同的选项，然后Hycube架构再说吧
        unordered_map<string, int> pos_dis;
        if (verticesToTry.empty()) {
            failed = false;
            clog << "FastPlacer: Failed. Nothing to try for " << toMap << endl;
        } else {
            //  sort the candidates by sharedNet
            unordered_map<string, size_t> distancedNet;
            unordered_set<string> inPortRRG;
            unordered_set<string> outPortRRG;

            for(const auto &vertexToTry: verticesToTry){
                inPortRRG.clear();
                outPortRRG.clear();
                int try_x;
                int try_y;
                if(vertexToTry.find("MEM") != string::npos){
                    string try_pos = split(vertexToTry,".")[1];
                    string try_mem = split(vertexToTry,".")[2];

                    try_x = str2num<int>(split(try_pos,"_")[1]);
                    try_y = str2num<int>(split(try_mem,"EM")[1]);
                }
                else{
                    string try_pos = split(vertexToTry,".")[1];

                    try_x = str2num<int>(split(try_pos,"_")[1]);
                    try_y = str2num<int>(split(try_pos,"_")[2]);
                }



                for(const auto &port: contractPorts[toMap]){//完全可以在这里对所有的进行遍历然后排序，找到特殊的，然后留出来。
                    //const->func const->load load->func load->func
                    int dis = 0;
                    for(const auto &edge: DFG.edgesIn(toMap + "." + port)){//(out)->in
                        if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                            continue;
                        }
                            if(vertexDFG2RRG[edge.from()].find("MEM") != string::npos){
                                string placed_pos = split(vertexDFG2RRG[edge.from()],".")[1];
                                int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                                string placed_mem = split(vertexDFG2RRG[edge.from()],".")[2];
                                int placed_y = str2num<int>(split(placed_mem,"EM")[1]);;//= str2num<int>(split(vertexDFG2RRG[edge.from()],".")[2].back());
                                // string try_cycle = split(vertexToTry,".")[0];
                                // string try_pos = split(vertexToTry,".")[1];
                                // cout << "try " << try_cycle << "  "<< try_pos << endl;
                                // cout << try_cycle.back() << endl;
                                // int try_x = str2num<int>(split(try_pos,"_")[1]);
                                // int try_y = str2num<int>(split(try_pos,"_")[2]);
                                dis = manhattan_distance(placed_x, placed_y, 
                                                         try_x, try_y);
                                // cout << "dis " << dis << endl;
                                pos_dis[vertexToTry] += dis;
                            }
                            else{
                                string placed_cycle = split(vertexDFG2RRG[edge.from()],".")[0];
                                string placed_pos = split(vertexDFG2RRG[edge.from()],".")[1];
                                // cout << "placed " << placed_cycle << "  "<< placed_pos << endl;
                                // cout << placed_cycle.back() << endl;
                                int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                                int placed_y = str2num<int>(split(placed_pos,"_")[2]);
                                
                                // string try_cycle = split(vertexToTry,".")[0];
                                // string try_pos = split(vertexToTry,".")[1];
                                // cout << "try " << try_cycle << "  "<< try_pos << endl;
                                // cout << try_cycle.back() << endl;
                                // int try_x = str2num<int>(split(try_pos,"_")[1]);
                                // int try_y = str2num<int>(split(try_pos,"_")[2]);
                                
                                dis = manhattan_distance(placed_x, placed_y, 
                                                         try_x, try_y);
                                // cout << "dis " << dis << endl;
                                pos_dis[vertexToTry] += dis;
                            }
                            
                            //计算距离
                            //
                      
                        string core = getFront(vertexDFG2RRG[edge.from()]);
                        if(vertexToTry.find(core) != string::npos){
                            //coreVertex.emplace(vertexToTry);
                            //cout <<core <<  "  " << vertexToTry<< endl;
                        }
                        inPortRRG.emplace(vertexDFG2RRG[edge.from()]);
                        // const string &fromRRG = vertexDFG2RRG[edge.from()];
                        // const string &toRRG = vertexToTry + "." + port;
                        // linksToValidate.insert({fromRRG, toRRG});
                    }
                    for(const auto &edge: DFG.edgesOut(toMap + "." + port)){//out->(in)
                        if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                            continue;
                        }
                        //if(1){//假如是const->那么直接const->func，距离为0
                        if(vertexDFG2RRG[edge.to()].find("MEM") != string::npos){
                            string placed_pos = split(vertexDFG2RRG[edge.to()],".")[1];
                            int placed_x = str2num<int>(split(placed_pos,"_")[1]);

                            string placed_mem =split(vertexDFG2RRG[edge.to()],".")[2];

                            int placed_y = str2num<int>(split(placed_mem,"EM")[1]);

                            // string try_cycle = split(vertexToTry,".")[0];
                            // string try_pos = split(vertexToTry,".")[1];
                            // cout << "try " << try_cycle << "  "<< try_pos << endl;
                            // cout << try_cycle.back() << endl;
                            // int try_x = str2num<int>(split(try_pos,"_")[1]);
                            // int try_y = str2num<int>(split(try_pos,"_")[2]);
                            dis = manhattan_distance(placed_x, placed_y, 
                                                        try_x, try_y);
                            cout << "dis " << dis << endl;
                            pos_dis[vertexToTry] += dis;
                        }
                        else{
                            string placed_cycle = split(vertexDFG2RRG[edge.to()],".")[0];
                            string placed_pos = split(vertexDFG2RRG[edge.to()],".")[1];
                            // cout << "placed " << placed_cycle << "  "<< placed_pos << endl;
                            // cout << placed_cycle.back() << endl;
                            cout << vertexDFG2RRG[edge.to()] <<endl;
                            int placed_x = str2num<int>(split(placed_pos,"_")[1]);
                            int placed_y = str2num<int>(split(placed_pos,"_")[2]);
                            
                            dis = manhattan_distance(placed_x, placed_y, 
                                                         try_x, try_y);
                                cout << "dis " << dis << endl;
                                pos_dis[vertexToTry] += dis;
                            
                            // if(vertexToTry.find("MEM") != string::npos){
                            //     string try_pos = split(vertexToTry,".")[1];
                            //     int try_x = str2num<int>(split(try_pos,"_")[1]);

                            //     string try_mem =split(vertexToTry,".")[2];
                            //     int try_y = str2num<int>(split(try_mem,"EM")[1]);

                            //     dis = manhattan_distance(placed_x, placed_y, 
                            //                                 try_x, try_y);
                            //     cout << "dis " << dis << endl;
                            //     pos_dis[vertexToTry] += dis;

                            // }
                            // else{
                            //     string try_cycle = split(vertexToTry,".")[0];
                            //     string try_pos = split(vertexToTry,".")[1];
                            //     cout << "try " << try_cycle << "  "<< try_pos << endl;
                            //     cout << try_cycle.back() << endl;
                            //     int try_x = str2num<int>(split(try_pos,"_")[1]);
                            //     int try_y = str2num<int>(split(try_pos,"_")[2]);
                            //     dis = manhattan_distance(placed_cycle.back(), placed_x, placed_y, 
                            //                              try_cycle.back(), try_x, try_y);
                            //     cout << "dis " << dis << endl;
                            //     pos_dis[vertexToTry] += dis;
                            //     //计算距离
                            //     //
                            // }
                            
                            
                        }
                            
                      
                        string core = getFront(vertexDFG2RRG[edge.to()]);
                        if(vertexToTry.find(core) != string::npos){
                            //coreVertex.emplace(vertexToTry);
                            //cout <<core <<  "  " << vertexToTry<< endl ;
                        }
                        outPortRRG.insert(vertexDFG2RRG[edge.to()]);
                        // const string &toRRG = vertexDFG2RRG[edge.to()];
                        // const string &fromRRG = vertexToTry + "." + port;
                        // linksToValidate.insert({fromRRG, toRRG});
                    }
                }
                // for(const auto &edge: DFG.edgesIn(toMap)){
                //     const string &toDFG = edge.to();
                //     const string &toRRG = vertexToTry ;
                //     inPortRRG.insert(toRRG);
                // }
                // // for(const auto &edge: DFG.edgesOut(toMap)){
                // //     const string &fromDFG = edge.from();
                // //     const string &fromRRG = vertexToTry ;
                // //     outPortRRG.insert(fromRRG);
                // // }
                for(const auto &inport: inPortRRG){
                    pair<vector<string>, bool> temp = router.get_distance(inport, vertexToTry);
                    if(temp.second){
                        distancedNet[vertexToTry] += temp.first.size();
                    }
                }
                for(const auto &outport: outPortRRG){
                    pair<vector<string>, bool> temp = router.get_distance(vertexToTry, outport);
                    if(temp.second){
                        distancedNet[vertexToTry] += temp.first.size();
                    }
                    // for(const auto &edge: _RRGAnalyzed.edgesOut(outport)){
                    //     if(vertexRRG2DFG.find(edge.to()) != vertexRRG2DFG.end()){
                    //         distancedNet[vertexToTry]++;
                    //     }
                    // }
                }

                //剪枝得到的,作为最前面的不会删除
                
            }
            // cout << verticesToTryRanked << endl;

            // cout << "fused " << fused << endl;

            // cout << coreVertex << endl;
            // cout << "before " << verticesToTryRanked << endl;
            // // random_shuffle(verticesToTryRanked.begin(), verticesToTryRanked.end());
            //sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return distancedNet[a] > distancedNet[b];});
            sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return pos_dis[a] < pos_dis[b];});
            
            // // cout << verticesToTryRanked << endl;
            
            cout << verticesToTryRanked << endl;
            // unordered_map<string, size_t> coreNum;
            // if(coreVertex.empty()){
            //     for(const auto &vertex: verticesToTryRanked){
            //         string core = getFront(vertex);
            //         if(coreNum.find(core) == coreNum.end()){
            //             coreNum[core] = 1;
            //         }
            //         else{
            //             coreNum[core] += 1;
            //         }
            //     }
            //     sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return coreNum[a] > coreNum[b];});
            // }
            // else{
            //     verticesToTryRanked.insert(verticesToTryRanked.begin(), coreVertex.begin(),coreVertex.end());
            // }
            
            // cout << "after " << verticesToTryRanked << endl;
            // try map vertex
            bool isSuccess = false;
            size_t IterVertextoTry = 0;
            
            while(!isSuccess && IterVertextoTry < verticesToTryRanked.size()){
                // -> Find edges that need to be mapped
                toTry = verticesToTryRanked[IterVertextoTry++];
                clog << "FastPlacer: try to map " << toMap << " to " << toTry << endl;
                edgesToMap.clear();
                edgesSignal.clear();
                if (!isSuccess) {
                    vector<string> portVertex = {};
                     
                    port2rrg.clear();
                    for(const auto &port: contractPorts[toMap]){
                        port2rrg[toMap + "." + port] = toTry + "." + port;
                        if(port.find("in") != string::npos){
                            edgesToMap.emplace_back(pair<string, string>(toTry + "." + port, toTry));
                            edgesSignal.emplace_back(toMap + "." + port);
                        }
                        else{
                            edgesToMap.emplace_back(pair<string, string>(toTry, toTry + "." + port));
                            edgesSignal.emplace_back(toMap);
                        }
                    }
                    // travesalVertexDFG[toMap] = true;
                    // vertexDFG2RRG[toMap] = toTry;
                    for(const auto &vertex: port2rrg){
                        for (const auto &edge : DFG.edgesIn(vertex.first)) {
                            if (travesalVertexDFG[edge.from()] ) {
                                edgesToMap.emplace_back(pair<string, string>(vertexDFG2RRG[edge.from()], vertex.second));
                                edgesSignal.emplace_back(edge.from());
                            }
                        }
                        for (const auto &edge : DFG.edgesOut(vertex.first)) {
                            if (travesalVertexDFG[edge.to()]) {
                                edgesToMap.emplace_back(pair<string, string>(vertex.second, vertexDFG2RRG[edge.to()]));
                                edgesSignal.emplace_back(vertex.first);
                            }
                        }
                    }
                    // travesalVertexDFG[toMap] = false;
                    // vertexDFG2RRG.erase(toMap);
                    clog << "FastPlacer: edgesToMap: " << edgesToMap.size() << " for " << toTry << endl;
                }
                if(edgesToMap.size() > 0){
                    isSuccess = router.norm_route(edgesToMap, edgesSignal);
                    // if(!isSuccess){
                    //     clog << "FastPlacer: Choose a loose route." << endl;
                    //     isSuccess = router.route(edgesToMap, edgesSignal);
                    // }
                } else {
                    isSuccess = true;
                }
            }
            if(!isSuccess){
                failed = true;
            }
        }
        if(!failed){
            vertexDFG2RRG[toMap] = toTry;
            vertexRRG2DFG[toTry] = toMap;
            travesalVertexDFG[toMap]=true;
            for(const auto &port: port2rrg){
                vertexDFG2RRG[port.first] = port.second;
                vertexRRG2DFG[port.second] = port.first;
                travesalVertexDFG[port.first] = true;
            }
            stackVerticesToTry.push(verticesToTry);
            stackEdgesToMap.push(edgesToMap);
            
            // string fuNameDFG = getPrefix(toMap);
            // string portNameDFG = getPostfix(toMap);
            // string fuNameRRG = portNameDFG.empty() ? toTry : getPrefix(toTry);
            // unordered_set<string> fuPortsUnmappedDFG;
            // unordered_set<string> fuInputsDFG;
            // unordered_set<string> fuOutputsDFG;
            // for (const auto &edge : DFG.edgesIn(fuNameDFG)) {
            //     assert(getPrefix(edge.from()) == fuNameDFG && edge.from().size() > fuNameDFG.size());
            //     string portNameTmp = getPostfix(edge.from());
            //     fuInputsDFG.insert(portNameTmp);
            //     if (edge.from() != toMap && !travesalVertexDFG[edge.from()]) {
            //         fuPortsUnmappedDFG.insert(portNameTmp);
            //     }
            // }
            // for (const auto &edge : DFG.edgesOut(fuNameDFG)) {
            //     assert(getPrefix(edge.to()) == fuNameDFG && edge.to().size() > fuNameDFG.size());
            //     string portNameTmp = getPostfix(edge.to());
            //     fuOutputsDFG.insert(portNameTmp);
            //     if (edge.to() != toMap && !travesalVertexDFG[edge.to()]) {
            //         fuPortsUnmappedDFG.insert(portNameTmp);
            //     }
            // }
            // for(const auto &node: Goorder){
            //     if (!travesalVertexDFG[node]) {
            //         toMap = node;
            //         verticesToTry = contractCompat.find(toMap)->second;
            //     }
            // }
            if(++coarseIter < Goorder.size()){
                toMap = Goorder[coarseIter];
                verticesToTry = contractCompat.find(toMap)->second;
            } else {
                break;
            }
        } else {
            failureCount++;
            if (_failedVertices.find(toMap) == _failedVertices.end()) {
                _failedVertices[toMap] = 0;
            }
            _failedVertices[toMap]++;
            if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
                clog << "FastPlacer: FAILED. Too many failure. " << endl;
                return {unordered_map<string, string>(), routerInit};
            }
            if(coarseIter == 0){
                clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
                return {unordered_map<string, string>(), routerInit};
            }
            toMap = Goorder[--coarseIter];
            verticesToTry = stackVerticesToTry.top();
            stackVerticesToTry.pop();
            clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
            vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
            stackEdgesToMap.pop();
            router.unroute(edgesToDelte);
            vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
            vertexDFG2RRG.erase(toMap);
            for(const auto &port: port2rrg){
                vertexDFG2RRG.erase(port.first);
                vertexRRG2DFG.erase(port.second);
                travesalVertexDFG[port.first] = false;
            }
            travesalVertexDFG[toMap]=false;
        }
        clog << endl << endl;

    }

    clog << "FastPlacer: finished placing the DFG. Failure count: " << failureCount << "." << endl
         << endl;

    return{vertexDFG2RRG, router};

}


std::pair<std::unordered_map<std::string, std::string>, FastRouter> FastPlacer::place_matching2(const Graph &DFG, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatible, const FastRouter &routerInit, 
                                                        const std::unordered_map<std::string, std::string>  &usedDFG2RRGInitial,  const NetworkAnalyzerLegacy &analyzerInitial, const NOrderValidator &validatorInitial,
                                                        const std::vector<std::vector<std::string>> &order)
{
    const size_t Max_Failed_Times = 64;

    // Clear the previous status
    _failedVertices.clear();
    // Data
    FastRouter router(routerInit);
    vector<string> Goorder = (order[0],order[1]);
    vector<string> Goorder1 = order[0];//order
    vector<string> Goorder2 = order[1];//order
    unordered_map<string, string> vertexDFG2RRG = usedDFG2RRGInitial;
    unordered_map<string, string> vertexRRG2DFG;
    unordered_map<string, bool>   travesalVertexDFG;

    // unordered_map<string, unordered_set<string>> coincompat = compatible;
    stack<vector<pair<string, string>>> stackEdgesToMap1;
    stack<unordered_set<string>> stackVerticesToTry1;
    stack<vector<pair<string, string>>> stackEdgesToMap2;
    stack<unordered_set<string>> stackVerticesToTry2;
    for(const auto &dfg2rrg: vertexDFG2RRG){
        vertexRRG2DFG[dfg2rrg.second] = dfg2rrg.first;
    }
    vector<string> contractVertex;
    unordered_map<string, unordered_set<string>> contractCompat;
    unordered_map<string, unordered_set<string>> contractPorts;
    for(const auto &item: compatible){
        if (getPostfix(item.first).empty()) {
                contractVertex.emplace_back(item.first);
                contractCompat.emplace(item);
        }
    }
    for(const auto &vertexDFG: DFG.vertices())
    {
        travesalVertexDFG[vertexDFG.first] = false;
        if(!getPostfix(vertexDFG.first).empty()){
            string prefix = getPrefix(vertexDFG.first);
            if (contractPorts.find(prefix) == contractPorts.end()) {
                contractPorts[prefix] = {};
            }
            contractPorts[prefix].emplace(getPostfix(vertexDFG.first));
        }
    }
    //N-order validation
    NetworkAnalyzerLegacy analyzer(analyzerInitial);
    Graph &RRGAnalyzed = analyzer.RRG();
    NOrderValidator validator(validatorInitial);
    // size_t unplacibleCount = 0;
    // for (const auto &vertexDFG : coarseDFG.vertices()) {
    //     if (!getPostfix(vertexDFG.first).empty() || vertexDFG.first.find("block") != string::npos) {
    //         continue;
    //     }
    //     if (coincompat.find(vertexDFG.first) == coincompat.end()) {
    //         WARN << "FastPlacement: Compatible vertices NOT FOUND: " + vertexDFG.first;
    //         return {unordered_map<string, string>(), router};
    //     }
    //     unordered_set<string> compatibles;
    //     for(const auto &vertexRRG: coincompat[vertexDFG.first]){
    //         clog << "\rFastPlacement: -> Validating " << vertexDFG.first << " : " << vertexRRG << "            ";
    //         if (validator.validateSlow(vertexDFG.first, vertexRRG, 2)) { //NorderValidate
    //             compatibles.insert(vertexRRG);
    //         }
    //     }
    //     clog << vertexDFG.first << ": " << coincompat[vertexDFG.first].size() << " -> " << compatibles.size() << "            ";
    //     coincompat[vertexDFG.first] = compatibles;
    // }
    // if(unplacibleCount > 0){
    //     clog << "VanillaPlacer: FAILED, uncompatible vertex found in first order validation. " << endl;
    //     return {unordered_map<string, string>(), router};
    // }

    size_t furthest = 0;
    size_t coarseIter1 = 0;
    size_t coarseIter2 = 0;
    size_t failureCount = 0;

    string toMap1 = Goorder1[0];
    string toMap2 = Goorder2[0];
    bool finised1 = false;
    bool finised2 = false;
    unordered_set<string> verticesToTry1 = contractCompat.find(toMap1)->second;
    unordered_set<string> verticesToTry2 = contractCompat.find(toMap2)->second;
    while((coarseIter1 + coarseIter2) < (Goorder1.size() + Goorder2.size())){
        furthest = max(furthest, coarseIter1 + coarseIter2);
        clog << "FastPlacer: New iteration, size of stack: " << vertexDFG2RRG.size() << " / " << travesalVertexDFG.size() << "; furthest: " << furthest << endl;
        bool failed = false;
        string toTry = "";
        auto prune = [&](auto &toMap, auto &verticesToTry) {
            unordered_set<string> toDelete;
            // Prepare to delete used RRG vertices
            for(const auto &vertexToTry: verticesToTry){
                if(vertexRRG2DFG.find(vertexToTry) != vertexRRG2DFG.end()){
                    toDelete.emplace(vertexToTry);
                }
            }

            for(const auto &vertexToTry: verticesToTry){
                if(toDelete.find(vertexToTry) != toDelete.end()){
                    continue;
                }
                // Prepare to unconnectable RRG vertice
                bool available = true;
                unordered_multimap<string, string> linksToValidate;
                unordered_set<string> portVertex;
                for(const auto &port: contractPorts[toMap]){
                    for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                        if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                            continue;
                        }
                        const string &fromRRG = vertexDFG2RRG[edge.from()];
                        const string &toRRG = vertexToTry + "." + port;
                        linksToValidate.insert({fromRRG, toRRG});
                    }
                    for(const auto &edge: DFG.edgesOut(toMap + "." + port)){
                        if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                            continue;
                        }
                        const string &toRRG = vertexDFG2RRG[edge.to()];
                        const string &fromRRG = vertexToTry + "." + port;
                        linksToValidate.insert({fromRRG, toRRG});
                    }
                }
                for(const auto &link: linksToValidate){
                    bool found = false;
                    for(const auto &edge: _RRGAnalyzed.edgesOut(link.first)){
                        if(edge.to() == link.second){
                            found = true;
                            break;
                        }
                    }
                    if(!found){
                        available = false;
                        break;
                    }
                }
                if(!available){
                    toDelete.insert(vertexToTry);
                }
            }
            //Delete
            for (const auto &vertex : toDelete) {
                verticesToTry.erase(vertex);
            }
        };

        if(!finised1){
            prune(toMap1, verticesToTry1);
        }
        if(!finised2){
            prune(toMap2, verticesToTry2);
        }
        clog << "FastPlacer: -> toMap: " << toMap1 << "; Candidates After Purge: " << verticesToTry1.size() << endl;
        clog << "FastPlacer: -> toMap: " << toMap2 << "; Candidates After Purge: " << verticesToTry2.size() << endl;

        auto sortPE = [&](auto &toMap, auto &verticesToTry, auto &verticesToTryRanked) {
            unordered_set<string> coreVertex;
            
            if (verticesToTry.empty()) {
                failed = false;
                clog << "FastPlacer: Failed. Nothing to try for " << toMap << endl;
                return failed;
            }
            else{
                //  sort the candidates by sharedNet
                unordered_map<string, size_t> distancedNet;
                unordered_set<string> inPortRRG;
                unordered_set<string> outPortRRG;
                for(const auto &vertexToTry: verticesToTry){
                    inPortRRG.clear();
                    outPortRRG.clear();
                    for(const auto &port: contractPorts[toMap]){
                        for(const auto &edge: DFG.edgesIn(toMap + "." + port)){//(out)->in
                            if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            string core = getFront(vertexDFG2RRG[edge.from()]);
                            if(vertexToTry.find(core) != string::npos){
                                coreVertex.emplace(vertexToTry);
                                cout <<core <<  "  " << vertexToTry<< endl;
                            }
                            inPortRRG.emplace(vertexDFG2RRG[edge.from()]);
                            // const string &fromRRG = vertexDFG2RRG[edge.from()];
                            // const string &toRRG = vertexToTry + "." + port;
                            // linksToValidate.insert({fromRRG, toRRG});
                        }
                        for(const auto &edge: DFG.edgesOut(toMap + "." + port)){//out->(in)
                            if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            string core = getFront(vertexDFG2RRG[edge.to()]);
                            if(vertexToTry.find(core) != string::npos){
                                coreVertex.emplace(vertexToTry);
                                cout <<core <<  "  " << vertexToTry<< endl ;
                            }
                            outPortRRG.insert(vertexDFG2RRG[edge.to()]);
                            // const string &toRRG = vertexDFG2RRG[edge.to()];
                            // const string &fromRRG = vertexToTry + "." + port;
                            // linksToValidate.insert({fromRRG, toRRG});
                        }
                    }
                    for(const auto &inport: inPortRRG){
                        pair<vector<string>, bool> temp = router.get_distance(inport, vertexToTry);
                        if(temp.second){
                            distancedNet[vertexToTry] += temp.first.size();
                        }
                    }
                    for(const auto &outport: outPortRRG){
                        pair<vector<string>, bool> temp = router.get_distance(vertexToTry, outport);
                        if(temp.second){
                            distancedNet[vertexToTry] += temp.first.size();
                        }
                    }
                }
                cout << "before " << verticesToTryRanked << endl;
                // random_shuffle(verticesToTryRanked.begin(), verticesToTryRanked.end());
                sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return distancedNet[a] > distancedNet[b];});
                // cout << verticesToTryRanked << endl;
                unordered_map<string, size_t> coreNum;
                if(coreVertex.empty()){
                    for(const auto &vertex: verticesToTryRanked){
                        string core = getFront(vertex);
                        if(coreNum.find(core) == coreNum.end()){
                            coreNum[core] = 1;
                        }
                        else{
                            coreNum[core] += 1;
                        }
                    }
                    sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return coreNum[a] > coreNum[b];});
                }
                else{
                    verticesToTryRanked.insert(verticesToTryRanked.begin(), coreVertex.begin(),coreVertex.end());
                }
                return failed;
            }
        };
        bool failed1;
        bool failed2;
        vector<string> verticesToTryRanked1(verticesToTry1.begin(), verticesToTry1.end());
        vector<string> verticesToTryRanked2(verticesToTry2.begin(), verticesToTry2.end());
        if(!finised1){
            
            failed1= sortPE(toMap1, verticesToTry1,verticesToTryRanked1);
        }
        if(!finised2){
           
           failed2 = sortPE(toMap2, verticesToTry2,verticesToTryRanked2);
        }

        // try map vertex
        auto map = [&](auto &failed, auto &toMap, auto &verticesToTryRanked, auto &coarseIter, 
                        auto &node_order, auto &stackVerticesToTry, auto &stackEdgesToMap, auto &verticesToTry, auto &finised){
            vector<pair<string, string>> edgesToMap;
            vector<string> edgesSignal;
            unordered_map<string, string> port2rrg;
            bool isSuccess = false;
            size_t IterVertextoTry = 0;
            while(!isSuccess && IterVertextoTry < verticesToTryRanked.size()){
                // -> Find edges that need to be mapped
                toTry = verticesToTryRanked[IterVertextoTry++];
                if(vertexRRG2DFG.find(toTry) != vertexRRG2DFG.end())
                    continue;
                clog << "FastPlacer: try to map " << toMap << " to " << toTry << endl;
                edgesToMap.clear();
                edgesSignal.clear();
                if (!isSuccess) {
                    vector<string> portVertex = {};
                     
                    port2rrg.clear();
                    for(const auto &port: contractPorts[toMap]){
                        port2rrg[toMap + "." + port] = toTry + "." + port;
                        if(port.find("in") != string::npos){
                            edgesToMap.emplace_back(pair<string, string>(toTry + "." + port, toTry));
                            edgesSignal.emplace_back(toMap + "." + port);
                        }
                        else{
                            edgesToMap.emplace_back(pair<string, string>(toTry, toTry + "." + port));
                            edgesSignal.emplace_back(toMap);
                        }
                    }
                    // travesalVertexDFG[toMap] = true;
                    // vertexDFG2RRG[toMap] = toTry;
                    for(const auto &vertex: port2rrg){
                        for (const auto &edge : DFG.edgesIn(vertex.first)) {
                            if (travesalVertexDFG[edge.from()] ) {
                                edgesToMap.emplace_back(pair<string, string>(vertexDFG2RRG[edge.from()], vertex.second));
                                edgesSignal.emplace_back(edge.from());
                            }
                        }
                        for (const auto &edge : DFG.edgesOut(vertex.first)) {
                            if (travesalVertexDFG[edge.to()]) {
                                edgesToMap.emplace_back(pair<string, string>(vertex.second, vertexDFG2RRG[edge.to()]));
                                edgesSignal.emplace_back(vertex.first);
                            }
                        }
                    }
                    // travesalVertexDFG[toMap] = false;
                    // vertexDFG2RRG.erase(toMap);
                    clog << "FastPlacer: edgesToMap: " << edgesToMap.size() << " for " << toTry << endl;
                }
                if(edgesToMap.size() > 0){
                    isSuccess = router.norm_route(edgesToMap, edgesSignal);
                    // if(!isSuccess){
                    //     clog << "FastPlacer: Choose a loose route." << endl;
                    //     isSuccess = router.route(edgesToMap, edgesSignal);
                    // }
                } else {
                    isSuccess = true;
                }
            }
            if(!isSuccess){
                failed = true;
            }

            if(!failed){
                vertexDFG2RRG[toMap] = toTry;
                vertexRRG2DFG[toTry] = toMap;
                travesalVertexDFG[toMap]=true;
                for(const auto &port: port2rrg){
                    vertexDFG2RRG[port.first] = port.second;
                    vertexRRG2DFG[port.second] = port.first;
                    travesalVertexDFG[port.first] = true;
                }
                stackVerticesToTry.push(verticesToTry);
                stackEdgesToMap.push(edgesToMap);

                if(++coarseIter < node_order.size()){
                    toMap = node_order[coarseIter];
                    verticesToTry = contractCompat.find(toMap)->second;
                } else {
                    finised = true;
                }
            }
            else{
                failureCount++;
                if (_failedVertices.find(toMap) == _failedVertices.end()) {
                    _failedVertices[toMap] = 0;
                }
                _failedVertices[toMap]++;
                if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
                    clog << "FastPlacer: FAILED. Too many failure. " << endl;
                    // return {unordered_map<string, string>(), routerInit};
                    return false;
                }
                if(coarseIter == 0){
                    clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
                    // return {unordered_map<string, string>(), routerInit};
                    return false;
                }
                toMap = node_order[--coarseIter];
                verticesToTry = stackVerticesToTry.top();
                stackVerticesToTry.pop();
                clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
                vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
                stackEdgesToMap.pop();
                router.unroute(edgesToDelte);
                vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
                vertexDFG2RRG.erase(toMap);
                for(const auto &port: port2rrg){
                    vertexDFG2RRG.erase(port.first);
                    vertexRRG2DFG.erase(port.second);
                    travesalVertexDFG[port.first] = false;
                }
                travesalVertexDFG[toMap]=false;
            }
        };
        if(!finised1){
            
            map(failed1, toMap1, verticesToTryRanked1, coarseIter1, Goorder1, stackVerticesToTry1, stackEdgesToMap1, verticesToTry1,finised1);
        }
        if(!finised2){
           map(failed2, toMap2, verticesToTryRanked2, coarseIter2, Goorder2, stackVerticesToTry2, stackEdgesToMap2, verticesToTry2,finised2);   
        }
        if(finised1 || finised2){
            return{vertexDFG2RRG, router};
        }
        // bool result1 = map(failed1, toMap1, verticesToTryRanked1, coarseIter1, Goorder1, stackVerticesToTry1, stackEdgesToMap1, verticesToTry1);     
        // bool result2 = map(failed2, toMap2, verticesToTryRanked2, coarseIter2, Goorder2, stackVerticesToTry2, stackEdgesToMap2, verticesToTry2);   
        // if((!result1) && (!result2))
        //     return  {unordered_map<string, string>(), routerInit};
        // }
        // if(!failed){
        //     vertexDFG2RRG[toMap] = toTry;
        //     vertexRRG2DFG[toTry] = toMap;
        //     travesalVertexDFG[toMap]=true;
        //     for(const auto &port: port2rrg){
        //         vertexDFG2RRG[port.first] = port.second;
        //         vertexRRG2DFG[port.second] = port.first;
        //         travesalVertexDFG[port.first] = true;
        //     }
        //     stackVerticesToTry.push(verticesToTry);
        //     stackEdgesToMap.push(edgesToMap);
            
            // string fuNameDFG = getPrefix(toMap);
            // string portNameDFG = getPostfix(toMap);
            // string fuNameRRG = portNameDFG.empty() ? toTry : getPrefix(toTry);
            // unordered_set<string> fuPortsUnmappedDFG;
            // unordered_set<string> fuInputsDFG;
            // unordered_set<string> fuOutputsDFG;
            // for (const auto &edge : DFG.edgesIn(fuNameDFG)) {
            //     assert(getPrefix(edge.from()) == fuNameDFG && edge.from().size() > fuNameDFG.size());
            //     string portNameTmp = getPostfix(edge.from());
            //     fuInputsDFG.insert(portNameTmp);
            //     if (edge.from() != toMap && !travesalVertexDFG[edge.from()]) {
            //         fuPortsUnmappedDFG.insert(portNameTmp);
            //     }
            // }
            // for (const auto &edge : DFG.edgesOut(fuNameDFG)) {
            //     assert(getPrefix(edge.to()) == fuNameDFG && edge.to().size() > fuNameDFG.size());
            //     string portNameTmp = getPostfix(edge.to());
            //     fuOutputsDFG.insert(portNameTmp);
            //     if (edge.to() != toMap && !travesalVertexDFG[edge.to()]) {
            //         fuPortsUnmappedDFG.insert(portNameTmp);
            //     }
            // }
            // for(const auto &node: Goorder){
            //     if (!travesalVertexDFG[node]) {
            //         toMap = node;
            //         verticesToTry = contractCompat.find(toMap)->second;
            //     }
            // }
            // if(++coarseIter < Goorder.size()){
            //     toMap = Goorder[coarseIter];
            //     verticesToTry = contractCompat.find(toMap)->second;
            // } else {
            //     break;
            // }
        // } else {
        //     failureCount++;
        //     if (_failedVertices.find(toMap) == _failedVertices.end()) {
        //         _failedVertices[toMap] = 0;
        //     }
        //     _failedVertices[toMap]++;
        //     if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
        //         clog << "FastPlacer: FAILED. Too many failure. " << endl;
        //         return {unordered_map<string, string>(), routerInit};
        //     }
        //     if(coarseIter == 0){
        //         clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
        //         return {unordered_map<string, string>(), routerInit};
        //     }
        //     toMap1 = Goorder1[--coarseIter];
        //     verticesToTry1 = stackVerticesToTry1.top();
        //     stackVerticesToTry.pop();
        //     clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
        //     vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
        //     stackEdgesToMap.pop();
        //     router.unroute(edgesToDelte);
        //     vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
        //     vertexDFG2RRG.erase(toMap);
        //     for(const auto &port: port2rrg){
        //         vertexDFG2RRG.erase(port.first);
        //         vertexRRG2DFG.erase(port.second);
        //         travesalVertexDFG[port.first] = false;
        //     }
        //     travesalVertexDFG[toMap]=false;
        // }
        // clog << endl << endl;

    }

    clog << "FastPlacer: finished placing the DFG. Failure count: " << failureCount << "." << endl
         << endl;

    return{vertexDFG2RRG, router};

}

std::pair<std::unordered_map<std::string, std::string>, FastRouter> FastPlacer::place_remain(const Graph &DFG, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatible, const FastRouter &routerInit, 
                                                        const std::unordered_map<std::string, std::string>  &usedDFG2RRGInitial,  const NetworkAnalyzerLegacy &analyzerInitial, const NOrderValidator &validatorInitial,
                                                        const std::vector<std::string> &order)
{
    const size_t Max_Failed_Times = 64;

    // Clear the previous status
    _failedVertices.clear();
    // Data
    FastRouter router(routerInit);
    vector<string> Goorder = order;
    unordered_map<string, string> vertexDFG2RRG = usedDFG2RRGInitial;
    unordered_map<string, string> vertexRRG2DFG;
    unordered_map<string, bool>   travesalVertexDFG;

    // unordered_map<string, unordered_set<string>> coincompat = compatible;
    stack<vector<pair<string, string>>> stackEdgesToMap;
    stack<unordered_set<string>> stackVerticesToTry;
    for(const auto &dfg2rrg: vertexDFG2RRG){
        vertexRRG2DFG[dfg2rrg.second] = dfg2rrg.first;
    }
    vector<string> contractVertex;
    unordered_map<string, unordered_set<string>> contractCompat;
    unordered_map<string, unordered_set<string>> contractPorts;
    for(const auto &item: compatible){
        if (getPostfix(item.first).empty()) {
                contractVertex.emplace_back(item.first);
                contractCompat.emplace(item);
        }
    }

    for(const auto &vertexDFG: DFG.vertices())
    {
        if(vertexDFG2RRG.find(vertexDFG.first) != vertexDFG2RRG.end()){
            travesalVertexDFG[vertexDFG.first] = true;
        }
        else{
            travesalVertexDFG[vertexDFG.first] = false;
        }
        if(!getPostfix(vertexDFG.first).empty()){
            string prefix = getPrefix(vertexDFG.first);
            if (contractPorts.find(prefix) == contractPorts.end()) {
                contractPorts[prefix] = {};
            }
            contractPorts[prefix].emplace(getPostfix(vertexDFG.first));
        }
    }


    // for (const auto &vertex : DFG.vertices()) {
    //     if (travesalVertexDFG[vertex.first]) {
    //         for (const auto &edge : DFG.edgesOut(vertex.first)) {
    //             if (travesalVertexDFG[edge.to()]) {
    //                 assert(vertexDFG2RRG.find(edge.from()) != vertexDFG2RRG.end() && vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end());
    //                 const string &fromRRG = vertexDFG2RRG[edge.from()];
    //                 const string &toRRG = vertexDFG2RRG[edge.to()];
    //                 if (router.paths().find(fromRRG) == router.paths().end() ||
    //                     router.paths().find(fromRRG)->second.find(toRRG) == router.paths().find(fromRRG)->second.end()) {
    //                     bool routed = router.route({
    //                                                               { fromRRG, toRRG },
    //                                                           },
    //                                                           {
    //                                                               edge.from(),
    //                                                           });
    //                     if (!routed) {
    //                         clog << "VanillaPlacer: FAILED, unroutable cuts. " << endl;
    //                         return { unordered_map<string, string>(), router };
    //                     }
    //                 }
    //             }
    //         }
    //     }
    // }

    //N-order validation
    NetworkAnalyzerLegacy analyzer(analyzerInitial);
    Graph &RRGAnalyzed = analyzer.RRG();
    NOrderValidator validator(validatorInitial);
    // size_t unplacibleCount = 0;
    // for (const auto &vertexDFG : coarseDFG.vertices()) {
    //     if (!getPostfix(vertexDFG.first).empty() || vertexDFG.first.find("block") != string::npos) {
    //         continue;
    //     }
    //     if (coincompat.find(vertexDFG.first) == coincompat.end()) {
    //         WARN << "FastPlacement: Compatible vertices NOT FOUND: " + vertexDFG.first;
    //         return {unordered_map<string, string>(), router};
    //     }
    //     unordered_set<string> compatibles;
    //     for(const auto &vertexRRG: coincompat[vertexDFG.first]){
    //         clog << "\rFastPlacement: -> Validating " << vertexDFG.first << " : " << vertexRRG << "            ";
    //         if (validator.validateSlow(vertexDFG.first, vertexRRG, 2)) { //NorderValidate
    //             compatibles.insert(vertexRRG);
    //         }
    //     }
    //     clog << vertexDFG.first << ": " << coincompat[vertexDFG.first].size() << " -> " << compatibles.size() << "            ";
    //     coincompat[vertexDFG.first] = compatibles;
    // }
    // if(unplacibleCount > 0){
    //     clog << "VanillaPlacer: FAILED, uncompatible vertex found in first order validation. " << endl;
    //     return {unordered_map<string, string>(), router};
    // }

    Goorder.clear();
    for(const auto &vertex: order)
    {
        
        if(vertexDFG2RRG.find(vertex) == vertexDFG2RRG.end())
        {
            Goorder.push_back(vertex);
        }
    }
   
    size_t furthest = 0;
    size_t coarseIter = 0;
    size_t failureCount = 0;
 
    string toMap = Goorder[0];
    
    unordered_set<string> verticesToTry = contractCompat.find(toMap)->second;

    while(coarseIter < Goorder.size()){
        furthest = max(furthest, coarseIter);
        clog << "FastPlacer: New iteration, size of stack: " << vertexDFG2RRG.size() << " / " << travesalVertexDFG.size() << "; furthest: " << furthest << endl;
        bool failed = false;
        string toTry = "";
        unordered_set<string> toDelete;
        // Prepare to delete used RRG vertices
        for(const auto &vertexToTry: verticesToTry){
            if(vertexRRG2DFG.find(vertexToTry) != vertexRRG2DFG.end()){
                toDelete.emplace(vertexToTry);
            }
        }
        bool const_flag = false;
        if(toMap.find("const") != string::npos){
            const_flag = true;
        }

        // Prepare to unconnectable RRG vertice
        for(const auto &vertexToTry: verticesToTry){
            if(toDelete.find(vertexToTry) != toDelete.end()){
                continue;
            }
            if(const_flag){
                string const_dfg = toMap + ".out0" ;
                string pos_rrg ;
                for(const auto &edge: DFG.edgesOut(const_dfg)){
                    if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
                        pos_rrg = split(vertexDFG2RRG[edge.to()],".")[1];
                    }
                }
                if(vertexToTry.find(pos_rrg) == string::npos){
                    toDelete.emplace(vertexToTry);
                }
            }
            
            bool available = true;
            unordered_multimap<string, string> linksToValidate;
            unordered_set<string> portVertex;
            for(const auto &port: contractPorts[toMap]){
                for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                    if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                        continue;
                    }
                    const string &fromRRG = vertexDFG2RRG[edge.from()];
                    const string &toRRG = vertexToTry + "." + port;
                    linksToValidate.insert({fromRRG, toRRG});
                }
                for(const auto &edge: DFG.edgesOut(toMap + "." + port)){
                    if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                        continue;
                    }
                    const string &toRRG = vertexDFG2RRG[edge.to()];
                    const string &fromRRG = vertexToTry + "." + port;
                    linksToValidate.insert({fromRRG, toRRG});
                }
            }
            
            for(const auto &link: linksToValidate){
                bool found = false;
                for(const auto &edge: _RRGAnalyzed.edgesOut(link.first)){
                    if(edge.to() == link.second){
                        found = true;
                        break;
                    }
                }
                if(!found){
                    available = false;
                    break;
                }
            }
            if(!available){
                toDelete.insert(vertexToTry);
            }
        }
        //Delete
        for (const auto &vertex : toDelete) {
            verticesToTry.erase(vertex);
        }
        
        clog << "FastPlacer: -> toMap: " << toMap << "; Candidates After Purge: " << verticesToTry.size() << endl;
        vector<string> verticesToTryRanked(verticesToTry.begin(), verticesToTry.end());
        vector<pair<string, string>> edgesToMap;
        vector<string> edgesSignal;
        unordered_set<string> coreVertex;
        unordered_map<string, string> port2rrg;
        if (verticesToTry.empty()) {
            failed = false;
            clog << "FastPlacer: Failed. Nothing to try for " << toMap << endl;
        } else {
            //  sort the candidates by sharedNet
            unordered_map<string, size_t> distancedNet;
            unordered_set<string> inPortRRG;
            unordered_set<string> outPortRRG;
            for(const auto &vertexToTry: verticesToTry){
                inPortRRG.clear();
                outPortRRG.clear();
                for(const auto &port: contractPorts[toMap]){
                    for(const auto &edge: DFG.edgesIn(toMap + "." + port)){//(out)->in
                        if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                            continue;
                        }
                        string core = getFront(vertexDFG2RRG[edge.from()]);
                        
                        if(vertexToTry.find(core) != string::npos){
                            coreVertex.emplace(vertexToTry);
                            cout <<core <<  "  " << vertexToTry<< endl ;
                        }
                        inPortRRG.emplace(vertexDFG2RRG[edge.from()]);
                        // const string &fromRRG = vertexDFG2RRG[edge.from()];
                        // const string &toRRG = vertexToTry + "." + port;
                        // linksToValidate.insert({fromRRG, toRRG});
                    }
                    for(const auto &edge: DFG.edgesOut(toMap + "." + port)){//out->(in)
                        if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                            continue;
                        }
                        string core = getFront(vertexDFG2RRG[edge.to()]);
                        if(vertexToTry.find(core) != string::npos){
                            coreVertex.emplace(vertexToTry);
                            cout <<core <<  "  " << vertexToTry<< endl ;
                        }
                        outPortRRG.insert(vertexDFG2RRG[edge.to()]);
                        // const string &toRRG = vertexDFG2RRG[edge.to()];
                        // const string &fromRRG = vertexToTry + "." + port;
                        // linksToValidate.insert({fromRRG, toRRG});
                    }
                }
                // for(const auto &edge: DFG.edgesIn(toMap)){
                //     const string &toDFG = edge.to();
                //     const string &toRRG = vertexToTry ;
                //     inPortRRG.insert(toRRG);
                // }
                // // for(const auto &edge: DFG.edgesOut(toMap)){
                // //     const string &fromDFG = edge.from();
                // //     const string &fromRRG = vertexToTry ;
                // //     outPortRRG.insert(fromRRG);
                // // }
                for(const auto &inport: inPortRRG){
                    pair<vector<string>, bool> temp = router.get_distance(inport, vertexToTry);
                    if(temp.second){
                        distancedNet[vertexToTry] += temp.first.size();
                    }
                }
                for(const auto &outport: outPortRRG){
                    pair<vector<string>, bool> temp = router.get_distance(vertexToTry, outport);
                    if(temp.second){
                        distancedNet[vertexToTry] += temp.first.size();
                    }
                    // for(const auto &edge: _RRGAnalyzed.edgesOut(outport)){
                    //     if(vertexRRG2DFG.find(edge.to()) != vertexRRG2DFG.end()){
                    //         distancedNet[vertexToTry]++;
                    //     }
                    // }
                }
            }
            // cout << verticesToTryRanked << endl;



            // cout << coreVertex << endl;
            // cout << "before " << verticesToTryRanked << endl;
            // // random_shuffle(verticesToTryRanked.begin(), verticesToTryRanked.end());
            sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return distancedNet[a] > distancedNet[b];});
            // // cout << verticesToTryRanked << endl;
            
            // cout << verticesToTryRanked << endl;
            // unordered_map<string, size_t> coreNum;
            // if(coreVertex.empty()){
            //     for(const auto &vertex: verticesToTryRanked){
            //         string core = getFront(vertex);
            //         if(coreNum.find(core) == coreNum.end()){
            //             coreNum[core] = 1;
            //         }
            //         else{
            //             coreNum[core] += 1;
            //         }
            //     }
            //     sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return coreNum[a] > coreNum[b];});
            // }
            // else{
            //     verticesToTryRanked.insert(verticesToTryRanked.begin(), coreVertex.begin(),coreVertex.end());
            // }
            
            // cout << "after " << verticesToTryRanked << endl;
            // try map vertex
            bool isSuccess = false;
            size_t IterVertextoTry = 0;
            
            while(!isSuccess && IterVertextoTry < verticesToTryRanked.size()){
                // -> Find edges that need to be mapped
                toTry = verticesToTryRanked[IterVertextoTry++];
                clog << "FastPlacer: try to map " << toMap << " to " << toTry << endl;
                edgesToMap.clear();
                edgesSignal.clear();
                if (!isSuccess) {
                    vector<string> portVertex = {};
                     
                    port2rrg.clear();
                    for(const auto &port: contractPorts[toMap]){
                        port2rrg[toMap + "." + port] = toTry + "." + port;
                        if(port.find("in") != string::npos){
                            edgesToMap.emplace_back(pair<string, string>(toTry + "." + port, toTry));
                            edgesSignal.emplace_back(toMap + "." + port);
                        }
                        else{
                            edgesToMap.emplace_back(pair<string, string>(toTry, toTry + "." + port));
                            edgesSignal.emplace_back(toMap);
                        }
                    }
                    // travesalVertexDFG[toMap] = true;
                    // vertexDFG2RRG[toMap] = toTry;
                    for(const auto &vertex: port2rrg){
                        for (const auto &edge : DFG.edgesIn(vertex.first)) {
                            if (travesalVertexDFG[edge.from()] ) {
                                edgesToMap.emplace_back(pair<string, string>(vertexDFG2RRG[edge.from()], vertex.second));
                                edgesSignal.emplace_back(edge.from());
                            }
                        }
                        for (const auto &edge : DFG.edgesOut(vertex.first)) {
                            if (travesalVertexDFG[edge.to()]) {
                                edgesToMap.emplace_back(pair<string, string>(vertex.second, vertexDFG2RRG[edge.to()]));
                                edgesSignal.emplace_back(vertex.first);
                            }
                        }
                    }
                    // travesalVertexDFG[toMap] = false;
                    // vertexDFG2RRG.erase(toMap);
                    clog << "FastPlacer: edgesToMap: " << edgesToMap.size() << " for " << toTry << endl;
                }
                if(edgesToMap.size() > 0){
                    isSuccess = router.norm_route(edgesToMap, edgesSignal);
                    // if(!isSuccess){
                    //     clog << "FastPlacer: Choose a loose route." << endl;
                    //     isSuccess = router.route(edgesToMap, edgesSignal);
                    // }
                } else {
                    isSuccess = true;
                }
            }
            if(!isSuccess){
                failed = true;
            }
        }
        if(!failed){
            vertexDFG2RRG[toMap] = toTry;
            vertexRRG2DFG[toTry] = toMap;
            travesalVertexDFG[toMap]=true;
            for(const auto &port: port2rrg){
                vertexDFG2RRG[port.first] = port.second;
                vertexRRG2DFG[port.second] = port.first;
                travesalVertexDFG[port.first] = true;
            }
            stackVerticesToTry.push(verticesToTry);
            stackEdgesToMap.push(edgesToMap);

            if(++coarseIter < Goorder.size()){
                toMap = Goorder[coarseIter];
                verticesToTry = contractCompat.find(toMap)->second;
            } else {
                break;
            }
        } else {
            failureCount++;
            if (_failedVertices.find(toMap) == _failedVertices.end()) {
                _failedVertices[toMap] = 0;
            }
            _failedVertices[toMap]++;
            if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
                clog << "FastPlacer: FAILED. Too many failure. " << endl;
                return {unordered_map<string, string>(), routerInit};
            }
            if(coarseIter == 0){
                clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
                return {unordered_map<string, string>(), routerInit};
            }
            toMap = Goorder[--coarseIter];
            verticesToTry = stackVerticesToTry.top();
            stackVerticesToTry.pop();
            clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
            vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
            stackEdgesToMap.pop();
            router.unroute(edgesToDelte);
            vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
            vertexDFG2RRG.erase(toMap);
            for(const auto &port: port2rrg){
                vertexDFG2RRG.erase(port.first);
                vertexRRG2DFG.erase(port.second);
                travesalVertexDFG[port.first] = false;
            }
            travesalVertexDFG[toMap]=false;
        }
        clog << endl << endl;

    }

    clog << "FastPlacer: finished placing the DFG. Failure count: " << failureCount << "." << endl
         << endl;

    return{vertexDFG2RRG, router};

}


std::mutex mtx;
std::atomic<bool> ready(false);
std::pair<std::unordered_map<std::string, std::string>, FastRouter> FastPlacer::place_matching3(const Graph &DFG, const std::unordered_map<std::string, std::unordered_set<std::string>> &compatible, const FastRouter &routerInit, 
                                                        const std::unordered_map<std::string, std::string>  &usedDFG2RRGInitial,  const NetworkAnalyzerLegacy &analyzerInitial, const NOrderValidator &validatorInitial,
                                                        const std::vector<std::vector<std::string>> &order)
{
    const size_t Max_Failed_Times = 64;

    // Clear the previous status
    _failedVertices.clear();
    // Data
    FastRouter router(routerInit);
    vector<string> Goorder = (order[0],order[1]);
    vector<string> Goorder1 = order[0];//order
    vector<string> Goorder2 = order[1];//order
    unordered_map<string, string> vertexDFG2RRG = usedDFG2RRGInitial;
    unordered_map<string, string> vertexRRG2DFG;
    unordered_map<string, bool>   travesalVertexDFG;

    // unordered_map<string, unordered_set<string>> coincompat = compatible;
    stack<vector<pair<string, string>>> stackEdgesToMap1;
    stack<unordered_set<string>> stackVerticesToTry1;
    stack<vector<pair<string, string>>> stackEdgesToMap2;
    stack<unordered_set<string>> stackVerticesToTry2;
    for(const auto &dfg2rrg: vertexDFG2RRG){
        vertexRRG2DFG[dfg2rrg.second] = dfg2rrg.first;
    }
    vector<string> contractVertex;
    unordered_map<string, unordered_set<string>> contractCompat;
    unordered_map<string, unordered_set<string>> contractPorts;
    for(const auto &item: compatible){
        if (getPostfix(item.first).empty()) {
                contractVertex.emplace_back(item.first);
                contractCompat.emplace(item);
        }
    }
    for(const auto &vertexDFG: DFG.vertices())
    {
        travesalVertexDFG[vertexDFG.first] = false;
        if(!getPostfix(vertexDFG.first).empty()){
            string prefix = getPrefix(vertexDFG.first);
            if (contractPorts.find(prefix) == contractPorts.end()) {
                contractPorts[prefix] = {};
            }
            contractPorts[prefix].emplace(getPostfix(vertexDFG.first));
        }
    }
    //N-order validation
    NetworkAnalyzerLegacy analyzer(analyzerInitial);
    Graph &RRGAnalyzed = analyzer.RRG();
    NOrderValidator validator(validatorInitial);
    
    size_t furthest = 0;
    size_t coarseIter1 = 0;
    size_t coarseIter2 = 0;
    size_t failureCount = 0;
    // cout << endl << "FastPlacer: Begin placing. " << endl;
    // Goorder.clear();
    // for(auto it = temp.rbegin(); it != temp.rend(); ++it) {
    //     Goorder.emplace_back(*it);
	// // std::cout << it->first << ", " << it->second << std::endl;
    // }
    // Goorder = temp;
    string toMap1 = Goorder1[0];
    string toMap2 = Goorder2[0];
    bool finised1 = false;
    bool finised2 = false;
    unordered_set<string> verticesToTry1 = contractCompat.find(toMap1)->second;
    unordered_set<string> verticesToTry2 = contractCompat.find(toMap2)->second;
    while((coarseIter1 + coarseIter2) < (Goorder1.size() + Goorder2.size())){
        furthest = max(furthest, coarseIter1 + coarseIter2);
        clog << "FastPlacer: New iteration, size of stack: " << vertexDFG2RRG.size() << " / " << travesalVertexDFG.size() << "; furthest: " << furthest << endl;
        bool failed = false;
        string toTry = "";
        auto prune = [&](string toMap, unordered_set<string> &verticesToTry) {
            unordered_set<string> toDelete;
            // Prepare to delete used RRG vertices
            for(const auto &vertexToTry: verticesToTry){
                if(vertexRRG2DFG.find(vertexToTry) != vertexRRG2DFG.end()){
                    toDelete.emplace(vertexToTry);
                }
            }//const->load/store->alu
            bool const_flag = false;//如果当前节点是pe已经不好，或者它的扇出节点已经布好，那就放上去
            if(toMap.find("const") != string::npos){
                const_flag = true;
            }
            for(const auto &vertexToTry: verticesToTry){
                if(toDelete.find(vertexToTry) != toDelete.end()){
                    continue;
                }
                if(const_flag){
                    string const_dfg = toMap + ".out0" ;
                    string pos_rrg ;
                    for(const auto &edge: DFG.edgesOut(const_dfg)){
                        if(vertexDFG2RRG.find(edge.to()) != vertexDFG2RRG.end()){
                            pos_rrg = split(vertexDFG2RRG[edge.to()],".")[1];
                        }
                    }
                    if(vertexToTry.find(pos_rrg) == string::npos){
                        toDelete.emplace(vertexToTry);
                    }
                }
                // Prepare to unconnectable RRG vertice
                // bool available = true;
                // unordered_multimap<string, string> linksToValidate;
                // unordered_set<string> portVertex;
                // for(const auto &port: contractPorts[toMap]){
                //     for(const auto &edge: DFG.edgesIn(toMap + "." + port)){
                //         if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                //             continue;
                //         }
                //         const string &fromRRG = vertexDFG2RRG[edge.from()];
                //         const string &toRRG = vertexToTry + "." + port;
                //         linksToValidate.insert({fromRRG, toRRG});
                //     }
                //     for(const auto &edge: DFG.edgesOut(toMap + "." + port)){
                //         if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                //             continue;
                //         }
                //         const string &toRRG = vertexDFG2RRG[edge.to()];
                //         const string &fromRRG = vertexToTry + "." + port;
                //         linksToValidate.insert({fromRRG, toRRG});
                //     }
                // }
                // for(const auto &link: linksToValidate){
                //     bool found = false;
                //     for(const auto &edge: _RRGAnalyzed.edgesOut(link.first)){
                //         if(edge.to() == link.second){
                //             found = true;
                //             break;
                //         }
                //     }
                //     if(!found){
                //         available = false;
                //         break;
                //     }
                // }
                // if(!available){
                //     toDelete.insert(vertexToTry);
                // }
            }
            //Delete
            for (const auto &vertex : toDelete) {
                verticesToTry.erase(vertex);
            }
        };

        clog << "FastPlacer: -> toMap: " << toMap1 << "; Candidates Before Purge: " << verticesToTry1.size() << endl;
        clog << "FastPlacer: -> toMap: " << toMap2 << "; Candidates Before Purge: " << verticesToTry2.size() << endl;
        std::thread t1(prune, toMap1, ref(verticesToTry1));
        std::thread t2(prune, toMap2, ref(verticesToTry2));
        t1.join();
        t2.join();  
        
        // if(!finised1){
        //     thread t1(prune, toMap1, verticesToTry1);
        //     // prune(toMap1, verticesToTry1);
        // }
        // if(!finised2){
        //     thread t2(prune, toMap2, verticesToTry2);
        //     // prune(toMap2, verticesToTry2);
        // }
        clog << "FastPlacer: -> toMap: " << toMap1 << "; Candidates After Purge: " << verticesToTry1.size() << endl;
        clog << "FastPlacer: -> toMap: " << toMap2 << "; Candidates After Purge: " << verticesToTry2.size() << endl;

        auto sortPE = [&](string toMap, unordered_set<string> verticesToTry, vector<string> &verticesToTryRanked) {
            unordered_set<string> coreVertex;
            
            if (verticesToTry.empty()) {
                failed = false;
                clog << "FastPlacer: Failed. Nothing to try for " << toMap << endl;
                return failed;
            }
            else{
                //  sort the candidates by sharedNet
                unordered_map<string, size_t> distancedNet;
                unordered_set<string> inPortRRG;
                unordered_set<string> outPortRRG;
                for(const auto &vertexToTry: verticesToTry){
                    inPortRRG.clear();
                    outPortRRG.clear();
                    for(const auto &port: contractPorts[toMap]){
                        for(const auto &edge: DFG.edgesIn(toMap + "." + port)){//(out)->in
                            if(vertexDFG2RRG.find(edge.from()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            string core = getFront(vertexDFG2RRG[edge.from()]);
                            if(vertexToTry.find(core) != string::npos){
                                coreVertex.emplace(vertexToTry);
                                cout <<core <<  "  " << vertexToTry<< endl;
                            }
                            inPortRRG.emplace(vertexDFG2RRG[edge.from()]);
                            // const string &fromRRG = vertexDFG2RRG[edge.from()];
                            // const string &toRRG = vertexToTry + "." + port;
                            // linksToValidate.insert({fromRRG, toRRG});
                        }
                        for(const auto &edge: DFG.edgesOut(toMap + "." + port)){//out->(in)
                            if(vertexDFG2RRG.find(edge.to()) == vertexDFG2RRG.end()){
                                continue;
                            }
                            string core = getFront(vertexDFG2RRG[edge.to()]);
                            if(vertexToTry.find(core) != string::npos){
                                coreVertex.emplace(vertexToTry);
                                cout <<core <<  "  " << vertexToTry<< endl ;
                            }
                            outPortRRG.insert(vertexDFG2RRG[edge.to()]);
                            // const string &toRRG = vertexDFG2RRG[edge.to()];
                            // const string &fromRRG = vertexToTry + "." + port;
                            // linksToValidate.insert({fromRRG, toRRG});
                        }
                    }
                    for(const auto &inport: inPortRRG){
                        pair<vector<string>, bool> temp = router.get_distance(inport, vertexToTry);
                        if(temp.second){
                            distancedNet[vertexToTry] += temp.first.size();
                        }
                    }
                    for(const auto &outport: outPortRRG){
                        pair<vector<string>, bool> temp = router.get_distance(vertexToTry, outport);
                        if(temp.second){
                            distancedNet[vertexToTry] += temp.first.size();
                        }
                    }
                }
                cout << "before " << verticesToTryRanked << endl;
                // random_shuffle(verticesToTryRanked.begin(), verticesToTryRanked.end());
                sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return distancedNet[a] > distancedNet[b];});
                // cout << verticesToTryRanked << endl;
                unordered_map<string, size_t> coreNum;
                if(coreVertex.empty()){
                    for(const auto &vertex: verticesToTryRanked){
                        string core = getFront(vertex);
                        if(coreNum.find(core) == coreNum.end()){
                            coreNum[core] = 1;
                        }
                        else{
                            coreNum[core] += 1;
                        }
                    }
                    sort(verticesToTryRanked.begin(), verticesToTryRanked.end(), [&](const string &a,const string &b){return coreNum[a] > coreNum[b];});
                }
                else{
                    verticesToTryRanked.insert(verticesToTryRanked.begin(), coreVertex.begin(),coreVertex.end());
                }
                return failed;
            }
        };
  
        vector<string> verticesToTryRanked1(verticesToTry1.begin(), verticesToTry1.end());
        vector<string> verticesToTryRanked2(verticesToTry2.begin(), verticesToTry2.end());
        std::thread s1(sortPE, toMap1, verticesToTry1, ref(verticesToTryRanked1));
        std::thread s2(sortPE, toMap2, verticesToTry2, ref(verticesToTryRanked2));
        s1.join();
        s2.join();  
        unordered_set<string> tempPE;

        
        // try map vertex
        bool failed1 = false;
        std::unordered_multimap<std::string, std::pair<std::string, std::string>>                  usedNode1; 
        std::unordered_map<std::string, std::string>                                               usedSignal1; 
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> paths1; 
        unordered_map<string, string> port2rrg1;
        vector<pair<string, string>> edgesToMap1;
        vector<string> edgesSignal1;
        
        bool failed2 = false;
        std::unordered_multimap<std::string, std::pair<std::string, std::string>>                  usedNode2; 
        std::unordered_map<std::string, std::string>                                               usedSignal2; 
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> paths2; 
        unordered_map<string, string> port2rrg2;
        vector<pair<string, string>> edgesToMap2;
        vector<string> edgesSignal2;
        auto map = [&](bool &failed, string toMap, vector<string> verticesToTryRanked, unordered_map<string, string> &port2rrg,
                       unordered_multimap<string, pair<string, string>> &usedNode, unordered_map<string, string> &usedSignal,
                       unordered_map<string, unordered_map<string, vector<string>>> &paths, unordered_set<string> &tempPE, 
                       vector<pair<string, string>> &edgesToMap, vector<string> &edgesSignal){
            
            bool isSuccess = false;
            size_t IterVertextoTry = 0;

            while(!isSuccess && IterVertextoTry < verticesToTryRanked.size()){
                // -> Find edges that need to be mapped
                toTry = verticesToTryRanked[IterVertextoTry++];
                if(tempPE.find(toTry) == tempPE.end()){
                    mtx.lock(); 
                    tempPE.emplace(toTry);
                    mtx.unlock();
                }
                else
                    continue;
                if(vertexRRG2DFG.find(toTry) != vertexRRG2DFG.end())
                    continue;
                
                clog << "FastPlacer: try to map " << toMap << " to " << toTry << endl;
                edgesToMap.clear();
                edgesSignal.clear();
                if (!isSuccess) {
                    vector<string> portVertex = {};
                    port2rrg.clear();
                    for(const auto &port: contractPorts[toMap]){
                        port2rrg[toMap + "." + port] = toTry + "." + port;
                        if(port.find("in") != string::npos){
                            edgesToMap.emplace_back(pair<string, string>(toTry + "." + port, toTry));
                            edgesSignal.emplace_back(toMap + "." + port);
                        }
                        else{
                            edgesToMap.emplace_back(pair<string, string>(toTry, toTry + "." + port));
                            edgesSignal.emplace_back(toMap);
                        }
                    }
                    // travesalVertexDFG[toMap] = true;
                    // vertexDFG2RRG[toMap] = toTry;
                    for(const auto &vertex: port2rrg){
                        for (const auto &edge : DFG.edgesIn(vertex.first)) {
                            if (travesalVertexDFG[edge.from()] ) {
                                edgesToMap.emplace_back(pair<string, string>(vertexDFG2RRG[edge.from()], vertex.second));
                                edgesSignal.emplace_back(edge.from());
                            }
                        }
                        for (const auto &edge : DFG.edgesOut(vertex.first)) {
                            if (travesalVertexDFG[edge.to()]) {
                                edgesToMap.emplace_back(pair<string, string>(vertex.second, vertexDFG2RRG[edge.to()]));
                                edgesSignal.emplace_back(vertex.first);
                            }
                        }
                    }
                    // travesalVertexDFG[toMap] = false;
                    // vertexDFG2RRG.erase(toMap);
                    clog << "FastPlacer: edgesToMap: " << edgesToMap.size() << " for " << toTry << endl;
                }
                if(edgesToMap.size() > 0){
                    // std::lock_guard<std::mutex> lock(mtx);
//                     while (!ready.load()) { // 循环等待原子变量
//     std::this_thread::yield(); // 让出CPU
//   }
                    isSuccess = router.try_route(edgesToMap, edgesSignal, usedNode, usedSignal, paths);
                    // if(!isSuccess){
                    //     clog << "FastPlacer: Choose a loose route." << endl;
                    //     isSuccess = router.route(edgesToMap, edgesSignal);
                    // }
                } else {
                    isSuccess = true;
                }
                mtx.lock(); 
                tempPE.erase(toTry);
                mtx.unlock();
            }

            if(!isSuccess){
                failed = true;
            }
        };
        
        auto remap = [&](auto &failed, auto &toMap, auto &verticesToTryRanked, auto &coarseIter, 
                        auto &node_order, auto &stackVerticesToTry, auto &stackEdgesToMap, auto &verticesToTry, auto &finised,
                        unordered_map<string, string> port2rrg, vector<pair<string, string>> &edgesToMap, vector<string> &edgesSignal){
            // vector<pair<string, string>> edgesToMap;
            // vector<string> edgesSignal;
            // unordered_map<string, string> port2rrg;
            bool isSuccess = false;
            size_t IterVertextoTry = 0;
            while(!isSuccess && IterVertextoTry < verticesToTryRanked.size()){
                // -> Find edges that need to be mapped
                toTry = verticesToTryRanked[IterVertextoTry++];
                if(vertexRRG2DFG.find(toTry) != vertexRRG2DFG.end())
                    continue;
                clog << "FastPlacer: try to map " << toMap << " to " << toTry << endl;
                // edgesToMap.clear();
                // edgesSignal.clear();
                if (!isSuccess) {
                    vector<string> portVertex = {};
  
                    clog << "FastPlacer: edgesToMap: " << edgesToMap.size() << " for " << toTry << endl;
                }
                if(edgesToMap.size() > 0){
                    isSuccess = router.norm_route(edgesToMap, edgesSignal);
                    // if(!isSuccess){
                    //     clog << "FastPlacer: Choose a loose route." << endl;
                    //     isSuccess = router.route(edgesToMap, edgesSignal);
                    // }
                } else {
                    isSuccess = true;
                }
            }
            if(!isSuccess){
                failed = true;
            }

            if(!failed){
                vertexDFG2RRG[toMap] = toTry;
                vertexRRG2DFG[toTry] = toMap;
                travesalVertexDFG[toMap]=true;
                for(const auto &port: port2rrg){
                    vertexDFG2RRG[port.first] = port.second;
                    vertexRRG2DFG[port.second] = port.first;
                    travesalVertexDFG[port.first] = true;
                }
                stackVerticesToTry.push(verticesToTry);
                stackEdgesToMap.push(edgesToMap);

                if(++coarseIter < node_order.size()){
                    toMap = node_order[coarseIter];
                    verticesToTry = contractCompat.find(toMap)->second;
                } else {
                    finised = true;
                }
            }
            else{
                failureCount++;
                if (_failedVertices.find(toMap) == _failedVertices.end()) {
                    _failedVertices[toMap] = 0;
                }
                _failedVertices[toMap]++;
                if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
                    clog << "FastPlacer: FAILED. Too many failure. " << endl;
                    // return {unordered_map<string, string>(), routerInit};
                    return false;
                }
                if(coarseIter == 0){
                    clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
                    // return {unordered_map<string, string>(), routerInit};
                    return false;
                }
                toMap = node_order[--coarseIter];
                verticesToTry = stackVerticesToTry.top();
                stackVerticesToTry.pop();
                clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
                vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
                stackEdgesToMap.pop();
                router.unroute(edgesToDelte);
                vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
                vertexDFG2RRG.erase(toMap);
                for(const auto &port: port2rrg){
                    vertexDFG2RRG.erase(port.first);
                    vertexRRG2DFG.erase(port.second);
                    travesalVertexDFG[port.first] = false;
                }
                travesalVertexDFG[toMap]=false;
            }
        };
        

        if(!finised1 && !finised2){

            std::thread map1(map, ref(failed1), toMap1, verticesToTryRanked1, ref(port2rrg1), ref(usedNode1), ref(usedSignal1), ref(paths1), ref(tempPE), ref(edgesToMap1), ref(edgesSignal1));
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            std::thread map2(map, ref(failed2), toMap2, verticesToTryRanked2, ref(port2rrg2), ref(usedNode2), ref(usedSignal2), ref(paths2), ref(tempPE), ref(edgesToMap2), ref(edgesSignal2));
            map1.join();
            map2.join(); 
            // cout << port2rrg2 << endl;
            // cout << usedNode1 << endl;
            // cout << usedSignal1 << endl;cout << paths1 << endl;//exit(0);
            // std::thread map1(map, failed1, ref(toMap1), verticesToTryRanked1, ref(coarseIter1), Goorder1, ref(stackVerticesToTry1), ref(stackEdgesToMap1), ref(verticesToTry1),ref(finised1), ref(tempPE));
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));
            // std::thread map2(map, failed2, ref(toMap2), verticesToTryRanked2, ref(coarseIter2), Goorder2, ref(stackVerticesToTry2), ref(stackEdgesToMap2), ref(verticesToTry2),ref(finised2), ref(tempPE));
            
             
            // map(failed1, ref(toMap1), verticesToTryRanked1, ref(coarseIter1), Goorder1, ref(stackVerticesToTry1), ref(stackEdgesToMap1), ref(verticesToTry1),ref(finised1), ref(tempPE));
            // map(failed2, ref(toMap2), verticesToTryRanked2, ref(coarseIter2), Goorder2, ref(stackVerticesToTry2), ref(stackEdgesToMap2), ref(verticesToTry2),ref(finised2), ref(tempPE));
        }
               //  vector<string> node_order, stack<unordered_set<string>> &stackVerticesToTry, 
                    
        auto update = [&](bool failed, string &toMap, unordered_map<string, string> port2rrg, size_t &coarseIter, bool &finised, unordered_set<string> &verticesToTry,
                          vector<string> node_order, stack<unordered_set<string>> &stackVerticesToTry, stack<vector<pair<string, string>>> &stackEdgesToMap, 
                          vector<pair<string, string>> &edgesToMap, vector<string> &edgesSignal, unordered_multimap<string, pair<string, string>> &usedNode, unordered_map<string, string> &usedSignal,
                       unordered_map<string, unordered_map<string, vector<string>>> &paths){
            if(!failed){
                router.update(usedNode, usedSignal, paths);
                vertexDFG2RRG[toMap] = toTry;
                vertexRRG2DFG[toTry] = toMap;
                travesalVertexDFG[toMap]=true;
                for(const auto &port: port2rrg){
                    vertexDFG2RRG[port.first] = port.second;
                    vertexRRG2DFG[port.second] = port.first;
                    travesalVertexDFG[port.first] = true;
                }
                stackVerticesToTry.push(verticesToTry);
                stackEdgesToMap.push(edgesToMap);

                if(++coarseIter < node_order.size()){
                    toMap = node_order[coarseIter];
                    verticesToTry = contractCompat.find(toMap)->second;
                } else {
                    finised = true;
                }
            }
            else{
                failureCount++;
                if (_failedVertices.find(toMap) == _failedVertices.end()) {
                    _failedVertices[toMap] = 0;
                }
                _failedVertices[toMap]++;
                if( _failedVertices[toMap] > Max_Failed_Times || failureCount > 32 * Max_Failed_Times){
                    clog << "FastPlacer: FAILED. Too many failure. " << endl;
                    // return {unordered_map<string, string>(), routerInit};
                    return false;
                }
                if(coarseIter == 0){
                    clog << "FastPlacer: ALL FAILED. Nothing to try. " << endl;
                    // return {unordered_map<string, string>(), routerInit};
                    return false;
                }
                toMap = node_order[--coarseIter];
                verticesToTry = stackVerticesToTry.top();
                stackVerticesToTry.pop();
                clog << "FastPlacer: Roll back to: " << toMap << "; Candidates: " << verticesToTry.size() << endl;
                vector<pair<string, string>> edgesToDelte = stackEdgesToMap.top();
                stackEdgesToMap.pop();
                router.unroute(edgesToDelte);
                vertexRRG2DFG.erase(vertexDFG2RRG[toMap]);
                vertexDFG2RRG.erase(toMap);
                for(const auto &port: port2rrg){
                    vertexDFG2RRG.erase(port.first);
                    vertexRRG2DFG.erase(port.second);
                    travesalVertexDFG[port.first] = false;
                }
                travesalVertexDFG[toMap]=false;
            }
        };
        {   
            bool check = false;
            for (const auto &elem : usedSignal1) {
                if (usedSignal2.find(elem.first) != usedSignal2.end()){
                        check = true;
                        break;
                }
            }
            if(check){
                if(usedSignal1.size() >= usedSignal2.size()){
                    update(failed1, toMap1, port2rrg1, coarseIter1, finised1, verticesToTry1, Goorder1, stackVerticesToTry1, stackEdgesToMap1, edgesToMap1, edgesSignal1, usedNode1, usedSignal1, paths1);
                    // update(1);
                    remap(failed2, toMap2, verticesToTryRanked2, coarseIter2, Goorder2, stackVerticesToTry2, stackEdgesToMap2, verticesToTry2, finised2, port2rrg2, edgesToMap2, edgesSignal2);
                }
                else{
                    update(failed2, toMap2, port2rrg2, coarseIter2, finised2, verticesToTry2, Goorder2, stackVerticesToTry2, stackEdgesToMap2, edgesToMap2, edgesSignal2, usedNode2, usedSignal2, paths2);
                    // update(2);
                    // remap(1);
                    remap(failed1, toMap1, verticesToTryRanked1, coarseIter1, Goorder1, stackVerticesToTry1, stackEdgesToMap1, verticesToTry1, finised1, port2rrg1, edgesToMap1, edgesSignal1);
                }
            }
            else{
                update(failed1, toMap1, port2rrg1, coarseIter1, finised1, verticesToTry1, Goorder1, stackVerticesToTry1, stackEdgesToMap1, edgesToMap1, edgesSignal1, usedNode1, usedSignal1, paths1);
                update(failed2, toMap2, port2rrg2, coarseIter2, finised2, verticesToTry2, Goorder2, stackVerticesToTry2, stackEdgesToMap2, edgesToMap2, edgesSignal2, usedNode2, usedSignal2, paths2);
            }
        }
        
        if(finised1 || finised2){
            return{vertexDFG2RRG, router};
        }
    }

    clog << "FastPlacer: finished placing the DFG. Failure count: " << failureCount << "." << endl
         << endl;

    return{vertexDFG2RRG, router};

}


}

