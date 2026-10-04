#include "1ee_hdrs.h"

Bitflag Vkey::status(0);
Array<Vkey::Item> *Vkey::array;
int Vkey::quantity(0);

//----------------------------------------------------------------------------
// Status flags
//----------------------------------------------------------------------------
const int vkey_Initialized  = 0x00000010; // Klasa Vkey zainicjalizowana

//-----------------------------------------------------------------------------
void Vkey::init ()
{
  array = NEW(Array<Item>(),"Vkey::array");
  array->alloc(VKEY_REF_SIZE ,"Vkey::array");
  array->fillbyte(0);
  quantity = 1;
  status.set(vkey_Initialized);
}
//-----------------------------------------------------------------------------
void Vkey::reset_content(void)
{
  array->fillbyte(0);
  quantity = 1;
}
//-----------------------------------------------------------------------------
void Vkey::quit (void)
{
  if (array)
  {
    array->free();
    delete array;
  }
  status.reset(vkey_Initialized);
}
//-----------------------------------------------------------------------------
int  Vkey::set(int device, unsigned char scan, char *name, unsigned char parallel_scan)
{
  (*array)[quantity].device = device;
  (*array)[quantity].scan   = scan;
  (*array)[quantity].name   = name;
  (*array)[quantity].parallel_scan = parallel_scan;  // port
  quantity++;
  return (quantity-1);
}
//-----------------------------------------------------------------------------
int  Vkey::is (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_vkey *st = &(Eem::selected()->vkey);
  if (code==anykey)
  {
    for (int i=0; i<VKEY_ARRAY_SIZE; i++)
      if (st->arr_struck[i]||st->arr_release[i]||st->arr_state[i])
        return(1);
    return(0);
  }
  else
    return ((Bit::is(st->arr_state, code))||(Bit::is(st->arr_release, code))||
            (Bit::is(st->arr_state, code))||(Bit::is(st->arr_release, code)));
}
//-----------------------------------------------------------------------------
int  Vkey::struck (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_vkey *st = &(Eem::selected()->vkey);
  if (code==anykey)
  {
    for (int i=0; i<VKEY_ARRAY_SIZE; i++)
      if (st->arr_struck[i])
        return(1);
    return(0);
  }
  else
    return ((Bit::is(st->arr_struck, code))||(Bit::is(st->arr_struck, code)));
}
//-----------------------------------------------------------------------------
int  Vkey::press  (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_vkey *st = &(Eem::selected()->vkey);
  if (code==anykey)
  {
    for (int i=0; i<VKEY_ARRAY_SIZE; i++)
      if (st->arr_state[i]&(~(st->arr_struck[i]|st->arr_release[i])))
        return(1);
    return(0);
  }
  else
    return ( (Bit::is(st->arr_state, code)&&(!Bit::is(st->arr_struck, code))&&(!Bit::is(st->arr_release, code)))||
             (Bit::is(st->arr_state, code)&&(!Bit::is(st->arr_struck, code))&&(!Bit::is(st->arr_release, code)))   );
}
//-----------------------------------------------------------------------------
int  Vkey::release (int code)
{
  DBG_CHECK(Eem::last_return_code!=0);
  DBG_CHECK(Eem::selected()!=NULL);
  Eem::St_vkey *st = &(Eem::selected()->vkey);
  if (code==anykey)
  {
    for (int i=0; i<VKEY_ARRAY_SIZE; i++)
      if (st->arr_release[i])
        return(1);
    return(0);
  }
  else
    return (Bit::is(st->arr_release, code)||Bit::is(st->arr_release, code));
}
//-----------------------------------------------------------------------------
int  Vkey::is_initialized(void)
{
  return(status.is(vkey_Initialized));
}
//-----------------------------------------------------------------------------
void Vkey::clear(void)                                                                                
{                                                                                                    
}                                                                                                    
//---------------------------------------------------------------------------                        
void Vkey::next_read (void)                                                                           
{                                                                                                    
}                                                                                                    
//---------------------------------------------------------------------------                        
void Vkey::complete (void)
{       
  Eem::St_kbd   *stk = &(Eem::get_new()->kbd);
  Eem::St_mouse *stm = &(Eem::get_new()->mouse);
  Eem::St_joy   *stj = &(Eem::get_new()->joy);
  Eem::St_vkey  *stv = &(Eem::get_new()->vkey);
  for (int i=0; i<VKEY_REF_SIZE; i++)
    switch ((*array)[i].device)
    {
      case dev_Kbd:   
      {
        if (Bit::is(stk->arr_state, (*array)[i].scan))
          Bit::set(stv->arr_state, i);
        if (Bit::is(stk->arr_struck, (*array)[i].scan))
          Bit::set(stv->arr_struck, i);
        if (Bit::is(stk->arr_release, (*array)[i].scan))
          Bit::set(stv->arr_release, i);
        if ((*array)[i].scan>=128)       
        {
          if (Bit::is(stk->arr_state, (*array)[i].scan-128))
            Bit::set(stv->arr_state, i);
          if (Bit::is(stk->arr_struck, (*array)[i].scan-128))
            Bit::set(stv->arr_struck, i);
          if (Bit::is(stk->arr_release, (*array)[i].scan-128))
            Bit::set(stv->arr_release, i);
        }
        else
        {
          if (Bit::is(stk->arr_state, (*array)[i].scan+128))
            Bit::set(stv->arr_state, i);
          if (Bit::is(stk->arr_struck, (*array)[i].scan+128))
            Bit::set(stv->arr_struck, i);
          if (Bit::is(stk->arr_release, (*array)[i].scan+128))
            Bit::set(stv->arr_release, i);
        }
        if ((*array)[i].parallel_scan)  // port: the second key (RegData: WASD for the cursor keys)
        {
          if (Bit::is(stk->arr_state, (*array)[i].parallel_scan))
            Bit::set(stv->arr_state, i);
          if (Bit::is(stk->arr_struck, (*array)[i].parallel_scan))
            Bit::set(stv->arr_struck, i);
          if (Bit::is(stk->arr_release, (*array)[i].parallel_scan))
            Bit::set(stv->arr_release, i);
        }
        break;
      }
      case dev_Mouse: 
      {
        if (Bit::is(&stm->arr_state, (*array)[i].scan))
          Bit::set(stv->arr_state, i);
        if (Bit::is(&stm->arr_struck, (*array)[i].scan))
          Bit::set(stv->arr_struck, i);
        if (Bit::is(&stm->arr_release, (*array)[i].scan))
          Bit::set(stv->arr_release, i);
        break;
      }
      case dev_Joy: 
      {
        if (Bit::is(stj->arr_state, (*array)[i].scan))
        {
          Bit::set(stv->arr_state, i);
          if (!Bit::is(stj->prev_arr_state, (*array)[i].scan))
            Bit::set(stv->arr_struck, i);
        }
        else
        {
          if (Bit::is(stj->prev_arr_state, (*array)[i].scan))
            Bit::set(stv->arr_release, i);
        }
        break;
      }
      default:
      {
        break;
      }
    }
}                                                                                                    
//---------------------------------------------------------------------------                                                                              
char *Vkey::get_key_name(int code)
{
  return((*array)[code].name);
}
//---------------------------------------------------------------------------                                                                              
char *Vkey::get_scan_name(int scan)
{
  switch ((*array)[scan].device)
  {
    case dev_Kbd:   return (Kbd::get_scan_name((*array)[scan].scan));
    case dev_Mouse: return (Mouse::get_scan_name((*array)[scan].scan));
    case dev_Joy:   return (Joy::get_scan_name((*array)[scan].scan));
    default: return(NULL);
  }
}
//---------------------------------------------------------------------------                                                                              

