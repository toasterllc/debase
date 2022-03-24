#import <Foundation/Foundation.h>
#import "State.h"
#import "Debase.h"
#import <filesystem>

std::filesystem::path StateDir() {
    NSArray<NSURL*>* urls = [[NSFileManager defaultManager] URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask];
    if (![urls count]) throw Toastbox::RuntimeError("-[NSFileManager URLsForDirectory:inDomains:] returned empty array");
    
    NSURL* appSupportDir = urls[0];
    std::filesystem::path path = [appSupportDir fileSystemRepresentation];
    return path / DebaseBundleId;
}
