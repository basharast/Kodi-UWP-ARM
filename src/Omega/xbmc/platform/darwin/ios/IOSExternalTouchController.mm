/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "IOSExternalTouchController.h"

#include "Util.h"
#import "XBMCController.h"
#include "filesystem/SpecialProtocol.h"
#include "guilib/LocalizeStrings.h"
#include "input/mouse/MouseStat.h"

//dim the touchscreen after 15 secs without touch event
const CGFloat touchScreenDimTimeoutSecs       = 15.0;
const CGFloat timeFadeSecs                    = 2.0;

@implementation IOSExternalTouchController
//--------------------------------------------------------------
- (id)init
{
  CGRect frame = [[UIScreen mainScreen] bounds];

  self = [super init];
  if (self)
  {
    UIImage       *xbmcLogo;
    UIImageView   *xbmcLogoView;
    UILabel       *descriptionLabel;

    _internalWindow = [[UIWindow alloc] initWithFrame:frame];
    _touchView = [[UIView alloc] initWithFrame:frame];
    /* Turn on autoresizing for the whole hierarchy*/
    [_touchView setAutoresizesSubviews:YES];
    [_touchView setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [_touchView setAlpha:0.0];//start with alpha 0 and fade in with animation below
    [_touchView setMultipleTouchEnabled:YES];
    [_touchView setContentMode:UIViewContentModeCenter];

    //load the splash image
    std::string strUserSplash = CUtil::GetSplashPath();
    xbmcLogo = [UIImage imageWithContentsOfFile:[NSString stringWithUTF8String:strUserSplash.c_str()]];
    
    //make a view with the image
    xbmcLogoView = [[UIImageView alloc] initWithImage:xbmcLogo];
    //center the image and add it to the view
    [xbmcLogoView setFrame:frame];
    [xbmcLogoView setContentMode:UIViewContentModeScaleAspectFill];
    //autoresize the image frame
    [xbmcLogoView setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [xbmcLogoView setAutoresizesSubviews:YES];
    [_touchView addSubview:xbmcLogoView];
    //send the image to the background
    [_touchView sendSubviewToBack:xbmcLogoView];

    CGRect labelRect = frame;
    labelRect.size.height/=2;
    //uilabel with the description
    descriptionLabel = [[UILabel alloc] initWithFrame:labelRect];
    //gray textcolor on transparent background
    [descriptionLabel setTextColor:[UIColor grayColor]];
    [descriptionLabel setBackgroundColor:[UIColor clearColor]];
    //text is aligned left in its frame
    [descriptionLabel setTextAlignment:NSTextAlignmentCenter];
    [descriptionLabel setContentMode:UIViewContentModeCenter];
    //setup multiline behaviour
    [descriptionLabel setLineBreakMode:(NSLineBreakMode)NSLineBreakByTruncatingTail];

    [descriptionLabel setNumberOfLines:5];
    std::string descText    = g_localizeStrings.Get(34404) + "\n";
    descText              += g_localizeStrings.Get(34405) + "\n";
    descText              += g_localizeStrings.Get(34406) + "\n";
    descText              += g_localizeStrings.Get(34407) + "\n";
    descText              += g_localizeStrings.Get(34408) + "\n";

    NSString *stringFromUTFString = [[NSString alloc] initWithUTF8String:descText.c_str()];

    [descriptionLabel setText:stringFromUTFString];

    //resize it to full view
    [descriptionLabel setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];
    [descriptionLabel setAutoresizesSubviews:YES];
    [_touchView addSubview:descriptionLabel];

    [[self view] addSubview: _touchView];

    [self createGestureRecognizers];

    [_internalWindow addSubview:[self view]];
    [_internalWindow setBackgroundColor:[UIColor blackColor]];
    [_internalWindow setScreen:[UIScreen mainScreen]];
    [_internalWindow makeKeyAndVisible];
    [_internalWindow setRootViewController:self];

    [self startSleepTimer];//will fade from black too
  }
  return self;
}
//--------------------------------------------------------------
- (UIInterfaceOrientationMask)supportedInterfaceOrientations
{
  return UIInterfaceOrientationMaskLandscape;
}
//--------------------------------------------------------------
- (void)startSleepTimer
{
  NSDate *fireDate = [NSDate dateWithTimeIntervalSinceNow:touchScreenDimTimeoutSecs];

  //schedule sleep timer to fire once in touchScreenDimTimeoutSecs if not reset
  _sleepTimer       = [[NSTimer alloc] initWithFireDate:fireDate
                                      interval:1
                                      target:self
                                      selector:@selector(sleepTimerCallback:)
                                      userInfo:nil
                                      repeats:NO];
  //schedule the timer to the runloop
  NSRunLoop *runLoop = [NSRunLoop currentRunLoop];
  [self fadeFromBlack];
  [runLoop addTimer:_sleepTimer forMode:NSDefaultRunLoopMode];
}
//--------------------------------------------------------------
- (void)stopSleepTimer
{
  if(_sleepTimer != nil)
  {
    [_sleepTimer invalidate];
    _sleepTimer = nil;
  }
}
//--------------------------------------------------------------
- (void)sleepTimerCallback:(NSTimer*)theTimer
{
  [self fadeToBlack];
  [self stopSleepTimer];
}
//--------------------------------------------------------------
- (bool)wakeUpFromSleep//returns false if we where dimmed, true if not
{
  if(_sleepTimer == nil)
  {
    [self startSleepTimer];
    return false;
  }
  else
  {
    NSDate *fireDate = [NSDate dateWithTimeIntervalSinceNow:touchScreenDimTimeoutSecs];
    [_sleepTimer setFireDate:fireDate];
    return true;
  }
}
//--------------------------------------------------------------
- (void)fadeToBlack
{
    [UIView animateWithDuration:timeFadeSecs delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    [_touchView setAlpha:0.01];//fade to black (don't fade to 0.0 else we don't get gesture callbacks)
    }
    completion:^(BOOL finished){}];
}
//--------------------------------------------------------------
- (void)fadeFromBlack
{
    [UIView animateWithDuration:timeFadeSecs delay:0 options:UIViewAnimationOptionCurveEaseInOut animations:^{
    [_touchView setAlpha:1.0];//fade in to full alpha
    }
    completion:^(BOOL finished){}];
}
//--------------------------------------------------------------
- (void)createGestureRecognizers
{
  //2 finger single tab - right mouse
  //single finger double tab delays single finger single tab - so we
  //go for 2 fingers here - so single finger single tap is instant
  UITapGestureRecognizer *doubleFingerSingleTap = [[UITapGestureRecognizer alloc]
                                                    initWithTarget:self action:@selector(handleDoubleFingerSingleTap:)];
  [doubleFingerSingleTap setNumberOfTapsRequired:1];
  [doubleFingerSingleTap setNumberOfTouchesRequired:2];
  [[self view] addGestureRecognizer:doubleFingerSingleTap];

  //1 finger single long tab - right mouse - alternative
  UITapGestureRecognizer *singleFingerSingleLongTap = (UITapGestureRecognizer*)[[UILongPressGestureRecognizer alloc]
                                                        initWithTarget:self action:@selector(handleSingleFingerSingleLongTap:)];
  singleFingerSingleLongTap.delaysTouchesBegan = YES;
  singleFingerSingleLongTap.delaysTouchesEnded = YES;
  singleFingerSingleLongTap.numberOfTouchesRequired = 1;
  [self.view addGestureRecognizer:singleFingerSingleLongTap];

  //1 finger single tab - left mouse
  UITapGestureRecognizer *singleFingerSingleTap = [[UITapGestureRecognizer alloc]
                                                    initWithTarget:self action:@selector(handleSingleFingerSingleTap:)];
  [singleFingerSingleTap setDelaysTouchesBegan:NO];
  [[self view] addGestureRecognizer:singleFingerSingleTap];

  //double finger swipe left for backspace ... i like this fast backspace feature ;)
  UISwipeGestureRecognizer *swipeDoubleLeft = [[UISwipeGestureRecognizer alloc]
                                          initWithTarget:self action:@selector(handleDoubleSwipeLeft:)];
  [swipeDoubleLeft setNumberOfTouchesRequired:2];
  [swipeDoubleLeft setDirection:UISwipeGestureRecognizerDirectionLeft];
  [[self view] addGestureRecognizer:swipeDoubleLeft];

  //single finger swipe left for left
  UISwipeGestureRecognizer *swipeLeft = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeLeft:)];
  [swipeLeft setNumberOfTouchesRequired:1];
  [swipeLeft setDirection:UISwipeGestureRecognizerDirectionLeft];
  [[self view] addGestureRecognizer:swipeLeft];

  //single finger swipe right for right
  UISwipeGestureRecognizer *swipeRight = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeRight:)];
  [swipeRight setNumberOfTouchesRequired:1];
  [swipeRight setDirection:UISwipeGestureRecognizerDirectionRight];
  [[self view] addGestureRecognizer:swipeRight];

  //single finger swipe up for up
  UISwipeGestureRecognizer *swipeUp = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeUp:)];
  [swipeUp setNumberOfTouchesRequired:1];
  [swipeUp setDirection:UISwipeGestureRecognizerDirectionUp];
  [[self view] addGestureRecognizer:swipeUp];

  //single finger swipe down for down
  UISwipeGestureRecognizer *swipeDown = [[UISwipeGestureRecognizer alloc]
                                      initWithTarget:self action:@selector(handleSwipeDown:)];
  [swipeDown setNumberOfTouchesRequired:1];
  [swipeDown setDirection:UISwipeGestureRecognizerDirectionDown];
  [[self view] addGestureRecognizer:swipeDown];

}
//--------------------------------------------------------------
- (IBAction)handleDoubleSwipeLeft:(UISwipeGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_BACKSPACE];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeLeft:(UISwipeGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_LEFT];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeRight:(UISwipeGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_RIGHT];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeUp:(UISwipeGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_UP];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSwipeDown:(UISwipeGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_DOWN];
  }
}
//--------------------------------------------------------------
- (IBAction)handleDoubleFingerSingleTap:(UIGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_c];
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleLongTap:(UIGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    if (sender.state == UIGestureRecognizerStateEnded)
    {
      [self handleDoubleFingerSingleTap:sender];
    }
  }
}
//--------------------------------------------------------------
- (IBAction)handleSingleFingerSingleTap:(UIGestureRecognizer *)sender
{
  if([self wakeUpFromSleep])
  {
    [g_xbmcController sendKey:XBMCK_RETURN];
  }
}
//--------------------------------------------------------------
- (void)viewWillAppear:(BOOL)animated
{
  [super viewWillAppear:animated];
}
//--------------------------------------------------------------
- (void)dealloc
{
  [self stopSleepTimer];
}
//--------------------------------------------------------------
@end
