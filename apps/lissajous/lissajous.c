#define USE_MAIN
#include "uvm32_target.h"

// https://github.com/shastro/CodeGolf-Lissajous/blob/master/lissa.c
// https://github.com/nicbk/donut-embedded/blob/ad8061ece9a2c156694c39af12f109ea6778da52/donut.c#L57

#define PI 3.14159265358979323846  // First 21 digits of pi

void movecursor(int x, int y) {
    print("\033[");
    printdec(y);
    print(";");
    printdec(x);
    print("f");
}

void sleep(uint32_t ms) {
    uint32_t start = millis();
    while (millis() < start + ms) {
        yield();
    }
}

// Absolute value function for doubles
double abs_c(double x) {
    if (x < 0) {
        return -x;
    } else {
        return x;
    }
}

// Recursive factorial capable of calculating a max of approximately 10^18 in size
unsigned long long fac(unsigned char x) {
    if (x == 0) {
        return 1;
    } else {
        return x * fac(x - 1);
    }
}

// Exponential function
// Ignores edge cases such as indeterminate forms for simplicity
double pow_c(double x, unsigned char n) {
    if (n == 0) {
        return 1;
    } else {
        double product = x;

        for (int i = 1; i < n; ++i) {
            product *= x;
        }

        return product;
    }
}

// MacLaurin series approximation of sin(x) from -PI/2 to PI/2
// with eight terms, and extended to the domain of all
// real numbers with modular arithmetic.
double sin(double x) {
    double translation = 0;
    if (x > -PI / 2) {
        translation = (int)((x + PI / 2) / PI);
    } else {
        translation = (int)((x - PI / 2) / PI);
    }

    x -= PI * translation;
    if ((int)abs_c(translation) % 2 == 1) {
        x = -x;
    }

    double sum = 0;

    for (int i = 0; i < 9; ++i) {
        sum += pow_c(-1, i) * pow_c(x, 2 * i + 1) / fac(2 * i + 1);
    }

    return sum;
}

// cos(x) function derived from sin(x)
double cos(double x) {
    return sin(PI / 2 - x);
}

void main(void) {
    float freq1 = 45;
    float freq2 = 90;

    for (int i = 0; i < 300; i++) {
        putc('\n');
    }
    print("\033[H");     // Cursor at Corner of Screen
    print("\033[?25l");  // hide cursor

    float x = 0;
    float y = 0;
    float angle = 0;
    float beta = 0.0;

    while (1) {
        uint32_t framestart = millis();

        print("\033[?2026h");  // pause updates
        print("\033[2J");      // cls

        for (angle = 0; angle < 2 * PI; angle += 0.2) {
            movecursor(x, y);
            x = 20 * cos(freq1 * angle + beta) + 30;
            y = 10 * sin(freq2 * angle) + 15;
            print("#");
        }

        print("\033[?2026l");  // resume updates

        // wait for next frame
        while (millis() < framestart + (1000 / 30)) {
            yield();
        }
        beta += 0.05;
        freq1 += 0.01;
        freq2 += 0.005;
        if (getc() != 0xFFFFFFFF) {
            return;
        }
    }
}
