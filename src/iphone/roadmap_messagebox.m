/* roadmap_messagebox.c - manage the roadmap dialogs used for user info.
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
 * SYNOPSYS:
 *
 *   See roadmap_messagebox.h
 */

#import <UIKit/UIAlert.h>

#include "roadmap.h"
#include "roadmap_start.h"
#import "roadmap_iphonemain.h"
#import "roadmap_iphonemessagebox.h"

#include "roadmap_messagebox.h"

static AlertsViewController *controller = NULL;

@implementation AlertsViewController

-(void)showWithTitle: (const char *) title text: (const char *) text modal: (int) modal
{
    UIAlertView * zSheet;
    NSString *nstitle = [[NSString alloc] initWithUTF8String: title];
    NSString *nstext = [[NSString alloc] initWithUTF8String: text];
    zSheet = [[UIAlertView alloc] initWithTitle:nstitle message: nstext delegate:self cancelButtonTitle: @"OK" otherButtonTitles: nil];
//    [zSheet setRunsModal: true]; //I'm a big fan of running sheet modally
    
//    [zSheet popupAlertAnimated:YES]; //Displays
              //Pauses here until user taps the sheet closed
    [zSheet show];
              
    [zSheet autorelease];
    [nstitle release];
    [nstext release];
}

-(void)alertView:(UIAlertView*)sheet clickedButtonAtIndex:(NSInteger)button
{
//   [sheet dismiss];
}
@end

static void *roadmap_messagebox_show (const char *title,
        const char *text, int modal) {
    if (controller == NULL)
        controller = [[AlertsViewController alloc] init];

    [controller showWithTitle: title text: text modal: modal];
    return controller;
}

void roadmap_messagebox_hide (void *handle) {
    //gtk_widget_destroy ((GtkWidget *)handle);
}
   


void *roadmap_messagebox (const char *title, const char *message) {
   return roadmap_messagebox_show (title, message, 0);
}

void *roadmap_messagebox_wait (const char *title, const char *message) {
   return roadmap_messagebox_show (title, message, 1);
}

void roadmap_messagebox_die (const char *title, const char *message) {
   (void)roadmap_messagebox_show (title, message, 1);
}
