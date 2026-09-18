#include "sim.h"

#define N_STARS 5000
#define N_STEPS 10000

// Fixed polong Q16.16
// 65536 -> 1.0
const long PI_FIX     = 205887;   // pi * 65536
const long K_INV      = 39797;    // 1/K for CORDIC
const long Z_MIN      = 16000; 
const long Z_MAX      = 100000; 
const long z_range = Z_MAX - Z_MIN;
const long DZ         = 700;
const long SWIRL      = 6500;
const long SCALE      = SIM_X_SIZE * 55 / 100;
const long CX         = SIM_X_SIZE / 2;
const long CY         = SIM_Y_SIZE / 2;

// atan(2^-i) table for CORDIC (Q16.16)
const long atan_table[16] = {
    51472, 30386, 16055, 8149, 4091, 2047, 1024, 512,
    256, 128, 64, 32, 16, 8, 4, 2
};

void setSinCosTables(long* sin_table, long* cos_table, const long* atan_table);
void putCross(long x, long y, long argb);
long colorShift();

void app(void) {
    long sx[N_STARS], sy[N_STARS], sz[N_STARS];  // stars positions
    long cr[N_STARS], cg[N_STARS], cb[N_STARS];  // color shifts (for randomization of purple)

    long cos_table[256];
    long sin_table[256];

    setSinCosTables(sin_table, cos_table, atan_table);

    for (long i = 0; i < N_STARS; ++i) {
        sx[i] = (simRand() % 2001 - 1000) * 65536 / 1000;
        sy[i] = (simRand() % 2001 - 1000) * 65536 / 1000;
        sz[i] = Z_MIN + (simRand() % 10000) * (Z_MAX - Z_MIN) / 10000;

        long shift = colorShift(); 
        cr[i] =  shift;
        cb[i] = -shift;             
        cg[i] = colorShift();   
    }

    for (long step = 0; step < N_STEPS; ++step) {
        // Stop by click
        if (simHasClick()) {
            simFlush();
            return;
        }

        for (long i = 0; i < N_STARS; ++i) {
            long z = sz[i];
            long zp = z + DZ * 6;

            // Current angular
            long idx = (((z - Z_MIN) * 255) / z_range);
            if (idx < 0) idx = 0;
            if (idx > 255) idx = 255;
            long cos_val = cos_table[idx];
            long sin_val = sin_table[idx];

            // Previous angular
            long idxp = (((zp - Z_MIN) * 255) / z_range);
            if (idxp < 0) idxp = 0;
            if (idxp > 255) idxp = 255;
            long cos_val_p = cos_table[idxp];
            long sin_val_p = sin_table[idxp];

            // Rotate (multiply to the rotation matrix)
            long rx  = ((sx[i] * cos_val   - sy[i] * sin_val  ) >> 16);
            long ry  = ((sx[i] * sin_val   + sy[i] * cos_val  ) >> 16);
            long rxp = ((sx[i] * cos_val_p - sy[i] * sin_val_p) >> 16);
            long ryp = ((sx[i] * sin_val_p + sy[i] * cos_val_p) >> 16);

            // Display projection
            long x0 = CX + ((rx  * SCALE) / z);
            long y0 = CY + ((ry  * SCALE) / z);
            long x1 = CX + ((rxp * SCALE) / zp);
            long y1 = CY + ((ryp * SCALE) / zp);

            // brightness
            long b = (((Z_MAX - z) * 255) / z_range);
            if (b < 0) b = 0;
            if (b > 255) b = 255;

            // Set the violet color R = b, G = b/3, B = b
            long r = b + cr[i];
            long g = b / 3 + cg[i];
            long bl = b + cb[i];

            if (r  < 0) r  = 0;  
            if (r  > 255) r  = 255;
            if (g  < 0) g  = 0;  
            if (g  > 255) g  = 255;
            if (bl < 0) bl = 0;  
            if (bl > 255) bl = 255;

            long argb = 0xFF000000 | (r << 16) | (g << 8) | bl;


            long dx = x0 - x1;
            long dy = y0 - y1;
            long adx = dx < 0 ? -dx : dx;
            long ady = dy < 0 ? -dy : dy;
            long len = adx > ady ? adx : ady;
            if (len < 1) len = 1;

            for (long k = 0; k <= len; ++k) {
                long x = x1 + dx * k / len;
                long y = y1 + dy * k / len;
                if (x >= 0 && x < SIM_X_SIZE && y >= 0 && y < SIM_Y_SIZE)
                    putCross(x, y, argb);
            }

            // Stars motion into the display
            sz[i] -= DZ;
            if (sz[i] < Z_MIN) {
                sx[i] = (simRand() % 2001 - 1000) * 65536 / 1000;
                sy[i] = (simRand() % 2001 - 1000) * 65536 / 1000;
                sz[i] = Z_MAX;
            }
        }

        simFlush();
    }
}

long colorShift() {
    return (simRand() % 61) - 40;
}

// put cross on the screen (5 polongs)
void putCross(long x, long y, long argb) {
    simPutPixel(x, y, argb);
    if (x + 1 < SIM_X_SIZE) simPutPixel(x + 1, y, argb);
    if (x - 1 >= 0)         simPutPixel(x - 1, y, argb);
    if (y + 1 < SIM_Y_SIZE) simPutPixel(x, y + 1, argb);
    if (y - 1 >= 0)         simPutPixel(x, y - 1, argb);
}

// Prefill sin and cos tables with 256 depth (CORDIC)
void setSinCosTables(long* sin_table, long* cos_table, const long* atan_table) {
    for (long i = 0; i < 256; ++i) {
        long z = Z_MIN + ((z_range * i) / 255);
        long t = ((SWIRL << 16) / z);

        long angle = t;
        long sign_sin = 1, sign_cos = 1;
        if (angle > PI_FIX) {
            angle -= PI_FIX;
            sign_sin = -1;
            sign_cos = -1;
        }
        if (angle >  PI_FIX / 2) {
            angle = PI_FIX - angle;
            sign_cos = -sign_cos;
        }

        long x = K_INV;
        long y = 0;
        long zc = angle;
        for (long j = 0; j < 16; ++j) {
            long x_new, y_new;
            if (zc >= 0) {
                x_new = x - (y >> j);
                y_new = y + (x >> j);
                zc -= atan_table[j];
            } else {
                x_new = x + (y >> j);
                y_new = y - (x >> j);
                zc += atan_table[j];
            }
            x = x_new;
            y = y_new;
        }
        if (sign_sin < 0) y = -y;
        if (sign_cos < 0) x = -x;

        cos_table[i] = x;
        sin_table[i] = y;
    }
}
