
#if 0

SLAB6.C by Ken Silverman (http://advsys.net/ken)

License for this code:
	* No commercial exploitation please
	* Do not remove my name or credit
	* You may distribute modified code/executables but please make it clear that it is modified

#endif

	//TODO:
	// L.Enter: Show-through mode
	//       R: 2D Rotate
	//       U: support multiple KVX's
	// Shift+' ': make it paint only to surface you see, not entire 3D sphere
	//  - Shift+R/Shift+F for 3D box selection
	//  - Reconfigure keys
	//  - optimize molding keys (voxfindspray/voxfindsuck)
	//  - copy&paste should have a mode that doesn't write transparent pixels
	//  - Clean up non-standard axis problems
	//  - Z-buffer
	//  - 32-bit / load KV6
	//  - SLABHI's .SCP (with KPLIB support)
	//  - UNDO :/
	//  - User-defined controls
	//  - Integrate poly2vox
	//  - Menu tool to sort 8-bit palette
	//
	//Maren's suggestions: (http://www.jonof.id.au/forum/index.php?topic=2027.0)
	//(*=fixed)
	//*  1. According to slab6.txt, pressing INSERT alone is enough to paste an image, while in practice that can only be
	//      achieved with SHIFT+INSERT. Not that it takes you more than a few seconds to figure that out, but it wouldn't
	//      hurt to correct that line.
	//   2. The brush radius for the slice editor is limited to either a single voxel or flood fill or its dangerous
	//      variant. It would be handy to be able to increase the radius of the brush in order to paint larger (but
	//      specific) areas more efficiently.
	//   3. An optional flood fill mode that fills all the air areas on the slice at once rather than just the one under
	//      the cursor. Since dangerous flood fill will destroy everything, it's not an option here.
	//   4. Allow the 3d selection tool to be positioned (kind of like shift+kp) without actually removing voxels from the
	//      model.
	//   5. Show the boundaries of the grid ala KubeGL.
	//   6. When working with massive voxels, rendering may become a lil sluggish, so I was wondering if being able to
	//      manually disable the 3d view while working on the slice editor would be an acceptable/feasible/practical idea.
	//   7. The slice editor reports the maximum dimension of the axis right after the "/", let's make it 64 for the
	//      example, yet the slices will be numbered from 0 to 63 rather than from 1 to 64, resulting in "63/64" when
	//      positioning the selector on the latest slice. I'm not saying there's anything wrong with this, but I'd be
	//      interested in reading the explanation for what seems to be an unnecessary inconsistency to me.
	//   8. Resize the grid to accommodate pasted images larger than the actual grid.
	//   9. Using Shift+SPACE to paint a model when rotating it results in slow rotation, which can cost you a considerable
	//      amount of time depending on the size of the surface. Can this behavior be improved?
	//  10. Building/carving slopes with any degree of precision is actually more complicated than it sounds. How about
	//      allowing the 3D selection tool to lower one of its sides so to create a sloped selection which should retain
	//      the capabilities of the regular "cubic" selection?
	//  11. Deep painting. Imagine you start painting on a slice and have either all the other slices or selectively those
	//      from one side or the other, painted as well. This would definitely be a big time saver. You could, for example,
	//      build a whole cylinder by just painting a circle on a single slice.
	//  12. Axis rotations through numerical input. Say I wanted to rotate a model exactly 45 (or -45) degrees on the X
	//      axis; I'd type in the number and presto! This would be extremely handy for creating sprites.
	//  13. Saving paletted screenshots with dimensions cropped down to those of the actual voxel model.
	//  14. Model stretching ala SLABSPRI.
	//  15. A tool for identifying unused colors.
	//  16. A voxel counter. This is to make it easier to establish a link between the amount of voxels and the framerate.
	//  17. Allow slab6 to keep more than just one image in the memory.
	//  18. Shortcuts for doubling and halving dimensions.
	//  19. 64 values for RGB? bring back the 255! Now seriously, why is this?
	//  20. Even though one of the voxel dimensions is limited to a maximum of 255, you're still able to select the 256th
	//      slice of the grid with the 3d selector, but any attempt to insert an extra voxel with this tool (not that you
	//      should be doing this) will cause slab to crash. Deleting big chunks from a 256x255x256 cube using the 3d
	//      selector will also cause slab to crash, but I guess it's just part of the instability inherent to working with
	//      voxel models bigger than the editor was intended to handle comfortably, especially on my ancient computer.
	//  21. Create a voxel model from a strip of properly numbered images resized and paletted to fit in slab6 as slices.
	//      You may choose which axis to align them to. Now that would be great.
	//  22. A variant of the "C" tool for changing colors that works just like the flood tool, and by this I mean, change
	//      ONLY the colors of the same-colored voxel cluster under the cursor, which would be akin to filling only the
	//      "air" area under the cursor using "F" in the slice editor.

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <math.h>
#include <direct.h>

#define OPTIMIZEFOR 6

#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>

#if !defined(max)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min)
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MAX_PATH 	2048

typedef struct tagPALETTEENTRY {
  unsigned char peRed;
  unsigned char peGreen;
  unsigned char peBlue;
  unsigned char peFlags;
} PALETTEENTRY, *PPALETTEENTRY, *LPPALETTEENTRY;

PALETTEENTRY pal[256];

#define MAXXDIM 2048
#define MAXYDIM 1536
#define PI 3.141592653589793

#define MAXXSIZ 256 //Default values
#define MAXYSIZ 256
#define BUFZSIZ 256 //(BUFZSIZ&7) MUST == 0
#define LIMZSIZ 255 //Limited to 255 by: char ylen[?][?]
//#define MAXXSIZ 1420  //REGFILE.VOX values:
//#define MAXYSIZ 1024
//#define BUFZSIZ 8
//#define LIMZSIZ 8

#define MAXVOXMIPS 5
#define CULLBYDIR 0   //1:renderboundcube faster, but thin parts get omitted
#define NORMBOXSIZ 3

typedef struct { float x, y, z; } point3d;
typedef struct { long x, y, z; } lpoint3d;
typedef struct { long f, p, x, y; } tiletype;

	//Orthonormal position vectors
point3d ipos, ifor, irig, idow;
point3d oirig, oidow, lightpos;
float mouseymul = .008, lightmul = 32.0;
long lightadd = 32;

	//Screen pre-calculation variables
#ifdef __cplusplus
extern "C" {
#endif
tiletype gdd;
long lcol[256];
#ifdef __cplusplus
}
#endif
tiletype ogdd = {0,0,0,0};

float hx, hy, hz, hds, sphk1, sphk2, sphk3;
long ihx, ihy;
long ylookup[MAXYDIM+1], lastx[MAXYDIM], slcol[3][256], pag;
char whitecol, graycol, blackcol, backgroundcol = 0;
long backgroundcolrgb18 = 0x18618;
long curcol = -1, curgamminterp = 1000;
static char curfilnam[MAX_PATH];

extern long ddrawuseemulation;

	//KVX loading variables
char fipalette[768];
long numbytes, xsiz, ysiz, zsiz, xstart[MAXXSIZ+1];
unsigned short xyoffs[MAXXSIZ][MAXYSIZ+1];

	//Unfortunately, KVX's don't have surface visibility set up correctly
	//The voxel must be de-compiled and filled to re-calculate visibilities.
static long vbit[(MAXXSIZ*MAXYSIZ*BUFZSIZ)>>5];
static long vbit2[(MAXXSIZ*MAXYSIZ*BUFZSIZ)>>(5+3)]; //temp array for saving lower mips of .KVX files

static char palgroup[256], palookup[64][256];

	//SLAB6 voxel file format: (siz = 4+24+xsiz*ysiz+numvoxs*2)
	//SLAB6 voxel memory format: (siz = 24+4+xsiz*2+xsiz*ysiz+numvoxs*4)
#define MAXVOXS 1048576  //numvoxs = 993756 for regfile.vox!
typedef struct { char z, col, vis, dir; } voxtype;
float xpiv, ypiv, zpiv;
long numvoxs; //sum of xlens and number of surface voxels
unsigned short xlen[MAXXSIZ];
char ylen[MAXXSIZ][MAXYSIZ];
voxtype voxdata[MAXVOXS];

	//2D MODE variables:
	//Window 0: (xsiz,ysiz)
	//Window 1: (xsiz,zsiz)
	//Window 2: (ysiz,zsiz)
	//Window 3: colorbar
long wx0[4], wy0[4], wx1[4], wy1[4], wz[4], wndorder[4], numwindows = 4;
long viewx = 0, viewy = 0, viewz = 0, voxindex = -1, inawindow = -1;
char edit2dmode = 0, adjustpivotmode = 0, rotatemode = 255;
long grabwindow = -1;

	//Copy&paste (2D slice editor)
long copx0, copy0, copx1, copy1, copshiftmode = 0, copwindow = -1;
long copbuf[max(MAXXSIZ,BUFZSIZ)][max(MAXYSIZ,BUFZSIZ)];

	//Copy&paste (3D using RMB)
lpoint3d corn[2];
long cornmode = 0, corngrab = 0;

	//Misc editor stuff:
long currad = 4, sprayhz = 512;
double spraytimer = 0;

float sx[8], sy[8], spherad2 = .75;
long ptface[6][4] = {0,2,6,4, 5,7,3,1, 5,4,6,7, 0,1,3,2, 0,4,5,1, 2,3,7,6};
unsigned char pow2char[8] = {1,2,4,8,16,32,64,128};
unsigned char pow2mask[8] = {254,253,251,247,239,223,191,127};
long drawmode = 2, reflectmode = -1, drawoption = 0;
point3d ztab[BUFZSIZ], cadd[8];

	//UNION OF FACES CUBE RENDERING TABLES:
	//(Given (char vis), 1st # says # of faces in list.  Last byte is filler.
	//lo 6 bits tell which faces are visible
char ptfaces[43][8] =
{
	0,0,0,0,0,0,0,0, 4,0,2,6,4,0,0,0, 4,1,5,7,3,0,0,0, 0,0,0,0,0,0,0,0,
	4,4,6,7,5,0,0,0, 6,0,2,6,7,5,4,0, 6,1,5,4,6,7,3,0, 0,0,0,0,0,0,0,0,
	4,0,1,3,2,0,0,0, 6,0,1,3,2,6,4,0, 6,0,1,5,7,3,2,0, 0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
	4,0,4,5,1,0,0,0, 6,0,2,6,4,5,1,0, 6,0,4,5,7,3,1,0, 0,0,0,0,0,0,0,0,
	6,0,4,6,7,5,1,0, 6,0,2,6,7,5,1,0, 6,0,4,6,7,3,1,0, 0,0,0,0,0,0,0,0,
	6,0,4,5,1,3,2,0, 6,1,3,2,6,4,5,0, 6,0,4,5,7,3,2,0, 0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
	4,2,3,7,6,0,0,0, 6,0,2,3,7,6,4,0, 6,1,5,7,6,2,3,0, 0,0,0,0,0,0,0,0,
	6,2,3,7,5,4,6,0, 6,0,2,3,7,5,4,0, 6,1,5,4,6,2,3,0, 0,0,0,0,0,0,0,0,
	6,0,1,3,7,6,2,0, 6,0,1,3,7,6,4,0, 6,0,1,5,7,6,2,0,
};

#if (CULLBYDIR)
char isvis[256];
#endif
long shadeoff[256];

	//FPS variables
#define AVERAGEFRAMES 32
double odtotclk, dtotclk, fsynctics, clsyncticsms = 0.0, averagefps, frameval[AVERAGEFRAMES];
long lsyncticsms = 0;
long framecnt = 0;

	//User message:
char message[256] = {0};
double messagetimeout = 0.0;

static long keyrepeat (long scancode)
{
	long i;

	// if (!keystatus[scancode]) return(0);
	// if (keystatus[scancode] == 1) { i = 1; keystatus[scancode] = 2;/*Avoid multiple hit when fps >= 1000fps*/  }
	// else if (keystatus[scancode] == 255) { i = 1; keystatus[scancode] = 222; }
	// else i = 0;
	// keystatus[scancode] = min(keystatus[scancode]+lsyncticsms,255);
	return(i);
}

//�����������������������������������������Ŀ
//� Rounding: D11-10:  � Precision: D9-8:   �
//�����������������������������������������Ĵ
//� 00 = near/even (0) � 00 = 24-bit    (0) �
//� 01 = -inf      (4) � 01 = reserved  (1) �
//� 10 = +inf      (8) � 10 = 53-bit    (2) �
//� 11 = 0         (c) � 11 = 64-bit    (3) �
//�������������������������������������������

long fpumode;
long fstcw ();
#ifdef _MSC_VER

#if (OPTIMIZEFOR == 6)

	//Better for Pentium Pros (with Fastvid)
static _inline void clearbufbyte (void *d, long c, long a)
{
	_asm
	{
		mov edi, d
		mov ecx, c
		mov eax, a
		test ecx, ecx
		js short skipit
		rep stosb
skipit:
	}
}

#else

	//Better for Pentiums
static _inline void clearbufbyte (void *d, long c, long a)
{
	_asm
	{
		mov edi, d
		mov ecx, c
		mov eax, a
		lea ecx, [edi+edi*2]
		and ecx, 3
		sub edx, ecx
		jle short skip1
		rep stosb
		mov ecx, edx
		and edx, 3
		shr ecx, 2
		rep stosd
skip1: add ecx, edx
		jl short skip2
		rep stosb
skip2:
	}
}

#endif

static _inline long fstcw ()
{
	long fpumode;
	_asm fstcw fpumode
	return(fpumode);
}

static _inline void fldcw (long fpumode)
{
	_asm fldcw fpumode
}

static _inline void ftol (float f, long *a)
{
	_asm
	{
		mov eax, a
		fld f
		fistp dword ptr [eax]
	}
}

static _inline void fcossin (float a, float *c, float *s)
{
	_asm
	{
		fld a
		fsincos
		mov eax, c
		fstp dword ptr [eax]
		mov eax, s
		fstp dword ptr [eax]
	}
}

#endif

#define LOGFSQSIZ 10
long fsqlookup[1<<LOGFSQSIZ];
void initfsqrtasm ()
{
	long i, j;
	float f, s1, s2;

	s1 = 16777216 / sqrt(1<<LOGFSQSIZ); s2 = s1*sqrt(0.5);
	for(i=(1<<(LOGFSQSIZ-1));i<(1<<LOGFSQSIZ);i++)
	{
		f = sqrt((float)i); j = (i<<(23-LOGFSQSIZ));
		fsqlookup[i-(1<<(LOGFSQSIZ-1))] = (long)(f*s1)-j+0x1f400000;
		fsqlookup[i                   ] = (long)(f*s2)-j+0x1f800000;
	}
}
#define fsqrtasm(i,o)\
	 ((*(long *)o) =\
	 ((*(unsigned long *)i)>>1)+\
	 fsqlookup[((*(long *)i)&(((1<<LOGFSQSIZ)-1)<<(24-LOGFSQSIZ)))>>(24-LOGFSQSIZ)])\

#define LOGFRECIPSIZ 8
long freciplookup[1<<LOGFRECIPSIZ];
void initfrecipasm ()
{
	long i;
	for(i=0;i<(1<<LOGFRECIPSIZ);i++)
	{
		freciplookup[i] = ((1<<LOGFRECIPSIZ)*16777216.0) / ((float)i+(1<<LOGFRECIPSIZ));
		freciplookup[i] += (i<<(23-LOGFRECIPSIZ))+0x7e000000;
	}
}
#define frecipasm(i,o)\
	((*(long *)o) = freciplookup[((*(long *)i)&0x007fffff)>>(23-LOGFRECIPSIZ)]-(*(long *)i))

#ifdef _MSC_VER

static _inline void qinterpolatedown16 (long *a, long c, long d, long s)
{
	_asm
	{
		mov eax, a
		mov ecx, c
		mov edx, d
		mov esi, s

		mov ebx, ecx
		shr ecx, 1
		jz short skipbegcalc
		begqcalc: lea edi, [edx+esi]
		sar edx, 16
		mov dword ptr [eax], edx
		lea edx, [edi+esi]
		sar edi, 16
		mov dword ptr [eax+4], edi
		add eax, 8
		dec ecx
		jnz short begqcalc
		test ebx, 1
		jz short skipbegqcalc2
skipbegcalc: sar edx, 16
		mov dword ptr [eax], edx
skipbegqcalc2:
	}
}

static _inline void qinterpolatehiadd16 (long *a, long c, long d, long s)
{
	_asm
	{
		mov eax, a
		mov ecx, c
		mov edx, d
		mov esi, s

		mov ebx, ecx
		shr ecx, 1
		jz short skipbegcalc
 begqcalc: lea edi, [edx+esi]
		and edx, 0xffff0000
		add dword ptr [eax], edx
		lea edx, [edi+esi]
		and edi, 0xffff0000
		add dword ptr [eax+4], edi
		add eax, 8
		dec ecx
		jnz short begqcalc
		test ebx, 1
		jz short skipbegqcalc2
skipbegcalc: and edx, 0xffff0000
		add dword ptr [eax], edx
skipbegqcalc2:
	}
}

#endif

	//NOTE: font is stored vertically first! (like .ART files)
static const unsigned long long font6x8[] = //256 DOS chars, from: DOSAPP.FON (tab blank)
{
	0x3E00000000000000,0x6F6B3E003E455145,0x1C3E7C3E1C003E6B,0x3000183C7E3C1800,
	0x7E5C180030367F36,0x000018180000185C,0x0000FFFFE7E7FFFF,0xDBDBC3FF00000000,
	0x0E364A483000FFC3,0x6000062979290600,0x0A7E600004023F70,0x2A1C361C2A003F35,
	0x0800081C3E7F0000,0x7F361400007F3E1C,0x005F005F00001436,0x22007F017F090600,
	0x606060002259554D,0x14B6FFB614000060,0x100004067F060400,0x3E08080010307F30,
	0x08083E1C0800081C,0x0800404040407800,0x3F3C3000083E083E,0x030F3F0F0300303C,
	0x0000000000000000,0x0003070000065F06,0x247E247E24000307,0x630000126A2B2400,
	0x5649360063640813,0x0000030700005020,0x00000000413E0000,0x1C3E080000003E41,
	0x08083E080800083E,0x0800000060E00000,0x6060000008080808,0x0204081020000000,
	0x00003E4549513E00,0x4951620000407F42,0x3649494922004649,0x2F00107F12141800,
	0x494A3C0031494949,0x0305097101003049,0x0600364949493600,0x6C6C00001E294949,
	0x00006CEC00000000,0x2400004122140800,0x2241000024242424,0x0609590102000814,
	0x7E001E555D413E00,0x49497F007E111111,0x224141413E003649,0x7F003E4141417F00,
	0x09097F0041494949,0x7A4949413E000109,0x00007F0808087F00,0x4040300000417F41,
	0x412214087F003F40,0x7F00404040407F00,0x04027F007F020402,0x3E4141413E007F08,
	0x3E00060909097F00,0x09097F005E215141,0x3249494926006619,0x3F0001017F010100,
	0x40201F003F404040,0x3F403C403F001F20,0x0700631408146300,0x4549710007087008,
	0x0041417F00000043,0x0000201008040200,0x01020400007F4141,0x8080808080800402,
	0x2000000007030000,0x44447F0078545454,0x2844444438003844,0x38007F4444443800,
	0x097E080008545454,0x7CA4A4A418000009,0x0000007804047F00,0x8480400000407D00,
	0x004428107F00007D,0x7C0000407F000000,0x04047C0078041804,0x3844444438000078,
	0x380038444444FC00,0x44784400FC444444,0x2054545408000804,0x3C000024443E0400,
	0x40201C00007C2040,0x3C6030603C001C20,0x9C00006C10106C00,0x54546400003C60A0,
	0x0041413E0800004C,0x0000000077000000,0x02010200083E4141,0x3C2623263C000001,
	0x3D001221E1A11E00,0x54543800007D2040,0x7855555520000955,0x2000785554552000,
	0x5557200078545555,0x1422E2A21C007857,0x3800085555553800,0x5555380008555455,
	0x00417C0100000854,0x0000004279020000,0x2429700000407C01,0x782F252F78007029,
	0x3400455554547C00,0x7F097E0058547C54,0x0039454538004949,0x3900003944453800,
	0x21413C0000384445,0x007C20413D00007D,0x3D00003D60A19C00,0x40413C00003D4242,
	0x002466241800003D,0x29006249493E4800,0x16097F00292A7C2A,0x02097E8840001078,
	0x0000785555542000,0x4544380000417D00,0x007D21403C000039,0x7A0000710A097A00,
	0x5555080000792211,0x004E51514E005E55,0x3C0020404D483000,0x0404040404040404,
	0x506A4C0817001C04,0x0000782A34081700,0x0014080000307D30,0x0814000814001408,
	0x55AA114411441144,0xEEBBEEBB55AA55AA,0x0000FF000000EEBB,0x0A0A0000FF080808,
	0xFF00FF080000FF0A,0x0000F808F8080000,0xFB0A0000FE0A0A0A,0xFF00FF000000FF00,
	0x0000FE02FA0A0000,0x0F0800000F080B0A,0x0F0A0A0A00000F08,0x0000F80808080000,
	0x080808080F000000,0xF808080808080F08,0x0808FF0000000808,0x0808080808080808,
	0xFF0000000808FF08,0x0808FF00FF000A0A,0xFE000A0A0B080F00,0x0B080B0A0A0AFA02,
	0x0A0AFA02FA0A0A0A,0x0A0A0A0AFB00FF00,0xFB00FB0A0A0A0A0A,0x0A0A0B0A0A0A0A0A,
	0x0A0A08080F080F08,0xF808F8080A0AFA0A,0x08080F080F000808,0x00000A0A0F000000,
	0xF808F8000A0AFE00,0x0808FF00FF080808,0x08080A0AFB0A0A0A,0xF800000000000F08,
	0xFFFFFFFFFFFF0808,0xFFFFF0F0F0F0F0F0,0xFF000000000000FF,0x0F0F0F0F0F0FFFFF,
	0xFE00241824241800,0x01017F0000344A4A,0x027E027E02000003,0x1800006349556300,
	0x2020FC00041C2424,0x000478040800001C,0x3E00085577550800,0x02724C00003E4949,
	0x0030595522004C72,0x1800182418241800,0x2A2A1C0018247E24,0x003C02023C00002A,
	0x0000002A2A2A2A00,0x4A4A510000242E24,0x00514A4A44000044,0x20000402FC000000,
	0x2A08080000003F40,0x0012241224000808,0x0000000609090600,0x0008000000001818,
	0x02023E4030000000,0x0900000E010E0100,0x3C3C3C0000000A0D,0x000000000000003C,
};
static void print6x8 (long ox, long y, long fcol, long bcol, const char *fmt, ...)
{
	va_list arglist;
	char st[1024], *c, *v, *cp, *cpx;
	long i, j, ie, x;

	if (!fmt) return;
	va_start(arglist,fmt);
	if (_vsnprintf((char *)&st,sizeof(st)-1,fmt,arglist)) st[sizeof(st)-1] = 0;
	va_end(arglist);

	cp = (char *)(y*gdd.p+gdd.f);
	for(j=1;j<256;y++,cp=(char *)(((long)cp)+gdd.p),j+=j)
		if ((unsigned)y < (unsigned)gdd.y)
			for(c=st,x=ox;*c;c++,x+=6)
			{
				v = (char *)(((long)*c)*6 + (long)font6x8);
				ie = min(gdd.x-x,6); cpx = &cp[x];
				for(i=max(-x,0);i<ie;i++) { if (v[i]&j) cpx[i] = fcol; else if (bcol >= 0) cpx[i] = bcol; }
				if ((*c) == 9) { if (bcol >= 0) for(i=6;i<18;i++) cpx[i] = bcol; x += 2*6; }
			}
}

//EQUIVEC code begins -----------------------------------------------------

#define GOLDRAT 0.3819660112501052 //Golden Ratio: 1 - 1/((sqrt(5)+1)/2)
typedef struct
{
	float fibx[45], fiby[45];
	float azval[20], zmulk, zaddk;
	long fib[47], aztop, npoints;
	point3d *p;  //For memory version :/
	long pcur;
} equivectyp;
static equivectyp equivec;

#ifdef _MSC_VER
static _inline long dmulshr0 (long a, long d, long s, long t)
{
	_asm
	{
		mov eax, a
		imul d
		mov ecx, eax
		mov eax, s
		imul t
		add eax, ecx
	}
}
#endif

void equiind2vec (long i, float *x, float *y, float *z)
{
	float r;
	(*z) = (float)i*equivec.zmulk + equivec.zaddk; r = sqrt(1.f - (*z)*(*z));
	fcossin((float)i*(GOLDRAT*PI*2),x,y); (*x) *= r; (*y) *= r;
}

	//(Pass n=0 to free buffer)
long equimemset (long n)
{
	long z;

	if (equivec.pcur == n) return(1); //Don't recalculate if same #
	if (equivec.p) { free(equivec.p); equivec.p = 0; }
	if (!n) return(1);

		//Init vector positions (equivec.p) for memory version
	if (!(equivec.p = (point3d *)malloc(((n+1)&~1)*sizeof(point3d))))
		return(0);

	equivec.pcur = n;
	equivec.zmulk = 2 / (float)n; equivec.zaddk = equivec.zmulk*.5 - 1.0;
	for(z=n-1;z>=0;z--)
		equiind2vec(z,&equivec.p[z].x,&equivec.p[z].y,&equivec.p[z].z);
	if (n&1) //Hack for when n=255 and want a <0,0,0> vector
		{ equivec.p[n].x = equivec.p[n].y = equivec.p[n].z = 0; }
	return(1);
}

	//Very fast; good quality, requires equivec.p[] :/
long equivec2indmem (float x, float y, float z)
{
	long b, i, j, k, bestc;
	float xy, zz, md, d;

	xy = atan2(y,x); //atan2 is 150 clock cycles!
	j = ((*(long *)&z)&0x7fffffff);
	bestc = equivec.aztop;
	do
	{
		if (j < *(long *)&equivec.azval[bestc]) break;
		bestc--;
	} while (bestc);

	zz = z + 1.f;
	ftol(equivec.fibx[bestc]*xy + equivec.fiby[bestc]*zz - .5,&i);
	bestc++;
	ftol(equivec.fibx[bestc]*xy + equivec.fiby[bestc]*zz - .5,&j);

	k = dmulshr0(equivec.fib[bestc+2],i,equivec.fib[bestc+1],j);
	if ((unsigned long)k < equivec.npoints)
	{
		md = equivec.p[k].x*x + equivec.p[k].y*y + equivec.p[k].z*z;
		j = k;
	} else md = -2.f;
	b = bestc+3;
	do
	{
		i = equivec.fib[b] + k;
		if ((unsigned long)i < equivec.npoints)
		{
			d = equivec.p[i].x*x + equivec.p[i].y*y + equivec.p[i].z*z;
			if (*(long *)&d > *(long *)&md) { md = d; j = i; }
		}
		b--;
	} while (b != bestc);
	return(j);
}

void equivecinit (long n)
{
	float t0, t1;
	long z;

		//Init constants for ind2vec
	equivec.npoints = n;
	equivec.zmulk = 2 / (float)n; equivec.zaddk = equivec.zmulk*.5 - 1.0;

	equimemset(n);

		//Init Fibonacci table
	equivec.fib[0] = 0; equivec.fib[1] = 1;
	for(z=2;z<47;z++) equivec.fib[z] = equivec.fib[z-2]+equivec.fib[z-1];

		//Init fibx/y LUT
	t0 = .5 / PI; t1 = (float)n * -.5;
	for(z=0;z<45;z++)
	{
		t0 = -t0; equivec.fibx[z] = (float)equivec.fib[z+2]*t0;
		t1 = -t1; equivec.fiby[z] = ((float)equivec.fib[z+2]*GOLDRAT - (float)equivec.fib[z])*t1;
	}

	t0 = 1 / ((float)n * PI);
	for(equivec.aztop=0;equivec.aztop<20;equivec.aztop++)
	{
		t1 = 1 - (float)equivec.fib[(equivec.aztop<<1)+6]*t0; if (t1 < 0) break;
		equivec.azval[equivec.aztop+1] = sqrt(t1);
	}
}

void equivecuninit () { equimemset(0); }

//EQUIVEC code ends -------------------------------------------------------

static char closestcol[262144];
void initclosestcolorfast (char *dapal)  //Fast approximate octahedron method
{
	long i, j, *clospos, *closend, *closcan;

	if (!(clospos = closend = closcan = (long *)malloc(262144*4)))
		{ printf("Out of memory!\n"); exit(0); }
	memset(closestcol,-1,sizeof(closestcol));
	for(j=0;j<255;j++)
	{
		i = (((long)dapal[j*3])<<12)+(((long)dapal[j*3+1])<<6)+((long)dapal[j*3+2]);
		if (closestcol[i] == 255) { *closend++ = i; closestcol[i] = j; }
	}
	do
	{
		i = *clospos++; j = closestcol[i];
		if ((i&0x3f000)          && (closestcol[i-4096] == 255)) { closestcol[i-4096] = j; *closend++ = i-4096; }
		if ((i < 0x3f000)        && (closestcol[i+4096] == 255)) { closestcol[i+4096] = j; *closend++ = i+4096; }
		if ((i&0xfc0)            && (closestcol[i-  64] == 255)) { closestcol[i-  64] = j; *closend++ = i-  64; }
		if (((i&0xfc0) != 0xfc0) && (closestcol[i+  64] == 255)) { closestcol[i+  64] = j; *closend++ = i+  64; }
		if ((i&0x3f)             && (closestcol[i-   1] == 255)) { closestcol[i-   1] = j; *closend++ = i-   1; }
		if (((i&0x3f) != 0x3f)   && (closestcol[i+   1] == 255)) { closestcol[i+   1] = j; *closend++ = i+   1; }
	} while (clospos != closend);
	free(closcan);
}

void renderdots ()
{
	voxtype *xv, *yv, *v0, *v1;
	point3d c, r0, r1, r2, *p, irhz, idhz;
	float f;
	long i, j, k, x, y, z, inz, splitx, splity, splity2;

	irhz.x = irig.x*hz; irhz.y = irig.y*hz; irhz.z = irig.z*hz;
	idhz.x = idow.x*hz; idhz.y = idow.y*hz; idhz.z = idow.z*hz;
	ftol( ipos.x+xpiv-.5,&x);
	ftol(-ipos.z+ypiv-.5,&y);
	ftol( ipos.y+zpiv-.5,&inz);
	splitx = min(max(x,0),xsiz);
	splity = min(max(y,0),ysiz);
	splity2 = min(max(y,-1),ysiz-1);
	c.x = -xpiv-ipos.x;
	c.y = +ypiv-ipos.z;
	c.z = -zpiv-ipos.y;
	for(i=1;i<zsiz;i++)
	{
		ztab[i].x = ztab[i-1].x+irhz.y;
		ztab[i].y = ztab[i-1].y+idhz.y;
		ztab[i].z = ztab[i-1].z+ifor.y;
	}
	r2.x = -irhz.z*(float)ysiz;
	r2.y = -idhz.z*(float)ysiz;
	r2.z = -ifor.z*(float)ysiz;

	xv = voxdata;

	r1.x = irhz.x*c.x+irhz.z*c.y+irhz.y*c.z;
	r1.y = idhz.x*c.x+idhz.z*c.y+idhz.y*c.z;
	r1.z = ifor.x*c.x+ifor.z*c.y+ifor.y*c.z;
	for(x=0;x<splitx;x++)
	{
		yv = xv+xlen[x]; r0 = r1;
		for(y=0;y<splity;y++)
		{
			v0 = xv; xv += ylen[x][y]; v1 = xv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+4+16))
				{
					p = &ztab[v0->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					//f = 1.0/f;
					frecipasm(&f,&f);
					//f = hz/f;
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v0->dir]][v0->col];
				}
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+4+32))
				{
					p = &ztab[v1->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v1->dir]][v1->col];
				}
			r0.x -= irhz.z; r0.y -= idhz.z; r0.z -= ifor.z;
		}
		xv = yv;
		r0.x = r2.x+r1.x; r1.x += irhz.x;
		r0.y = r2.y+r1.y; r1.y += idhz.x;
		r0.z = r2.z+r1.z; r1.z += ifor.x;
		for(y=ysiz-1;y>=splity;y--)
		{
			r0.x += irhz.z; r0.y += idhz.z; r0.z += ifor.z;
			v1 = yv-1; yv -= ylen[x][y]; v0 = yv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+8+16))
				{
					p = &ztab[v0->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v0->dir]][v0->col];
				}
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+8+32))
				{
					p = &ztab[v1->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v1->dir]][v1->col];
				}
		}
	}

	xv = &voxdata[numvoxs]; f = (float)xsiz+c.x;
	r1.x = irhz.x*f+irhz.z*c.y+irhz.y*c.z;
	r1.y = idhz.x*f+idhz.z*c.y+idhz.y*c.z;
	r1.z = ifor.x*f+ifor.z*c.y+ifor.y*c.z;
	for(x=xsiz-1;x>=splitx;x--)
	{
		yv = xv-xlen[x];
		r1.x -= irhz.x; r0.x = r1.x+r2.x;
		r1.y -= idhz.x; r0.y = r1.y+r2.y;
		r1.z -= ifor.x; r0.z = r1.z+r2.z;
		for(y=ysiz-1;y>splity2;y--)
		{
			r0.x += irhz.z; r0.y += idhz.z; r0.z += ifor.z;
			v1 = xv-1; xv -= ylen[x][y]; v0 = xv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+8+16))
				{
					p = &ztab[v0->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v0->dir]][v0->col];
				}
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+8+32))
				{
					p = &ztab[v1->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v1->dir]][v1->col];
				}
		}
		xv = yv; r0 = r1;
		for(y=0;y<=splity2;y++)
		{
			v0 = yv; yv += ylen[x][y]; v1 = yv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+4+16))
				{
					p = &ztab[v0->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v0->dir]][v0->col];
				}
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+4+32))
				{
					p = &ztab[v1->z];
					f = p->z+r0.z; if (*(long *)&f <= 0x3e000000) continue; //<=.125
					frecipasm(&f,&f);
					ftol((p->y+r0.y)*f+hy,&j); if ((unsigned long)j >= (unsigned long)gdd.y) continue;
					ftol((p->x+r0.x)*f+hx,&i); if ((unsigned long)i >= (unsigned long)gdd.x) continue;
					*(char *)(gdd.f+ylookup[j]+i) = palookup[shadeoff[v1->dir]][v1->col];
				}
			r0.x -= irhz.z; r0.y -= idhz.z; r0.z -= ifor.z;
		}
	}
}

#define HLINMAX 256
long minx0[HLINMAX], maxx0[HLINMAX];
long minx1[HLINMAX], maxx1[HLINMAX];
long minx2[HLINMAX], maxx2[HLINMAX];
#define ORTHMAX 4096
typedef struct { long x, y, p, v; } lpoint4d;
lpoint4d ortho[ORTHMAX], *orthead[4*4+1];

void fillparalbox (float ox, float oy, float ax, float ay, float bx, float by,
						 float r, long *bx0, long *bx1, long *ymin, long *ymax)
{
	float px[4], py[4], npx[8], npy[8], slop;
	long i, ilef, itop, y0, y1, y2, y3, x, xi;

	px[0] = ox; px[1] = ox+ax; px[2] = px[1]+bx; px[3] = ox+bx;
	py[0] = oy; py[1] = oy+ay; py[2] = py[1]+by; py[3] = oy+by;

	ilef = (((*(unsigned long *)&px[1]) - (*(unsigned long *)&px[0]))>>31);
	itop = (((*(unsigned long *)&py[1]) - (*(unsigned long *)&py[0]))>>31);
	i = (((*(unsigned long *)&px[3]) - (*(unsigned long *)&px[2]))>>31)+2;
	ilef += ((((*(long *)&px[i])-(*(long *)&px[ilef]))>>31) & (i-ilef));
	i = (((*(unsigned long *)&py[3]) - (*(unsigned long *)&py[2]))>>31)+2;
	itop += ((((*(long *)&py[i])-(*(long *)&py[itop]))>>31) & (i-itop));

	npx[3] = px[itop]-r; npy[3] = py[itop]-r;
	npx[4] = px[itop]+r; npy[4] = py[itop]-r; i = (itop^2);
	npx[0] = px[i   ]-r; npy[0] = py[i   ]+r;
	npx[7] = px[i   ]+r; npy[7] = py[i   ]+r;
	if ((itop^ilef)&1)
	{
		npx[1] = px[ilef]-r; npy[1] = py[ilef]+r;
		npx[2] = px[ilef]-r; npy[2] = py[ilef]-r; ilef ^= 2;
		npx[5] = px[ilef]+r; npy[5] = py[ilef]-r;
		npx[6] = px[ilef]+r; npy[6] = py[ilef]+r;
	}
	else
	{
		i = ((itop+1)&3);
		i ^= ((-((py[i^2]-py[itop])*(px[i]-px[itop]) <
					(px[i^2]-px[itop])*(py[i]-py[itop])))&2);
		if (itop != ilef)
		{
			npx[5] = px[itop  ]+r; npy[5] = py[itop  ]+r;
			npx[6] = px[i     ]+r; npy[6] = py[i     ]+r;
			npx[1] = px[itop^2]-r; npy[1] = py[itop^2]-r;
			npx[2] = px[i^2   ]-r; npy[2] = py[i^2   ]-r;
		}
		else
		{
			npx[5] = px[i     ]+r; npy[5] = py[i     ]-r;
			npx[6] = px[itop^2]+r; npy[6] = py[itop^2]-r;
			npx[1] = px[i^2   ]-r; npy[1] = py[i^2   ]+r;
			npx[2] = px[itop  ]-r; npy[2] = py[itop  ]+r;
		}
	}

	for(i=0;i<3;i++)
	{
		y0 = (long)(npy[i+1])+1; if (y0 < 0) y0 = 0;
		y2 = (long)(npy[i+4])+1; if (y2 < 0) y2 = 0;
		y1 = (long)(npy[i+0]); if (y1 >= HLINMAX) y1 = HLINMAX-1;
		y3 = (long)(npy[i+5]); if (y3 >= HLINMAX) y3 = HLINMAX-1;
		if ((y0 <= y1) || (y2 <= y3))
		{
			slop = (npx[i+1]-npx[i])*4096.0f / (npy[i+1]-npy[i]);
			xi = (long)slop;
		}
		if (y0 <= y1)
		{
			x = (long)((y0-npy[i+0])*slop + npx[i+0]*4096);
			for(;y0<=y1;y0++) { bx0[y0] = (x>>12); x += xi; }
		}
		if (y2 <= y3)
		{
			x = (long)((y2-npy[i+4])*slop + npx[i+4]*4096)+1; xi++;
			for(;y2<=y3;y2++) { bx1[y2] = (x>>12); x += xi; }
		}
	}

	(*ymin) = (long)(npy[3]);
	(*ymax) = (long)(npy[0]);
}

#define LROCSC 10
typedef struct { long x, y; } lpoint2d;
static lpoint2d lzlut[BUFZSIZ];
static char vislut[64] =
{
	2,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
	2,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
	2,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
	2,0,0,0,1,0,0,0,1,0,0,0,1,0,0,0,
};
void renderorthocube ()
{
	point3d nstr, nhei, nfor, npos;
	float vx[3], vy[3], /*vz[3],*/ ox, oy, /*oz,*/ nox, noy;
	float sd, hd, fd, d, fro, hfro;
	long x0, y0, z0, x1, y1, z1, xi, yi, zi, xpos[6], vind[6], vis, vismask;
	long x, y, xx, yy, isx, isy, ihx, ihy, ifx, ify, iox, ioy, ocnt, p, dlcol;
	long i, j, k, c, v, ymin, ymax, ymin0, ymax0, ymin1, ymax1, ymin2, ymax2;
	long sx, sy, sx0, sy0, sx1, sy1, onum, onum1, sx0i, sy0i, sx1i, sy1i, dsh;
	voxtype *v0, *v1, *v2, *v3;
	lpoint4d *op, *ope;

#if 0
		//Bad projection
	d = hz / (-ipos.x*ifor.x - ipos.y*ifor.y - ipos.z*ifor.z);
	vx[0] = -xpiv-ipos.x; vx[1] = -zpiv-ipos.y; vx[2] = +ypiv-ipos.z;
	npos.x = (vx[0]*irig.x + vx[1]*irig.y + vx[2]*irig.z)*d + hx;
	npos.y = (vx[0]*idow.x + vx[1]*idow.y + vx[2]*idow.z)*d + hy;
	nstr.x =  irig.x*d; nstr.y =  idow.x*d; nstr.z =  ifor.x;
	nhei.x = -irig.z*d; nhei.y = -idow.z*d; nhei.z = -ifor.z;
	nfor.x =  irig.y*d; nfor.y =  idow.y*d; nfor.z =  ifor.y;
#else
		//Good projection
	npos.x = -(irig.x*ipos.x + irig.y*ipos.y + irig.z*ipos.z);
	npos.y = -(idow.x*ipos.x + idow.y*ipos.y + idow.z*ipos.z);
	npos.z = -(ifor.x*ipos.x + ifor.y*ipos.y + ifor.z*ipos.z);

		//nox=hz/npos.z; noy=hz/(npos.z+ifor.x); //COMBINED / trick!
	nox = npos.z+ifor.x; noy = npos.z;
	d = hz / (nox*noy); nox *= d; noy *= d;
	ox = npos.x*nox;
	oy = npos.y*nox;
	nstr.x = (npos.x+irig.x)*noy-ox;
	nstr.y = (npos.y+idow.x)*noy-oy;
		//nox=hz/(npos.z-ifor.z); noy=hz/(npos.z+ifor.y); //COMBINED / trick!
	nox = npos.z+ifor.y; noy = npos.z-ifor.z;
	d = hz / (nox*noy); nox *= d; noy *= d;
	nhei.x = (npos.x-irig.z)*nox-ox;
	nhei.y = (npos.y-idow.z)*nox-oy;
	nfor.x = (npos.x+irig.y)*noy-ox;
	nfor.y = (npos.y+idow.y)*noy-oy;

	npos.x = ox+hx - (nstr.x*xpiv + nhei.x*ypiv + nfor.x*zpiv);
	npos.y = oy+hy - (nstr.y*xpiv + nhei.y*ypiv + nfor.y*zpiv);

		//Use cross product to calculate correct plane sides
	nstr.z = nhei.x*nfor.y - nhei.y*nfor.x;
	nhei.z = nfor.x*nstr.y - nfor.y*nstr.x;
	nfor.z = nstr.x*nhei.y - nstr.y*nhei.x;
#endif

	v = 0;
	isx = (long)(nstr.x*(float)(1<<LROCSC)); isy = (long)(nstr.y*(float)(1<<LROCSC));
	ihx = (long)(nhei.x*(float)(1<<LROCSC)); ihy = (long)(nhei.y*(float)(1<<LROCSC));
	ifx = (long)(nfor.x*(float)(1<<LROCSC)); ify = (long)(nfor.y*(float)(1<<LROCSC));
	iox = (long)(npos.x*(float)(1<<LROCSC)); ioy = (long)(npos.y*(float)(1<<LROCSC));
	xi = -(((*(long *)&nstr.z)>>31)|1); if (xi > 0) { x0 = 0; x1 = xsiz; } else { x0 = xsiz-1; x1 = -1; }
	yi = -(((*(long *)&nhei.z)>>31)|1); if (yi > 0) { y0 = 0; y1 = ysiz; } else { y0 = ysiz-1; y1 = -1; }
	zi = -(((*(long *)&nfor.z)>>31)|1); if (zi > 0) { z0 = 0; z1 = zsiz; } else { z0 = zsiz-1; z1 = -1; }

	x = y = 0;
	for(i=0;i<zsiz;i++)
	{
		lzlut[i].x = x; x += ifx;
		lzlut[i].y = y; y += ify;
	}

	vismask = (2-(xi<0)) + (2-(yi<0))*4 + (2-(zi<0))*16;

//--------------------------------------------------------------------------

		//Generate pre-compiled sprites for very fast rendering
	if (xi < 0) { vx[0] =  nstr.x; vy[0] =  nstr.y; /*vz[0] =  nstr.z;*/ }
			 else { vx[0] = -nstr.x; vy[0] = -nstr.y; /*vz[0] = -nstr.z;*/ }
	if (yi < 0) { vx[1] =  nhei.x; vy[1] =  nhei.y; /*vz[1] =  nhei.z;*/ }
			 else { vx[1] = -nhei.x; vy[1] = -nhei.y; /*vz[1] = -nhei.z;*/ }
	if (zi < 0) { vx[2] =  nfor.x; vy[2] =  nfor.y; /*vz[2] =  nfor.z;*/ }
			 else { vx[2] = -nfor.x; vy[2] = -nfor.y; /*vz[2] = -nfor.z;*/ }
	ox = (-vx[0]-vx[1]-vx[2] + (float)(HLINMAX-1)) * .5;
	oy = (-vy[0]-vy[1]-vy[2] + (float)(HLINMAX-1)) * .5;
	//oz = (-vz[0]-vz[1]-vz[2]) * .5;

		//Choose number (onum^2) of pre-compiled sprites to generate:
	sd = nstr.x*nstr.x + nstr.y*nstr.y + nstr.z*nstr.z;
	hd = nhei.x*nhei.x + nhei.y*nhei.y + nhei.z*nhei.z;
	fd = nfor.x*nfor.x + nfor.y*nfor.y + nfor.z*nfor.z;
	d = sd*sd + hd*hd + fd*fd;
		  if (d < 128)  { onum = 4; dsh = LROCSC-2; }
	else if (d < 8192) { onum = 2; dsh = LROCSC-1; }
	else               { onum = 1; dsh = LROCSC;   }

	onum1 = onum-1; fro = 1.0/onum; hfro = fro*.501;

	ocnt = 0;
	for(yy=0,noy=oy;yy<onum;yy++,noy+=fro)
		for(xx=0,nox=ox;xx<onum;xx++,nox+=fro)
		{
			orthead[yy*onum+xx] = &ortho[ocnt];

			fillparalbox(nox,noy,vx[2],vy[2],vx[1],vy[1],hfro,minx0,maxx0,&ymin0,&ymax0);
			fillparalbox(nox,noy,vx[0],vy[0],vx[2],vy[2],hfro,minx1,maxx1,&ymin1,&ymax1);
			fillparalbox(nox,noy,vx[1],vy[1],vx[0],vy[0],hfro,minx2,maxx2,&ymin2,&ymax2);

			if (ymin0 < ymin1) ymin = ymin0; else ymin = ymin1;
			if (ymax0 > ymax1) ymax = ymax0; else ymax = ymax1;
			if (ymin2 < ymin) ymin = ymin2;
			if (ymax2 > ymax) ymax = ymax2;

			if (ymin < -1) ymin = -1;
			if (ymax >= HLINMAX) ymax = HLINMAX-1;
			for(y=ymin+1;y<=ymax;y++)
			{
				c = 0;
				if ((y > ymin0) && (y <= ymax0))
				{
					xpos[c+0] = minx0[y]; vind[c+0] = 0x3;
					xpos[c+1] = maxx0[y]; vind[c+1] = 0x3; c += 2;
				}
				if ((y > ymin1) && (y <= ymax1))
				{
					xpos[c+0] = minx1[y]; vind[c+0] = 0xc;
					xpos[c+1] = maxx1[y]; vind[c+1] = 0xc; c += 2;
				}
				if ((y > ymin2) && (y <= ymax2))
				{
					xpos[c+0] = minx2[y]; vind[c+0] = 0x30;
					xpos[c+1] = maxx2[y]; vind[c+1] = 0x30; c += 2;
				}

				for(i=1;i<c;i++)
					for(j=0;j<i;j++)
						if (xpos[i] < xpos[j])
						{
							k = xpos[i]; xpos[i] = xpos[j]; xpos[j] = k;
							k = vind[i]; vind[i] = vind[j]; vind[j] = k;
						}

				v = 0; x = xpos[0]+1;
				for(i=0;i<c-1;i++)
				{
					v ^= vind[i];
					while (x <= xpos[i+1])
					{
						if (ocnt >= ORTHMAX) return;
						op = &ortho[ocnt];
						op->x = x - (HLINMAX>>1);
						op->y = y - (HLINMAX>>1);
						op->p = op->y*gdd.p + op->x;
						op->v = v;
						if (!(v&0x3))
						{
							if (v&0xc0) op->v |= 0x40000000;
							else if (v&0x30) op->v |= 0x80000000;
						}
						ocnt++; x++;
					}
				}
			}
		}
	orthead[onum*onum] = &ortho[ocnt];

//--------------------------------------------------------------------------

	if (xi < 0) { sx0i = -isx; sy0i = -isy; v0 = &voxdata[numvoxs]; }
			 else { sx0i =  isx; sy0i =  isy; v0 = voxdata; }
	if (yi < 0) { sx1i = -ihx; sy1i = -ihy; }
			 else { sx1i =  ihx; sy1i =  ihy; }
	sx0 = isx*x0 + ihx*y0 + iox;
	sy0 = isy*x0 + ihy*y0 + ioy;
	for(x=x0;x!=x1;x+=xi,sx0+=sx0i,sy0+=sy0i)
	{
		i = (long)xlen[x];
		if (xi < 0) v0 -= i;
		if (yi < 0) v1 = v0+i; else v1 = v0;
		if (xi >= 0) v0 += i;
		sx1 = sx0; sy1 = sy0;
		for(y=y0;y!=y1;y+=yi,sx1+=sx1i,sy1+=sy1i)
		{
			i = (long)ylen[x][y];
			if (yi < 0) v1 -= i;
			if (zi < 0) { v2 = v1+i-1; v3 = v1-1; } else { v2 = v1; v3 = v1+i; }
			if (yi >= 0) v1 += i;
			for(;v2!=v3;v2+=zi)
			{
				vis = (v2->vis&vismask); if (!vis) continue;
				sx = lzlut[v2->z].x+sx1;
				sy = lzlut[v2->z].y+sy1;
				i = ((sy>>dsh)&onum1)*onum+((sx>>dsh)&onum1);
				sx >>= LROCSC; sy >>= LROCSC;
				p = sy*gdd.p + sx + gdd.f;
				dlcol = palookup[shadeoff[v2->dir]][v2->col];
				for(op=orthead[i],ope=orthead[i+1];op<ope;op++)
				{
					i = (op->v&vis); if (!i) continue;
					if ((unsigned long)(op->y+sy) >= gdd.y) continue;
					if ((unsigned long)(op->x+sx) >= gdd.x) continue;

					//*(char *)(op->p+p) = slcol[((unsigned long)op->v)>>30][dlcol];

					//     if (i&0x03) i = 0;
					//else if (i&0x0c) i = 1;
					//else             i = 2;
					//*(char *)(op->p+p) = slcol[i][dlcol];

					*(char *)(op->p+p) = slcol[vislut[i]][dlcol];
				}
			}
		}
	}
}

void drawcube (float rx, float ry, float rz, char col, char visside)
{
	float t, u, xx[4], yy[4];
	long i, j, k, p, np, topp, botp, sx1, sy1, sy2, dx, xs, dlcol, snum;

	if (*(long *)&rz <= 0x40000000) return; //0x40000000 = 2

	u = hz / ((rz+ifor.x)*rz);  // 4/, 28*, 44+
	t = (rz+ifor.x)*u; sx[0] =  rx        *t + hx; sy[0] =  ry        *t + hy;
	t =  rz        *u; sx[1] = (rx+irig.x)*t + hx; sy[1] = (ry+idow.x)*t + hy;
	rz += ifor.y; u = hz / ((rz+ifor.x)*rz); rx += irig.y; ry += idow.y;
	t = (rz+ifor.x)*u; sx[2] =  rx        *t + hx; sy[2] =  ry        *t + hy;
	t =  rz        *u; sx[3] = (rx+irig.x)*t + hx; sy[3] = (ry+idow.x)*t + hy;
	rz += ifor.z-ifor.y; u = hz / ((rz+ifor.x)*rz); rx += irig.z-irig.y; ry += idow.z-idow.y;
	t = (rz+ifor.x)*u; sx[4] =  rx        *t + hx; sy[4] =  ry        *t + hy;
	t =  rz        *u; sx[5] = (rx+irig.x)*t + hx; sy[5] = (ry+idow.x)*t + hy;
	rz += ifor.y; u = hz / ((rz+ifor.x)*rz); rx += irig.y; ry += idow.y;
	t = (rz+ifor.x)*u; sx[6] =  rx        *t + hx; sy[6] =  ry        *t + hy;
	t =  rz        *u; sx[7] = (rx+irig.x)*t + hx; sy[7] = (ry+idow.x)*t + hy;

		//4 or 6 points are:    //When uncommenting, be sure xx[6], yy[6], if (np >= snum)...
	//snum = ptfaces[visside][0];
	//topp = botp = 0;
	//xx[0] = sx[ptfaces[visside][1]]*65536;
	//yy[0] = sy[ptfaces[visside][1]];
	//for(i=1;i<snum;i++)
	//{
	//   xx[i] = sx[ptfaces[visside][i+1]]*65536;
	//   yy[i] = sy[ptfaces[visside][i+1]];
	//   t = yy[i]-yy[topp]; if (*(long *)&t < 0) topp = i;
	//   t = yy[i]-yy[botp]; if (*(long *)&t > 0) botp = i;
	//}
	//dlcol = lcol[col];

	for(k=5;k>=0;k--)
	{
		if (!(visside&pow2char[k])) continue;

		xx[0] = sx[ptface[k][0]]*65536; yy[0] = sy[ptface[k][0]];
		xx[1] = sx[ptface[k][1]]*65536; yy[1] = sy[ptface[k][1]];
		xx[2] = sx[ptface[k][2]]*65536; yy[2] = sy[ptface[k][2]];
		xx[3] = sx[ptface[k][3]]*65536; yy[3] = sy[ptface[k][3]];
		k &= ~1; dlcol = slcol[k>>1][col];

			//Nice method for getting top&bot indices (No floatcmp, no j??)
		t = yy[1]-yy[0]; i = ((*(unsigned long *)&t) >> 31);
		t = yy[3]-yy[2]; j = ((*(unsigned long *)&t) >> 31)+2;
		t = yy[j]-yy[i]; topp = i + ((j-i)&((*(long *)&t)>>31));
		t = yy[i^1]-yy[j^1]; botp = (i^1)+(((j^1)-(i^1))&((*(long *)&t)>>31));

			//Note: .5 should be added to all ftol's with normal rounding mode.
			//Since I'm rounding using "round to �" mode, this code is optimized out!

		p = botp;
		ftol(yy[p],&sy1); if (sy1 > gdd.y) sy1 = gdd.y;
		do
		{
			np = (p+1)&3; //if (np >= snum) np = 0;
			ftol(yy[np],&sy2); if (sy2 < 0) sy2 = 0;
			if (sy1 > sy2)
			{
				t = (xx[np]-xx[p])/(yy[np]-yy[p]);
				ftol(((float)sy2-yy[np])*t+xx[np],&xs); ftol(t,&dx);
				qinterpolatedown16(&lastx[sy2],sy1-sy2,xs,dx); sy1 = sy2;
			}
			p = np;
		} while (np != topp);
		j = gdd.f+ylookup[sy1];
		do
		{
			np = (p+1)&3; //if (np >= snum) np = 0;
			ftol(yy[np],&sy2); if (sy2 > gdd.y) sy2 = gdd.y;
			if (sy2 > sy1)
			{
				t = (xx[np]-xx[p])/(yy[np]-yy[p]);
				ftol(((float)sy1-yy[p])*t+xx[p],&xs); ftol(t,&dx);
				do
				{
					sx1 = lastx[sy1]; if (sx1 < 0) sx1 = 0;
					clearbufbyte((void *)(sx1+j),min(xs>>16,gdd.x)-sx1,dlcol);
					xs += dx; j += gdd.p; sy1++;
				} while (sy1 < sy2);
			}
			p = np;
		} while (np != botp);
	}
}

void rendercube ()
{
	voxtype *xv, *yv, *v0, *v1;
	point3d c, r0, r1, r2, *p;
	float f, bakhz;
	long i, j, k, x, y, z, inz, splitx, splity, splity2, ofpumode;

	ofpumode = fstcw(); fldcw((ofpumode&~0xf00)|0x800); //24 bit, round to �

		//Ugly hack to avoid hz multiplies!
	oirig = irig; irig.x *= hz; irig.y *= hz; irig.z *= hz;
	oidow = idow; idow.x *= hz; idow.y *= hz; idow.z *= hz;
	bakhz = hz; hz = 1;

	ftol( ipos.x+xpiv-.5,&x);
	ftol(-ipos.z+ypiv-.5,&y);
	ftol( ipos.y+zpiv-.5,&inz);
	splitx = min(max(x,0),xsiz);
	splity = min(max(y,0),ysiz);
	splity2 = min(max(y,-1),ysiz-1);
	c.x = -xpiv-ipos.x;
	c.y = +ypiv-ipos.z;
	c.z = -zpiv-ipos.y;
	for(i=1;i<zsiz;i++)
	{
		ztab[i].x = ztab[i-1].x+irig.y;
		ztab[i].y = ztab[i-1].y+idow.y;
		ztab[i].z = ztab[i-1].z+ifor.y;
	}
	r2.x = -irig.z*(float)ysiz;
	r2.y = -idow.z*(float)ysiz;
	r2.z = -ifor.z*(float)ysiz;

	xv = voxdata;

	c.x -= .5; c.y -= .5; c.z -= .5;

	r1.x = irig.x*c.x+irig.z*c.y+irig.y*c.z;
	r1.y = idow.x*c.x+idow.z*c.y+idow.y*c.z;
	r1.z = ifor.x*c.x+ifor.z*c.y+ifor.y*c.z;
	for(x=0;x<splitx;x++)
	{
		yv = xv+xlen[x]; r0 = r1;
		for(y=0;y<splity;y++)
		{
			v0 = xv; xv += ylen[x][y]; v1 = xv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+4+16))
					{ p = &ztab[v0->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(1+4+16)); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+4+32))
					{ p = &ztab[v1->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(1+4+32)); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
		xv = yv;
		r0.x = r2.x+r1.x; r1.x += irig.x;
		r0.y = r2.y+r1.y; r1.y += idow.x;
		r0.z = r2.z+r1.z; r1.z += ifor.x;
		for(y=ysiz-1;y>=splity;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = yv-1; yv -= ylen[x][y]; v0 = yv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+8+16))
					{ p = &ztab[v0->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(1+8+16)); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+8+32))
					{ p = &ztab[v1->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(1+8+32)); }
		}
	}
	xv = &voxdata[numvoxs]; f = (float)xsiz-splitx;
	r1.x += irig.x*f; r1.y += idow.x*f; r1.z += ifor.x*f;
	for(x=xsiz-1;x>=splitx;x--)
	{
		yv = xv-xlen[x];
		r1.x -= irig.x; r0.x = r1.x+r2.x;
		r1.y -= idow.x; r0.y = r1.y+r2.y;
		r1.z -= ifor.x; r0.z = r1.z+r2.z;
		for(y=ysiz-1;y>splity2;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = xv-1; xv -= ylen[x][y]; v0 = xv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+8+16))
					{ p = &ztab[v0->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(2+8+16)); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+8+32))
					{ p = &ztab[v1->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(2+8+32)); }
		}
		xv = yv; r0 = r1;
		for(y=0;y<=splity2;y++)
		{
			v0 = yv; yv += ylen[x][y]; v1 = yv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+4+16))
					{ p = &ztab[v0->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(2+4+16)); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+4+32))
					{ p = &ztab[v1->z]; drawcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(2+4+32)); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
	}

		//Ugly hack to avoid hz multiplies!
	irig = oirig;
	idow = oidow;
	hz = bakhz;

	fldcw(ofpumode); //restore fpumode to value at beginning of function
}


static long gotsse = 0;
long checksse ()  //Returns 1 if you have >= Pentium III, otherwise 0
{
 return 0;
}

typedef struct { float x, y, z, z2; } point4d;

#ifdef __cplusplus
extern "C" {
#endif

extern void *caddasm;
#define cadd4 ((point4d *)&caddasm)
extern void *ztabasm;
#define ztab4 ((point4d *)&ztabasm)
extern short qsum0[4], qsum1[4], qbplbpp[4];
extern long kv6frameplace, kv6bpl;

void s6_asm_dep_unlock ();
void drawboundcubeasm (point4d *, long, long);

char ptfaces16[43][8] =
{
	0, 0, 0,  0,  0, 0, 0,0,  4, 0,32,96, 64, 0,32,0,  4,16,80,112,48, 16,80,0,  0,0,0,0,0,0,0,0,
	4,64,96,112, 80,64,96,0,  6, 0,32,96,112,80,64,0,  6,16,80, 64,96,112,48,0,  0,0,0,0,0,0,0,0,
	4, 0,16, 48, 32, 0,16,0,  6, 0,16,48, 32,96,64,0,  6, 0,16, 80,112,48,32,0,  0,0,0,0,0,0,0,0,
	0, 0, 0,  0,  0, 0, 0,0,  0, 0, 0, 0,  0, 0, 0,0,  0, 0, 0,  0,  0, 0, 0,0,  0,0,0,0,0,0,0,0,
	4, 0,64, 80, 16, 0,64,0,  6, 0,32,96, 64,80,16,0,  6, 0,64, 80,112,48,16,0,  0,0,0,0,0,0,0,0,
	6, 0,64, 96,112,80,16,0,  6, 0,32,96,112,80,16,0,  6, 0,64, 96,112,48,16,0,  0,0,0,0,0,0,0,0,
	6, 0,64, 80, 16,48,32,0,  6,16,48,32, 96,64,80,0,  6, 0,64, 80,112,48,32,0,  0,0,0,0,0,0,0,0,
	0, 0, 0,  0,  0, 0, 0,0,  0, 0, 0, 0,  0, 0, 0,0,  0, 0, 0,  0,  0, 0, 0,0,  0,0,0,0,0,0,0,0,
	4,32,48,112, 96,32,48,0,  6, 0,32,48,112,96,64,0,  6,16,80,112, 96,32,48,0,  0,0,0,0,0,0,0,0,
	6,32,48,112, 80,64,96,0,  6, 0,32,48,112,80,64,0,  6,16,80, 64, 96,32,48,0,  0,0,0,0,0,0,0,0,
	6, 0,16, 48,112,96,32,0,  6, 0,16,48,112,96,64,0,  6, 0,16, 80,112,96,32,0,
};

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
static _inline void movps (point4d *dest, point4d *src)
{
	_asm
	{
		mov edx, dest
		mov ebx, src
		movaps xmm7, [ebx]
		movaps [edx], xmm7
	}
}

static _inline void intss (point4d *dest, long src)
{
	_asm
	{
		mov edx, dest
		mov eax, src
		cvtsi2ss xmm7, eax
		shufps xmm7, xmm7, 0
		movaps [edx], xmm7
	}
}

static _inline void addps (point4d *sum, point4d *a, point4d *b)
{
	_asm
	{
		mov edx, sum
		mov ebx, a
		mov ecx, b
		movaps xmm7, [ebx]
		movaps xmm1, [ecx]
		addps xmm7, xmm1
		movaps [edx], xmm7
	}
}

static _inline void mulps (point4d *sum, point4d *a, point4d *b)
{
	_asm
	{
		mov edx, sum
		mov ebx, a
		mov ecx, b
		movaps xmm7, [ebx]
		movaps xmm1, [ecx]
		mulps xmm7, xmm1
		movaps [edx], xmm7
	}
}

static _inline void subps (point4d *sum, point4d *a, point4d *b)
{
	_asm
	{
		mov edx, sum
		mov ebx, a
		mov ecx, b
		movaps xmm7, [ebx]
		movaps xmm1, [ecx]
		subps xmm7, xmm1
		movaps [edx], xmm7
	}
}

static _inline void emms ()
{
	_asm emms
}
#endif

static void initboundcubescr (long x, long y, long dabpl, long dacolbits)
{
	qsum0[3] = qsum0[1] = (0x7fff-(y>>1)); qsum1[3] = qsum1[1] = (0x7fff-y);
	qsum0[2] = qsum0[0] = (0x7fff-(x>>1)); qsum1[2] = qsum1[0] = (0x7fff-x);
	qbplbpp[1] = gdd.p; qbplbpp[0] = ((dacolbits+7)>>3);
}

void renderboundcubep3 ()
{
	voxtype *xv, *yv, *v0, *v1;
	point4d *r0, *r1, *r2, *q, *irig4, *idow4, *ifor4;
	float f;
	long i, x, y, inz, splitx, splity, splity2;
	static long didunlock = 0;

	r0 = &ztab4[BUFZSIZ]; r1 = &ztab4[BUFZSIZ+1]; r2 = &ztab4[BUFZSIZ+2];
	q = &ztab4[BUFZSIZ+3]; irig4 = &ztab4[BUFZSIZ+4];
	idow4 = &ztab4[BUFZSIZ+5]; ifor4 = &ztab4[BUFZSIZ+6];

	kv6frameplace = gdd.f - (qsum1[0]*qbplbpp[0] + qsum1[1]*qbplbpp[1]);
	kv6bpl = gdd.p;

#if (CULLBYDIR)
	if (reflectmode < 1)
		memset((void *)isvis,1,sizeof(isvis));
	else
	{
		for(i=255;i>0;i--)
		{
			f = equivec.p[i].x*ipos.x - equivec.p[i].y*ipos.z + equivec.p[i].z*ipos.y;
			isvis[i] = ((((char *)&f)[3])>>7);
		}
		isvis[0] = 1;
	}
#endif

	ftol( ipos.x+xpiv-.5,&x);
	ftol(-ipos.z+ypiv-.5,&y);
	ftol( ipos.y+zpiv-.5,&inz);
	splitx = min(max(x,0),xsiz);
	splity = min(max(y,0),ysiz);
	splity2 = min(max(y,-1),ysiz-1);

		//Ugly hack to avoid hz multiplies!
	irig4->x = irig.x*hz; irig4->y = irig.y*hz; irig4->z = irig.z*hz; irig4->z2 = irig4->z;
	idow4->x = idow.x*hz; idow4->y = idow.y*hz; idow4->z = idow.z*hz; idow4->z2 = idow4->z;
	ifor4->x = ifor.x   ; ifor4->y = ifor.y   ; ifor4->z = ifor.z   ; ifor4->z2 = ifor4->z;

	q->x = -xpiv-ipos.x-.5;
	q->y = -zpiv-ipos.y-.5;
	q->z = +ypiv-ipos.z-.5;
	r1->x = irig4->x*q->x + irig4->y*q->y + irig4->z*q->z;
	r1->y = idow4->x*q->x + idow4->y*q->y + idow4->z*q->z;
	r1->z = ifor4->x*q->x + ifor4->y*q->y + ifor4->z*q->z;
	r1->z2 = r1->z;

	cadd4[1].x = irig4->x; cadd4[1].y = idow4->x; cadd4[1].z = ifor4->x; cadd4[1].z2 = cadd4[1].z;
	cadd4[2].x = irig4->y; cadd4[2].y = idow4->y; cadd4[2].z = ifor4->y; cadd4[2].z2 = cadd4[2].z;
	cadd4[4].x = irig4->z; cadd4[4].y = idow4->z; cadd4[4].z = ifor4->z; cadd4[4].z2 = cadd4[4].z;
	addps(&cadd4[3],&cadd4[1],&cadd4[2]);
	addps(&cadd4[5],&cadd4[1],&cadd4[4]);
	addps(&cadd4[6],&cadd4[2],&cadd4[4]);
	addps(&cadd4[7],&cadd4[3],&cadd4[4]);

	for(i=1;i<zsiz;i++) addps(&ztab4[i],&ztab4[i-1],&cadd4[2]);
	intss(r2,-ysiz); mulps(r2,r2,&cadd4[4]);

	xv = voxdata;
	for(x=0;x<splitx;x++)
	{
		yv = xv+xlen[x]; movps(r0,r1);
		for(y=0;y<splity;y++)
		{
			v0 = xv; xv += ylen[x][y]; v1 = xv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(1+4+16))
#else
				if (isvis[v0->dir])
#endif
					{ addps(q,r0,&ztab4[v0->z]); drawboundcubeasm(q,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(1+4+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(1+4+32))
#else
				if (isvis[v1->dir])
#endif
					{ addps(q,r0,&ztab4[v1->z]); drawboundcubeasm(q,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(1+4+32)); }
			subps(r0,r0,&cadd4[4]);
		}
		xv = yv;
		addps(r0,r1,r2);
		addps(r1,r1,&cadd4[1]);
		for(y=ysiz-1;y>=splity;y--)
		{
			addps(r0,r0,&cadd4[4]);
			v1 = yv-1; yv -= ylen[x][y]; v0 = yv;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(1+8+16))
#else
				if (isvis[v0->dir])
#endif
					{ addps(q,r0,&ztab4[v0->z]); drawboundcubeasm(q,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(1+8+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(1+8+32))
#else
				if (isvis[v1->dir])
#endif
					{ addps(q,r0,&ztab4[v1->z]); drawboundcubeasm(q,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(1+8+32)); }
		}
	}
	xv = &voxdata[numvoxs];
	intss(q,xsiz-splitx); mulps(q,q,&cadd4[1]);
	addps(r1,r1,q);
	for(x=xsiz-1;x>=splitx;x--)
	{
		yv = xv-xlen[x];
		subps(r1,r1,&cadd4[1]);
		addps(r0,r1,r2);
		for(y=ysiz-1;y>splity2;y--)
		{
			addps(r0,r0,&cadd4[4]);
			v1 = xv-1; xv -= ylen[x][y]; v0 = xv;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(2+8+16))
#else
				if (isvis[v0->dir])
#endif
					{ addps(q,r0,&ztab4[v0->z]); drawboundcubeasm(q,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(2+8+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(2+8+32))
#else
				if (isvis[v1->dir])
#endif
					{ addps(q,r0,&ztab4[v1->z]); drawboundcubeasm(q,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(2+8+32)); }
		}
		xv = yv; movps(r0,r1);
		for(y=0;y<=splity2;y++)
		{
			v0 = yv; yv += ylen[x][y]; v1 = yv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(2+4+16))
#else
				if (isvis[v0->dir])
#endif
					{ addps(q,r0,&ztab4[v0->z]); drawboundcubeasm(q,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(2+4+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(2+4+32))
#else
				if (isvis[v1->dir])
#endif
					{ addps(q,r0,&ztab4[v1->z]); drawboundcubeasm(q,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(2+4+32)); }
			subps(r0,r0,&cadd4[4]);
		}
	}
	emms();

		//Ugly hack to avoid hz multiplies!
	//irig = oirig;
	//idow = oidow;
}

void drawboundcube (float rx, float ry, float rz, char col, char visside)
{
	point3d *c0, *c1, *c2, *c3;
	float t0, t1, t2, t3;
	long x, y, x0, y0, x1, y1, dlcol;
	char *ptr;

	if (*(long *)&rz <= 0x40000000) return; //0x40000000 = 2

	ptr = &ptfaces[visside][0];

	//gs[0] += gi[0]; gs[1] += gi[1]; gs[2] += gi[2];
	//c0 = &cadd[(gs[0]<     0)+((gs[1]<     0)<<1)+((gs[2]<     0)<<2)];
	//ftol((c0->x+rx)/(c0->z+rz),&x0);
	//c0 = &cadd[(gs[0]>=65536)+((gs[1]>=65536)<<1)+((gs[2]>=65536)<<2)];
	//ftol((c0->x+rx)/(c0->z+rz),&x1);
	//gs[3] += gi[3]; gs[4] += gi[4]; gs[5] += gi[5];
	//c0 = &cadd[(gs[3]<     0)+((gs[4]<     0)<<1)+((gs[5]<     0)<<2)];
	//ftol((c0->y+ry)/(c0->z+rz),&y0);
	//c0 = &cadd[(gs[3]>=65536)+((gs[4]>=65536)<<1)+((gs[5]>=65536)<<2)];
	//ftol((c0->y+ry)/(c0->z+rz),&y1);

		//Note: frecipasm's assume hz = 1.0 from global hacks
	c0 = &cadd[ptr[1]]; t0 = c0->z+rz;
	c1 = &cadd[ptr[2]]; t1 = c1->z+rz;
	c2 = &cadd[ptr[3]]; t2 = c2->z+rz;
	c3 = &cadd[ptr[4]]; t3 = c3->z+rz;
	frecipasm(&t0,&t0); //t0 = hz / (c0->z+rz);
	frecipasm(&t1,&t1); //t1 = hz / (c1->z+rz);
	frecipasm(&t2,&t2); //t2 = hz / (c2->z+rz);
	frecipasm(&t3,&t3); //t3 = hz / (c3->z+rz);
	ftol((c0->x+rx)*t0,&x0);
	ftol((c0->y+ry)*t0,&y0);
	ftol((c1->x+rx)*t1,&x1); if (x1 < x0) { x = x1; x1 = x0; x0 = x; }
	ftol((c1->y+ry)*t1,&y1); if (y1 < y0) { y = y1; y1 = y0; y0 = y; }
	ftol((c2->x+rx)*t2,&x); if (x < x0) x0 = x; else if (x > x1) x1 = x;
	ftol((c2->y+ry)*t2,&y); if (y < y0) y0 = y; else if (y > y1) y1 = y;
	ftol((c3->x+rx)*t3,&x); if (x < x0) x0 = x; else if (x > x1) x1 = x;
	ftol((c3->y+ry)*t3,&y); if (y < y0) y0 = y; else if (y > y1) y1 = y;
	if (ptr[0] == 6)
	{
		c0 = &cadd[ptr[5]]; t0 = c0->z+rz;
		c1 = &cadd[ptr[6]]; t1 = c1->z+rz;
		frecipasm(&t0,&t0); //t0 = hz / (c0->z+rz);
		frecipasm(&t1,&t1); //t1 = hz / (c1->z+rz);
		ftol((c0->x+rx)*t0,&x); if (x < x0) x0 = x; else if (x > x1) x1 = x;
		ftol((c0->y+ry)*t0,&y); if (y < y0) y0 = y; else if (y > y1) y1 = y;
		ftol((c1->x+rx)*t1,&x); if (x < x0) x0 = x; else if (x > x1) x1 = x;
		ftol((c1->y+ry)*t1,&y); if (y < y0) y0 = y; else if (y > y1) y1 = y;
	}

	x0 += ihx; if (x0 < 0) x0 = 0;
	x1 += ihx; if (x1 > gdd.x) x1 = gdd.x;
	y0 += ihy; if (y0 < 0) y0 = 0;
	y1 += ihy; if (y1 > gdd.y) y1 = gdd.y;
	x1 -= x0; y1 -= y0; if ((x1 <= 0) || (y1 <= 0)) return;

#ifdef VBEAFDRV
	vbh->DrawRect(vbh,col,x0,y0,x1,y1); return;
#endif
	y0 = gdd.f+ylookup[y0]+x0; y1 = ylookup[y1]+y0; dlcol = lcol[col];
	do
	{
		clearbufbyte((void *)y0,x1,dlcol);
		y0 += gdd.p;
	} while (y0 != y1);
}

void renderboundcube ()
{
	voxtype *xv, *yv, *v0, *v1;
	point3d c, r0, r1, r2, *p;
	float f;
	long i, j, k, x, y, z, inz, splitx, splity, splity2;

		//Ugly hack to avoid hz multiplies!
	oirig = irig; irig.x *= hz; irig.y *= hz; irig.z *= hz;
	oidow = idow; idow.x *= hz; idow.y *= hz; idow.z *= hz;

	ftol( ipos.x+xpiv-.5,&x);
	ftol(-ipos.z+ypiv-.5,&y);
	ftol( ipos.y+zpiv-.5,&inz);
	splitx = min(max(x,0),xsiz);
	splity = min(max(y,0),ysiz);
	splity2 = min(max(y,-1),ysiz-1);
	c.x = -xpiv-ipos.x;
	c.y = +ypiv-ipos.z;
	c.z = -zpiv-ipos.y;
	for(i=1;i<zsiz;i++)
	{
		ztab[i].x = ztab[i-1].x+irig.y;
		ztab[i].y = ztab[i-1].y+idow.y;
		ztab[i].z = ztab[i-1].z+ifor.y;
	}
	r2.x = -irig.z*(float)ysiz;
	r2.y = -idow.z*(float)ysiz;
	r2.z = -ifor.z*(float)ysiz;

	xv = voxdata;

	cadd[0].x = cadd[0].y = cadd[0].z = 0;
	cadd[1].x = irig.x; cadd[1].y = idow.x; cadd[1].z = ifor.x;
	for(i=0;i<2;i++)
	{
		cadd[i+2].x = cadd[i].x+irig.y;
		cadd[i+2].y = cadd[i].y+idow.y;
		cadd[i+2].z = cadd[i].z+ifor.y;
	}
	for(i=0;i<4;i++)
	{
		cadd[i+4].x = cadd[i].x+irig.z;
		cadd[i+4].y = cadd[i].y+idow.z;
		cadd[i+4].z = cadd[i].z+ifor.z;
	}

#if (CULLBYDIR)
	if (reflectmode < 1)
		memset(isvis,1,sizeof(isvis));
	else
	{
		for(i=255;i>=0;i--)
		{
			f = equivec.p[i].x*ipos.x - equivec.p[i].y*ipos.z + equivec.p[i].z*ipos.y;
			isvis[i] = ((((char *)&f)[3])>>7);
		}
	}
#endif

	c.x -= .5; c.y -= .5; c.z -= .5;

	r1.x = irig.x*c.x+irig.z*c.y+irig.y*c.z;
	r1.y = idow.x*c.x+idow.z*c.y+idow.y*c.z;
	r1.z = ifor.x*c.x+ifor.z*c.y+ifor.y*c.z;
	for(x=0;x<splitx;x++)
	{
		yv = xv+xlen[x]; r0 = r1;
		for(y=0;y<splity;y++)
		{
			v0 = xv; xv += ylen[x][y]; v1 = xv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(1+4+16))
#else
				if (isvis[v0->dir])
#endif
					{ p = &ztab[v0->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(1+4+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(1+4+32))
#else
				if (isvis[v1->dir])
#endif
					{ p = &ztab[v1->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(1+4+32)); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
		xv = yv;
		r0.x = r2.x+r1.x; r1.x += irig.x;
		r0.y = r2.y+r1.y; r1.y += idow.x;
		r0.z = r2.z+r1.z; r1.z += ifor.x;
		for(y=ysiz-1;y>=splity;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = yv-1; yv -= ylen[x][y]; v0 = yv;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(1+8+16))
#else
				if (isvis[v0->dir])
#endif
					{ p = &ztab[v0->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(1+8+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(1+8+32))
#else
				if (isvis[v1->dir])
#endif
					{ p = &ztab[v1->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(1+8+32)); }
		}
	}
	xv = &voxdata[numvoxs]; f = (float)xsiz-splitx;
	r1.x += irig.x*f; r1.y += idow.x*f; r1.z += ifor.x*f;
	for(x=xsiz-1;x>=splitx;x--)
	{
		yv = xv-xlen[x];
		r1.x -= irig.x; r0.x = r1.x+r2.x;
		r1.y -= idow.x; r0.y = r1.y+r2.y;
		r1.z -= ifor.x; r0.z = r1.z+r2.z;
		for(y=ysiz-1;y>splity2;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = xv-1; xv -= ylen[x][y]; v0 = xv;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(2+8+16))
#else
				if (isvis[v0->dir])
#endif
					{ p = &ztab[v0->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(2+8+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(2+8+32))
#else
				if (isvis[v1->dir])
#endif
					{ p = &ztab[v1->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(2+8+32)); }
		}
		xv = yv; r0 = r1;
		for(y=0;y<=splity2;y++)
		{
			v0 = yv; yv += ylen[x][y]; v1 = yv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
#if (!CULLBYDIR)
				if (v0->vis&~(2+4+16))
#else
				if (isvis[v0->dir])
#endif
					{ p = &ztab[v0->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col],v0->vis&~(2+4+16)); }
			for(;v0<=v1;v1--)
#if (!CULLBYDIR)
				if (v1->vis&~(2+4+32))
#else
				if (isvis[v1->dir])
#endif
					{ p = &ztab[v1->z]; drawboundcube(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col],v1->vis&~(2+4+32)); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
	}

		//Ugly hack to avoid hz multiplies!
	irig = oirig;
	idow = oidow;
}

void drawsphere (float cx, float cy, float cz, char col)
{
	float a, b, c, d, e, f, g, h, t, cxcx, cycy, Za, Zb, Zc, ysq;
	float r2a, rr2a, nb, nbi, isq, isqi, isqii;
	long sy, sx1, sy1, sx2, sy2, p, dlcol;

	if (*(long *)&cz <= 0x40000000) return; //0x40000000 = 2

		//3D Sphere projection (see spherast.txt for derivation) (13 multiplies)
	cxcx = cx*cx; cycy = cy*cy; g = spherad2 - cxcx - cycy - cz*cz;
	a = g + cxcx; if (!a) return;
	b = cx*cy; b += b;
	c = g + cycy;
	f = hx*cx + hy*cy - hz*cz;
	d = -cx*f - hx*g; d += d;
	e = -cy*f - hy*g; e += e;
	f = f*f + g*hds;

		//isq = (b*b-4*a*c)y� + (2*b*d-4*a*e)y + (d*d-4*a*f) = 0
	Za = b*b - a*c*4; if (!Za) return;
	Zb = b*d*2 - a*e*4;
	Zc = d*d - a*f*4;
	ysq = Zb*Zb - Za*Zc*4; if (ysq <= 0) return;
	fsqrtasm(&ysq,&t); h = .5/Za;
	ftol((-Zb+t)*h,&sy1); if (sy1 < 0) sy1 = 0;
	ftol((-Zb-t)*h,&sy2); if (sy2 > gdd.y) sy2 = gdd.y;
	if (sy1 >= sy2) return;
	r2a = .5/a; rr2a = r2a*r2a;
	nbi = -b*r2a; nb = nbi*(float)sy1-d*r2a;
	h = Za*(float)sy1; isq = ((float)sy1*(h+Zb)+Zc)*rr2a;
	isqi = (h+h+Za+Zb)*rr2a; isqii = Za*rr2a*2;

	p = gdd.f+ylookup[sy1]; sy2 = gdd.f+ylookup[sy2]; dlcol = lcol[col];
	while (1)  //(a)x� + (b*y+d)x + (c*y*y+e*y+f) = 0
	{
		fsqrtasm(&isq,&t);

		ftol(nb-t,&sx1); if (sx1 < 0) sx1 = 0;
		ftol(nb+t,&sx2);
		clearbufbyte((void *)(sx1+p),min(sx2,gdd.x)-sx1,dlcol);
		p += gdd.p; if (p == sy2) return;

		isq += isqi; isqi += isqii; nb += nbi;
	}
}

void rendersphere ()
{
	voxtype *xv, *yv, *v0, *v1;
	point3d c, r0, r1, r2, *p;
	float f;
	long i, j, k, x, y, z, inz, splitx, splity, splity2;

	ftol( ipos.x+xpiv-.5,&x);
	ftol(-ipos.z+ypiv-.5,&y);
	ftol( ipos.y+zpiv-.5,&inz);
	splitx = min(max(x,0),xsiz);
	splity = min(max(y,0),ysiz);
	splity2 = min(max(y,-1),ysiz-1);
	c.x = -xpiv-ipos.x;
	c.y = +ypiv-ipos.z;
	c.z = -zpiv-ipos.y;
	for(i=1;i<zsiz;i++)
	{
		ztab[i].x = ztab[i-1].x+irig.y;
		ztab[i].y = ztab[i-1].y+idow.y;
		ztab[i].z = ztab[i-1].z+ifor.y;
	}
	r2.x = -irig.z*(float)ysiz;
	r2.y = -idow.z*(float)ysiz;
	r2.z = -ifor.z*(float)ysiz;
	
	xv = voxdata;

	r1.x = irig.x*c.x+irig.z*c.y+irig.y*c.z;
	r1.y = idow.x*c.x+idow.z*c.y+idow.y*c.z;
	r1.z = ifor.x*c.x+ifor.z*c.y+ifor.y*c.z;
	for(x=0;x<splitx;x++)
	{
		yv = xv+xlen[x]; r0 = r1;
		for(y=0;y<splity;y++)
		{
			v0 = xv; xv += ylen[x][y]; v1 = xv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+4+16))
					{ p = &ztab[v0->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+4+32))
					{ p = &ztab[v1->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
		xv = yv;
		r0.x = r2.x+r1.x; r1.x += irig.x;
		r0.y = r2.y+r1.y; r1.y += idow.x;
		r0.z = r2.z+r1.z; r1.z += ifor.x;
		for(y=ysiz-1;y>=splity;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = yv-1; yv -= ylen[x][y]; v0 = yv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+8+16))
					{ p = &ztab[v0->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+8+32))
					{ p = &ztab[v1->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
		}
	}
	xv = &voxdata[numvoxs]; f = (float)xsiz-splitx;
	r1.x += irig.x*f; r1.y += idow.x*f; r1.z += ifor.x*f;
	for(x=xsiz-1;x>=splitx;x--)
	{
		yv = xv-xlen[x];
		r1.x -= irig.x; r0.x = r1.x+r2.x;
		r1.y -= idow.x; r0.y = r1.y+r2.y;
		r1.z -= ifor.x; r0.z = r1.z+r2.z;
		for(y=ysiz-1;y>splity2;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = xv-1; xv -= ylen[x][y]; v0 = xv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+8+16))
					{ p = &ztab[v0->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+8+32))
					{ p = &ztab[v1->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
		}
		xv = yv; r0 = r1;
		for(y=0;y<=splity2;y++)
		{
			v0 = yv; yv += ylen[x][y]; v1 = yv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+4+16))
					{ p = &ztab[v0->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+4+32))
					{ p = &ztab[v1->z]; drawsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
	}
}

void drawboundsphere (float cx, float cy, float cz, char col)
{
	float ax, bx, ay, by, f;
	long x0, y0, x1, y1, dlcol;

	if (*(long *)&cz <= 0x40000000) return; //0x40000000 = 2

	f = cz*cz; bx = (sphk3+f)*f + sphk2;
	f -= spherad2; frecipasm(&f,&f); //f = 1.0/(f-spherad2);
	ax = cz*hz*f;
	ay = ax*cy + hy; by = bx+cy*cy*sphk1; fsqrtasm(&by,&by); by *= f;
	ax = ax*cx + hx; bx +=   cx*cx*sphk1; fsqrtasm(&bx,&bx); bx *= f;

	ftol(ay-by,&y0); if (y0 < 0) y0 = 0;
	ftol(ay+by,&y1); if (y1 > gdd.y) y1 = gdd.y;
	y1 -= y0; if (y1 <= 0) return;
	ftol(ax-bx,&x0); if (x0 < 0) x0 = 0;
	ftol(ax+bx,&x1); if (x1 > gdd.x) x1 = gdd.x;
	x1 -= x0; if (x1 <= 0) return;

#ifdef VBEAFDRV
	vbh->DrawRect(vbh,col,x0,y0,x1,y1); return;
#endif
	y0 = gdd.f+ylookup[y0]+x0; y1 = ylookup[y1]+y0; dlcol = lcol[col];
	do
	{
		clearbufbyte((void *)y0,x1,dlcol);
		y0 += gdd.p;
	} while (y0 != y1);
}

void renderboundsphere ()
{
	voxtype *xv, *yv, *v0, *v1;
	point3d c, r0, r1, r2, *p;
	float f;
	long i, j, k, x, y, z, inz, splitx, splity, splity2;

	ftol( ipos.x+xpiv-.5,&x);
	ftol(-ipos.z+ypiv-.5,&y);
	ftol( ipos.y+zpiv-.5,&inz);
	splitx = min(max(x,0),xsiz);
	splity = min(max(y,0),ysiz);
	splity2 = min(max(y,-1),ysiz-1);
	c.x = -xpiv-ipos.x;
	c.y = +ypiv-ipos.z;
	c.z = -zpiv-ipos.y;
	for(i=1;i<zsiz;i++)
	{
		ztab[i].x = ztab[i-1].x+irig.y;
		ztab[i].y = ztab[i-1].y+idow.y;
		ztab[i].z = ztab[i-1].z+ifor.y;
	}
	r2.x = -irig.z*(float)ysiz;
	r2.y = -idow.z*(float)ysiz;
	r2.z = -ifor.z*(float)ysiz;

	xv = voxdata;

	r1.x = irig.x*c.x+irig.z*c.y+irig.y*c.z;
	r1.y = idow.x*c.x+idow.z*c.y+idow.y*c.z;
	r1.z = ifor.x*c.x+ifor.z*c.y+ifor.y*c.z;
	for(x=0;x<splitx;x++)
	{
		yv = xv+xlen[x]; r0 = r1;
		for(y=0;y<splity;y++)
		{
			v0 = xv; xv += ylen[x][y]; v1 = xv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+4+16))
					{ p = &ztab[v0->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+4+32))
					{ p = &ztab[v1->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
		xv = yv;
		r0.x = r2.x+r1.x; r1.x += irig.x;
		r0.y = r2.y+r1.y; r1.y += idow.x;
		r0.z = r2.z+r1.z; r1.z += ifor.x;
		for(y=ysiz-1;y>=splity;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = yv-1; yv -= ylen[x][y]; v0 = yv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(1+8+16))
					{ p = &ztab[v0->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(1+8+32))
					{ p = &ztab[v1->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
		}
	}
	xv = &voxdata[numvoxs]; f = (float)xsiz-splitx;
	r1.x += irig.x*f; r1.y += idow.x*f; r1.z += ifor.x*f;
	for(x=xsiz-1;x>=splitx;x--)
	{
		yv = xv-xlen[x];
		r1.x -= irig.x; r0.x = r1.x+r2.x;
		r1.y -= idow.x; r0.y = r1.y+r2.y;
		r1.z -= ifor.x; r0.z = r1.z+r2.z;
		for(y=ysiz-1;y>splity2;y--)
		{
			r0.x += irig.z; r0.y += idow.z; r0.z += ifor.z;
			v1 = xv-1; xv -= ylen[x][y]; v0 = xv;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+8+16))
					{ p = &ztab[v0->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+8+32))
					{ p = &ztab[v1->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
		}
		xv = yv; r0 = r1;
		for(y=0;y<=splity2;y++)
		{
			v0 = yv; yv += ylen[x][y]; v1 = yv-1;
			for(;v0<=v1 && v0->z<inz;v0++)
				if (v0->vis&~(2+4+16))
					{ p = &ztab[v0->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v0->dir]][v0->col]); }
			for(;v0<=v1;v1--)
				if (v1->vis&~(2+4+32))
					{ p = &ztab[v1->z]; drawboundsphere(p->x+r0.x,p->y+r0.y,p->z+r0.z,palookup[shadeoff[v1->dir]][v1->col]); }
			r0.x -= irig.z; r0.y -= idow.z; r0.z -= ifor.z;
		}
	}
}

void drawline2d (float x1, float y1, float x2, float y2, char col)
{
	float dx, dy, fxresm1, fyresm1;
	long i, j, incr, ie;

	dx = x2-x1; dy = y2-y1; if ((dx == 0) && (dy == 0)) return;
	fxresm1 = (float)gdd.x-1; fyresm1 = (float)gdd.y-1;
	if (x1 >= fxresm1) { if (x2 >= fxresm1) return; y1 += (fxresm1-x1)*dy/dx; x1 = fxresm1; }
	else if (x1 < 0) { if (x2 < 0) return; y1 += (0-x1)*dy/dx; x1 = 0; }
	if (x2 >= fxresm1) { y2 += (fxresm1-x2)*dy/dx; x2 = fxresm1; }
	else if (x2 < 0) { y2 += (0-x2)*dy/dx; x2 = 0; }
	if (y1 >= fyresm1) { if (y2 >= fyresm1) return; x1 += (fyresm1-y1)*dx/dy; y1 = fyresm1; }
	else if (y1 < 0) { if (y2 < 0) return; x1 += (0-y1)*dx/dy; y1 = 0; }
	if (y2 >= fyresm1) { x2 += (fyresm1-y2)*dx/dy; y2 = fyresm1; }
	else if (y2 < 0) { x2 += (0-y2)*dx/dy; y2 = 0; }

	if (fabs(dx) >= fabs(dy))
	{
		if (x2 > x1) { ftol(x1,&i); ftol(x2,&ie); } else { ftol(x2,&i); ftol(x1,&ie); }
		ftol(1048576.0*dy/dx,&incr); ftol(y1*1048576.0+((float)i+.5f-x1)*incr,&j);
		for(;i<=ie;i++,j+=incr)
			if ((unsigned long)(j>>20) < (unsigned long)gdd.y)
				*(char *)(ylookup[j>>20]+i+gdd.f) = col;
	}
	else
	{
		if (y2 > y1) { ftol(y1,&i); ftol(y2,&ie); } else { ftol(y2,&i); ftol(y1,&ie); }
		ftol(1048576.0*dx/dy,&incr); ftol(x1*1048576.0+((float)i+.5f-y1)*incr,&j);
		for(;i<=ie;i++,j+=incr)
			if ((unsigned long)(j>>20) < (unsigned long)gdd.x)
				*(char *)(ylookup[i]+(j>>20)+gdd.f) = col;
	}
}

#define SCISSORDIST 1.0
void drawline3d (float x0, float z0, float y0, float x1, float z1, float y1, char col)
{
	float ox, oy, oz, r;

		//NOTE: unusual coordinate system!
	ox = -xpiv+x0-ipos.x; oy = -zpiv+y0-ipos.y; oz = ypiv-z0-ipos.z;
	x0 = ox*irig.x + oy*irig.y + oz*irig.z;
	y0 = ox*idow.x + oy*idow.y + oz*idow.z;
	z0 = ox*ifor.x + oy*ifor.y + oz*ifor.z;

	ox = -xpiv+x1-ipos.x; oy = -zpiv+y1-ipos.y; oz = ypiv-z1-ipos.z;
	x1 = ox*irig.x + oy*irig.y + oz*irig.z;
	y1 = ox*idow.x + oy*idow.y + oz*idow.z;
	z1 = ox*ifor.x + oy*ifor.y + oz*ifor.z;

	if (z0 < SCISSORDIST)
	{
		if (z1 < SCISSORDIST) return;
		r = (SCISSORDIST-z0)/(z1-z0); z0 = SCISSORDIST;
		x0 += (x1-x0)*r; y0 += (y1-y0)*r;
	}
	else if (z1 < SCISSORDIST)
	{
		r = (SCISSORDIST-z1)/(z1-z0); z1 = SCISSORDIST;
		x1 += (x1-x0)*r; y1 += (y1-y0)*r;
	}

	ox = hz/z0; oy = hz/z1;
	drawline2d(x0*ox+hx,y0*ox+hy,x1*oy+hx,y1*oy+hy,col);
}

long getvis (long x, long y, long z)
{
	long i, j, vis;

	i = (x*MAXYSIZ+y)*BUFZSIZ+z; j = (1<<i); vis = 0;
	if ((x ==      0) || (vbit[(i-MAXYSIZ*BUFZSIZ)>>5]&j)) vis |= 1;
	if ((x == xsiz-1) || (vbit[(i+MAXYSIZ*BUFZSIZ)>>5]&j)) vis |= 2;
	if ((y ==      0) || (vbit[(i        -BUFZSIZ)>>5]&j)) vis |= 4;
	if ((y == ysiz-1) || (vbit[(i        +BUFZSIZ)>>5]&j)) vis |= 8;
	if ((z ==      0) || (vbit[(i    -1)>>5]&_lrotr(j,1))) vis |= 16;
	if ((z == zsiz-1) || (vbit[(i    +1)>>5]&_lrotl(j,1))) vis |= 32;
	return(vis);
}

void updatereflects (float px, float py, float pz)
{
	float f, ff, fx, fy, fz, vx, vy, vz, ax, ay, az;
	long i, j;

	f = 1.0 / sqrt(px*px + py*py + pz*pz);
	fx = -px*f; fy = pz*f; fz = -py*f;

	f = 1.0 / sqrt(ipos.x*ipos.x + ipos.y*ipos.y + ipos.z*ipos.z);
	vx = -ipos.x*f; vy = ipos.z*f; vz = -ipos.y*f;

	for(i=255;i>=0;i--)
	{
			//Position independent light
		f = equivec.p[i].x*fx + equivec.p[i].y*fy + equivec.p[i].z*fz;

			//Position dependent light (didn't seem to do anything)
#if 0
		if (*(long *)&f > 0)
		{
			ff = equivec.p[i].x*vx + equivec.p[i].y*vy + equivec.p[i].z*vz; ff += ff;
			ax = equivec.p[i].x*ff - vx;
			ay = equivec.p[i].y*ff - vy;
			az = equivec.p[i].z*ff - vz;
			ff = ax*fx + ay*fy + az*fz;
			if (*(long *)&ff > 0) { ff *= f; ff *= ff; f += ff*ff*2; }
		}
#endif

		ftol(f*lightmul,&j);

		shadeoff[i] = min(max(j+lightadd,0),63);
	}
}

void disablereflects ()
{
	long i;

	for(i=255;i>=0;i--) shadeoff[i] = 48;
}

static long bitnum[128] =
{
	0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
	3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
};
static long bitsum[128] =
{
	0,-3,-2,-5,-1,-4,-3,-6, 0,-3,-2,-5,-1,-4,-3,-6,
	1,-2,-1,-4, 0,-3,-2,-5, 1,-2,-1,-4, 0,-3,-2,-5,
	2,-1, 0,-3, 1,-2,-1,-4, 2,-1, 0,-3, 1,-2,-1,-4,
	3, 0, 1,-2, 2,-1, 0,-3, 3, 0, 1,-2, 2,-1, 0,-3,
	3, 0, 1,-2, 2,-1, 0,-3, 3, 0, 1,-2, 2,-1, 0,-3,
	4, 1, 2,-1, 3, 0, 1,-2, 4, 1, 2,-1, 3, 0, 1,-2,
	5, 2, 3, 0, 4, 1, 2,-1, 5, 2, 3, 0, 4, 1, 2,-1,
	6, 3, 4, 1, 5, 2, 3, 0, 6, 3, 4, 1, 5, 2, 3, 0
};
static long zheight[49] =
{
  -1,1,2,2,2,1,-1,
	1,2,3,3,3,2, 1,
	2,3,3,3,3,3, 2,
	2,3,3,3,3,3, 2,
	2,3,3,3,3,3, 2,
	1,2,3,3,3,2, 1,
  -1,1,2,2,2,1,-1
};
static long zmask[49] =
{
	 0, 28, 62, 62, 62, 28,  0,
	28, 62,127,127,127, 62, 28,
	62,127,127,127,127,127, 62,
	62,127,127,127,127,127, 62,
	62,127,127,127,127,127, 62,
	28, 62,127,127,127, 62, 28,
	 0, 28, 62, 62, 62, 28,  0
};

long getvdir (long x, long y, long z)
{
	float f; //, maxf, fx, fy, fz;
	long i, j, k, xx, yy, zz, ox, oy, oz;

		//Find number of solid cubes in local sphere
#if (NORMBOXSIZ == 3)
	long *zh = &zheight[3];
	ox = oy = oz = 0;
	if (((unsigned long)(y-3) >= ysiz-6) || ((unsigned long)(z-3) >= zsiz-6))
	{
		for(xx=-3;xx<=3;xx++,zh+=7)
		{
			if ((unsigned long)(x+xx) >= (unsigned long)xsiz) continue;
			for(yy=-3;yy<=3;yy++)
			{
				if ((unsigned long)(y+yy) >= (unsigned long)ysiz) continue;
				j = zh[yy];
				for(zz=-j;zz<=j;zz++)
				{
					if ((unsigned long)(z+zz) >= (unsigned long)zsiz) continue;
					i = ((x+xx)*MAXYSIZ+(y+yy))*BUFZSIZ+(z+zz);
					if (!(vbit[i>>5]&(1<<i))) { ox += xx; oy += yy; oz += zz; }
				}
			}
		}
	}
	else
	{
		long tx, *zm = &zmask[3];
		char *cbit = (char *)vbit;
		i = ((x-3)*MAXYSIZ+(y-3))*BUFZSIZ+(z-3);
		for(xx=-3;xx<=3;xx++,i+=MAXYSIZ*BUFZSIZ,zm+=7)
		{
			if ((unsigned long)(x+xx) >= (unsigned long)xsiz) continue;
			tx = 0;
			for(yy=-3,j=i;yy<=3;yy++,j+=BUFZSIZ)
			{
				k = (~((*(long *)&cbit[j>>3])>>(j&7)))&zm[yy];
				tx += bitnum[k];
				oy += bitnum[k]*yy;
				oz += bitsum[k];
			}
			ox += tx*xx;
		}
	}
#else
	ox = oy = oz = 0;
	for(xx=-NORMBOXSIZ;xx<=NORMBOXSIZ;xx++)
	{
		if ((unsigned long)(x+xx) >= (unsigned long)xsiz) continue;
		for(yy=-NORMBOXSIZ;yy<=NORMBOXSIZ;yy++)
		{
			if ((unsigned long)(y+yy) >= (unsigned long)ysiz) continue;
			for(zz=-NORMBOXSIZ;zz<=NORMBOXSIZ;zz++)
			{
				if ((unsigned long)(z+zz) >= (unsigned long)zsiz) continue;
				if (xx*xx+yy*yy+zz*zz >= (NORMBOXSIZ+1)*(NORMBOXSIZ+1)) continue;

				i = ((x+xx)*MAXYSIZ+(y+yy))*BUFZSIZ+(z+zz);
				if (!(vbit[i>>5]&(1<<i))) { ox += xx; oy += yy; oz += zz; }
			}
		}
	}
#endif

		//If voxels aren't directional (thin), return the 0 vector (no direction)
	f = ox*ox+oy*oy+oz*oz; if (f < 32*32) return(255);
#if 1
	f = 1 / sqrt(f); //NOTE: don't use rsqrtss: sqrt must be accurate here
	return(equivec2indmem(ox*f,oy*f,oz*f));
#else
	fx = ox; fy = oy; fz = oz; maxf = -1e32; j = 0;
	for(i=0;i<255;i++)
	{
		f = equivec.p[i].x*fx + equivec.p[i].y*fy + equivec.p[i].z*fz;
		if (f > maxf) { maxf = f; j = i; }
	}
	return(j);
#endif
}

void rendercubehack (long x0, long y0, long z0, long x1, long y1, long z1)
{
	voxtype *vptr, *vptr2;
	float ox, oy, oz, fx, fy, fz;
	long x, y, sx, sy, ofpumode, oslcol[3];
	char col;

	switch ((framecnt>>3)&3)
	{
		case 0: return;
		case 1: col = whitecol; break;
		case 2: return;
		case 3: col = blackcol; break;
	}

	for(x=0;x<3;x++) { oslcol[x] = slcol[x][col]; slcol[x][col] = lcol[col]; }

	ofpumode = fstcw(); fldcw((ofpumode&~0xf00)|0x800); //24 bit, round to �

	vptr = voxdata;
	for(x=0;x<xsiz;x++)
	{
		if ((x < x0) || (x > x1)) { vptr += xlen[x]; continue; }
		for(y=0;y<ysiz;y++)
		{
			if ((y < y0) || (y > y1)) { vptr += ylen[x][y]; continue; }
			vptr2 = &vptr[ylen[x][y]];
			for(;vptr<vptr2;vptr++)
				if ((vptr->z >= z0) && (vptr->z <= z1))
				{
						//NOTE: unusual coordinate system!
					ox = -xpiv      +x-ipos.x-.5;
					oy = -zpiv+vptr->z-ipos.y-.5;
					oz =  ypiv      -y-ipos.z-.5;
					fx = ox*irig.x + oy*irig.y + oz*irig.z;
					fy = ox*idow.x + oy*idow.y + oz*idow.z;
					fz = ox*ifor.x + oy*ifor.y + oz*ifor.z;
					drawcube(fx,fy,fz,col,vptr->vis);
				}
		}
	}

	fldcw(ofpumode); //restore fpumode to value at beginning of function

	for(x=0;x<3;x++) slcol[x][col] = oslcol[x];
}

void setfaceshademode (long damode)
{
	long i, j, k, x, y, z;

	if (damode == 0)
	{
		for(i=0;i<256;i++) slcol[0][i] = slcol[1][i] = slcol[2][i] = lcol[i];
		return;
	}
	for(i=0;i<256;i++) slcol[1][i] = lcol[i];
		for(j=0;j<=2;j+=2)
			for(i=0;i<256;i++)
				for(k=64+(j-1)*8;(k>=0)&&(k<128);k+=j-1)
				{
					x = min(max((fipalette[i*3  ]*k)>>6,0),63);
					y = min(max((fipalette[i*3+1]*k)>>6,0),63);
					z = min(max((fipalette[i*3+2]*k)>>6,0),63);
					slcol[j][i] = lcol[closestcol[(x<<12)+(y<<6)+z]];
					if (slcol[j][i] != lcol[i]) break;
				}
}

void calcallvis () //Refresh all .vis's
{
	voxtype *v, *ve;
	long x, y;

	v = voxdata; //Refresh all .vis's
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
			for(ve=v+ylen[x][y];v<ve;v++)
				v->vis = getvis(x,y,v->z);
}

void calcalldir () //Refresh all .dir's
{
	voxtype *v, *ve;
	long x, y;

	v = voxdata;
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
			for(ve=v+ylen[x][y];v<ve;v++)
				v->dir = (char)getvdir(x,y,v->z);
}

//FLOODFILL2D/3D begins -----------------------------------------------------

#define FILLBUFSIZ 8192
typedef struct { char x, y, z0, z1; } cpoint4d;
static cpoint4d fbuf[FILLBUFSIZ];

#ifdef _MSC_VER
static _inline long bsf (long a) { _asm bsf eax, a }
static _inline long bsr (long a) { _asm bsr eax, a }
#endif

long dntil0 (long xy, long z)
{
	//   //This line does the same thing (but slow & brute force)
	//while ((z < zsiz) && (vbit[(xy+z)>>5]&(1<<z))) z++; return(z);

	long i, *lptr = &vbit[xy>>5];

	i = (lptr[z>>5]|((1<<z)-1)); z &= ~31;
	while (i == -1)
	{
		z += 32; if (z >= zsiz) return(zsiz);
		i = lptr[z>>5];
	}
	return(bsf(~i)+z);

	//                 z=11
	//                    |
	//11110101110000100111111110011111 lptr[z>>5]
	//00000000000000000000011111111111 (1<<z)-1
	//11110101110000100111111111111111
}

long dntil1 (long xy, long z)
{
	//   //This line does the same thing (but slow & brute force)
	//while ((z < zsiz) && (!(vbit[(xy+z)>>5]&(1<<z)))) z++; return(z);

	long i, *lptr = &vbit[xy>>5];

	i = (lptr[z>>5]&(-1<<z)); z &= ~31;
	while (!i)
	{
		z += 32; if (z >= zsiz) return(zsiz);
		i = lptr[z>>5];
	}
	return(bsf(i)+z);
}

long uptil1 (long xy, long z)
{
	//   //This line does the same thing (but slow & brute force)
	//while ((z > 0) && (!(vbit[(xy+z-1)>>5]&(1<<(z-1))))) z--; return(z);

	long i, *lptr = &vbit[xy>>5];

	if (!z) return(0); //Prevent possible crash
	i = (lptr[(z-1)>>5]&(~(-1<<z))); z &= ~31;
	while (!i)
	{
		z -= 32; if (z < 0) return(0);
		i = lptr[z>>5];
	}
	return(bsr(i)+z+1);
}

	//Set all bits in vbit from (x,y,z0) to (x,y,z1-1) to 0's
void setzrange0 (long xy, long z0, long z1)
{
	z0 += xy; z1 += xy;
	if (!((z0^z1)&~31)) { vbit[z0>>5] &= ((~(-1<<z0))|(-1<<z1)); return; }
	xy = (z0>>5);
	vbit[xy] &= (~(-1<<z0));
	for(xy++;xy<(z1>>5);xy++) vbit[xy] = 0;
	vbit[xy] &= (-1<<z1);
}

	//Set all bits in vbit from (x,y,z0) to (x,y,z1-1) to 1's
void setzrange1 (long xy, long z0, long z1)
{
	z0 += xy; z1 += xy;
	if (!((z0^z1)&~31)) { vbit[z0>>5] |= ((~(-1<<z1))&(-1<<z0)); return; }
	xy = (z0>>5);
	vbit[xy] |= (-1<<z0);
	for(xy++;xy<(z1>>5);xy++) vbit[xy] = -1;
	vbit[xy] |= (~(-1<<z1));
}

voxtype *getvptr(long, long, long);
void deletevoxel(long, long, long);
void insertvoxel(long, long, long, char);
long ff2dget (long x, long y, long z)
{
	 voxtype *v;
	 long i;

	 i = (x*MAXYSIZ+y)*BUFZSIZ+z; if (vbit[i>>5]&(1<<i)) return(-1);
	 if (v = getvptr(x,y,z)) return((long)v->col); else return(-2);
}

void ff2dset (long x, long y, long z, long dacol)
{
	voxtype *v;
	long i;

	i = (x*MAXYSIZ+y)*BUFZSIZ+z;
	if (dacol < 0) { if (!(vbit[i>>5]&(1<<i))) deletevoxel(x,y,z); return; }
	if (vbit[i>>5]&(1<<i)) { insertvoxel(x,y,z,dacol); return; }
	if (v = getvptr(x,y,z)) { v->col = dacol; return; }
	//can't set (interior voxel!)
}

	//Note: the follow parameter MUST be 1 or 0, for example:
	//floodfill2d(x,y,12,13,0); fills color 12, boundary is color 13
	//   If there is a leak in the boundary, this will destroy the whole image
	//floodfill2d(x,y,12,13,1); fills color 12, follows color 13
	//   This is safer because it only follows a single color
	//axis: 0=yz, 1=xz, 2=xy
void floodfill2d (long x, long y, long z, long axis, long colset, long coltest, long follow)
{
	long c, col;

	//NOTE: -2 from ff2dget must ALWAYS be a boundary or else lockup!
	//P.S.: Don't bother optimizing this for slabs: harder than it looks :/

	if ((ff2dget(x,y,z) == coltest) != follow) return;
	c = 0; goto floodfill2dskip;
	do
	{
		x = fbuf[c].x; y = fbuf[c].y; z = fbuf[c].z0;
floodfill2dskip:;
		if (axis != 0)
		{
			if (x > 0)
			{
				col = ff2dget(x-1,y,z);
				if ((col != -2) && ((col == coltest) == follow))
				{
					fbuf[c].x = x-1; fbuf[c].y = y; fbuf[c].z0 = z;
					ff2dset(x-1,y,z,colset); c++; if (c >= FILLBUFSIZ) return;
				}
			}
			if (x < xsiz-1)
			{
				col = ff2dget(x+1,y,z);
				if ((col != -2) && ((col == coltest) == follow))
				{
					fbuf[c].x = x+1; fbuf[c].y = y; fbuf[c].z0 = z;
					ff2dset(x+1,y,z,colset); c++; if (c >= FILLBUFSIZ) return;
				}
			}
		}
		if (axis != 1)
		{
			if (y > 0)
			{
				col = ff2dget(x,y-1,z);
				if ((col != -2) && ((col == coltest) == follow))
				{
					fbuf[c].x = x; fbuf[c].y = y-1; fbuf[c].z0 = z;
					ff2dset(x,y-1,z,colset); c++; if (c >= FILLBUFSIZ) return;
				}
			}
			if (y < ysiz-1)
			{
				col = ff2dget(x,y+1,z);
				if ((col != -2) && ((col == coltest) == follow))
				{
					fbuf[c].x = x; fbuf[c].y = y+1; fbuf[c].z0 = z;
					ff2dset(x,y+1,z,colset); c++; if (c >= FILLBUFSIZ) return;
				}
			}
		}
		if (axis != 2)
		{
			if (z > 0)
			{
				col = ff2dget(x,y,z-1);
				if ((col != -2) && ((col == coltest) == follow))
				{
					fbuf[c].x = x; fbuf[c].y = y; fbuf[c].z0 = z-1;
					ff2dset(x,y,z-1,colset); c++; if (c >= FILLBUFSIZ) return;
				}
			}
			if (z < zsiz-1)
			{
				col = ff2dget(x,y,z+1);
				if ((col != -2) && ((col == coltest) == follow))
				{
					fbuf[c].x = x; fbuf[c].y = y; fbuf[c].z0 = z+1;
					ff2dset(x,y,z+1,colset); c++; if (c >= FILLBUFSIZ) return;
				}
			}
		}
		c--;
	} while (c >= 0);
}

void floodfill3dbits (long x, long y, long z) //Conducts on 0's and sets to 1's
{
	long i, j, xy, z0, z1, i0, i1;
	cpoint4d a;

	xy = (x*MAXYSIZ+y)*BUFZSIZ;
	if (vbit[(xy+z)>>5]&(1<<z)) return;
	a.x = x; a.z0 = uptil1(xy,z);
	a.y = y; a.z1 = dntil1(xy,z+1);
	setzrange1(xy,a.z0,a.z1);
	i0 = i1 = 0; goto floodfill3dskip;
	do
	{
		a = fbuf[i0]; i0 = ((i0+1)&(FILLBUFSIZ-1));
floodfill3dskip:;
		for(j=3;j>=0;j--)
		{
			if (j&1) { x = a.x+(j&2)-1; if ((unsigned long)x >= xsiz) continue; y = a.y; }
				 else { y = a.y+(j&2)-1; if ((unsigned long)y >= ysiz) continue; x = a.x; }
			xy = (x*MAXYSIZ+y)*BUFZSIZ;

			if (vbit[(xy+a.z0)>>5]&(1<<a.z0)) { z0 = dntil0(xy,a.z0); z1 = z0; }
												  else { z0 = uptil1(xy,a.z0); z1 = a.z0; }
			while (z1 < a.z1)
			{
				z1 = dntil1(xy,z1);
				fbuf[i1].x = x; fbuf[i1].y = y;
				fbuf[i1].z0 = z0; fbuf[i1].z1 = z1; i1 = ((i1+1)&(FILLBUFSIZ-1));
				setzrange1(xy,z0,z1);
				z0 = dntil0(xy,z1); z1 = z0;
			}
		}
	} while (i0 != i1);
}

//FLOODFILL2D/3D ends -------------------------------------------------------

void hollowfill ()
{
	voxtype *v, *ve, *v2;
	long i, x, y, z;

	clearbufbyte(vbit,sizeof(vbit),0);

	v = voxdata;
	for(x=0;x<xsiz;x++) //Set surface voxels to 1 else 0
		for(y=0;y<ysiz;y++)
		{
			z = (x*MAXYSIZ+y)*BUFZSIZ;
			for(ve=v+ylen[x][y];v<ve;v++)
				vbit[(v->z+z)>>5] |= (1<<(v->z+z));
		}

		//Set all seeable 0's to 1's
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
			{ floodfill3dbits(x,y,0L); floodfill3dbits(x,y,zsiz-1); }
	for(x=0;x<xsiz;x++)
		for(z=0;z<zsiz;z++)
			{ floodfill3dbits(x,0L,z); floodfill3dbits(x,ysiz-1,z); }
	for(y=0;y<ysiz;y++)
		for(z=0;z<zsiz;z++)
			{ floodfill3dbits(0L,y,z); floodfill3dbits(xsiz-1,y,z); }

	v = voxdata;
	for(x=0;x<xsiz;x++) //Set all surface voxels to 0
		for(y=0;y<ysiz;y++)
		{
			z = (x*MAXYSIZ+y)*BUFZSIZ;
			for(ve=v+ylen[x][y];v<ve;v++)
				vbit[(v->z+z)>>5] &= ~(1<<(v->z+z));
		}

	v2 = v = voxdata; //Update .vis, .dir and remove any unexposed voxels
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			//z = (x*MAXYSIZ+y)*BUFZSIZ;
			for(ve=v+ylen[x][y];v<ve;v++)
			{
				i = getvis(x,y,v->z);
				if (!i) { ylen[x][y]--; xlen[x]--; numvoxs--; continue; }
				(*v2) = (*v);
				v2->vis = i;
				v2->dir = (char)getvdir(x,y,v->z);
				v2++;
			}
		}
}

void initpal (char *dapal)
{
	long i, j, k, x, y, z;
	float f;

	for(i=0;i<256;i++)
	{
		pal[i].peRed =   (dapal[i*3  ]<<2);
		pal[i].peGreen = (dapal[i*3+1]<<2);
		pal[i].peBlue =  (dapal[i*3+2]<<2);
		pal[i].peFlags = 0x4; //PC_NOCOLLAPSE
	}
	//updatepalette(0,256);

	j = 0; k = 0;
	for(i=0;i<256;i++)
	{
		palgroup[i] = k;
		if (i <= j+1) continue;
		if ((labs(dapal[i*3+3] - dapal[i*3  ]*2 + dapal[i*3-3]) > 16) ||
			 (labs(dapal[i*3+4] - dapal[i*3+1]*2 + dapal[i*3-2]) > 16) ||
			 (labs(dapal[i*3+5] - dapal[i*3+2]*2 + dapal[i*3-1]) > 16))
			{ j = i; k++; }
	}
	palgroup[255] = -1; //Make transparent color not associated with anything

	if ((!(palgroup[  0]^palgroup[ 31])) &&
		 (!(palgroup[ 32]^palgroup[ 63])) &&
		 (!(palgroup[ 64]^palgroup[ 95])) &&
		 (!(palgroup[ 96]^palgroup[127])) &&
		 (!(palgroup[128]^palgroup[159])) &&
		 (!(palgroup[160]^palgroup[191])) &&
		 (!(palgroup[192]^palgroup[223])) &&
		 (!(palgroup[224]^palgroup[254])) && (palgroup[254] == 7))
		 palgroup[255] = -2;  //Specify Ken-style palette (8 bands of 32 colors)

		//NOTE: According to disablereflects, table #24 is defined as no change!
	if (palgroup[255] == -2)
	{
		for(i=63;i>=0;i--)
			for(j=255;j>=0;j--)
				palookup[i][j] = (j&0xe0)+min(max((j&31)+(i>>1)-24,0),31);
	}
	else
	{
		for(i=63;i>=0;i--)
		{
			f = ((float)i-48.0)/48.0 + 1.0;   //range: 0 <= f < 4/3
			for(j=255;j>=0;j--)
			{
				x = min(max(dapal[j*3  ]*f,0),63);
				y = min(max(dapal[j*3+1]*f,0),63);
				z = min(max(dapal[j*3+2]*f,0),63);
				palookup[i][j] = closestcol[(x<<12)+(y<<6)+z];
			}
		}
	}

#if (BUFZSIZ != 8)
	setfaceshademode(drawoption);
#else
	drawoption = 1;
	for(i=0;i<256;i++) slcol[1][i] = lcol[i];

		//UGLY HACK MADE SPECIALLY FOR MAGIC PALETTE!
	for(i=0;i<256;i++) slcol[0][i] = lcol[i+16];
	for(i=0;i<256;i++) slcol[2][i] = lcol[i+128];
	setfaceshademode(1);
#endif
	blackcol = closestcol[0x0];
	graycol = closestcol[0x20820];
	whitecol = closestcol[0x3ffff];
	backgroundcol = closestcol[backgroundcolrgb18];
}

void updatewindowborders ()
{
	long i;

	wx1[0] = wx0[0] + (xsiz<<wz[0]);
	wy1[0] = wy0[0] + (ysiz<<wz[0]);
	wx1[1] = wx0[1] + (xsiz<<wz[1]);
	wy1[1] = wy0[1] + (zsiz<<wz[1]);
	wx1[2] = wx0[2] + (ysiz<<wz[2]);
	wy1[2] = wy0[2] + (zsiz<<wz[2]);
	for(i=0;i<2;i++)
	{
		if (wx1[i] < 1) { wx0[i] += 1-wx1[i]; wx1[i] = 1; }
		if (wy1[i] < 1) { wy0[i] += 1-wy1[i]; wy1[i] = 1; }
	}
}

void resetwindowborders ()
{
	//if (gdd.x >= 512) { wz[0] = wz[1] = wz[2] = 1; wz[3] = 3; }
	//             else { wz[0] = wz[1] = wz[2] = 0; wz[3] = 2; }
	wz[0] = wz[1] = wz[2] = 1; wz[3] = 3;

		//xsiz,ysiz
	wx0[1] = 1;
	wy1[1] = gdd.y-1;
	wx1[1] = wx0[1]+(xsiz<<wz[1]);
	wy0[1] = wy1[1]-(zsiz<<wz[1]);

		//xsiz,zsiz
	wx0[0] = wx0[1];
	wx1[0] = wx1[1];
	wy1[0] = wy0[1]-10;
	wy0[0] = wy1[0]-(ysiz<<wz[0]);

		//ysiz,zsiz
	wx0[2] = wx1[1]+10;
	wx1[2] = wx0[2]+(ysiz<<wz[2]);
	wy0[2] = wy0[1];
	wy1[2] = wy1[1];

#if 0 //Put palette bar on top-right
	wx1[3] = gdd.x;
	wx0[3] = gdd.x-(32<<wz[3]);
	wy0[3] = 0;
	wy1[3] = wy0[3]+(10<<wz[3]);
#else //Put palette bar on bottom-right
	wx1[3] = gdd.x;
	wx0[3] = gdd.x-(32<<wz[3]);
	wy1[3] = gdd.y;
	wy0[3] = wy1[3]-(8<<wz[3]);
#endif

	wndorder[0] = 0; wndorder[1] = 1; wndorder[2] = 2; wndorder[3] = 3;
}

	//    ShiftHI!                        ShiftLO!
	//0:Insert x(-1) 1:Insert x(xsiz)  2:Delete x(0)  3:Delete x(xsiz-1)
	//4:Insert y(-1) 5:Insert y(ysiz)  6:Delete y(0)  7:Delete y(ysiz-1)
	//8:Insert z(-1) 9:Insert z(zsiz) 10:Delete z(0) 11:Delete z(zsiz-1)
void changedimen (long dir)
{
	long i, j, x, y, z, xs, ys, zs, xe, ye, ze, xd, yd, zd;

		//If necessary, shift: vbit[], xlen[], ylen[][] or voxdata[]->z
	if (!(dir&1))
	{
		xs = 0; xe = xsiz;
		ys = 0; ye = ysiz;
		zs = 0; ze = zsiz;
		switch(dir)
		{
			case 0: xpiv++; xs = xsiz-1; xe = -1; j = MAXYSIZ*BUFZSIZ;
				for(y=ysiz-1;y>=0;y--)
				{
					for(x=xsiz-1;x>=0;x--) ylen[x+1][y] = ylen[x][y];
					ylen[0][y] = 0;
				}
				for(x=xsiz-1;x>=0;x--) xlen[x+1] = xlen[x]; xlen[0] = 0;
				break;
			case 2: xpiv--; xs++; j = -MAXYSIZ*BUFZSIZ;
				for(x=0;x<xsiz-1;x++)
				{
					for(y=ysiz-1;y>=0;y--) ylen[x][y] = ylen[x+1][y];
					xlen[x] = xlen[x+1];
				}
				break;
			case 4: ypiv++; ys = ysiz-1; ye = -1; j = BUFZSIZ;
				for(x=xsiz-1;x>=0;x--)
				{
					for(y=ysiz-1;y>=0;y--) ylen[x][y+1] = ylen[x][y];
					ylen[x][0] = 0;
				}
				break;
			case 6: ypiv--; ys++; j = -BUFZSIZ;
				for(x=xsiz-1;x>=0;x--)
					for(y=0;y<ysiz-1;y++) ylen[x][y] = ylen[x][y+1];
				break;
			case 8: zpiv++; zs = zsiz-1; ze = -1; j = 1;
				for(i=numvoxs-1;i>=0;i--) voxdata[i].z++;
				break;
			case 10: zpiv--; zs++; j = -1;
				for(i=numvoxs-1;i>=0;i--) voxdata[i].z--;
				break;
		}
		xd = (((xe-xs)>>31)|1);
		yd = (((ye-ys)>>31)|1);
		zd = (((ze-zs)>>31)|1);
		for(x=xs;x!=xe;x+=xd)
			for(y=ys;y!=ye;y+=yd)
				for(z=zs;z!=ze;z+=zd)
				{
					i = (x*MAXYSIZ+y)*BUFZSIZ+z;
					if (vbit[i>>5]&(1<<i)) vbit[(i+j)>>5] |=  (1<<(i+j));
											else vbit[(i+j)>>5] &= ~(1<<(i+j));
				}
	}

		//For insert operation, make sure new vbit is empty space
	if (!(dir&2))
	{
		xs = 0; xe = xsiz-1;
		ys = 0; ye = ysiz-1;
		zs = 0; ze = zsiz-1;
		switch(dir)
		{
			case 0: xe = 0; break;
			case 1: xs = xe = xsiz;
				for(y=ysiz-1;y>=0;y--) ylen[xsiz][y] = 0; xlen[xsiz] = 0; break;
			case 4: ye = 0; break;
			case 5: ys = ye = ysiz;
				for(x=xsiz-1;x>=0;x--) ylen[x][ysiz] = 0; break;
			case 8: ze = 0; break;
			case 9: zs = ze = zsiz; break;
		}
		for(x=xs;x<=xe;x++)
			for(y=ys;y<=ye;y++)
				for(z=zs;z<=ze;z++)
				{
					i = (x*MAXYSIZ+y)*BUFZSIZ+z;
					vbit[i>>5] |= (1<<i);
				}
	}

		//Actually change dimensions here!
	switch (dir>>1)
	{
		case 0: xsiz++; break;
		case 1: xsiz--; break;
		case 2: ysiz++; break;
		case 3: ysiz--; break;
		case 4: zsiz++; break;
		case 5: zsiz--; break;
	}

	updatewindowborders();
}

long optimizexdimen (void)
{
	long i, j, x, y, z;

	while (xsiz > 0)
	{
		x = xsiz-1;
		for(y=ysiz-1;y>=0;y--)
			for(z=zsiz-1;z>=0;z--)
			{
				i = ((x*MAXYSIZ)+y)*BUFZSIZ+z;
				if (!(vbit[i>>5]&(1<<i))) goto skipxmax;
			}
		changedimen(3);
	}
skipxmax:;
	j = 0;
	while (xsiz > 0)
	{
		x = 0;
		for(y=ysiz-1;y>=0;y--)
			for(z=zsiz-1;z>=0;z--)
			{
				i = ((x*MAXYSIZ)+y)*BUFZSIZ+z;
				if (!(vbit[i>>5]&(1<<i))) return(j);
			}
		changedimen(2);
		j--;
	}
	return(j);
}

long optimizeydimen (void)
{
	long i, j, x, y, z;

	while (ysiz > 0)
	{
		y = ysiz-1;
		for(x=xsiz-1;x>=0;x--)
			for(z=zsiz-1;z>=0;z--)
			{
				i = ((x*MAXYSIZ)+y)*BUFZSIZ+z;
				if (!(vbit[i>>5]&(1<<i))) goto skipymax;
			}
		changedimen(7);
	}
skipymax:;
	j = 0;
	while (ysiz > 0)
	{
		y = 0;
		for(x=xsiz-1;x>=0;x--)
			for(z=zsiz-1;z>=0;z--)
			{
				i = ((x*MAXYSIZ)+y)*BUFZSIZ+z;
				if (!(vbit[i>>5]&(1<<i))) return(j);
			}
		changedimen(6);
		j--;
	}
	return(j);
}

long optimizezdimen (void)
{
	long i, j, x, y, z;

	while (zsiz > 0)
	{
		z = zsiz-1;
		for(x=xsiz-1;x>=0;x--)
			for(y=ysiz-1;y>=0;y--)
			{
				i = ((x*MAXYSIZ)+y)*BUFZSIZ+z;
				if (!(vbit[i>>5]&(1<<i))) goto skipzmax;
			}
		changedimen(11);
	}
skipzmax:;
	j = 0;
	while (zsiz > 0)
	{
		z = 0;
		for(x=xsiz-1;x>=0;x--)
			for(y=ysiz-1;y>=0;y--)
			{
				i = ((x*MAXYSIZ)+y)*BUFZSIZ+z;
				if (!(vbit[i>>5]&(1<<i))) return(j);
			}
		changedimen(10);
		j--;
	}
	return(j);
}

void checkpalimito64 (char *dapal)
{
	long i;
	for(i=767;i>=0;i--) if (dapal[i]&0xc0) break;
	if (i >= 0) { for(i=767;i>=0;i--) dapal[i] = (char)(((unsigned char)dapal[i])>>2); }
}

long loadkv6 (char *filnam)
{
	long i, j, x, y, z;
	unsigned short u;
	voxtype *vptr, *vend;
	FILE *fil;

	if (!(fil = fopen(filnam,"rb"))) return(0);
	fread(&i,4,1,fil); if (i != 0x6c78764b) return(0); //Kvxl
	fread(&xsiz,4,1,fil); fread(&ysiz,4,1,fil); fread(&zsiz,4,1,fil);
	if ((xsiz > MAXXSIZ) || (ysiz > MAXYSIZ) || (zsiz > LIMZSIZ)) { fclose(fil); return(0); }
	fread(&xpiv,4,1,fil); fread(&ypiv,4,1,fil); fread(&zpiv,4,1,fil);
	fread(&numvoxs,4,1,fil);

		//Added 06/30/2007: suggested palette (optional)
	fseek(fil,32+numvoxs*8+(xsiz<<2)+((xsiz*ysiz)<<1),SEEK_SET);
	fread(&i,4,1,fil);
	if (i == 0x6c615053) //SPal
	{
		fread(fipalette,768,1,fil);
		initclosestcolorfast(fipalette);
		fseek(fil,32,SEEK_SET);
		goto kv6_loadrest;
	} else fseek(fil,32,SEEK_SET);

	for(i=767;i>=0;i--) if (fipalette[i]) break;
	if (i < 0)
	{
		long palnum = 0;
		memset(closestcol,-1,sizeof(closestcol));
		for(vptr=voxdata,vend=&voxdata[numvoxs];vptr<vend;vptr++)
		{
			fread(&i,4,1,fil);
			i = ((i>>6)&0x3f000)+((i>>4)&0xfc0)+((i>>2)&0x3f);
			if (closestcol[i] != 255) vptr->col = closestcol[i];
			else if (palnum < 256)
			{
				fipalette[palnum*3+0] = ((i>>12)&63);
				fipalette[palnum*3+1] = ((i>> 6)&63);
				fipalette[palnum*3+2] = ( i     &63);
				vptr->col = palnum;
				closestcol[i] = palnum;
				palnum++;
			}
			else vptr->col = 255;

			vptr->z = fgetc(fil); fgetc(fil);
			vptr->vis = fgetc(fil); vptr->dir = fgetc(fil);
		}
		initclosestcolorfast(fipalette);
	}
	else
	{
kv6_loadrest:;
		for(vptr=voxdata,vend=&voxdata[numvoxs];vptr<vend;vptr++)
		{
			fread(&i,4,1,fil); vptr->col = closestcol[((i>>6)&0x3f000)+((i>>4)&0xfc0)+((i>>2)&0x3f)];
			vptr->z = fgetc(fil); fgetc(fil);
			vptr->vis = fgetc(fil); vptr->dir = fgetc(fil);
		}
	}
	for(x=0;x<xsiz;x++) { fread(&i,4,1,fil); xlen[x] = i; }
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++) { fread(&u,2,1,fil); ylen[x][y] = u; }
	fclose(fil);

	memset(vbit,-1,sizeof(vbit));
	vptr = voxdata;
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			j = ((x*MAXYSIZ)+y)*BUFZSIZ;
			vend = &vptr[ylen[x][y]];
			for(;vptr<vend;vptr++)
			{
				if (vptr->vis&16) z = vptr->z;
				if (vptr->vis&32) setzrange0(j,z,vptr->z+1);
			}
		}

	return(1);
}

void savekv6 (char *filnam)
{
	FILE *fil;
	long i, x, y;
	unsigned short u;
	voxtype *vptr, *vend;

	optimizexdimen();
	optimizeydimen();
	optimizezdimen();
	if (reflectmode < 0) { calcalldir(); reflectmode = 0; }

	if (!(fil = fopen(filnam,"wb"))) return;
	i = 0x6c78764b; fwrite(&i,4,1,fil); //Kvxl
	fwrite(&xsiz,4,1,fil); fwrite(&ysiz,4,1,fil); fwrite(&zsiz,4,1,fil);
	fwrite(&xpiv,4,1,fil); fwrite(&ypiv,4,1,fil); fwrite(&zpiv,4,1,fil);
	fwrite(&numvoxs,4,1,fil);
	for(vptr=voxdata,vend=&voxdata[numvoxs];vptr<vend;vptr++)
	{
		i = ((long)vptr->col)*3;
		fputc(fipalette[i+2]<<2,fil); fputc(fipalette[i+1]<<2,fil);
		fputc(fipalette[i  ]<<2,fil); fputc(128,fil);
		fputc(vptr->z,fil); fputc(0,fil);
		fputc(vptr->vis,fil); fputc(vptr->dir,fil);
	}
	for(x=0;x<xsiz;x++) { i = xlen[x]; fwrite(&i,4,1,fil); }
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++) { u = ylen[x][y]; fwrite(&u,2,1,fil); }

		//Added 06/30/2007: suggested palette (optional)
	i = 0x6c615053; fwrite(&i,4,1,fil); //SPal
	fwrite(fipalette,768,1,fil);

	fclose(fil);
}

long loadkvx (char *filnam)
{
	long i, j, k, x, y, z, z1, z2, fidatpos;
	char header[3];
	FILE *fil;

	if (!(fil = fopen(filnam,"rb"))) return(0);

		//Load KVX file data into memory
	fread(&numbytes,4,1,fil);
	fread(&xsiz,4,1,fil);
	fread(&ysiz,4,1,fil);
	fread(&zsiz,4,1,fil);
	if ((xsiz > MAXXSIZ) || (ysiz > MAXYSIZ) || (zsiz > LIMZSIZ)) { fclose(fil); return(0); }
	fread(&i,4,1,fil); xpiv = ((float)i)/256.0;
	fread(&i,4,1,fil); ypiv = ((float)i)/256.0;
	fread(&i,4,1,fil); zpiv = ((float)i)/256.0;
	fread(xstart,(xsiz+1)<<2,1,fil);
	for(i=0;i<xsiz;i++) fread(&xyoffs[i][0],(ysiz+1)<<1,1,fil);

	fidatpos = ftell(fil);
	fseek(fil,-768,SEEK_END);
	fread(fipalette,768,1,fil);
	checkpalimito64(fipalette);
	initclosestcolorfast(fipalette);

		//Convert KVX data to SLAB6 format while fixing 6-face visibilities
		//Unfortunately KVX files don't have the right type of visibility info
	clearbufbyte(vbit,sizeof(vbit),0);

	fseek(fil,fidatpos,SEEK_SET);
	for(x=0;x<xsiz;x++) //Set surface voxels to 1 else 0
		for(y=0;y<ysiz;y++)
		{
			i = xyoffs[x][y+1] - xyoffs[x][y]; if (!i) continue;
			j = ((x*MAXYSIZ)+y)*BUFZSIZ;
			while (i)
			{
				fread(header,3,1,fil);
				z1 = (long)header[0]; k = (long)header[1]; i -= k+3; z2 = z1+k;
				fseek(fil,z2-z1,SEEK_CUR);
				setzrange1(j,z1,z2);
				//for(z=z1;z<z2;z++) vbit[(j+z)>>5] |= (1<<(j+z));
			}
		}

		//Set all seeable 0's to 1's
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
			{ floodfill3dbits(x,y,0L); floodfill3dbits(x,y,zsiz-1); }
	for(x=0;x<xsiz;x++)
		for(z=0;z<zsiz;z++)
			{ floodfill3dbits(x,0L,z); floodfill3dbits(x,ysiz-1,z); }
	for(y=0;y<ysiz;y++)
		for(z=0;z<zsiz;z++)
			{ floodfill3dbits(0L,y,z); floodfill3dbits(xsiz-1,y,z); }

	fseek(fil,fidatpos,SEEK_SET);
	for(x=0;x<xsiz;x++) //Set all surface voxels to 0
		for(y=0;y<ysiz;y++)
		{
			i = xyoffs[x][y+1] - xyoffs[x][y]; if (!i) continue;
			j = (x*MAXYSIZ+y)*BUFZSIZ;
			while (i)
			{
				fread(header,3,1,fil);
				z1 = (long)header[0]; k = (long)header[1]; i -= k+3; z2 = z1+k;
				fseek(fil,z2-z1,SEEK_CUR);
				setzrange0(j,z1,z2);
				//for(z=z1;z<z2;z++) vbit[(j+z)>>5] &= ~(1<<(j+z));
			}
		}

		//Finally convert to SLAB6 format, now that visibility info is made
	numvoxs = 0; fseek(fil,fidatpos,SEEK_SET);
	for(x=0;x<xsiz;x++)
	{
		xlen[x] = 0;
		for(y=0;y<ysiz;y++)
		{
			ylen[x][y] = 0;
			i = xyoffs[x][y+1] - xyoffs[x][y]; if (!i) continue;
			j = (x*MAXYSIZ+y)*BUFZSIZ;
			do
			{
				fread(header,3,1,fil);
				z1 = (long)header[0]; k = (long)header[1]; i -= k+3; z2 = z1+k;
				for(z=z1;z<z2;z++)
				{
					voxdata[numvoxs].z = z;
					voxdata[numvoxs].col = fgetc(fil);
					voxdata[numvoxs].vis = getvis(x,y,z);
					//voxdata[numvoxs].dir = getvdir(x,y,z);
					ylen[x][y]++; numvoxs++;
				}
			} while (i);
			xlen[x] += ylen[x][y];
		}
	}

	fclose(fil);
	return(1);
}

	//Note: Doesn't store lower MIP-levels (don't bother with it: smaller file!)
void savekvx (char *filnam, long numips)
{
	FILE *fil;
	voxtype *vptr, *vend;
	long i, j, k, l, m, oneupm, seekpos, x, y, z, xx, yy, zz, xxx, yyy, zzz;
	long nxsiz, nysiz, nzsiz, vis, r, g, b, n;
	char voxdat[LIMZSIZ+3];

	optimizexdimen();
	optimizeydimen();
	optimizezdimen();

	nxsiz = xsiz; nysiz = ysiz; nzsiz = zsiz;

	if (!(fil = fopen(filnam,"wb"))) return;
	for(m=0;m<numips;m++)
	{
		oneupm = (1<<m);

		seekpos = ftell(fil);
		i = 0; fwrite(&i,4,1,fil);
		fwrite(&nxsiz,4,1,fil);
		fwrite(&nysiz,4,1,fil);
		fwrite(&nzsiz,4,1,fil);
		i = (((long)((float)xpiv*256.0))>>m); fwrite(&i,4,1,fil);
		i = (((long)((float)ypiv*256.0))>>m); fwrite(&i,4,1,fil);
		i = (((long)((float)zpiv*256.0))>>m); fwrite(&i,4,1,fil);
		i = ((nxsiz+1)<<2) + (((nysiz+1)*nxsiz)<<1); //xoffset+xyoffset table sizes
		fseek(fil,i,SEEK_CUR);

		vptr = voxdata;  //SLAB6 array (input)
		for(x=0;x<nxsiz;x++)
		{
			xstart[x] = i;
			for(y=0;y<nysiz;y++)
			{
				xyoffs[x][y] = (short)(i-xstart[x]);
				j = 0;
#if 0
					// Input: (vptr,vend): z, col, vis
					//Output: (voxdat[0..j-1]): z0, zleng, vis, col[zleng]
				vend = &vptr[ylen[x][y]];
				for(;vptr<vend;vptr++)
				{
					if ((!j) || (vptr->z > voxdat[0]+voxdat[1]))
					{
						if (j) { fwrite(voxdat,j,1,fil); i += j; }
						voxdat[0] = vptr->z; voxdat[1] = voxdat[2] = 0; j = 3;
					}
					voxdat[1]++; voxdat[2] |= vptr->vis; voxdat[j++] = vptr->col;
				}
#else
				k = ((x*MAXYSIZ)+y)*BUFZSIZ; xxx = (x<<m); yyy = (y<<m);
				for(z=0;z<nzsiz;z++)
				{
					l = (1<<z); if (vbit[(k+z)>>5]&l) continue;

						//Get vis
					vis = 0;
					if ((x ==       0) || (vbit[(k+z-MAXYSIZ*BUFZSIZ)>>5]&l)) vis |= 1;
					if ((x == nxsiz-1) || (vbit[(k+z+MAXYSIZ*BUFZSIZ)>>5]&l)) vis |= 2;
					if ((y ==       0) || (vbit[(k+z        -BUFZSIZ)>>5]&l)) vis |= 4;
					if ((y == nysiz-1) || (vbit[(k+z        +BUFZSIZ)>>5]&l)) vis |= 8;
					if ((z ==       0) || (vbit[(k+z    -1)>>5]&_lrotr(l,1))) vis |= 16;
					if ((z == nzsiz-1) || (vbit[(k+z    +1)>>5]&_lrotl(l,1))) vis |= 32;
					if (!vis) continue;

						//Get col
					r = g = b = n = 0; zzz = (z<<m);
					for(xx=min(xxx+oneupm,xsiz)-1;xx>=xxx;xx--)
						for(yy=min(yyy+oneupm,ysiz)-1;yy>=yyy;yy--)
							for(zz=min(zzz+oneupm,zsiz)-1;zz>=zzz;zz--)
							{
								vptr = getvptr(xx,yy,zz); if (!vptr) continue;
								r += (long)fipalette[((long)vptr->col)*3  ];
								g += (long)fipalette[((long)vptr->col)*3+1];
								b += (long)fipalette[((long)vptr->col)*3+2];
								n++;
							}
					if (n)
					{
						r = (r+(n>>1))/n;
						g = (g+(n>>1))/n;
						b = (b+(n>>1))/n;
					}

					if ((!j) || (z > voxdat[0]+voxdat[1]))
					{
						if (j) { fwrite(voxdat,j,1,fil); i += j; }
						voxdat[0] = z; voxdat[1] = voxdat[2] = 0; j = 3;
					}
					voxdat[1]++; voxdat[2] |= vis; voxdat[j++] = closestcol[(r<<12)+(g<<6)+b];
				}
#endif
				if (j) { fwrite(voxdat,j,1,fil); i += j; }
			}
			xyoffs[x][y] = (short)(i-xstart[x]);
		}
		xstart[x] = i;

		x = ftell(fil);
		fseek(fil,seekpos+28,SEEK_SET);
		fwrite(xstart,(nxsiz+1)<<2,1,fil);
		for(i=0;i<nxsiz;i++) fwrite(&xyoffs[i][0],(nysiz+1)<<1,1,fil);

		fseek(fil,seekpos,SEEK_SET);
		i = xstart[nxsiz]+24; fwrite(&i,4,1,fil);
		fseek(fil,x,SEEK_SET);

		if (m < numips-1) //Halve vbit in all dimensions (which corrupts it)
		{
			if (nxsiz&1)
			{
				for(y=0;y<nysiz;y++)
					for(z=0;z<nzsiz;z++)
					{
						x = nxsiz-1;
						if (vbit[((x*MAXYSIZ+y)*BUFZSIZ+z)>>5]&(1<<z)) vbit[((nxsiz*MAXYSIZ+y)*BUFZSIZ+z)>>5] |= (1<<z);
																				else vbit[((nxsiz*MAXYSIZ+y)*BUFZSIZ+z)>>5] &= ~(1<<z);
					}
				nxsiz++;
			}
			if (nysiz&1)
			{
				for(x=0;x<nxsiz;x++)
					for(z=0;z<nzsiz;z++)
					{
						y = nysiz-1;
						if (vbit[((x*MAXYSIZ+y)*BUFZSIZ+z)>>5]&(1<<z)) vbit[((x*MAXYSIZ+nysiz)*BUFZSIZ+z)>>5] |= (1<<z);
																				else vbit[((x*MAXYSIZ+nysiz)*BUFZSIZ+z)>>5] &= ~(1<<z);
					}
				nysiz++;
			}
			if (nzsiz&1)
			{
				for(x=0;x<nxsiz;x++)
					for(y=0;y<nysiz;y++)
					{
						z = nzsiz-1;
						if (vbit[((x*MAXYSIZ+y)*BUFZSIZ+z)>>5]&(1<<z)) vbit[((x*MAXYSIZ+y)*BUFZSIZ+nzsiz)>>5] |= (1<<nzsiz);
																				else vbit[((x*MAXYSIZ+y)*BUFZSIZ+nzsiz)>>5] &= ~(1<<nzsiz);
					}
				nzsiz++;
			}

			nxsiz >>= 1; nysiz >>= 1; nzsiz >>= 1;
			for(x=0;x<nxsiz;x++)
				for(y=0;y<nysiz;y++)
				{
					l = (x*MAXYSIZ+y)*BUFZSIZ; k = (l<<1);
					for(z=0;z<nzsiz;z++)
					{
						j = 0; //# air voxels
						if (vbit[(k+(z<<1)                        )>>5]&(1<<(z<<1))) j++;
						if (vbit[(k+(z<<1)                        )>>5]&(2<<(z<<1))) j++;
						if (vbit[(k+(z<<1)+BUFZSIZ                )>>5]&(1<<(z<<1))) j++;
						if (vbit[(k+(z<<1)+BUFZSIZ                )>>5]&(2<<(z<<1))) j++;
						if (vbit[(k+(z<<1)        +MAXYSIZ*BUFZSIZ)>>5]&(1<<(z<<1))) j++;
						if (vbit[(k+(z<<1)        +MAXYSIZ*BUFZSIZ)>>5]&(2<<(z<<1))) j++;
						if (vbit[(k+(z<<1)+BUFZSIZ+MAXYSIZ*BUFZSIZ)>>5]&(1<<(z<<1))) j++;
						if (vbit[(k+(z<<1)+BUFZSIZ+MAXYSIZ*BUFZSIZ)>>5]&(2<<(z<<1))) j++;
						if (j >= 5) vbit[(l+z)>>5] |= (1<<z);
								 else vbit[(l+z)>>5] &= ~(1<<z);
					}
				}
		}
	}

	fwrite(fipalette,768,1,fil);
	fclose(fil);

	if (numips > 1) //Restore corrupted vbit
	{
		memset(vbit,-1,sizeof(vbit));
		vptr = voxdata;
		for(x=0;x<xsiz;x++)
			for(y=0;y<ysiz;y++)
			{
				j = ((x*MAXYSIZ)+y)*BUFZSIZ;
				vend = &vptr[ylen[x][y]];
				for(;vptr<vend;vptr++)
				{
					if (vptr->vis&16) z = vptr->z;
					if (vptr->vis&32) setzrange0(j,z,vptr->z+1);
				}
			}
	}
}

long loadvox (char *filnam)
{
	char ch, davis;
	long i, j, x, y, z;
	FILE *fil;

	if (!(fil = fopen(filnam,"rb"))) return(0);

	fread(&xsiz,4,1,fil);
	fread(&ysiz,4,1,fil);
	fread(&zsiz,4,1,fil);
	if ((xsiz > MAXXSIZ) || (ysiz > MAXYSIZ) || (zsiz > LIMZSIZ))
		{ fclose(fil); return(0); }

	xpiv = ((float)xsiz)*0.5;
	ypiv = ((float)ysiz)*0.5;
	zpiv = ((float)zsiz)*0.5;

	memset(vbit,-1,sizeof(vbit));
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			j = ((x*MAXYSIZ)+y)*BUFZSIZ;
			for(z=0;z<zsiz;z++)
				if (fgetc(fil) != 255) vbit[(j+z)>>5] &= ~(1<<(j+z));
		}

	fread(fipalette,768,1,fil);
	checkpalimito64(fipalette);
	initclosestcolorfast(fipalette);

		//Convert KVX data to SLAB6 format while fixing 6-face visibilities
		//Unfortunately KVX files don't have the right type of visibility info

	//Floodfill junk not done for VOX files, but should be done:(

		//Finally convert to SLAB6 format, now that visibility info is made
	numvoxs = 0; fseek(fil,12,SEEK_SET);
	
	// for(x=0;x<xsiz;x++)
	// {
	// 	xlen[x] = 0;
	// 	for(y=0;y<ysiz;y++)
	// 	{
	// 		ylen[x][y] = 0;
	// 		j = (x*MAXYSIZ+y)*BUFZSIZ;
	// 		for(z=0;z<zsiz;z++)
	// 		{
	// 			ch = fgetc(fil); if (ch == 255) continue;
	// 			davis = getvis(x,y,z); if (!davis) continue;

	// 			voxdata[numvoxs].z = z;
	// 			voxdata[numvoxs].col = ch;
	// 			voxdata[numvoxs].vis = davis;
	// 			//voxdata[numvoxs].dir = getvdir(x,y,z);

	// 			ylen[x][y]++; numvoxs++;
	// 		}
	// 		xlen[x] += ylen[x][y];
	// 	}
	// }
	fclose(fil);
	return(1);
}

//#define KENSLOGO
#ifdef KENSLOGO
static long kenslogo[(64>>5)*24] =
{
	0xfc01f003,0xff00fff8,0xfc03f007,0xff83fffe,0xfc07f007,0xff87ffff,
	0xfc0ff007,0xff87ffff,0xfc1fe003,0xff0fffff,0xfc3fc000,0xfc0ff01e,
	0xfc7f8000,0xfc0fe000,0xfcff0000,0xfc0fe000,0xfdfe0000,0xfc0fe000,
	0xfffc0000,0xfc0ff000,0xfff80000,0xfc07fff0,0xfff00000,0xfc07fffc,
	0xfff00000,0xfc03fffe,0xfff80000,0xfc00fffe,0xfffc0000,0xfc00007f,
	0xfdfe0000,0xfc00003f,0xfcff0000,0xfc00003f,0xfc7f8000,0xfc00003f,
	0xfc3fc060,0xfc07807f,0xfc1fe0f1,0xfc0fffff,0xfc0ff1ff,0xf80ffffe,
	0xfc07f1ff,0xf80ffffe,0xfc03f0ff,0xf007fffc,0xfc01f07f,0xe001fff0,
};
static char getkenslogocol (long x, long y, long z)
{
	char ch;
	if (!(kenslogo[z*2+(x>>5)]&(0x80000000>>(x&31)))) return(255);
	if (!y) return(53);
	if (y < 5) return(55);
	if ((!x) || (!z) || (x == xsiz-1) || (z == zsiz-1)) return(61);
	if (!(kenslogo[(z-1)*2+(x>>5)]&(0x80000000>>(x&31)))) return(61);
	if (!(kenslogo[(z+1)*2+(x>>5)]&(0x80000000>>(x&31)))) return(61);
	if (!(kenslogo[z*2+((x-1)>>5)]&(0x80000000>>((x-1)&31)))) return(61);
	if (!(kenslogo[z*2+((x+1)>>5)]&(0x80000000>>((x+1)&31)))) return(61);
	return(58);
}
#endif
static char paldef[24] =
	{63,63,63,48,48,63,63,48,32,63,32,24,63,24,24,32,32,63,24,63,24,63,32,63};
static int loadefaultkvx ()
{
	char ch, davis;
	long i, j, x, y, z;

	z = 0;
	for(x=0;x<24;x+=3)
		for(y=1;y<=32;y++)
		{
			fipalette[z  ] = ((paldef[x  ]*y)>>5);
			fipalette[z+1] = ((paldef[x+1]*y)>>5);
			fipalette[z+2] = ((paldef[x+2]*y)>>5);
			z += 3;
		}

#ifdef KENSLOGO
	xsiz = 64; ysiz = 6; zsiz = 24;
#else
	xsiz = 16; ysiz = 16; zsiz = 16;
#endif
	xpiv = ((float)xsiz)*.5;
	ypiv = ((float)ysiz)*.5;
	zpiv = ((float)zsiz)*.5;

	memset(vbit,-1,sizeof(vbit));
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			j = ((x*MAXYSIZ)+y)*BUFZSIZ;
			for(z=0;z<zsiz;z++)
#ifdef KENSLOGO
				if (getkenslogocol(x,y,z) != 255) vbit[(j+z)>>5] &= ~(1<<(j+z));
#else
				if ((x*2-15)*(x*2-15) + (y*2-15)*(y*2-15) + (z*2-15)*(z*2-15) < (8*2)*(8*2)) vbit[(j+z)>>5] &= ~(1<<(j+z));
#endif
		}

	initclosestcolorfast(fipalette);

		//Convert KVX data to SLAB6 format while fixing 6-face visibilities
		//Unfortunately KVX files don't have the right type of visibility info

	//Floodfill junk not done for VOX files, but should be done:(

		//Finally convert to SLAB6 format, now that visibility info is made
	numvoxs = 0;
	for(x=0;x<xsiz;x++)
	{
		xlen[x] = 0;
		for(y=0;y<ysiz;y++)
		{
			ylen[x][y] = 0;
			j = (x*MAXYSIZ+y)*BUFZSIZ;
			for(z=0;z<zsiz;z++)
			{
#ifdef KENSLOGO
				ch = getkenslogocol(x,y,z); if (ch == 255) continue;
#else
				if ((x*2-15)*(x*2-15) + (y*2-15)*(y*2-15) + (z*2-15)*(z*2-15) >= (8*2)*(8*2)) continue;
				ch = 16;
#endif
				davis = getvis(x,y,z); if (!davis) continue;

				voxdata[numvoxs].z = z;
				voxdata[numvoxs].col = ch;
				voxdata[numvoxs].vis = davis;
				//voxdata[numvoxs].dir = getvdir(x,y,z);

				ylen[x][y]++; numvoxs++;
			}
			xlen[x] += ylen[x][y];
		}
	}
	return(1);
}

void savevox (char *filnam)
{
	FILE *fil;
	long i, x, y, z;
	voxtype *vptr, *vptr2;

	optimizexdimen();
	optimizeydimen();
	optimizezdimen();

	if (!(fil = fopen(filnam,"wb"))) return;

	fwrite(&xsiz,4,1,fil);
	fwrite(&ysiz,4,1,fil);
	fwrite(&zsiz,4,1,fil);
	vptr = voxdata;
	for(x=0;x<xsiz;x++)
		for(y=0;y<ysiz;y++)
		{
			vptr2 = &vptr[ylen[x][y]];
			for(z=0;z<zsiz;z++)
			{
				if ((vptr < vptr2) && (vptr->z == z))
					{ fputc(vptr->col,fil); vptr++; }
				else
				{
					i = (x*MAXYSIZ+y)*BUFZSIZ+z;
					if (!(vbit[i>>5]&(1<<i))) fputc(0,fil);
					else fputc(255,fil);
				}
			}
		}
	fwrite(fipalette,768,1,fil);
	fclose(fil);
}

long convert2palette (char *filnam, long convertcol)
{
	FILE *fil;
	long i, x, y, z, nv;
	char newpal[768], convbuf[256];

	i = strlen(filnam); if (i < 4) return(0);
	if (!(fil = fopen(filnam,"rb"))) return(0);

		//.DAT,.PAL : palette stored at beg, range: 0-63
		//.VOX,.KVX : palette stored at end, range: 0-63
		//.PCX      : palette stored at end, range: 0-255
		//.KV6      : palette stored at end, range: 0-63, but only if 'Spal' id is found

	if (filnam[i-1] == '6') //Loads suggested palette
	{
		fread(&z,4,1,fil); if (z != 0x6c78764b) { fclose(fil); return(0); } //Kvxl
		fread(&x,4,1,fil); fread(&y,4,1,fil); fseek(fil,16,SEEK_CUR); fread(&nv,4,1,fil);
		fseek(fil,32+nv*8+(x<<2)+((x*y)<<1),SEEK_SET);
		fread(&z,4,1,fil); if (z != 0x6c615053) { fclose(fil); return(0); } //SPal
	}
	else
	{
		if ((filnam[i-1] == 'X') || (filnam[i-1] == 'x')) fseek(fil,-768,SEEK_END);
	}

	fread(newpal,768,1,fil);
	if ((filnam[i-2] == 'C') || (filnam[i-2] == 'c')) { for(i=0;i<768;i++) newpal[i] >>= 2; }
	fclose(fil);
	checkpalimito64(newpal);

	if (convertcol)
	{
			//convert all colors from fipalette[768] to newpal[768] here!
		initclosestcolorfast(newpal);
		for(i=0;i<256;i++)
		{
			x = fipalette[i*3  ];
			y = fipalette[i*3+1];
			z = fipalette[i*3+2];
			convbuf[i] = closestcol[(x<<12)+(y<<6)+z];
		}
		for(i=0;i<numvoxs;i++) voxdata[i].col = convbuf[voxdata[i].col];
	}

	memcpy(fipalette,newpal,768);
	initclosestcolorfast(fipalette);
	initpal(fipalette);

	return(1);
}

voxtype *getvptr (long x, long y, long z)
{
	long i;
	voxtype *v;

	v = voxdata;
	if ((x<<1) < xsiz) { for(i=0     ;i< x;i++) v += xlen[i]; }
	else { v += numvoxs; for(i=xsiz-1;i>=x;i--) v -= xlen[i]; }
	if ((y<<1) < ysiz) { for(i=0     ;i< y;i++) v += ylen[x][i]; }
	else { v += xlen[x]; for(i=ysiz-1;i>=y;i--) v -= ylen[x][i]; }
	for(i=ylen[x][y];i;i--,v++) if (v->z == z) return(v);
	return((voxtype *)0);
}

long hitscan (long searchx, long searchy, long *hitx, long *hity, long *hitz, long *lastmove)
{
	point3d nipos;
	float f, vdist, vx, vy, vz;
	long i, dx, dy, dz, dxi, dyi, dzi, ix, iy, iz, ixi, iyi, izi;

	if (drawmode == 1) //renderortho
	{
		return(0);
		//Doesn't work yet!!!
		//vx =  ifor.x;
		//vy = -ifor.z;
		//vz =  ifor.y;
		//
		//f = hz / (-ipos.x*ifor.x - ipos.y*ifor.y - ipos.z*ifor.z);
		//nipos.x =  (((searchx-hx+1)*irig.x + (searchy-hy)*idow.x)*f + ipos.x+xpiv+.5);
		//nipos.y = -(((searchx-hx+1)*irig.z + (searchy-hy)*idow.z)*f + ipos.z-ypiv-.5);
		//nipos.z =  (((searchx-hx+1)*irig.y + (searchy-hy)*idow.y)*f + ipos.y+zpiv+.5);
	}
	else
	{
			//Don't mess with these coordinate hacks: they were hard enough to obtain!
		vx =   (searchx-hx+1)*irig.x + (searchy-hy)*idow.x + hz*ifor.x;
		vy = -((searchx-hx+1)*irig.z + (searchy-hy)*idow.z + hz*ifor.z);
		vz =   (searchx-hx+1)*irig.y + (searchy-hy)*idow.y + hz*ifor.y;

			//Note: (ix,iy,iz) MUST be rounded towards -inf
		nipos.x = +ipos.x+xpiv+.5;
		nipos.y = -ipos.z+ypiv+.5;
		nipos.z = +ipos.y+zpiv+.5;
	}
	ix = (long)nipos.x; ixi = (vx >= 0)*2 - 1;
	iy = (long)nipos.y; iyi = (vy >= 0)*2 - 1;
	iz = (long)nipos.z; izi = (vz >= 0)*2 - 1;

	vdist = sqrt(vx*vx + vy*vy + vz*vz) * 65536.0;

	dx = dy = dz = 0x7fffffff;
	if (vx != 0)
	{
		f = vdist / fabs(vx);
		if (f < 2147483648)
			{ ftol(f,&dxi); ftol((nipos.x-(float)ix)*f,&dx); if (vx > 0) dx = dxi-dx; }
	}
	if (vy != 0)
	{
		f = vdist / fabs(vy);
		if (f < 2147483648)
			{ ftol(f,&dyi); ftol((nipos.y-(float)iy)*f,&dy); if (vy > 0) dy = dyi-dy; }
	}
	if (vz != 0)
	{
		f = vdist / fabs(vz);
		if (f < 2147483648)
			{ ftol(f,&dzi); ftol((nipos.z-(float)iz)*f,&dz); if (vz > 0) dz = dzi-dz; }
	}

	while (1)
	{
			//Check cube at: (nix,niy,niz)
		i = (ix*MAXYSIZ+iy)*BUFZSIZ+iz;
		if ((unsigned long)ix < (unsigned long)xsiz)
			if ((unsigned long)iy < (unsigned long)ysiz)
				if ((unsigned long)iz < (unsigned long)zsiz)
					if (!(vbit[i>>5]&(1<<i)))
						{ (*hitx) = ix; (*hity) = iy; (*hitz) = iz; return(1); }

		if (ixi < 0) { if (ix < 0) return(0); } else if (ix >= xsiz) return(0);
		if (iyi < 0) { if (iy < 0) return(0); } else if (iy >= ysiz) return(0);
		if (izi < 0) { if (iz < 0) return(0); } else if (iz >= zsiz) return(0);

		if ((dx < dy) && (dx < dz)) { ix += ixi; dx += dxi; (*lastmove) = (ixi>>1)+1; }
					 else if (dy < dz) { iy += iyi; dy += dyi; (*lastmove) = (iyi>>1)+3; }
									  else { iz += izi; dz += dzi; (*lastmove) = (izi>>1)+5; }
	}
	//lastmove: 0:x--  1:x++
	//          2:y--  3:y++
	//          4:z--  5:z++
}

void calcdirad (long hitx, long hity, long hitz)
{
	long i, j, ixy, x, y, z, xxyy;
	voxtype *vptr;

	j = (hitx*MAXYSIZ+hity)*BUFZSIZ+hitz;
	for(x=-NORMBOXSIZ;x<=NORMBOXSIZ;x++)
	{
		if ((unsigned long)(x+hitx) >= (unsigned long)xsiz) continue;
		for(y=-NORMBOXSIZ;y<=NORMBOXSIZ;y++)
		{
			if ((unsigned long)(y+hity) >= (unsigned long)ysiz) continue;
			xxyy = x*x + y*y; ixy = (x*MAXYSIZ+y)*BUFZSIZ+j;
			for(z=-NORMBOXSIZ;z<=NORMBOXSIZ;z++)
			{
				if ((unsigned long)(z+hitz) >= (unsigned long)zsiz) continue;
				if (z*z+xxyy >= (NORMBOXSIZ+1)*(NORMBOXSIZ+1)) continue;

				if (vbit[(z+ixy)>>5]&(1<<(z+ixy))) continue;
				vptr = getvptr(x+hitx,y+hity,z+hitz);
				if (vptr) vptr->dir = getvdir(x+hitx,y+hity,z+hitz);
			}
		}
	}
}

void deletevoxel (long hitx, long hity, long hitz)
{
	long i, j, x, y, z;
	voxtype *vptr;
	char ch;

	i = (hitx*MAXYSIZ+hity)*BUFZSIZ+hitz;
	vbit[i>>5] |= (1<<i);

	vptr = getvptr(hitx,hity,hitz);
	if (vptr)
	{
		ch = vptr->col;

		numvoxs--; xlen[hitx]--; ylen[hitx][hity]--;
		for(i=(long)(vptr-voxdata);i<numvoxs;i++)
			voxdata[i] = voxdata[i+1];
	} else ch = curcol;

	for(j=0;j<6;j++)
	{
		switch(j)
		{
			case 0: x = hitx-1; y = hity; z = hitz; break;
			case 1: x = hitx+1; y = hity; z = hitz; break;
			case 2: x = hitx; y = hity-1; z = hitz; break;
			case 3: x = hitx; y = hity+1; z = hitz; break;
			case 4: x = hitx; y = hity; z = hitz-1; break;
			case 5: x = hitx; y = hity; z = hitz+1; break;
		}

		if ((unsigned long)x >= (unsigned long)xsiz) continue;
		if ((unsigned long)y >= (unsigned long)ysiz) continue;
		if ((unsigned long)z >= (unsigned long)zsiz) continue;

		i = (x*MAXYSIZ+y)*BUFZSIZ+z;
		if (!(vbit[i>>5]&(1<<i)))
		{
			vptr = getvptr(x,y,z);
			if (vptr) { vptr->vis |= pow2char[j^1]; }
			else  //Add voxel to compressed surface
			{
				vptr = voxdata;
				for(i=0;i<x;i++) vptr += xlen[i];
				for(i=0;i<y;i++) vptr += ylen[x][i];
				for(i=ylen[x][y];i>0;i--,vptr++)
					if (vptr->z > z) break;

				numvoxs++; xlen[x]++; ylen[x][y]++;

				for(i=numvoxs-1;i>=(long)(vptr-voxdata)+1;i--)
					voxdata[i] = voxdata[i-1];
				vptr->z = z;
				vptr->col = ch;
				vptr->vis = pow2char[j^1];
				//vptr->dir = getvdir(x,y,z);
			}
		}
	}
	calcdirad(hitx,hity,hitz);
}

void insertvoxel (long hitx, long hity, long hitz, char ch)
{
	long i, j, x, y, z;
	voxtype *vptr, *vptr2;

	if (numvoxs >= MAXVOXS) return;
	if ((hitx < -1) || (hitx > xsiz)) return;
	if ((hity < -1) || (hity > ysiz)) return;
	if ((hitz < -1) || (hitz > zsiz)) return;
	if (hitx == -1) { if (xsiz >= MAXXSIZ) hitx += optimizexdimen(); if (xsiz >= MAXXSIZ) return; changedimen(0); hitx++; }
	if (hity == -1) { if (ysiz >= MAXYSIZ) hity += optimizeydimen(); if (ysiz >= MAXYSIZ) return; changedimen(4); hity++; }
	if (hitz == -1) { if (zsiz >= LIMZSIZ) hitz += optimizezdimen(); if (zsiz >= LIMZSIZ) return; changedimen(8); hitz++; }
	if (hitx == xsiz) { if (xsiz >= MAXXSIZ) hitx += optimizexdimen(); if (xsiz >= MAXXSIZ) return; changedimen(1); }
	if (hity == ysiz) { if (ysiz >= MAXYSIZ) hity += optimizeydimen(); if (ysiz >= MAXYSIZ) return; changedimen(5); }
	if (hitz == zsiz) { if (zsiz >= LIMZSIZ) hitz += optimizezdimen(); if (zsiz >= LIMZSIZ) return; changedimen(9); }

	i = (hitx*MAXYSIZ+hity)*BUFZSIZ+hitz;
	vbit[i>>5] &= ~(1<<i);

	vptr = voxdata;
	for(i=0;i<hitx;i++) vptr += xlen[i];
	for(i=0;i<hity;i++) vptr += ylen[hitx][i];
	for(i=ylen[hitx][hity];i>0;i--,vptr++)
		if (vptr->z > hitz) break;

	numvoxs++; xlen[hitx]++; ylen[hitx][hity]++;

	for(i=numvoxs-1;i>=(long)(vptr-voxdata)+1;i--)
		voxdata[i] = voxdata[i-1];
	vptr->z = hitz;
	vptr->col = ch;
	vptr->vis = 0;
	//vptr->dir = getvdir(hitx,hity,hitz);

	vptr2 = vptr;

	for(j=0;j<6;j++)
	{
		switch(j)
		{
			case 0: x = hitx-1; y = hity; z = hitz; break;
			case 1: x = hitx+1; y = hity; z = hitz; break;
			case 2: x = hitx; y = hity-1; z = hitz; break;
			case 3: x = hitx; y = hity+1; z = hitz; break;
			case 4: x = hitx; y = hity; z = hitz-1; break;
			case 5: x = hitx; y = hity; z = hitz+1; break;
		}

		if (((unsigned long)x >= (unsigned long)xsiz) ||
			 ((unsigned long)y >= (unsigned long)ysiz) ||
			 ((unsigned long)z >= (unsigned long)zsiz))
		{
			vptr2->vis |= pow2char[j];
		}
		else
		{
			i = (x*MAXYSIZ+y)*BUFZSIZ+z;
			if (!(vbit[i>>5]&(1<<i)))
			{
				vptr = getvptr(x,y,z);
				vptr->vis &= pow2mask[j^1];
				if (!vptr->vis)  //Remove voxel from compressed surface
				{
					if (vptr2 > vptr) vptr2--;  //Watch out!

					numvoxs--; xlen[x]--; ylen[x][y]--;
					for(i=(long)(vptr-voxdata);i<numvoxs;i++)
						voxdata[i] = voxdata[i+1];
				}
			}
			else vptr2->vis |= pow2char[j];
		}
	}
	calcdirad(hitx,hity,hitz);
}

static long popcount (long i)
{
	i -= ((i>>1)&0x55555555);
	i = ((i>>2)&0x33333333)+(i&0x33333333);
	i = (((i>>4)+i)&0xf0f0f0f);
	return((i*0x01010101)>>24);
}

	//Find number of solid cubes in local sphere(x,y,z,r)
long getlocalmass (long x, long y, long z, long r)
{
	long xx, yy, zz, i, k, l, x2, y2, z2, x3, y3, z3, daz, daz2, r0, r1, r2, damsk;
	r2 = r*r;

		//k = 0;
		//for(xx=x-r+1;xx<x+r;xx++)
		//   for(yy=y-r+1;yy<y+r;yy++)
		//      for(zz=z-r+1;zz<z+r;zz++)
		//         if (((unsigned long)xx < (unsigned long)xsiz) &&
		//             ((unsigned long)yy < (unsigned long)ysiz) &&
		//             ((unsigned long)zz < (unsigned long)zsiz) &&
		//             ((xx-x)*(xx-x)+(yy-y)*(yy-y)+(zz-z)*(zz-z) < r2))
		//            { i = (xx*MAXYSIZ+yy)*BUFZSIZ+zz; if (!(vbit[i>>5]&(1<<i))) k++; }
	x2 = max(x-r+1,0); x3 = min(x+r-1,xsiz-1);
	y2 = max(y-r+1,0); y3 = min(y+r-1,ysiz-1);
	z2 = max(z-r+1,0); z3 = min(z+r-1,zsiz-1);
	daz = daz2 = k = 0;
	for(xx=x2;xx<=x3;xx++)
	{
		r0 = r2-(xx-x)*(xx-x); if (r0 <= 0) continue;
		l = xx*MAXYSIZ*BUFZSIZ;
		for(yy=y2;yy<=y3;yy++)
		{
			r1 = r0-(yy-y)*(yy-y); if (r1 <= 0) continue;

				//daz2 <= r1, (daz2 = daz*daz)
			while (daz2 <= r1) { daz2 += daz; daz++; daz2 += daz; }
			while (daz2 >  r1) { daz2 -= daz; daz--; daz2 -= daz; }

			zz = max(z-daz,z2); damsk = (((1<<(min(z+daz,z3)+1-zz))-1)<<(zz&7));
			k += popcount((~(*(long *)(((long)vbit)+((yy*BUFZSIZ+l+zz)>>3))))&damsk);
		}
	}

	return(k);
}

long isvoxelsolid (long x, long y, long z)
{
	long i;

	if ((unsigned long)x >= (unsigned long)xsiz) return(0);
	if ((unsigned long)y >= (unsigned long)ysiz) return(0);
	if ((unsigned long)z >= (unsigned long)zsiz) return(0);
	i = (x*MAXYSIZ+y)*BUFZSIZ+z;
	return(!(vbit[i>>5]&(1<<i)));
}

long voxfindspray (long hitx, long hity, long hitz, long r, long *ox, long *oy, long *oz)
{
	long i, j, k, l, x, y, z, r2;
	char ch;

	j = 0; r2 = r*r;
	for(x=hitx-r;x<=hitx+r;x++)
		for(y=hity-r;y<=hity+r;y++)
			for(z=hitz-r;z<=hitz+r;z++)
			{
				if ((x-hitx)*(x-hitx)+(y-hity)*(y-hity)+(z-hitz)*(z-hitz) > r2) continue;

				l  = (((unsigned long)x < (unsigned long)xsiz)   );
				l |= (((unsigned long)y < (unsigned long)ysiz)<<1);
				l |= (((unsigned long)z < (unsigned long)zsiz)<<2);

				if ((!(l&1)) && (xsiz >= MAXXSIZ)) continue;
				if ((!(l&2)) && (ysiz >= MAXYSIZ)) continue;
				if ((!(l&4)) && (zsiz >= LIMZSIZ)) continue;

#if 1
					//(x,y,z) must be an empty voxel
				i = (x*MAXYSIZ+y)*BUFZSIZ+z;
				if ((l == 7) && (!(vbit[i>>5]&(1<<i)))) continue;

				k = 0;  //Make sure voxel is a space touching the surface!
				if (((unsigned long)(x-1) >= (unsigned long)xsiz) || ((l&6) != 6) || (vbit[(i-MAXYSIZ*BUFZSIZ)>>5]&(1<<(i-MAXYSIZ*BUFZSIZ)))) k |= 1;
				if (((unsigned long)(x+1) >= (unsigned long)xsiz) || ((l&6) != 6) || (vbit[(i+MAXYSIZ*BUFZSIZ)>>5]&(1<<(i+MAXYSIZ*BUFZSIZ)))) k |= 2;
				if (((unsigned long)(y-1) >= (unsigned long)ysiz) || ((l&5) != 5) || (vbit[(i-BUFZSIZ)>>5]&(1<<(i-BUFZSIZ)))) k |= 4;
				if (((unsigned long)(y+1) >= (unsigned long)ysiz) || ((l&5) != 5) || (vbit[(i+BUFZSIZ)>>5]&(1<<(i+BUFZSIZ)))) k |= 8;
				if (((unsigned long)(z-1) >= (unsigned long)zsiz) || ((l&3) != 3) || (vbit[(i-1)>>5]&(1<<(i-1)))) k |= 16;
				if (((unsigned long)(z+1) >= (unsigned long)zsiz) || ((l&3) != 3) || (vbit[(i+1)>>5]&(1<<(i+1)))) k |= 32;
#else
				if (isvoxelsolid(x,y,z)) continue;
				k = 0;
				if (!isvoxelsolid(x-1,y,z)) k |= 1;
				if (!isvoxelsolid(x+1,y,z)) k |= 2;
				if (!isvoxelsolid(x,y-1,z)) k |= 4;
				if (!isvoxelsolid(x,y+1,z)) k |= 8;
				if (!isvoxelsolid(x,y,z-1)) k |= 16;
				if (!isvoxelsolid(x,y,z+1)) k |= 32;
#endif
				if (k == 63) continue;


				k = getlocalmass(x,y,z,4);
				if (k > j) { j = k; (*ox) = x; (*oy) = y; (*oz) = z; }
			}

	if (j <= 0) return(-1);

	j = k = 0;  //j is value, k is counter
	if (isvoxelsolid((*ox)-1,*oy,*oz)) { j += (0<<k); k += 4; }
	if (isvoxelsolid((*ox)+1,*oy,*oz)) { j += (1<<k); k += 4; }
	if (isvoxelsolid(*ox,(*oy)-1,*oz)) { j += (2<<k); k += 4; }
	if (isvoxelsolid(*ox,(*oy)+1,*oz)) { j += (3<<k); k += 4; }
	if (isvoxelsolid(*ox,*oy,(*oz)-1)) { j += (4<<k); k += 4; }
	if (isvoxelsolid(*ox,*oy,(*oz)+1)) { j += (5<<k); k += 4; }
	switch ((j>>((rand()%k)&~3))&15)  //Select random, valid voxel
	{
		case 0: ch = getvptr((*ox)-1,*oy,*oz)->col; break;
		case 1: ch = getvptr((*ox)+1,*oy,*oz)->col; break;
		case 2: ch = getvptr(*ox,(*oy)-1,*oz)->col; break;
		case 3: ch = getvptr(*ox,(*oy)+1,*oz)->col; break;
		case 4: ch = getvptr(*ox,*oy,(*oz)-1)->col; break;
		case 5: ch = getvptr(*ox,*oy,(*oz)+1)->col; break;
	}
	return((long)ch);
}

void voxfindsuck (long hitx, long hity, long hitz, long r, long *ox, long *oy, long *oz)
{
	long i, j, k, x, y, z, x0, y0, z0, x1, y1, z1, r0, r1, r2;
	voxtype *v0, *v1, *v;

	j = 0x7fffffff; (*ox) = hitx; (*oy) = hity; (*oz) = hitz; r2 = r*r;

	x0 = max(hitx-r,0); x1 = min(hitx+r,xsiz-1);
	y0 = max(hity-r,0); y1 = min(hity+r,ysiz-1);
	z0 = max(hitz-r,0); z1 = min(hitz+r,zsiz-1);

	v0 = voxdata;
	if ((x0<<1) < xsiz) { for(i=0     ;i< x0;i++) v0 += xlen[i]; }
	else { v0 += numvoxs; for(i=xsiz-1;i>=x0;i--) v0 -= xlen[i]; }
	for(x=x0;x<=x1;v0+=xlen[x++])
	{
		r0 = r2-(x-hitx)*(x-hitx);
		v1 = v0;
		if ((y0<<1) < ysiz) { for(i=0     ;i< y0;i++) v1 += ylen[x][i]; }
		else { v1 += xlen[x]; for(i=ysiz-1;i>=y0;i--) v1 -= ylen[x][i]; }
		for(y=y0;y<=y1;v1+=ylen[x][y++])
		{
			r1 = r0-(y-hity)*(y-hity);
			for(v=v1,i=ylen[x][y];i;i--,v++)
			{
				z = v->z; if ((z-hitz)*(z-hitz) > r1) continue;
				k = getlocalmass(x,y,z,4);
				if (k < j) { j = k; (*ox) = x; (*oy) = y; (*oz) = z; }
			}
		}
	}
}

void junkcolors (long hitx, long hity, long hitz, long r)
{
	long i, j, k, x, y, z, xx, yy, zz, ox, oy, oz, r2;
	voxtype *vptr, *vptr2;
	char tbuf[8];

	r2 = r*r;
	for(x=hitx-r;x<=hitx+r;x++)
	{
		if ((unsigned long)x >= (unsigned long)xsiz) continue;
		for(y=hity-r;y<=hity+r;y++)
		{
			if ((unsigned long)y >= (unsigned long)ysiz) continue;
			for(z=hitz-r;z<=hitz+r;z++)
			{
				if ((unsigned long)z >= (unsigned long)zsiz) continue;
				if ((x-hitx)*(x-hitx)+(y-hity)*(y-hity)+(z-hitz)*(z-hitz) > r2) continue;

				i = (x*MAXYSIZ+y)*BUFZSIZ+z;
				if (vbit[i>>5]&(1<<i)) continue;

				vptr = getvptr(x,y,z); if (!vptr) continue;

				j = k = 0;  //j is value, k is counter
				if ((x !=      0) && (!(vbit[(i-MAXYSIZ*BUFZSIZ)>>5]&(1<<(i-MAXYSIZ*BUFZSIZ))))) { j += (0<<k); k += 4; }
				if ((x != xsiz-1) && (!(vbit[(i+MAXYSIZ*BUFZSIZ)>>5]&(1<<(i+MAXYSIZ*BUFZSIZ))))) { j += (1<<k); k += 4; }
				if ((y !=      0) && (!(vbit[(i-BUFZSIZ)>>5]&(1<<(i-BUFZSIZ))))) { j += (2<<k); k += 4; }
				if ((y != ysiz-1) && (!(vbit[(i+BUFZSIZ)>>5]&(1<<(i+BUFZSIZ))))) { j += (3<<k); k += 4; }
				if ((z !=      0) && (!(vbit[(i-1)>>5]&(1<<(i-1))))) { j += (4<<k); k += 4; }
				if ((z != zsiz-1) && (!(vbit[(i+1)>>5]&(1<<(i+1))))) { j += (5<<k); k += 4; }
				if ((k == 0) || (k == 24)) continue;

				xx = k-4; k = 0;
				for(;xx>=0;xx-=4)
				{
					switch ((j>>xx)&15)
					{
						case 0: vptr2 = getvptr(x-1,y,z); break;
						case 1: vptr2 = getvptr(x+1,y,z); break;
						case 2: vptr2 = getvptr(x,y-1,z); break;
						case 3: vptr2 = getvptr(x,y+1,z); break;
						case 4: vptr2 = getvptr(x,y,z-1); break;
						case 5: vptr2 = getvptr(x,y,z+1); break;
					}
					if ((vptr2) && (palgroup[vptr->col] == palgroup[vptr2->col]))
						tbuf[k++] = vptr2->col;
				}

					//Skip if voxel is already different from all neighbors
				for(i=k-1;i>=0;i--)
					if (tbuf[i] == vptr->col) break;
				if (i < 0) continue;

					//Start at average value
				xx = (long)vptr->col;
				for(i=k-1;i>=0;i--) xx += (long)tbuf[i];
				xx = (xx+((k+1)>>1))/(k+1);

					//     +1 -2 +3 -4 +5 -6 +7 -8
					//or:  -1 +2 -3 +4 -5 +6 -7 +8
				zz = (rand()&1);
				for(yy=1;yy<16;yy++)
				{
					if (palgroup[vptr->col] != palgroup[xx]) continue;
					for(i=k-1;i>=0;i--) if (tbuf[i] == xx) break;
					if (i < 0) break;
					if ((zz^yy)&1) xx += yy; else xx -= yy;
				}
				if (yy < 16) vptr->col = xx;
			}
		}
	}
}

void smoothcolors (long hitx, long hity, long hitz, long r, long smoothtype)
{
	long i, j, k, x, y, z, xx, yy, ox, oy, oz, r2;
	voxtype *vptr, *vptr2;

	r2 = r*r;
	for(x=hitx-r;x<=hitx+r;x++)
	{
		if ((unsigned long)x >= (unsigned long)xsiz) continue;
		for(y=hity-r;y<=hity+r;y++)
		{
			if ((unsigned long)y >= (unsigned long)ysiz) continue;
			for(z=hitz-r;z<=hitz+r;z++)
			{
				if ((unsigned long)z >= (unsigned long)zsiz) continue;
				if ((x-hitx)*(x-hitx)+(y-hity)*(y-hity)+(z-hitz)*(z-hitz) > r2) continue;

				i = (x*MAXYSIZ+y)*BUFZSIZ+z;
				if (vbit[i>>5]&(1<<i)) continue;

				vptr = getvptr(x,y,z); if (!vptr) continue;

				j = k = 0;  //j is value, k is counter
				if ((x !=      0) && (!(vbit[(i-MAXYSIZ*BUFZSIZ)>>5]&(1<<(i-MAXYSIZ*BUFZSIZ))))) { j += (0<<k); k += 4; }
				if ((x != xsiz-1) && (!(vbit[(i+MAXYSIZ*BUFZSIZ)>>5]&(1<<(i+MAXYSIZ*BUFZSIZ))))) { j += (1<<k); k += 4; }
				if ((y !=      0) && (!(vbit[(i-BUFZSIZ)>>5]&(1<<(i-BUFZSIZ))))) { j += (2<<k); k += 4; }
				if ((y != ysiz-1) && (!(vbit[(i+BUFZSIZ)>>5]&(1<<(i+BUFZSIZ))))) { j += (3<<k); k += 4; }
				if ((z !=      0) && (!(vbit[(i-1)>>5]&(1<<(i-1))))) { j += (4<<k); k += 4; }
				if ((z != zsiz-1) && (!(vbit[(i+1)>>5]&(1<<(i+1))))) { j += (5<<k); k += 4; }
				if ((k == 0) || (k == 24)) continue;

				xx = k-4;
				if (smoothtype == 1)
				{
					ox = (long)fipalette[vptr->col*3  ];
					oy = (long)fipalette[vptr->col*3+1];
					oz = (long)fipalette[vptr->col*3+2];
				}
				else { yy = (long)vptr->col; }
				k = 1;
				for(;xx>=0;xx-=4)
				{
					switch ((j>>xx)&15)
					{
						case 0: vptr2 = getvptr(x-1,y,z); break;
						case 1: vptr2 = getvptr(x+1,y,z); break;
						case 2: vptr2 = getvptr(x,y-1,z); break;
						case 3: vptr2 = getvptr(x,y+1,z); break;
						case 4: vptr2 = getvptr(x,y,z-1); break;
						case 5: vptr2 = getvptr(x,y,z+1); break;
					}
					if (!vptr2) continue;
					if (smoothtype == 1)
					{
						ox += (long)fipalette[vptr2->col*3  ];
						oy += (long)fipalette[vptr2->col*3+1];
						oz += (long)fipalette[vptr2->col*3+2];
						k++;
					}
					else if (palgroup[vptr->col] == palgroup[vptr2->col])
						{ yy += vptr2->col; k++; }
				}
					//Set to average value
				if (smoothtype == 1)
				{
					ox = (ox+(k>>1)) / k;
					oy = (oy+(k>>1)) / k;
					oz = (oz+(k>>1)) / k;
					vptr->col = closestcol[(ox<<12)+(oy<<6)+oz];
				}
				else
					{ vptr->col = (yy+(k>>1))/k; }
			}
		}
	}
}

void paintsphere (long hitx, long hity, long hitz, long r, long col)
{
	long i, x, y, z, r2;
	voxtype *vptr;

	r2 = r*r;
	for(x=hitx-r;x<=hitx+r;x++)
	{
		if ((unsigned long)x >= (unsigned long)xsiz) continue;
		for(y=hity-r;y<=hity+r;y++)
		{
			if ((unsigned long)y >= (unsigned long)ysiz) continue;
			for(z=hitz-r;z<=hitz+r;z++)
			{
				if ((unsigned long)z >= (unsigned long)zsiz) continue;
				if ((x-hitx)*(x-hitx)+(y-hity)*(y-hity)+(z-hitz)*(z-hitz) > r2) continue;

				i = (x*MAXYSIZ+y)*BUFZSIZ+z;
				if (vbit[i>>5]&(1<<i)) continue;

				vptr = getvptr(x,y,z);
				if (vptr) vptr->col = (char)col;
			}
		}
	}
}


static long capturecount = 0, doscreencap = 0;
static unsigned char pcxheader[128] =
{
	0xa,0x5,0x1,0x8,0x0,0x0,0x0,0x0,0x3f,0x1,0xc7,0x0,
	0x40,0x1,0xc8,0x0,0x0,0x0,0x0,0x8,0x8,0x8,0x10,0x10,
	0x10,0x18,0x18,0x18,0x20,0x20,0x20,0x28,0x28,0x28,0x30,0x30,
	0x30,0x38,0x38,0x38,0x40,0x40,0x40,0x48,0x48,0x48,0x50,0x50,
	0x50,0x58,0x58,0x58,0x60,0x60,0x60,0x68,0x68,0x68,0x70,0x70,
	0x70,0x78,0x78,0x78,0x0,0x1,0x40,0x1,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
	0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
};
static char sl6dcap[] = "SL6D0000.PCX";
void screencapture(char *filnam)
{
	FILE *fil;
	long i, x;
	char *cptr, *cend, col;

	while (1)
	{
		filnam[4] = ((capturecount/1000)%10)+48;
		filnam[5] = ((capturecount/100)%10)+48;
		filnam[6] = ((capturecount/10)%10)+48;
		filnam[7] = (capturecount%10)+48;
		fil = fopen(filnam,"rb"); if (!fil) break;
		capturecount++;
		if (capturecount > 1024) return; //Just in case the OS doesn't like you
		fclose(fil);
	}
	if (!(fil = fopen(filnam,"wb"))) return;

	*(short *)&pcxheader[8] = gdd.x-1;
	*(short *)&pcxheader[10] = gdd.y-1;
	*(short *)&pcxheader[12] = *(short *)&pcxheader[66] = gdd.x;
	*(short *)&pcxheader[14] = gdd.y;
	fwrite(pcxheader,128,1,fil);

	x = gdd.x; cptr = (char *)gdd.f; cend = &cptr[ylookup[gdd.y]];
	while (cptr < cend)
	{
		col = *cptr++; x--;
		for(i=1;i<63;i++,x--,cptr++)
		{
			if (x <= 0) { x = gdd.x; cptr += gdd.p-gdd.x; break; }
			if ((*cptr) != col) break;
		}
		if ((i > 1) || (col >= 0xc0)) fputc(i|0xc0,fil);
		fputc(col,fil);
	}
	fputc(0xc,fil);
	for(i=0;i<768;i++) fputc(fipalette[i]<<2,fil);
	fclose(fil);
}

static char ocol[MAXXDIM];
void draw2dslice (long w)
{
	long i, x, y, z, xx, yy, zz, x0, y0, x1, y1, daz;
	long bx, by, startx, dax0, day0, dax1, day1, dainc;
	char ch, col, *ptr, dabuf[64];
	voxtype *ptr2;

	x0 = wx0[w]; y0 = wy0[w];
	x1 = wx1[w]; y1 = wy1[w];
	daz = wz[w];

	if (w == 3)
	{
		for(i=0;i<256;i++)
		{
			x = ((i&31)<<daz)+x0; y = ((i>>5)<<daz)+y0;

			ch = (char)i;
			if ((framecnt&4) && (curcol == i)) ch = (framecnt&255);

			for(yy=y+(1<<daz)-1;yy>=y;yy--)
				if ((unsigned)yy < (unsigned)gdd.y)
					for(xx=x+(1<<daz)-1;xx>=x;xx--)
						if ((unsigned)xx < (unsigned)gdd.x)
							*(char *)(gdd.f+ylookup[yy]+xx) = ch;
		}
		if (((unsigned)curcol < 256) && (inawindow == 3) && (edit2dmode))
		{
			print6x8(x0     ,y0-8,whitecol           ,-1,"%3d:",curcol);
			print6x8(x0+ 5*6,y0-8,closestcol[0x3f7df],-1,"%2d",fipalette[curcol*3  ]);
			print6x8(x0+ 8*6,y0-8,closestcol[0x1ffdf],-1,"%2d",fipalette[curcol*3+1]);
			print6x8(x0+11*6,y0-8,closestcol[0x1f7ff],-1,"%2d",fipalette[curcol*3+2]);
			print6x8(x0+14*6,y0-8,whitecol           ,-1,"GAMMA:%d.%d",curgamminterp/1000,(curgamminterp/100)%10);

			x1 = min((32<<daz)+x0,gdd.x); x0 = max(x0+24*6,0);
			for(yy=y0-8;yy<y0-1;yy++)
				if ((unsigned long)yy < gdd.y)
					for(xx=x0;xx<x1;xx++)
						*(char *)(gdd.f+ylookup[yy]+xx) = curcol;
		}
		return;
	}

	if ((long)pal[backgroundcol].peRed +
		 (long)pal[backgroundcol].peGreen +
		 (long)pal[backgroundcol].peBlue < 128*3)
		col = whitecol; else col = blackcol;
	  //Draw border around window
	for(xx=wx0[w]-1;xx<=wx1[w];xx++)
	{
		yy = wy0[w]-1;
		if (((unsigned)xx < (unsigned)gdd.x) && ((unsigned)yy < (unsigned)gdd.y))
			*(char *)(gdd.f+ylookup[yy]+xx) = col;
		yy = wy1[w];
		if (((unsigned)xx < (unsigned)gdd.x) && ((unsigned)yy < (unsigned)gdd.y))
			*(char *)(gdd.f+ylookup[yy]+xx) = col;
	}
	for(yy=wy0[w];yy<wy1[w];yy++)
	{
		xx = wx0[w]-1;
		if (((unsigned)xx < (unsigned)gdd.x) && ((unsigned)yy < (unsigned)gdd.y))
			*(char *)(gdd.f+ylookup[yy]+xx) = col;
		xx = wx1[w];
		if (((unsigned)xx < (unsigned)gdd.x) && ((unsigned)yy < (unsigned)gdd.y))
			*(char *)(gdd.f+ylookup[yy]+xx) = col;
	}

	if (col == whitecol)
	{
		  //Draw border around window
		for(xx=wx0[w];xx<=wx1[w];xx++)
		{
			yy = wy1[w]+1;
			if (((unsigned)xx < (unsigned)gdd.x) && ((unsigned)yy < (unsigned)gdd.y))
				*(char *)(gdd.f+ylookup[yy]+xx) = blackcol;
		}
		for(yy=wy0[w]+1;yy<=wy1[w]+1;yy++)
		{
			xx = wx1[w]+1;
			if (((unsigned)xx < (unsigned)gdd.x) && ((unsigned)yy < (unsigned)gdd.y))
				*(char *)(gdd.f+ylookup[yy]+xx) = blackcol;
		}
	}

	dax0 = max(x0,0); dax1 = min(x1,gdd.x);
	day0 = max(y0,0); day1 = min(y1,gdd.y);
	dainc = (65536>>daz);
	by = (day0-y0) * dainc;
	startx = (dax0-x0) * dainc;

	if (adjustpivotmode)
	{
		print6x8(0,16,whitecol,-1,"Xpiv:%.2f/%ld",xpiv,xsiz);
		print6x8(0,24,whitecol,-1,"Ypiv:%.2f/%ld",ypiv,ysiz);
		print6x8(0,32,whitecol,-1,"Zpiv:%.2f/%ld",zpiv,zsiz);
	}

	switch(w)
	{
		case 0: sprintf(dabuf,"Z:%ld/%ld",viewz,zsiz); break;
		case 1: sprintf(dabuf,"Y:%ld/%ld",viewy,ysiz); break;
		case 2: sprintf(dabuf,"X:%ld/%ld",viewx,xsiz); break;
	}
	print6x8(x0,y0-8,whitecol,-1,"%s",dabuf);

	if ((dax0 >= dax1) || (day0 >= day1)) return;
	viewx = max(min(viewx,xsiz-1),0);
	viewy = max(min(viewy,ysiz-1),0);
	viewz = max(min(viewz,zsiz-1),0);

	switch(w)
	{
		case 0:
			for(y=day0;y<day1;y++)
			{
				if ((!(by&65535)) || (y == day0))
				{
					bx = startx; x = dax0; goto intoslicex;
					for(;x<dax1;x++)
					{
						if (!(bx&65535))
						{
intoslicex:;         i = ((bx>>16)*MAXYSIZ+(by>>16))*BUFZSIZ+viewz;
							if (vbit[i>>5]&(1<<i)) ocol[x] = backgroundcol;
							else
							{
								ptr2 = getvptr(bx>>16,by>>16,viewz);
								if (ptr2) ocol[x] = ptr2->col; else ocol[x] = 255;
							}
						} else ocol[x] = ocol[x-1];
						bx += dainc;
					}
				}
				memcpy((void *)(gdd.f+ylookup[y]+dax0),(void *)&ocol[dax0],dax1-dax0);
				by += dainc;
			}
			break;
		case 1:
			for(y=day0;y<day1;y++)
			{
				if ((!(by&65535)) || (y == day0))
				{
					bx = startx; x = dax0; goto intoslicey;
					for(;x<dax1;x++)
					{
						if (!(bx&65535))
						{
intoslicey:;         i = ((bx>>16)*MAXYSIZ+viewy)*BUFZSIZ+(by>>16);
							if (vbit[i>>5]&(1<<i)) ocol[x] = backgroundcol;
							else
							{
								ptr2 = getvptr(bx>>16,viewy,by>>16);
								if (ptr2) ocol[x] = ptr2->col; else ocol[x] = 255;
							}
						} else ocol[x] = ocol[x-1];
						bx += dainc;
					}
				}
				memcpy((void *)(gdd.f+ylookup[y]+dax0),(void *)&ocol[dax0],dax1-dax0);
				by += dainc;
			}
			break;
		case 2:
			for(y=day0;y<day1;y++)
			{
				if ((!(by&65535)) || (y == day0))
				{
					bx = startx; x = dax0; goto intoslicez;
					for(;x<dax1;x++)
					{
						if (!(bx&65535))
						{
intoslicez:;         i = (viewx*MAXYSIZ+(bx>>16))*BUFZSIZ+(by>>16);
							if (vbit[i>>5]&(1<<i)) ocol[x] = backgroundcol;
							else
							{
								ptr2 = getvptr(viewx,bx>>16,by>>16);
								if (ptr2) ocol[x] = ptr2->col; else ocol[x] = 255;
							}
						} else ocol[x] = ocol[x-1];
						bx += dainc;
					}
				}
				memcpy((void *)(gdd.f+ylookup[y]+dax0),(void *)&ocol[dax0],dax1-dax0);
				by += dainc;
			}
			break;
	}
	if (adjustpivotmode)
	{
		col = (rand()&255);
		switch(w)
		{
			case 0:
				for(y=y0;y<y1;y++)
					if (((unsigned)y < (unsigned)gdd.y) && ((unsigned)((long)(xpiv*(float)(1<<daz))+x0) < (unsigned)gdd.x))
						*(char *)(gdd.f+ylookup[y]+(long)(xpiv*(float)(1<<daz))+x0) = col;
				for(x=x0;x<x1;x++)
					if (((unsigned)((long)(ypiv*(float)(1<<daz))+y0) < (unsigned)gdd.y) && ((unsigned)x < (unsigned)gdd.x))
						*(char *)(gdd.f+ylookup[(long)(ypiv*(float)(1<<daz))+y0]+x) = col;
				break;
			case 1:
				for(y=y0;y<y1;y++)
					if (((unsigned)y < (unsigned)gdd.y) && ((unsigned)((long)(xpiv*(float)(1<<daz))+x0) < (unsigned)gdd.x))
						*(char *)(gdd.f+ylookup[y]+(long)(xpiv*(float)(1<<daz))+x0) = col;
				for(x=x0;x<x1;x++)
					if (((unsigned)((long)(zpiv*(float)(1<<daz))+y0) < (unsigned)gdd.y) && ((unsigned)x < (unsigned)gdd.x))
						*(char *)(gdd.f+ylookup[(long)(zpiv*(float)(1<<daz))+y0]+x) = col;
				break;
			case 2:
				for(y=y0;y<y1;y++)
					if (((unsigned)y < (unsigned)gdd.y) && ((unsigned)((long)(ypiv*(float)(1<<daz))+x0) < (unsigned)gdd.x))
						*(char *)(gdd.f+ylookup[y]+(long)(ypiv*(float)(1<<daz))+x0) = col;
				for(x=x0;x<x1;x++)
					if (((unsigned)((long)(zpiv*(float)(1<<daz))+y0) < (unsigned)gdd.y) && ((unsigned)x < (unsigned)gdd.x))
						*(char *)(gdd.f+ylookup[(long)(zpiv*(float)(1<<daz))+y0]+x) = col;
				break;
		}
	}
}

void axisflip3d (long axes)
{
	voxtype bvox, *vptr, *vptr2;
	long i, j, x, y, z;

	switch(axes)
	{
		case 0:
			vptr = voxdata;
			for(x=0;x<xsiz;x++)
				for(y=0;y<ysiz;y++)
				{
					j = ylen[x][y];
					for(i=j-1;i>=0;i--) vptr[i].z = zsiz-1-vptr[i].z;
					for(i=(j>>1)-1,vptr2=&vptr[j-1];i>=0;i--)
						{ bvox = vptr[i]; vptr[i] = vptr2[-i]; vptr2[-i] = bvox; }
					vptr += j;
				}

			for(x=xsiz-1;x>=0;x--)
				for(y=ysiz-1;y>=0;y--)
					for(z=(zsiz>>1)-1;z>=0;z--)
					{
						i = (x*MAXYSIZ+y)*BUFZSIZ+z;
						j = (zsiz-1-z)-z+i;
						if (((vbit[i>>5]&(1<<i)) != 0) !=
							 ((vbit[j>>5]&(1<<j)) != 0))
						{
							vbit[i>>5] ^= (1<<i);
							vbit[j>>5] ^= (1<<j);
						}
					}

			zpiv = (float)zsiz-zpiv;
			break;
		case 1:
			vptr = voxdata;
			for(x=0;x<xsiz;x++)
			{
				j = xlen[x];
				for(i=(j>>1)-1,vptr2=&vptr[j-1];i>=0;i--)
					{ bvox = vptr[i]; vptr[i] = vptr2[-i]; vptr2[-i] = bvox; }
				for(y=(ysiz>>1)-1;y>=0;y--)
					{ i = ylen[x][y]; ylen[x][y] = ylen[x][ysiz-1-y]; ylen[x][ysiz-1-y] = i; }
				for(y=0;y<ysiz;y++)
				{
					j = ylen[x][y];
					for(i=(j>>1)-1,vptr2=&vptr[j-1];i>=0;i--)
						{ bvox = vptr[i]; vptr[i] = vptr2[-i]; vptr2[-i] = bvox; }
					vptr += j;
				}
			}

			for(x=xsiz-1;x>=0;x--)
				for(y=(ysiz>>1)-1;y>=0;y--)
					for(z=zsiz-1;z>=0;z--)
					{
						i = (x*MAXYSIZ+y)*BUFZSIZ+z;
						j = ((ysiz-1-y)-y)*BUFZSIZ+i;
						if (((vbit[i>>5]&(1<<i)) != 0) !=
							 ((vbit[j>>5]&(1<<j)) != 0))
						{
							vbit[i>>5] ^= (1<<i);
							vbit[j>>5] ^= (1<<j);
						}
					}

			ypiv = (float)ysiz-ypiv;
			break;
		case 2:

			vptr = voxdata;
			for(x=(xsiz>>1)-1;x>=0;x--)
			{
				j = xsiz-1-x;
				i = xlen[x]; xlen[x] = xlen[j]; xlen[j] = i;
				for(y=ysiz-1;y>=0;y--)
					{ i = ylen[x][y]; ylen[x][y] = ylen[j][y]; ylen[j][y] = i; }
			}
			for(i=(numvoxs>>1)-1,vptr2=&vptr[numvoxs-1];i>=0;i--)
				{ bvox = vptr[i]; vptr[i] = vptr2[-i]; vptr2[-i] = bvox; }
			for(x=0;x<xsiz;x++)
			{
				j = xlen[x];
				for(i=(j>>1)-1,vptr2=&vptr[j-1];i>=0;i--)
					{ bvox = vptr[i]; vptr[i] = vptr2[-i]; vptr2[-i] = bvox; }
				vptr += xlen[x];
			}

			for(x=(xsiz>>1)-1;x>=0;x--)
				for(y=ysiz-1;y>=0;y--)
					for(z=zsiz-1;z>=0;z--)
					{
						i = (x*MAXYSIZ+y)*BUFZSIZ+z;
						j = ((xsiz-1-x)-x)*MAXYSIZ*BUFZSIZ+i;
						if (((vbit[i>>5]&(1<<i)) != 0) !=
							 ((vbit[j>>5]&(1<<j)) != 0))
						{
							vbit[i>>5] ^= (1<<i);
							vbit[j>>5] ^= (1<<j);
						}
					}

			xpiv = (float)xsiz-xpiv;
			break;
	}
	calcallvis();
	calcalldir();
}

void axisswap3d (long axes)
{
	voxtype **optr, *ovox, *vptr, *vptr2, *vptr3;
	long i, j, x, y, z, *olen;
	float f;

	switch (axes)
	{
		case 0: //swap x&y
			if (optr = (voxtype **)malloc(xsiz*4+numvoxs*sizeof(voxtype)))
			{
				ovox = (voxtype *)(((long)optr)+xsiz*4);
				memcpy(ovox,voxdata,numvoxs*sizeof(voxtype));

				for(x=0,vptr=ovox;x<xsiz;vptr+=xlen[x++]) optr[x] = vptr;

				vptr = voxdata;
				for(y=0;y<ysiz;y++)
					for(x=0;x<xsiz;x++)
					{
						i = ylen[x][y];
						memcpy(vptr,optr[x],i*sizeof(voxtype));
						vptr += i; optr[x] += i;
					}

				for(x=max(xsiz,ysiz)-1;x>=0;x--)
					for(y=x-1;y>=0;y--)
						{ i = ylen[x][y]; ylen[x][y] = ylen[y][x]; ylen[y][x] = i; }

				i = xsiz; xsiz = ysiz; ysiz = i; updatewindowborders();
				for(x=0;x<xsiz;x++)
				{
					for(y=i=0;y<ysiz;y++) i += ylen[x][y];
					xlen[x] = i;
				}

				for(x=max(xsiz,ysiz)-1;x>=0;x--)
					for(y=x-1;y>=0;y--)
						for(z=zsiz-1;z>=0;z--)
						{
							i = (x*MAXYSIZ+y)*BUFZSIZ+z;
							j = (y*MAXYSIZ+x)*BUFZSIZ+z;
							if (((vbit[i>>5]&(1<<i)) != 0) !=
								 ((vbit[j>>5]&(1<<j)) != 0))
							{
								vbit[i>>5] ^= (1<<i);
								vbit[j>>5] ^= (1<<j);
							}
						}

				f = xpiv; xpiv = ypiv; ypiv = f;
				free(optr);
			}
			break;
		case 1: //swap x&z (emulated with cases 0&2)
			break;
		case 2: //swap y&z
			for(x=xsiz-1,j=0;x>=0;x--) if (xlen[x] > j) j = xlen[x];
			if (optr = (voxtype **)malloc(zsiz*8+j*sizeof(voxtype)))
			{
				olen = (long *)(((long)optr)+zsiz*4);
				ovox = (voxtype *)(((long)olen)+zsiz*4);
				vptr = voxdata;
				for(x=0;x<xsiz;x++)
				{
					memcpy(ovox,vptr,xlen[x]*sizeof(voxtype));

					for(z=0;z<zsiz;z++) olen[z] = 0;
					for(y=0,vptr2=vptr;y<ysiz;y++)
						for(vptr3=&vptr2[ylen[x][y]];vptr2<vptr3;vptr2++)
							olen[vptr2->z]++;

					for(z=0;z<zsiz;z++) { optr[z] = vptr; vptr += olen[z]; }

					for(y=0,vptr2=ovox;y<ysiz;y++)
						for(vptr3=&vptr2[ylen[x][y]];vptr2<vptr3;vptr2++)
						{
							z = vptr2->z;
							(*(optr[z])) = (*vptr2);
							optr[z]->z = y;
							optr[z]++;
						}

					for(z=0;z<zsiz;z++) ylen[x][z] = olen[z];
				}

				for(x=xsiz-1;x>=0;x--)
					for(y=max(ysiz,zsiz)-1;y>=0;y--)
						for(z=y-1;z>=0;z--)
						{
							i = (x*MAXYSIZ+y)*BUFZSIZ+z;
							j = (x*MAXYSIZ+z)*BUFZSIZ+y;
							if (((vbit[i>>5]&(1<<i)) != 0) !=
								 ((vbit[j>>5]&(1<<j)) != 0))
							{
								vbit[i>>5] ^= (1<<i);
								vbit[j>>5] ^= (1<<j);
							}
						}

				i = ysiz; ysiz = zsiz; zsiz = i; updatewindowborders();
				f = ypiv; ypiv = zpiv; zpiv = f;
				free(optr);
			}
			break;
	}
	calcallvis();
	calcalldir();
}

	//NOTE: only 1 param can be non-zero!
void boxmove3d (long ax, long ay, long az)
{
	voxtype *v;
	long i, j, k, x, y, z, ox, oy, oz, nx, ny, nz, ix, iy, iz, tx, ty, tz;
	char ch;

	long bnumvoxs;
	short *bxlen;
	char *bylen;
	voxtype *bvoxdata;

	ox = min(corn[0].x,corn[1].x); nx = max(corn[0].x,corn[1].x);
	oy = min(corn[0].y,corn[1].y); ny = max(corn[0].y,corn[1].y);
	oz = min(corn[0].z,corn[1].z); nz = max(corn[0].z,corn[1].z);
	if (ax > 0) { i = ox; ox = nx; nx = i; }
	if (ay > 0) { i = oy; oy = ny; ny = i; }
	if (az > 0) { i = oz; oz = nz; nz = i; }
	if (nx >= ox) { nx++; ix = 1; } else { nx--; ix = -1; }
	if (ny >= oy) { ny++; iy = 1; } else { ny--; iy = -1; }
	if (nz >= oz) { nz++; iz = 1; } else { nz--; iz = -1; }

		//Update bounds before it can move out of bounds...
	if ((unsigned long)(ox+ax) >= xsiz)
	{
		if (xsiz >= MAXXSIZ) { i = optimizexdimen(); ox += i; nx += i; corn[0].x += i; corn[1].x += i; }
		if (xsiz >= MAXXSIZ) return;
	}
	if ((unsigned long)(oy+ay) >= ysiz)
	{
		if (ysiz >= MAXYSIZ) { i = optimizeydimen(); oy += i; ny += i; corn[0].y += i; corn[1].y += i; }
		if (ysiz >= MAXYSIZ) return;
	}
	if ((unsigned long)(oz+az) >= zsiz)
	{
		if (zsiz >= LIMZSIZ) { i = optimizezdimen(); oz += i; nz += i; corn[0].z += i; corn[1].z += i; }
		if (zsiz >= LIMZSIZ) return;
	}
	if (ox+ax < 0)     { changedimen(0); ox++; nx++; corn[0].x++; corn[1].x++; }
	if (oy+ay < 0)     { changedimen(4); oy++; ny++; corn[0].y++; corn[1].y++; }
	if (oz+az < 0)     { changedimen(8); oz++; nz++; corn[0].z++; corn[1].z++; }
	if (ox+ax >= xsiz) { changedimen(1); }
	if (oy+ay >= ysiz) { changedimen(5); }
	if (oz+az >= zsiz) { changedimen(9); }

		//Remember old colors by backing up up all RLE data
	if (!(bxlen = (short *)malloc(xsiz*sizeof(xlen[0])+xsiz*ysiz*sizeof(ylen[0][0])+numvoxs*sizeof(voxtype)))) return;
	bylen = (char *)(((long)bxlen)+xsiz*sizeof(xlen[0]));
	bvoxdata = (voxtype *)(((long)bylen)+xsiz*ysiz*sizeof(ylen[0][0]));
	bnumvoxs = numvoxs;
	memcpy(bxlen,xlen,xsiz*sizeof(xlen[0]));
	for(i=0;i<xsiz;i++) memcpy(&bylen[i*ysiz],&ylen[i][0],ysiz*sizeof(ylen[0][0]));
	memcpy(bvoxdata,voxdata,numvoxs*sizeof(voxtype));

	for(tx=ox;tx!=nx;tx+=ix)
		for(ty=oy;ty!=ny;ty+=iy)
			for(tz=oz;tz!=nz;tz+=iz)
			{
					//Write position out of bounds
				if (((unsigned long)(tx+ax) >= xsiz) ||
					 ((unsigned long)(ty+ay) >= ysiz) ||
					 ((unsigned long)(tz+az) >= zsiz)) continue;

				if (((unsigned long)tx < xsiz) &&
					 ((unsigned long)ty < ysiz) &&
					 ((unsigned long)tz < zsiz))
				{
					j = (tx*MAXYSIZ+ty)*BUFZSIZ+tz;
					i = (vbit[j>>5]&(1<<j));
				} else i = 1; //Read out of bounds, so delete

				k = ((tx+ax)*MAXYSIZ+(ty+ay))*BUFZSIZ+(tz+az);
				if (i) vbit[k>>5] |= (1<<k);
				  else vbit[k>>5] &= ~(1<<k);
			}

	if (ax) //Clear last slice
	{
		tx -= ix;
		for(ty=oy;ty!=ny;ty+=iy)
			for(tz=oz;tz!=nz;tz+=iz)
			{
				if (((unsigned long)tx >= xsiz) ||
					 ((unsigned long)ty >= ysiz) ||
					 ((unsigned long)tz >= zsiz)) continue;
				j = (tx*MAXYSIZ+ty)*BUFZSIZ+tz;
				vbit[j>>5] |= (1<<j);
			}
	}
	else if (ay)
	{
		ty -= iy;
		for(tx=ox;tx!=nx;tx+=ix)
			for(tz=oz;tz!=nz;tz+=iz)
			{
				if (((unsigned long)tx >= xsiz) ||
					 ((unsigned long)ty >= ysiz) ||
					 ((unsigned long)tz >= zsiz)) continue;
				j = (tx*MAXYSIZ+ty)*BUFZSIZ+tz;
				vbit[j>>5] |= (1<<j);
			}
	}
	else if (az)
	{
		tz -= iz;
		for(tx=ox;tx!=nx;tx+=ix)
			for(ty=oy;ty!=ny;ty+=iy)
			{
				if (((unsigned long)tx >= xsiz) ||
					 ((unsigned long)ty >= ysiz) ||
					 ((unsigned long)tz >= zsiz)) continue;
				j = (tx*MAXYSIZ+ty)*BUFZSIZ+tz;
				vbit[j>>5] |= (1<<j);
			}
	}

	for(i=1;i>=0;i--) { corn[i].x += ax; corn[i].y += ay; corn[i].z += az; }
	ox = min(corn[0].x,corn[1].x); nx = max(corn[0].x,corn[1].x)-ox;
	oy = min(corn[0].y,corn[1].y); ny = max(corn[0].y,corn[1].y)-oy;
	oz = min(corn[0].z,corn[1].z); nz = max(corn[0].z,corn[1].z)-oz;

	numvoxs = 0;
	for(x=0;x<xsiz;x++)
	{
		xlen[x] = 0;
		for(y=0;y<ysiz;y++)
		{
			ylen[x][y] = 0;
			j = (x*MAXYSIZ+y)*BUFZSIZ;
			for(z=0;z<zsiz;z++)
			{
				if (vbit[(j+z)>>5]&(1<<(j+z))) continue;
				k = getvis(x,y,z); if (!k) continue;

				voxdata[numvoxs].z = z;

				tx = x; ty = y; tz = z;
				if (((unsigned long)(x-ox) <= nx) &&
					 ((unsigned long)(y-oy) <= ny) &&
					 ((unsigned long)(z-oz) <= nz))
					{ tx -= ax; ty -= ay; tz -= az; }

					//Grabs color of (tx,ty,tz) from backup RLE data (SLOW!)
				v = bvoxdata;
				if ((tx<<1) < xsiz)  { for(i=0     ;i< tx;i++) v += bxlen[i]; }
				else { v += bnumvoxs;  for(i=xsiz-1;i>=tx;i--) v -= bxlen[i]; }
				if ((ty<<1) < ysiz)  { for(i=0     ;i< ty;i++) v += bylen[tx*ysiz+i]; }
				else { v += bxlen[tx]; for(i=ysiz-1;i>=ty;i--) v -= bylen[tx*ysiz+i]; }
				for(i=bylen[tx*ysiz+ty];i;i--,v++) if (v->z == tz) { ch = v->col; break; }
				if (!i) ch = curcol;

				voxdata[numvoxs].col = ch;
				voxdata[numvoxs].vis = (char)k;
				//voxdata[numvoxs].dir = getvdir(x,y,z);

				ylen[x][y]++; numvoxs++;
			}
			xlen[x] += ylen[x][y];
		}
	}
	free(bxlen);
	calcalldir();
}

	//op: 0=del, 1=ins
void insdelbox3d (long ox, long oy, long oz, long nx, long ny, long nz, long op)
{
	voxtype *v;
	long i, j, k, x, y, z;
	char ch;

	long bnumvoxs;
	short *bxlen;
	char *bylen;
	voxtype *bvoxdata;

		//Remember old colors by backing up up all RLE data
	if (!(bxlen = (short *)malloc(xsiz*sizeof(xlen[0])+xsiz*ysiz*sizeof(ylen[0][0])+numvoxs*sizeof(voxtype)))) return;
	bylen = (char *)(((long)bxlen)+xsiz*sizeof(xlen[0]));
	bvoxdata = (voxtype *)(((long)bylen)+xsiz*ysiz*sizeof(ylen[0][0]));
	bnumvoxs = numvoxs;
	memcpy(bxlen,xlen,xsiz*sizeof(xlen[0]));
	for(i=0;i<xsiz;i++) memcpy(&bylen[i*ysiz],&ylen[i][0],ysiz*sizeof(ylen[0][0]));
	memcpy(bvoxdata,voxdata,numvoxs*sizeof(voxtype));

		//Delete box
	if (ox < 0) ox = 0; if (nx > xsiz) nx = xsiz;
	if (oy < 0) oy = 0; if (ny > ysiz) ny = ysiz;
	if (oz < 0) oz = 0; if (nz > zsiz) nz = zsiz;
	if (!op)
	{
		for(x=ox;x<nx;x++)
			for(y=oy;y<ny;y++)
			{
				j = (x*MAXYSIZ+y)*BUFZSIZ;
				for(z=oz;z<nz;z++) vbit[(j+z)>>5] |= (1<<(j+z));
			}
	}
	else
	{
		for(x=ox;x<nx;x++)
			for(y=oy;y<ny;y++)
			{
				j = (x*MAXYSIZ+y)*BUFZSIZ;
				for(z=oz;z<nz;z++) vbit[(j+z)>>5] &= ~(1<<(j+z));
			}
	}

	numvoxs = 0;
	for(x=0;x<xsiz;x++)
	{
		xlen[x] = 0;
		for(y=0;y<ysiz;y++)
		{
			ylen[x][y] = 0;
			j = (x*MAXYSIZ+y)*BUFZSIZ;
			for(z=0;z<zsiz;z++)
			{
				if (vbit[(j+z)>>5]&(1<<(j+z))) continue;
				k = getvis(x,y,z); if (!k) continue;

				voxdata[numvoxs].z = z;

					//Grabs color of (x,y,z) from backup RLE data (SLOW!)
				v = bvoxdata;
				if ((x<<1) < xsiz)  { for(i=0     ;i< x;i++) v += bxlen[i]; }
				else { v += bnumvoxs; for(i=xsiz-1;i>=x;i--) v -= bxlen[i]; }
				if ((y<<1) < ysiz)  { for(i=0     ;i< y;i++) v += bylen[x*ysiz+i]; }
				else { v += bxlen[x]; for(i=ysiz-1;i>=y;i--) v -= bylen[x*ysiz+i]; }
				for(i=bylen[x*ysiz+y];i;i--,v++) if (v->z == z) { ch = v->col; break; }
				if (!i) ch = curcol;

				voxdata[numvoxs].col = ch;
				voxdata[numvoxs].vis = (char)k;
				//voxdata[numvoxs].dir = getvdir(x,y,z);

				ylen[x][y]++; numvoxs++;
			}
			xlen[x] += ylen[x][y];
		}
	}
	free(bxlen);
	calcalldir();
}

static point3d unitaxis[6] = {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}};
static char unitaxlu[24][3] =
{
	0,2,4, 0,3,5, 1,2,5, 1,3,4, 0,5,2, 0,4,3, 1,4,2, 1,5,3,
	2,0,5, 3,0,4, 2,1,4, 3,1,5, 2,4,0, 3,5,0, 2,5,1, 3,4,1,
	4,0,2, 5,0,3, 5,1,2, 4,1,3, 5,2,0, 4,3,0, 4,2,1, 5,3,1,
// Here are the 24 mirrored rotations: (remember to change to unitaxlu[48][3])
// 0,2,5, 0,3,4, 1,2,4, 1,3,5, 0,4,2, 0,5,3, 1,5,2, 1,4,3,
// 2,0,4, 3,0,5, 2,1,5, 3,1,4, 2,5,0, 3,4,0, 2,4,1, 3,5,1,
// 5,0,2, 4,0,3, 4,1,2, 5,1,3, 4,2,0, 5,3,0, 5,2,1, 4,3,1,
};

	//This uses code from slerp(...) - given 2 orthonormal bases, it returns
	//   the cosine of the angle along the shortest (optimal) axis of rotation
	//  (range: -1.0 to 1.0)
static float orthodist (point3d *a0, point3d *a1, point3d *a2, point3d *b0, point3d *b1, point3d *b2)
{
	point3d ax;
	float t, ox, oy, c, s;

	ax.x = a0->y*b0->z - a0->z*b0->y + a1->y*b1->z - a1->z*b1->y + a2->y*b2->z - a2->z*b2->y;
	ax.y = a0->z*b0->x - a0->x*b0->z + a1->z*b1->x - a1->x*b1->z + a2->z*b2->x - a2->x*b2->z;
	ax.z = a0->x*b0->y - a0->y*b0->x + a1->x*b1->y - a1->y*b1->x + a2->x*b2->y - a2->y*b2->x;
	t = ax.x*ax.x + ax.y*ax.y + ax.z*ax.z;

	ox = a0->x*ax.x + a0->y*ax.y + a0->z*ax.z;
	oy = a1->x*ax.x + a1->y*ax.y + a1->z*ax.z;
	if (fabs(ox) < fabs(oy))
		{ c = a0->x*b0->x + a0->y*b0->y + a0->z*b0->z; s = ox*ox; }
	else
		{ c = a1->x*b1->x + a1->y*b1->y + a1->z*b1->z; s = oy*oy; }
	if (t == s) //Exactly aligned on axes: must use dot product test instead
	{
		if (a0->x*b0->x + a0->y*b0->y + a0->z*b0->z < .5) return(-1.0);
		if (a1->x*b1->x + a1->y*b1->y + a1->z*b1->z < .5) return(-1.0);
		if (a2->x*b2->x + a2->y*b2->y + a2->z*b2->z < .5) return(-1.0);
		return(1.0);
	}
	return((c*t - s) / (t-s));
}

static void orthorotate (float ox, float oy, float oz, point3d *iri, point3d *ido, point3d *ifo)
{
	float f, t, dx, dy, dz, rr[9];

	fcossin(ox,&ox,&dx);
	fcossin(oy,&oy,&dy);
	fcossin(oz,&oz,&dz);
	f = ox*oz; t = dx*dz; rr[0] =  t*dy + f; rr[7] = -f*dy - t;
	f = ox*dz; t = dx*oz; rr[1] = -f*dy + t; rr[6] =  t*dy - f;
	rr[2] = dz*oy; rr[3] = -dx*oy; rr[4] = ox*oy; rr[8] = oz*oy; rr[5] = dy;
	ox = iri->x; oy = ido->x; oz = ifo->x;
	iri->x = ox*rr[0] + oy*rr[3] + oz*rr[6];
	ido->x = ox*rr[1] + oy*rr[4] + oz*rr[7];
	ifo->x = ox*rr[2] + oy*rr[5] + oz*rr[8];
	ox = iri->y; oy = ido->y; oz = ifo->y;
	iri->y = ox*rr[0] + oy*rr[3] + oz*rr[6];
	ido->y = ox*rr[1] + oy*rr[4] + oz*rr[7];
	ifo->y = ox*rr[2] + oy*rr[5] + oz*rr[8];
	ox = iri->z; oy = ido->z; oz = ifo->z;
	iri->z = ox*rr[0] + oy*rr[3] + oz*rr[6];
	ido->z = ox*rr[1] + oy*rr[4] + oz*rr[7];
	ifo->z = ox*rr[2] + oy*rr[5] + oz*rr[8];
	//orthofit3x3(iri,ido,ifo); //Warning: assumes ido and ifo follow iri in memory!
}

#if 0
!endif
#endif
