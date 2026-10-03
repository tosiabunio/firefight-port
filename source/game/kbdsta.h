#ifndef __KBDSTAT__
#define __KBDSTAT__

#define GETPACK(ID,WHAT)\
{\
  check_init();\
  switch (ID)\
  {\
   case -1: {FOR_ALL_USERS(_ID_) {if ((pack[_ID_].WHAT)!=0) return _ID_;}}\
            return 0;\
    case 0: return (pack[NoNet::get_this_id()].WHAT);\
   default: check_user(ID);\
            return (pack[ID].WHAT);\
  }\
}\

class KbdStat
{
  public:
   enum {chat_buf_size=10,max_demo_size=2000000};
   enum DemoMode
   {
     PLAY_DEMO=0x01,
     SAVE_DEMO=0x02,
   };
  private:
   static int initialized;
   static int demo_started;
   struct Pack
   {
     //USER FLAGS
     unsigned char  USER_FLAGS         :8;
     //SHIP STEERING
     unsigned char  STEERING_MODE      :2;
     unsigned char  TURBO_LOCKED       :1;
     unsigned char  USING_MENU         :1;
     unsigned char  USING_NETMSG       :1;
     unsigned char  SLOWING_DOWN       :1;
     unsigned char  LEFT               :1;
     unsigned char  RIGHT              :1;
     unsigned char  FORWARD            :1;
     unsigned char  BACKWARD           :1;
     unsigned char  STRAFE_LEFT        :1;
     unsigned char  STRAFE_RIGHT       :1;
     unsigned char  FIRE               :1;
     unsigned char  TURBO              :1;
     unsigned char  AUTO_RUN           :1;
     unsigned char  START              :1;
     unsigned char  ABORT_BEAMING      :1;
     unsigned char  WEAP1              :1;
     unsigned char  WEAP2              :1;
     unsigned char  WEAP3              :1;
     unsigned char  WEAP4              :1;
     unsigned char  WEAP5              :1;
     unsigned char  WEAP6              :1;
     unsigned char  NEXT_WEAP          :1;
     unsigned char  PREV_WEAP          :1;
     unsigned char  MOUSE_READ         :1;
     unsigned char  MOUSE_GOTO         :1;
     int            MOUSE_X            :16;
     int            MOUSE_Y            :16;
     unsigned char  USE_ITEM           :1;
     unsigned char  NEXT_ITEM          :1;
     unsigned char  PREV_ITEM          :1;
     //ADDITIONAL KEYS
     unsigned char  PAUSE              :1;
     unsigned char  QUIT               :1;
     unsigned char  ABORT              :1;
     unsigned char  HELP               :1;
     unsigned char  RESTART            :1;
     unsigned char  HIRES              :1;
     unsigned char  GAMMA_PLUS         :1;
     unsigned char  GAMMA_MINUS        :1;
     unsigned char  CONTROLS           :1;
     unsigned char  DETAIL_LEVEL       :1;
     unsigned char  CAPTSCREEN         :1;
     unsigned char  CAPTSCREEN_NOINFO  :1;
     unsigned char  COLISWORLD         :1;
     unsigned char  DISPLAY_FPS        :1;
     unsigned char  NEXT_VIEW          :1;
     unsigned char  MAKE_FAILURE       :1;
     unsigned char  POINTS             :1;
     unsigned char  CHEATING           :1;
     unsigned char  NO_WALL            :1;
     unsigned char  NET_CHEATING       :1;
     unsigned char  HEADER_ANYKEY      :1;
     unsigned char  LAST_OBJECTIVE     :1;
     unsigned char  TEST_OBJECTIVE     :1;
     //MENU KEYS
     unsigned char  MENU_ON            :1;
     unsigned char  MENU_SECRET_ON     :1;
     unsigned char  MENU_UP            :1;
     unsigned char  MENU_DOWN          :1;
     unsigned char  MENU_PAGE_UP       :1;
     unsigned char  MENU_PAGE_DOWN     :1;
     unsigned char  MENU_SET_OPTION    :1;
     unsigned char  MENU_LEFT          :1;
     unsigned char  MENU_RIGHT         :1;
     unsigned char  MENU_LEFT_STRUCK   :1;
     unsigned char  MENU_RIGHT_STRUCK  :1;
     unsigned char  MENU_QUIT          :1;
     unsigned char  MENU_ACCEPT        :1;
     unsigned char  MENU_ANYKEY        :1;
     //NET HEADER KEYS
     unsigned char  NET_HEAD_ACCEPT    :1;
     unsigned char  NET_HEAD_REJECT    :1;
     unsigned char  NET_HEAD_NEXTLEV   :1;
     unsigned char  NET_HEAD_PREVLEV   :1;
     //CHAT & NETMSG DATA
     char           CHAT_BUF[chat_buf_size];
     char           CHAT_HOLDED_KEY;
     unsigned char  NETMSG_REQUEST     :1;
     unsigned char  NETMSG_ABORT       :1;
     unsigned char  NETMSG_BACKSPACE   :1;
     unsigned char  CHAT_REQUEST       :1;
     unsigned char  CHAT_ACCEPT        :1;
     unsigned char  CHAT_BACKSPACE     :1;
     unsigned char  CHAT_DELETE        :1;
     unsigned char  CHAT_ABORT         :1;
     unsigned char  CHAT_PREVWORD      :1;
     unsigned char  CHAT_NEXTWORD      :1;
     unsigned char  CHAT_LEFT          :1;
     unsigned char  CHAT_RIGHT         :1;
     unsigned char  CHAT_BEG           :1;
     unsigned char  CHAT_END           :1;
     unsigned char  CHAT_PREV_COMMAND  :1;
   };
   struct Data
   {
     int frame;
     Pack pack[NoNet::max_users+1];
   };
   static NoQueue<Data> *queue;
   static Pack  pack[NoNet::max_users+1];
   static int   abort_demo;
   static int   demo_mode;
   static int   time_ticks;
   static int   cnt;
   static int   display;
   static int   frame_numb;
   static int   local_frame_numb;
   static int   ignore_my;
   static char *letters;
   static void  check_init(void) {DBG_CHECK(initialized);}
   static void  check_user(int id) {DBG_CHECK(NoNet::check_id(id));}
   static void  fill_keys(int id);
   static void  fill_keys(void);
   static unsigned char make_user_flags(void);
   static void  demo_update_keys(int id);
   static void  demo_check(void);
  public:
   static void  debug_init(void);
   static void  debug_done(void);
   static void  init(void);
   static void  done(void);
   static int   demo_init(DemoMode mode,char *filename,char *data,unsigned *size);
   static int   is_demo(void);
   static void  clear_demo_mode(void);
   static void  reinitframes(void)         {check_init();frame_numb=0;}
   static void  dump(void);
   static void  set_ignore(void)           {check_init();ignore_my=1;}
   static void  disable_ignore(void)       {check_init();ignore_my=0;}
   static int   run(unsigned sync=0);
   static int   time(void)                 {check_init();return time_ticks;}
   static int   frame(void)                {return frame_numb;}
   static int   local_frame(void)          {return local_frame_numb;}
   static int   counter(void)              {check_init();return cnt;}
   //SHIP
   static int steering_mode(int id=0)      {GETPACK(id,STEERING_MODE);}
   static int turbo_locked(int id=0)       {GETPACK(id,TURBO_LOCKED);}
   static int using_menu(int id=0)         {GETPACK(id,USING_MENU);}
   static int using_netmsg(int id=0)       {GETPACK(id,USING_NETMSG);}
   static int slowing_down(int id=0)       {GETPACK(id,SLOWING_DOWN);}
   static int left(int id=0)               {GETPACK(id,LEFT);}
   static int right(int id=0)              {GETPACK(id,RIGHT);}
   static int forward(int id=0)            {GETPACK(id,FORWARD);}
   static int backward(int id=0)           {GETPACK(id,BACKWARD);}
   static int strafe_left(int id=0)        {GETPACK(id,STRAFE_LEFT);}
   static int strafe_right(int id=0)       {GETPACK(id,STRAFE_RIGHT);}
   static int fire(int id=0)               {GETPACK(id,FIRE);}
   static int turbo(int id=0)              {GETPACK(id,TURBO);}
   static int auto_run(int id=0)           {GETPACK(id,AUTO_RUN);}
   static int start(int id=0)              {GETPACK(id,START);}
   static int abort_beaming(int id=0)      {GETPACK(id,ABORT_BEAMING);}
   static int weap1(int id=0)              {GETPACK(id,WEAP1);}
   static int weap2(int id=0)              {GETPACK(id,WEAP2);}
   static int weap3(int id=0)              {GETPACK(id,WEAP3);}
   static int weap4(int id=0)              {GETPACK(id,WEAP4);}
   static int weap5(int id=0)              {GETPACK(id,WEAP5);}
   static int weap6(int id=0)              {GETPACK(id,WEAP6);}
   static int next_weap(int id=0)          {GETPACK(id,NEXT_WEAP);}
   static int prev_weap(int id=0)          {GETPACK(id,PREV_WEAP);}
   static int mouse_read(int id=0)         {GETPACK(id,MOUSE_READ);}
   static int mouse_x(int id=0)            {GETPACK(id,MOUSE_X);}
   static int mouse_y(int id=0)            {GETPACK(id,MOUSE_Y);}
   static int mouse_goto(int id=0)         {GETPACK(id,MOUSE_GOTO);}
   static int use_item(int id=0)           {GETPACK(id,USE_ITEM);}
   static int next_item(int id=0)          {GETPACK(id,NEXT_ITEM);}
   static int prev_item(int id=0)          {GETPACK(id,PREV_ITEM);}
   //MENU
   static int menu_on(int id=0)            {GETPACK(id,MENU_ON);}
   static int menu_secret_on(int id=0)     {GETPACK(id,MENU_SECRET_ON);}
   static int menu_up(int id=0)            {GETPACK(id,MENU_UP);}
   static int menu_down(int id=0)          {GETPACK(id,MENU_DOWN);}
   static int menu_left(int id=0)          {GETPACK(id,MENU_LEFT);}
   static int menu_right(int id=0)         {GETPACK(id,MENU_RIGHT);}
   static int menu_left_struck(int id=0)   {GETPACK(id,MENU_LEFT_STRUCK);}
   static int menu_right_struck(int id=0)  {GETPACK(id,MENU_RIGHT_STRUCK);}
   static int menu_quit(int id=0)          {GETPACK(id,MENU_QUIT);}
   static int menu_accept(int id=0)        {GETPACK(id,MENU_ACCEPT);}
   static int menu_set_option(int id=0)    {GETPACK(id,MENU_SET_OPTION);}
   static int menu_page_up(int id=0)       {GETPACK(id,MENU_PAGE_UP);}
   static int menu_page_down(int id=0)     {GETPACK(id,MENU_PAGE_DOWN);}
   static int menu_anykey(int id=0)        {GETPACK(id,MENU_ANYKEY);}
   //SHORTCUTS
   static int help(int id=0)               {GETPACK(id,HELP);}
   static int quit(int id=0)               {GETPACK(id,QUIT);}
   static int pause(int id=0)              {GETPACK(id,PAUSE);}
   static int abort(int id=0)              {GETPACK(id,ABORT);}
   static int restart(int id=0)            {GETPACK(id,RESTART);}
   static int hires(int id=0)              {GETPACK(id,HIRES);}
   static int gamma_plus(int id=0)         {GETPACK(id,GAMMA_PLUS);}
   static int gamma_minus(int id=0)        {GETPACK(id,GAMMA_MINUS);}
   static int controls(int id=0)           {GETPACK(id,CONTROLS);}
   static int detail_level(int id=0)       {GETPACK(id,DETAIL_LEVEL);}
   static int captscreen(int id=0)         {GETPACK(id,CAPTSCREEN);}
   static int captscreen_noinfo(int id=0)  {GETPACK(id,CAPTSCREEN_NOINFO);}
   static int colisworld(int id=0)         {GETPACK(id,COLISWORLD);}
   static int display_fps(int id=0)        {GETPACK(id,DISPLAY_FPS);}
   static int next_view(int id=0)          {GETPACK(id,NEXT_VIEW);}
   static int make_failure(int id=0)       {GETPACK(id,MAKE_FAILURE);}
   static int points(int id=0)             {GETPACK(id,POINTS);}
   static int cheating(int id=0)           {GETPACK(id,CHEATING);}
   static int no_wall(int id=0)            {GETPACK(id,NO_WALL);}
   static int net_cheating(int id=0)       {GETPACK(id,NET_CHEATING);}
   static int header_anykey(int id=0)      {GETPACK(id,HEADER_ANYKEY);}
   static int last_objective(int id=0)     {GETPACK(id,LAST_OBJECTIVE);}
   static int test_objective(int id=0)     {GETPACK(id,TEST_OBJECTIVE);}
   //NET HEADER
   static int net_head_accept(int id=0)    {GETPACK(id,NET_HEAD_ACCEPT);}
   static int net_head_reject(int id=0)    {GETPACK(id,NET_HEAD_REJECT);}
   static int net_head_nextlev(int id=0)   {GETPACK(id,NET_HEAD_NEXTLEV);}
   static int net_head_prevlev(int id=0)   {GETPACK(id,NET_HEAD_PREVLEV);}
   //CHAT&NETMSG DATA
   static char *chat_buf(int id)           {check_init();check_user(id);return pack[id].CHAT_BUF;}
   static char chat_holded_key(int id)     {GETPACK(id,CHAT_HOLDED_KEY)}
   static int netmsg_request(int id=0)     {GETPACK(id,NETMSG_REQUEST);}
   static int netmsg_abort(int id=0)       {GETPACK(id,NETMSG_ABORT);}
   static int netmsg_backspace(int id=0)   {GETPACK(id,NETMSG_BACKSPACE);}
   static char *netmsg_buf(int id)         {check_init();check_user(id);return pack[id].CHAT_BUF;}
   static int chat_request(int id=0)       {GETPACK(id,CHAT_REQUEST);}
   static int chat_accept(int id=0)        {GETPACK(id,CHAT_ACCEPT);}
   static int chat_backspace(int id=0)     {GETPACK(id,CHAT_BACKSPACE);}
   static int chat_delete(int id=0)        {GETPACK(id,CHAT_DELETE);}
   static int chat_abort(int id=0)         {GETPACK(id,CHAT_ABORT);}
   static int chat_prevword(int id=0)      {GETPACK(id,CHAT_PREVWORD);}
   static int chat_nextword(int id=0)      {GETPACK(id,CHAT_NEXTWORD);}
   static int chat_left(int id=0)          {GETPACK(id,CHAT_LEFT);}
   static int chat_right(int id=0)         {GETPACK(id,CHAT_RIGHT);}
   static int chat_beg(int id=0)           {GETPACK(id,CHAT_BEG);}
   static int chat_end(int id=0)           {GETPACK(id,CHAT_END);}
   static int chat_prev_command(int id=0)  {GETPACK(id,CHAT_PREV_COMMAND);}
};

#endif