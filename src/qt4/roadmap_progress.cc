/*
 * LICENSE:
 *
 *   (c) Copyright 2008 Alessandro Briosi
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

extern "C" {
#include "roadmap_progress.h"
#include "roadmap.h"
}

#include <qmap.h>
#include "qt_progress.h"

QMap<int, RMapProgressDialog*> progressDialogs;
static int lastTag = 0;

int
roadmap_progress_new(void)
{
    /* had to add this, 'cause on init it counts always 1 element (?) */
    if (lastTag == 0)
	progressDialogs.clear();

    RMapProgressDialog *pd = new RMapProgressDialog();
    lastTag++;
    progressDialogs[lastTag] = pd;
    pd->show();
    return lastTag;
}

void
roadmap_progress_update(int tag, int total, int progress)
{
    RMapProgressDialog *pd = progressDialogs[tag];
    if (pd != 0) {
	pd->setMaximum(total);
	pd->setProgress(progress);
    }
}

void
roadmap_progress_close(int tag)
{
    RMapProgressDialog *pd = progressDialogs[tag];

    if (pd != 0) {
	pd->hide();
	progressDialogs.remove(tag);
	delete pd;		/* is this necessary ? */
	if (progressDialogs.count() == 0)
	    lastTag = 0;
    }
}
