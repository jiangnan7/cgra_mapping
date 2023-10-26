#include "./common/Common.h"
#include "./common/Logger.h"
#include "./common/HyperGraph.h"
#include "./common/HierGraph.h"
#include "./util/NetworkAnalyzer.h"
#include "./util/GraphSort.h"
#include "./util/Utils.h"
#include "./mapping/model/FastRouter.h"

using namespace std; 
using namespace CGRA; 

int main(int argc, char **argv){
    srand(time(nullptr));
    unordered_map<string, unordered_map<string, vector<string>>> config = Utils::readConifgFile("./arch/arch.ini");

    string RRG = config["Global"]["TopRRG"][0];
    string FUs = config["Global"]["TopFUs"][0];
    string TopRRGAnalyzed = config["Global"]["TopRRGAnalyzed"][0];
    string TopLinksAnalyzed = config["Global"]["TopLinksAnalyzed"][0];
    string cacheFile = config["Global"]["cache"][0];
    Graph graphRRG(RRG);
    unordered_map<string, unordered_set<string>> FU = NetworkAnalyzer::parse(FUs); 
    NetworkAnalyzerLegacy analyzer(FU, graphRRG);
    analyzer.dumpAnalysis(TopRRGAnalyzed, TopLinksAnalyzed);

    unordered_map<string, unordered_set<string>> links = analyzer.linksTable(); 
    // unordered_set<string> needFU = {"MEM", "IN", "OUT", "ALU", "func", "mem_unit", "OE"};
    // rf, const
    // FastRouter router(graphRRG, FU);
    // vector<pair<string, string>> edgesToMap;
    // vector<string> edgesSignal;
    // size_t i = 0;
    // for(const auto &froms: links)
    // {   
    //     if(froms.first.find("rf") != string::npos){
    //         continue;
    //     }
    //     for(const auto &tos: froms.second)
    //     {   
    //         edgesToMap.clear();
    //         edgesSignal.clear();
    //         router.forbid({});
    //         if(tos.find("rf") != string::npos || getPrefix(froms.first) == getPrefix(tos)){//tos.find("const") != string::npos ||
    //             continue;
    //         }
    //         if(froms.first.find("const") != string::npos && tos.find("func") != string::npos){
    //             continue;
    //         }
    //         string temp = num2str(i++);
    //         edgesToMap.emplace_back(pair<string, string>(froms.first, tos));
    //         edgesSignal.emplace_back(temp); 
            
    //         string pathname = froms.first + "->" + tos;
    //         ifstream fin(cacheFile + pathname + string(".txt"));
    //         if(!fin){
    //             fin.close();
    //             router.search_route(edgesToMap, edgesSignal);
    //             for(const auto &path: router.cache())
    //             {
    //                 ofstream fout(cacheFile + pathname + string(".txt")); 
    //                 for(const auto &pathCandidates: path.second){
    //                     for(const auto &candidate: pathCandidates){
    //                         fout << candidate << " "; 
    //                     }
    //                     fout << endl;
    //                 }
    //                 fout.close(); 
    //             }
    //         }
    //     }
    // }
    return 0;
}   