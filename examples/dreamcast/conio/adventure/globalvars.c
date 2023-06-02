#include "hdr.h"

int datfd;
int yea;

int loc, newloc, oldloc, oldlc2, wzdark, gaveup, kq, k, k2;
char *wd1, *wd2;                        /* the complete words           */
int verb, obj, spk;
int saved, savet, mxscor, latncy;

struct hashtab voc[HTSIZE];

struct text rtext[RTXSIZ];              /* random text messages         */
struct text mtext[MAGSIZ];              /* magic messages               */

int clsses;
struct text ctext[CLSMAX];              /* classes of adventurer        */
int cval[CLSMAX];

struct text ptext[101];                 /* object descriptions          */

#define LOCSIZ  141                     /* number of locations          */
struct text ltext[LOCSIZ];              /* long loc description         */
struct text stext[LOCSIZ];              /* short loc descriptions       */

struct travlist *travel[LOCSIZ], *tkk;            /* travel is closer to keys(...)*/

int atloc[LOCSIZ];

int  plac[101];                         /* initial object placement     */
int  fixd[101], fixed[101];             /* location fixed?              */

int actspk[35];                         /* rtext msg for verb <n>       */

int cond[LOCSIZ];                       /* various condition bits       */               

int hntmax;
int hints[20][5];                       /* info on hints                */
int hinted[20], hintlc[20];

int place[101], prop[101], linkx[201];
int abb[LOCSIZ];

int maxtrs, tally, tally2;              /* treasure values              */

int keys, lamp, grate, cage, rod, rod2, steps, /* mnemonics                    */
    bird, door, pillow, snake, fissur, tablet, clam, oyster, magzin,
    dwarf, knife, food, bottle, water, oil, plant, plant2, axe, mirror, dragon,
    chasm, troll, troll2, bear, messag, vend, batter,
    nugget, coins, chest, eggs, tridnt, vase, emrald, pyram, pearl, rug, chain,
    spices,
    back, look, cave, null, entrnc, dprssn,
    enter, stream, pour,
    say, lock, throw, find, invent;

int chloc, chloc2, dseen[7], dloc[7],   /* dwarf stuff                  */
        odloc[7], dflag, daltlc;

int tk[21], stick, dtotal, attack;
int turns, lmwarn, iwest, knfloc, detail, /* various flags & counters     */
        abbnum, maxdie, numdie, holdng, dkill, foobar, bonus, clock1, clock2,
        closng, apanic, closed, scorng;

int demo, limit;
