/* roadmap_iphonemain.h
 *
 * LICENSE:
 *
 *   Copyright 2002 Pascal F. Martin
 *   Copyright 2008 Morten Bek Ditlevsen
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * DESCRIPTION:
 *
 *   This defines a toolkit-independent interface for the canvas used
 *   to draw the map. Even while the canvas implementation is highly
 *   toolkit-dependent, the interface must not.
 */

#ifndef INCLUDE__ROADMAP_IPHONE_MAIN__H
#define INCLUDE__ROADMAP_IPHONE_MAIN__H

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <UIKit/CDStructures.h>
#import <UIKit/UIWindow.h>
#import <UIKit/UIView-Hierarchy.h>
#import <UIKit/UIView-Geometry.h>
#import <UIKit/UIHardware.h>
#import <UIKit/UIKit.h>
#import <UIKit/UIApplication.h>
#import <UIKit/UIImageView.h>
#import <UIKit/UIImage.h>
#import <UIKit/UITextView.h>
#import <UIKit/UIButtonBarTextButton.h>
#import <UIKit/UIPushButton.h>
#import <UIKit/UIView-Geometry.h>

@interface RoadMapApp : UIApplication {
}
-(void)buttonBarItemTapped:(id) sender;
-(UIButtonBar *)createButtonBar;
-(void) newWithTitle: (const char *)title andWidth: (int) width andHeight: (int) height;
-(void) ioCallback: (id) notify;
-(void) setInput: (RoadMapIO*) io andCallback: (RoadMapInput) callback;
-(void) removeInput: (RoadMapIO*) io;
-(void) periodicCallback: (NSTimer *) nstimer;
-(void) setPeriodic: (float) interval andCallback: (RoadMapCallback) callback;
-(void) removePeriodic: (RoadMapCallback) callback;

@end

#endif // INCLUDE__ROADMAP_IPHONE_CANVAS__H

