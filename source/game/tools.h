#ifndef __TOOLS__
#define __TOOLS__

class Tools
{
  private:
   static int initialized;
  public:
   enum {max_radartools=8,max_smoketools=8};
   enum Type
   {
    _FIRST,
     NET_FIRST,
     NET_SECOND,
     NET_THIRD,
     NET_FOURTH,
     BLACKWHITE,
     SHADOW,
     LSHADOW,
     VLSHADOW,
     LIGHT,
     LIGHT1,
     LIGHT2,
     LIGHT3,
     LIGHT4,
     LIGHT5,
     LIGHT6,
     LIGHT7,
     LIGHT8,
     LIGHT9,
     LIGHT10,
     LIGHT11,
     LIGHT12,
     LIGHT13,
     LIGHT14,
     LIGHT15,
     LIGHT16,
     DISPINFO,
     ALIENSMOKE,
     BACKSPRITE,
     HELP,
    _LAST
   };
   static int colors[Tools::_LAST];
   static Tool tool[Tools::_LAST];
   static Tool radar_tool[max_radartools];
   static Tool smokeblack_tool[max_smoketools];
   static Tool smokewhite_tool[max_smoketools];
   static Text text;
   static void init_tool(Tool& tool,char *descript);
   static void init_color(int& color,char *descript);
   static void init_light_tool(void);
   static void init_smoke_tool(void);
   static void init_radar_tool(void);
   static void init(char *file);
   static void quit(void);
};

#endif



