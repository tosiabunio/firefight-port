#ifndef __ITEM__
#define __ITEM__

//KLASA ITEM
//Klasa ta jest reprezentacja konkretnej pozycji w menu. Na tejze klasie
//nalezy budowac konkretne klasy obslugujace poszczegolne opcje.
class Item : public Heap_object
{
  protected:
   int select_status;
   int status;
   Item(int _select_status);
  public:
   virtual ~Item(void) {}
   virtual void draw(Screen &screen,int x,int y,int active)=0;
   virtual int run(Menu &menu)=0;
   virtual int selectable(int status=-1);
   virtual int set_status(int _status);
   virtual int get_dy()=0;
   virtual int get_dx()=0;
};
//KLASA TEXTITEM
//Klasa ta jest reprezentacja tekstu wypisywanego na ekranie
class TextItem : public Item
{
  private:
   char *text;
  public:
   TextItem(char* description,Text& descript_text);
   virtual ~TextItem(void) {}
   virtual void draw(Screen &screen,int x,int y,int active);
   virtual int run(Menu &menu);
   virtual int get_dy();
   virtual int get_dx();
};
//KLASA SUBITEM
//Klasa ta jest reprezentacja menu, po wybraniu ktorego wchodzi sie do
//innego menu (czyli swoisty podkatalog). W stanie nieaktywnym wyglada
//ona jak zwykly napis, natomiast w stanie aktywnym jej reprezentacja
//jest podswietlony napis
class SubItem : public Item
{
  private:
   char *text;
  public:
   SubItem(char* description,Text& descript_text);
   virtual ~SubItem(void) {}
   virtual void draw(Screen &screen,int x,int y,int active);
   virtual int run(Menu &menu);
   virtual int get_dy();
   virtual int get_dx();
};
//KLASA BUTTITEM
//Klasa ta jest reprezentacja switcha ON OFF.
class ButtItem : public Item
{
  private:
   char *text;
  public:
   ButtItem(char* description,Text& descript_text);
   virtual ~ButtItem(void);
   virtual void draw(Screen &screen,int x,int y,int active);
   virtual int run(Menu &menu);
   virtual int get_dy();
   virtual int get_dx();
};
//KLASA RBUTTITEM
//Klasa ta jest reprezentacja switcha radiowego.
class RButtItem : public Item
{
  private:
   char **text;
   int size;
   int potential;
   int item_left;
   int space_y;
  public:
   RButtItem(char* description,Text& descript_text);
   virtual ~RButtItem(void);
   virtual void draw(Screen &screen,int x,int y,int active);
   virtual int set_status(int _status);
   virtual int run(Menu &menu);
   virtual int get_dy();
   virtual int get_dx();
};
//KLASA ZIPPITEM
//Klasa ta jest reprezentacja suwaka.
class ZippItem : public Item
{
  private:
   char *text;
   int down_range;
   int up_range;
   int zipper_length;
   int zipper_states;
   int zipper_item_width;
   int state;
   int first_time_status;
  public:
   ZippItem(char* description,Text& descript_text);
   virtual ~ZippItem(void);
   virtual void draw(Screen &screen,int x,int y,int active);
   virtual int run(Menu &menu);
   virtual int set_status(int _status);
   virtual int get_dy();
   virtual int get_dx();
};

#endif





