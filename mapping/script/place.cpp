#include "./common/Common.h"
#include "./common/Logger.h"
#include "./common/HyperGraph.h"
#include "./common/HierGraph.h"
#include "./util/NetworkAnalyzer.h"
#include "./util/GraphSort.h"
#include "./util/Utils.h"
#include "./mapping/model/FastRouter.h"
#include "./mapping/model/FastPlacer.h"
#include "./mapping/FastPlace.h"

using namespace std; 
using namespace CGRA; 

int main(int argc, char **argv){

    string way        = string(argv[1]);
    srand(time(nullptr));
    bool result = false;
    
    unordered_map<string, unordered_map<string, vector<string>>> config = Utils::readConifgFile("./arch/arch.ini");

    string RRG = config["Global"]["TopRRG"][0];
    string FUs = config["Global"]["TopFUs"][0];
    string TopRRGAnalyzed = config["Global"]["TopRRGAnalyzed"][0];
    string TopLinksAnalyzed = config["Global"]["TopLinksAnalyzed"][0];
    
    Graph graphRRG(RRG);
    unordered_map<string, unordered_set<string>> FU = NetworkAnalyzer::parse(FUs); 
    NetworkAnalyzerLegacy analyzer(FU, graphRRG);
    analyzer.dumpAnalysis(TopRRGAnalyzed, TopLinksAnalyzed);
    NetworkAnalyzerLegacy::setDefault(TopRRGAnalyzed, TopLinksAnalyzed);
    if(way == "place_matching"){
        assert(argc >= 4);
        string globalDfg  = string(argv[2]); 
        string compat     = string(argv[3]);
        string sortMode   = "TVS";
        string seed       = "";
        if(argc >= 5){
            sortMode = string(argv[4]);
        } else if(argc >= 6){
            seed = string(argv[5]);
        } else if (argc >= 7) {
            // srand((unsigned)stoi(string(argv[7])));
        } else {
            // srand(time(nullptr));
        }
        string predict= globalDfg.substr(0, globalDfg.size() - 7) + "predict.txt";
        string mapped = globalDfg.substr(0, globalDfg.size() - 3) + "placed.txt";
        string routed = globalDfg.substr(0, globalDfg.size() - 3) + "routed.txt";

        result = FastPlace::PlaceMatch1(predict, compat, globalDfg, RRG, FUs, 
                                mapped, routed, sortMode, {}, seed);
    }
    else if(way == "place_matching2"){
        assert(argc >= 4);
        // string predict        = string(argv[2]); 
        string globalDfg  = string(argv[2]); 
        string compat     = string(argv[3]);
        string sortMode   = "TVS";
        string seed       = "";
        if(argc >= 5){
            sortMode = string(argv[4]);
        } else if(argc >= 6){
            seed = string(argv[5]);
        } else if (argc >= 7) {
            // srand((unsigned)stoi(string(argv[7])));
        } else {
            // srand(time(nullptr));
        }
        string predict= globalDfg.substr(0, globalDfg.size() - 7) + "predict.txt";
        cout << predict << endl;
        
        string mapped = globalDfg.substr(0, globalDfg.size() - 3) + "placed.txt";
        string routed = globalDfg.substr(0, globalDfg.size() - 3) + "routed.txt";

        result = FastPlace::PlaceMatch2(predict, compat, globalDfg, RRG, FUs, 
                                mapped, routed, sortMode, {}, seed);
    }
    else if(way == "validate")
    {
        string dfg        = string(argv[2]); 
        string compat     = string(argv[3]);

        NetworkAnalyzerLegacy::setDefault(TopRRGAnalyzed, TopLinksAnalyzed);

        string mapped = dfg.substr(0, dfg.size() - 3) + "placed.txt";
        string routed = dfg.substr(0, dfg.size() - 3) + "routed.txt";

        result = Utils::validate(dfg, RRG, compat, mapped, routed);
    }
    return !result;
}