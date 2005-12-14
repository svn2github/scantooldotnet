#include <string.h>
#include "globals.h"
#include "custom_gui.h"
#include "version.h"
#include "about.h"

static int logo_proc(int msg, DIALOG *d, int c);
static int large_text_proc(int msg, DIALOG *d, int c);
static int about_this_computer_proc(int msg, DIALOG *d, int c);
static int thanks_proc(int msg, DIALOG *d, int c);

static char whatisit[256];
static char whatcanitdo[256];
static char wheretoget[256];

#define VER_STR   "Version " SCANTOOL_VERSION_EX_STR " for " SCANTOOL_PLATFORM_STR ", Copyright © " SCANTOOL_YEAR_STR " ScanTool.net"

static DIALOG about_dialog[] =
{
   /* (proc)                   (x)  (y)  (w)  (h)  (fg)     (bg)          (key) (flags) (d1) (d2) (dp)                    (dp2) (dp3) */
   { d_clear_proc,             0,   0,   0,   0,   0,       C_WHITE,       0,    0,      0,   0,   NULL,                   NULL, NULL },
   { logo_proc,                105, 25,  430, 58,  0,       0,             0,    0,      0,   0,   NULL,                   NULL, NULL },
   { st_ctext_proc,            320, 86,  216, 18,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   VER_STR,                NULL, NULL },
   { large_text_proc,          25,  112, 256, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "What is it?",          NULL, NULL },
   { super_textbox_proc,       25,  136, 407, 80,  C_BLACK, C_WHITE,       0,    0,      0,   0,   whatisit,               NULL, NULL },
   { large_text_proc,          25,  224, 256, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "What can it do?",      NULL, NULL },
   { super_textbox_proc,       25,  248, 407, 98,  C_BLACK, C_WHITE,       0,    0,      0,   0,   whatcanitdo,            NULL, NULL },
   { large_text_proc,          25,  354, 256, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "Where can I get it?",  NULL, NULL },
   { super_textbox_proc,       25,  378, 590, 80,  C_BLACK, C_WHITE,       0,    0,      0,   0,   wheretoget,             NULL, NULL },
   { d_box_proc,               440, 136, 175, 175, C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,                   NULL, NULL },
   { about_this_computer_proc, 448, 144, 160, 48,  C_BLACK, C_GREEN,       'c',  D_EXIT, 0,   0,   "About this &computer", NULL, NULL },
   { thanks_proc,              448, 200, 160, 48,  C_BLACK, C_DARK_YELLOW, 'r',  D_EXIT, 0,   0,   "C&redits",             NULL, NULL },
   { d_button_proc,            448, 256, 160, 48,  C_BLACK, C_PURPLE,      'm',  D_EXIT, 0,   0,   "Main &Menu",           NULL, NULL },
   { NULL,                     0,   0,   0,   0,   0,       0,             0,    0,      0,   0,   NULL,                   NULL, NULL }
};

static DIALOG thanks_dialog[] =
{
   /* (proc)            (x)  (y)  (w)  (h)  (fg)   (bg)            (key) (flags) (d1) (d2) (dp)      (dp2) (dp3) */
   { d_shadow_box_proc, 0,   0,   608, 266, C_BLACK, C_LIGHT_GRAY,  0,    0,      0,   0,   NULL,     NULL, NULL },
   { d_shadow_box_proc, 0,   0,   608, 24,  C_BLACK, C_DARK_GRAY,   0,    0,      0,   0,   NULL,     NULL, NULL },
   { caption_proc,      304, 2,   301, 19,  C_WHITE, C_TRANSP,      0,    0,      0,   0,   "We would like to thank the following people and organizations:", NULL, NULL },
   { d_text_proc,       24,  36,  560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- DJ Delorie (www.delorie.com) for the DJGPP compiler ", NULL, NULL },
   { d_text_proc,       24,  60,  560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- Bloodshed Software (www.bloodshed.net) for the Dev-C++ IDE", NULL, NULL },
   { d_text_proc,       24,  84,  560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- Shawn Hargreaves for creating the Allegro library", NULL, NULL },
   { d_text_proc,       24,  108, 560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- Dim Zegebart and Neil Townsend for the DZComm serial library", NULL, NULL },
   { d_text_proc,       24,  132, 560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- Julien Cugniere for his Allegro dialog editor", NULL, NULL },
   { d_text_proc,       24,  156, 560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- Eric Botcazou and Allegro mailing list folks for their tips and suggestions", NULL, NULL },
   { d_text_proc,       24,  180, 560, 24,  C_BLACK, C_TRANSP,      0,    0,      0,   0,   "- All users who provided feedback and bug reports", NULL, NULL },
   { d_button_proc,     248, 211, 112, 40,  C_BLACK, C_DARK_YELLOW, 'c',  D_EXIT, 0,   0,   "&Close", NULL, NULL },
   { NULL,              0,   0,   0,   0,   0,       0,             0,    0,      0,   0,   NULL,     NULL, NULL }
};


int display_about()
{
   strcpy(whatisit, "ScanTool.net is multi-platform, open-source software designed to work with the ElmScan family of OBD-II interfaces, our inexpensive alternatives to professional diagnostic scan tools.");
   sprintf(whatcanitdo, "ScanTool.net v%s allows you to read and clear trouble codes, and display real-time sensor data.  Software with more advanced features, such as data logging,  virtual dashboard, and support for additional test modes is available from third-party vendors.", SCANTOOL_VERSION_STR);
   strcpy(wheretoget, "You can download the latest version of this software, and buy your scan tool from www.ScanTool.net. There, you can also find contact information for your local distributor, and links to third-party software.");

   return do_dialog(about_dialog, -1); // do the dialog
}


int logo_proc(int msg, DIALOG *d, int c)
{
   switch (msg)
   {
      case MSG_START:
         d->dp = create_bitmap(430, 58);
         pivot_sprite(d->dp, datafile[LOGO_BMP].dat, 430, 0, 0, 0, itofix(64));
         break;
         
      case MSG_END:
         destroy_bitmap(d->dp);
         break;
   }
   
   return d_bitmap_proc(msg, d, c);
}


int thanks_proc(int msg, DIALOG *d, int c)
{
   int ret = d_button_proc(msg, d, c);

   if (msg == MSG_START)
      centre_dialog(thanks_dialog);
   
   if (ret == D_CLOSE)
   {
      popup_dialog(thanks_dialog, -1);
      return D_REDRAWME;
   }

   return ret;
}


int large_text_proc(int msg, DIALOG *d, int c)
{
   if (msg == MSG_START)
      d->dp2 = datafile[ARIAL18_FONT].dat; // load the font

   return d_text_proc(msg, d, c);
}


int about_this_computer_proc(int msg, DIALOG *d, int c)
{
   char cpu_info_buf[96];
   char os_type_buf[96];
   char os_name[32];
   char processor_vendor[64];
   char processor_family[64];
   char processor_model[64];

   // clear strings
   cpu_info_buf[0] = 0;
   os_type_buf[0] = 0;
   os_name[0] = 0;
   processor_vendor[0] = 0;
   processor_family[0] = 0;
   processor_model[0] = 0;
   
   int ret = d_button_proc(msg, d, c);
   
   if (ret == D_CLOSE)
   {
      // reset the variables
      processor_model[0] = 0;

      // determine processor vendor
      if(strcmp("GenuineIntel", cpu_vendor) == 0)
         strcpy(processor_vendor, "an Intel");
      else if(strcmp("AuthenticAMD", cpu_vendor) == 0)
         strcpy(processor_vendor, "an AMD");
      else if(strcmp("CyrixInstead", cpu_vendor) == 0)
         strcpy(processor_vendor, "a Cyrix");
      else if(strcmp("CentaurHauls", cpu_vendor) == 0)
         strcpy(processor_vendor, "a Centaur");
      else if(strcmp("NexGenDriven", cpu_vendor) == 0)
         strcpy(processor_vendor, "a NexGen");
      else if(strcmp("GenuineTMx86", cpu_vendor) == 0)
         strcpy(processor_vendor, "a Transmeta");
      else if(strcmp("RISERISERISE", cpu_vendor) == 0)
         strcpy(processor_vendor, "a Rise");
      else if(strcmp("UMC UMC UMC", cpu_vendor) == 0)
         strcpy(processor_vendor, "an UMC");
      else
         strcpy(processor_vendor, "an unknown CPU vendor");


      if(!strcmp("GenuineIntel", cpu_vendor))
      {
         if (cpu_family == 4)
         {
            switch(cpu_model)
            {
               case 0:
                  strcpy(processor_model, " 486 DX-25/33");
                  break;
               case 1:
                  strcpy(processor_model, " 486 DX-50");
                  break;
               case 2:
                  strcpy(processor_model, " 486 SX");
                  break;
               case 3:
                  strcpy(processor_model, " 486 DX/2");
                  break;
               case 4:
                  strcpy(processor_model, " 486 SL");
                  break;
               case 5:
                  strcpy(processor_model, " 486 SX/2");
                  break;
               case 7:
                  strcpy(processor_model, " 486 DX/2-WB");
                  break;
               case 8:
                  strcpy(processor_model, " 486 DX/4");
                  break;
               case 9:
                  strcpy(processor_model, " 486 DX/4-WB");
            }
         }

         if (cpu_family == 5)
         {
            switch(cpu_model)
            {
               case 0:
                  strcpy(processor_model, " Pentium 60/66 A-step");
                  break;
               case 1:
                  strcpy(processor_model, " Pentium 60/66");
                  break;
               case 2:
                  strcpy(processor_model, " Pentium 75 - 200");
                  break;
               case 3:
                  strcpy(processor_model, " OverDrive PODP5V83");
                  break;
               case 4:
                  strcpy(processor_model, " Pentium MMX");
                  break;
               case 7:
                  strcpy(processor_model, " Mobile Pentium 75 - 200");
                  break;
               case 8:
                  strcpy(processor_model, " Mobile Pentium MMX");
                  break;
            }
         }

         if (cpu_family == 6)
         {
            switch(cpu_model)
            {
               case 0:
                  strcpy(processor_model, " Pentium Pro A-step");
                  break;
               case 1:
                  strcpy(processor_model, " Pentium Pro");
                  break;
               case 3:
                  strcpy(processor_model, " Pentium II (Klamath)");
                  break;
               case 5:
                  strcpy(processor_model, " Pentium II, Celeron, or Mobile Pentium II");
                  break;
               case 6:
                  strcpy(processor_model, " Mobile Pentium II or Celeron (Mendocino)");
                  break;
               case 7:
                  strcpy(processor_model, " Pentium III (Katmai)");
                  break;
               case 8:
                  strcpy(processor_model, " Pentium III (Coppermine)");
                  break;
            }
         }

         if (cpu_family >= 15)
            strcpy(processor_model, " Pentium IV or better");
      } // end of Intel block

      if(!strcmp("AuthenticAMD", cpu_vendor))
      {
         switch(cpu_family)
         {
            case 4:
               switch (cpu_model)
               {
                  case 3:
                     strcpy(processor_model, " 486 DX/2");
                     break;
                  case 7:
                     strcpy(processor_model, " 486 DX/2-WB");
                     break;
                  case 8:
                     strcpy(processor_model, " 486 DX/4");
                     break;
                  case 9:
                     strcpy(processor_model, " 486 DX/4-WB");
                     break;
                  case 14:
                     strcpy(processor_model, " Am5x86-WT");
                     break;
                  case 15:
                     strcpy(processor_model, " Am5x86-WB");
                     break;
               }
               break;

            case 5:
               switch (cpu_model)
               {
                  case 0:
                     strcpy(processor_model, " K5/SSA5");
                     break;
                  case 1: case 2: case 3:
                     strcpy(processor_model, " K5");
                     break;
                  case 6: case 7:
                     strcpy(processor_model, " K6");
                     break;
                  case 8:
                     strcpy(processor_model, " K6-2");
                     break;
                  case 9:
                     strcpy(processor_model, " K6-3");
                     break;
                  case 13:
                     strcpy(processor_model, " K6-2+ or K6-III+");
                     break;
               }
               break;
            
            case 6:
               switch (cpu_model)
               {
                  case 0: case 1:
                     strcpy(processor_model, " Athlon (25 um)");
                     break;
                  case 2:
                     strcpy(processor_model, " Athlon (18 um)");
                     break;
                  case 3:
                     strcpy(processor_model, " Duron");
                     break;
                  case 4:
                     strcpy(processor_model, " Athlon (Thunderbird)");
                     break;
                  case 6:
                     strcpy(processor_model, " Athlon (Palamino)");
                     break;
                  case 7:
                     strcpy(processor_model, " Duron (Morgan)");
                     break;
               }
               break;
         }
      } // end of AMD block

      if(strcmp("CyrixInstead", cpu_vendor) == 0)
      {
         switch(cpu_family)
         {
            case 4:
               strcpy(processor_model, " MediaGX");
               break;
               
            case 5:
               switch(cpu_model)
               {
                  case 2:
                     strcpy(processor_model, " 6x86/6x86L");
                     break;
                  case 4:
                     strcpy(processor_model, " MediaGX MMX Enhanced");
                     break;
               }
               break;
               
            case 6:
               switch(cpu_model)
               {
                  case 0:
                     strcpy(processor_model, " m II");
                     break;
                  case 5:
                     strcpy(processor_model, " VIA Cyrix M2 core");
                     break;
                  case 6:
                     strcpy(processor_model, " WinChip C5A");
                     break;
                  case 7:
                     strcpy(processor_model, " WinChip C5B");
                     break;
               }
               break;
         }
      } // end of Cyrix block
      
      
      if(!strcmp("UMC UMC UMC", cpu_vendor))
      {
         if (cpu_family == 4)
         {
            if (cpu_model == 1)
               strcpy(processor_model, " U5D");
            if (cpu_model == 2)
               strcpy(processor_model, " U5S");
         }
      }  // end of UMC block
      

      if(strcmp("GenuineTMx86", cpu_vendor) == 0)
         if ((cpu_family == 5) && (cpu_model == 0))
            strcpy(processor_model, " Nx586"); // end of NexGen block
            
      if(strcmp("RISERISERISE", cpu_vendor) == 0)
         if ((cpu_family == 5) && ((cpu_model == 0) || (cpu_model == 1)))
            strcpy(processor_model, " mP6"); // end of Rise block
            

      switch (os_type)
      {
         case OSTYPE_UNKNOWN:
            strcpy(os_name, "MSDOS");
            break;

         case OSTYPE_WIN3:
            strcpy(os_name, "Windows 3.x");
            break;

         case OSTYPE_WIN95:
            strcpy(os_name, "Windows 95");
            break;
            
         case OSTYPE_WIN98:
            strcpy(os_name, "Windows 98");
            break;

         case OSTYPE_WINME:
            strcpy(os_name, "Windows ME");
            break;

         case OSTYPE_WINNT:
            strcpy(os_name, "Windows NT");
            break;

         case OSTYPE_WIN2000:
            strcpy(os_name, "Windows 2000");
            break;

         case OSTYPE_WINXP:
            strcpy(os_name, "Windows XP");
            break;

         case OSTYPE_OS2:
            strcpy(os_name, "OS/2");
            break;

         case OSTYPE_WARP:
            strcpy(os_name, "OS/2 Warp 3");
            break;

         case OSTYPE_DOSEMU:
            strcpy(os_name, "Linux DOSEMU");
            break;

         case OSTYPE_OPENDOS:
            strcpy(os_name, "Caldera OpenDOS");
            break;

         case OSTYPE_LINUX:
            strcpy(os_name, "Linux");
            break;

         case OSTYPE_FREEBSD:
            strcpy(os_name, "Free BSD"); // do not translate!
            break;

         case OSTYPE_UNIX:
            strcpy(os_name, "Unix");
            break;

         case OSTYPE_BEOS:
            strcpy(os_name, "BeOS");
            break;

         case OSTYPE_QNX:
            strcpy(os_name, "QNX");
            break;

         case OSTYPE_MACOS:
            strcpy(os_name, "Mac OS");
            break;

         default:
            strcpy(os_name, "Unknown OS");
      }


      sprintf(cpu_info_buf, "You have %s%s%s based computer (%i:%i),", processor_vendor, processor_family, processor_model, cpu_family, cpu_model);
      sprintf(os_type_buf, "running %s version %i.%i", os_name, os_version, os_revision);
      alert(cpu_info_buf, os_type_buf, NULL, "OK", NULL, 0, 0);

      return D_REDRAWME;
   }

   return ret;
}
