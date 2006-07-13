/* roadmap_linefont.c - draw text on a map.
 *
 * LICENSE:
 *
 *   Copyright 2006 Paul G. Fox
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
 * Some of the platforms on which RoadMap runs do not have font support
 * sophisticated enough to render angled or scaled text.  (The GTK-1.2
 * desktop is one of these, some builds of QT are another.)  This file
 * provides a fallback mechanism for drawing text on a map image, for
 * those platforms.
 *
 * The code in this file borrows heavily from the Xrotfont package
 * posted to Usenet in 1992 by Alan Richardson.  His implementation
 * was based in turn on an earlier work distributed with the Hershey
 * fonts themselves.  Credits as listed in Alan Richardson's compilation
 * are copied here.  Additional original use restrictions appear
 * at bottom of this file.
 *
 *
 *     ******************************************************************
 *
 *     hersheytext.c - routines to facilitate the loading and
 *                      painting of hershey fonts
 *
 *     Functions:  hersheytext()
 *                 load_hershey_font()
 *                 scanint()      
 *                 
 *     Date:       8/5/92
 *
 *     Copyright   (c) 1992 Alan Richardson
 *
 *     *****************************************************************
 *
 *     The code in this file is based heavily on the program "hershey.c",
 *      distributed with the Hershey fonts. The credits for that program ...
 *
 *       Translated by Pete Holzmann
 *           Octopus Enterprises
 *           19611 La Mar Court
 *           Cupertino, CA 95014
 *   
 *        Original...
 *       .. By James Hurt when with
 *       ..    Deere and Company
 *       ..    John Deere Road
 *       ..    Moline, IL 61265
 *    
 *       .. now with Cognition, Inc.
 *       ..          900 Technology Park Drive
 *       ..          Billerica, MA 01821
 *     
 *
 *
 *
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roadmap.h"
#include "roadmap_types.h"
#include "roadmap_gui.h"
#include "roadmap_math.h"

#include "roadmap_canvas.h"
#include "roadmap_linefont.h"

#ifdef ROADMAP_USE_LINEFONT

#define MAXCHARS 100
#define MAXPOINTS 256

typedef struct {
    unsigned char x, y;
} hpoint;

/* hershey font stuff ... */
typedef struct {
    hpoint *pt;
    unsigned char npnt;
    unsigned char minx, maxx;
} hershey_char;

typedef struct hershey_font {
    hershey_char hc[MAXCHARS];
    short nchars;
    unsigned char miny, maxy;
} hershey_font;

static hershey_font hf[1];

static const unsigned char *fontp;

const unsigned char romans_font[];

/* this function scans an integer, using n characters of the input stream,
    ignoring newlines ... */

static int
scanint(int n)
{
    char buf[20];
    int i, c;

    for (i = 0; i < n; i++) {
        while ((c = *fontp++) == '\n'); /* discard spare newlines */
        if (c == '\0')
            return (-1);
        buf[i] = c;
    }

    buf[i] = 0;
    return (atoi(buf));
}


/* this loads font information from a file into memory */
static void
roadmap_load_hershey_font(void)
{
    int np;
    int i, j;


    /* point to hershey font data ...  data has already been
     * sorted into ASCII order.
     * (Note: RoadMap only supports "Roman Simplex")
     */
    fontp = romans_font;

    /* loop through the characters in the file ... */
    for (j = 0; j < MAXCHARS; j++) {
        /* read character number and data ... */
        if (scanint(5) < 1) {
            break;
        } else {
            /* get the number of vertices in this figure ... */
            np = scanint(3);

            /* point directly at the ASCII font data */
            hf->hc[j].pt = (hpoint *)fontp;
            fontp += np * sizeof(hpoint);

            if (np > MAXPOINTS) {
                roadmap_log(ROADMAP_WARNING,
                    "Too many points in hershey character, truncated");
                np = MAXPOINTS;
            }

            hf->hc[j].npnt = np;

        }
    }

    if (j >= MAXCHARS) {
        roadmap_log(ROADMAP_WARNING,
            "Too many characters in line font, font truncated");
        j = MAXCHARS - 1;
    }
    hf->nchars = j;

    /* determine the size of each character ... */

    /* initialise ... */
    hf->miny = 255;
    hf->maxy = 0;

    /* loop through each character ... */
    for (j = 1; j < hf->nchars; j++) {
        hf->hc[j].minx = 255;
        hf->hc[j].maxx = 0;
        for (i = 1; i < hf->hc[j].npnt; i++) {
            if (hf->hc[j].pt[i].x != ' ') {
                if (hf->hc[j].pt[i].x < hf->hc[j].minx)
                    hf->hc[j].minx = hf->hc[j].pt[i].x;
                if (hf->hc[j].pt[i].x > hf->hc[j].maxx)
                    hf->hc[j].maxx = hf->hc[j].pt[i].x;
                if (hf->hc[j].pt[i].y < hf->miny)
                    hf->miny = hf->hc[j].pt[i].y;
                if (hf->hc[j].pt[i].y > hf->maxy)
                    hf->maxy = hf->hc[j].pt[i].y;
            }
        }
    }

    /* do the space character by hand ... same width as 't' */
    hf->hc[0].minx = hf->hc['t' - ' '].minx;
    hf->hc[0].maxx = hf->hc['t' - ' '].maxx;

    /* a little offset for a nice appearance ... */
    for (j = 0; j < hf->nchars; j++) {
        hf->hc[j].minx -= 3;
        hf->hc[j].maxx += 3;
    }
}

void roadmap_linefont_extents
        (const char *text, int size, int *width, int *ascent, int *descent) {

    int len, scale;
    const char *t;
    int k;

    if (fontp == 0) {
        roadmap_load_hershey_font();
    }

    scale = 1024 * size / (hf->maxy - hf->miny);

    /* find the length of the string in pixels ... */
    len = 0;

    for (t = text; *t; t++) {
        k = *t - ' ';
        if (k < hf->nchars) {
            len += hf->hc[k].maxx - hf->hc[k].minx;
        }
    }
    *width = scale * len / 1024;
    *ascent = scale * (hf->maxy - hf->miny)/ 1024;
    *descent = 0;   /* hershey gives no info re: "baseline" */

}

/* render a string to the screen ... */
void roadmap_linefont_text (const char *text, int where, 
        RoadMapGuiPoint *center, int size, int theta)
{

    RoadMapGuiPoint *p, points[MAXPOINTS];
    int i, k, len, height, xp, yp, count;
    long scale;
    const char *t;

    /* scale factor ... */
    scale = 1024 * size / (hf->maxy - hf->miny);

    /* find the length of the string in pixels ... */
    len = 0;

    for (t = text; *t; t++) {
        k = *t - ' ';
        if (k < hf->nchars) {
            len += hf->hc[k].maxx - hf->hc[k].minx;
        }
    }

    /* alignment stuff ... */

    xp = center->x + 1;
    yp = center->y;

    len = scale * len / 1024;
    height = scale * (hf->maxy - hf->miny) / 1024;

    if (where & ROADMAP_LINEFONT_RIGHT)
        xp -= len;
    else if (where & ROADMAP_LINEFONT_CENTER_X)
        xp -= len / 2;

    if (where & ROADMAP_LINEFONT_LOWER)
        yp -= height;
    else if (where & ROADMAP_LINEFONT_CENTER_Y)
        yp -= (height / 2);


    p = points;
    count = 0;
    /* loop through each character in the string ... */
    for (t = text; *t; t++) {

        k = *t - ' ';
        if (k >= hf->nchars) {
            continue;
        }

        /* loop through each vertex ... */
        for (i = 1; i < hf->hc[k].npnt; i++) {
            if (hf->hc[k].pt[i].x == ' ') {
                if (count) {
                    roadmap_canvas_draw_multiple_lines (1, &count, points, 1);
                    p = points;
                    count = 0;
                }
            } else {
                p->x = xp + scale * (hf->hc[k].pt[i].x - hf->hc[k].minx) / 1024;
                p->y = yp + scale * (hf->hc[k].pt[i].y - hf->miny) / 1024;

                p->x = p->x - center->x;
                p->y = center->y - p->y;

                roadmap_math_rotate_point (p, center, theta);

                p++;
                count++;

            }
        }
        if (count) {
            roadmap_canvas_draw_multiple_lines (1, &count, points, 1);
            p = points;
            count = 0;
        }

        /* advance the starting coordinate ... */
        xp += scale * (hf->hc[k].maxx - hf->hc[k].minx) / 1024;

    }
}

/* This string is simply an in-line version of the file "romans",
 * the Roman-Simplex font distributed on Usenet with with Alan
 * Richardson's Xrotfont package.  To inline another such font,
 * simply quote each line, escape all '\' characters, and join
 * lines that are clearly continuations lines.  (The data in
 * this array is used directly by the code above, after some
 * initial pointers are set up into the array.)
 * 
 */
const unsigned char romans_font[] = 
    "  699  1JZ\n"
    "  714  9MWRFRT RRYQZR[SZRY\n"
    "  717  6JZNFNM RVFVM\n"
    "  733 12H]SBLb RYBRb RLOZO RKUYU\n"
    "  719 27H\\PBP_ RTBT_ RYIWGTFPFMGKIKKLMMNOOUQWRXSYUYXWZT[P[MZKX\n"
    " 2271 32F^[FI[ RNFPHPJOLMMKMIKIIJGLFNFPGSHVHYG[F RWTUUTWTYV[X[ZZ[X[VYTWT\n"
    "  734 35E_\\O\\N[MZMYNXPVUTXRZP[L[JZIYHWHUISJRQNRMSKSIRGPFNGMIMKNNPQUXWZY[[[\\Z\\Y\n"
    "  731  8MWRHQGRFSGSIRKQL\n"
    "  721 11KYVBTDRGPKOPOTPYR]T`Vb\n"
    "  722 11KYNBPDRGTKUPUTTYR]P`Nb\n"
    " 2219  9JZRFRR RMIWO RWIMO\n"
    "  725  6E_RIR[ RIR[R\n"
    "  711  9MWSZR[QZRYSZS\\R^Q_\n"
    "  724  3E_IR[R\n"
    "  710  6MWRYQZR[SZRY\n"
    "  720  3G][BIb\n"
    "  700 18H\\QFNGLJKOKRLWNZQ[S[VZXWYRYOXJVGSFQF\n"
    "  701  5H\\NJPISFS[\n"
    "  702 15H\\LKLJMHNGPFTFVGWHXJXLWNUQK[Y[\n"
    "  703 16H\\MFXFRNUNWOXPYSYUXXVZS[P[MZLYKW\n"
    "  704  7H\\UFKTZT RUFU[\n"
    "  705 18H\\WFMFLOMNPMSMVNXPYSYUXXVZS[P[MZLYKW\n"
    "  706 24H\\XIWGTFRFOGMJLOLTMXOZR[S[VZXXYUYTXQVOSNRNOOMQLT\n"
    "  707  6H\\YFO[ RKFYF\n"
    "  708 30H\\PFMGLILKMMONSOVPXRYTYWXYWZT[P[MZLYKWKTLRNPQOUNWMXKXIWGTFPF\n"
    "  709 24H\\XMWPURRSQSNRLPKMKLLINGQFRFUGWIXMXRWWUZR[P[MZLX\n"
    "  712 12MWRMQNROSNRM RRYQZR[SZRY\n"
    "  713 15MWRMQNROSNRM RSZR[QZRYSZS\\R^Q_\n"
    " 2241  4F^ZIJRZ[\n"
    "  726  6E_IO[O RIU[U\n"
    " 2242  4F^JIZRJ[\n"
    "  715 21I[LKLJMHNGPFTFVGWHXJXLWNVORQRT RRYQZR[SZRY\n"
    " 2273 56E`WNVLTKQKOLNMMPMSNUPVSVUUVS RQKOMNPNSOUPV RWKVSVUXVZV\\T]Q]O\\L[JYHWGTFQFNGLHJJILHOHRIUJWLYNZQ[T[WZYYZX RXKWSWUXV\n"
    "  501  9I[RFJ[ RRFZ[ RMTWT\n"
    "  502 24G\\KFK[ RKFTFWGXHYJYLXNWOTP RKPTPWQXRYTYWXYWZT[K[\n"
    "  503 19H]ZKYIWGUFQFOGMILKKNKSLVMXOZQ[U[WZYXZV\n"
    "  504 16G\\KFK[ RKFRFUGWIXKYNYSXVWXUZR[K[\n"
    "  505 12H[LFL[ RLFYF RLPTP RL[Y[\n"
    "  506  9HZLFL[ RLFYF RLPTP\n"
    "  507 23H]ZKYIWGUFQFOGMILKKNKSLVMXOZQ[U[WZYXZVZS RUSZS\n"
    "  508  9G]KFK[ RYFY[ RKPYP\n"
    "  509  3NVRFR[\n"
    "  510 11JZVFVVUYTZR[P[NZMYLVLT\n"
    "  511  9G\\KFK[ RYFKT RPOY[\n"
    "  512  6HYLFL[ RL[X[\n"
    "  513 12F^JFJ[ RJFR[ RZFR[ RZFZ[\n"
    "  514  9G]KFK[ RKFY[ RYFY[\n"
    "  515 22G]PFNGLIKKJNJSKVLXNZP[T[VZXXYVZSZNYKXIVGTFPF\n"
    "  516 14G\\KFK[ RKFTFWGXHYJYMXOWPTQKQ\n"
    "  517 25G]PFNGLIKKJNJSKVLXNZP[T[VZXXYVZSZNYKXIVGTFPF RSWY]\n"
    "  518 17G\\KFK[ RKFTFWGXHYJYLXNWOTPKP RRPY[\n"
    "  519 21H\\YIWGTFPFMGKIKKLMMNOOUQWRXSYUYXWZT[P[MZKX\n"
    "  520  6JZRFR[ RKFYF\n"
    "  521 11G]KFKULXNZQ[S[VZXXYUYF\n"
    "  522  6I[JFR[ RZFR[\n"
    "  523 12F^HFM[ RRFM[ RRFW[ R\\FW[\n"
    "  524  6H\\KFY[ RYFK[\n"
    "  525  7I[JFRPR[ RZFRP\n"
    "  526  9H\\YFK[ RKFYF RK[Y[\n"
    " 2223 12KYOBOb RPBPb ROBVB RObVb\n"
    "  804  3KYKFY^\n"
    " 2224 12KYTBTb RUBUb RNBUB RNbUb\n"
    " 2262 11JZPLRITL RMORJWO RRJR[\n"
    "  999  3JZJ]Z]\n"
    "  730  8MWSFRGQIQKRLSKRJ\n"
    "  601 18I\\XMX[ RXPVNTMQMONMPLSLUMXOZQ[T[VZXX\n"
    "  602 18H[LFL[ RLPNNPMSMUNWPXSXUWXUZS[P[NZLX\n"
    "  603 15I[XPVNTMQMONMPLSLUMXOZQ[T[VZXX\n"
    "  604 18I\\XFX[ RXPVNTMQMONMPLSLUMXOZQ[T[VZXX\n"
    "  605 18I[LSXSXQWOVNTMQMONMPLSLUMXOZQ[T[VZXX\n"
    "  606  9MYWFUFSGRJR[ ROMVM\n"
    "  607 23I\\XMX]W`VaTbQbOa RXPVNTMQMONMPLSLUMXOZQ[T[VZXX\n"
    "  608 11I\\MFM[ RMQPNRMUMWNXQX[\n"
    "  609  9NVQFRGSFREQF RRMR[\n"
    "  610 12MWRFSGTFSERF RSMS^RaPbNb\n"
    "  611  9IZMFM[ RWMMW RQSX[\n"
    "  612  3NVRFR[\n"
    "  613 19CaGMG[ RGQJNLMOMQNRQR[ RRQUNWMZM\\N]Q][\n"
    "  614 11I\\MMM[ RMQPNRMUMWNXQX[\n"
    "  615 18I\\QMONMPLSLUMXOZQ[T[VZXXYUYSXPVNTMQM\n"
    "  616 18H[LMLb RLPNNPMSMUNWPXSXUWXUZS[P[NZLX\n"
    "  617 18I\\XMXb RXPVNTMQMONMPLSLUMXOZQ[T[VZXX\n"
    "  618  9KXOMO[ ROSPPRNTMWM\n"
    "  619 18J[XPWNTMQMNNMPNRPSUTWUXWXXWZT[Q[NZMX\n"
    "  620  9MYRFRWSZU[W[ ROMVM\n"
    "  621 11I\\MMMWNZP[S[UZXW RXMX[\n"
    "  622  6JZLMR[ RXMR[\n"
    "  623 12G]JMN[ RRMN[ RRMV[ RZMV[\n"
    "  624  6J[MMX[ RXMM[\n"
    "  625 10JZLMR[ RXMR[P_NaLbKb\n"
    "  626  9J[XMM[ RMMXM RM[X[\n"
    " 2225 40KYTBRCQDPFPHQJRKSMSOQQ RRCQEQGRISJTLTNSPORSTTVTXSZR[Q]Q_Ra RQSSUSWRYQZP\\P^Q`RaTb\n"
    "  723  3NVRBRb\n"
    " 2226 40KYPBRCSDTFTHSJRKQMQOSQ RRCSESGRIQJPLPNQPURQTPVPXQZR[S]S_Ra RSSQUQWRYSZT\\T^S`RaPb\n\n"
    " 2246 24F^IUISJPLONOPPTSVTXTZS[Q RISJQLPNPPQTTVUXUZT[Q[O\n"
    "  718 14KYQFOGNINKOMQNSNUMVKVIUGSFQF\n";


/* 

original restriction text from mod.sources posting of the fonts follows:

----quote begins---

Mod.sources:  Volume 4, Issue 42
Submitted by: pyramid!octopus!pete (Pete Holzmann)

This is part 1 of five parts of the first Usenet distribution of
the Hershey Fonts. See the README file for more details.

Peter Holzmann, Octopus Enterprises
USPS: 19611 La Mar Court, Cupertino, CA 95014
UUCP: {hplabs!hpdsd,pyramid}!octopus!pete
Phone: 408/996-7746

This distribution is made possible through the collective encouragement
of the Usenet Font Consortium, a mailing list that sprang to life to get
this accomplished and that will now most likely disappear into the mists
of time... Thanks are especially due to Jim Hurt, who provided the packed
font data for the distribution, along with a lot of other help.

This file describes the Hershey Fonts in general, along with a description of
the other files in this distribution and a simple re-distribution restriction.

USE RESTRICTION:
        This distribution of the Hershey Fonts may be used by anyone for
        any purpose, commercial or otherwise, providing that:
                1. The following acknowledgements must be distributed with
                        the font data:
                        - The Hershey Fonts were originally created by Dr.
                                A. V. Hershey while working at the U. S.
                                National Bureau of Standards.
                        - The format of the Font data in this distribution
                                was originally created by
                                        James Hurt
                                        Cognition, Inc.
                                        900 Technology Park Drive
                                        Billerica, MA 01821
                                        (mit-eddie!ci-dandelion!hurt)
                2. The font data in this distribution may be converted into
                        any other format *EXCEPT* the format distributed by
                        the U.S. NTIS (which organization holds the rights
                        to the distribution and use of the font data in that
                        particular format). Not that anybody would really
                        *want* to use their format... each point is described
                        in eight bytes as "xxx yyy:", where xxx and yyy are
                        the coordinate values as ASCII numbers.

----quote ends---

[ the original text continues, but describes files and formats
not present here. -pgf ]

 */

#endif  /* ROADMAP_USE_LINEFONT */
