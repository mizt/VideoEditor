#import <Cocoa/Cocoa.h>
#import <vector>
#import <string>

typedef unsigned long u64;
typedef long s64;

namespace QTM {
    
    namespace Event {
        NSString *SAVE_COMPLETE = @"SAVE_COMPLETE";
        NSString *RESET = @"RESET";
    }

    namespace EventEmitter {
    
        class Observer {
            
            private:
            
                NSString *_type = nil;
                id _observer = nil;
            
            public:
            
                Observer(NSString *type, id observer) {
                    this->_type = type;
                    this->_observer = observer;
                }
            
                NSString *type() { return this->_type; }
                id observer() { return this->_observer; }
            
                ~Observer() {
                    this->_observer = nil;
                }
        };
    
        std::vector<Observer *> events;
    
        void on(NSString *type, void (^callback)(NSNotification *)) {
            id observer = nil;
            long len = events.size();
            while(len--) {
                if(events[len]->type()&&[events[len]->type() compare:type]==NSOrderedSame) {
                    observer = events[len]->observer();
                    break;
                }
            }
            if(observer==nil) {
                observer = [[NSNotificationCenter defaultCenter]
                    addObserverForName:type
                    object:nil
                    queue:[NSOperationQueue mainQueue]
                    usingBlock:callback
                ];
                events.push_back(new Observer(type,observer));
            }
            else {
                NSLog(@"%@ is already registered",type);
            }
        }
    
        void off(NSString *type) {
            id observer = nil;
            long len = events.size();
            while(len--) {
                if(events[len]->type()&&[events[len]->type() compare:type]==NSOrderedSame) {
                    observer = events[len]->observer();
                    events.erase(events.begin()+len);
                    break;
                }
            }
            if(observer) {
                [[NSNotificationCenter defaultCenter] removeObserver:(id)observer];
                observer = nil;
            }
        }
    
        void emit(NSString *event) {
            [[NSNotificationCenter defaultCenter] postNotificationName:event object:nil];
        }
    };

    typedef struct _TrackInfo {
        unsigned short width = 1920;
        unsigned short height = 1080;
        unsigned short depth = 24.0;
        double fps = 30.0;
        std::string codec = "jpeg";
        std::string trak = "trak";
    } TrackInfo;
    
    u64 toU64(unsigned char *p) {
        return *((u64 *)p);
    }
    
    unsigned int toU32(unsigned char *p) {
        return *((unsigned int *)p);
    }
    
    unsigned short toU16(unsigned char *p) {
        return *((unsigned short *)p);
    }
    
    u64 swapU64(u64 n) {
        return ((n>>56)&0xFF)|(((n>>48)&0xFF)<<8)|(((n>>40)&0xFF)<<16)|(((n>>32)&0xFF)<<24)|(((n>>24)&0xFF)<<32)|(((n>>16)&0xFF)<<40)|(((n>>8)&0xFF)<<48)|((n&0xFF)<<56);
    }
    
    unsigned int swapU32(unsigned int n) {
        return ((n>>24)&0xFF)|(((n>>16)&0xFF)<<8)|(((n>>8)&0xFF)<<16)|((n&0xFF)<<24);
    }
    
    unsigned short swapU16(unsigned short n) {
        return ((n>>8)&0xFF)|((n&0xFF)<<8);
    }
    
    u64 U64(unsigned char *p) {
        return swapU64(toU64(p));
    }
    
    unsigned int U32(unsigned char *p) {
        return swapU32(toU32(p));
    }
    
    unsigned short U16(unsigned char *p) {
        return swapU16(toU16(p));
    }
    
    class AtomUtils {
            
        protected:
            
            void reset() {
                this->bin.clear();
            }
            
            std::vector<unsigned char> bin;
            unsigned int CreationTime = 3061152000;
            
            void setU64(u64 value) {
                bin.push_back((value>>56)&0xFF);
                bin.push_back((value>>48)&0xFF);
                bin.push_back((value>>40)&0xFF);
                bin.push_back((value>>32)&0xFF);
                bin.push_back((value>>24)&0xFF);
                bin.push_back((value>>16)&0xFF);
                bin.push_back((value>>8)&0xFF);
                bin.push_back((value)&0xFF);
            }
            
            void setU32(unsigned int value) {
                bin.push_back((value>>24)&0xFF);
                bin.push_back((value>>16)&0xFF);
                bin.push_back((value>>8)&0xFF);
                bin.push_back((value)&0xFF);
            }
            
            void setU16(unsigned short value) {
                bin.push_back((value>>8)&0xFF);
                bin.push_back((value)&0xFF);
            }
            
            void setU8(unsigned char value) {
                bin.push_back(value);
            }
            
            void setZero(u64 length) {
                for(u64 n=0; n<length; n++) bin.push_back(0);
            }
    };

    typedef std::pair<std::string,u64> Atom;
    
    class SampleData {
        
        private:
            
            unsigned long _offset = 0;
            unsigned long _length = 0;
            unsigned int _chank = 1;
            
            std::vector<bool> _keyframes;
            std::vector<unsigned long> _offsets;
            std::vector<unsigned int> _lengths;
            std::vector<unsigned int> _chunks;
            
        public:
            
            std::vector<bool> *keyframes() { return &this->_keyframes; }
            std::vector<unsigned long> *offsets() { return &this->_offsets; }
            std::vector<unsigned int> *lengths() { return &this->_lengths; };
            std::vector<unsigned int> *chunks() { return &this->_chunks; };
            
            void writeData(NSFileHandle *handle, unsigned char *bytes, unsigned int length, bool keyframe) {
                this->_offsets.push_back(this->_offset+this->_length);
                this->_lengths.push_back(length);
                this->_chunks.push_back(this->_chank++);
                [handle writeData:[[NSData alloc] initWithBytes:bytes length:length]];
                [handle seekToEndOfFile];
                this->_length+=length;
                this->_keyframes.push_back(keyframe);
            }
            
            void writeSize(NSFileHandle *handle) {
                [handle seekToFileOffset:this->_offset+8];
                [handle writeData:[[NSData alloc] initWithBytes:new unsigned char[8]{
                    (unsigned char)((this->_length>>56)&0xFF),
                    (unsigned char)((this->_length>>48)&0xFF),
                    (unsigned char)((this->_length>>40)&0xFF),
                    (unsigned char)((this->_length>>32)&0xFF),
                    (unsigned char)((this->_length>>24)&0xFF),
                    (unsigned char)((this->_length>>16)&0xFF),
                    (unsigned char)((this->_length>>8)&0xFF),
                    (unsigned char)(this->_length&0xFF)} length:8]];
                [handle seekToEndOfFile];
            }
            
            SampleData(NSFileHandle *handle, unsigned long offset) {
                
                this->_offset = offset;
                [handle writeData:[[NSData alloc] initWithBytes:new unsigned char[16]{
                    0,0,0,1,
                    'm','d','a','t',
                    0,0,0,0,
                    0,0,0,0
                } length:16]];
                [handle seekToEndOfFile];
                this->_length+=16;
            }
            
            ~SampleData() {
                this->_keyframes.clear();
                this->_offsets.clear();
                this->_lengths.clear();
                this->_chunks.clear();
            }
    };

    // https://developer.apple.com/library/archive/documentation/QuickTime/QTFF/QTFFChap2/qtff2.html    
    class ftyp : public AtomUtils {
        
        public:
            
            std::vector<unsigned char> *get() {
                return &this->bin;
            }
            
            ftyp() {
                this->reset();
                this->setU32(0x00000014); // 20
                this->setU32(0x66747970); // 'ftyp'
                this->setU32(0x71742020); // 'qt  '
                this->setU32(0x00000000);
                this->setU32(0x71742020); // 'qt  '
                this->setU32(0x00000008); // 8
                this->setU32(0x77696465); // 'wide'
            }
            
            ~ftyp() {
                this->reset();
            }
    };
    
    class moov : public AtomUtils {
            
        private:
            
            const bool is64 = true;
            const int Transfer = 1;
            unsigned int ModificationTime = CreationTime;
            unsigned int TimeScale = 30000;
            unsigned short Language = 21956;
            
            Atom initAtom(std::string str, unsigned int size=0, bool dump=false) {
                u64 pos = this->bin.size();
                this->setU32(size);
                this->setString(str);
                return std::make_pair(str,pos);
            }
            
            void setAtomSize(u64 pos) {
                unsigned int size = (unsigned int)(this->bin.size()-pos);
                this->bin[pos+0] = (size>>24)&0xFF;
                this->bin[pos+1] = (size>>16)&0xFF;
                this->bin[pos+2] = (size>>8)&0xFF;
                this->bin[pos+3] = size&0xFF;
            }
            
            void setString(std::string str, unsigned int length=4) {
                unsigned char *s = (unsigned char *)str.c_str();
                for(s64 n=0; n<str.length(); n++) this->setU8(s[n]);
                s64 len = length-str.length();
                if(len>=1) {
                    while(len--) this->setU8(0);
                }
            }
            
            void setPascalString(std::string str) {
                //assert(str.length()<255);
                this->setU8(str.size());
                this->setString(str,(unsigned int)str.size());
            }
            
            void setCompressorName(std::string str) {
                //assert(str.length()<32);
                this->setPascalString(str);
                int len = 31-(int)str.length();
                while(len--) this->setU8(0);
            }
            
            void setMatrix() {
                // All values in the matrix are 32-bit fixed-point numbers divided as 16.16, except for the {u, v, w} column, which contains 32-bit fixed-point numbers divided as 2.30.
                this->setU32(0x00010000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x00010000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x00000000);
                this->setU32(0x40000000);
            }
            
            void setVersionWithFlag(unsigned char version=0, unsigned int flag=0) {
                this->setU8(version);
                this->setU8((flag>>16)&0xFF);
                this->setU8((flag>>8)&0xFF);
                this->setU8(flag&0xFF);
            }
            
        public:
            
            std::vector<unsigned char> *get() {
                return &this->bin;
            }
            
            moov(TrackInfo *info, SampleData *mdat) {
                this->reset();
                Atom moov = this->initAtom("moov");
                unsigned int track = 1;
                unsigned int maxDuration = 0;
                unsigned int TotalFrames = (unsigned int)(mdat->keyframes()->size());
                float FPS = (float)((*info).fps);
                unsigned int Duration = (unsigned int)(TotalFrames*(this->TimeScale/FPS));
                if(Duration>maxDuration) maxDuration = Duration;
                unsigned int SampleDelta = (unsigned int)(this->TimeScale/FPS);
                Atom mvhd = this->initAtom("mvhd");
                this->setVersionWithFlag();
                this->setU32(this->CreationTime);
                this->setU32(this->ModificationTime);
                this->setU32(this->TimeScale);
                this->setU32(maxDuration);
                this->setU32(1<<16); // Preferred rate
                this->setU16(0); // Preferred volume
                this->setZero(10); // Reserved
                this->setMatrix();
                this->setU32(0); // Preview time
                this->setU32(0); // Preview duration
                this->setU32(0); // Poster time
                this->setU32(0); // Selection time
                this->setU32(0); // Selection duration
                this->setU32(0); // Current time
                this->setU32(1); // Next track ID
                this->setAtomSize(mvhd.second);
                Atom trak = this->initAtom((*info).trak);
                Atom tkhd = this->initAtom("tkhd");
                this->setVersionWithFlag(0,0xF);
                this->setU32(CreationTime);
                this->setU32(ModificationTime);
                this->setU32(track); // Track id
                this->setZero(4); // Reserved
                this->setU32(Duration);
                this->setZero(8); // Reserved
                this->setU16(0); // Layer
                this->setU16(0); // Alternate group
                this->setU16(0); // Volume
                this->setU16(0); // Reserved
                this->setMatrix();
                this->setU32(((*info).width)<<16);
                this->setU32(((*info).height)<<16);
                this->setAtomSize(tkhd.second);
                Atom tapt = this->initAtom("tapt");
                this->initAtom("clef",20);
                this->setVersionWithFlag();
                this->setU32(((*info).width)<<16);
                this->setU32(((*info).height)<<16);
                this->initAtom("prof",20);
                this->setVersionWithFlag();
                this->setU32(((*info).width)<<16);
                this->setU32(((*info).height)<<16);
                this->initAtom("enof",20);
                this->setVersionWithFlag();
                this->setU32(((*info).width)<<16);
                this->setU32(((*info).height)<<16);
                this->setAtomSize(tapt.second);
                Atom edts = this->initAtom("edts");
                Atom elst = this->initAtom("elst");
                this->setVersionWithFlag();
                this->setU32(1); // Number of entries
                this->setU32(Duration);
                this->setU32(0); // Media time
                this->setU32(1<<16); // Media rate
                this->setAtomSize(elst.second);
                this->setAtomSize(edts.second);
                Atom mdia = this->initAtom("mdia");
                Atom mdhd = this->initAtom("mdhd");
                this->setVersionWithFlag();
                this->setU32(CreationTime);
                this->setU32(ModificationTime);
                this->setU32(TimeScale);
                this->setU32(Duration);
                this->setU16(Language);
                this->setU16(0); // Quality
                this->setAtomSize(mdhd.second);
                Atom hdlr = this->initAtom("hdlr");
                this->setVersionWithFlag();
                this->setString("mhlr");
                this->setString("vide");
                this->setU32(0); // Reserved
                this->setZero(8); // Reserved
                this->setPascalString("Video");
                this->setAtomSize(hdlr.second);
                Atom minf = this->initAtom("minf");
                Atom vmhd = this->initAtom("vmhd");
                this->setVersionWithFlag(0,1);
                this->setU16(0); // Graphics mode 64? Copy = 0, 64 = Dither Copy
                this->setU16(32768); // Opcolor
                this->setU16(32768); // Opcolor
                this->setU16(32768); // Opcolor
                this->setAtomSize(vmhd.second);
                hdlr = this->initAtom("hdlr");
                this->setVersionWithFlag();
                this->setString("dhlr");
                this->setString("alis");
                this->setU32(0); // Reserved 0
                this->setZero(8); // Reserved
                this->setPascalString("Handler");
                this->setAtomSize(hdlr.second);
                Atom dinf = this->initAtom("dinf");
                Atom dref = this->initAtom("dref");
                this->setVersionWithFlag();
                this->setU32(1); // Number of entries
                this->initAtom("alis",12);
                this->setVersionWithFlag(0,1);
                this->setAtomSize(dref.second);
                this->setAtomSize(dinf.second);
                Atom stbl = this->initAtom("stbl");
                Atom stsd = this->initAtom("stsd");
                this->setVersionWithFlag();
                this->setU32(1); // Number of entries
                Atom table = initAtom(((*info).codec));
                this->setZero(6); // Reserved
                this->setU16(1); // Data reference index
                this->setU16(0); // Version
                this->setU16(0); // Revision level
                this->setU32(0); // Vendor
                this->setU32(0); // Temporal quality
                this->setU32(1024); // Spatial quality
                this->setU16(((*info).width));
                this->setU16(((*info).height));
                this->setU32(72<<16); // Horizontal resolution
                this->setU32(72<<16); // Vertical resolution
                this->setZero(4);
                this->setU16(1); // Frame count
                this->setCompressorName("'"+((*info).codec)+"'"); // 32
                this->setU16(((*info).depth)); // Depth
                this->setU16(0xFFFF); // Color table ID
                this->initAtom("colr",18);
                this->setString("nclc");
                this->setU16(1); // Primaries index
                this->setU16(Transfer); // Transfer function index
                this->setU16(1); // Matrix index
                if(this->Transfer==2) {
                    this->initAtom("gama",12);
                    this->setU32(144179); // 2.2 = 144179, 1.96 = 128512
                }
                this->setU32(0); // Some sample descriptions terminate with four zero bytes that are not otherwise indicated.
                this->setAtomSize(table.second);
                this->setAtomSize(stsd.second);
                this->initAtom("stts",24);
                this->setVersionWithFlag();
                this->setU32(1); // Number of entries
                this->setU32(TotalFrames);
                this->setU32(SampleDelta);
                Atom stsc = this->initAtom("stsc",28);
                this->setVersionWithFlag();
                this->setU32(1); // Number of entries
                std::vector<unsigned int> *chunks = mdat->chunks();
                for(int k=0; k<chunks->size(); k++) {
                    this->setU32((*chunks)[k]); // First chunk
                    this->setU32(1); // Samples per chunk
                    this->setU32(1+k); // Sample description ID
                }
                this->setAtomSize(stsc.second);
                Atom stsz = this->initAtom("stsz",20);
                this->setVersionWithFlag();
                this->setU32(0); // If this field is set to 0, then the samples have different sizes, and those sizes are stored in the sample size table.
                this->setU32(TotalFrames); // Number of entries
                this->setAtomSize(stsz.second);
                std::vector<unsigned int> *lengths = mdat->lengths();
                for(int k=0; k<lengths->size(); k++) {
                    this->setU32((*lengths)[k]);
                }
                this->setAtomSize(stsz.second);
                if(this->is64) {
                    Atom co64 = this->initAtom("co64");
                    this->setVersionWithFlag();
                    this->setU32(TotalFrames);
                    std::vector<unsigned long> *offsets = mdat->offsets();
                    for(int k=0; k<offsets->size(); k++) {
                        this->setU64((*offsets)[k]);
                    }
                    this->setAtomSize(co64.second);
                }
                else {
                    Atom stco = this->initAtom("stco");
                    this->setVersionWithFlag();
                    this->setU32(TotalFrames);
                    std::vector<unsigned long> *offsets = mdat->offsets();
                    for(int k=0; k<offsets->size(); k++) {
                        this->setU32((unsigned int)((*offsets)[k]));
                    }
                    this->setAtomSize(stco.second);
                }
                this->setAtomSize(minf.second);
                this->setAtomSize(stbl.second);
                this->setAtomSize(mdia.second);
                this->setAtomSize(trak.second);
                this->setAtomSize(moov.second);
            }
            
            ~moov() {
                this->reset();
            }
    };

    class Buffer {
        
        private:
            
            bool _keyframe = false;
            unsigned char *_bytes = nullptr;
            unsigned int _length = 0;
        
        public:
        
            Buffer(unsigned char *bytes, unsigned int length, bool keyframe) {
                this->_bytes = new unsigned char[length];
                this->_keyframe = keyframe;
                this->_length = length;
                memcpy(this->_bytes,bytes,this->_length);
            }
        
            bool keyframe() { return this->_keyframe; }
            unsigned int length() { return this->_length; }
            unsigned char *bytes() { return this->_bytes; }
        
            ~Buffer() {
                
                if(this->_bytes) {
                    delete[] this->_bytes;
                    this->_bytes = nullptr;
                }
            }
    };
    
    class VideoRecorder {
        
        protected:
            
            NSString *_path;
            bool _isRunning = false;
            bool _isSaving = false;
            NSFileHandle *_handle;
            std::vector<u64> _frames;
            TrackInfo *_info;
            
            NSString *filePath(NSString *directory, NSString *extension) {
                long unixtime = (CFAbsoluteTimeGetCurrent()+kCFAbsoluteTimeIntervalSince1970)*1000;
                NSString *timeStampString = [NSString stringWithFormat:@"%f",(unixtime/1000.0)];
                NSDate *date = [NSDate dateWithTimeIntervalSince1970:[timeStampString doubleValue]];
                NSDateFormatter *format = [[NSDateFormatter alloc] init];
                [format setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"ja_JP"]];
                [format setDateFormat:@"yyyy-MM-dd'T'HH.mm.ss.SSS"];
                return [NSString stringWithFormat:@"%@/%@.%@",directory,[format stringFromDate:date],extension];
            }
            
        public:
            
            VideoRecorder(NSString *fileName, TrackInfo *info) {
                
                if(fileName) this->_path = fileName;
                else {
                    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSMoviesDirectory,NSUserDomainMask,YES);
                    this->_path = this->filePath([paths objectAtIndex:0],@"mov");
                }
                this->_info = info;
            }
        
            NSString *path() { return this->_path; }
            
            ~VideoRecorder() {}
    };


    class Recorder : public VideoRecorder {
        
        protected:
                
            dispatch_source_t _timer = nullptr;
            std::vector<Buffer *> _queue;
            SampleData *_mdat = nullptr;
                    
            void inialized() {
                QTM::ftyp *ftyp = new QTM::ftyp();
                unsigned long length = 0;
                std::vector<unsigned char> *bin = ftyp->get();
                [[[NSData alloc] initWithBytes:bin->data() length:bin->size()] writeToFile:this->_path options:NSDataWritingAtomic error:nil];
                length = bin->size();
                this->_handle = [NSFileHandle fileHandleForWritingAtPath:this->_path];
                [this->_handle seekToEndOfFile];
                this->_mdat = new SampleData(this->_handle,length);
            }
        
            void cleanup() {
                if(this->_timer){
                    dispatch_source_cancel(this->_timer);
                    this->_timer = nullptr;
                }
            }
        
            void setup() {
                this->_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER,0,0,dispatch_queue_create("ENTER_FRAME",0));
                dispatch_source_set_timer(this->_timer,DISPATCH_TIME_NOW,NSEC_PER_SEC/300.0,0);
                dispatch_source_set_event_handler(this->_timer,^{
                    
                    if(!(this->_isSaving||this->_isRunning)) return;
                    
                    unsigned long size = this->_queue.size();
                    bool finished = (size==0)?true:false;
                        
                    for(int k=0; k<size; k++) {
                        if(this->_queue[k]) {
                            bool keyframe = this->_queue[k]->keyframe();
                            unsigned int length = this->_queue[k]->length();
                            unsigned char *bytes = this->_queue[k]->bytes();
                            this->_mdat->writeData(this->_handle,bytes,length,keyframe);
                            delete this->_queue[k];
                            this->_queue[k] = nullptr;
                            break;
                        }
                        if(k==size-1) finished = true;
                    }
                    
                    if(finished&&this->_isSaving==true&&this->_isRunning==false) {
                        if(this->_timer){
                            dispatch_source_cancel(this->_timer);
                            this->_timer = nullptr;
                        }
                        this->_mdat->writeSize(this->_handle);
                        QTM::moov *moov = new QTM::moov(this->_info,this->_mdat);
                        [this->_handle seekToEndOfFile];
                        std::vector<unsigned char> *bin = moov->get();
                        [this->_handle writeData:[[NSData alloc] initWithBytes:bin->data() length:bin->size()]];
                        delete moov;
                        this->_isSaving = false;
                        EventEmitter::emit(QTM::Event::SAVE_COMPLETE);
                    }
                });
                if(this->_timer) dispatch_resume(this->_timer);
            }
        
        public:
            
            Recorder(NSString *fileName,TrackInfo *info) : VideoRecorder(fileName,info) {
                this->inialized();
                this->setup();
            }
            
            Recorder(TrackInfo *info) : VideoRecorder(nil,info) {
                this->inialized();
                this->setup();
            }
            
            Recorder *add(unsigned char *data, unsigned int length, bool keyframe=true) {
                if(!this->_isSaving) {
                    if(this->_isRunning==false) this->_isRunning = true;
                    this->_queue.push_back(new Buffer(data,length,keyframe));
                }
                return this;
            }
            
            void save() {
                if(this->_isRunning&&!this->_isSaving) {
                    this->_isSaving = true;
                    this->_isRunning = false;
                }
            }
            
            ~Recorder() {}
    };
    
    class Parser {
            
        private:
            TrackInfo *_info;
            unsigned int _totalFrames;
            std::pair<u64,unsigned int> *_frames;
            NSFileHandle *_handle;
            
            void reset() {
                this->_info = nullptr;
                this->_totalFrames = 0;
                this->_frames = nullptr;
            }
            
            void clear() {
                delete[] this->_frames;
                this->reset();
            }
            
        public:
            
            unsigned int length() {
                return this->_totalFrames;
            };
            
            unsigned short width() {
                return (this->_info)?(*this->_info).width:0;
            };
            
            unsigned short height() {
                return (this->_info)?(*this->_info).height:0;
            };
            
            unsigned int depth() {
                return (this->_info)?(*this->_info).depth:0;
            };
            
            std::string codec() {
                return (this->_info)?(*this->_info).codec:"";
            };
            
            unsigned int FPS() {
                return (this->_info)?(*this->_info).fps:0;
            };
            
            unsigned int atom(std::string str) {
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
            
            void parseTrack(unsigned char *moov, u64 len) {
                unsigned int trak;
                for(int k=0; k<len; k++) {
                    if(U32(moov+k)==this->atom("trak")) {
                        trak = k;
                        break;
                    }
                }
                unsigned int TimeScale = 0;
                for(int k=0; k<len-(4*4); k++) {
                    if(U32(moov+k)==this->atom("mvhd")) {
                        TimeScale = U32(moov+k+(4*4));
                        break;
                    }
                }
                TrackInfo *info = nullptr;
                unsigned int begin = trak;
                unsigned int end = begin + U32(moov+trak-4)-4;
                unsigned int info_offset[4];
                info_offset[0] = 4*4; 
                info_offset[1] = info_offset[0]+4+6+2+2+2+4+4+4; 
                info_offset[2] = info_offset[1]+2; 
                info_offset[3] = info_offset[2]+4+4+4+2+32+2; 
                for(int k=begin; k<end-info_offset[3]; k++) {
                    if(U32(moov+k)==this->atom("stsd")) {
                        info = new TrackInfo;
                        info->codec = atom(U32(moov+k+info_offset[0]));
                        info->width  = U16(moov+k+info_offset[1]);
                        info->height = U16(moov+k+info_offset[2]);
                        info->depth  = U16(moov+k+info_offset[3]);
                        break;
                    }
                }
                if(info) {
                    double fps = 0;
                    for(int k=begin; k<end-(4*4); k++) {
                        if(U32(moov+k)==atom("stts")) {
                            fps = TimeScale/(double)(U32(moov+k+(4*4)));
                            break;
                        }
                    }
                    if(fps>0) {
                        info->fps = fps;
                        this->_info = info;
                        unsigned int totalFrames = 0;
                        std::pair<u64,unsigned int> *frames = nullptr;
                        for(int k=begin; k<end-((4*3)+4); k++) {
                            if(U32(moov+k)==atom("stsz")) {
                                k+=(4*3);
                                totalFrames = U32(moov+k);
                                if(frames) delete[] frames;
                                frames = new std::pair<u64,unsigned int>[totalFrames];
                                for(int f=0; f<totalFrames; f++) {
                                    k+=4; // 32
                                    unsigned int size = U32(moov+k);
                                    frames[f] = std::make_pair(0,size);
                                }
                                break;
                            }
                        }
                        for(int k=begin; k<end-((4*2)+4); k++) {
                            if(U32(moov+k)==atom("stco")) {
                                k+=(4*2);
                                if(totalFrames==U32(moov+k)) {
                                    k+=4;
                                    for(int f=0; f<totalFrames; f++) {
                                        frames[f].first = U32(moov+k);
                                        k+=4; // 32
                                    }
                                    break;
                                }
                            }
                            else if(U32(moov+k)==atom("co64")) {
                                k+=(4*2);
                                if(totalFrames==U32(moov+k)) {
                                    k+=4;
                                    for(int f=0; f<totalFrames; f++) {
                                        frames[f].first = U64(moov+k);
                                        k+=8; // 64
                                    }
                                    break;
                                }
                            }
                        }
                        this->_totalFrames = totalFrames;
                        this->_frames = frames;
                    }
                }
                if(this->_info==nullptr) {
                    this->clear();
                }
            }
            
            NSData *get(u64 n, unsigned int tracks) {
                if(n<this->_totalFrames) {
                    [this->_handle seekToOffset:this->_frames[n].first error:nil];
                    return [this->_handle readDataUpToLength:this->_frames[n].second error:nil];
                }
                return nil;
            }
            
            Parser(NSString *path) {
                if([[NSFileManager defaultManager] fileExistsAtPath:path]) {
                    this->_handle = [NSFileHandle fileHandleForReadingAtPath:path];
                    unsigned long offset = 0;
                    [this->_handle seekToOffset:0 error:nil];
                    NSData *data = [this->_handle readDataUpToLength:4 error:nil];
                    offset+=U32((unsigned char *)(data.bytes));
                    [this->_handle seekToOffset:offset error:nil];
                    data = [this->_handle readDataUpToLength:4 error:nil];
                    offset+=U32((unsigned char *)(data.bytes));
                    offset+=4;
                    [this->_handle seekToOffset:offset error:nil];
                    data = [this->_handle readDataUpToLength:4 error:nil];
                    unsigned long mdat = 0;
                    if(U32((unsigned char *)(data.bytes))==0x6D646174) { // mdat
                        [this->_handle seekToOffset:offset-4 error:nil];
                        data = [this->_handle readDataUpToLength:4 error:nil];
                        if(U32((unsigned char *)(data.bytes))==1) {
                            [this->_handle seekToOffset:offset+4 error:nil];
                            data = [this->_handle readDataUpToLength:8 error:nil];
                            offset+=U64((unsigned char *)(data.bytes));
                        }
                        else {
                            offset+=U32((unsigned char *)(data.bytes));
                        }
                        [this->_handle seekToOffset:offset error:nil];
                        data = [this->_handle readDataUpToLength:4 error:nil];
                        if(U32((unsigned char *)(data.bytes))==0x6D6F6F76) { // moov
                            [this->_handle seekToOffset:offset-4 error:nil];
                            data = [this->_handle readDataUpToLength:4 error:nil];
                            unsigned long length = U32((unsigned char *)(data.bytes));
                            [this->_handle seekToOffset:offset+4 error:nil];
                            this->parseTrack((unsigned char *)[this->_handle readDataUpToLength:length-8 error:nil].bytes,length-8);
                        }
                    }
                    else if(U32((unsigned char *)(data.bytes))==0x6D6F6F76) { // moov
                        [this->_handle seekToOffset:offset-4 error:nil];
                        data = [this->_handle readDataUpToLength:4 error:nil];
                        unsigned long length = U32((unsigned char *)(data.bytes));
                        [this->_handle seekToOffset:offset+4 error:nil];
                        this->parseTrack((unsigned char *)[this->_handle readDataUpToLength:length-8 error:nil].bytes,length-8);
                    }
                }
            }
            
            ~Parser() {
                if(this->_handle) [this->_handle closeFile];
            }
    };
};
