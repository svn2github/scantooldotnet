#include <string.h>
#include "globals.h"
#include "trouble_code_reader.h"
#include "about.h"
#include "sensors.h"
#include "options.h"
#include "serial.h"
#include "custom_gui.h"
#include "main_menu.h"

#define LOGO                       1
#define INEXPENSIVE_ALTERNATIVES   2
#define SUNFIRE                    3
#define READ_CODES                 5
#define SENSOR_DATA                6
#define FREEZE_FRAME               7
#define TESTS                      8
#define OPTIONS                    9
#define ABOUT                      10
#define EXIT                       11


// procedure declarations
static int read_codes_proc(int msg, DIALOG *d, int c);
static int sensor_data_proc(int msg, DIALOG *d, int c);
static int freeze_frame_proc(int msg, DIALOG *d, int c);
static int tests_proc(int msg, DIALOG *d, int c);
static int options_proc(int msg, DIALOG *d, int c);
static int about_proc(int msg, DIALOG *d, int c);
static int exit_proc(int msg, DIALOG *d, int c);
static int button_desc_proc(int msg, DIALOG *d, int c);
static int unimplemented_proc(int msg, DIALOG *d, int c);
static int reset_proc(int msg, DIALOG *d, int c);

static char button_description[256];
static char current_description[256];
static char welcome_message[256];

static DIALOG main_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg)         (bg)     (key) (flags) (d1) (d2) (dp)                 (dp2) (dp3) */
   { d_clear_proc,      0,   0,   0,   0,   0,           C_WHITE, 0,    0,      0,   0,   NULL,                NULL, NULL },
   { d_bitmap_proc,     25,  25,  58,  430, 0,           0,       0,    0,      0,   0,   NULL,                NULL, NULL },
   { d_bitmap_proc,     115, 25,  260, 106, 0,           0,       0,    0,      0,   0,   NULL,                NULL, NULL },
   { d_bitmap_proc,     115, 141, 260, 142, 0,           0,       0,    0,      0,   0,   NULL,                NULL, NULL },
   { button_desc_proc,  115, 283, 260, 147, C_DARK_GRAY, C_WHITE, 0,    0,      0,   0,   current_description, NULL, NULL },
   { read_codes_proc,   408, 25,  207, 57,  0,           0,       0,    D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { sensor_data_proc,  408, 87,  207, 57,  0,           0,       0,    D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { freeze_frame_proc, 408, 149, 207, 57,  0,           0,       0,    D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { tests_proc,        408, 211, 207, 57,  0,           0,       0,    D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { options_proc,      408, 273, 207, 57,  0,           0,       0,    D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { about_proc,        408, 335, 207, 57,  0,           0,       0,    D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { exit_proc,         408, 397, 207, 57,  0,           0,       'x',  D_EXIT, 0,   0,   NULL,                NULL, NULL },
   { NULL,              0,   0,   0,   0,   0,           0,       0,    0,      0,   0,   NULL,                NULL, NULL }
};

static DIALOG tests_dialog[] =
{
   /* (proc)             (x)  (y)  (w)  (h)  (fg)     (bg)           (key) (flags) (d1) (d2) (dp)                           (dp2) (dp3) */
   { d_shadow_box_proc,  0,   0,   248, 248, C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,                          NULL, NULL },
   { d_shadow_box_proc,  0,   0,   248, 24,  C_BLACK, C_DARK_GRAY,   0,    0,      0,   0,   NULL,                          NULL, NULL },
   { caption_proc,       124, 2,   121, 19,  C_WHITE, C_TRANSP,      0,    0,      0,   0,   "Test Results",                NULL, NULL },
   { unimplemented_proc, 16,  40,  216, 40,  C_BLACK, C_GREEN,       'o',  D_EXIT, 0,   0,   "&Oxygen Sensors",             NULL, NULL },
   { unimplemented_proc, 16,  88,  216, 40,  C_BLACK, C_DARK_YELLOW, 't',  D_EXIT, 0,   0,   "Con&tinuously Monitored",     NULL, NULL },
   { unimplemented_proc, 16,  136, 216, 40,  C_BLACK, C_PURPLE,      'n',  D_EXIT, 0,   0,   "&Non-Continuously Monitored", NULL, NULL },
   { d_button_proc,      80,  192, 88,  40,  C_BLACK, C_WHITE,       'c',  D_EXIT, 0,   0,   "&Close",                      NULL, NULL },
   { NULL,               0,   0,   0,   0,   0,       0,             0,    0,      0,   0,   NULL,                          NULL, NULL }
};

static DIALOG reset_chip_dialog[] =
{
   /* (proc)            (x)  (y) (w)  (h) (fg)     (bg)          (key) (flags) (d1) (d2) (dp)                               (dp2) (dp3) */
   { d_shadow_box_proc, 0,   0,  300, 64, C_BLACK, C_LIGHT_GRAY, 0,    0,      0,   0,   NULL,                              NULL, NULL },
   { caption_proc,      150, 24, 138, 16, C_BLACK, C_TRANSP,     0,    0,      0,   0,   "Resetting hardware interface...", NULL, NULL },
   { reset_proc,        0,   0,  0,   0,  0,       0,            0,    0,      0,   0,   NULL,                              NULL, NULL },
   { NULL,              0,   0,  0,   0,  0,       0,            0,    0,      0,   0,   NULL,                              NULL, NULL }
};


int display_main_menu()
{
   // load all the buttons:
   // dp = original image
   // dp2 = mouseover image
   // dp3 = clicked image
   main_dialog[LOGO].dp = datafile[LOGO_BMP].dat;
   main_dialog[INEXPENSIVE_ALTERNATIVES].dp = datafile[INEXPENSIVE_ALTERNATIVES_BMP].dat;
   main_dialog[SUNFIRE].dp = datafile[SUNFIRE_BMP].dat;
   main_dialog[READ_CODES].dp = datafile[READ_CODES1_BMP].dat;
   main_dialog[SENSOR_DATA].dp = datafile[SENSOR_DATA1_BMP].dat;
   main_dialog[FREEZE_FRAME].dp = datafile[FREEZE_FRAME1_BMP].dat;
   main_dialog[TESTS].dp = datafile[TESTS1_BMP].dat;
   main_dialog[OPTIONS].dp = datafile[OPTIONS1_BMP].dat;
   main_dialog[ABOUT].dp = datafile[ABOUT1_BMP].dat;
   main_dialog[EXIT].dp = datafile[EXIT1_BMP].dat;

   main_dialog[READ_CODES].dp2 = datafile[READ_CODES2_BMP].dat;
   main_dialog[SENSOR_DATA].dp2 = datafile[SENSOR_DATA2_BMP].dat;
   main_dialog[FREEZE_FRAME].dp2 = datafile[FREEZE_FRAME2_BMP].dat;
   main_dialog[TESTS].dp2 = datafile[TESTS2_BMP].dat;
   main_dialog[OPTIONS].dp2 = datafile[OPTIONS2_BMP].dat;
   main_dialog[ABOUT].dp2 = datafile[ABOUT2_BMP].dat;
   main_dialog[EXIT].dp2 = datafile[EXIT2_BMP].dat;

   main_dialog[READ_CODES].dp3 = datafile[READ_CODES3_BMP].dat;
   main_dialog[SENSOR_DATA].dp3 = datafile[SENSOR_DATA3_BMP].dat;
   main_dialog[FREEZE_FRAME].dp3 = datafile[FREEZE_FRAME3_BMP].dat;
   main_dialog[TESTS].dp3 = datafile[TESTS3_BMP].dat;
   main_dialog[OPTIONS].dp3 = datafile[OPTIONS3_BMP].dat;
   main_dialog[ABOUT].dp3 = datafile[ABOUT3_BMP].dat;
   main_dialog[EXIT].dp3 = datafile[EXIT3_BMP].dat;

   sprintf(welcome_message, "Roll mouse cursor over menu buttons to see their descriptions.");
   strcpy(button_description, welcome_message);

   return do_dialog(main_dialog, -1);
}


void reset_chip()
{
   centre_dialog(reset_chip_dialog);
   popup_dialog(reset_chip_dialog, -1);
}


int reset_proc(int msg, DIALOG *d, int c)
{
   static int command_sent = FALSE;
   char temp_buf[80];
   
   switch (msg)
   {
      case MSG_START:
         command_sent = FALSE; // program just started, let's reset the chip
         break;
   
      case MSG_IDLE:
         if (!command_sent) // if command wasn't sent
         {
            if (serial_timer_running) // and if serial timer is running
            // wait until we either get a prompt or the timer times out
               while ((read_comport(temp_buf) != PROMPT) && !serial_time_out);  
            send_command("atz"); // reset the chip
            start_serial_timer(ATZ_TIMEOUT);  // start serial timer
            command_sent = TRUE;
         }
         else // if command was sent
         {
            if (serial_time_out || (read_comport(temp_buf) == PROMPT)) // if the timer timed out or we got the prompt
            {
               stop_serial_timer(); // stop the timer
               return D_CLOSE; // close dialog
            }
         }
         break;
   }
   
   return D_O_K;
}


int read_codes_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (msg == MSG_GOTMOUSE) // if we got mouse, display description
      sprintf(button_description, "Read codes and their definitions, turn off MIL and erase diagnostic test data.");

   if (ret == D_CLOSE)           // trap the close value
   {
     display_trouble_codes(); // display trouble code dialog
     strcpy(button_description, welcome_message);
     return D_REDRAW;
   }
   return ret;  // return
}


int sensor_data_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (msg == MSG_GOTMOUSE) // if we got mouse, display description
      sprintf(button_description, "Display current sensor data (RPM, Engine Load, Coolant Temperature, Speed, etc.)");

   if (ret == D_CLOSE)           // trap the close value
   {
      if (comport.status != READY)
      {
         if (open_comport() != 0)
         {
            comport.status = NOT_OPEN;   // reset comport status

            while (comport.status == NOT_OPEN)
            {
               if (alert("COM Port could not be opened.", "Please check that port settings are correct", "and that no other application is using it", "&Configure Port", "&Ignore", 'c', 'i') == 1)
                  display_options();
               else
                  comport.status = USER_IGNORED;
            }
         }
         else
            comport.status = READY;
      }
      display_sensor_dialog();  // display sensor data dialog
      strcpy(button_description, welcome_message);
      return D_REDRAW;
   }
   return ret;  // return
}


int freeze_frame_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (msg == MSG_GOTMOUSE) // if we got mouse, display description
      sprintf(button_description, "Display freeze frame data (not implemented in this version).");

   if (ret == D_CLOSE)           // trap the close value
   {
      alert("This feature is not implemented", " in this version", NULL, "OK", NULL, 0, 0);
      //display_freeze_frame(); // display freeze frame data
      //strcpy(button_description, welcome_message);
      return D_REDRAWME;
   }
   return ret;  // return
}


int tests_proc(int msg, DIALOG *d, int c)
{
   int ret;

   switch (msg)
   {
      case MSG_START:
         centre_dialog(tests_dialog);
         break;
         
      case MSG_GOTMOUSE: // if we got mouse, display description
         sprintf(button_description, "Display mode 5, 6, & 7 test results (not implemented in this version).");
         break;
   }

   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (ret == D_CLOSE)           // trap the close value
   {  
      popup_dialog(tests_dialog, -1);
      return D_REDRAWME;
   }
   return ret;  // return
}


int options_proc(int msg, DIALOG *d, int c)
{
   static int chip_was_reset = FALSE; 
   int ret;

   switch (msg)
   {
      case MSG_GOTMOUSE: // if we got mouse, display description
         sprintf(button_description, "Select system of measurements (US or Metric), and select serial port.");
         break;

      case MSG_IDLE:
         if (comport.status == NOT_OPEN)
         {
            if (alert("COM Port could not be opened.", "Please check that port settings are correct", "and that no other application is using it", "&Configure Port", "&Ignore", 'c', 'i') == 1)
               display_options();
            else
               comport.status = USER_IGNORED;
         }
         else if ((comport.status == READY) && (chip_was_reset == FALSE)) // if the port is ready,
         {
            reset_chip();
            chip_was_reset = TRUE;
         }
         break;
   }

   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (ret == D_CLOSE)           // trap the close value
   {
      display_options(); // display options dialog
      return D_REDRAWME;
   }
   return ret;  // return
}


int about_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (msg == MSG_GOTMOUSE) // if we got mouse, display description
      sprintf(button_description, "Learn more about this program, and find out where you can buy the OBD-II interface.");

   if (ret == D_CLOSE)           // trap the close value
   {
      display_about(); // display information about the program
      strcpy(button_description, welcome_message);
      return D_REDRAW;
   }
   return ret;  // return
}


int exit_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = nostretch_icon_proc(msg, d, c); // call the parent object

   if (msg == MSG_GOTMOUSE) // if we got mouse, display description
      sprintf(button_description, "Exit the program.");

   if (ret == D_CLOSE)           // trap the close value
   {
      if (alert("Do you really want to exit?", NULL, NULL, "&Yes", "&No", 'y', 'n') == 2)
         return D_REDRAWME;
   }
   return ret;  // return
}


int button_desc_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = super_textbox_proc(msg, d, c); // call the parent object

   if (msg == MSG_START)
      d->dp2 = datafile[ARIAL18_FONT].dat;

   if (strcmp(current_description, button_description)) // if buttons are different
   {
      strcpy(current_description, button_description);
      return D_REDRAWME;
   }

   return ret;  // return
}


int unimplemented_proc(int msg, DIALOG *d, int c)
{
   int ret;
   ret = d_button_proc(msg, d, c);
   
   if (ret == D_CLOSE)
   {
      alert("This feature is to be implemented", " in future versions", NULL, "OK", NULL, 0, 0);
      return D_REDRAWME;
   }
   
   return ret;
}
