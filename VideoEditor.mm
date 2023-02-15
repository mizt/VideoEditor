#import <Cocoa/Cocoa.h>
#import "MultiTrackQTMovie.h"
#import "MultiTrackQTMovieParser.h"

typedef struct {
    unsigned short width;
    unsigned short height;
    unsigned int frames;
} Info;

NSRegularExpression *regexp(NSString *pattern) {
    return [NSRegularExpression regularExpressionWithPattern:pattern options:NSRegularExpressionCaseInsensitive error:nil];
}

int main(int argc, char *argv[]) {
    @autoreleasepool {
        
        bool load = false;
        
        int srcFrames = 0;
        int dstFrames = 0;
        
        std::vector<MultiTrackQTMovie::TrackInfo> info;
        MultiTrackQTMovie::Recorder *recorder = nullptr;
        MultiTrackQTMovie::Parser *parser = nullptr;
        
        if(argc==2||argc==3) {
            
            NSString *dst = nil;
            
            if(argc==3&&[[NSString stringWithFormat:@"%s",argv[2]] hasSuffix:@".mov"]) {
                dst = [NSString stringWithFormat:@"%s",argv[2]];
            }
            
            NSRegularExpression *number = regexp(@"[0-9]+");
            NSRegularExpression *range = regexp(@"[0-9]+-[0-9]+");
            NSRegularExpression *repeate = regexp(@"[0-9]+x[0-9]+");

            NSString *commands = [NSString stringWithFormat:@"%s",argv[1]];
            NSArray *arr = [commands componentsSeparatedByString:@","];
            for(int n=0; n<arr.count; n++) {
                
                NSString *command = arr[n];
                NSRange mask = NSMakeRange(0,command.length);
                
                if([command hasSuffix:@".mov"]) {
                    if(!parser) {
                        NSError *err = nil;
                        [[NSFileManager defaultManager] attributesOfItemAtPath:command error:&err];
                        
                        if(!err) {
                            parser = new MultiTrackQTMovie::Parser(command);
                            if(parser->type(0)=="jpeg") {
                                srcFrames = parser->length(0);
                                info.push_back({.width=parser->width(0),.height=parser->height(0),.depth=24,.fps=30.,.type=parser->type(0)});
                                load = true;
                                continue;
                            }
                        }
                        
                    }
                    else {
                        NSLog(@".mov is already loaded");
                        break;
                    }
                }
                
                if(load) {
                    
                    NSTextCheckingResult *match;
                    
                    match = [range firstMatchInString:command options:0 range:mask];
                    if(match) {
                        NSArray *tmp = [command componentsSeparatedByString:@"-"];
                        if(tmp.count==2) {
                            int begin = [tmp[0] intValue];
                            int end = [tmp[1] intValue];
                            if(begin<=end) {
                                if(end<srcFrames) {
                                    if(!recorder) recorder = new MultiTrackQTMovie::Recorder(dst,&info);
                                    for(int k=begin; k<=end; k++) {
                                        NSData *data = parser->get(k,0);
                                        recorder->add((unsigned char *)data.bytes,data.length,0);
                                        dstFrames++;
                                    }
                                    continue;
                                }
                            }
                            else {
                                if(begin<srcFrames) {
                                    if(!recorder) recorder = new MultiTrackQTMovie::Recorder(dst,&info);
                                    for(int k=begin; k>=end; k--) {
                                        NSData *data = parser->get(k,0);
                                        recorder->add((unsigned char *)data.bytes,data.length,0);
                                        dstFrames++;
                                    }
                                    continue;
                                }
                            }
                        }
                    }
                    
                    match = [repeate firstMatchInString:command options:0 range:mask];
                    if(match) {
                        NSArray *tmp = [command componentsSeparatedByString:@"x"];
                        if(tmp.count==2) {
                            int frame = [tmp[0] intValue];
                            int times = [tmp[1] intValue];
                            if(frame<srcFrames) {
                                if(!recorder) recorder = new MultiTrackQTMovie::Recorder(dst,&info);
                                for(int k=0; k<times; k++) {
                                    NSData *data = parser->get(frame,0);
                                    recorder->add((unsigned char *)data.bytes,data.length,0);
                                    dstFrames++;
                                }
                                continue;
                            }
                        }
                    }
                    
                    match = [number firstMatchInString:command options:0 range:mask];
                    if(match) {
                        if(command.length==match.range.length) {
                            int frame = [command intValue];
                            if(frame<srcFrames) {
                                if(!recorder) recorder = new MultiTrackQTMovie::Recorder(dst,&info);
                                NSData *data = parser->get(frame,0);
                                recorder->add((unsigned char *)data.bytes,data.length,0);
                                dstFrames++;
                                continue;
                            }
                        }
                    }
                }
                else {
                    NSLog(@".mov is not loaded");
                    break;
                }
                
                NSLog(@"? \"%@\"",command);
            }
        }
        
        NSLog(@"frames = %d",dstFrames);
        
        if(recorder) {
            recorder->save();
            delete recorder;
        }
    }
}
