/*
setvol - set system sound volume (Mac OS X 10.6)

Usage:
./setvol 0 ./setvol 0.5 ./setvol 1

code based on: http://www.cocoadev.com/index.pl?SoundVolume (AudioToolbox snippet by Ryan)

compile with: gcc -Wall -O3 -x objective-c -framework AudioToolbox -framework Foundation -framework CoreAudio -o setvol setvol.c


shw 2019-03-11
was supposed to be at http://www.cocoadev.com/SoundVolume
retrieved from archive.org
de-wikified (restored the three imports and fixed a few objc message brackets)
compiles fine on Mac OS 10.6
*/


#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>

@interface KNVolumeControl : NSObject
{
  float volume; 
  AudioDeviceID defaultOutputDeviceID; 
}
+ (float) volume;  // setting system volume - mutes if under threshhold 
+ (void) setVolume: (Float32) newVolume;
+ (AudioDeviceID) defaultOutputDeviceID;
@end


@implementation KNVolumeControl

+ (float) volume
{
  Float32 outputVolume;
  
  UInt32 propertySize = 0;
  OSStatus status = noErr;
  AudioObjectPropertyAddress propertyAOPA;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
  propertyAOPA.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  
  AudioDeviceID outputDeviceID = [KNVolumeControl defaultOutputDeviceID];
  
  if (outputDeviceID == kAudioObjectUnknown)
  {
      NSLog(@"Unknown device");
      return 0.0;
  }
  
  if (!AudioHardwareServiceHasProperty(outputDeviceID, &propertyAOPA))
  {
      NSLog(@"No volume returned for device 0x%0x", outputDeviceID);
      return 0.0;
  }
  
  propertySize = sizeof(Float32);
  
  status = AudioHardwareServiceGetPropertyData(outputDeviceID, &propertyAOPA, 0, NULL, &propertySize, &outputVolume);
  
  if (status)
  {
      NSLog(@"No volume returned for device 0x%0x", outputDeviceID);
      return 0.0;
  }
  
  if (outputVolume < 0.0 || outputVolume > 1.0) return 0.0;
  
  return outputVolume;
}

// setting system volume - mutes if under threshhold
+ (void) setVolume:(Float32)newVolume
{
    if (newVolume < 0.0 || newVolume > 1.0)
    {
      NSLog(@"Requested volume out of range (%.2f)", newVolume);
      return;
    }
  
  // get output device device
  UInt32 propertySize = 0;
  OSStatus status = noErr;
  AudioObjectPropertyAddress propertyAOPA;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
  propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
  
  if (newVolume < 0.001)
  {
      NSLog(@"Requested mute");
      propertyAOPA.mSelector = kAudioDevicePropertyMute;
  }
  else
  {
      NSLog(@"Requested volume %.2f", newVolume);
      propertyAOPA.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
  }
  
  AudioDeviceID outputDeviceID = [KNVolumeControl defaultOutputDeviceID];
  
  if (outputDeviceID == kAudioObjectUnknown)
  {
      NSLog(@"Unknown device");
      return;
  }
  
  if (!AudioHardwareServiceHasProperty(outputDeviceID, &propertyAOPA))
  {
      NSLog(@"Device 0x%0x does not support volume control", outputDeviceID);
      return;
  }
  
  Boolean canSetVolume = NO;
  
  status = AudioHardwareServiceIsPropertySettable(outputDeviceID, &propertyAOPA, &canSetVolume);
  
  if (status || canSetVolume == NO)
  {
      NSLog(@"Device 0x%0x does not support volume control", outputDeviceID);
      return;
  }

  if (propertyAOPA.mSelector == kAudioDevicePropertyMute)
  {
    propertySize = sizeof(UInt32);
    UInt32 mute = 1;
    status = AudioHardwareServiceSetPropertyData(outputDeviceID, &propertyAOPA, 0, NULL, propertySize, &mute);      
  }
  else
  {
    propertySize = sizeof(Float32);

    status = AudioHardwareServiceSetPropertyData(outputDeviceID, &propertyAOPA, 0, NULL, propertySize, &newVolume);

    if (status)
    {
        NSLog(@"Unable to set volume for device 0x%0x", outputDeviceID);
    }

    // make sure we're not muted
    propertyAOPA.mSelector = kAudioDevicePropertyMute;
    propertySize = sizeof(UInt32);
    UInt32 mute = 0;

    if (!AudioHardwareServiceHasProperty(outputDeviceID, &propertyAOPA))
    {
        NSLog(@"Device 0x%0x does not support muting", outputDeviceID);
        return;
    }

    Boolean canSetMute = NO;

    status = AudioHardwareServiceIsPropertySettable(outputDeviceID, &propertyAOPA, &canSetMute);

    if (status || !canSetMute)
    {
        NSLog(@"Device 0x%0x does not support muting", outputDeviceID);
        return;
    }

    status = AudioHardwareServiceSetPropertyData(outputDeviceID, &propertyAOPA, 0, NULL, propertySize, &mute);
  }

  if (status)
  {
      NSLog(@"Unable to set volume for device 0x%0x", outputDeviceID);
  }
}

+ (AudioDeviceID) defaultOutputDeviceID
{
  AudioDeviceID outputDeviceID = kAudioObjectUnknown;
  
  // get output device device
  UInt32 propertySize = 0;
  OSStatus status = noErr;
  AudioObjectPropertyAddress propertyAOPA;
  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
  propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  
  if (!AudioHardwareServiceHasProperty(kAudioObjectSystemObject, &propertyAOPA))
  {
      NSLog(@"Cannot find default output device!");
      return outputDeviceID;
  }
  
  propertySize = sizeof(AudioDeviceID);
  
  status = AudioHardwareServiceGetPropertyData(kAudioObjectSystemObject, &propertyAOPA, 0, NULL, &propertySize, &outputDeviceID);
  
  if(status) 
  {
      NSLog(@"Cannot find default output device!");
  }
  return outputDeviceID;
}

@end


int main (int argc, char *argv[])
{
  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
  
  Float32 involume;
  
  if (argc != 2) return 1;
  
  NSString *input = [NSString stringWithUTF8String:argv[1] ];
  involume = [input floatValue];
  
  if (involume < 0.0 || involume > 1.0) return 1;
  
  [KNVolumeControl setVolume: involume];
  
  [pool release];
  
  return 0;
}
