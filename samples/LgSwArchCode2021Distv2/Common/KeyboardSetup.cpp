//------------------------------------------------------------------------------------------------
// File: KeyboardSetup.cpp
// Project: LG Exec Ed Program
// Versions:
// 1.0 April 2017 - initial version
// Configures the Keyboard so that the enter key does have to be hit to read a character, also 
// the keyboard will not echo characters and the getchar function will not block.
//------------------------------------------------------------------------------------------------
#include <unistd.h>
#include <termios.h>    /*termios, TCSANOW, ECHO, ICANON */
#include <fcntl.h>
#include "KeyboardSetup.h"

static struct termios saved_attributes;
static int            saved_file_status_flags;
static bool           KeyboardConfigured=false;

//-----------------------------------------------------------------
// ConfigKeyboardNoEnterBlockEcho - Configures the Keyboard so that 
// the enter key does have to be hit to read a character, also 
// the keyboard will not echo characters and the getchar function 
// will not block. The prevous state of the keyboard is alos saved
//-----------------------------------------------------------------
void ConfigKeyboardNoEnterBlockEcho(void)
{
 struct termios   modified_attributes;

 if (KeyboardConfigured) return;
 /*tcgetattr gets the parameters of the current terminal
   STDIN_FILENO will tell tcgetattr that it should write the settings
   of stdin to oldt*/
 tcgetattr( STDIN_FILENO, &saved_attributes);
 /*now the settings will be copied*/
 modified_attributes= saved_attributes;

 /*ICANON normally takes care that one line at a time will be processed
 that means it will return if it sees a "\n" or an EOF or an EOL*/
 modified_attributes.c_lflag &= ~(ICANON | ECHO);

 /*Those new settings will be set to STDIN
 TCSANOW tells tcsetattr to change attributes immediately. */
 tcsetattr( STDIN_FILENO, TCSANOW, &modified_attributes);
 saved_file_status_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
 fcntl(STDIN_FILENO, F_SETFL, saved_file_status_flags| O_NONBLOCK);
 KeyboardConfigured=true;
}
//-----------------------------------------------------------------
// END ConfigKeyboardNoEnterBlockEcho
//-----------------------------------------------------------------
//----------------------------------------------------------------
// RestoreKeyboard - Restores the Keyboard back to it's normal
// state
//-----------------------------------------------------------------
void RestoreKeyboard(void)
{
 if (!KeyboardConfigured)  return;
 /*restore the old settings*/
 fcntl(STDIN_FILENO, F_SETFL, saved_file_status_flags);
 tcsetattr( STDIN_FILENO, TCSANOW, &saved_attributes);
 KeyboardConfigured=false;
}
//-----------------------------------------------------------------
// END RestoreKeyboard
//-----------------------------------------------------------------
//-----------------------------------------------------------------
// END of File
//-----------------------------------------------------------------
