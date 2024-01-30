#import <Cocoa/Cocoa.h>
#import "QTM.h"

namespace VideoEditor {
    
    static const NSArray *supportedCodecs = @[@"jpeg",@"png "];
    unsigned int srcFrames = 0;
    unsigned int dstFrames = 0;
    const NSString *BASE_PATH = @"./";
    
    namespace RegExp {
        NSRegularExpression *regularExpression(NSString *pattern) {
            return [NSRegularExpression regularExpressionWithPattern:pattern options:NSRegularExpressionCaseInsensitive error:nil];
        }
        const NSRegularExpression *Number = regularExpression(@"[0-9]+");
        const NSRegularExpression *Range = regularExpression(@"[0-9]+-[0-9]+");
        const NSRegularExpression *Repeate = regularExpression(@"[0-9]+x[0-9]+");
    }
    
    namespace Type {
        bool match(NSString *command,const NSRegularExpression *regexp) {
            NSTextCheckingResult *match;
            match = [regexp firstMatchInString:command options:0 range:NSMakeRange(0,command.length)];
            return (match&&command.length==match.range.length)?true:false;
        }
        bool Movie(NSString *command) { return [command hasSuffix:@".mov"]; }
        bool Number(NSString *command) { return match(command,RegExp::Number); }
        bool Range(NSString *command) { return match(command,RegExp::Range);; }
        bool Repeate(NSString *command) { return match(command,RegExp::Repeate); }
        bool Other(NSString *command) {
            return (!Movie(command))&&(!Number(command))&&(!Range(command))&&(!Repeate(command));
        }
    }
    
    bool fileExistsAtPath(NSString *path) {
        NSError *err = nil;
        [[NSFileManager defaultManager] attributesOfItemAtPath:path error:&err];
        return (err==nil)?true:false;
    }
    
    NSData *file(NSString *path) {
        return [[NSFileManager defaultManager] contentsAtPath:path];
    }
        
    QTM::Recorder *setup(QTM::Parser *parser, NSString *filename, QTM::TrackInfo *info) {
        QTM::Recorder *recorder = new QTM::Recorder(filename,info);
        return recorder;
    }
    
    void add(QTM::Recorder *recorder, QTM::Parser *parser, NSData *data) {
        if(recorder) {
            unsigned char *bytes = (unsigned char *)data.bytes;
            long length = data.length;
            std::string codecType = parser->codec();
            recorder->add((unsigned char *)bytes,length,true);
            dstFrames++;
        }
    }
    
    NSMutableString *MutableString(NSString *cmd) {
        return [NSMutableString stringWithString:cmd];
    }
    
    NSMutableArray *split(NSString *commands) {
        NSMutableArray *arr = [[commands componentsSeparatedByString:@","] mutableCopy];
        if(arr&&arr.count>=2&&Type::Other(arr[0])&&Type::Movie(arr[1])) {
            [arr exchangeObjectAtIndex:0 withObjectAtIndex:1];
        }
        return arr;
    }
    
    int indexOf(NSString *cmd, NSString *str) {
        NSRange range = [cmd rangeOfString:str];
        return (range.length>0)?range.location:-1;
    }
    
    int count(NSString *cmd, NSRegularExpression *regexp) {
        return [regexp numberOfMatchesInString:cmd options:0 range:NSMakeRange(0,cmd.length)];
    }
    
    NSMutableString *trim(NSMutableString *cmd) {
        while(cmd.length) {
            if([[cmd substringWithRange:NSMakeRange(0,1)] isEqualToString:@"{"]&&[[cmd substringWithRange:NSMakeRange(cmd.length-1,1)] isEqualToString:@"}"]) {
                cmd = MutableString([cmd substringWithRange:NSMakeRange(1,cmd.length-2)]);
            }
            else {
                break;
            }
        }
        return cmd;
    }
    
    NSString *number(int n) {
        return [NSString stringWithFormat:@"%d",n];
    }
    
    NSString *range(int begin, int end) {
        return [NSString stringWithFormat:@"%d-%d",begin,end];
    }
    
    NSString *substring(NSString *cmd, int begin, int end) {
        return [cmd substringWithRange:NSMakeRange(begin,end)];
    }
    
    int U32(NSString *cmd) {
        return [cmd intValue];
    }
    
    void parse(NSString *commands, NSString *dst) {
        NSMutableArray<NSString *> *list = split(commands);
        if(list) {
            QTM::TrackInfo info;
            QTM::Recorder *recorder = nullptr;
            QTM::Parser *parser = nullptr;
            std::string codecType = "";
            bool load = false;
            for(int n=0; n<list.count; n++) {
                NSMutableString *command = MutableString(list[n]);
                if(command.length==0) continue;
                if(Type::Movie(command)) {
                    if(parser) {
                        delete parser;
                        parser = nullptr;
                    }
                    
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
                                if(fileExistsAtPath(path)) break;
                            }
                        }
                        if(fileExistsAtPath(path)) {
                            parser = new QTM::Parser(path);
                            codecType = parser->codec();
                            bool isSupport = false;
                            for(int k=0; k<supportedCodecs.count; k++) {
                                if(codecType==[supportedCodecs[k] UTF8String]) {
                                    isSupport = true;
                                    break;
                                }
                            }
                            if(isSupport) {
                                NSLog(@"%d",parser->length());
                                srcFrames = parser->length();
                                if(!load) {
                                    info = {.width=parser->width(),.height=parser->height(),.depth=24,.fps=30.,.codec=parser->codec()};
                                    load = true;
                                    continue;
                                }
                                else {
                                    if(parser->codec()==codecType) {
                                        if(parser->width()==info.width&&parser->height()==info.height) {
                                            continue;
                                        }
                                        else {
                                            NSLog(@".mov resolution is different");
                                        }
                                    }
                                    else {
                                        NSLog(@"%s codec is not supported",codecType.c_str());
                                    }
                                }
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
                    command = trim(command);
                    // check repeat
                    int repeat = 1;
                    NSArray<NSString *> *arr = [command componentsSeparatedByString:@"}x"];
                    if(arr.count==2) {
                        if(indexOf(command,@"{")==0&&Type::Number(arr[1])) {
                            repeat = U32(arr[1]);
                            command = MutableString(substring(arr[0],1,arr[0].length-1));
                        }
                        else if(indexOf(command,@"rand{")==0) {
                            repeat = U32(arr[1]);
                            command = MutableString(arr[0]);
                        }
                        else {
                            repeat = 0;
                            command = MutableString(@"");
                        }
                    }
                    
                    if(indexOf(command,@"rand{")==0) {
                        if(indexOf(command,@"}")==command.length-1) {
                            command = MutableString(substring(command,0,command.length-1));
                        }
                        if([command isEqualToString:@"rand{"]) {
                            command = MutableString(@"");
                            for(int r=0; r<repeat; r++) {
                                [command appendString:number(random()%srcFrames)];
                                if(r!=repeat-1) [command appendString:@","];
                            }
                            repeat = 0;
                        } 
                        else {
                            int min = 0;
                            int max = srcFrames-1;
                            arr = [command componentsSeparatedByString:@"{"];
                            if(arr.count==2) {
                                command = MutableString(@"");
                                if(Type::Number(arr[1])) {
                                    int max = U32(arr[1]);
                                    for(int l=0; l<repeat; l++) {
                                        [command appendString:number(random()%(max+1))];
                                        if(l!=repeat-1) [command appendString:@","];
                                    }
                                }
                                else if(Type::Range(arr[1])) {
                                    NSArray *value = [arr[1] componentsSeparatedByString:@"-"];
                                    if(value.count==2) {
                                        
                                        if([value[0] intValue]<[value[1] intValue]) {
                                            min = U32(value[0]);
                                            max = U32(value[1]);
                                        }
                                        else {
                                            min = U32(value[1]);
                                            max = U32(value[0]);
                                        }
                                        
                                        if(min>=srcFrames) {
                                            command = MutableString(@"");
                                            repeat = 0;
                                        }
                                        else if(max>=srcFrames) {
                                            command = MutableString(@"");
                                            repeat = 0;
                                        }
                                        else {
                                            for(int l=0; l<repeat; l++) {
                                                [command appendString:number(min+random()%((max-min)+1))];
                                                if(l!=repeat-1) [command appendString:@","];
                                            }
                                        }
                                        
                                    }
                                }
                                repeat = 0;
                            }
                            else {
                                command = MutableString(@"");
                                repeat = 0;
                            }
                        }
                    }
                    else if(repeat>=1&&count(command,RegExp::regularExpression(@"x"))<=1) {
                        command = trim(command);
                        if(indexOf(command,@"x{")==-1&&indexOf(command,@"}+")==-1) { // 0-29
                            if(!(Type::Number(command)||Type::Range(command))) {
                                arr = [command componentsSeparatedByString:@"x"];
                                if(arr.count==2&&Type::Number(arr[0])&&Type::Number(arr[1])) {
                                    repeat = U32(arr[1]);
                                    command = MutableString(arr[0]);
                                }
                                else {
                                    repeat = 0;
                                    command = MutableString(@"");
                                }
                            }
                        }
                        else {
                            
                            int scale = 1;
                            int add = 0;
                            
                            // check scale
                            if(indexOf(command,@"x{")>=1) {
                                arr = [command componentsSeparatedByString:@"x"];
                                if(Type::Number(arr[0])) {
                                    scale = U32(arr[0]);
                                    command = MutableString(arr[1]);
                                    command = trim(command);
                                }
                            }
                            
                            // check add
                            arr = [command componentsSeparatedByString:@"+"];
                            if(arr.count==2&&Type::Number(arr[1])) {
                                add = U32(arr[1]);
                                command = MutableString(arr[0]);
                                command = trim(command);
                            }
                            
                            if(scale!=0) {
                                if(scale==1) {
                                    if(Type::Number(command)) {
                                        command = MutableString(number([command intValue]+add));
                                    }
                                    else if(Type::Range(command)) {
                                        NSArray *value = [command componentsSeparatedByString:@"-"];
                                        if(value.count==2) {
                                            command = MutableString(range(U32(value[0])+add,U32(value[1])+add));
                                        }
                                    }
                                    else {
                                        repeat = 0;
                                        command = MutableString(@"");
                                    }
                                }
                                else {
                                    if(Type::Number(command)) {
                                        command = MutableString(number(scale*[command intValue]+add));
                                    }
                                    else if(Type::Range(command)) {
                                        NSArray *value = [command componentsSeparatedByString:@"-"];
                                        if(value.count==2) {
                                            int begin = U32(value[0]);
                                            int end = U32(value[1]);
                                            
                                            if(scale*begin+add>=srcFrames) {
                                                command = MutableString(@"");
                                                repeat = 0;
                                            }
                                            else if(scale*end+add>=srcFrames) {
                                                command = MutableString(@"");
                                                repeat = 0;
                                            }
                                            else {
                                                command = MutableString(@"");
                                                if(begin<end) {
                                                    for(int l=begin; l<=end; l++) {
                                                        [command appendString:number(scale*l+add)];
                                                        if(l!=end) [command appendString:@","];
                                                    }
                                                }
                                                else {
                                                    for(int l=begin; l>=end; l--) {
                                                        [command appendString:number(scale*l+add)];
                                                        if(l!=end) [command appendString:@","];
                                                    }
                                                }
                                            }
                                        }
                                        else {
                                            repeat = 0;
                                            command = MutableString(@"");
                                        }
                                    }
                                    else {
                                        repeat = 0;
                                        command = MutableString(@"");
                                    }
                                }
                            }
                            else {
                                repeat = 0;
                                command = MutableString(@"");
                            }
                        }
                    }
                    else {
                        repeat = 0;
                        command = MutableString(@"");
                    }
                    
                    if(command.length>=1) {
                        if(repeat>=1) {
                            NSString *c = [NSString stringWithFormat:@"%@",[command copy]];
                            if(Type::Range(c)) {
                                command = MutableString(@"");
                                for(int r=0; r<repeat; r++) {
                                    NSArray *value = [c componentsSeparatedByString:@"-"];
                                    if(value.count==2) {
                                        int begin = U32(value[0]);
                                        int end = U32(value[1]);
                                        
                                        if(begin>=srcFrames) {
                                            command = MutableString(@"");
                                            repeat = 0;
                                        }
                                        else if(end>=srcFrames) {
                                            command = MutableString(@"");
                                            repeat = 0;
                                        }
                                        else {
                                            if(begin<end) {
                                                for(int i=begin; i<=end; i++) {
                                                    [command appendString:number(i)];
                                                    if(i!=end) [command appendString:@","];
                                                }
                                            }
                                            else {
                                                for(int i=begin; i>=end; i--) {
                                                    [command appendString:number(i)];
                                                    if(i!=end) [command appendString:@","];
                                                }
                                            }
                                        }
                                    }
                                    if(r!=repeat-1) [command appendString:@","];
                                }
                            }
                            else {
                                command = MutableString(@"");
                                for(int r=0; r<repeat; r++) {
                                    [command appendString:c]; 
                                    if(r!=repeat-1) [command appendString:@","];
                                }
                            }
                        }
                        
                        NSMutableArray<NSString *> *result = [[command componentsSeparatedByString:@","] mutableCopy];
                        int num = 0;
                        for(int i=0; i<result.count; i++) {
                            if(Type::Number(result[i])) {
                                int frame = U32(result[i]);
                                if(frame<srcFrames) {
                                    if(!recorder) recorder = setup(parser,dst,&info);
                                    add(recorder,parser,parser->get(frame,0));
                                    num++;
                                }
                                else {
                                    NSLog(@"? %d",frame);
                                }
                            }
                        }
                        if(num) continue;
                    }
                }
                else {
                    NSLog(@".mov is not loaded");
                    break;
                }
                
                NSLog(@"? \"%@\"",list[n]);
            }
            
            if(load&&dstFrames>=1) {
                __block bool wait = true;
                QTM::EventEmitter::on(QTM::Event::SAVE_COMPLETE,^(NSNotification *notification) {
                    NSLog(@"SAVE_COMPLETE");
                    QTM::EventEmitter::off(QTM::Event::SAVE_COMPLETE);
                    wait = false;
                });
                recorder->save();
                while(wait) {
                    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:1.0f]];
                }
            }
            
            if(recorder) {
                delete recorder;
                recorder = nullptr;
            }
        }
    }
};

int main(int argc, char *argv[]) {
    @autoreleasepool {
        if(argc==2||argc==3) {
            srandom(CFAbsoluteTimeGetCurrent());
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