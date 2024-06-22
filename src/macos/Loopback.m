#include "../loopback.h"
#include "../state.h"

#include <AVFAudio/AVFAudio.h>
#include <AVKit/AVKit.h>
#include <CoreAudioTypes/CoreAudioTypes.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>
#include <Foundation/Foundation.h>
#include <MacTypes.h>
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#include <stdlib.h>
#include <sys/types.h>

@interface StreamOutput : NSObject <SCStreamOutput, SCStreamDelegate> {
}
@end

@implementation StreamOutput
- (void)stream:(SCStream *)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type {
    if (!CMSampleBufferIsValid(sampleBuffer)) {
        NSLog(@"SampleBuffer is invalid");
        return;
    }

    switch (type) {
    case SCStreamOutputTypeScreen: {
    } break;
    case SCStreamOutputTypeAudio: {
        size_t bufferListSizeNeeded;
        CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
            sampleBuffer, &bufferListSizeNeeded, nil, 0, nil, nil, 0, nil);

        AudioBufferList *bufferList =
            (AudioBufferList *)malloc(bufferListSizeNeeded);

        CMBlockBufferRef block = NULL;

        OSStatus status =
            CMSampleBufferGetAudioBufferListWithRetainedBlockBuffer(
                sampleBuffer, nil, bufferList, bufferListSizeNeeded, nil, nil,
                0, &block);

        if (status != noErr) {
            NSLog(@"%s\n\n\n",
                  [[NSError errorWithDomain:NSOSStatusErrorDomain
                                       code:status
                                   userInfo:nil].description cString]);
            return;
        }

        CMAudioFormatDescriptionRef desc =
            CMSampleBufferGetFormatDescription(sampleBuffer);

        const AudioStreamBasicDescription *audioDesc =
            CMAudioFormatDescriptionGetStreamBasicDescription(desc);

        AVAudioFormat *format = [[AVAudioFormat alloc]
            initStandardFormatWithSampleRate:audioDesc->mSampleRate
                                    channels:audioDesc->mChannelsPerFrame];

        AVAudioPCMBuffer *buffer = [[AVAudioPCMBuffer alloc]
            initWithPCMFormat:format
             bufferListNoCopy:bufferList
                  deallocator:^(const AudioBufferList *_Nonnull deallocator) {
                    free(bufferList);
                  }];

        B8 nothing_playing = true;
        if (StateGetLoopback()) {
            for (uint i = 0; i < buffer.frameLength; ++i) {
                if (buffer.floatChannelData[0][i] != 0) {
                    nothing_playing = false;
                }

                StatePushFrame(buffer.floatChannelData[0][i]);
            }
        }

        StateSetZeroFrequencies(nothing_playing);

        CFRelease(block);
    } break;
    default:
        break;
    }
}
@end

void StartAudioLoopback(SCShareableContent *_Nullable shareableContent) {
    NSError *error;

    SCDisplay *selectedDisplay;
    for (SCDisplay *display in [shareableContent displays]) {
        if (display.displayID == CGMainDisplayID()) {
            selectedDisplay = display;
        }
    }

    SCContentFilter *filter = [[SCContentFilter alloc]
              initWithDisplay:[[shareableContent displays] firstObject]
        excludingApplications:[NSArray array]
             exceptingWindows:[NSArray array]];

    SCStreamConfiguration *config = [[SCStreamConfiguration alloc] init];
    [config setWidth:filter.contentRect.size.width * filter.pointPixelScale];
    [config setHeight:filter.contentRect.size.height * filter.pointPixelScale];

    [config setMinimumFrameInterval:CMTimeMake(1, 60)];
    [config setCapturesAudio:true];
    [config setExcludesCurrentProcessAudio:true];
    [config setSampleRate:48000];
    [config setChannelCount:2];
    [config setMinimumFrameInterval:CMTimeMake(0, 144)];

    StreamOutput *streamOutput = [[StreamOutput alloc] init];

    SCStream *stream = [[SCStream alloc] initWithFilter:filter
                                          configuration:config
                                               delegate:streamOutput];

    [stream addStreamOutput:streamOutput
                       type:SCStreamOutputTypeAudio
         sampleHandlerQueue:dispatch_get_main_queue()
                      error:&error];

    if (error) {
        NSLog(@"Error occured: %@", error);
    }

    [stream startCaptureWithCompletionHandler:^(NSError *_Nullable error) {
      if (error) {
          NSLog(@"Error occured: %@", error);
      }
    }];
}

void LoopbackBegin() {
    [SCShareableContent
        getShareableContentExcludingDesktopWindows:false
                               onScreenWindowsOnly:false
                                 completionHandler:^(
                                     SCShareableContent
                                         *_Nullable shareableContent,
                                     NSError *_Nullable error) {
                                   if (error) {
                                       switch (error.code) {
                                       case SCStreamErrorUserDeclined:
                                           [[NSWorkspace sharedWorkspace]
                                               openURL:
                                                   [[NSURL alloc]
                                                       initWithString:
                                                           @"x-apple."
                                                           @"systempreferences:"
                                                           @"com."
                                                           @"apple.preference."
                                                           @"security?Privacy_"
                                                           @"ScreenCapture"]];
                                           break;
                                       default:
                                           NSLog(@"Error: %@",
                                                 error.localizedDescription);
                                           break;
                                       }

                                       return;
                                   }

                                   StartAudioLoopback(shareableContent);
                                 }];
}
