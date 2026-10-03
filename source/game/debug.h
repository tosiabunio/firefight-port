#ifndef __DEBUGINFO__
#define __DEBUGINFO__

//KLASA DEBUGINFO
class Debuginfo
{
  private:
   static int status;
   static int options;
  public:
   static void run(void);
   static void draw(Screen& screen);
};
#endif

