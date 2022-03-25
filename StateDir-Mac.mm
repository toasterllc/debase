#import <Foundation/Foundation.h>
#import "State.h"
#import "Debase.h"
#import "StateDir.h"
#import <filesystem>

std::filesystem::path StateDir() {
    NSArray<NSURL*>* urls = [[NSFileManager defaultManager] URLsForDirectory:NSLibraryDirectory inDomains:NSUserDomainMask];
    if (![urls count]) throw Toastbox::RuntimeError("-[NSFileManager URLsForDirectory:inDomains:] returned empty array");
    
    NSURL* libDir = urls[0];
    std::filesystem::path path = [libDir fileSystemRepresentation];
    return path / "Preferences" / DebaseBundleId;
}
