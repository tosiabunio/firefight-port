#ifndef __PRINT__
#define __PRINT__

class Print
{
  private:
#if HI_DEBUG
   static int initialized;
#endif
  public:
   enum Filters 
   {
     FNORMAL,
     FHIGHLIGHTED,
     FDISABLED,
     FSTATIC,
     FWARNING,
     FJAXON,
     FRILEY,
     FSTACY,
     FSHADOW,
     FINVUND,
     FINVOVR,
     FMENULEVELPANEL,
     FMENULEVELMISSIONS,
     FMENULEVELTEXT,
     FPLAYER1,
     FPLAYER2,
     FPLAYER3,
     FPLAYER4,
    _FLAST
   };
   enum Type 
   {
     TSPRITE,
     TFONT,
    _TLAST
   };
//  private:
  public:
   static Sprite sml;
   static Sprite big;
   static Sprite sml_italic;
   static Sprite big_italic;
   static unsigned char conv[256];
   static char case_term;
   static char italic_term;
   static Tool tools[_FLAST+1]; 
   static int colors[_FLAST+1];
   static int space_x;
   static int space_y;
   static int sh_offset_x;
   static int sh_offset_y;
   static int get_dx(Type type,unsigned char character,int use_big,int use_italic);
   static int get_dy(Type type,int use_big,int use_italic);
  public:
   static void init(char *small_name,char *small_italic_name,char *big_name,char *big_italic_name,char *descr_text);
   static void quit(void);
   static void print(Screen& screen,Type type,Filters filter,int x,int y,char *text,...);
   static int get_dx(Type type,char *text,...);
   static int get_dy(Type type,char *text,...);
   static Tool& get_tool(Filters filter);
   static int get_color(Filters filter);
};

#endif
