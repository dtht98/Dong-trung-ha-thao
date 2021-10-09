#ifndef _GRADIENT_
#define _GRADIENT_

/*******************************************************************************
* Definitions
******************************************************************************/

#define MAX 8

/* RGB */
typedef struct
{
    uint8_t r, g, b;
} Color;

/* Gradient parrent */
class Grad
{
  public:
    int p[MAX];
    int n;
    Color color[MAX]; //Maximum 2, can be changed, at least 2
};

/* Gradient horizon */
class GradRectH : public Grad
{
  public:
    int x, len;
    void draw();
};

/* Gradient vertical */
class GradRectV : public Grad
{
  public:
    int y, len;
    void draw();
};

/* Gradient circle */
class GradCircle : public Grad
{
  public:
    int x, y;
    // p[] is an array of radius in this case
    void draw();
};

/* Gradient line (two sides fading out) */
class GradLineH : public GradRectV
{
  public:
    /**
     * @brief Construct a new Grad Line H object
     *
     * @param xx X coordinate
     * @param yy Y coordinate
     * @param lenn Line length
     * @param lw Line width
     * @param c1 Color in the middle
     * @param c2 Color in two side (fading out)
     */
    GradLineH(int xx, int yy, int lenn, int lw, Color c1, Color c2);
};

/*******************************************************************************
* API
******************************************************************************/

#endif /* _GRADIENT_ */
