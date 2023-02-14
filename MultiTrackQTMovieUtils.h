#ifdef EMSCRIPTEN
    typedef unsigned long long U64;
    typedef long long S64;
#else
    typedef unsigned long U64;
    typedef long S64;
#endif

namespace MultiTrackQTMovie {

    typedef struct {
        unsigned short width;
        unsigned short height;
        unsigned short depth;
        double fps;
        std::string type;
    } TrackInfo;

#ifdef EMSCRIPTEN
    
    U64 toU64(unsigned char *p) {
        U64 p0 = p[0];
        U64 p1 = p[1];
        U64 p2 = p[2];
        U64 p3 = p[3];
        U64 p4 = p[4];
        U64 p5 = p[5];
        U64 p6 = p[6];
        U64 p7 = p[7];
        return p7<<56|p6<<48|p5<<40|p4<<32|p3<<24|p2<<16|p1<<8|p0;
    }
    
    unsigned int toU32(unsigned char *p) {
        return p[3]<<24|p[2]<<16|p[1]<<8|p[0];
    }
    
    unsigned short toU16(unsigned char *p) {
        return p[1]<<8|p[0];
    }
    
    U64 swapU64(U64 n) {
        U64 p0 = (n>>56)&0xFF;
        U64 p1 = (n>>48)&0xFF;
        U64 p2 = (n>>40)&0xFF;
        U64 p3 = (n>>32)&0xFF;
        U64 p4 = (n>>24)&0xFF;
        U64 p5 = (n>>16)&0xFF;
        U64 p6 = (n>>8)&0xFF;
        U64 p7 = (n)&0xFF;
        return p7<<56|p6<<48|p5<<40|p4<<32|p3<<24|p2<<16|p1<<8|p0;
    }
    
#else
    
    U64 toU64(unsigned char *p) {
        return *((U64 *)p);
    }
    
    unsigned int toU32(unsigned char *p) {
        return *((unsigned int *)p);
    }
    
    unsigned short toU16(unsigned char *p) {
        return *((unsigned short *)p);
    }
    
    U64 swapU64(U64 n) {
        return ((n>>56)&0xFF)|(((n>>48)&0xFF)<<8)|(((n>>40)&0xFF)<<16)|(((n>>32)&0xFF)<<24)|(((n>>24)&0xFF)<<32)|(((n>>16)&0xFF)<<40)|(((n>>8)&0xFF)<<48)|((n&0xFF)<<56);
    }
    
#endif
    
    unsigned int swapU32(unsigned int n) {
        return ((n>>24)&0xFF)|(((n>>16)&0xFF)<<8)|(((n>>8)&0xFF)<<16)|((n&0xFF)<<24);
    }
    
    unsigned short swapU16(unsigned short n) {
        return ((n>>8)&0xFF)|((n&0xFF)<<8);
    }

}