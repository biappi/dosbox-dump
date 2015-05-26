#ifndef DEBUG_EXEINFO
#define DEBUG_EXEINFO

class DEBUG_Exeinfo {
    const char * name;
    Bit16u relocation;

    Bit16u prevSeg;
    Bit16u count;

public:
    DEBUG_Exeinfo()
        : prevSeg(0),
          count(0)
    {
    }

    void Name(char * name) { this->name = name; };
    void Relocation(Bit16u i) { this->relocation = i; };

    void AddRelocationSegment(Bit16u seg);

    static void SegmentName(Bit16u seg, char * string, int n);
    static void ShowRelocations();
};

#endif
