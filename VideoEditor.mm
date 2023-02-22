#import <Cocoa/Cocoa.h>
#import "MultiTrackQTMovie.h"
#import "MultiTrackQTMovieParser.h"

namespace VideoEditor {
    
    static const NSArray *supportedCodecs = @[@"jpeg",@"png "];
    
    NSRegularExpression *regexp(NSString *pattern) {
        return [NSRegularExpression regularExpressionWithPattern:pattern options:NSRegularExpressionCaseInsensitive error:nil];
    }
    
    NSData *file(NSString *path) {
        return [[NSFileManager defaultManager] contentsAtPath:path];
    }
    
    NSRegularExpression *number = regexp(@"[0-9]+");
    NSRegularExpression *range = regexp(@"[0-9]+-[0-9]+");
    NSRegularExpression *repeate = regexp(@"[0-9]+x[0-9]+");
    
    unsigned int srcFrames = 0;
    unsigned int dstFrames = 0;
    
    const NSString *BASE_PATH = @"./";

    MultiTrackQTMovie::Recorder *setup(MultiTrackQTMovie::Parser *parser, NSString *filename, std::vector<MultiTrackQTMovie::TrackInfo> *info) {        
        MultiTrackQTMovie::Recorder *recorder = new MultiTrackQTMovie::Recorder(filename,info);
        return recorder;
    }
    
    void add(MultiTrackQTMovie::Recorder *recorder, MultiTrackQTMovie::Parser *parser, unsigned char *bytes, long length) {
        if(recorder) {
            std::string codecType = parser->type(0);
            if(codecType=="jpeg"||codecType=="png ") {
                recorder->add((unsigned char *)bytes,length,0,true);
                dstFrames++;
            }
        }
    }
    
    void parse(NSString *commands, NSString *dst) {
        
        bool load = false;
        
        std::vector<MultiTrackQTMovie::TrackInfo> info;
        MultiTrackQTMovie::Recorder *recorder = nullptr;
        MultiTrackQTMovie::Parser *parser = nullptr;
        
        NSMutableDictionary *colors = nil;
        
        std::string codecType = "";
        
        NSArray *arr = [commands componentsSeparatedByString:@","];
        for(int n=0; n<arr.count; n++) {
            
            NSString *command = arr[n];
            NSRange mask = NSMakeRange(0,command.length);
                        
            if([command hasSuffix:@".mov"]) {
                if(!parser) {
                    
                    NSMutableString *path = [NSMutableString stringWithString:command];
                    
                    if([path hasPrefix:@"~/"]) {
                        [path setString:[path stringByReplacingOccurrencesOfString:@"~/" withString:[NSString stringWithFormat:@"/Users/%@/",NSUserName()]]];
                    }
                    else if([path rangeOfString:@"/"].location==NSNotFound) {
                        const NSArray *searchPath = @[
                            [NSString stringWithFormat:@"./%@",path],
                            [NSString stringWithFormat:@"/Users/%@/Movies/%@",NSUserName(),path],
                            [NSString stringWithFormat:@"/Users/%@/Downloads/%@",NSUserName(),path],
                            [NSString stringWithFormat:@"/Users/%@/Documents/%@",NSUserName(),path]
                        ];
                        for(int k=0; k<searchPath.count; k++) {
                            [path setString:searchPath[k]];
                            if([[NSFileManager defaultManager] attributesOfItemAtPath:path error:nil]) break;
                        }
                    }
                    
                    NSError *err = nil;
                    [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&err];
                    
                    if(!err) {
                        parser = new MultiTrackQTMovie::Parser(path);
                        codecType = parser->type(0);
                        bool isSupport = false;
                        for(int k=0; k<supportedCodecs.count; k++) {
                            if(codecType==[supportedCodecs[k] UTF8String]) {
                                isSupport = true;
                                break;
                            }
                        }
                        
                        if(isSupport) {
                            srcFrames = parser->length(0);
                            info.push_back({.width=parser->width(0),.height=parser->height(0),.depth=24,.fps=30.,.type=parser->type(0)});
                            if(codecType=="avc1"||codecType=="hvc1") {
                                NSError *err = nil;
                                NSString *path = @"./colors.json";
                                NSString *json= [[NSString alloc] initWithContentsOfFile:path encoding:NSUTF8StringEncoding error:&err];
                                if(err==nil) {
                                    NSData *jsonData = [json dataUsingEncoding:NSUnicodeStringEncoding];
                                    err = nil;
                                    colors = [NSJSONSerialization JSONObjectWithData:jsonData options:NSJSONReadingMutableContainers error:&err];
                                    if(err!=nil) colors = nil;
                                }
                            }
                            load = true;
                            continue;
                        }
                        else {
                            NSLog(@"%s codec is not supported",codecType.c_str());
                        }
                    }
                }
                else {
                    NSLog(@".mov is already loaded");
                    break;
                }
            }
            
            if(load) {
                if(colors) {
                    if(codecType=="avc1"||codecType=="hvc1") {
                        bool found = false;
                        for(NSString *color in colors) {
                            if([command compare:color]==NSOrderedSame) {
                                NSString *path = [NSString stringWithFormat:@"%@%s/%d/%d/%@.bin",BASE_PATH,codecType.c_str(),parser->width(0),parser->height(0),command];
                                NSError *err = nil;
                                [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&err];
                                if(!err) {
                                    if(!recorder) recorder = setup(parser,dst,&info);
                                    NSData *data = file(path);
                                    add(recorder,parser,(unsigned char *)data.bytes,data.length);
                                    found = true;
                                    break;
                                }
                            }
                        }
                        if(found) continue;
                    }
                }
                
                NSTextCheckingResult *match;
                
                match = [range firstMatchInString:command options:0 range:mask];
                if(match) {
                    NSArray *value = [command componentsSeparatedByString:@"-"];
                    if(value.count==2) {
                        int begin = [value[0] intValue];
                        int end = [value[1] intValue];
                        if(begin<=end) {
                            if(end<srcFrames) {
                                if(!recorder) recorder = setup(parser,dst,&info);
                                for(int k=begin; k<=end; k++) {
                                    NSData *data = parser->get(k,0);
                                    add(recorder,parser,(unsigned char *)data.bytes,data.length);
                                }
                                continue;
                            }
                        }
                        else {
                            if(begin<srcFrames) {
                                if(!recorder) recorder = setup(parser,dst,&info);
                                for(int k=begin; k>=end; k--) {
                                    NSData *data = parser->get(k,0);
                                    add(recorder,parser,(unsigned char *)data.bytes,data.length);
                                }
                                continue;
                            }
                        }
                    }
                }
                
                match = [repeate firstMatchInString:command options:0 range:mask];
                if(match) {
                    NSArray *value = [command componentsSeparatedByString:@"x"];
                    if(value.count==2) {
                        int frame = [value[0] intValue];
                        int times = [value[1] intValue];
                        if(frame<srcFrames) {
                            if(!recorder) recorder = setup(parser,dst,&info);
                            for(int k=0; k<times; k++) {
                                NSData *data = parser->get(frame,0);
                                unsigned char *bytes = (unsigned char *)data.bytes;
                                add(recorder,parser,(unsigned char *)data.bytes,data.length);
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
                            if(!recorder) recorder = setup(parser,dst,&info);
                            NSData *data = parser->get(frame,0);
                            unsigned char *bytes = (unsigned char *)data.bytes;
                            add(recorder,parser,(unsigned char *)data.bytes,data.length);
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
        
        if(recorder) {
            //NSLog(@"%@ (%d)",recorder->path(),dstFrames);
            recorder->save();
            delete recorder;
        }
    }
    
};


int main(int argc, char *argv[]) {
    @autoreleasepool {
        
        if(argc==2||argc==3) {
            
            NSString *filename = nil;
            
            if(argc==3&&[[NSString stringWithFormat:@"%s",argv[2]] hasSuffix:@".mov"]) {
                filename = [NSString stringWithFormat:@"%s",argv[2]];
            }
            
            VideoEditor::parse(
                [NSString stringWithFormat:@"%s",argv[1]],
                filename
            );
        }
    }
}
    