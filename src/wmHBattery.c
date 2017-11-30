/*
 *
 *  	wmHBattery-1.0
 * 
 *
 * 	This program is free software; you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	the Free Software Foundation; either version 2, or (at your option)
 * 	any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	You should have received a copy of the GNU General Public License
 * 	along with this program (see the file COPYING); if not, write to the
 * 	Free Software Foundation, Inc., 59 Temple Place - Suite 330, 
 *      Boston, MA  02111-1307, USA
 *
 *
 *      Changes:
 *
 *	Version 1.00  - Released. (2017-06-27)
 *
 *	Version 1.01  - Fixed an issue preventing Cairo drawing due to non-teminated
 *                      colour strings. (2017-07-27)
 *
 */


/*  
 *   Includes  
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <X11/X.h>
#include <X11/xpm.h>
#include <cairo.h>
#include <cairo-xlib.h>
#include <dirent.h>
#include "xutils_cairo.h"
#include "wmHBattery_master.xpm"
#include "wmHBattery_mask.xbm"


// Delay between refreshes (in microseconds) 
#define DELAY 10000L
#define WMHBATTERY_VERSION "1.01"

// Color struct, h = hex (000000), rgb components in range 0-1.
struct patCol {
  char  h[7];
  float r,g,b;
};
typedef struct patCol PatCol;

void ParseCMDLine(int argc, char *argv[]);
void ButtonPressEvent(XButtonEvent *);
void print_usage();
void h2dCol(PatCol *col);

int     winH = 64, winW = 64;
int     GotFirstClick1, GotDoubleClick1;
int     GotFirstClick2, GotDoubleClick2;
int     GotFirstClick3, GotDoubleClick3;
int     DblClkDelay;
int     HasExecute = 0;		/* controls perf optimization */
char	ExecuteCommand[1024];


char    TimeColor[30]    	= "#ffff00";
char    BackgroundColor[30]    	= "#181818";
char	batState[15], batStatus[40], batCapacity[40];
int	batLevel = 0;
int     bs2 = 10, bs3 = 25, bs4 = 50, bs5 = 75, pt = 0, bfA = 90, cfA = 90, pfA = 90;

PatCol  bg, bfs, bfe, cfs, cfe, pfs, pfe, cfs1, cfe1, cfs2, cfe2, cfs3, cfe3, cfs4, cfe4, cfs5, cfe5;


// Function to convert hex color to rgb values
void h2dCol(PatCol *col) {
  char hCol[3]; 
  strncpy(hCol, col->h, 2);
  hCol[2] = '\0';
  col->r = strtol(hCol, NULL, 16)/255.0;
  strncpy(hCol, col->h+2, 2);
  hCol[2] = '\0';
  col->g = strtol(hCol, NULL, 16)/255.0;
  strncpy(hCol, col->h+4, 2);
  hCol[2] = '\0';
  col->b = strtol(hCol, NULL, 16)/255.0;
}


// Find the battery files location
void findBatt() {
  char  testPath[40] = "/sys/class/power_supply/BAT", batPath[40], c[3] = "0";
  int i = 0; 
  DIR *dir = NULL; 

  // Find a the battery
  while (!dir) {
    sprintf(c,"%d",i);
    strcpy(batPath, testPath);
    strcat(batPath, c);
    dir = opendir(batPath);
    i++;

    // Exit if no batterys found
    if (i > 10) {
      fprintf(stderr, "ERROR: Can't find a battery at /sys/class/power_supply/BAT[0-9].\n");
      exit(1);
    }
  }

  strcpy(batCapacity, batPath);
  strcat(batCapacity, "/capacity");
  strcpy(batStatus, batPath);
  strcat(batStatus, "/status");
  closedir(dir);
}


// Function to get battery charge level and charging status
void getBatInfo() {
  FILE 	*fp;
  char	*ptr;
  int tbatLevel;

  // Get battery Level...
  fp = fopen(batCapacity, "r");
  fgets(batState , 4, fp);
  strtok(batState, "\n");
  tbatLevel = strtol(batState, &ptr, 10);
  if (tbatLevel >= 0 && tbatLevel <= 100)
    batLevel = tbatLevel;
  fclose(fp);

  // ... and charging/discharging state
  strcpy(batState, "");
  fp = fopen(batStatus, "r");
  fgets(batState, 15, fp);
  strtok(batState, "\n");
  fclose(fp);
}


// Draw the battery
void drawBatt(cairo_t *ctx) {
  cairo_pattern_t *bPat, *cPat, *pPat;
  int batH = 41, batW = 26, capH = 4, capW = 16, rad = 5, batB = 4, clrH = 0;
  int bsY = (winH - batH)/2+capH-1, bsX = (winW - batW)/2, csX = (winW - capW)/2;
  int plugH = 20, plugW = 10, plugY = (winH - plugH)/2 + capH - 2, plugX = (winW - plugW)/2;
  int batCX = bsX + batW/2, batCY = bsY + batH/2, grX, grY, fH = batH - (batB*2);
  double degrees = M_PI / 180.0;

  // Re-enable antialiasing
  cairo_set_antialias(ctx, CAIRO_ANTIALIAS_BEST);

  // Clear Window
  cairo_set_source_rgb(ctx, bg.r, bg.g, bg.b);
  cairo_rectangle(ctx, 5, 5, 54, 54);
  cairo_fill(ctx);

  // Create a gradient pattern for the battery outline
  grX = cos(bfA * degrees) * (batH/2);
  grY = sin(bfA * degrees) * (batH/2);
  bPat = cairo_pattern_create_linear(batCX - grX, batCY - grY, batCX + grX, batCY + grY);
  cairo_pattern_add_color_stop_rgb(bPat, 0, bfs.r, bfs.g, bfs.b);
  cairo_pattern_add_color_stop_rgb(bPat, 1, bfe.r, bfe.g, bfe.b);
  cairo_set_source(ctx, bPat);
 
  // Draw battery outline
  cairo_new_sub_path (ctx);
  cairo_arc (ctx, bsX + batW - rad, bsY + rad, rad, -90 * degrees, 0 * degrees);
  cairo_arc (ctx, bsX + batW - rad, bsY + batH - rad, rad, 0 * degrees, 90 * degrees);
  cairo_arc (ctx, bsX + rad, bsY + batH - rad, rad, 90 * degrees, 180 * degrees);
  cairo_arc (ctx, bsX + rad, bsY + rad, rad, 180 * degrees, 270 * degrees);
  cairo_close_path (ctx);
  cairo_fill_preserve (ctx);

  // Top cap
  cairo_rectangle(ctx, csX, bsY - capH, capW, capH);
  cairo_fill(ctx);

  // Check which colour to use
  if (batLevel >= bs5) {
    cfs = cfs5;
    cfe = cfe5;
  } else if (batLevel >= bs4) {
    cfs = cfs4;
    cfe = cfe4;
  } else if (batLevel >= bs3) {
    cfs = cfs3;
    cfe = cfe3;
  } else if (batLevel >= bs2) {
    cfs = cfs2;
    cfe = cfe2;
  } else {
    cfs = cfs1;
    cfe = cfe1;
  }

  // Create a gradient pattern for the battery color
  grX = cos(cfA * degrees) * (fH/2);
  grY = sin(cfA * degrees) * (fH/2);
  cPat = cairo_pattern_create_linear(batCX - grX, batCY - grY, batCX + grX, batCY + grY);
  cairo_pattern_add_color_stop_rgb(cPat, 0, cfs.r, cfs.g, cfs.b);
  cairo_pattern_add_color_stop_rgb(cPat, 1, cfe.r, cfe.g, cfe.b);
  cairo_set_source(ctx, cPat);

  // Draw the battery color
  cairo_rectangle(ctx, bsX + batB, bsY + batB, batW - (batB*2), fH);
  cairo_fill(ctx);

  // Clear a percent of the fill to show battery charge
  clrH = (1-batLevel/100.0)*fH;
  cairo_set_source_rgb(ctx, bg.r, bg.g, bg.b);
  cairo_rectangle(ctx, bsX + batB, bsY + batB, batW - (batB*2), clrH);
  cairo_fill(ctx);

  // Create a gradient pattern for the plug
  grX = cos(pfA * degrees) * (batH/2);
  grY = sin(pfA * degrees) * (batH/2);
  pPat = cairo_pattern_create_linear(batCX - grX, batCY - grY, batCX + grX, batCY + grY);
  cairo_pattern_add_color_stop_rgb(pPat, 0, pfs.r, pfs.g, pfs.b);
  cairo_pattern_add_color_stop_rgb(pPat, 1, pfe.r, pfe.g, pfe.b);
  cairo_set_source(ctx, pPat);

  // Draw charging icon
  if (strcmp(batState, "Full") == 0 || strcmp(batState, "Charging") == 0) {
    // Plug icon
    if (pt == 0) {
      cairo_rectangle(ctx, plugX + 2, plugY, 2, 8);
      cairo_rectangle(ctx, plugX + 6, plugY, 2, 8);
      cairo_rectangle(ctx, plugX, plugY + 4, plugW, 2);
      cairo_rectangle(ctx, plugX + 4, plugY + 4, 2, 16);
      cairo_arc(ctx, plugX + 5, plugY + 9, 3, 0, 2 * M_PI);
      cairo_fill(ctx);

    // Lightning icon
    } else  {
      cairo_move_to(ctx, plugX + plugW/2 + 1, plugY);
      cairo_line_to(ctx, plugX, plugY + plugH/2 + 1);
      cairo_line_to(ctx, plugX + plugW/2 - 1, plugY + plugH/2 + 1);
      cairo_line_to(ctx, plugX + plugW/2 - 1, plugY + plugH);
      cairo_line_to(ctx, plugX + plugW, plugY + plugH/2 - 1);
      cairo_line_to(ctx, plugX + plugW/2 + 1, plugY + plugH/2 - 1);
      cairo_line_to(ctx, plugX + plugW/2 + 1, plugY);
      cairo_fill(ctx);

    }
  }
  cairo_pattern_destroy(bPat);
  cairo_pattern_destroy(cPat);
  cairo_pattern_destroy(pPat);
}


// main  
int main(int argc, char *argv[]) {
  XEvent	event;
  int		n = 1000;
  PatCol        *bgptr = &bg, *bfsptr = &bfs, *bfeptr = &bfe;
  PatCol	*cfsptr = &cfs, *cfeptr = &cfe, *pfsptr = &pfs, *pfeptr = &pfe;
  PatCol        *cfs1ptr = &cfs1, *cfs2ptr = &cfs2, *cfs3ptr = &cfs3, *cfs4ptr = &cfs4, *cfs5ptr = &cfs5;
  PatCol        *cfe1ptr = &cfe1, *cfe2ptr = &cfe2, *cfe3ptr = &cfe3, *cfe4ptr = &cfe4, *cfe5ptr = &cfe5;

  cairo_surface_t       *sfc;
  cairo_t               *ctx;

  // Find the battery
  findBatt();

  // Default Colors
  strcpy(bg.h, "d2dae4");
  strcpy(bfs.h, "939494");
  strcpy(bfe.h, "2f2f2f");
  strcpy(cfs.h, "72ce46");
  strcpy(cfe.h, "218c22");
  strcpy(pfs.h, "939494");
  strcpy(pfe.h, "2f2f2f");
  strcpy(cfs1.h, "e99494");
  strcpy(cfs2.h, "ebc984");
  strcpy(cfs3.h, "dcc200");
  strcpy(cfs4.h, "bcd788");
  strcpy(cfs5.h, "72ce46");
  strcpy(cfe1.h, "d63c3c");
  strcpy(cfe2.h, "d59a21");
  strcpy(cfe3.h, "cfb505");
  strcpy(cfe4.h, "7ea336");
  strcpy(cfe5.h, "218c22");

  // Parse any command line arguments.
  ParseCMDLine(argc, argv);

  // Set colors rgb elements
  h2dCol(bgptr);
  h2dCol(bfsptr);
  h2dCol(bfeptr);
  h2dCol(cfsptr);
  h2dCol(cfeptr);
  h2dCol(pfsptr);
  h2dCol(pfeptr);
  h2dCol(cfs1ptr);
  h2dCol(cfs2ptr);
  h2dCol(cfs3ptr);
  h2dCol(cfs4ptr);
  h2dCol(cfs5ptr);
  h2dCol(cfe1ptr);
  h2dCol(cfe2ptr);
  h2dCol(cfe3ptr);
  h2dCol(cfe4ptr);
  h2dCol(cfe5ptr);

  // Init X window & Icon
  initXwindow(argc, argv);
  openXwindow(argc, argv, wmHBattery_master, wmHBattery_mask_bits, wmHBattery_mask_width, wmHBattery_mask_height);

  // Create a Cairo surface and context in the icon window 
  sfc = cairo_create_x11_surface0(winW, winH);
  ctx = cairo_create(sfc);


  // Loop until we die
  while(1) {
    // Only recalculate every Nth loop
    if (n > 10) {
      n = 0;
      getBatInfo();
      drawBatt(ctx);

    }

    // Increment counter
    ++n;
 

    // Keep track of click events. If Delay too long, set GotFirstClick's to False
    if (DblClkDelay > 1500) {
      DblClkDelay = 0;
      GotFirstClick1 = 0; GotDoubleClick1 = 0;
      GotFirstClick2 = 0; GotDoubleClick2 = 0;
      GotFirstClick3 = 0; GotDoubleClick3 = 0;

    } else {
      ++DblClkDelay;
    }

    // Process any pending X events.
    while(XPending(display)){
      XNextEvent(display, &event);
      switch(event.type){
        case ButtonPress:
          ButtonPressEvent(&event.xbutton);
          break;
        case ButtonRelease:
          break;
      }
    }

    // Redraw and wait for next update 
  
    RedrawWindow();
    usleep(500000L);

  }
 cairo_destroy(ctx);
 cairo_close_x11_surface(sfc);
}


// Function to check a valid #000000 color is provided
void valid_color(char argv[10], char ccol[7]) {
  if (strcmp(ccol, "missing") == 0 ||ccol[0] == '-' ) {
    fprintf(stderr, "ERROR: No color found following %s flag.\n", argv);
    print_usage();
    exit(-1);
  }

  if (strlen(ccol) != 6 || ccol[strspn(ccol, "0123456789abcdefABCDEF")] != 0) {
    fprintf(stderr, "ERROR: Invalid color following %s flag, should be valid hex \"000000\" format.\n", argv);
    print_usage();
    exit(-1);
  }
}


// Parse command line arguments
void ParseCMDLine(int argc, char *argv[]) {
  int  i;
  char ccol[1024] = "missing";
 
  for (i = 1; i < argc; i++) {
    if (argc > i+1 && strlen(argv[i+1])<=7)
      strcpy(ccol, argv[i+1]);

    if (!strcmp(argv[i], "-e")){
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No command found following -e flag.\n");
        print_usage();
        exit(-1);
      }
      strcpy(ExecuteCommand, argv[++i]);
      HasExecute = 1;

    } else if (!strcmp(argv[i], "-p")) {
      pt = 0;

    } else if (!strcmp(argv[i], "-l")) {
      pt = 1;

    } else if (!strcmp(argv[i], "-s2")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No battery level step percent provided after -s2 flag.\n");
        print_usage();
        exit(-1);
      }
      bs2 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-s3")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No battery level step percent provided after -s3 flag.\n");
        print_usage();
        exit(-1);
      }
      bs3 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-s4")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No battery level step percent provided after -s4 flag.\n");
        print_usage();
        exit(-1);
      }
      bs4 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-s5")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No battery level step percent provided after -s5 flag.\n");
        print_usage();
        exit(-1);
      }
      bs5 = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-bg")) {
      valid_color(argv[i], ccol);
      strcpy(bg.h, argv[++i]);

    } else if (!strcmp(argv[i], "-bs")) {
      valid_color(argv[i], ccol);
      strcpy(bfs.h, argv[++i]);

    } else if (!strcmp(argv[i], "-be")) {
      valid_color(argv[i], ccol);
      strcpy(bfe.h, argv[++i]);

    } else if (!strcmp(argv[i], "-ps")) {
      valid_color(argv[i], ccol);
      strcpy(pfs.h, argv[++i]);

    } else if (!strcmp(argv[i], "-pe")) {
      valid_color(argv[i], ccol);
      strcpy(pfe.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fs1")) {
      valid_color(argv[i], ccol);
      strcpy(cfs1.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fs2")) {
      valid_color(argv[i], ccol);
      strcpy(cfs2.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fs3")) {
      valid_color(argv[i], ccol);
      strcpy(cfs3.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fs4")) {
      valid_color(argv[i], ccol);
      strcpy(cfs4.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fs5")) {
      valid_color(argv[i], ccol);
      strcpy(cfs5.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fe1")) {
      valid_color(argv[i], ccol);
      strcpy(cfs5.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fe2")) {
      valid_color(argv[i], ccol);
      strcpy(cfe2.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fe3")) {
      valid_color(argv[i], ccol);
      strcpy(cfe3.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fe4")) {
      valid_color(argv[i], ccol);
      strcpy(cfe4.h, argv[++i]);

    } else if (!strcmp(argv[i], "-fe5")) {
      valid_color(argv[i], ccol);
      strcpy(cfe5.h, argv[++i]);

    } else if (!strcmp(argv[i], "-ba")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No degrees provided after -ba flag.\n");
        print_usage();
        exit(-1);
      }
      bfA = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-fa")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No degrees provided after -ca flag.\n");
        print_usage();
        exit(-1);
      }
      cfA = strtol(argv[++i], NULL, 10);

    } else if (!strcmp(argv[i], "-pa")) {
      if ((i+1 >= argc)||(argv[i+1][0] == '-')) {
        fprintf(stderr, "ERROR: No degrees provided after -pa flag.\n");
        print_usage();
        exit(-1);
      }
      pfA = strtol(argv[++i], NULL, 10);

    } else {
      print_usage();
      exit(1);
    }
  }
}


// Print usage instructions
void print_usage(){
  printf("\nwmHBattery version: %s\n\n", WMHBATTERY_VERSION);
  printf("usage: wmHBattery [-e <Command>] [-p] [-l]\n");
  printf("  [-s2 <Percent>] [-s3 <Percent>] [-s4 <Percent>] [-s5 <Percent>]\n");
  printf("  [-bg <Colour>] [-bs <Colour>] [-be <Colour>] [-ps <Colour>] [-pe <Colour>]\n");
  printf("  [-fs1 <Colour>] [-fs2 <Colour>] [-fs3 <Colour>] [-fs4 <Colour>] [-fs5 <Colour>]\n");
  printf("  [-fe1 <Colour>] [-fe2 <Colour>] [-fe3 <Colour>] [-fe4 <Colour>] [-fe5 <Colour>]\n");
  printf("  [-ba <Degrees>] [-fa <Degrees>] [-pa <Degrees>] [-h]\n\n");
  printf("   -e <Command>    Command to execute via double click of mouse button 1\n");
  printf("   -p              Show a plug symbol when charging (Default)\n");
  printf("   -l              Show a lightning symbol when charging\n");
  printf("   -s2 <Percent>   Percent at which to change battery fill color (Default: 10)\n");
  printf("   -s3 <Percent>   Percent at which to change battery fill color (Default: 25)\n");
  printf("   -s4 <Percent>   Percent at which to change battery fill color (Default: 50)\n");
  printf("   -s5 <Percent>   Percent at which to change battery fill color (Default: 75)\n");
  printf("   -bg <Colour>    Background color (Default: d2dae4)\n");
  printf("   -bs <Colour>    Battery border gradient start color (Defailt: 939494)\n");
  printf("   -be <Colour>    Battery border gradient end color (Defailt: 2f2f2f)\n");
  printf("   -ps <Colour>    Charging symbol gradient start color (Default: 939494)\n");
  printf("   -pe <Colour>    Charging symbol gradient end color (Default: 2f2f2f)\n");
  printf("   -fs1 <Colour>   Fill gradient start color when charge level > 0%% (Defult: e99494)\n");
  printf("   -fs2 <Colour>   Fill gradient start color when charge level > s2 (Defult: ebc984)\n");
  printf("   -fs3 <Colour>   Fill gradient start color when charge level > s3 (Defult: dcc200)\n");
  printf("   -fs4 <Colour>   Fill gradient start color when charge level > s4 (Defult: bcd788)\n");
  printf("   -fs5 <Colour>   Fill gradient start color when charge level > s5 (Defult: 72ce46)\n");
  printf("   -fe1 <Colour>   Fill gradient end color when charge level > 0%% (Defult: d63c3c)\n");
  printf("   -fe2 <Colour>   Fill gradient end color when charge level > s2 (Defult: d59a21)\n");
  printf("   -fe3 <Colour>   Fill gradient end color when charge level > s3 (Defult: cfb505)\n");
  printf("   -fe4 <Colour>   Fill gradient end color when charge level > s4 (Defult: 7ea336)\n");
  printf("   -fe5 <Colour>   Fill gradient end color when charge level > s5 (Defult: 218c22)\n");
  printf("   -ba <Degrees>   Angle of the battery border gradient (Default: 90)\n");
  printf("   -fa <Degrees>   Angle of the battery fill gradient (Default: 90)\n");
  printf("   -pa <Degrees>   Angle of the charging symbol gradient (Default: 90)\n");
  printf("   -h              Show this usage message\n");
  printf("\nExample: wmHBattery -bg \"0000FF\" -l -fa 45  \n\n");

}




 // This routine handles button presses.
void ButtonPressEvent(XButtonEvent *xev){
  char Command[512];

  if( HasExecute == 0) return; /* no command specified.  Ignore clicks. */
  DblClkDelay = 0;
  if ((xev->button == Button1) && (xev->type == ButtonPress)){
    if (GotFirstClick1) GotDoubleClick1 = 1;
    else GotFirstClick1 = 1;
  } else if ((xev->button == Button2) && (xev->type == ButtonPress)){
    if (GotFirstClick2) GotDoubleClick2 = 1;
    else GotFirstClick2 = 1;
  } else if ((xev->button == Button3) && (xev->type == ButtonPress)){
    if (GotFirstClick3) GotDoubleClick3 = 1;
    else GotFirstClick3 = 1;
  }

  // We got a double click on Mouse Button1 (i.e. the left one)
  if (GotDoubleClick1) {
    GotFirstClick1 = 0;
    GotDoubleClick1 = 0;
    sprintf(Command, "%s &", ExecuteCommand);
    system(Command);
  }

  // We got a double click on Mouse Button2 (i.e. the left one)
  if (GotDoubleClick2) {
    GotFirstClick2 = 0;
    GotDoubleClick2 = 0;
  }

  // We got a double click on Mouse Button3 (i.e. the left one)
  if (GotDoubleClick3) {
    GotFirstClick3 = 0;
    GotDoubleClick3 = 0;
  }

  return;
}

