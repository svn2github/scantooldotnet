#include <string.h>
#include <ctype.h>
#include "globals.h"
#include "serial.h"
#include "options.h"
#include "custom_gui.h"
#include "error_handlers.h"
#include "trouble_code_reader.h"

#define MSG_READ_CODES    MSG_USER
#define MSG_CLEAR_CODES   MSG_USER+1
#define MSG_READY         MSG_USER+2

#define NUM_OF_CODES   0
#define READ_CODES     1
#define CLEAR_CODES    2

#define FIELD_DELIMITER     '\t'
#define RECORD_DELIMITER    0xA

#define SIM_CODES_STRING   "43012507360455\n43114301960234\n43044302990357\n43C001C101C106"

#define NUM_OF_RETRIES   3

static char unknown_code_description[] = "Manufacturer-specific code.  Please refer to your vehicle's service manual for more information";

typedef struct TROUBLE_CODE
{
   char code[6];
   char *description;
   char *solution;
   struct TROUBLE_CODE *next;
} TROUBLE_CODE;


static int num_of_codes = 0;
static int num_of_codes_reported = 0;
static int current_code_index;
static int simulate = FALSE;
static int mil_is_on; // MIL is ON or OFF

static TROUBLE_CODE trouble_codes[] = {{"", NULL, NULL, NULL}};

static TROUBLE_CODE *add_trouble_code(const TROUBLE_CODE *);
static TROUBLE_CODE *get_trouble_code(int index);
static int get_number_of_codes();
static void clear_trouble_codes();

// procedure definitions:
static int tr_code_proc(int msg, DIALOG *d, int c);
static int mil_status_proc(int msg, DIALOG *d, int c);
static int mil_text_proc(int msg, DIALOG *d, int c);
static int simulate_proc(int msg, DIALOG *d, int c);
static int num_of_codes_proc(int msg, DIALOG *d, int c);
static int code_list_proc(int msg, DIALOG *d, int c);
static char* code_list_getter(int index, int *list_size);
static int read_tr_codes_proc(int msg, DIALOG *d, int c);
static int clear_codes_proc(int msg, DIALOG *d, int c);
static int tr_description_proc(int msg, DIALOG *d, int c);
static int tr_solution_proc(int msg, DIALOG *d, int c);
static int current_code_proc(int msg, DIALOG *d, int c);

// function definitions:
static void trouble_codes_simulator(int show);
static int handle_num_of_codes(char *);
static int handle_read_codes(const char *);
static void swap_strings(char *, char *);
static void handle_errors(int error, int operation);
static PACKFILE *file_handle(char code_letter);

static DIALOG read_codes_dialog[] =
{
   /* (proc)              (x)  (y)  (w)  (h)  (fg)    (bg)           (key) (flags) (d1) (d2) (dp)                                   (dp2) (dp3) */
   { d_clear_proc,        0,   0,   0,   0,   0,       C_WHITE,       0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { d_box_proc,          25,  25,  112, 24,  C_BLACK, C_DARK_GRAY,   0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { caption_proc,        81,  28,  54,  19,  C_WHITE, C_TRANSP,      0,    0,      0,   0,   "Current DTC",                         NULL, NULL },
   { d_box_proc,          25,  49,  112, 40,  C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { current_code_proc,   81,  57,  54,  24,  C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { code_list_proc,      25,  96,  112, 208, C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   code_list_getter,                      NULL, NULL },
   { d_box_proc,          25,  311, 112, 64,  C_BLACK, C_WHITE,       0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { num_of_codes_proc,   81,  319, 54,  24,  C_BLACK, C_WHITE,       0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { st_ctext_proc,       80,  351, 55,  20,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "DTCs Found",                          NULL, NULL },
   { d_box_proc,          25,  383, 112, 72,  C_BLACK, C_WHITE,       0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { mil_status_proc,     56,  390, 50,  30,  0,       0,             0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { mil_text_proc,       81,  430, 54,  23,  C_BLACK, C_WHITE,       0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { d_box_proc,          152, 25,  463, 24,  C_BLACK, C_DARK_GRAY,   0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { caption_proc,        383, 28,  230, 19,  C_WHITE, C_TRANSP,      0,    0,      0,   0,   "Diagnostic Trouble Code Definition",  NULL, NULL },
   { tr_description_proc, 152, 49,  463, 100, C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { d_box_proc,          152, 164, 463, 25,  C_BLACK, C_DARK_GRAY,   0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { caption_proc,        383, 167, 230, 20,  C_WHITE, C_TRANSP,      0,    0,      0,   0,   "Possible Causes And Known Solutions", NULL, NULL },
   { tr_solution_proc,    152, 189, 463, 185, C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { simulate_proc,       152, 408, 88,  16,  C_BLACK, C_WHITE,       0,    0,      1,   0,   "Simulate",                            NULL, NULL },
   { read_tr_codes_proc,  256, 408, 110, 48,  C_BLACK, C_GREEN,       'r',  D_EXIT, 0,   0,   "&Read",                               NULL, NULL },
   { clear_codes_proc,    381, 408, 110, 48,  C_BLACK, C_DARK_YELLOW, 'c',  D_EXIT, 0,   0,   "&Clear",                              NULL, NULL },
   { d_button_proc,       506, 408, 110, 48,  C_BLACK, C_GREEN,       'm',  D_EXIT, 0,   0,   "Main &Menu",                          NULL, NULL },
   { tr_code_proc,        0,   0,   0,   0,   0,     0,               0,    0,      0,   0,   NULL,                                  NULL, NULL },
   { NULL,                0,   0,   0,   0,   0,     0,               0,    0,      0,   0,   NULL,                                  NULL, NULL }
};

static DIALOG confirm_clear_dialog[] =
{
   /* (proc)             (x)  (y)  (w)  (h)  (fg)     (bg)          (key) (flags) (d1) (d2) (dp)                                 (dp2) (dp3) */
   { d_shadow_box_proc,  0,   0,   327, 248, C_BLACK, C_WHITE,       0,    0,      0,   0,   NULL,                                NULL, NULL },
   { d_shadow_box_proc,  0,   0,   327, 24,  C_BLACK, C_DARK_GRAY,   0,    0,      0,   0,   NULL,                                NULL, NULL },
   { caption_proc,       164, 2,   160, 19,  C_WHITE, C_TRANSP,      0,    0,      0,   0,   "Clear Trouble Codes",               NULL, NULL },
   { super_textbox_proc, 24,  32,  279, 136, C_BLACK, C_WHITE,       0,    0,      0,   0,   "This will reset the MIL, clear any existing trouble codes, freeze frame data, and on-board monitoring test results. The loss of data may cause the vehicle to run poorly for a short period of time while the system recalibrates itself.", NULL, NULL },
   { st_ctext_proc,      163, 169, 161, 24,  C_RED,   C_TRANSP,      0,    0,      0,   0,   "Are you sure you want to do this?", NULL, NULL },
   { d_button_proc,      16,  198, 140, 35,  C_BLACK, C_GREEN,       'y',  D_EXIT, 0,   0,   "&Yes, I am sure",                   NULL, NULL },
   { d_button_proc,      171, 198, 140, 35,  C_BLACK, C_DARK_YELLOW, 'n',  D_EXIT, 0,   0,   "&No, cancel",                       NULL, NULL },
   { NULL,               0,   0,   0,   0,   0,     0,               0,    0,      0,   0,   NULL,                                NULL, NULL }
}; 


int display_trouble_codes()
{
   int ret;
   
   mil_is_on = FALSE; // reset MIL status
   num_of_codes = 0;
   num_of_codes_reported = 0;
   
   centre_dialog(confirm_clear_dialog);  // center the popup dialog

   if (comport.status == USER_IGNORED)
      comport.status = NOT_OPEN;

   ret = do_dialog(read_codes_dialog, -1);
   
   if (get_number_of_codes() > 0)    // if the structure is not empty,
      clear_trouble_codes();
      
   return ret;
}


void trouble_codes_simulator(int show)
{
   char buf[128];
   
   if (show)
   {
      strcpy(buf, SIM_CODES_STRING);
      process_response(NULL, buf);
      num_of_codes_reported = num_of_codes = handle_read_codes(buf);
      mil_is_on = TRUE;
   }
   else
      num_of_codes_reported = num_of_codes = 0;

   broadcast_dialog_message(MSG_READY, 0);
}
      

int current_code_proc(int msg, DIALOG *d, int c)
{
   switch (msg)
   {
      case MSG_START:
         d->dp2 = datafile[ARIAL18_FONT].dat;
         d->dp = empty_string;
         break;

      case MSG_READY:
         if (num_of_codes == 0)
            d->dp = empty_string;
         else
            d->dp = get_trouble_code(current_code_index)->code;
         return D_REDRAWME;
      
      case MSG_DRAW:
         rectfill(screen, d->x-d->w, d->y, d->x+d->w-1, d->y+d->h-1, d->bg);  // clear the element
         break;
   }
      
   return st_ctext_proc(msg, d, c);
}


int tr_description_proc(int msg, DIALOG *d, int c)   // procedure which displays a textbox
{
   switch (msg)
   {
      case MSG_START:
         d->dp = empty_string;
         break;

      case MSG_READY:   // when it receives MSG_READY
         if (num_of_codes == 0)
            d->dp = empty_string;
         else if (!(d->dp = get_trouble_code(current_code_index)->description))
            d->dp = unknown_code_description;
         return D_REDRAWME;
   }
      
   return d_textbox_proc(msg, d, c);
}


int tr_solution_proc(int msg, DIALOG *d, int c)   // procedure which displays a textbox
{
   switch (msg)
   {
      case MSG_START:
         d->dp = empty_string;
         break;
      
      case MSG_READY:   // when it receives MSG_READY
         if (num_of_codes == 0)
            d->dp = empty_string;
         else if(!(d->dp = get_trouble_code(current_code_index)->solution))
            d->dp = empty_string;
         return D_REDRAWME;
   }

   return d_textbox_proc(msg, d, c);
}


int num_of_codes_proc(int msg, DIALOG *d, int c)
{
   switch (msg)
   {
      case MSG_START:
         d->dp2 = datafile[ARIAL18_FONT].dat;
         if (!(d->dp = calloc(8, sizeof(char))))
            fatal_error("Could not allocate enough memory for num_of_codes_proc buffer");
         strcpy(d->dp, "0");
         break;

      case MSG_END:
         free(d->dp);
         break;
         
      case MSG_DRAW:
         rectfill(screen, d->x-d->w, d->y, d->x+d->w-1, d->y+d->h-1, d->bg);  // clear the element
         break;

      case MSG_READY:
         sprintf(d->dp, "%i", num_of_codes_reported);
         return D_REDRAWME;
   }
   
   return st_ctext_proc(msg, d, c);
}


int mil_status_proc(int msg, DIALOG *d, int c)
{
   switch (msg)
   {
      case MSG_START:
         d->dp = datafile[MIL_OFF_BMP].dat;
         break;

      case MSG_READY:
         if (num_of_codes_reported == 0)
            mil_is_on = FALSE;
            
         if (mil_is_on)
            d->dp = datafile[MIL_ON_BMP].dat;
         else
            d->dp = datafile[MIL_OFF_BMP].dat;
            
         return D_REDRAWME;
   }
   
   return d_bitmap_proc(msg, d, c);
}


int mil_text_proc(int msg, DIALOG *d, int c)
{
   switch (msg)
   {
      case MSG_START:
         if (!(d->dp = calloc(16, sizeof(char))))
            fatal_error("Could not allocate enough memory for mil_text_proc buffer");
         strcpy(d->dp, "MIL is OFF");
         break;
         
      case MSG_END:
         free(d->dp);
         break;
         
      case MSG_DRAW:
         rectfill(screen, d->x-d->w, d->y, d->x+d->w-1, d->y+d->h-1, d->bg);  // clear the element
         break;
         
      case MSG_READY:
         if (num_of_codes_reported == 0)
            mil_is_on = FALSE;
            
         if (mil_is_on)
            strcpy(d->dp, "MIL is ON");
         else
            strcpy(d->dp, "MIL is OFF");
      
         return D_REDRAWME;
   }
   
   return st_ctext_proc(msg, d, c);
}


int simulate_proc(int msg, DIALOG *d, int c)
{
   int ret;
   
   ret = d_check_proc(msg, d, c);
   
   if ((d->flags & D_SELECTED) && (d->d2 == 0))
   {
      d->d2 = 1;
      simulate = TRUE;
      trouble_codes_simulator(TRUE);
   }
   else if (!(d->flags & D_SELECTED) && (d->d2 == 1))
   {
      d->d2 = 0;
      simulate = FALSE;
      trouble_codes_simulator(FALSE);
   }
      
   return ret;
}   


int read_tr_codes_proc(int msg, DIALOG *d, int c)
{
   int ret;
   
   switch (msg)
   {
      case MSG_READ_CODES: case MSG_CLEAR_CODES:
         // if we are currently reading or clearing codes, and the button is enabled,
         if (!(d->flags & D_DISABLED))
         {
            d->flags |= D_DISABLED;     // disable the button
            return D_REDRAWME;
         }
         break;
         
      case MSG_READY:
         // if we're not reading or clearing codes, and the button is disabled,
         if (d->flags & D_DISABLED)
         {
            d->flags &= ~D_DISABLED;   // enable it
            return D_REDRAWME;
         }
         break;
   }

   ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE)
   {
      if (simulate)
         trouble_codes_simulator(TRUE);
      else
      // if button was clicked, send everyone MSG_READ_CODES:
      broadcast_dialog_message(MSG_READ_CODES, 0);
      
      return D_REDRAWME;
   }

   return ret;
}


int clear_codes_proc(int msg, DIALOG *d, int c)
{
   int ret;
   switch (msg)
   {
      case MSG_READ_CODES: case MSG_CLEAR_CODES:
         // if we are currently reading or clearing codes, and the button is enabled,
         if (!(d->flags & D_DISABLED))
         {
            d->flags |= D_DISABLED;     // disable the button
            return D_REDRAWME;
         }
         break;
         
      case MSG_READY:
         // if we're not reading or clearing codes, and the button is disabled,
         if (d->flags & D_DISABLED)
         {
            d->flags &= ~D_DISABLED;   // enable it
            return D_REDRAWME;
         }
         break;
   }
   
   ret = d_button_proc(msg, d, c);

   if (ret == D_CLOSE)
   {    
      // if user confirmed that he wants to erase the codes,
      if(popup_dialog(confirm_clear_dialog, 6) == 5)
      {
         if (simulate)
            trouble_codes_simulator(FALSE);
         else
            broadcast_dialog_message(MSG_CLEAR_CODES, 0);  //send everyone MSG_CLEAR_CODES
      }
      
      return D_REDRAWME;
   }

   return ret;
}


// heart of the trouble_code_reader module:
int tr_code_proc(int msg, DIALOG *d, int c)
{
   static char vehicle_response[512];        // character buffer for car response
   static int receiving_response = FALSE;    // flag, "are we receiving response?"
   static int verifying_connection = FALSE;  // flag, "are we verifying connection?"
   static int current_request;               // NUM_OF_CODES, READ_CODES, CLEAR_CODES
   static int temp_num_of_codes;             // temporary storage for num_of_codes
   int response_status = EMPTY;              // EMPTY, DATA, PROMPT
   int response_type;                        // BUS_BUSY, BUS_ERROR, DATA_ERROR, etc.
   char comport_buffer[80];                  // temporary storage for comport data

   switch (msg)
   {
      case MSG_IDLE:
         if (comport.status == READY)
         {
            if (verifying_connection && !receiving_response)
            {
               send_command("0100"); // send request that requires a response
               receiving_response = TRUE; // now we're waiting for response
               vehicle_response[0] = 0; //get buffer ready for the response
               start_serial_timer(OBD_REQUEST_TIMEOUT); // start the timer
            }
            else if (receiving_response)
            {
               response_status = read_comport(comport_buffer);
               
               if (simulate && (response_status == PROMPT))
                  serial_time_out = TRUE;
               else if(!simulate)
               {
                  if(response_status == DATA) // if data detected in com port buffer
                  {
                     // append contents of comport_buffer to vehicle_response:
                     strcat(vehicle_response, comport_buffer);
                     start_serial_timer(OBD_REQUEST_TIMEOUT);  // we got data, reset the timer
                  }
                  else if(response_status == PROMPT) // if ">" is detected
                  {
                     receiving_response = FALSE; // we're not waiting for response any more
                     stop_serial_timer();        // stop the timer
                     // append contents of comport_buffer to vehicle_response:
                     strcat(vehicle_response, comport_buffer);

                     if(verifying_connection)     // *** if we are verifying connection ***
                     {  // NOTE: we only get here if we got "NO DATA" somewhere else
                        response_type = process_response("0100", vehicle_response);

                        if(response_type == HEX_DATA) // if everything seems to be fine now,
                        {
                           if(current_request == CLEAR_CODES)
                              alert("There may have been a temporary loss of connection.", "Please try clearing codes again.", NULL, "OK", NULL, 0, 0);
                           else
                              alert("There may have been a temporary loss of connection.", "Please try reading codes again.", NULL, "OK", NULL, 0, 0);
                        }
                        else  // if we got an error message, respond accordingly
                        {
                           if(current_request == CLEAR_CODES) // if we were clearing codes,
                              alert("Communication problem: vehicle did not confirm successful", "deletion of trouble codes.  Please check connection to the vehicle,", "make sure the ignition is ON, and try clearing the codes again.", "OK", NULL, 0, 0);
                           else // if we were reading codes,
                              alert("There may have been a loss of connection.", "Please check connection to the vehicle,", "and make sure the ignition is ON", "OK", NULL, 0, 0);
                        }
                        verifying_connection = FALSE; // we're not verifying connection anymore
                        broadcast_dialog_message(MSG_READY, 0); // tell everyone we're done
                     }

                     else if(current_request == NUM_OF_CODES) // *** if we are getting number of codes ***
                     {
                        response_type = process_response("0101", vehicle_response);

                        if(response_type == ERR_NO_DATA)   // if we received "NO DATA"
                           verifying_connection = TRUE;  // verify connection
                        else if(response_type != HEX_DATA) // if we got an error,
                           handle_errors(response_type, NUM_OF_CODES);  // handle it
                        else    // if process response returned HEX_DATA (i.e. there's no errors)
                        {  // extract # of codes from vehicle_response and store in temp_num_of_codes
                           temp_num_of_codes = handle_num_of_codes(vehicle_response);
                        
                           if (temp_num_of_codes > 0) // if there's at least one code,
                           {
                              send_command("03");   // request the codes themselves
                              current_request = READ_CODES;  // we're reading codes now
                              receiving_response = TRUE;     // and receiving response
                              vehicle_response[0] = '\0';    // clear the buffer
                              start_serial_timer(OBD_REQUEST_TIMEOUT); // start the timer...
                           }
                           else
                           {
                              num_of_codes_reported = num_of_codes = 0;
                              broadcast_dialog_message(MSG_READY,0);
                           }
                        }
                     }
                     else if(current_request == READ_CODES) // if we are reading codes,
                     {
                        response_type = process_response("03", vehicle_response);

                        if (response_type == ERR_NO_DATA)// vehicle didn't respond, check connection
                        {
                           verifying_connection = TRUE;
                           num_of_codes = num_of_codes_reported = 0;
                        }
                        else if(response_type != HEX_DATA) // if we got an error,
                           handle_errors(response_type, NUM_OF_CODES);
                        else  // if there were *no* errors,
                        {
                           num_of_codes_reported = temp_num_of_codes;
                           // do some magic: convert chip response to actual code,
                           // find DTC description and solution (if they exist) and store the actual number of codes read in num_of_codes
                           num_of_codes = handle_read_codes(vehicle_response);
                           broadcast_dialog_message(MSG_READY, 0); // tell everyone we're done
                        }
                     }
                     else if(current_request == CLEAR_CODES)
                     {
                        response_type = process_response("04", vehicle_response);

                        if (response_type == ERR_NO_DATA)// vehicle didn't respond, check connection
                           verifying_connection = TRUE;
                        else if(response_type != HEX_DATA) // if we got an error,
                           handle_errors(response_type, CLEAR_CODES);
                        else // if everything's fine (received confirmation)
                        {
                           num_of_codes = num_of_codes_reported = 0;
                           broadcast_dialog_message(MSG_READY, 0);
                        }
                     }
                  }
               }
            }
         }
         
         if (serial_time_out)     // if request timed out,
         {
            receiving_response = FALSE;
            broadcast_dialog_message(MSG_READY, 0);
            if (!simulate)
            {
               if(alert("Device is not responding.", "Please check that it is connected", "and the port settings are correct", "OK",  "&Configure Port", 27, 'c') == 2)
                  display_options();   // let the user choose correct settings
            
               while (comport.status == NOT_OPEN)
               {
                  if (alert("Port is not ready.", "Please check that you specified the correct port", "and that no other application is using it", "&Configure Port", "&Ignore", 'c', 'i') == 1)
                     display_options(); // let the user choose correct settings
                  else
                     comport.status = USER_IGNORED;
               }
            }
            stop_serial_timer();
         }
         break;  // end case MSG_IDLE

      case MSG_START:
         receiving_response = FALSE;
         verifying_connection = FALSE;
         if (!simulate)
            broadcast_dialog_message(MSG_READ_CODES, 0);
         break;

      case MSG_READ_CODES:
         if (comport.status == READY)
         {
            send_command("0101"); // request number of trouble codes
            current_request = NUM_OF_CODES;
            receiving_response = TRUE; // now we're waiting for response
            vehicle_response[0] = 0;
            start_serial_timer(OBD_REQUEST_TIMEOUT); // start the timer
         }
         else
            serial_time_out = TRUE;
         break;

      case MSG_CLEAR_CODES:
         if (comport.status == READY)
         {
            send_command("04"); // "clear codes" request
            current_request = CLEAR_CODES;
            receiving_response = TRUE; // now we're waiting for response
            vehicle_response[0] = 0;
            start_serial_timer(OBD_REQUEST_TIMEOUT); // start the timer
         }
         else
            serial_time_out = TRUE;
         break;

      case MSG_END:
         stop_serial_timer();
         break;
   }

   return D_O_K;
}  // end of tr_codes_proc()


int code_list_proc(int msg, DIALOG *d, int c)
{
   static int curr_num_of_codes = 0;
   int ret;
   
   if (msg == MSG_READY)
   {
      if ((num_of_codes == 0) || (curr_num_of_codes != num_of_codes))
      {
         d->d1 = 0;
         d->d2 = 0;
         current_code_index = 0;
         curr_num_of_codes = num_of_codes;
      }
      return D_REDRAWME;
   }
     
   ret = d_list_proc(msg, d, c);
   
   if (current_code_index != d->d1)
   {
      current_code_index = d->d1;
      broadcast_dialog_message(MSG_READY, 0);
   }
   
   return ret;
}


char* code_list_getter(int index, int *list_size)
{
   if (index < 0)
   {
      if (num_of_codes > 0)
         *list_size = get_number_of_codes();
      else
         *list_size = 0;
      return NULL;
   }

   return get_trouble_code(index)->code;
}


int handle_num_of_codes(char *vehicle_response)
{
   int temp;
   char *response = vehicle_response;
   char buf[16];
   int ret = 0;

   while (*response)
   {
      if (find_valid_response(buf, response, "41", &response))
      {
         buf[6] = 0;
         temp = (int)strtol(buf + 4, 0, 16); // convert hex ascii string to integer
         if (temp & 0x80)
            mil_is_on = TRUE; // get MIL status from temp
         ret = ret + (temp & 0x7F);
      }
      else
         break;
   }

   return ret;
}


int handle_read_codes(const char *vehicle_response)
{
   char code_letter[] = "PCBU";
   int i = 0, j, k, min;
   int trouble_codes_count = 0;
   char character;
   char temp_buf[1024];
   TROUBLE_CODE *trouble_codes_list;
   TROUBLE_CODE temp_trouble_code;
   PACKFILE *code_def_file;
   
   if (get_number_of_codes() > 0)    // if the structure is not empty,
      clear_trouble_codes();
   
   while(vehicle_response[i])
   {
      if((i == 0) || (vehicle_response[i] == SPECIAL_DELIMITER)) // if we're just starting to read, or encountered delimiter
      {
         if ((vehicle_response[i] == SPECIAL_DELIMITER))
            i++;  // skip delimiter
            
         if (vehicle_response[i] == '4' && vehicle_response[i+1] == '3')  // if response does starts with "43"
            i += 2;  // skip "43"
         else
         {
            // skip the response to the next delimiter
            while(vehicle_response[i] && vehicle_response[i] != SPECIAL_DELIMITER)
               i++;
            continue;
         }

         for(j = 0; j < 3; j++)    // read 3 codes (1 line has 3 codes)
          {
            temp_trouble_code.code[0] = ' '; // make first position a blank space
            // begin to copy from vehicle_response to temp_trouble_code.code beginning with position #1
            for (k = 1; k < 5; k++)
            {
               temp_trouble_code.code[k] = vehicle_response[i];
               i++;
            }
            temp_trouble_code.code[k] = '\0';  // terminate string
            
            if (strcmp(temp_trouble_code.code, " 0000") == 0) // if there's no trouble code,
               break;      // break out of the for() loop
            
            // begin with position #1 (skip blank space), convert to hex, extract first two bits
            // use the result as an index into the code_letter array to get the corresponding code letter
            temp_trouble_code.code[0] = code_letter[strtol(temp_trouble_code.code + 1, 0, 16) >> 14];
            
            // get the 2nd digit of the trouble code ('1' in P1234)
            // by right-shifting 12 bits (3 nibbles) and ANDing with 0011b
            temp_trouble_code.code[1] = (strtol(temp_trouble_code.code + 1, 0, 16) >> 12) & 0x03;
            temp_trouble_code.code[1] += 0x30; // convert to ASCII
            temp_trouble_code.description = NULL; // clear the corresponding description...
            temp_trouble_code.solution = NULL;  // ..and solution
            add_trouble_code(&temp_trouble_code);
            trouble_codes_count++;
         }
         continue;  // encountered SPECIAL_DELIMITER, do not increment i
      }
      i++;   // increment i until we find next SPECIAL_DELIMITER
   }  // end of while(), finished extracting codes
   
   for (i = 0; i < trouble_codes_count; i++)    // sort codes in ascending order
   {
      min = i;
      
      for (j = i+1; j < trouble_codes_count; j++)
         if(strcmp(get_trouble_code(j)->code, get_trouble_code(min)->code) < 0)
            min = j;
      
      swap_strings(get_trouble_code(i)->code, get_trouble_code(min)->code);
   }
   
   for (trouble_codes_list = trouble_codes->next; trouble_codes_list; trouble_codes_list = trouble_codes_list->next)   // search for descriptions and solutions
   {
      // pass the letter (B, C, P, or U) to file_handle, which returns the file handle
      // if we reached EOF, or the file does not exist, go to the next DTC
      if ((code_def_file = file_handle(trouble_codes_list->code[0])) == NULL)
         continue;
      
      while (TRUE)
      {
         j = 0;
      
         // copy DTC from file to temp_buf
         while (((character = pack_getc(code_def_file)) != FIELD_DELIMITER) && (character != RECORD_DELIMITER) && (character != EOF))
         {
            temp_buf[j] = character;
            j++;
         }
         temp_buf[j] = '\0';
         
         if (character == EOF) // reached end of file, break out of while()
            break;             // advance to next code

         if (strcmp(trouble_codes_list->code, temp_buf) == 0) // if we found the code,
         {
            if (character == RECORD_DELIMITER)  // reached end of record, no description or solution,
               break;                        // break out of while(), advance to next code

            j = 0;

            //copy description from file to temp_buf
            while (((character = pack_getc(code_def_file)) != FIELD_DELIMITER) && (character != RECORD_DELIMITER) && (character != EOF))
            {
               temp_buf[j] = character;
               j++;
            }
            temp_buf[j] = '\0';  // terminate string
            if (j > 0)
            {
               if (!(trouble_codes_list->description = (char *)malloc(sizeof(char)*(j+1))))
               {
                  sprintf(temp_error_buf, "Could not allocate enough memory for trouble code description [%i]", trouble_codes_count);
                  fatal_error(temp_error_buf);
               }
               strcpy(trouble_codes_list->description, temp_buf);  // copy description from temp_buf
            }
         
            if (character == FIELD_DELIMITER)   // if we have solution,
            {
               j = 0;

               // copy solution from file to temp_buf
               while (((character = pack_getc(code_def_file)) != RECORD_DELIMITER) && (character != EOF))
               {
                  temp_buf[j] = character;
                  j++;
               }
               temp_buf[j] = '\0';   // terminate string
               if (j > 0)
               {
                  if (!(trouble_codes_list->solution = (char *)malloc(sizeof(char)*(j+1))))
                  {
                     sprintf(temp_error_buf, "Could not allocate enough memory for trouble code solution [%i]", trouble_codes_count);
                     fatal_error(temp_error_buf);
                  }
                  strcpy(trouble_codes_list->solution, temp_buf);  // copy solution from temp_buf
               }
            }
            break;  // break out of while(TRUE)
         }
         else
         {
            // skip to next record
            while (((character = pack_getc(code_def_file)) != RECORD_DELIMITER) && (character != EOF));
            
            if (character == EOF)
               break;   // break out of while(TRUE), advance to next code
         }  
      } // end of while(TRUE)
   } // end of for() loop
   file_handle(0); // close the code definition file if it's still open
   
   return trouble_codes_count; // return the actual number of codes read
}


void handle_errors(int error, int operation)
{
   static int retry_attempts = NUM_OF_RETRIES;

   if(error == BUS_ERROR) // if we received "BUS ERROR"
   {
      alert("Bus Error: OBDII bus is shorted to Vbatt or Ground.", NULL, NULL, "OK", NULL, 0, 0);
      retry_attempts = NUM_OF_RETRIES;
      broadcast_dialog_message(MSG_READY, 0); // tell everyone that we're done
   }
   
   else    // if we received "BUS BUSY", "DATA ERROR", "<DATA ERROR", SERIAL_ERROR, or RUBBISH,
   {       // try to re-send the request, do nothing if successful and alert user if failed:
      if(retry_attempts > 0) //
      {
         retry_attempts--;
         switch (operation)
         {
            case READ_CODES: case NUM_OF_CODES:  // if we are currently reading codes,
               broadcast_dialog_message(MSG_READ_CODES, 0); // try reading again
               break;
            case CLEAR_CODES:   // if we are currently clearing codes,
               broadcast_dialog_message(MSG_CLEAR_CODES, 0);  // try clearing again
               break;
         }
      }
      else
      {
         switch (error)
         {
            case BUS_BUSY:
               alert("Bus Busy: try reading the codes again", NULL, NULL, "OK", NULL, 0, 0);
               break;
            case DATA_ERROR: case DATA_ERROR2:
               alert("Data Error: there has been a loss of data.", "You may have a bad connection to the vehicle,", "check the cable and try again.", "OK", NULL, 0, 0);
               break;
            case SERIAL_ERROR: case RUBBISH:
               alert("Serial Link Error: please check connection", "between computer and OBD interface.", NULL, "OK", NULL, 0, 0);
               break;
         }
         retry_attempts = NUM_OF_RETRIES; // reset the number of retry attempts
         broadcast_dialog_message(MSG_READY, 0); // tell everyone that we're done
      }
   }     // end of BUS_BUSY, DATA_ERROR, DATA_ERROR2, SERIAL_ERROR, and RUBBISH
}


void swap_strings(char *str1, char *str2)
{
   char temp[256];

   strcpy(temp, str1);
   strcpy(str1, str2);
   strcpy(str2, temp);
}


TROUBLE_CODE *add_trouble_code(const TROUBLE_CODE * init_code)
{
   TROUBLE_CODE *trouble_code = trouble_codes;

   while(trouble_code->next)
      trouble_code = trouble_code->next;

   if (!(trouble_code->next = (TROUBLE_CODE *)malloc(sizeof(TROUBLE_CODE))))
   {
      sprintf(temp_error_buf, "Could not allocate enough memory for new trouble code [%i]", num_of_codes);
      fatal_error(temp_error_buf);
   }
   
   if (init_code)
   {
      strcpy(trouble_code->next->code, init_code->code);
      trouble_code->next->description = init_code->description;
      trouble_code->next->solution = init_code->solution;
   }
   else
   {
      trouble_code->next->code[0] = '\0';
      trouble_code->next->description = NULL;
      trouble_code->next->solution = NULL;
   }
   trouble_code->next->next = NULL;

   return trouble_code->next;
}


TROUBLE_CODE *get_trouble_code(int index)
{
   int i;
   TROUBLE_CODE *trouble_code = trouble_codes;

   for(i = 0; i <= index; i++)
   {
      if (!trouble_code->next)
         return NULL;
      trouble_code = trouble_code->next;
   }

   return trouble_code;
}


int get_number_of_codes()
{
   int ret = 0;
   TROUBLE_CODE *trouble_code = trouble_codes;
   
   while((trouble_code = trouble_code->next))
      ret++;
   
   return ret;
}


void clear_trouble_codes()
{
   TROUBLE_CODE *trouble_code = trouble_codes;
   TROUBLE_CODE *next;
   
   if (trouble_codes->next)
   {
      next = trouble_codes->next;
      do
      {
         trouble_code = next;
         next = trouble_code->next;
         if (trouble_code->description)
            free(trouble_code->description);
         if (trouble_code->solution)
            free(trouble_code->solution);
         free(trouble_code);
      } while(next);
      
      trouble_codes->next = NULL;
   }
}


PACKFILE *file_handle(char code_letter)
{
   static PACKFILE *file = NULL;
   static char current_code_letter = 0;
   char file_name[30];
   
   if (code_letter == 0)
   {
      current_code_letter = 0;
      if (file != NULL)
         pack_fclose(file);
      file = NULL;
   }
   else if (code_letter != current_code_letter)
   {
      if (file != NULL)
      {
         pack_fclose(file);
         file = NULL;
      }
   
      sprintf(file_name, "%s#%ccodes", code_defs_file_name, tolower(code_letter));
      packfile_password(PASSWORD);
      file = pack_fopen(file_name, F_READ_PACKED);
      packfile_password(NULL);
      current_code_letter = code_letter;
   }
   
   if (file == NULL)     
      return NULL;
      
   if (pack_feof(file) == TRUE) 
      return NULL;
      
   return file;
}
