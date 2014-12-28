#ifndef DEBUG_EXEINFO
#define DEBUG_EXEINFO

class DEBUG_Exeinfo {
    Bit16u prevSeg;
    Bit16u count;

public:
    DEBUG_Exeinfo()
        : prevSeg(0),
          count(0)
    {
    }

    void AddRelocationSegment(Bit16u seg);
    static void SegmentName(Bit16u seg, char * string, int n);
};

#endif
