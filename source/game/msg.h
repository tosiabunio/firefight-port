#ifndef __MESSAGE__
#define __MESSAGE__

const int Max_mmessage=24;
const int MAX_NOTEXT=24;
const int MAX_SYSTEXT=6;

struct Message
{
  int time,x,y,kind;
  char string[256];
};


class MManager
{
  private:
   static Message table[Max_mmessage];
   static Message systable[MAX_SYSTEXT];
   static int syscount;
   struct Mission_msg
   {
     char string[50];
     int  sound,kind;
     char full;
   };
   static Mission_msg  notext[MAX_NOTEXT];
   static int size,serv_message,last_smp,fls;
   static int X,Y;
   static int rand;
   static int first,objective;
   static Sample *smp;
   static int  smpflag;
   static void add_string(char* txt,int time,int _x=MManager::X,int _y=MManager::Y,int type=1,int kind=-1);
   static void display_string(Screen& screen,int x,int y,char *text,int col);
  public:
   static void init(int _y);
   static void run(void);
   static void display(Screen& screen);
   static void add_notext(Field* fld,int full);
   static void add_objective(char* txt);
   static void add_sysstring(char* txt,int id,int col=0);
   static void serv_mission_msg(void);
   static void flush(void);
   static void flush_notxt(void);
   static int  is_speaking(void);
};

#define MSGTXT MManager::add_sysstring

const int MAX_SMSAMPLE=30;

class SManager
{
  private:
    static int count;
    static int head;
    static int end;
    static int last_playing;
    static Sample*  stable[MAX_SMSAMPLE];
    static int blocked;
  public:
    static void init(void);
    static void run(void);
    static int  playing(void);
    static void flush(void);
    static void lock(void) {blocked=1;}
    static void unlock(void) {blocked=0;}
    static void add_sample(Sample* s1,Sample* s2=NULL,Sample* s3=NULL,Sample* s4=NULL,Sample* s5=NULL);
};

#endif


