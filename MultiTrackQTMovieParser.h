#import <vector>
#import <string>

#import "MultiTrackQTMovieUtils.h"

namespace MultiTrackQTMovie {

    class Parser {
        
        private:
        
            unsigned int _tracks = 0;
            std::vector<TrackInfo *> _info;
            std::vector<unsigned int> _totalFrames;
            std::vector<std::pair<U64,unsigned int> *> _frames;
        
#ifdef EMSCRIPTEN
    
            unsigned char *_bytes;
            U64 _length;
    
#else 
        
            FILE *_fp;
            
            NSData *cut(off_t offset, size_t bytes) {
                if(this->_fp) {
                    unsigned char *data = new unsigned char[bytes];
                    if(fseeko(this->_fp,offset,SEEK_SET)!=-1) {
                        fread(data,1,bytes,this->_fp);
                        NSData *buffer = [NSData dataWithBytes:data length:bytes];
                        delete[] data;
                        return buffer;
                    }
                }
                return nil;
            }
#endif

            void reset() {
                this->_info.clear();
                this->_totalFrames.clear();
                this->_frames.clear();
            }

            void clear() {
                
                for(int n=0; n<this->_frames.size(); n++) {
                    delete[] this->_frames[n];
                }
                
                this->_tracks = 0;

                this->reset();
                
            }
        
        public:
            
            unsigned int tracks() {
                return this->_tracks;
            }
            
            unsigned int length(unsigned int trackid) {
                if(trackid<this->_totalFrames.size()) {
                    return this->_totalFrames[trackid];
                }
                return 0;
            };
            
            unsigned short width(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).width;
                }
                return 0;
            };
            
            unsigned short height(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).height;
                }
                return 0;
            };
            
            unsigned int depth(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).depth;
                }
                return 0;
            };
            
            std::string type(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).type;
                }
                return "";
            };
            
            unsigned int FPS(unsigned int trackid) {
                if(trackid<this->_info.size()) {
                    return (*this->_info[trackid]).fps;
                }
                return 0;
            };
        

            unsigned int atom(std::string str) {
                
#ifndef EMSCRIPTEN
                assert(str.length()==4);
#endif
                unsigned char *key =(unsigned char *)str.c_str();
                return key[0]<<24|key[1]<<16|key[2]<<8|key[3];
            }
            
            std::string atom(unsigned int v) {
                char str[5];
                str[0] = (v>>24)&0xFF;
                str[1] = (v>>16)&0xFF;
                str[2] = (v>>8)&0xFF;
                str[3] = (v)&0xFF;
                str[4] = '\0';
                
                return str;
            }
        
            void parseTrack(unsigned char *moov, int len) {
                
                std::vector<unsigned int> track;
                
                for(int k=0; k<len; k++) {
                    if(swapU32(toU32(moov+k))==atom("trak")) {
                        track.push_back(k);
                    }
                }
                
                this->_tracks = (unsigned int)track.size();
                
                unsigned int TimeScale = 0;
                for(int k=0; k<len-(4*4); k++) {
                    if(swapU32(toU32(moov+k))==atom("mvhd")) {
                        TimeScale = swapU32(toU32(moov+k+(4*4)));
                        break;
                    }
                }
                
                std::vector<TrackInfo> info;
                                
                for(int n=0; n<track.size(); n++) {
                    
                    TrackInfo *info = nullptr;
                    
                    unsigned int begin = track[n];
                    unsigned int end = begin + swapU32(toU32(moov+track[n]-4))-4;
                    
                    unsigned int info_offset[4];
                    info_offset[0] = 4*4; 
                    info_offset[1] = info_offset[0]+4+6+2+2+2+4+4+4; 
                    info_offset[2] = info_offset[1]+2; 
                    info_offset[3] = info_offset[2]+4+4+4+2+32+2; 
                    
                    for(int k=begin; k<end-info_offset[3]; k++) {
                        if(swapU32(toU32(moov+k))==this->atom("stsd")) {
                            
                            info = new TrackInfo;
                            info->type   = atom(swapU32(toU32(moov+k+info_offset[0])));
                            info->width  = swapU16(toU16(moov+k+info_offset[1]));
                            info->height = swapU16(toU16(moov+k+info_offset[2]));
                            info->depth  = swapU16(toU16(moov+k+info_offset[3]));
                                                        
                            break;
                        }
                    }
                    
                    if(info) {
                        
                        double fps = 0;
                        for(int k=begin; k<end-(4*4); k++) {
                            if(swapU32(toU32(moov+k))==atom("stts")) {
                                fps = TimeScale/(double)(swapU32(toU32(moov+k+(4*4))));
                                break;
                            }
                        }
                        
                        if(fps>0) {
                            
                            info->fps = fps;
                            this->_info.push_back(info);
                            
                            unsigned int totalFrames = 0;
                            std::pair<U64,unsigned int> *frames = nullptr;
                            
                            for(int k=begin; k<end-((4*3)+4); k++) {
                                if(swapU32(toU32(moov+k))==atom("stsz")) {
                                    k+=(4*3);
                                    totalFrames = swapU32(toU32(moov+k));
                                    if(frames) delete[] frames;
                                    frames = new std::pair<U64,unsigned int>[totalFrames];
                                    for(int f=0; f<totalFrames; f++) {
                                        k+=4; // 32
                                        unsigned int size = swapU32(toU32(moov+k));
                                        frames[f] = std::make_pair(0,size);
                                    }
                                    break;
                                }
                            }
                            
                            this->_totalFrames.push_back(totalFrames);
                            
                            for(int k=begin; k<end-((4*2)+4); k++) {
                                if(swapU32(toU32(moov+k))==atom("stco")) {
                                    k+=(4*2);
                                    if(totalFrames==swapU32(toU32(moov+k))) {
                                        k+=4;
                                        for(int f=0; f<totalFrames; f++) {
                                            frames[f].first = swapU32(toU32(moov+k));
                                            k+=4; // 32
                                        }
                                        break;
                                    }
                                }
                                else if(swapU32(toU32(moov+k))==atom("co64")) {
                                    k+=(4*2);
                                    if(totalFrames==swapU32(toU32(moov+k))) {
                                        k+=4;
                                        for(int f=0; f<totalFrames; f++) {
                                            frames[f].first = swapU64(toU64(moov+k));
                                            k+=8; // 64
                                        }
                                        break;
                                    }
                                }
                            }
                            
                            this->_frames.push_back(frames);
                            
                        }
                    }
                }
                
                if((this->_info.size()!=this->_tracks)||(this->_totalFrames.size()!=this->_tracks)||(this->_frames.size()!=this->_tracks)) {
                    
                    this->clear();
                }
                
            }


#ifdef EMSCRIPTEN 
        
            unsigned char *bytes() {
                return this->_bytes;
            };
        
            bool get(unsigned int current,unsigned int trackid,U64 *offset, unsigned int *size) {
                
                if(current<this->_totalFrames[trackid]) {
                    *offset = this->_frames[trackid][current].first;
                    *size = this->_frames[trackid][current].second;
                    return true;
                }
                
                return false;
            }
        
            void parse(unsigned char *bytes, U64 length) {
            
                this->reset();
                
                if(bytes&&length>0) {
                    
                    this->_bytes = bytes;
                    this->_length = length;
                    
                    unsigned char *seek = this->_bytes;
                    seek+=(4+4)+(swapU32(toU32(seek)))+(4);
                    
                    if(swapU32(toU32(seek))==this->atom("mdat")) {
                                                
                        seek+=swapU32(toU32(seek-4));
                        
                        while(true) {
                            if(swapU32(toU32(seek))==this->atom("mdat")) {
                                seek+=swapU32(toU32(seek-4));
                            }
                            else if(swapU32(toU32(seek))==this->atom("moov")) {
                                unsigned char *moov = seek+4;
                                this->parseTrack(moov,(swapU32(toU32(moov-8))-4)-3);
                                break;
                            }
                            else {
                                break;
                            }
                        }
                    }
                }

            }
        
            Parser(unsigned char *bytes, U64 length) {
                this->parse(bytes,length);
            }
            
            ~Parser() {
                this->_length = 0;
                this->_bytes = nullptr;
                this->clear();
            }
            
#else
        
            U64 size() {
                fpos_t size = 0;
                fseeko(this->_fp,0,SEEK_END);
                fgetpos(this->_fp,&size);
                return size;
            }
            
            NSData *get(U64 n,unsigned int tracks) {
                if(n<this->_totalFrames[tracks]) {
                    return this->cut(this->_frames[tracks][n].first,this->_frames[tracks][n].second);
                }
                return nil;
            }
        
            void parse(FILE *fp) {
                
                this->reset();
                
                this->_fp = fp;
                
                if(fp!=NULL){
                    
                    unsigned int buffer;
                    fseeko(fp,4*7,SEEK_SET);
                    fread(&buffer,sizeof(unsigned int),1,fp); // 4*7
                    U64 offset = swapU32(buffer);
                    fread(&buffer,sizeof(unsigned int),1,fp); // 4*8
                        
                    if(swapU32(buffer)==this->atom("mdat")) {
                        
                        fseeko(fp,(4*8)+offset-4,SEEK_SET);
                        fread(&buffer,sizeof(unsigned int),1,fp);
                        int len = swapU32(buffer);
                        fread(&buffer,sizeof(unsigned int),1,fp);
                        
                        while(true) {
                            
                            if(swapU32(buffer)==this->atom("mdat")) {
                                
                                fseeko(fp,(4*8)+offset+len-4,SEEK_SET);
                                fread(&buffer,sizeof(unsigned int),1,fp);
                                len+=swapU32(buffer);
                                fread(&buffer,sizeof(unsigned int),1,fp);
                                
                            }
                            else if(swapU32(buffer)==this->atom("moov")) {
                                
                                unsigned char *moov = new unsigned char[len-4];
                                fread(moov,sizeof(unsigned char),len-4,fp);
                                
                                this->parseTrack(moov,(len-4)-3);
                                
                                delete[] moov;
                                
                                break;
                                
                            }
                            else {
                                
                                break;
                                
                            }
                        }
                    }
                }
            }
        
            Parser(FILE *fp) {
                this->parse(fp);
            }
            
            Parser(NSString *path) {
                FILE *fp = fopen([path UTF8String],"rb");
                this->parse(fp);
            }
        
            ~Parser() {
                
                if(this->_fp) {
                    fclose(this->_fp);
                }
                this->_fp = NULL;
                this->clear();
            }
        
#endif
            
    };

};

                                
