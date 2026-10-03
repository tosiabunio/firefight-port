#include "1ee_hdrs.h"

// Demo files and network packets hold raw structs in the byte order of the original x86 build.
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#error "Fire Fight data formats are little-endian; big-endian targets are not supported"
#endif
static_assert(sizeof(int)==4 && sizeof(short)==2, "Fire Fight data formats assume 32-bit int, 16-bit short");

static const int rec_Initialized  = 0x00000001;
static const int rec_Opened       = 0x00000002;

static const int play_Initialized = 0x00000001;
static const int play_Opened      = 0x00000002;

static const unsigned DEMO_MAX_PACKET_SIZE   = 16000;
static const unsigned DEMO_NEED_COMPRESS_BUF = DEMO_MAX_PACKET_SIZE + 1000;

static char  last_demo_file[_MAX_PATH] = "";

char        *Demo_recorder::main_buffer = NULL;
char        *Demo_recorder::cprs_buffer = NULL;
Demo_header  Demo_recorder::header;
unsigned     Demo_recorder::buf_size = 0;
Bitflag      Demo_recorder::status(0);
unsigned     Demo_recorder::version_of_data(0);
char         Demo_recorder::rec_file_name[_MAX_PATH]; 

char        *Demo_player::main_buffer = NULL;
char        *Demo_player::cprs_buffer = NULL;
Demo_header  Demo_player::header;
unsigned     Demo_player::pointer = 0;
unsigned     Demo_player::block_pointer = 0;
Bitflag      Demo_player::status(0);
unsigned     Demo_player::version_of_data(0);

//=============================================================================
// Demo_recorder
//=============================================================================
void  Demo_recorder::init (unsigned data_version)
{
  DBG_CHECK(!status.is(rec_Initialized));
  version_of_data = data_version;  
  rec_file_name[0] = 0;
  status.set(rec_Initialized);
}
//-----------------------------------------------------------------------------
void  Demo_recorder::quit (void)
{
  DBG_CHECK(status.is(rec_Initialized));
  status.reset(rec_Initialized);
}
//-----------------------------------------------------------------------------
void  Demo_recorder::open (char *file_name, char *user_data, unsigned user_data_size,
                           unsigned store_method, int players_num, unsigned max_size)
{
  DBG_CHECK(status.is(rec_Initialized));
  DBG_CHECK(!status.is(rec_Opened));
  DBG_CHECK(max_size >= (sizeof(Demo_header) + user_data_size + 4));
  DBG_CHECK(players_num > 0);
  DBG_CHECK(user_data_size <= DEMO_MAX_PACKET_SIZE);
  buf_size = max_size;
  main_buffer=(char*)VirtualAlloc(NULL, buf_size, MEM_COMMIT, PAGE_READWRITE);
  cprs_buffer=(char*)VirtualAlloc(NULL, DEMO_NEED_COMPRESS_BUF, MEM_COMMIT, PAGE_READWRITE);
  if ((main_buffer == NULL)||(cprs_buffer == NULL))
  {
    if (main_buffer==NULL)
      VirtualFree(main_buffer, 0, MEM_RELEASE);
    if (cprs_buffer==NULL)
      VirtualFree(main_buffer, 0, MEM_RELEASE);
    FAILURE("Demo_recorder::open - not enough memory to allocate buffer for demo Demo_recorder");
  }
  memset(&header, 0, sizeof(Demo_header));
  header.data_version = version_of_data;
  header.store_method = store_method;
  header.players_num  = players_num;
  header.blocks       = 0;
  header.size         = sizeof(Demo_header);
  if ((user_data!=NULL)&&(user_data_size!=0))
  {
    *(unsigned short*)(&(main_buffer[header.size])) = (unsigned short)user_data_size;
    header.size += 2;
    memcpy(&(main_buffer[header.size]), user_data, user_data_size);
    header.size += user_data_size;
  }
  else
  {
    memset(&(main_buffer[header.size]), 0, 2);
    header.size += 2;
  }
  if (file_name != NULL)
    strncpy(rec_file_name, file_name, _MAX_PATH-1);
  else
    rec_file_name[0] = 0;
  status.set(rec_Opened);
}
//-----------------------------------------------------------------------------
BOOL  Demo_recorder::record (char *buffer, unsigned size)
{
  DBG_CHECK(status.is(rec_Initialized));
  DBG_CHECK(status.is(rec_Opened));
  DBG_CHECK(buffer!=NULL);
  DBG_CHECK(size < DEMO_MAX_PACKET_SIZE);
  BOOL result = FALSE;
  switch (header.store_method)
  {
    case store_RLE8:
      {
        unsigned pkt_size = rle_8_compress((unsigned char*)cprs_buffer, (unsigned char*)buffer, size);
//------------ DIAGNOSTICS -------------
        static unsigned char diag_buf[16000];
        unsigned diag_size = rle_8_decompress((unsigned char*)diag_buf, (unsigned char*)cprs_buffer, pkt_size);
        if (diag_size != size)
          FAILURE("Demo_recorder - RLE8 failure (size)");
        if (memcmp(diag_buf, buffer, size)!= 0)
          FAILURE("Demo_recorder - RLE8 failure (memcmp)");
//---------------------------------------

        if (header.size + pkt_size + 2 > buf_size)
          result = FALSE;
        else
        {
          *((unsigned short*)(&(main_buffer[header.size]))) = (unsigned short)pkt_size;
          header.size += 2;
          memcpy(&(main_buffer[header.size]), cprs_buffer, pkt_size);
          header.size += pkt_size;
          result = TRUE;
        }
      }
      break;
    case store_LZW:
      {
        unsigned pkt_size = Xio::compress(cprs_buffer, buffer, size);
        if (header.size + pkt_size + 2 > buf_size)
          result = FALSE;
        else
        {
          *((unsigned short*)(&(main_buffer[header.size]))) = (unsigned short)pkt_size;
          header.size += 2;
          memcpy(&(main_buffer[header.size]), cprs_buffer, pkt_size);
          header.size += pkt_size;
          result = TRUE;
        }
      }
      break;
    case store_Plain:
      {
        if (header.size + size + 2 > buf_size)
          result = FALSE;
        else
        {
          *((unsigned short*)(&(main_buffer[header.size]))) = (unsigned short)size;
          header.size += 2;
          memcpy(&(main_buffer[header.size]), buffer, size);
          header.size += size;
          result = TRUE;
        }
      }
      break;
    default:
      result = FALSE;
  }
  if (result)
    header.blocks++;
  return(result);
}
//-----------------------------------------------------------------------------
void  Demo_recorder::close  (void)
{
  DBG_CHECK(status.is(rec_Initialized));
  DBG_CHECK(status.is(rec_Opened));
  BOOL  name_found = FALSE;
  int   fh;
  char  name [_MAX_PATH];
  if (strlen(rec_file_name) != 0)
  {
    strncpy(name, rec_file_name, _MAX_PATH-1);
    name_found = TRUE;
  }
  else
  {
    char tmppath[_MAX_PATH];
    if(GetTempPath(sizeof(tmppath),tmppath)==0)
      sprintf(tmppath,"c:");
    int i = -1;
    BOOL error = FALSE;
    do
    {
      i++;
      sprintf (name, "%s\\demo_%03d.rec", tmppath, i);
      fh = _open(name, O_RDONLY);
      _close (fh);
    }
    while ((i<1000) && (fh!=-1));
    if (i<1000)
      name_found = TRUE;
  }
  BOOL error = FALSE;
  if (name_found)
  {
    if ((fh = _open(name, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, _S_IREAD | _S_IWRITE)) != -1)
    {
      memcpy(main_buffer, &header, sizeof(Demo_header));
      ((unsigned short*)main_buffer)[0] = crc16(&(main_buffer[4]), header.size - 4);
      ((unsigned short*)main_buffer)[1] = sum16(&(main_buffer[4]), header.size - 4);
      if (_write (fh, main_buffer, header.size)==-1)
        error = TRUE;
      _close (fh);
    }
    else
      error = TRUE;
  }
  else
    error = TRUE;
  if (error)
    FAILURE("Demo_recorder::close - error saving demo file");
  else
  {
    MESSAGE("demo %s saved - %d players, %d bytes, %d blocks", 
            name, header.players_num, header.size, header.blocks);
    strncpy(last_demo_file, name, _MAX_PATH-1);
  }
  if (main_buffer!=NULL)
  {
    VirtualFree(main_buffer, 0, MEM_RELEASE);
    main_buffer = NULL;
  }
  if (cprs_buffer!=NULL)
  {
    VirtualFree(cprs_buffer, 0, MEM_RELEASE);
    cprs_buffer = NULL;
  }
  status.reset(rec_Opened);
}
//=============================================================================
// Demo_player
//=============================================================================
void  Demo_player::init (unsigned data_version)
{
  DBG_CHECK(!status.is(play_Initialized));
  version_of_data = data_version;  
  status.set(play_Initialized);
}
//-----------------------------------------------------------------------------
void  Demo_player::quit (void)
{
  DBG_CHECK(status.is(play_Initialized));
  status.reset(play_Initialized);
}
//-----------------------------------------------------------------------------
BOOL Demo_player::read_disk_file (char *file_name)
{
  int fh;
  fh = _open(file_name, O_RDONLY, O_BINARY);
  if (fh==-1)
  {
    WARNING ("Demo_player::open - cannot open demo file");
    return(FALSE);
  }
  if (_read (fh, &header, sizeof(Demo_header))==-1)
  {
    _close (fh);
    WARNING ("Demo_player::open - error reading demo file");
    return(FALSE);
  }
  if (header.data_version != version_of_data)
  {
    WARNING ("Demo_player::open - incorrect demo version");
    return (FALSE);
  }
  main_buffer=(char*)VirtualAlloc(NULL, header.size, MEM_COMMIT, PAGE_READWRITE);
  cprs_buffer=(char*)VirtualAlloc(NULL, DEMO_NEED_COMPRESS_BUF, MEM_COMMIT, PAGE_READWRITE);
  if ((main_buffer == NULL)||(cprs_buffer == NULL))
  {
    if (main_buffer==NULL)
      VirtualFree(main_buffer, 0, MEM_RELEASE);
    if (cprs_buffer==NULL)
      VirtualFree(main_buffer, 0, MEM_RELEASE);
    WARNING ("Demo_player::open - cannot allocate buffer for demo Demo_player");
    return (FALSE);
  }
  memcpy(main_buffer, &header, sizeof(Demo_header));
  if (_read (fh, &(main_buffer[sizeof(Demo_header)]), header.size - sizeof(Demo_header))==-1)
  {
    _close (fh);
    WARNING ("Demo_player::open - error reading demo file");
    return(FALSE);
  }
  _close (fh);
  unsigned short c16 = crc16(&(main_buffer[4]), header.size - 4);
  unsigned short s16 = sum16(&(main_buffer[4]), header.size - 4);
  if ((((unsigned short*)main_buffer)[0] != c16)||(((unsigned short*)main_buffer)[1] != s16))
  {
    WARNING ("Demo_player::open - demo file corrupted");
    return (FALSE);
  }
  else
  {
    if (!Comm::production)
      MESSAGE("demo %s loaded - %d players, %d bytes", file_name, header.players_num, header.size);
    return (TRUE);
  }
}
//-----------------------------------------------------------------------------
BOOL Demo_player::read_volume_file (char *file_name)
{
  if ((!File::specified(file_name))||(!File::exist(file_name)))
  {
    WARNING ("Demo_player::open - demo file not found");
    return (FALSE);
  }
  File::open(file_name);
  File::read((char*)(&header), sizeof(Demo_header));
  if (header.data_version != version_of_data)
  {
    WARNING ("Demo_player::open - incorrect demo version");
    return (FALSE);
  }
  main_buffer=(char*)VirtualAlloc(NULL, header.size, MEM_COMMIT, PAGE_READWRITE);
  cprs_buffer=(char*)VirtualAlloc(NULL, DEMO_NEED_COMPRESS_BUF, MEM_COMMIT, PAGE_READWRITE);
  if ((main_buffer == NULL)||(cprs_buffer == NULL))
  {
    if (main_buffer==NULL)
      VirtualFree(main_buffer, 0, MEM_RELEASE);
    if (cprs_buffer==NULL)
      VirtualFree(main_buffer, 0, MEM_RELEASE);
    WARNING ("Demo_player::open - cannot allocate buffer for demo Demo_player");
    return (FALSE);
  }
  memcpy(main_buffer, &header, sizeof(Demo_header));
  File::read((char*)(&(main_buffer[sizeof(Demo_header)])), header.size - sizeof(Demo_header));
  unsigned short c16 = crc16(&(main_buffer[4]), header.size - 4);
  unsigned short s16 = sum16(&(main_buffer[4]), header.size - 4);
  File::close();
  if ((((unsigned short*)main_buffer)[0] != c16)||(((unsigned short*)main_buffer)[1] != s16))
  {
    WARNING ("Demo_player::open - demo file corrupted");
    return (FALSE);
  }
  else
  {
    if (!Comm::production)
      MESSAGE("demo %s loaded - %d players, %d bytes, %d blocks", 
               file_name, header.players_num, header.size, header.blocks);
    return (TRUE);
  }
}
//-----------------------------------------------------------------------------
BOOL Demo_player::open (char *file_name, unsigned *players, char *user_data, unsigned *size)
{
  DBG_CHECK(status.is(play_Initialized));
  DBG_CHECK(!status.is(play_Opened));
  DBG_CHECK(players != NULL);
  BOOL io_res = FALSE;
  if ((file_name == NULL)||(file_name[0]=='!'))
  {
    char disk_file[_MAX_PATH];
    if ((file_name == NULL)&&(strlen(last_demo_file)==0))
      return(FALSE);
    else if (file_name == NULL)
      strncpy(disk_file, last_demo_file, _MAX_PATH-1);
    else
      strncpy(disk_file, &(file_name[1]), _MAX_PATH-1);
    io_res = read_disk_file(disk_file);
  }
  else
    io_res = read_volume_file(file_name);
  if (!io_res)
    return(FALSE);
  pointer = sizeof(Demo_header);
  unsigned short ui_size = *(unsigned short*)(&(main_buffer[pointer]));
  pointer += 2;
  DBG_CHECK(ui_size<=DEMO_MAX_PACKET_SIZE);
  if (size != NULL)
    *size = ui_size;
  if (ui_size > 0)
  {
    if (user_data!=NULL)
      memcpy(user_data, &(main_buffer[pointer]), ui_size);
    pointer += ui_size;
  }
  block_pointer = 0;
  status.set(play_Opened);
  *players = header.players_num;
  return (TRUE);
}
//-----------------------------------------------------------------------------
BOOL  Demo_player::play (char *buffer, unsigned *size)
{
  DBG_CHECK(status.is(play_Initialized));
  DBG_CHECK(status.is(play_Opened));
  DBG_CHECK(buffer!=NULL);
  DBG_CHECK(size!=NULL);
  BOOL result = FALSE;
  if (pointer >= header.size)
    result = FALSE;
  else
  {
    switch (header.store_method)
    {
      case store_RLE8:
        {
          unsigned cprs_size = *((unsigned short*)(&(main_buffer[pointer])));
          pointer += 2;
          unsigned pkt_size = rle_8_decompress((unsigned char*)cprs_buffer, 
                                               (unsigned char*)(&(main_buffer[pointer])), cprs_size);
          pointer += cprs_size;
          memcpy(buffer, cprs_buffer, pkt_size);
          *size = pkt_size;
          result = TRUE;
        }
        break;
      case store_LZW:
        {
          unsigned cprs_size = *((unsigned short*)(&(main_buffer[pointer])));
          pointer += 2;
          unsigned pkt_size = Xio::decompress(cprs_buffer, &(main_buffer[pointer]), cprs_size);
          pointer += cprs_size;
          memcpy(buffer, cprs_buffer, pkt_size);
          *size = pkt_size;
          result = TRUE;
        }
        break;
      case store_Plain:
        {
          unsigned pkt_size = *((unsigned short*)(&(main_buffer[pointer])));
          pointer += 2;
          memcpy(buffer, &(main_buffer[pointer]), pkt_size);
          pointer += pkt_size;
          *size = pkt_size;
          result = TRUE;
        }
        break;
      default:
        result = FALSE;
    }
  }
  if (result)
    block_pointer++;
  return(result);
}
//-----------------------------------------------------------------------------
void  Demo_player::close (void)
{
  DBG_CHECK(status.is(play_Initialized));
  DBG_CHECK(status.is(play_Opened));
  if (main_buffer != NULL)
  {
    VirtualFree(main_buffer, 0, MEM_RELEASE);
    VirtualFree(cprs_buffer, 0, MEM_RELEASE);
    main_buffer = NULL;
    cprs_buffer = NULL;
  }
  status.reset(play_Opened);
}
//=============================================================================
