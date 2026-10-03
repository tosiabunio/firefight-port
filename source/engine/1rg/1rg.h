#ifndef _1RG_H_INCLUDED
#define _1RG_H_INCLUDED

#ifndef EXCLUDE_LIBS
#ifdef _DEBUG
#pragma comment(lib,"1rgd.lib")
#else
#pragma comment(lib,"1rg.lib")
#endif
#endif

#define REG_APP_NAME_LEN   80
#define REG_BUFFER_LEN   1024

//=============================================================================
// class Registry
//=============================================================================

class Registry
{
  static const char s_software_key[];
  static const char s_company_key[];
  static const char s_version_key[];
  static char       buffer[REG_BUFFER_LEN];
  static char       s_application_key[REG_APP_NAME_LEN];
  static char       s_application_sub_key[REG_APP_NAME_LEN];
  static Bitflag    status;
  static HKEY       base_key;

  static void quit (void);
public:
  enum
  {
    mode_User    = 1,
    mode_Machine = 2
  };
  static void      init (char *application_name, char *application_sub_name=NULL, 
                         int mode=mode_User, int version=0);
  static HKEY      lock_key (char *section=NULL);
  static void      unlock_key(HKEY hkey);
  static void      set_mode (int mode, int version=0);

  static unsigned    get_int    (char *section, char *entry,	unsigned def);
  static const char *get_string (char *section, char *entry,	char *def);
  static BOOL  get_string (char *section, char *entry,	char *def, char *destination, unsigned *length);
  static BOOL  get_binary (char *section, char *entry,	char* data, unsigned* size);
  static BOOL  set_int    (char *section, char *entry,	unsigned value);
  static BOOL  set_string (char *section, char *entry,	char *value);
  static BOOL  set_binary (char *section, char *entry,	char* data, unsigned size);
  static BOOL  delete_entry   (char *section, char *entry);
  static BOOL  delete_section (char *section);
  static BOOL  delete_application_key(void);
};

//=============================================================================
// class Cmd_line
//=============================================================================



class Cmd_line
{
  static const  char s_def_yes[];
  static const  char s_def_no[];
  static char        command_line [_MAX_PATH];
  static char        return_buffer[_MAX_PATH];
  static Bitflag     status;
public:
  enum
  {
    assignment_char = '=',
    separator_char  = ' ',
    file_Name       = 0x00000001,
    file_Drive      = 0x00000002,
    file_Dir        = 0x00000004,
    file_Ext        = 0x00000008,
    file_Path       = file_Drive|file_Dir,
    file_Full       = 0xFFFFFFFF
  };
  static void      init (char *cmd_line = NULL);
  static void      quit (void);
  static char     *get_exe (int components = file_Full);
  static char     *get(void);
  static char     *get_string(char *label, char *def=NULL);
  static unsigned  get_int(char *label, int def);
};


/*=============================================================================
// class Registry description
===============================================================================
W poddrzewie HKEY_CURENT_USER\Software jest tworzony klucz odpowiadajacy nazwie 
producenta (w tym wypadku "chaos_works"). Nastepnie tworzony jest podklucz 
odpowiadajacy nazwie produktu (podaje sie do funkcji init()). Wewnatrz tego klucza 
uzytkownik moze tworzyc sobie sekcje i umieszczac w nich zmienne. Zmienna to lancuch char*
lub unsigned lub dowolny blok binarny. Zmienne oraz sekcje mozna kasowac. Oprocz tego 
funkcja lock_key pozwala uzyskac uchwyt do danej sekcji jako klucza w registry. 
Uzytkownik moze sobie wtedy uzyc funkcji Reg* z API Windows do tworzenia dowolnej 
struktury wewnatrz tego klucza. Nie zaleca sie tego. Po ewentualnym uzyciu nalezy 
uzyc unlock_key. Wszystkie funkcje powoduja automatyczne zalozenie odpowiedniej sekcji 
i zmiennej w momencie wywolania. Nie potrzeba wiec zadnych  funkcji do tworzenia sekcji.

OPIS METOD KLASY Registry
-------------------------

void init (char *application_key);
   Inicjuje klase. Musi to byc pierwsza wywolana metoda. 
   Parametry:
      application_name  - nazwa aplikacji. Musi byc bez spacji i znakow przestankowych 
                          (tylko znaki od 33 do 127). Na jej podstawie zostanie stworzony 
                          klucz w registry.
   
unsigned get_int (char *section, char *entry,	unsigned def);
   Zwraca wartosc zmiennej numerycznej w registry (unsigned). Jezeli zmiennej nie
   ma w registry zwraca wartosc parametru def. Nie mozna tej funkcji uzywac do 
   odczytania zmiennej lancuchowej ani bloku binarnego.
   Parametry:
      section - nazwa sekcji w poddrzewie dla danej aplikacji
      entry   - nazwa zmiennej
      def     - wartosc defaultowa

const char *get_string (char *section, char *entry,	char *def);
   Zwraca wartosc zmiennej lancuchowej w registry (char*). Jezeli zmiennej nie
   ma w registry zwraca wartosc parametru def. Nie mozna tej funkcji uzywac do 
   odczytania zmiennej numerycznej ani bloku binarnego. zwrocone char* zawiera 
   wskaznik do oczekiwanej wartosci zmiennej, do czasu nastepnego uzycia dowolnej 
   metody klasy Registry. Potem lancuch znajdujacy sie pod zwroconym adresem 
   jest nie zdefiniowany.
   Parametry:
      section - nazwa sekcji w poddrzewie dla danej aplikacji
      entry   - nazwa zmiennej
      def     - wartosc defaultowa

BOOL get_string (char *section, char *entry,	char *def, char *destination, unsigned *length);
   Druga metoda do uzyskania zmiennej numerycznej z registry. Wartosc jest kopiowana
   do wskazanego bufora.
   Parametry:
      section      - nazwa sekcji w poddrzewie dla danej aplikacji
      entry        - nazwa zmiennej
      def          - wartosc defaultowa
      destination  - wskaznik do bufora do ktorego ma byc wkopiowana wartosc zmiennej
      length       - wskaznik do zmiennej ktora zawiera dlugosc bufora. Jezeli
                     bufor jest za maly operacja nie zostanie wykonana. Tak czy inaczej 
                     w zmiennej length po wywolaniu tej funkcji znajdzie sie faktyczna 
                     dlugosc lancucha (bez znaku '\0') Bufor destination musi byc 
                     przynajmniej taki dlugi aby zmiescil sie w nim lancuch i konczacy 
                     go znak '\0'. Jezeli adres bufora podamy jako NULL w zmiennej length 
                     zostanie zwrocona dlugosc lancucha (bez znaku konczacego). Takie
                     wywolanie tej funkcji sluzy okresleniu jak duzy bufor trzeba zaalokowac.
   Jezeli funkcja zwroci TRUE operacja sie powiodla, jezeli nie - nastapil blad i nie nalezy 
   niczego oczekiwac po danych zwroconych w destination i length.
      
BOOL get_binary (char *section, char *entry,	char* data, unsigned* size);
   Zwraca blok binarny z registry. Nie mozna tej metody uzywac do odczytu zmiennej numerycznj 
   ani lancuchowej.
   Parametry:
      section  - nazwa sekcji w poddrzewie dla danej aplikacji
      entry    - nazwa zmiennej
      data     - wskaznik do bufora do ktorego ma byc wkopiowany blok binarny
      size     - wskaznik do zmiennej ktora zawiera dlugosc bufora. Jezeli bufor jest za maly 
                 operacja nie zostanie wykonana. W zmiennej size po wywolaniu tej funkcji 
                 znajdzie sie faktyczna dlugosc bloku. Bufor data musi byc przynajmniej taki 
                 dlugi aby zmiescil sie w nim blok. Jezeli adres bufora podamy jako NULL w 
                 zmiennej length zostanie zwrocona dlugosc bloku. Takie  wywolanie tej funkcji 
                 sluzy okresleniu jak duzy bufor trzeba zaalokowac. 
   Jezeli funkcja zwroci TRUE operacja sie powiodla, jezeli nie - nastapil blad i nie nalezy 
   niczego oczekiwac po danych zwroconych w data i size.

BOOL set_int (char *section, char *entry,	unsigned value);
   Zapisuje z registry zmienna typu unsigned.
   Parametry:
      section  - nazwa sekcji w poddrzewie dla danej aplikacji
      entry    - nazwa zmiennej
      value    - wartosc zmiennej
   Jezeli wykonanie funkcji zakonczylo sie sukcesem zostaje zwrocone TRUE w przeciwnym
   wypadku funkcja zwraca FALSE.
      
BOOL set_string (char *section, char *entry,	char *value);
   Zapisuje z registry zmienna lancuchowa.
   Parametry:
      section  - nazwa sekcji w poddrzewie dla danej aplikacji
      entry    - nazwa zmiennej
      value    - wartosc zmiennej
   Jezeli wykonanie funkcji zakonczylo sie sukcesem zostaje zwrocone TRUE w przeciwnym
   wypadku funkcja zwraca FALSE.
      
BOOL set_binary (char *section, char *entry,	char* data, unsigned size);
   Zapisuje z registry blok binarny.
   Parametry:
      section  - nazwa sekcji w poddrzewie dla danej aplikacji
      entry    - nazwa zmiennej
      data     - adres bloku
      size     - rozmiar bloku
   Jezeli wykonanie funkcji zakonczylo sie sukcesem zostaje zwrocone TRUE w przeciwnym
   wypadku funkcja zwraca FALSE.
      
BOOL delete_entry   (char *section, char *entry);
   Kasuje zmienna entry z sekcji section.

BOOL delete_section (char *section);
   Kasuje sekcje section.

BOOL delete_application_key(void);
   Kasuje cale poddrzewo dla danej aplikacji


HKEY lock_key (char *section=NULL);
   Zwraca uchwyt do klucza w registry. Poslugujac sie tym uchwytem mozna manipulowac 
   danymi w registry przy pomocy API windows. Klucz nalezy "odwiesic" funkcja 
   unlock_key. Jezeli funkcja nie powiedzie sie - zwraca NULL. Jezeli podamy section funkcja 
   zwroci klucz do danej sekcji, jezeli nie - zwroci klucz do danych calej aplikacji.

void unlock_key(HKEY hkey);
   Zwalnia klucz zalockowany przez lock_key.


=============================================================================*/

#endif