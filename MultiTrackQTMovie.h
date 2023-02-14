#import <vector>
#import <string>

#import "MultiTrackQTMovieUtils.h"
#import "MultiTrackQTMovieAtomUtils.h"

namespace MultiTrackQTMovie {
    
    class VideoRecorder {
        
        protected:
            
            NSString *_fileName;
            
            bool _isRunning = false;
            bool _isRecorded = false;
            
            NSFileHandle *_handle;
            std::vector<U64> *_frames;
            
            const unsigned int MDAT_LIMIT = (1024*1024*1024)/10; // 0.1 = 100MB
            U64 _mdat_offset = 0;
            NSMutableData *_mdat = nil;
            
            U64 _chunk_offset = 0;
            std::vector<U64> *_chunks;
        
            std::vector<bool> *_keyframes;
            
            std::vector<TrackInfo> *_info;
            
            NSData *_vps = nil;
            NSData *_sps = nil;
            NSData *_pps = nil;
        
            NSString *filename() {
                long unixtime = (CFAbsoluteTimeGetCurrent()+kCFAbsoluteTimeIntervalSince1970)*1000;
                NSString *timeStampString = [NSString stringWithFormat:@"%f",(unixtime/1000.0)];
                NSDate *date = [NSDate dateWithTimeIntervalSince1970:[timeStampString doubleValue]];
                NSDateFormatter *format = [[NSDateFormatter alloc] init];
                [format setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"ja_JP"]];
                [format setDateFormat:@"yyyy_MM_dd_HH_mm_ss_SSS"];
    #if TARGET_OS_OSX
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSMoviesDirectory,NSUserDomainMask,YES);
    #else
                NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES);
    #endif
                NSString *directory = [paths objectAtIndex:0];
                return [NSString stringWithFormat:@"%@/%@",directory,[format stringFromDate:date]];
            }
            
            
        public:
            
            VideoRecorder(NSString *fileName, std::vector<TrackInfo> *info) {
                
                if(fileName) this->_fileName = fileName;
                else this->_fileName = [NSString stringWithFormat:@"%@.mov",this->filename()];
                this->_info = info;
                this->_frames = new std::vector<U64>[this->_info->size()];
                this->_chunks = new std::vector<U64>[this->_info->size()];
                this->_keyframes = new std::vector<bool>[this->_info->size()];
            }
        
            bool isVPS() { return this->_vps?true:false; }
            void setVPS(NSData *data) { this->_vps = [[NSData alloc] initWithData:data]; }
            
            bool isSPS() { return this->_sps?true:false; }
            void setSPS(NSData *data) { this->_sps = [[NSData alloc] initWithData:data]; }
            
            bool isPPS() { return this->_pps?true:false; }
            void setPPS(NSData *data) { this->_pps = [[NSData alloc] initWithData:data]; }
        
            ~VideoRecorder() {
                for(int k=0; k<this->_info->size(); k++) {
                    this->_frames[k].clear();
                    this->_chunks[k].clear();
                    this->_keyframes[k].clear();
                }
                delete[] this->_frames;
                delete[] this->_chunks;
                delete[] this->_keyframes;
            }
    };
    
    class Recorder : public VideoRecorder {
        
        protected:
            
            void inialized() {
                
                MultiTrackQTMovie::ftyp *ftyp = new MultiTrackQTMovie::ftyp();
                
                unsigned long length = 0;
                
        #ifdef USE_VECTOR
                std::vector<unsigned char> *bin = ftyp->get();
                [[[NSData alloc] initWithBytes:bin->data() length:bin->size()] writeToFile:this->_fileName options:NSDataWritingAtomic error:nil];
                length = bin->size();
        #else
                NSData *bin = ftyp->get();
                [bin writeToFile:this->_fileName options:NSDataWritingAtomic error:nil];
                length = [bin length];

        #endif
                
                this->_handle = [NSFileHandle fileHandleForWritingAtPath:this->_fileName];
                [this->_handle seekToEndOfFile];
                
                this->_mdat_offset = length;
                this->_chunk_offset = length;
            }
            
        public:
            
            Recorder(NSString *fileName,std::vector<TrackInfo> *info) : VideoRecorder(fileName,info) {
                this->inialized();
            }
            
            Recorder(std::vector<TrackInfo> *info) : VideoRecorder(nil,info) {
                this->inialized();
            }
            
            Recorder *add(unsigned char *data, unsigned long length, unsigned int trackid, bool keyframe=true, bool padding=false) {
                
                if(!this->_isRecorded) {
                    
                    if(this->_isRunning==false) this->_isRunning = true;
                    
                    if(trackid>=0&&trackid<this->_info->size()) {
                        
                        U64 size = length;
                        U64 diff = 0;
                        
                        if(padding&&(length%4!=0)) {
                            diff=(((length+3)>>2)<<2)-length;
                            size+=diff;
                        }
                        
                        if(this->_mdat&&([this->_mdat length]+size)>=MDAT_LIMIT) {
                            
                            [this->_handle seekToEndOfFile];
                            [this->_handle writeData:this->_mdat];
                            
                            [this->_handle seekToFileOffset:this->_mdat_offset];
                            NSData *tmp = [[NSData alloc] initWithBytes:new unsigned int[1]{swapU32((unsigned int)[this->_mdat length])} length:4];
                            [this->_handle writeData:tmp];
                            [this->_handle seekToEndOfFile];
                            
                            this->_mdat_offset+=[this->_mdat length];
                            this->_mdat = nil;
                        }
                        
                        if(this->_mdat==nil) {
                            
                            this->_mdat = [[NSMutableData alloc] init];
                            [this->_mdat appendBytes:new unsigned int[1]{swapU32(0)} length:4];
                            [this->_mdat appendBytes:new unsigned int[1]{swapU32(0x6D646174) } length:4]; // 'mdat' 
                            this->_chunk_offset+=8;
                        }
                        
                        if(((*this->_info)[trackid].type)=="avc1") {
                            this->_frames[trackid].push_back(length);
                        }
                        else if(((*this->_info)[trackid].type)=="hvc1") {
                            this->_frames[trackid].push_back(length);
                        }
                        else {
                            this->_frames[trackid].push_back(size);
                        }
                        
                        this->_chunks[trackid].push_back(this->_chunk_offset);
                        
                        this->_keyframes[trackid].push_back(keyframe);
                        
                        [this->_mdat appendBytes:data length:length];
                        if(diff) {
                            [this->_mdat appendBytes:new unsigned char[diff]{0} length:diff];
                        }
                        
                        this->_chunk_offset+=size;
                    }
                }
                
                return this;
            }
            
            void save() {
                
                if(this->_isRunning&&!this->_isRecorded) {
                    
                    bool avc1 = false;
                    bool hvc1 = false;
                    
                    for(int n=0; n<this->_info->size(); n++) {
                        
                        if((*this->_info)[n].type=="avc1") {
                            avc1 = true;
                        }
                        else if((*this->_info)[n].type=="hvc1") {
                            hvc1 = true;
                        }
                        
                        if(hvc1) {
                            if(!(this->_vps&&this->_sps&&this->_pps)) {
                                this->_isRunning = false;
                                this->_isRecorded = true;
                                return;
                            }
                        }
                        else if(avc1) {
                            if(!(this->_sps&&this->_pps)) {
                                this->_isRunning = false;
                                this->_isRecorded = true;
                                return;
                            }
                        }
                    }
                    
                    if(this->_mdat) {
                        [this->_handle seekToEndOfFile];
                        [this->_handle writeData:this->_mdat];
                        [this->_handle seekToFileOffset:this->_mdat_offset];
                        NSData *tmp = [[NSData alloc] initWithBytes:new unsigned int[1]{0} length:4];
                        *((unsigned int *)[tmp bytes]) = swapU32((unsigned int)[this->_mdat length]);
                        [this->_handle writeData:tmp];
                        [this->_handle seekToEndOfFile];
                        this->_mdat = nil;
                    }
                    
                    MultiTrackQTMovie::moov *moov = nullptr;
                    
                    if(hvc1) {
                                                
                        moov = new MultiTrackQTMovie::moov(this->_info,this->_frames,this->_chunks,this->_keyframes,(unsigned char *)[this->_sps bytes],[this->_sps length],(unsigned char *)[this->_pps bytes],[this->_pps length],(unsigned char *)[this->_vps bytes],[this->_vps length]);
                    }
                    else if(avc1) {
                        moov = new MultiTrackQTMovie::moov(this->_info,this->_frames,this->_chunks,this->_keyframes,(unsigned char *)[this->_sps bytes],[this->_sps length],(unsigned char *)[this->_pps bytes],[this->_pps length],nullptr,0);
                    }
                    else {
                        moov = new MultiTrackQTMovie::moov(this->_info,this->_frames,this->_chunks,this->_keyframes,nullptr,0,nullptr,0,nullptr,0);
                    }
                  

                    [this->_handle seekToEndOfFile];
                    
#ifdef USE_VECTOR
                    std::vector<unsigned char> *bin = moov->get();
                    [this->_handle writeData:[[NSData alloc] initWithBytes:bin->data() length:bin->size()]];

#else
                    [this->_handle writeData:moov->get()];

#endif
                    delete moov;
                    
                    this->_isRunning = false;
                    this->_isRecorded = true;
                }
            }
            
            ~Recorder() {
                
            }
    };

};

        
        