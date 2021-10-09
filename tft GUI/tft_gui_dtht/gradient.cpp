/*******************************************************************************
* Includes
******************************************************************************/

#include "MCUFRIEND_kbv.h"
#include "gradient.h"

/*******************************************************************************
* Definitions
******************************************************************************/

/*******************************************************************************
* Prototypes
******************************************************************************/

/*******************************************************************************
* Variables
******************************************************************************/

MCUFRIEND_kbv tft;

/*******************************************************************************
* Codes Static
******************************************************************************/

/*******************************************************************************
* Codes API
******************************************************************************/

void GradRectH::draw()
{
    for (uint8_t i = 0; i < n - 1; i++)
    {
        for (int row = p[i]; row < p[i + 1]; row++)
        {
            uint8_t r, g, b;
            r = map(row, p[i], p[i + 1], color[i].r, color[i + 1].r);
            g = map(row, p[i], p[i + 1], color[i].g, color[i + 1].g);
            b = map(row, p[i], p[i + 1], color[i].b, color[i + 1].b);
            tft.drawFastHLine(x, row, len, tft.color565(r, g, b));
        }
    }
}

void GradRectV::draw()
{
    for (uint8_t i = 0; i < n - 1; i++)
    {
        for (int column = p[i]; column < p[i + 1]; column++)
        {
            uint8_t r, g, b;
            r = map(column, p[i], p[i + 1], color[i].r, color[i + 1].r);
            g = map(column, p[i], p[i + 1], color[i].g, color[i + 1].g);
            b = map(column, p[i], p[i + 1], color[i].b, color[i + 1].b);
            tft.drawFastVLine(column, y, len, tft.color565(r, g, b));
        }
    }
}

void GradCircle::draw()
{
    for (uint8_t i = 0; i < n - 1; i++)
    {
        for (int radius = p[i]; radius < p[i + 1]; radius++)
        {
            uint8_t r, g, b;
            r = map(radius, p[i], p[i + 1], color[i].r, color[i + 1].r);
            g = map(radius, p[i], p[i + 1], color[i].g, color[i + 1].g);
            b = map(radius, p[i], p[i + 1], color[i].b, color[i + 1].b);
            tft.drawCircle(x, y, radius, tft.color565(radius, g, b));
        }
    }
}

GradLineH::GradLineH(int xx, int yy, int lenn, int lw, Color c1, Color c2)
{
    y = yy;
    p[0] = xx;
    p[1] = xx + lenn / 4;
    p[2] = xx + 3 * lenn / 4;
    p[3] = xx + lenn;
    len = lw;
    n = 4;
    color[0] = color[3] = c2;
    color[1] = color[2] = c1;
}

ImageFromSRAM::ImageFromSRAM(uint16_t* dt, uint16_t ww, uint16_t hh)
{
    arr = dt;
    w = ww, h = hh;
}
void draw(int x, int y)
{
    uint32_t i = 0;
    for (int row = h - 1; row >= 0; row--)
    {
        for (int c = 0; c < w; c++)
        {
            tft.drawPixel(x + c, y + row, pgm_read_word_near(arr + i));
            i++;
        }
    }
}

