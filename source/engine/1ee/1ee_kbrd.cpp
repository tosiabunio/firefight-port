#include "1ee_hdrs.h"

//----------------------------------------------------------------------------
// Kbd scan code masks
//----------------------------------------------------------------------------
const int mask_Released      = 0x00000080; // oznacza ze klawisz o danym scan kodzie zostal
                                           // puszczony (nacisniecie jest bez flagi)
const int mask_Extended      = 0x0000e000; // oznacza ze scan code reprezentuje extended key (szary)
// Kbd
const int kbd_Initialized    = 0x00000010; // Kbd zainicjalizowana

//----------------------------------------------------------------------------
// Kbd static variables
//----------------------------------------------------------------------------
unsigned      Kbd::diag_string_overflows(0);
int           Kbd::string_idx(0);
Bitflag       Kbd::status(0);
unsigned char Kbd::ref[KBD_REF_SIZE][2];
char         *Kbd::names[KBD_NAMES_SIZE];
HHOOK         Kbd::hook_handler(NULL);
int           Kbd::last_key(0);
//===========================================================================
// Kbd
//===========================================================================
LRESULT CALLBACK Kbd::callback_keyboard (int nCode, WPARAM wParam, LPARAM lParam)
{
  int code = (lParam>>16) & 0x000000ff;
  int repeat_count = lParam & 0x0000FFFF;
  int extended = (lParam & 0x01000000)>>24;
  int released = ((lParam & 0x80000000)>>31);
  int event = code;
  event |= ev_Kbd;
  if (extended)
    event |= mask_Extended;
  if (released)
    event |= mask_Released;
  if (last_key!=event)
  {
    Eem::push_event(event);
    last_key=event;
  }
  if (nCode<0)             
    return(CallNextHookEx(hook_handler, nCode, wParam, lParam));
  else
    return(0);
}
//---------------------------------------------------------------------------
void Kbd::init ()
{
  DBG_CHECK(!status.is(kbd_Initialized));
  status.set(kbd_Initialized);
  clear();
  init_ref();
  hook_handler = SetWindowsHookEx(WH_KEYBOARD, (HOOKPROC)Kbd::callback_keyboard,
                                  NULL, Eem::get_thread());
  if (hook_handler==NULL)
    FAILURE2(Eem_error::general, "unable to hook keyboard [Kbd::init]");
  MESSAGE ("keyboard connected.");
}
//---------------------------------------------------------------------------
void Kbd::quit (void)
{
  UnhookWindowsHookEx(hook_handler);
  status = 0;
  hook_handler = NULL;
  ENGINFO ("keyboard status: string overflows = %d", diag_string_overflows);
  MESSAGE ("keyboard disconnected");
}
//---------------------------------------------------------------------------
int Kbd::is (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_kbd *st = &(Eem::selected()->kbd);
  if (code==anykey)
  {
    for (int i=0; i<KBD_ARRAY_SIZE; i++)
      if (st->arr_struck[i]||st->arr_release[i]||st->arr_state[i])
        return(1);
    return(0);
  }
  else
    return ((Bit::is(st->arr_state, ref[code][0]))||(Bit::is(st->arr_release, ref[code][0]))||
            (Bit::is(st->arr_state, ref[code][1]))||(Bit::is(st->arr_release, ref[code][1])));
}
//---------------------------------------------------------------------------
int Kbd::struck (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_kbd *st = &(Eem::selected()->kbd);
  if (code==anykey)
  {
    for (int i=0; i<KBD_ARRAY_SIZE; i++)
      if (st->arr_struck[i])
        return(1);
    return(0);
  }
  else
    return ((Bit::is(st->arr_struck, ref[code][0]))||(Bit::is(st->arr_struck, ref[code][1])));
}
//---------------------------------------------------------------------------
int Kbd::release (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_kbd *st = &(Eem::selected()->kbd);
  if (code==anykey)
  {
    for (int i=0; i<KBD_ARRAY_SIZE; i++)
      if (st->arr_release[i])
        return(1);
    return(0);
  }
  else
    return (Bit::is(st->arr_release, ref[code][0])||Bit::is(st->arr_release, ref[code][1]));
}
//---------------------------------------------------------------------------
int Kbd::press (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_kbd *st = &(Eem::selected()->kbd);
  if (code==anykey)
  {
    for (int i=0; i<KBD_ARRAY_SIZE; i++)
      if (st->arr_state[i]&(~(st->arr_struck[i]|st->arr_release[i])))
        return(1);
    return(0);
  }
  else
    return ( (Bit::is(st->arr_state, ref[code][0])&&(!Bit::is(st->arr_struck, ref[code][0]))&&(!Bit::is(st->arr_release, ref[code][0])))||
             (Bit::is(st->arr_state, ref[code][1])&&(!Bit::is(st->arr_struck, ref[code][1]))&&(!Bit::is(st->arr_release, ref[code][1])))   );
}
//---------------------------------------------------------------------------
int Kbd::is_initialized(void)
{
  return (status.is(kbd_Initialized));
}
//---------------------------------------------------------------------------
// Kbd private
//---------------------------------------------------------------------------
void Kbd::get_event(int event)
{
  Eem::St_kbd *st = &(Eem::get_new()->kbd);
  int modifier = 0;
  event&=~ev_Kbd;
  if ((event&mask_Extended)!=0)
  {
    modifier = 128;
    event&=~mask_Extended;
  }
  if ((event&mask_Released)!=0)
  {
    event&=~mask_Released;
    Bit::reset(st->arr_state, modifier + event);
    Bit::set(st->arr_release, modifier + event);
  }
  else
  {
    Bit::set(st->arr_state, modifier + event);
    Bit::set(st->arr_struck, modifier + event);
    if (string_idx<(KBD_STRING_SIZE-1))
    {
      UINT vkey = MapVirtualKey(event, 1);                               
      if (vkey!=0)
      {
        UINT ckey = MapVirtualKey(vkey, 2);
        if (ckey!=0)
        {
          char cvalue = (char)ckey;
          if ((cvalue>=32)&&(cvalue<128))
          {
            if ((cvalue>='A')&&(cvalue<='Z'))
              cvalue += 32;
            if ((Bit::is(st->arr_state, ref[shift][0]))||
                (Bit::is(st->arr_state, ref[shift][1])))
              cvalue = upper_scan(cvalue);
            st->string[string_idx] = cvalue;
            string_idx++;
          }
        }
      }
    }
    else
      diag_string_overflows++;
  }
}
//--------------------------------------------------------------------------- 
char *Kbd::get_string(void)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_kbd *st = &(Eem::selected()->kbd);
  return(st->string);
}
//--------------------------------------------------------------------------- 
void Kbd::init_ref(void)
{
  for (int i=0; i<256; i++)
  {
    ref[i][0]=0;
    ref[i][1]=0;
    names[i] = NULL;
  }
  ref[key0][0]=11;                                             
  ref[key1][0]=2;                                              
  ref[key2][0]=3;                                              
  ref[key3][0]=4;                                              
  ref[key4][0]=5;                                              
  ref[key5][0]=6;                                              
  ref[key6][0]=7;                                              
  ref[key7][0]=8;                                              
  ref[key8][0]=9;                                              
  ref[key9][0]=10;                                             
  ref[keya][0]=30;                                             
  ref[keyb][0]=48;                                             
  ref[keyc][0]=46;                                             
  ref[keyd][0]=32;                                             
  ref[keye][0]=18;                                             
  ref[keyf][0]=33;                                             
  ref[keyg][0]=34;                                             
  ref[keyh][0]=35;                                             
  ref[keyi][0]=23;                                             
  ref[keyj][0]=36;                                             
  ref[keyk][0]=37;                                             
  ref[keyl][0]=38;                                             
  ref[keym][0]=50;                                             
  ref[keyn][0]=49;                                             
  ref[keyo][0]=24;                                             
  ref[keyp][0]=25;                                             
  ref[keyq][0]=16;                                             
  ref[keyr][0]=19;                                             
  ref[keys][0]=31;                                             
  ref[keyt][0]=20;                                             
  ref[keyu][0]=22;                                             
  ref[keyv][0]=47;                                             
  ref[keyw][0]=17;                                             
  ref[keyx][0]=45;                                             
  ref[keyy][0]=21;                                             
  ref[keyz][0]=44;                                             
  ref[f1][0]=59;                                               
  ref[f2][0]=60;                                               
  ref[f3][0]=61;                                               
  ref[f4][0]=62;                                               
  ref[f5][0]=63;                                               
  ref[f6][0]=64;                                               
  ref[f7][0]=65;                                               
  ref[f8][0]=66;                                               
  ref[f9][0]=67;                                               
  ref[f10][0]=68;                                              
  ref[f11][0]=87;                                              
  ref[f12][0]=88;                                              
  ref[backapostrophe][0]=41;                                   
  ref[backslash][0]=43;                                        
  ref[backspace][0]=14;                                        
  ref[lbracket][0]=26;                                         
  ref[rbracket][0]=27;                                         
  ref[semicolon][0]=39;                                        
  ref[apostrophe][0]=40;                                        
  ref[point][0]=52;                                            
  ref[comma][0]=51;                                            
  ref[space][0]=57;                                            
  ref[spacebar][0]=57;                                        
  ref[left][0]=75;              ref[left][1]=128+75;           
  ref[right][0]=77;             ref[right][1]=128+77;          
  ref[up][0]=72;                ref[up][1]=128+72;             
  ref[down][0]=80;              ref[down][1]=128+80;           
  ref[middle][0]=76;                                           
  ref[tab][0]=15;                                              
  ref[plus][0]=78;                                             
  ref[minus][0]=74;             ref[minus][1]=12;              
  ref[asterisk][0]=55;                                         
  ref[slash][0]=53;             ref[slash][1]=128+53;          
  ref[equal][0]=13;                                            
  ref[enter][0]=28;             ref[enter][1]=128+28;          
  ref[esc][0]=1;                                               
  ref[shift][0]=42;             ref[shift][1]=54;              
  ref[ctrl][0]=29;              ref[ctrl][1]=128+29;           
  ref[alt][0]=56;               ref[alt][1]=128+56;            
  ref[home][0]=71;              ref[home][1]=128+71;           
  ref[end][0]=79;               ref[end][1]=128+79;            
  ref[pageup][0]=73;            ref[pageup][1]=128+73;         
  ref[pgup][0]=73;              ref[pgup][1]=128+73;           
  ref[pagedown][0]=81;          ref[pagedown][1]=128+81;       
  ref[pagedn][0]=81;            ref[pagedn][1]=128+81;         
  ref[pgdn][0]=81;              ref[pgdn][1]=128+81;           
  ref[pgdown][0]=81;            ref[pgdown][1]=128+81;         
  ref[ins][0]=82;               ref[ins][1]=128+82;            
  ref[del][0]=83;               ref[del][1]=128+83;            
  ref[printscreen][0]=128+55;                                  
  ref[prtsc][0]=128+55;                                        
  ref[prtscr][0]=128+55;                                       
  ref[pause][0]=69;
  ref[lshift][0]=42;                                           
  ref[lalt][0]=56;                                             
  ref[lctrl][0]=29;                                            
  ref[rshift][0]=54;                                           
  ref[ralt][0]=128+56;                                         
  ref[rctrl][0]=128+29;                                        
  ref[gray_plus][0]=78;                                        
  ref[gray_minus][0]=74;                                       
  ref[capslock][0]=58;                                         
  ref[scrolllock][0]=70;                                       
  ref[numlock][0]=128+69;                                          
  ref[keyb_slash][0]=53;                                       
  ref[keyb_enter][0]=28;                                       
  ref[keyb_0][0]=11;                                           
  ref[keyb_1][0]=2;                                            
  ref[keyb_2][0]=3;                                            
  ref[keyb_3][0]=4;                                            
  ref[keyb_4][0]=5;                                            
  ref[keyb_5][0]=6;                                            
  ref[keyb_6][0]=7;                                            
  ref[keyb_7][0]=8;                                            
  ref[keyb_8][0]=9;                                            
  ref[keyb_9][0]=10;                                           
  ref[keyb_minus][0]=12;                                       
  ref[ext_insert][0]=128+82;                                   
  ref[ext_ins][0]=128+82;                                      
  ref[ext_home][0]=128+71;                                    
  ref[ext_pgup][0]=128+73;                                    
  ref[ext_pageup][0]=128+73;                                  
  ref[ext_delete][0]=128+83;                                  
  ref[ext_del][0]=128+83;                                     
  ref[ext_end][0]=128+79;                                     
  ref[ext_pgdown][0]=128+81;                                  
  ref[ext_pgdn][0]=128+81;                                    
  ref[ext_pagedown][0]=128+81;                                
  ref[ext_pagedn][0]=128+81;                                  
  ref[ext_up][0]=128+72;                                      
  ref[ext_down][0]=128+80;                                    
  ref[ext_left][0]=128+75;                                    
  ref[ext_right][0]=128+77;                                   
  ref[num_insert][0]=82;                                      
  ref[num_ins][0]=82;                                         
  ref[num_home][0]=71;                                        
  ref[num_pgup][0]=73;                                        
  ref[num_pageup][0]=73;                                      
  ref[num_delete][0]=83;                                      
  ref[num_del][0]=83;                                         
  ref[num_end][0]=79;                                         
  ref[num_pgdown][0]=81;                                      
  ref[num_pgdn][0]=81;                                        
  ref[num_pagedown][0]=81;                                    
  ref[num_pagedn][0]=81;                                      
  ref[num_up][0]=72;                                          
  ref[num_down][0]=80;                                        
  ref[num_left][0]=75;                                        
  ref[num_right][0]=77;                                       
  ref[num_middle][0]=76;                                      
  ref[num_slash][0]=128+53;                                   
  ref[num_minus][0]=74;                                       
  ref[num_plus][0]=78;                                        
  ref[num_enter][0]=128+28;                                   
  ref[num_asterisk][0]=55;                                    
  ref[num_0][0]=82;                                           
  ref[num_1][0]=79;                                           
  ref[num_2][0]=80;                                           
  ref[num_3][0]=81;                                           
  ref[num_4][0]=75;                                           
  ref[num_5][0]=76;                                           
  ref[num_6][0]=77;                                           
  ref[num_7][0]=71;                                           
  ref[num_8][0]=72;                                           
  ref[num_9][0]=73;                    

  names[11]      = "0";                                      
  names[2]       = "1";                                       
  names[3]       = "2";                                       
  names[4]       = "3";                                       
  names[5]       = "4";                                       
  names[6]       = "5";                                       
  names[7]       = "6";                                       
  names[8]       = "7";                                       
  names[9]       = "8";                                       
  names[10]      = "9";                                      
  names[30]      = "a";                                      
  names[48]      = "b";                                      
  names[46]      = "c";                                      
  names[32]      = "d";                                      
  names[18]      = "e";                                      
  names[33]      = "f";                                      
  names[34]      = "g";                                      
  names[35]      = "h";                                      
  names[23]      = "i";                                      
  names[36]      = "j";                                      
  names[37]      = "k";                                      
  names[38]      = "l";                                      
  names[50]      = "m";                                      
  names[49]      = "n";                                      
  names[24]      = "o";                                      
  names[25]      = "p";                                      
  names[16]      = "q";                                      
  names[19]      = "r";                                      
  names[31]      = "s";                                      
  names[20]      = "t";                                      
  names[22]      = "u";                                      
  names[47]      = "v";                                      
  names[17]      = "w";                                      
  names[45]      = "x";                                      
  names[21]      = "y";                                      
  names[44]      = "z";                                      
  names[59]      = "f1";                                   
  names[60]      = "f2";                                   
  names[61]      = "f3";                                   
  names[62]      = "f4";                                   
  names[63]      = "f5";                                   
  names[64]      = "f6";                                   
  names[65]      = "f7";                                   
  names[66]      = "f8";                                   
  names[67]      = "f9";                                   
  names[68]      = "f10";                                  
  names[87]      = "f11";                                  
  names[88]      = "f12";                                  
  names[41]      = "tilde";                                
  names[43]      = "backslash";                  
  names[14]      = "backspace";                  
  names[26]      = "left bracket";               
  names[27]      = "right bracket";              
  names[39]      = "semicolon";                  
  names[40]      = "apostrophe";                 
  names[52]      = "period";
  names[51]      = "comma";                      
  names[57]      = "space";                      
  names[75]      = "left";                       
  names[77]      = "right";                      
  names[72]      = "up";                         
  names[80]      = "down";                       
  names[76]      = "numpad 5";                   
  names[15]      = "tab";                        
  names[78]      = "plus";                       
  names[12]      = "minus";                      
  names[74]      = "minus";                 
  names[55]      = "asterisk";                   
  names[53]      = "slash";                      
  names[13]      = "equal sign";                 
  names[28]      = "enter";                      
  names[1]       = "escape";                     
  names[42]      = "left shift";                 
  names[54]      = "right shift";                

  names[29]      = "ctrl";                  
  names[56]      = "alt";                   
  /*    NOT_EXTENDED_KEYBOARD
  names[29]      = "left ctrl";                  
  names[56]      = "left alt";                   
  */
  names[71]      = "home";                       
  names[79]      = "end";                        
  names[73]      = "page up";                    
  names[81]      = "page down";                  
  names[82]      = "ins";                        
  names[83]      = "del";                        
  names[69]      = "pause";                     
  names[58]      = "caps lock";                 
  names[70]      = "scroll lock";               
  names[128+69]  = "num lock";                   
  names[128+75]  = "gray left";                  
  names[128+77]  = "gray right";                 
  names[128+72]  = "gray up";                    
  names[128+80]  = "gray down";                  
  names[128+78]  = "gray plus";                  
  names[128+53]  = "gray slash";                 
  names[128+28]  = "small enter";                
  names[128+29]  = "right ctrl";                 
  names[128+56]  = "right alt";                  
  names[128+71]  = "gray home";                  
  names[128+79]  = "gray end";                   
  names[128+73]  = "gray page up";               
  names[128+81]  = "gray page down";             
  names[128+82]  = "gray ins";                   
  names[128+83]  = "gray del";                   
  names[128+55]  = "prtscr";                     
}                                                                                                                                                          
//---------------------------------------------------------------------------                                                                              
char *Kbd::get_key_name(int code)
{
  return(names[ref[code][0]]);
}
//---------------------------------------------------------------------------
unsigned char Kbd::get_scan(int code)
{
  return(ref[code][0]);
}
//---------------------------------------------------------------------------
char *Kbd::get_scan_name(unsigned char scan)
{
  return(names[scan]);
}
//---------------------------------------------------------------------------
unsigned char Kbd::get_struck(void)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_kbd *st = &(Eem::selected()->kbd);
  for (int i=0; i<KBD_ARRAY_SIZE; i++)
    if (st->arr_struck[i]!=0)
    {
      for (int k=0; k<8; k++)
        if (Bit::is(&(st->arr_struck[i]), k))
           return((unsigned char)(i*8+k));
    }
  return(no_struck);
}
//---------------------------------------------------------------------------
char *Kbd::get_struck_name(void)
{
  return (names[get_struck()]);
}
//---------------------------------------------------------------------------                                                                              
void Kbd::clear(void)                                                                                
{                                                                                                    
  last_key=0;                                                                                        
}                                                                                                    
//---------------------------------------------------------------------------                        
void Kbd::next_read (void)                                                                           
{                                                                                                    
  string_idx = 0;
}                                                                                                    
//---------------------------------------------------------------------------                        
void Kbd::complete (void)
{                                                                                                    
}                                                                                                    
//===========================================================================                        
char Kbd::upper_scan(char ch)
{
  if ((ch>='a')&&(ch<='z'))
    return ((char)(ch-32));
  else
  {
    switch (ch)
    {
      case '0' : return(')');  break;
      case '1' : return('!');  break;
      case '2' : return('@');  break;
      case '3' : return('#');  break;
      case '4' : return('$');  break;
      case '5' : return('%');  break;
      case '6' : return('^');  break;
      case '7' : return('&');  break;
      case '8' : return('*');  break;
      case '9' : return('(');  break;
      case '\\': return('|');  break;
      case '\'': return('\"'); break;
      case '.' : return('>');  break;
      case ',' : return('<');  break;
      case '-' : return('_');  break;
      case '/' : return('?');  break;
      case '=' : return('+');  break;
      case '[' : return('{');  break;
      case ']' : return('}');  break;
      case ';' : return(':');  break;
      default  : return('\0'); break;
    }
  }         
}
//---------------------------------------------------------------------------                        

                                                                                                      