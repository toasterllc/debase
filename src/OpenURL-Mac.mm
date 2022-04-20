#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>
#import <string>
#import "OpenURL.h"

void OpenURL(std::string_view url) {
    @autoreleasepool {
        LSOpenCFURLRef((__bridge CFURLRef)[NSURL URLWithString:@(url.data())], nil);
    }
}

void OpenMailto(std::string_view email) {
    std::string mailto = "mailto:"+std::string(email);
    OpenURL(mailto);
}
