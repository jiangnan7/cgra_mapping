#ifndef __CGRA_FastMatcher__
#define __CGRA_FastMatcher__

#include "common/Common.h"
#include "common/HyperGraph.h"

namespace CGRA
{

class FastMatcher
{
private: 
    std::unordered_map<std::string, std::string> _matchTable; 
    std::unordered_map<std::string, std::string> _matchTableRev; 
    std::unordered_map<std::string, std::unordered_set<std::string>> _candidates; 

public: 
    FastMatcher() {}
    FastMatcher(const std::unordered_map<std::string, std::unordered_set<std::string>> &candidates): _candidates(candidates) {}
    FastMatcher(const FastMatcher &matcher): _matchTable(matcher._matchTable), _matchTableRev(matcher._matchTableRev), _candidates(matcher._candidates) {}
    FastMatcher(FastMatcher &&matcher): _matchTable(std::move(matcher._matchTable)), _matchTableRev(std::move(matcher._matchTableRev)), _candidates(std::move(matcher._candidates)) {}
    
    const std::unordered_map<std::string, std::string> &matchTable()    const {return _matchTable; }
    const std::unordered_map<std::string, std::string> &matchTableRev() const {return _matchTableRev; }

    std::unordered_map<std::string, std::unordered_set<std::string>>       &candidates()       {return _candidates; }
    const std::unordered_map<std::string, std::unordered_set<std::string>> &candidates() const {return _candidates; }

    bool _match(const std::string &name, std::unordered_set<std::string> &visited); 
    size_t match(); 
}; 

}

#endif
