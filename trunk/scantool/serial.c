#include <string.h>
#include <ctype.h>
#include "globals.h"
#include <allegro/internal/aintern.h>
#ifdef ALLEGRO_WINDOWS
   #include <winalleg.h>
#else
   #include <dzcomm.h>
#endif
#include "serial.h"

#ifdef ALLEGRO_WINDOWS
   static HANDLE com_port;
#else
   static comm_port *com_port;
#endif


//timer interrupt handler for sensor data
static void serial_time_out_handler()
{
   serial_time_out = TRUE;
   serial_timer_running = FALSE;
}
END_OF_STATIC_FUNCTION(serial_time_out_handler)


void start_serial_timer(int delay)
{
    install_int_ex(serial_time_out_handler, MSEC_TO_TIMER(delay));  // install the timer
    serial_time_out = FALSE;
    serial_timer_running = TRUE;
}


void stop_serial_timer()
{
   remove_int(serial_time_out_handler);
   serial_time_out = FALSE;
   serial_timer_running = FALSE;
}


void serial_module_init()
{
   #ifndef ALLEGRO_WINDOWS
      dzcomm_init();
   #endif
   serial_timer_running = FALSE;
   /* all variables and code used inside interrupt handlers must be locked */
   LOCK_VARIABLE(serial_time_out);
   LOCK_FUNCTION(serial_time_out_handler);
   _add_exit_func(serial_module_shutdown);
}


void serial_module_shutdown()
{
   close_comport();
   
   #ifndef ALLEGRO_WINDOWS
      // dzcomm_closedown();
   #endif
   
   _remove_exit_func(serial_module_shutdown);
}


int open_comport()
{
   #ifdef ALLEGRO_WINDOWS
      DCB dcb;
      COMMTIMEOUTS timeouts;
      char temp_str[16];
   #endif
   
   if (comport.status == READY)    // if the comport is open,
      close_comport();    // close it

   #ifdef ALLEGRO_WINDOWS
      sprintf(temp_str, "COM%i", comport.number + 1);
      com_port = CreateFile(temp_str, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
      if (com_port == INVALID_HANDLE_VALUE)
      {
         comport.status = NOT_OPEN; //port was not open
         return -1; // return error
      }

      GetCommState(com_port, &dcb);
      dcb.BaudRate = 9600;
      dcb.ByteSize = 8;
      dcb.StopBits = ONESTOPBIT;
      dcb.fParity = FALSE;
      dcb.Parity = NOPARITY;
      dcb.fOutxCtsFlow = FALSE;
      dcb.fOutxDsrFlow = FALSE;
      dcb.fOutX = FALSE;
      dcb.fInX = FALSE;
      dcb.fDtrControl = DTR_CONTROL_ENABLE;
      dcb.fRtsControl = RTS_CONTROL_ENABLE;
      dcb.fDsrSensitivity = FALSE;
      dcb.fErrorChar = FALSE;
      dcb.fAbortOnError = FALSE;
      SetCommState(com_port, &dcb);
      
      timeouts.ReadIntervalTimeout = MAXWORD;
      timeouts.ReadTotalTimeoutMultiplier = 0;
      timeouts.ReadTotalTimeoutConstant = 0;
      timeouts.WriteTotalTimeoutMultiplier = 0;
      timeouts.WriteTotalTimeoutConstant = 0;
      SetCommTimeouts(com_port, &timeouts);
   #else
      com_port = comm_port_init(comport.number);
      comm_port_set_baud_rate(com_port, _9600);
      comm_port_set_parity(com_port, NO_PARITY);
      comm_port_set_data_bits(com_port, BITS_8);
      comm_port_set_stop_bits(com_port, STOP_1);
      comm_port_set_flow_control(com_port, NO_CONTROL);
      if (comm_port_install_handler(com_port) != 1)
      {
         comport.status = NOT_OPEN; //port was not open
         return -1; // return error
      }
   #endif
   serial_time_out = FALSE;
   comport.status = READY;
   
   return 0; // everything is okay
}


void close_comport()
{
   if (comport.status == READY)    // if the comport is open, close it
   {
      #ifdef ALLEGRO_WINDOWS
         PurgeComm(com_port, PURGE_TXCLEAR|PURGE_RXCLEAR);
         CloseHandle(com_port);
      #else
         comm_port_flush_output(com_port);
         comm_port_flush_input(com_port);
         comm_port_uninstall(com_port);
      #endif
   }
   comport.status = NOT_OPEN;
}


void send_command(char *command)
{
   #ifdef ALLEGRO_WINDOWS
      DWORD bytes_written;
      
      PurgeComm(com_port, PURGE_TXCLEAR|PURGE_RXCLEAR);
      WriteFile(com_port, command, strlen(command), &bytes_written, 0);
      WriteFile(com_port, "\r", 1, &bytes_written, 0);
   #else
      comm_port_flush_output(com_port);
      comm_port_flush_input(com_port);
      comm_port_command_send(com_port, command);
   #endif
}


int read_comport(char *response)
{
   char *prompt_pos = NULL;

   #ifdef ALLEGRO_WINDOWS
      DWORD bytes_read = 0;
      DWORD errors;
      COMSTAT stat;
      
      response[0] = '\0';
      ClearCommError(com_port, &errors, &stat);
      if (stat.cbInQue > 0)
         ReadFile(com_port, response, stat.cbInQue, &bytes_read, 0);
      response[bytes_read] = '\0';
   #else
      int i = 0;
      
      while((response[i] = comm_port_test(com_port)) != -1) // while the serial buffer is not empty, read comport
         i++;
      response[i] = '\0'; // terminate string, erase -1
   #endif
   
   prompt_pos = strchr(response, '>');
   if (prompt_pos != NULL)
   {
      *prompt_pos = '\0'; // erase ">"
      return PROMPT;      // command prompt detected
   }
   else if (strlen(response) == 0)  // if the string is empty,
      return EMPTY;
   else                         //otherwise,
      return DATA;
}


int process_response(const char *cmd_sent, char *msg_received)
{
   int i = 0;
   char *msg = msg_received;
   int echo_on = TRUE; //echo status
   int is_hex_num = TRUE;
   char temp_buf[80];

   if (cmd_sent)
   {
      for(i = 0; cmd_sent[i]; i++)
      {
         if (cmd_sent[i] != *msg)    // if the characters are not the same,
         {
            echo_on = FALSE;  // say that echo is off
            break;            // break out of the loop
         }
         msg++;
      }

      if (echo_on == TRUE)  //if echo is on
      {
         send_command("ate0"); // turn off the echo
         start_serial_timer(AT_TIMEOUT);
         // wait for chip response or timeout
         while ((read_comport(temp_buf) != PROMPT) && !serial_time_out);
         stop_serial_timer();
      }
      else //if echo is off
         msg = msg_received;
   }

   while(*msg && (*msg <= ' '))
      msg++;

   for(i = 0; *msg; msg++) //loop to copy data
   {
      if (*msg > ' ')  // if the character is not a special character or space
      {
         msg_received[i] = *msg; // rewrite response
         if (!isxdigit(*msg))
            is_hex_num = FALSE;
         i++;
      }
      else if (((*msg == '\n') || (*msg == '\r')) && (msg_received[i-1] != SPECIAL_DELIMITER)) // if the character is a CR or LF
         msg_received[i++] = SPECIAL_DELIMITER; // replace CR with SPECIAL_DELIMITER
   }
   
   if (i > 0)
      if (msg_received[i-1] == SPECIAL_DELIMITER)
         i--;
   msg_received[i] = '\0'; // terminate the string

   if (is_hex_num)
      return HEX_DATA;

// DO NOT TRANSLATE BELOW THIS LINE!
   if (strcmp(msg_received, "NODATA") == 0)
      return ERR_NO_DATA;
   if (strcmp(msg_received, "BUSBUSY") == 0)
      return BUS_BUSY;
   if (strcmp(msg_received, "DATAERROR") == 0)
      return DATA_ERROR;
   if (strstr(msg_received, "<DATAERROR") != NULL)
      return DATA_ERROR2;
   if (strcmp(msg_received, "BUSERROR") == 0)
      return BUS_ERROR;
   if (strcmp(msg_received, "BUSINIT:...ERROR") == 0)
      return ERR_NO_DATA;
   if (strcmp(msg_received, "?") == 0)
      return SERIAL_ERROR;

   return RUBBISH;
}

