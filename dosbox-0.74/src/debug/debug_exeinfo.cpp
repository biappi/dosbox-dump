#include "debug_exeinfo.h"

#include <string>
#include <map>
#include <vector>

typedef std::map<Bit16u, std::string> RelocationMap;
RelocationMap relocations;

struct RelocationInfo {
    Bit16u relocation;
    RelocationMap segmentsMap;
};

typedef std::map<std::string, struct RelocationInfo> FilesMap;
FilesMap filesToRelocation;

void DEBUG_Exeinfo::AddRelocationSegment(Bit16u seg)
{
    if (seg == this->prevSeg)
        return;

    char name[50];
    snprintf(name, sizeof(name), "seg%03d", this->count);
    relocations[seg] = name;
    this->prevSeg = seg;
    this->count++;

    std::string the_name(this->name);
    filesToRelocation[the_name].relocation = this->relocation;
    filesToRelocation[std::string(this->name)].segmentsMap[seg] = name;

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

//static
void DEBUG_Exeinfo::ShowRelocations()
{
    for (FilesMap::iterator iter = filesToRelocation.begin();
         iter != filesToRelocation.end();
         iter++)
    {
        DEBUG_ShowMsg("file: %s; relocation: %04x", iter->first.c_str(), iter->second.relocation);

        for (RelocationMap::iterator it2 = iter->second.segmentsMap.begin();
             it2 != iter->second.segmentsMap.end();
             it2++)
        {
            DEBUG_ShowMsg("file: %s; %04x: %s",
                          iter->first.c_str(),
                          it2->first,
                          it2->second.c_str());
        }
    }
}
