#include "debug_exeinfo.h"

#include <string>
#include <map>
#include <vector>

typedef std::map<Bit16u, std::string> RelocationMap;
RelocationMap relocations;

void DEBUG_Exeinfo::AddRelocationSegment(Bit16u seg)
{
    if (seg == this->prevSeg)
        return;

    char name[50];
    snprintf(name, sizeof(name), "seg%03d", this->count);
    relocations[seg] = name;
    this->prevSeg = seg;
    this->count++;

    n_log("added segment %s 0x%04x\n", name, seg);
}

//static
void DEBUG_Exeinfo::SegmentName(Bit16u seg, char * string, int n)
{
    RelocationMap::iterator it = relocations.find(seg);
    if (it != relocations.end()) {
        strncpy(string, it->second.c_str(), n);
    }
    else {
        snprintf(string, n, "%04X", seg);
    }

    string[n-1] = 0;
}
