//From https://bisqwit.iki.fi/story/howto/dither/jy/

//~ #include <gd.h>
#include <stdio.h>
#include <math.h>
#include <algorithm> /* For std::sort() */
#include <vector>
#include <map>       /* For associative container, std::map<> */

#define COMPARE_RGB 1

static const double Gamma = 2.2; // Gamma correction we use.

double GammaCorrect(double v)   {
	return pow(v, Gamma);
}
double GammaUncorrect(double v) {
	return pow(v, 1.0 / Gamma);
}

/* CIE C illuminant */
static const double illum[3*3] = {
	0.488718, 0.176204, 0.000000,
	0.310680, 0.812985, 0.0102048,
	0.200602, 0.0108109, 0.989795
};
struct LabItem { // CIE L*a*b* color value with C and h added.
	double L,a,b,C,h;

	LabItem() { }
	LabItem(double R,double G,double B) {
		Set(R,G,B);
	}
	void Set(double R,double G,double B) {
		const double* const i = illum;
		double X = i[0]*R + i[3]*G + i[6]*B, x = X / (i[0] + i[1] + i[2]);
		double Y = i[1]*R + i[4]*G + i[7]*B, y = Y / (i[3] + i[4] + i[5]);
		double Z = i[2]*R + i[5]*G + i[8]*B, z = Z / (i[6] + i[7] + i[8]);
		const double threshold1 = (6*6*6.0)/(29*29*29.0);
		const double threshold2 = (29*29.0)/(6*6*3.0);
		double x1 = (x > threshold1) ? pow(x, 1.0/3.0) : (threshold2*x)+(4/29.0);
		double y1 = (y > threshold1) ? pow(y, 1.0/3.0) : (threshold2*y)+(4/29.0);
		double z1 = (z > threshold1) ? pow(z, 1.0/3.0) : (threshold2*z)+(4/29.0);
		L = (29*4)*y1 - (4*4);
		a = (500*(x1-y1) );
		b = (200*(y1-z1) );
		C = sqrt(a*a + b+b);
		h = atan2(b, a);
	}
	LabItem(unsigned rgb) {
		Set(rgb);
	}
	void Set(unsigned rgb) {
		Set( (rgb>>16)/255.0, ((rgb>>8)&0xFF)/255.0, (rgb&0xFF)/255.0 );
	}
};

/* From the paper "The CIEDE2000 Color-Difference Formula: Implementation Notes, */
/* Supplementary Test Data, and Mathematical Observations", by */
/* Gaurav Sharma, Wencheng Wu and Edul N. Dalal, */
/* Color Res. Appl., vol. 30, no. 1, pp. 21-30, Feb. 2005. */
/* Return the CIEDE2000 Delta E color difference measure squared, for two Lab values */
double ColorCompare(const LabItem& lab1, const LabItem& lab2) {
#define RAD2DEG(xx) (180.0/M_PI * (xx))
#define DEG2RAD(xx) (M_PI/180.0 * (xx))
	/* Compute Cromanance and Hue angles */
	double C1,C2, h1,h2;
	{
		double Cab = 0.5 * (lab1.C + lab2.C);
		double Cab7 = pow(Cab,7.0);
		double G = 0.5 * (1.0 - sqrt(Cab7/(Cab7 + 6103515625.0)));
		double a1 = (1.0 + G) * lab1.a;
		double a2 = (1.0 + G) * lab2.a;
		C1 = sqrt(a1 * a1 + lab1.b * lab1.b);
		C2 = sqrt(a2 * a2 + lab2.b * lab2.b);

		if (C1 < 1e-9)
			h1 = 0.0;
		else {
			h1 = RAD2DEG(atan2(lab1.b, a1));
			if (h1 < 0.0)
				h1 += 360.0;
		}

		if (C2 < 1e-9)
			h2 = 0.0;
		else {
			h2 = RAD2DEG(atan2(lab2.b, a2));
			if (h2 < 0.0)
				h2 += 360.0;
		}
	}

	/* Compute delta L, C and H */
	double dL = lab2.L - lab1.L, dC = C2 - C1, dH;
	{
		double dh;
		if (C1 < 1e-9 || C2 < 1e-9) {
			dh = 0.0;
		} else {
			dh = h2 - h1;
			/**/ if (dh > 180.0)  dh -= 360.0;
			else if (dh < -180.0) dh += 360.0;
		}

		dH = 2.0 * sqrt(C1 * C2) * sin(DEG2RAD(0.5 * dh));
	}

	double h;
	double L = 0.5 * (lab1.L  + lab2.L);
	double C = 0.5 * (C1 + C2);
	if (C1 < 1e-9 || C2 < 1e-9) {
		h = h1 + h2;
	} else {
		h = h1 + h2;
		if (fabs(h1 - h2) > 180.0) {
			/**/ if (h < 360.0)  h += 360.0;
			else if (h >= 360.0) h -= 360.0;
		}
		h *= 0.5;
	}
	double T = 1.0
		   - 0.17 * cos(DEG2RAD(h - 30.0))
		   + 0.24 * cos(DEG2RAD(2.0 * h))
		   + 0.32 * cos(DEG2RAD(3.0 * h + 6.0))
		   - 0.2 * cos(DEG2RAD(4.0 * h - 63.0));
	double hh = (h - 275.0)/25.0;
	double ddeg = 30.0 * exp(-hh * hh);
	double C7 = pow(C,7.0);
	double RC = 2.0 * sqrt(C7/(C7 + 6103515625.0));
	double L50sq = (L - 50.0) * (L - 50.0);
	double SL = 1.0 + (0.015 * L50sq) / sqrt(20.0 + L50sq);
	double SC = 1.0 + 0.045 * C;
	double SH = 1.0 + 0.015 * C * T;
	double RT = -sin(DEG2RAD(2 * ddeg)) * RC;
	double dLsq = dL/SL, dCsq = dC/SC, dHsq = dH/SH;
	return dLsq*dLsq + dCsq*dCsq + dHsq*dHsq + RT*dCsq*dHsq;
#undef RAD2DEG
#undef DEG2RAD
}

double ColorCompare(int r1,int g1,int b1, int r2,int g2,int b2) {
	double luma1 = (r1*299 + g1*587 + b1*114) / (255.0*1000);
	double luma2 = (r2*299 + g2*587 + b2*114) / (255.0*1000);
	double lumadiff = luma1-luma2;
	double diffR = (r1-r2)/255.0, diffG = (g1-g2)/255.0, diffB = (b1-b2)/255.0;
	return (diffR*diffR*0.299 + diffG*diffG*0.587 + diffB*diffB*0.114)*0.75
	       + lumadiff*lumadiff;
}

/* Palette */
static const unsigned palettesize = 16;

/* Luminance for each palette entry, to be initialized as soon as the program begins */
static unsigned luma[palettesize];
static LabItem  meta[palettesize];
static double   pal_g[palettesize][3]; // Gamma-corrected palette entry

inline bool PaletteCompareLuma(unsigned index1, unsigned index2) {
	return luma[index1] < luma[index2];
}

typedef std::vector<unsigned> MixingPlan;
MixingPlan DeviseBestMixingPlan(unsigned color, size_t limit) {
	// Input color in RGB
	int input_rgb[3] = { (int)((color>>16)&0xFF),
			     (int)((color>>8)&0xFF),
			     (int)(color&0xFF)
			   };

	// Input color in CIE L*a*b*
	LabItem input(color);

	// Tally so far (gamma-corrected)
	double so_far[3] = { 0,0,0 };

	MixingPlan result;
	while(result.size() < limit) {
		unsigned chosen_amount = 1;
		unsigned chosen        = 0;

		const unsigned max_test_count = result.empty() ? 1 : result.size();

		double least_penalty = -1;
		for(unsigned index=0; index<palettesize; ++index) {
			//~ const unsigned color = pal[index];
			double sum[3] = { so_far[0], so_far[1], so_far[2] };
			double add[3] = { pal_g[index][0], pal_g[index][1], pal_g[index][2] };

			for(unsigned p=1; p<=max_test_count; p*=2) {
				for(unsigned c=0; c<3; ++c) sum[c] += add[c];
				for(unsigned c=0; c<3; ++c) add[c] += add[c];
				double t = result.size() + p;

				double test[3] = { GammaUncorrect(sum[0]/t),
						   GammaUncorrect(sum[1]/t),
						   GammaUncorrect(sum[2]/t)
						 };

#if COMPARE_RGB
				double penalty = ColorCompare(
							 input_rgb[0],input_rgb[1],input_rgb[2],
							 test[0]*255, test[1]*255, test[2]*255);
#else
				LabItem test_lab( test[0], test[1], test[2] );
				double penalty = ColorCompare(test_lab, input);
#endif
				if(penalty < least_penalty || least_penalty < 0) {
					least_penalty = penalty;
					chosen        = index;
					chosen_amount = p;
				}
			}
		}

		// Append "chosen_amount" times "chosen" to the color list
		result.resize(result.size() + chosen_amount, chosen);

		for(unsigned c=0; c<3; ++c)
			so_far[c] += pal_g[chosen][c] * chosen_amount;
	}
	// Sort the colors according to luminance
	std::sort(result.begin(), result.end(), PaletteCompareLuma);
	return result;
}

