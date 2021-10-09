#ifndef _GUI_ELEMENT_
#define _GUI_ELEMENT_

/*******************************************************************************
* Definitions
******************************************************************************/

// Assign human-readable names to some common 16-bit color values:
#define BLACK       0x0000
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0
#define WHITE       0xFFFF
#define Transparent GREEN
#define Backcolor   0x967
#define GRAY        0x630c

#define MAX 4 //gradient max stop point

#define MAX_BMP      10 // bmp file num
#define FILENAME_LEN 20 // max file name length


class ImageFromSRAM {
  public:
    uint16_t* arr;
    uint16_t w, h;

    ImageFromSRAM(uint16_t* dt, uint16_t ww, uint16_t hh);
};

/*******************************************************************************
* API
******************************************************************************/



#endif /* _GUI_ELEMENT_ */
