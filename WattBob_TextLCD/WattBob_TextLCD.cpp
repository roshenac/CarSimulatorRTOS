/* draft mbed TextLCD 
 * (c) 2007/8, sford
 * Modified jherd
 */
 
#include "WattBob_TextLCD.h"
#include "MCP23017.h"

#include "mbed.h"

/*
 * Initialisation
 * ==============
 *
 * After attaching the supply voltage/after a reset, the display needs to be brought in to a defined state
 *
 * - wait approximately 15 ms so the display is ready to execute commands
 * - Execute the command 0x30 ("Display Settings") three times (wait 1,64ms after each command, the busy flag cannot be queried now). 
 * - The display is in 8 bit mode, so if you have only connected 4 data pins you should only transmit the higher nibble of each command.
 * - If you want to use the 4 bit mode, now you can execute the command to switch over to this mode now.
 * - Execute the "clear display" command
 *
 * Timing
 * ======
 *
 * Nearly all commands transmitted to the display need 40us for execution. 
 * Exceptions are the commands "Clear Display and Reset" and "Set Cursor to Start Position" 
 * These commands need 1.64ms for execution. These timings are valid for all displays working with an 
 * internal clock of 250kHz. But I do not know any displays that use other frequencies. Any time you 
 * can use the busy flag to test if the display is ready to accept the next command.
 * 
 * _e is kept high apart from calling clock
 * _rw is kept 0 (write) apart from actions that uses it differently
 * _rs is set by the data/command writes
 * 
 * RS = 7
 * RW = 6
 * E  = 5
 * Back light = 4
 * D4 = 0
 * D5 = 1  
 * D6 = 2
 * D7 = 3  
 */

WattBob_TextLCD::WattBob_TextLCD(MCP23017 *port) {
//
// Initialise pointer to MCP23017 object
//
    par_port = port;
    par_port->config(0x0F00, 0x0F00, 0x0F00);
    
    _rows = 2;
    _columns = 16;

    // 
    // Time to allow unit to initialise
    //
     wait(DISPLAY_INIT_DELAY_SECS); 
     
    _rw(0);
    _e(0);
    _rs(0); // command mode
    
    //
    // interface defaults to an 8-bit interface. However, we need to ensure that we
    // are in 8-bit mode
    //   
    for(int i=0; i<3; i++) {
        writeNibble(CMD4_SET_8_BIT_INTERFACE);
        wait(0.00164);      // this command takes 1.64ms, so wait for it
    }
    
    writeNibble(CMD4_SET_4_BIT_INTERFACE);       // now force into 4-bit mode   
    
//    writeCommand(CMD_NULL);
//    writeCommand(CMD_NULL);
//    writeCommand(CMD_NULL);
//    writeCommand(CMD_NULL);
//    writeCommand(CMD_NULL);
          
    writeCommand(CMD_FUNCTION_SET | INTERFACE_4_BIT | TWO_LINE_DISPLAY | FONT_5x8 | ENGL_JAPAN_FONT_SET); //  0x28
    writeCommand(CMD_DISPLAY_CONTROL | DISPLAY_ON | CURSOR_OFF | CURSOR_CHAR_BLINK_OFF); // 0xC0
    cls();
    writeCommand(CMD_RETURN_HOME);    
    writeCommand(CMD_ENTRY_MODE | CURSOR_STEP_RIGHT | DISPLAY_SHIFT_OFF );  // 0x06
}

int WattBob_TextLCD::_putc(int value) {
    if(value == '\n') {
        newline();
    } else {
        writeData(value);
    }
    return value;
}

int WattBob_TextLCD::_getc() {
    return 0;
}

void WattBob_TextLCD::newline() {
    _column = 0;
    _row++;
    if(_row >= _rows) {
        _row = 0;
    }
    locate(_column, _row); 
}

void WattBob_TextLCD::locate(int row, int column) {
    if(column < 0 || column >= _columns || row < 0 || row >= _rows) {
        // error("locate(%d,%d) out of range on %dx%d display", column, row, _columns, _rows);
        return;
    }
    
    _row = row;
    _column = column;
    int address = 0x80 + (_row * 0x40) + _column; // memory starts at 0x80, and internally it is 40 chars per row (only first 16 used)
    writeCommand(address);            
}

void WattBob_TextLCD::cls() {
    writeCommand(CMD_CLEAR_DISPLAY);  // 0x01
    wait(DISPLAY_CLEAR_DELAY);                    // 
    locate(0, 0);
}

void WattBob_TextLCD::reset() {
    cls();
}

void WattBob_TextLCD::clock() {
    wait(0.000040f);
    _e(1);
    wait(0.000040f);  // most instructions take 40us
    _e(0);    
}

void WattBob_TextLCD::writeNibble(int value) {
    _d(value);
    clock();
}

void WattBob_TextLCD::writeByte(int value) {
    writeNibble((value >> 4) & 0x000F);
    writeNibble((value >> 0) & 0x000F);
}

void WattBob_TextLCD::writeCommand(int command) {
    _rs(0);
    writeByte(command);
}

void WattBob_TextLCD::writeData(int data) {
    _rs(1);
    writeByte(data);
    _column++;
    if(_column >= _columns) {
        newline();
    } 
}

void WattBob_TextLCD::_rs(int data) {
    par_port->write_bit(data, RS_BIT);
}

void WattBob_TextLCD::_rw(int data) {
    par_port->write_bit(data, RW_BIT);
}

void WattBob_TextLCD::_e(int data) {
    par_port->write_bit(data, E_BIT);
}

void WattBob_TextLCD::_d(int data) {
    par_port->write_mask((unsigned short)data, (unsigned short)0x000F);
}
