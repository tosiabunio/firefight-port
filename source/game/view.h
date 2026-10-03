#ifndef __VIEW__
#define __VIEW__

//KLASA VISIBLE
#define NIL -1

class Visible;
class View
{
   enum {planes_numb=10,max_objects=800};
  public:
   enum Vprty
   {
     BEFORESHD,
     MYFIRST,
     LIGHT,
     BUILD,
     ASTAT,
     ACSHT,
     MYCAP,
     AWEAP,
     AMOV,
     MYSHIP,
     MYWEAP,
     NEXPL,
     MYLAST,
     INFO,
     AFTERSHD
   };
  private:
   static int initialized;
   static int max_used;
   struct Cell
   {
     Visible *ptr;
     int next;
   };
   static Cell space[max_objects];
   static int begin[planes_numb][View::AFTERSHD+1];
   static int first_free;
  public:
   static void init(void);
   static void quit(void);
   static void run(Screen& screen,int plane);
   static void add(Visible* ptr,int plane,View::Vprty prty);
   static void flush(void);
#if HI_DEBUG
   static void statistics(void);
#endif
};
//KLASA DISPLAY
class Display
{
  public:
   static Screen& screen(void);
   static void copy2vga(void);
   static void clear_vga(int palette);
   static void release_screen(void);
};

#endif
