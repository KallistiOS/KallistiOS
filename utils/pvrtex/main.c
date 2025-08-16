#define PVRTEX_VERSION	"2.00"

#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <libgen.h>

#include "stb_image_write.h"
#include "pvr_texture_encoder.h"
#include "optparse.h"
#include "mycommon.h"
#include "file_pvr.h"
#include "file_tex.h"

extern int LoadPalette(const char *fname, PvrTexEncoder *pte);

#include "info/examples.h"
#include "info/options.h"

char const * program_name;

int log_level = LOG_PROGRESS;
void pteLogLocV(unsigned level, const char *file, unsigned line, const char *fmt, va_list args) {
	static const char * logtypes[] = {
		[LOG_ALL] = "ALL",
		[LOG_DEBUG] = "DEBUG",
		[LOG_INFO] = "INFO",
		[LOG_PROGRESS] = "PROGRESS",
		[LOG_WARNING] = "WARNING",
		[LOG_COMPLETION] = "COMPLETION",
		[LOG_NONE] = "NONE"
	};
	
	if (level > log_level)
		return;
	
	if (log_level == LOG_DEBUG) {
		if (level >= LOG_DEBUG)
			level = LOG_DEBUG;
		if (file == NULL)
			file = "unk";
		fprintf(stderr, "[%s, ln %i] %s: ", file, line, logtypes[level]);
	}
	vfprintf(stderr, fmt, args);
}

void pteLogLoc(unsigned level, const char *file, unsigned line, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	pteLogLocV(level, file, line, fmt, args);
	va_end(args);
}

void ErrorExitV(const char *fmt, va_list args) {
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, args);

	fprintf(stderr, "\nUsage:\t%s -i inputimage -o output.pvr -f format [options]\n", program_name);
	fprintf(stderr, "\n\t%s --examples for usage examples\n"
	"\n\t%s --help for command line options\n\n", program_name, program_name);
	exit(1);
}
	
void ErrorExit(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	ErrorExitV(fmt, args);
	va_end(args);
	
	exit(1);
}

void ErrorExitOn(int cond, const char *fmt, ...) {
	if (!cond)
		return;
	
	va_list args;
	va_start(args, fmt);
	ErrorExitV(fmt, args);
	va_end(args);
}

typedef enum {
	EXT_UNKNOWN,
	EXT_PVR,
	EXT_DT,
	EXT_TEX,
	EXT_PAL,
} FileExtension;

//https://cfengine.com/blog/2021/optional-arguments-with-getopt-long/
#define OPTARG_FIX_UP do { \
	if (options.optarg == NULL && options.optind < argc && options.argv[options.optind][0] != '-') \
		options.optarg = options.argv[options.optind++]; \
	} while(0)

typedef struct {
	const char *name;
	int value;
} OptionMap;

static const OptionMap file_extension[] = {
	{".pvr", EXT_PVR},
	{".dt", EXT_DT},
	{".tex", EXT_TEX},
	{".vq", EXT_TEX},
	{".pal", EXT_PAL},
};
static const OptionMap supported_pixel_formats[] = {
	{"RGB565", PT_RGB565},
	{"ARGB1555", PT_ARGB1555},
	{"ARGB4444", PT_ARGB4444},
	{"YUV", PT_YUV},
	{"YUV422", PT_YUV},
	{"PAL8BPP", PT_PALETTE_8B},
	{"PAL4BPP", PT_PALETTE_4B},
	{"BUMPMAP", PTE_BUMP},
	{"NORMAL", PT_NORMAL},
	{"AUTO", PTE_AUTO},
	{"AUTOYUV", PTE_AUTO_YUV},
};
static const OptionMap resize_options[] = {
	{"none", PTE_FIX_NONE},
	{"near", PTE_FIX_NEAREST},
	{"nearest", PTE_FIX_NEAREST},
	{"up", PTE_FIX_UP},
	{"down", PTE_FIX_DOWN},
};
static const OptionMap mip_resize_options[] = {
	{"none", PTE_FIX_MIP_NONE},
	{"x2", PTE_FIX_MIP_NARROW_X2},
	{"x4", PTE_FIX_MIP_NARROW_X4},
	{"up", PTE_FIX_MIP_MAX},
	{"down", PTE_FIX_MIP_MIN},
	{"opt", PTE_FIX_MIP_OPTIONAL},
	{"optional", PTE_FIX_MIP_OPTIONAL},
};
static const OptionMap edge_options[] = {
	{"clamp", STBIR_EDGE_CLAMP},
	{"reflect", STBIR_EDGE_REFLECT},
	{"wrap", STBIR_EDGE_WRAP},
	{"zero", STBIR_EDGE_ZERO},
};
static const OptionMap normal_style_options[] = {
	{"texconv", PTE_NORM_TEXCONV},
	{"regular", PTE_NORM_REGULAR},
};

//Search through OptionMap for match and return it's value.
//If name is not found and invalid_msg is NULL, it returns default_value 
//If name is not found and invalid_msg is not NULL, it prints invalid_msg and exits
int GetOptMap(const OptionMap *map, size_t mapsize, const char *name, int default_value, const char *invalid_msg) {
	if (name == NULL) {
		if (invalid_msg)
			ErrorExit("%s", invalid_msg);
		return default_value;
	}
	
	for(size_t i = 0; i < mapsize; i++) {
		if (!strcasecmp(map[i].name, name)) {
			return map[i].value;
		}
	}
	if (invalid_msg)
		ErrorExit("%s", invalid_msg);
	return default_value;
}

int main(int argc, char **argv) {
	program_name = (char*)basename(argv[0]);

	PvrTexEncoder pte;
	pteInit(&pte);
	
	struct optparse_long longopts[] = {
		{"help", 'h', OPTPARSE_NONE},
		{"examples", 'E', OPTPARSE_NONE},
		{"out", 'o', OPTPARSE_REQUIRED},
		{"in", 'i', OPTPARSE_REQUIRED},
		{"format", 'f', OPTPARSE_REQUIRED},
		{"gamma", 'g', OPTPARSE_REQUIRED},
		{"gamma-alpha", 'G', OPTPARSE_REQUIRED},
		{"compress", 'c', OPTPARSE_OPTIONAL},
		{"max-color", 'C', OPTPARSE_REQUIRED},
		{"mipmap", 'm', OPTPARSE_OPTIONAL},
		{"perfect-mip", 'M', OPTPARSE_OPTIONAL},
		{"high-weight", 'H', OPTPARSE_REQUIRED},
		{"preview", 'p', OPTPARSE_REQUIRED},
		{"palette", 'P', OPTPARSE_REQUIRED},
		{"bilinear", 'b', OPTPARSE_NONE},
		{"dither", 'd', OPTPARSE_OPTIONAL},
		{"nearest", 'n', OPTPARSE_NONE},
		{"verbose", 'v', OPTPARSE_NONE},
		{"version", 'V', OPTPARSE_NONE},
		{"no-mip-shift", 'S', OPTPARSE_NONE},
		{"resize", 'r', OPTPARSE_OPTIONAL},
		{"mip-resize", 'R', OPTPARSE_OPTIONAL},
		{"stride", 's', OPTPARSE_NONE},
		{"edge", 'e', OPTPARSE_REQUIRED},
		{"normal-style", 1, OPTPARSE_REQUIRED},
		{"flip-v", 2, OPTPARSE_NONE},
		{"flip-y", 2, OPTPARSE_NONE},
		{0}
	};
	
	#define MAX_FNAMES	11
	const char *fnames[MAX_FNAMES];
	unsigned fname_cnt = 0;
	const char *outname = "";
	const char *prevname = "";
	const char *palfile = NULL;
	
	//Parse command line parameters
	struct optparse options;
	int option;
	optparse_init(&options, argv);
	while ((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch(option) {
		case 'h':
			printf("%.*s", options_txt_size, options_txt_data);
			return 0;
			break;
		case 'E':
			printf("%.*s", examples_txt_size, examples_txt_data);
			return 0;
			break;
		case 'i':
			ErrorExitOn(fname_cnt >= MAX_FNAMES, "Too many input files have been specified\n");
			fnames[fname_cnt++] = options.optarg;
			break;
		case 'o':
			outname = options.optarg;
			break;
		case 'f':
			pte.pixel_format = GetOptMap(supported_pixel_formats, ARR_SIZE(supported_pixel_formats), options.optarg, -1, "invalid pixel format\n");
			break;
		case 'g':
			if (sscanf(options.optarg, "%f", &pte.rgb_gamma) != 1)
				ErrorExit("invalid gamma\n");
			break;
		case 'G':
			if (sscanf(options.optarg, "%f", &pte.alpha_gamma) != 1)
				ErrorExit("invalid alpha gamma\n");
			break;
		case 'r':
			OPTARG_FIX_UP;
			pte.resize = PTE_FIX_NEAREST;
			if (options.optarg) {
				pte.resize = GetOptMap(resize_options, ARR_SIZE(resize_options), options.optarg, PTE_FIX_UP, "invalid resize value\n");
			}
			break;
		case 'R':
			OPTARG_FIX_UP;
			pte.mipresize = PTE_FIX_MIP_NARROW_X2;
			if (options.optarg) {
				pte.mipresize = GetOptMap(mip_resize_options, ARR_SIZE(mip_resize_options), options.optarg, PTE_FIX_MIP_NARROW_X2, "invalid mip resize value\n");
			}
			break;
		case 'p':
			prevname = options.optarg;
			break;
		case 'S':
			pte.mip_shift_correction = false;
			break;
		case 's':
			pte.stride = true;
			break;
		case 'e':
			pte.edge_method = GetOptMap(edge_options, ARR_SIZE(edge_options), options.optarg, -0, "invalid edge handling method\n");
			break;
		case 'H':
			if (sscanf(options.optarg, "%u", &pte.high_weight_mips) != 1) {
				ErrorExit("invalid high weight parameter, must be an integer between 1 and the number of mipmap levels\n");
			}
			break;
		case 'n':
			ErrorExit("Option -%c not supported yet\n", option);
			break;
		case 'v':
			log_level = LOG_INFO;
			
			//If someone runs this with only -v as a parameter, they probably want the version
			if (argc != 2)
				break;
			//Fallthrough
		case 'V':
			printf("pvrtex - Dreamcast Texture Encoder - Version "PVRTEX_VERSION"\n");
			return 0;
		case 'b':
			pteLog(LOG_WARNING, "Option --bilinear does nothing\n");
			break;
		case 'd': {
			OPTARG_FIX_UP;
			pte.dither = 1.0f;
			if (options.optarg) {
				if ((sscanf(options.optarg, "%f", &pte.dither) != 1) || (pte.dither < 0) || (pte.dither > 1))  {
					ErrorExit("invalid dither amount parameter, should be in the range [0, 1]\n");
				}
			}
			} break;
		case 'c': {
			OPTARG_FIX_UP;
			unsigned cbsize = 256;
			if (options.optarg) {
				if (!strcasecmp(options.optarg, "small") || !strcasecmp(options.optarg, "sm")) {
					pte.auto_small_vq = true;
				} else if ((sscanf(options.optarg, "%u", &cbsize) != 1) || (cbsize <= 0) || (cbsize > 256))  {
					ErrorExit("invalid compression parameter (%s)\n", options.optarg);
				}
			}
			pteSetCompressed(&pte, cbsize);
			} break;
		case 'm':
			OPTARG_FIX_UP;
			
			pte.want_mips = PTE_MIP_QUALITY;
			if (options.optarg) {
				if (!strcasecmp(options.optarg, "fast"))
					pte.want_mips = PTE_MIP_FAST;
				else if (!strcasecmp(options.optarg, "quality"))
					;	//default
				else
					ErrorExit("Unknown mipmap parameter (%s)\n", options.optarg);
			}
			break;
		case 'M':
			OPTARG_FIX_UP;
			pte.perfect_mips = 3;
			if (options.optarg) {
				if (sscanf(options.optarg, "%u", &pte.perfect_mips) != 1)  {
					ErrorExit("bad perfect mip value\n");
				}
			}
			break;
		case 'C':
			if ((sscanf(options.optarg, "%u", &pte.palette_size) != 1) || (pte.palette_size <= 1) || (pte.palette_size > 256))  {
					ErrorExit("invalid max palette size parameter (should be [1, 16] for 4bpp, or [1, 256] for 8bpp)\n");
				}
			break;
		case 'P':
			palfile = options.optarg;
			break;
		case 1:
			pte.normal_style = GetOptMap(normal_style_options, ARR_SIZE(normal_style_options), options.optarg, -0, "invalid normal style method\n");
			break;
		case 2:
			pte.flip_v = true;
			break;
		default:
			ErrorExit("%s\n", options.errmsg);
		}
	}
	
	bool have_output = strlen(outname) > 0;
	bool have_preview = strlen(prevname) > 0;
	bool already_have_pal_file = false;
	bool only_want_pal = false;
	FileExtension output_file_type = EXT_UNKNOWN;
	
	//Get output extension
	if (have_output) {
		const char *extension = "";
		extension = strrchr(outname, '.');
		if (extension != NULL) {
			output_file_type = GetOptMap(file_extension, ARR_SIZE(file_extension), extension, EXT_UNKNOWN, NULL);
		}
	}
	
	if (output_file_type == EXT_PAL) {
		output_file_type = EXT_DT;
		only_want_pal = true;
	}
	
	ErrorExitOn(!have_output && !have_preview, "No output or preview file name specified, nothing to do\n");
	ErrorExitOn(fname_cnt == 0, "No input files specified\n");

	pteLog(LOG_PROGRESS, "Reading input...\n");
	pteLoadFromFiles(&pte, fnames, fname_cnt);
	
	//Check and fix up image size
	pteSetSize(&pte);
	
	if (pte.pixel_format == PTE_AUTO || pte.pixel_format == PTE_AUTO_YUV)
		pteAutoSelectPixelFormat(&pte);
	
	//Fix some stuff up for .PVR files
	if (output_file_type == EXT_PVR) {
		if (pteIsCompressed(&pte)) {
			//.PVR seems to require square textures if compressed
			pteMakeSquare(&pte);
			
			if (pte.auto_small_vq == true) {
				//For other sizes, we make a full size codebook texture, but don't use all the entries
				pte.codebook_size = fPvrSmallVQCodebookSize(pte.w, pte.want_mips);
				if (pte.w != pte.h) {
					pteLog(LOG_WARNING, ".PVR file does not support small VQ with non-square textures, using full size codebook\n");
					pte.auto_small_vq = false;
				} else if (pte.codebook_size < 256) {
					pteLog(LOG_INFO, "Making small codebook .PVR VQ is CB size of %u\n", pte.codebook_size);
					
				} else {
					pteLog(LOG_WARNING, ".PVR file does not support small VQ with current size/mipmap combination, using full size codebook\n");
					pte.auto_small_vq = false;
				}
			}
		}
	}
	
	if (output_file_type == EXT_DT) {
		if (pte.auto_small_vq) {
			//8x8 no mips has 10 entries, 128x128 with mips has 192 entires
			//Pick something in between
			float small_uncomp = CalcTextureSize(8, 8, PT_ARGB1555, 0, 0, 0);
			float large_uncomp = CalcTextureSize(128, 128, PT_ARGB1555, 1, 0, 0);
			unsigned small_cbsize = 10;
			unsigned large_cbsize = 192;
			
			unsigned idxsize = CalcTextureSize(pte.w, pte.h, PT_ARGB1555, pteHasMips(&pte), 1, 0);
			float uncompsize = CalcTextureSize(pte.w, pte.h, PT_ARGB1555, pteHasMips(&pte), 0, 0);
			
			float ratio = (uncompsize - small_uncomp) / (large_uncomp - small_uncomp);
			unsigned cbsize = lerp(ratio, small_cbsize, large_cbsize);
			
			//If size is less than 32, add extra entries to use any padding that would result
			unsigned rndamt = 32;
			unsigned size = idxsize + cbsize*8;
			unsigned roundupsize = (size + rndamt - 1) & ~(rndamt-1);
			unsigned extraroom = roundupsize - size;
			pteLog(LOG_DEBUG, "Idx %u, CBsize %u, Extra %u\n", idxsize, cbsize, extraroom);
			
			pte.codebook_size = CLAMP(8, cbsize + extraroom/8, 256);
		}
		
		//.DT supports codebook offsets for true reduced codebooks
		pte.pvr_idx_offset = PVR_FULL_CODEBOOK - pte.codebook_size;
	}
	
	//If no edge method is specified, use clamp if no using mipmaps, or wrap if we are
	if (pte.edge_method == 0) {
		if (pte.want_mips)
			pte.edge_method = STBIR_EDGE_WRAP;
		else
			pte.edge_method = STBIR_EDGE_CLAMP;
	}
	
	//Load external palette if one exists
	if (palfile) {
		const char *pal_ext = strrchr(palfile, '.');
		if (pal_ext)
			already_have_pal_file = strcasecmp(pal_ext, ".pal") == 0;
		LoadPalette(palfile, &pte);
	}
	
	pteEncodeTexture(&pte);
	
	//Make preview
	if (have_preview) {
		const char *prevextension = strrchr(prevname, '.');
		if (prevextension != NULL) {
			pteLog(LOG_PROGRESS, "Writing preview to \"%s\"...\n", prevname);
			pteGeneratePreviews(&pte);
					
			//Write preview image to file
			if (strcasecmp(prevextension, ".png") == 0)
				stbi_write_png(prevname, pte.final_preview_w, pte.h, 4, pte.final_preview, 0);
			else if (strcasecmp(prevextension, ".jpg") == 0 || strcasecmp(prevextension, ".jpeg") == 0)
				stbi_write_jpg(prevname, pte.final_preview_w, pte.h, 4, pte.final_preview, 95);
			else if (strcasecmp(prevextension, ".bmp") == 0)
				stbi_write_bmp(prevname, pte.final_preview_w, pte.h, 4, pte.final_preview);
			else if (strcasecmp(prevextension, ".tga") == 0)
				stbi_write_tga(prevname, pte.final_preview_w, pte.h, 4, pte.final_preview);
			else
				pteLog(LOG_WARNING, "Skipping preview creation because of unknown file type (%s). Supported types are PNG, JPG, BMP, and TGA.\n", prevextension);
		} else {
			pteLog(LOG_WARNING, "No extension specified for preview, don't know what type to make. Supported types are PNG, JPG, BMP, and TGA.\n");
		}
	}
	
	//Write resulting texture
	if (have_output) {
		if (only_want_pal) {
			fTexWritePalette(&pte, outname);
		} else if (output_file_type == EXT_PVR) {
			pteLog(LOG_COMPLETION, "Writing .PVR to \"%s\"...\n", outname);
			fPvrWrite(&pte, outname);
		} else if (output_file_type == EXT_TEX) {
			pteLog(LOG_COMPLETION, "Writing texconv .TEX to \"%s\"...\n", outname);
			fTexWrite(&pte, outname);
			
			if (!already_have_pal_file && pteIsPalettized(&pte))
				fTexWritePaletteAppendPal(&pte, outname);
		} else if (output_file_type == EXT_DT) {
			pteLog(LOG_COMPLETION, "Writing .DT to \"%s\"...\n", outname);
			void fDtWrite(const PvrTexEncoder *pte, const char *outfname);
			fDtWrite(&pte, outname);
			
			if (!already_have_pal_file && pteIsPalettized(&pte))
				fTexWritePaletteAppendPal(&pte, outname);
		} else {
			ErrorExit("Unsupported output file type for \"%s\"\n", outname);
		}
	} else {
		pteLog(LOG_COMPLETION, "No output file specified\n");
	}
	
	pteFree(&pte);
	
	return 0;
}
