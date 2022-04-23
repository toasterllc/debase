#import <Foundation/Foundation.h>
#import <CoreServices/CoreServices.h>
#import <string>
#import "OpenURL.h"

void OpenURL(std::string_view url) noexcept {
    @autoreleasepool {
        LSOpenCFURLRef((__bridge CFURLRef)[NSURL URLWithString:@(url.data())], nil);
    }
}
