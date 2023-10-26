#ifndef __FASTPLACE__
#define __FASTPLACE__

#include "./common/Common.h"
#include "./common/HyperGraph.h"
#include "./util/NOrderValidator.h"
#include "./util/NetworkAnalyzer.h"
#include "./util/GraphSort.h"
#include "./util/Utils.h"
#include "./model/FastPlacer.h"
#include "./model/FastRouter.h"

namespace CGRA
{
    namespace FastPlace
    {

        std::unordered_map<std::string, std::unordered_set<std::string>> updateCompaTable(const Graph &DFG, const std::unordered_map<std::string, std::unordered_set<std::string>> &compat,
                    const std::unordered_map<std::string, std::unordered_set<std::string>> &FUs);

        void deleteForbiddened(std::unordered_map<std::string, std::unordered_set<std::string>> &compat, const std::unordered_multimap<std::string, std::string> &forbid);

        std::vector<std::string> sortDFG(const Graph &dfg, const std::string &sortMode);

        std::vector<std::string> sortDFG(const Graph &dfg, const std::string &sortMode, const std::string &seed);
        inline bool isFileExists(const std::string &name)   { std::ifstream f(name.c_str()); return f.good();}

        bool PlaceMatch1(const std::string &predict, const std::string &compat,const std::string &dfgGlobal, 
                    const std::string &rrg, const std::string &fus, const std::string &mapped, const std::string &routed, 
                    const std::string &sortMode, const std::unordered_multimap<std::string, std::string> &forbidAdditional, const std::string &seed);
        bool PlaceMatch2(const std::string &predict, const std::string &compat,const std::string &dfgGlobal, 
                    const std::string &rrg, const std::string &fus, const std::string &mapped, const std::string &routed, 
                    const std::string &sortMode, const std::unordered_multimap<std::string, std::string> &forbidAdditional, const std::string &seed);
    
    }
}

#endif