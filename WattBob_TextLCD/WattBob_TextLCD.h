/* draft mbed TextLCD 
 * (c) 2007/8, sford
 */
 
#ifndef WATTBOB_TEXTLCD_H
#define WATTBOB_TEXTLCD_H

#include "mbed.h"
#include "Stream.h"
#include "MCP23017.h"

#define     RS_BIT      7
#define     RW_BIT      6
#define     E_BIT       5
#define     BL_BIT      4   

//
// Registers and bit definitions for 2*16 character display chip
//
#define     CMD_NULL                0x00

#define     CMD_CLEAR_DISPLAY       0x01

#define     CMD_RETURN_HOME         0x02

#define     CMD_ENTRY_MODE          0x04
#define       CURSOR_STEP_LEFT      0x00
#define       CURSOR_STEP_RIGHT     0x02
#define       DISPLAY_SHIFT_OFF     0x00
#define       DISPLAY_SHIFT_ON      0x01
       
#define     CMD_DISPLAY_CONTROL     0x08
#define       DISPLAY_OFF           0x00
#define       DISPLAY_ON            0x04
#define       CURSOR_OFF            0x00
#define       CURSOR_ON             0x02
#define       CURSOR_CHAR_BLINK_OFF 0x00
#define       CURSOR_CHAR_BLINK_ON  0x01

#define     CMD_CURSOR_SHIFT        0x10
#define       SHIFT_CURSOR_LEFT     0x00
#define       SHIFT_CURSOR_RIGHT    0x04
#define       SHIFT_DISPLAY_LEFT    0x08
#define       SHIFT_DISPLAY_RIGHT   0x0C

#define     CMD_MODE_POWER          0x13
#define       CHARACTER_MODE        0x00
#define       GRAPHICS_MODE         0x08
#define       INTERNAL_POWER_OFF    0x00
#define       INTERNAL_POWER_ON     0x04

#define     CMD_FUNCTION_SET        0x20
#define       ENGL_JAPAN_FONT_SET   0x00
#define       EUROPE_FONT_SET       0x01
#define       ENGL_RUSSIAN_FONT_SET 0x20
#define       FONT_5x8              0x00
#define       FONT_5x10             0x04
#define       ONE_LINE_DISPLAY      0x00
#define       TWO_LINE_DISPLAY      0x08
#define       INTERFACE_4_BIT       0x00
#define       INTERFACE_8_BIT       0x10

#define     CMD_SET_CGRAM_ADDRESS   0x40

#define     CMD_SET_DDRAM_ADDRESS   0x80
//
// nibble commands
//
#define     CMD4_SET_4_BIT_INTERFACE 0x2
#define     CMD4_SET_8_BIT_INTERFACE 0x3
//
// Misc 2*16 character display constants
//
#define     DISPLAY_INIT_DELAY_SECS    0.5f       // 500mS
#define     DISPLAY_CLEAR_DELAY        0.01f      // 10 mS (spec is 6.2mS)

/** Class to access 16*2 LCD display connected to an MCP23017 I/O extender chip
 *
 * Derived from the "stream" class to be able to use methods such as "printf"
 *
 * Example :
 * @code
 * .....
 * #include "MCP23017.h"
 * #include "WattBob_TextLCD.h"
 * .....
 * MCP23017            *par_port;
 * WattBob_TextLCD     *lcd;
 *      .....
 * int main()
 *      par_port = new MCP23017(p9, p10, 0x40);
 *      par_port->config(0x0F00, 0x0F00, 0x0F00);           // configure MCP23017 chip on WattBob       
 *      lcd = new WattBob_TextLCD(par_port);
 *
 *      par_port->write_bit(1,BL_BIT);   // turn LCD backlight ON
 *      lcd->cls(); lcd->locate(0,0);
 *      lcd->printf("%s", message);
 *      lcd->locate(1,0);lcd->printf("press 1 to cont"); 
 * @endcode
 */ 
class WattBob_TextLCD : public Stream {

public:
    /** Create TextLCD object connected to a MCP23017 device
     *
     * @param   port    pointer to MCP23017 object
     */ 
    WattBob_TextLCD(MCP23017 *port);
    
    /** Set cursor to a known point
     *
     * Virtual function for stream class
     *
     * @param   row   integer row number (0 or 1)
     * @param   col   integer column number (0 or 15)     
     */            
    virtual void locate(int row, int column);
    
    /** clear display
     *
     * Virtual function for stream class
     */     
    virtual void cls();

    /** reset the display
     *
     * Virtual function for stream class
     */             
    virtual void reset();
        
protected:

    virtual int _putc(int c);        
    virtual int _getc();
    virtual void newline();      
    
    void clock();
    void writeData(int data);
    void writeCommand(int command);
    void writeByte(int value);
    void writeNibble(int value);
    
    void _rs (int data);
    void _rw (int data);    
    void _e (int data);
    void _d (int data);
           
    int _rows;
    int _columns;
    int _row;
    int _column;   
    
private:
    MCP23017    *par_port; 
};

#endif
