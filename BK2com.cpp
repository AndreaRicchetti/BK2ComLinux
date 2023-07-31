// -----------------------------------------------------------------------------
// BK2com
// -----------------------------------------------------------------------------

#include "BK2com.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef __linux__
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <fcntl.h>
  #include <termios.h>
  #include <unistd.h>
  #include <time.h>
  #include <ctype.h>
  #include <errno.h>

  typedef unsigned char BYTE;
  typedef unsigned short UI16;
  #define _stricmp strcmp

  #define BK2_COMM_PORT    "/dev/ttyUSB0"
//  #define BK2_COMM_PORT    "/dev/ttymxc0"

  static int com_port = -1;
#else
  #include <windows.h>

  #define BK2_COMM_PORT     "COM1"

  static HANDLE hComm = INVALID_HANDLE_VALUE;
#endif

#define BK2_COMM_RETRIES  3
#define BK2_COMM_TIMEOUT  1000
#define BK2_COMM_SPEED    38400

// -----------------------------------------------------------------------------

int com_init ( char *dev )
{
#ifdef __linux__
  com_port = open(dev, O_RDWR | O_NOCTTY | O_NDELAY);
  if (com_port != -1)
  {
    fcntl(com_port, F_SETFL, FNDELAY);
    return 1;
  }
#else
  hComm = ::CreateFile(dev,
    GENERIC_READ | GENERIC_WRITE,
    0, NULL, OPEN_EXISTING, 0, NULL);
  if (hComm != INVALID_HANDLE_VALUE)
  {
    ::SetupComm(hComm, 4096, 2048);
    COMMTIMEOUTS ct;
    ct.ReadIntervalTimeout = MAXDWORD;
    ct.ReadTotalTimeoutMultiplier   = 0;
    ct.ReadTotalTimeoutConstant     = 0;
    ct.WriteTotalTimeoutMultiplier  = 100;
    ct.WriteTotalTimeoutConstant    = 1000;
    ::SetCommTimeouts(hComm, &ct);
    return 1;
  }
#endif
  return 0;
}

int com_read ()
{
  int c = 0;
#ifdef __linux__
  if (read(com_port, &c, 1) != 1)
  {
    usleep(10);
    return -1;
  }
#else
  DWORD ret;
  COMSTAT cst;
  ::ClearCommError(hComm, &ret, &cst);
  if (cst.cbInQue == 0)
  {
    ::Sleep(1);
    return -1;
  }
  ::ReadFile(hComm, &c, 1, &ret, NULL);
#endif
  return c;
}

void com_send ( char c )
{
#ifdef __linux__
  write(com_port, &c, 1);
#else
  DWORD ret;
  ::WriteFile(hComm, &c, 1, &ret, NULL);
  ::ClearCommError(hComm, &ret, NULL);
#endif
}

void com_sect ( char *buf )
{
#ifdef __linux__
  write(com_port, buf, 512);
#else
  DWORD ret;
  WriteFile(hComm, buf, 512, &ret, NULL);
  ClearCommError(hComm, &ret, NULL);
#endif
}

void com_setmode ( int baud )
{
#ifdef __linux__
  struct termios ts;
  tcgetattr(com_port, &ts);
  // set N81 (no parity, no flow control)
  ts.c_cflag = CLOCAL | CREAD | CS8;
  switch (baud)
  {
    case 19200: ts.c_cflag |= B19200; break;
    case 38400: ts.c_cflag |= B38400; break;
    default: ts.c_cflag |= B9600; break;
  }
  // set to RAW binary data
  ts.c_lflag = 0; // no echo processing
  ts.c_iflag = 0; // no input processing
  ts.c_oflag = 0; // no output processing
  ts.c_cc[VMIN] = 0;
  ts.c_cc[VTIME] = 10;
  tcsetattr(com_port, TCSAFLUSH, &ts);
#else
  DCB dcb;
  dcb.DCBlength = sizeof(dcb);
  ::GetCommState(hComm, &dcb);
  dcb.BaudRate = baud;
  dcb.ByteSize = 8;
  dcb.StopBits = ONESTOPBIT;
  dcb.Parity = NOPARITY;
  dcb.fParity = FALSE;
  dcb.fBinary = TRUE;
  dcb.fNull = FALSE;
  dcb.fInX = FALSE;
  dcb.fOutX = FALSE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fOutxCtsFlow = FALSE;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_ENABLE;
  ::SetCommState(hComm, &dcb);
#endif
}

void com_flush ()
{
#ifdef __linux__
  while (com_read() >= 0);
#else
  DWORD err;
  ::PurgeComm(hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);
  ::ClearCommError(hComm, &err, NULL);
#endif
}

void com_close ()
{
#ifdef __linux__
  close(com_port);
#else
    if (hComm != INVALID_HANDLE_VALUE)
        ::CloseHandle(hComm);
    hComm = INVALID_HANDLE_VALUE;
#endif
}

unsigned msec_timer ( void )
{
#ifdef __linux__
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return (t.tv_sec*1000 + t.tv_nsec/1000000);
#else
  return GetTickCount();
#endif
}

// -----------------------------------------------------------------------------

// Serial frame encapsulation (from RFC 1055 - SLIP)
#define COMM_END            0300    // 0xC0 (192)
#define COMM_ESC            0333    // 0xDB (219)
#define COMM_ESC_END        0334    // 0xDC (220)
#define COMM_ESC_ESC        0335    // 0xDD (221)

#define COMM_BUFSIZE        128     // power of 2

typedef struct comm_buff_struct
{
  BYTE ptr;
  BYTE esc;
  BYTE buf [COMM_BUFSIZE + 2];
}
comm_buff;

typedef struct comm_chan_struct
{
  comm_buff i;
  comm_buff o;
}
comm_chan;

static comm_chan comm_ch;

// -----------------------------------------------------------------------------

BYTE CL_PacketRecv ( void )
{
  register comm_buff *p = &(comm_ch.i);
  while (1)
  {
    int c = com_read();
    if (c < 0)
      break;
    if (c == COMM_END)
    {
      BYTE *b = p->buf;
      BYTE n = p->ptr;
      p->esc = 0;
      p->ptr = 0;
      if (n > 1)
      {
        if (*b++ == (n - 2)) // check length
        {
          BYTE s = 0;
          while (n-- > 1)
            s += *b++;
          if (s == 0) // 2-complement checksum
          {
            return p->buf[1];
          }
        }
        return (p->buf[1] = 0xFF); // checksum error -> invalidates command
      }
    }
    else if (c == COMM_ESC)
    {
      p->esc = c;
    }
    else
    {
      if (p->esc)
      {
        p->esc = 0;
        if (c == COMM_ESC_ESC) c = COMM_ESC;
        if (c == COMM_ESC_END) c = COMM_END;
      }
      if (p->ptr <= COMM_BUFSIZE)
        p->buf[p->ptr++] = c;
    }
  }
  return 0;
}

// -----------------------------------------------------------------------------

void CL_PacketSend ( void )
{
  register comm_buff *p = &(comm_ch.o);
  if (p->ptr)
  {
    register BYTE c = 0;
    register BYTE i = p->ptr++;
    register BYTE *b = p->buf;
    *b++ = i;
    while (i--)
      c += *b++;
    *b = (BYTE)-c; // 2-complement checksum
    i = 0;

    while (p->ptr)
    {
      if (p->esc)
      {
        c = p->esc;
        p->esc = 0;
      }
      else if (i <= p->ptr)
      {
        c = p->buf[i++];
        if (c == COMM_ESC)
        {
          c = COMM_ESC;
          p->esc = COMM_ESC_ESC;
        }
        if (c == COMM_END)
        {
          c = COMM_ESC;
          p->esc = COMM_ESC_END;
        }
      }
      else
      {
        c = COMM_END;
        p->ptr = 0;
      }
      com_send(c);
    }
  }
}

// -----------------------------------------------------------------------------

void CL_SendByte ( BYTE data )
{
  register comm_buff *p = &(comm_ch.o);
  if (p->ptr <= COMM_BUFSIZE)
    p->buf[++(p->ptr)] = data;
}

void CL_SendInit ( BYTE token )
{
  comm_ch.o.ptr = 0;
  CL_SendByte(token);
}

void CL_SendShort ( short data )
{
  CL_SendByte(data >> 8);
  CL_SendByte(data);
}

void CL_SendLong ( long data )
{
  CL_SendByte(data >> 24);
  CL_SendByte(data >> 16);
  CL_SendByte(data >> 8);
  CL_SendByte(data);
}

void CL_SendDouble ( double data )
{
  register BYTE *p = (BYTE*)&data;
  register int i = sizeof(data);
  while (i--)
    CL_SendByte(*p++);
}

void CL_SendString ( char *data )
{
  while (data && *data)
    CL_SendByte(*data++);
  CL_SendByte(0);
}

// -----------------------------------------------------------------------------

BYTE CL_RecvByte ( void )
{
  register comm_buff *p = &(comm_ch.i);
  register BYTE i = p->buf[0]++;
  return p->buf[i];
}

BYTE CL_RecvInit ( void )
{
  comm_ch.i.buf[0] = 1;
  return CL_RecvByte();
}

short CL_RecvShort ( void )
{
  short x = 0;
  x = (x << 8) | CL_RecvByte();
  x = (x << 8) | CL_RecvByte();
  return x;
}

long CL_RecvLong ( void )
{
  long x = 0;
  x = (x << 8) | CL_RecvByte();
  x = (x << 8) | CL_RecvByte();
  x = (x << 8) | CL_RecvByte();
  x = (x << 8) | CL_RecvByte();
  return x;
}

double CL_RecvDouble ( void )
{
  double x;
  register BYTE *p = (BYTE*)&x;
  register int i = sizeof(x);
  while (i--)
    *p++ = CL_RecvByte();
  return x;
}

void CL_RecvString ( char *data, size_t data_dize )
{
  if (data)
  {
    BYTE c;
    size_t s = 0;
    do
    {
      c = CL_RecvByte();
      *data++ = c;
      s++;
    }
    while (c && s < data_dize);
  }
}

// -----------------------------------------------------------------------------

void BK2_Init ( char *com )
{
  char dev[80];
#ifdef __linux__
  if(com != NULL)
    snprintf(dev, sizeof(dev), "%s", com);
  else
    snprintf(dev, sizeof(dev), "%s", BK2_COMM_PORT);
#else
  strcpy_s(dev, "\\\\.\\");
  if (!com)
    strcat_s(dev, BK2_COMM_PORT);
  else
    strcat_s(dev, com);
#endif
  if (!com_init(dev))
  {
    printf("Error: opening serial port '%s' (%s)\n", dev, strerror(errno));
    exit(0);
  }
  // set baudrate and flush
  com_setmode(BK2_COMM_SPEED);
  com_send(COMM_END);
  com_flush();
}

bool BK2_Client ( BYTE cmd, const char *fmt = NULL, ... )
{
  unsigned to = BK2_COMM_TIMEOUT;
  int retries = BK2_COMM_RETRIES;
  while (retries--)
  {
    CL_SendInit(cmd);
    if (fmt)
    {
      const char *p = fmt;
      va_list arg;
      va_start(arg, fmt);
      while (*p)
      {
        char a = *p++;
        switch (a)
        {
          case 'b': // byte
            CL_SendByte((BYTE)va_arg(arg, int));
            break;
          case 'i': // long int
            CL_SendLong((long)va_arg(arg, long));
            break;
          case 's': // short int
            CL_SendShort((short)va_arg(arg, int));
            break;
          case 'x': // double
            CL_SendDouble((double)va_arg(arg, double));
            break;
          case 'z': // zero terminated string
            CL_SendString((char *)va_arg(arg, char *));
            break;
          case 'p': // byte block (len,pointer)
          case 'P':
            {
              BYTE len = (BYTE)va_arg(arg, int);
              BYTE *bb = (BYTE*)va_arg(arg, BYTE *);
              if (a == 'p')
                CL_SendByte(len);
              while (len--)
                CL_SendByte(*bb++);
            }
            break;
          case '-': // only one retry
            retries = 0;
          case '+': // and set timeout
            to = atoi(p);
            //printf("R=%d,TO=%d\n", retries, to);
            break;
        }
      }
      va_end(arg);
    }
    CL_PacketSend();
    unsigned t = msec_timer();
    unsigned delta = 0;
    do
    {
      delta = msec_timer() - t;
      BYTE ack = CL_PacketRecv();
      if (ack)
      {
        if (ack == cmd)
        {
          CL_RecvInit();
          return true;
        }
      }
    }
    while (delta < to);
  }
  return false;
}

void BK2_Close ( void )
{
  com_close();
}

// -----------------------------------------------------------------------------

static char pboot_ret [8];

bool pboot_ack ( unsigned msec )
{
  unsigned t = msec_timer();
  int i = 0;
  pboot_ret[i] = 0;
  //FlushFileBuffers(hComm);
  while ((msec_timer() - t) < msec)
  {
    int ch = com_read();
    if (ch == '$')
      i = 0;
    if (ch >= 0 && i < 7)
      pboot_ret[i++] = ch;
    pboot_ret[i] = 0;
    if (ch == '*')
    {
      //printf("%s\n", pboot_ret);
      return true;
    }
  }
  return false;
}

void BK2_Update(const char* file_name)
{
    int sent_sectors = 0;
    int filesize = 0;

    FILE *f = fopen(file_name, "r");
    if(f == NULL)
        return;

    fseek(f, 0L, SEEK_END);
    filesize = ftell(f) / 512;
    fseek(f, 0L, SEEK_SET);
    if(filesize <= 0)
    {
        fclose(f);
        return;
    }

    unsigned t = msec_timer();
    BK2_Client(CMD_RESET, "-1");
    // Ignoring the return code and trying anyway to talk to the boot loader...

    unsigned t1 = msec_timer();
    int retry = 0;
    int perc = 0;
    int prev_perc = -1;
    do
    {
        retry++;
        com_send('$'); // SYNC
        pboot_ack(10);
        if (pboot_ret[0] == '$')
        {
            com_send('$');
            com_send('V'); // VERSION
            if (pboot_ack(100))
            {
                printf("ERASE\r");
                com_send('$');
                com_send('E'); // ERASE
                if (pboot_ack(2000))
                {
                    long j = 0;
                    char buf[512];
                    int bytes = 0;
                    fseek(f, 0L, SEEK_SET);
                    sent_sectors = 0;
                    while( (bytes = fread(buf, 512, 1, f) > 0))
                    {
                        perc = (++j * 100L)/filesize;
                        if(perc != prev_perc)
                        {
                            printf("Update progress: %d\n", perc);
                            prev_perc = perc;
                        }
                        com_send('$');
                        com_send('W'); // WRITE
                        com_sect(buf);
                        sent_sectors += bytes;

                        if (!pboot_ack(1000))
                        {
                            printf("ERROR (sent_sectors=%d; filesize_sectors=%d; retry=%d)\n", sent_sectors, filesize, retry);
                            break;
                        }
                    }
                }
                printf("RESTART (sent_sectors=%d; filesize_sectors=%d; retry=%d)\n", sent_sectors, filesize, retry);
                com_send('$');
                com_send('R'); // RESET
                pboot_ack(100);
            }
            break;
        }
    }
    while ((msec_timer() - t) < 10000);

    unsigned t2 = msec_timer();
    printf("cmd retry=%d... (%d msec)\n", retry, t2-t1);
    if(perc < 100)
        printf("Update failed!\n");
    else
        printf("Update OK!\n");
    fclose(f);
}

// -----------------------------------------------------------------------------

#define PI 3.14159265358979323846
//#define TO_DEG(x) ((x)*180.0/3.1415926536)
#define TO_DEG(x) ((x)*180.0/PI)

double NormPI(double x)
{
  while (x < -PI) x += (PI*2.0);
  while (x >= PI) x -= (PI*2.0);
  return x;
}

void po_cmd ( char *cmd )
{
  int po = strtol(cmd, &cmd, 10);
  if (po)
  {
    int po2 = po;
    if (*cmd == '-')
    {
      po2 = strtol(cmd+1, NULL, 10);
    }
    else if (*cmd == '=')
    {
      double x = strtod(cmd+1, NULL);
      BK2_Client(CMD_PO_SET_DOUBLE, "sx", po, x);
    }
    do
    {
      long x = 0;
      double val = 0.0;
      double arg = 0.0;
      printf("P%d=", po);
      if (BK2_Client(CMD_PO_GET_COMPLEX, "s", po))
      {
        val = CL_RecvDouble();
        arg = CL_RecvDouble();
        if (val != 0.0 || arg != 0.0)
        {
          printf("%.3f @ %.1f\n", val, TO_DEG(arg));
          continue;
        }
      }
      if (BK2_Client(CMD_PO_GET_STRING, "s", po))
      {
        char buf[32];
        CL_RecvString(buf, sizeof(buf));
        if (buf[0])
        {
          printf("'%s'\n", buf);
          continue;
        }
      }
      if (BK2_Client(CMD_PO_GET_LONG, "s", po))
        x = CL_RecvLong();
      if (BK2_Client(CMD_PO_GET_DOUBLE, "s", po))
        val = CL_RecvDouble();
      if ((double)x != val)
        printf("%.3f\n", val);
      else
        printf("%ld\n", x);
    }
    while (++po <= po2);
  }
}

// -----------------------------------------------------------------------------

long ie [512];

#define CH_A     0x8000
#define CH_B     0x4000
#define CAP_MASK 0x3FFF

void ie_test ( void )
{
  int i;
  int sync = (ie[0] & CH_A) ? 0 : 1;
  int zcap = 0;
  int zero = -1;
  int tmcap = 0;
  int sum_bar = 0;
  int sum_gap = 0;
  int max_bar = 0;
  int max_gap = 0;
  int min_bar = CAP_MASK;
  int min_gap = CAP_MASK;

  for (i = 0; i < 512; i++)
  {
    int x = ie[i] & CAP_MASK;
    if ((i & 1) == sync) // BAR
      if (x > zcap)
        zcap = x, zero = i; // ZERO = max bar
  }
  for (i = 0; i < 512; i++)
  {
    int x = ie[i] & CAP_MASK;
    if ((i & 1) == sync) // BAR
    {
      sum_bar += x;
      if (i != zero)
      {
        if (x < min_bar) min_bar = x;
        if (x > max_bar) max_bar = x;
      }
    }
    else // GAP
    {
      sum_gap += x;
      if (x < min_gap) min_gap = x;
      if (x > max_gap) max_gap = x;
    }
  }
  sum_bar += (sum_bar >> 8) - zcap;
  tmcap = (sum_bar + sum_gap) >> 8;
  if (tmcap)
  {
    fprintf(stderr, "SPEED\t%d\n",
      60000000/(sum_bar+sum_gap));
    fprintf(stderr, "ZERO\t%d\t%.1f\t%.1f\n",
      zero,
      (zcap*100.0)/(sum_bar >> 8),
      (zcap*100.0)/tmcap);
    fprintf(stderr, "BAR\t%.1f\t%.1f\t%.1f\n",
      (sum_bar*100.0)/(sum_bar+sum_gap),
      (min_bar*100.0)/tmcap,
      (max_bar*100.0)/tmcap);
    fprintf(stderr, "GAP\t%.1f\t%.1f\t%.1f\n",
      (sum_gap*100.0)/(sum_bar+sum_gap),
      (min_gap*100.0)/tmcap,
      (max_gap*100.0)/tmcap);
  }

  for (i = 0; i < 512; i++)
    printf("%d\t%d\t%d\t%ld\n", i,
      (ie[i] & CH_A) ? 1 : 0,
      (ie[i] & CH_B) ? 1 : 0,
      ie[i] & CAP_MASK);
}

// -----------------------------------------------------------------------------

int main ( int argc, char **argv )
{
  if (argc > 1 && !strcmp(argv[1], "/?"))
  {
    printf("Syntax:%s [COM#] [-v] {commands|po#[=x]} [--]\n", argv[0]);
    exit(0);
  }
  if (argc > 2 && !strncmp(argv[1], "-d", 2))
  {
    BK2_Init(argv[2]);
    argv++;
    argc--;
  }
  else
  {
    BK2_Init(NULL);
  }

  if (!BK2_Client(CMD_LOGON_RO))
  {
    printf("Error: connection failed\n");
    //goto exit_without_logout;
  }
  while (argc > 1)
  {
    char *cmd = argv[1];
    argv++;
    argc--;
    if (!strcmp(cmd, "--"))
    {
      goto exit_without_logout;
    }
    else if (!_stricmp(cmd, "-v") || !_stricmp(cmd, "/V"))
    {
      char ver[32];
      if (BK2_Client(CMD_GET_VERSION))
      {
        memset(ver, 0, sizeof(ver));
        CL_RecvString(ver, sizeof(ver)-1);
        printf("BK: %s\n", ver);
      }

      if(BK2_Client(CMD_GET_APP_VERSION))
      {
        memset(ver, 0, sizeof(ver));
        CL_RecvString(ver, sizeof(ver)-1);
        printf("UI: %s\n", ver);
      }
    }
    else if (!_stricmp(cmd, "reset"))
    {
      BK2_Client(CMD_RESET, "-1");
      goto exit_without_logout;
    }
    else if (!_stricmp(cmd, "poformat!"))
    {
      BK2_Client(CMD_PO_FORMAT, "-5000");
    }
    else if (!_stricmp(cmd, "poflush"))
    {
      BK2_Client(CMD_PO_FLUSH);
    }
    else if (!_stricmp(cmd, "C85!"))
    {
      BK2_Client(CMD_PO_COPY_EEP1_EEP2, "-5000");
    }
    else if (!_stricmp(cmd, "C86!"))
    {
      BK2_Client(CMD_PO_COPY_EEP2_EEP1, "-5000");
    }
    else if (!_stricmp(cmd, "abort"))
    {
      BK2_Client(CMD_ABORT);
    }
    else if (!_stricmp(cmd, "start"))
    {
      BK2_Client(CMD_START, "bx", 10, 1.0);
    }
    else if (!_stricmp(cmd, "stop"))
    {
      BK2_Client(CMD_STOP);
    }
    else if (!_stricmp(cmd, "lock"))
    {
      BK2_Client(CMD_WHEEL_LOCK);
    }
    else if (!_stricmp(cmd, "unlock"))
    {
      BK2_Client(CMD_WHEEL_UNLOCK);
    }
    else if (!_stricmp(cmd, "unclamp"))
    {
      BK2_Client(CMD_WHEEL_UNCLAMP);
    }
    else if (!_stricmp(cmd, "clamp"))
    {
      BK2_Client(CMD_WHEEL_CLAMP);
    }
    else if (!_stricmp(cmd, "power"))
    {
      if (BK2_Client(CMD_GET_VCC))
        printf("Vcc=%4.2fV\n", (double)CL_RecvShort()/1000.0);
      if (BK2_Client(CMD_GET_LINE_VOLT))
        printf("Vac=%dV @ ", CL_RecvShort());
      if (BK2_Client(CMD_GET_LINE_FREQ))
        printf("%dHz\n", CL_RecvShort());
    }
    else if (!_stricmp(cmd, "temp"))
    {
      if (BK2_Client(CMD_GET_TEMPERATURE))
        printf("T=%4.1f^C\n", CL_RecvDouble());
    }
    else if (!_stricmp(cmd, "lamp"))
    {
      BK2_Client(CMD_LIGHT, "s", 1);
    }
    else if (!_stricmp(cmd, "disp"))
    {
      BK2_Client(CMD_DISPLAY, "bp", 1, 6, "..ÿÿÿÿ");
      BK2_Client(CMD_DISPLAY, "bp", 2, 6, "..ÿÿÿÿ");
      //BK2_Client(CMD_DISPLAY, "bp", 0, 4, "CIAO");
    }
    else if (!_stricmp(cmd, "dwp"))
    {
      if (BK2_Client(CMD_GET_DWP))
        printf("DWP=%5.1f\n", (double)CL_RecvLong()/100.0);
    }
    else if (!_stricmp(cmd, "dwv"))
    {
      if (BK2_Client(CMD_GET_DWV))
        printf("DWV=%d\n", CL_RecvShort());
    }
    else if (!strncmp(cmd, "ad-in", 5))
    {
      short ch = atoi(cmd + 5);
      if (BK2_Client(CMD_GET_ADI, "b", ch))
        printf("ADin%02d=%5.3f\n", ch, CL_RecvDouble());
    }
    else if (!strncmp(cmd, "ad-ext", 6))
    {
      short ch = atoi(cmd + 6);
      if (BK2_Client(CMD_GET_ADE, "b", ch))
        printf("ADext%d=%+5.3f\n", ch, CL_RecvDouble());
    }
    else if (!_stricmp(cmd, "pedals"))
    {
      if (BK2_Client(CMD_GET_KX))
      {
        unsigned long kx = CL_RecvLong();
        while (kx)
        {
          if ((kx & 0xff) == 0x81) printf("STOP\n");
          if ((kx & 0xff) == 0x82) printf("W-GUARD\n");
          if ((kx & 0xff) == 0x83) printf("PEDAL-DN\n");
          if ((kx & 0xff) == 0x84) printf("PEDAL-UP\n");
          kx >>= 8;
        }
      }
    }
    else if (!_stricmp(cmd, "piezo-test"))
    {
      if (BK2_Client(CMD_TEST, "b", 5))
      {
        int ch, i;
        while (BK2_Client(CMD_GET_TS) && ((CL_RecvShort() >> 8) == 1)) // busy
          BK2_Client(CMD_SLEEPB, "s", 500);
        for (ch = 1; ch <= 2; ch++)
          for (i = 0; i < 6; i++)
            if (BK2_Client(CMD_GET_TAIC, "bb", ch, i+1))
            {
              double x = CL_RecvDouble();
              printf("C%d-%d=%5.3f\n", ch, i, x);
            }
      }
    }
    else if (!_stricmp(cmd, "ass-test"))
    {
      if (BK2_Client(CMD_TEST, "b", 7))
      {
        short ts = 0;
        while (BK2_Client(CMD_GET_TS) && (((ts = CL_RecvShort()) >> 8) == 1)) // busy
          BK2_Client(CMD_SLEEPB, "s", 500);
        printf("%04X %s\n", ts, (ts == 0x0200) ? "OK" : "Err");
      }
    }
    else if (!_stricmp(cmd, "c1"))
    {
      if (BK2_Client(CMD_GET_C1))
      {
        double mod = CL_RecvDouble();
        double arg = CL_RecvDouble();
        printf("C1=%.3f @ %.1f\n", mod, TO_DEG(arg));
        printf("As C103 -> %f\n", TO_DEG(NormPI(arg - PI / 2.0)));
      }
    }
    else if (!_stricmp(cmd, "c2"))
    {
      if (BK2_Client(CMD_GET_C2))
      {
        double mod = CL_RecvDouble();
        double arg = CL_RecvDouble();
        printf("C2=%.3f @ %.1f\n", mod, TO_DEG(arg));
        printf("As C103 -> %f\n", TO_DEG(NormPI(arg - PI / 2.0)));
      }
    }
    else if (!_stricmp(cmd, "errors"))
    {
      int i;
      for (i = 0; i < 10; i++)
      {
        if (BK2_Client(CMD_GET_ERROR, "b", i))
        {
          int e = CL_RecvShort();
          short c = CL_RecvShort();
          short m = CL_RecvShort();
          if (!e) break;
          e |= (m << 16);
          printf("Err.%d: %06X x %d\n", i+1, e, c);
        }
      }
    }
    else if (!_stricmp(cmd, "counts"))
    {
      if (BK2_Client(CMD_PO_GET_LONG, "s", 2))
        printf("CNT:%ld\n", CL_RecvLong());
      if (BK2_Client(CMD_PO_GET_LONG, "s", 3))
        printf("COK:%ld\n", CL_RecvLong());
      if (BK2_Client(CMD_PO_GET_LONG, "s", 4))
        printf("CPC:%ld\n", CL_RecvLong());
      if (BK2_Client(CMD_PO_GET_LONG, "s", 15))
        printf("COP:%ld\n", CL_RecvLong());
      if (BK2_Client(CMD_PO_GET_LONG, "s", 16))
      {
        long pot = CL_RecvLong();
        printf("POT:%02ld:%02ld:%02ld\n", pot/3600, (pot/60)%60, pot%60);
      }
    }
    else if (!_stricmp(cmd, "status"))
    {
      if (BK2_Client(CMD_GET_KEYB))
        printf("Keyb=%d\n", CL_RecvShort());
      if (BK2_Client(CMD_GET_DISP))
        printf("Disp=%d\n", CL_RecvShort());
      if (BK2_Client(CMD_GET_PO_STATUS))
        printf("POSt=%d\n", CL_RecvByte());
      if (BK2_Client(CMD_PO_USAGE))
        printf("POUs=%d\n", CL_RecvShort());
    }
    else if (!_stricmp(cmd, "tudulu"))
    {
      BK2_Client(CMD_BEEP, "sss", 587, 50, 200);
      BK2_Client(CMD_SLEEPB, "s", 240);
      BK2_Client(CMD_BEEP, "sss", 698, 50, 200);
      BK2_Client(CMD_SLEEPB, "s", 240);
      BK2_Client(CMD_BEEP, "sss", 880, 50, 200);
      BK2_Client(CMD_SLEEPB, "s", 240);
    }
    else if (!_stricmp(cmd, "lpkit"))
    {
      BK2_Client(CMD_LASER_KIT, "xx", 286.0, 344.0);
      BK2_Client(CMD_SLEEPB, "s", 2000);
      BK2_Client(CMD_LASER_KIT, "xx", 160.0, 386.0);
      BK2_Client(CMD_SLEEPB, "s", 2000);
      BK2_Client(CMD_LASER_KIT, "xx",   0.0,   0.0);
    }
    else if (!_stricmp(cmd, "kloop"))
    {
      long old_kx = -1;
      long old_kc = -1;
      do
      {
        long kx = BK2_Client(CMD_GET_KX) ? CL_RecvLong() : -1;
        long kc = BK2_Client(CMD_GET_KC) ? CL_RecvLong() : -1;
        if (kx != old_kx || kc != old_kc)
        {
          printf("KX=%08lX KC=%08lX\n", kx, kc);
          old_kx = kx;
          old_kc = kc;
        }
      }
      while ((old_kx & 0xFF) != 0x81); // STOP key
    }
    else if (!_stricmp(cmd, "bearingtest"))
    {
      unsigned t = msec_timer();
      short dps = 0;
      short cnt = 0;
      BK2_Client(CMD_START_DS);
      while (cnt < 50) // 5 seconds
      {
        unsigned t1 = msec_timer();
        if ((t1 - t) >= 100) // every 0.1 sec
        {
          cnt++;
          t = t1;
          if (BK2_Client(CMD_GET_DWV))
            dps = CL_RecvShort();
          printf("RPM=%d\r", dps/6);
        }
      }
      BK2_Client(CMD_ABORT);
      cnt = 0;
      do
      {
        unsigned t1 = msec_timer();
        if ((t1 - t) >= 100) // every 0.1 sec
        {
          cnt++;
          t = t1;
          if (BK2_Client(CMD_GET_DWV))
            dps = CL_RecvShort();
          printf("%5.1f\t%5.1f\n", (double)cnt*0.1, (double)dps/6.0);
        }
      }
      while (dps > 0);
    }
    else if (!_stricmp(cmd, "encodertest"))
    {
      unsigned t = msec_timer();
      BK2_Client(CMD_TEST, "b", 8); // TEST_IE
      while ((msec_timer() - t) < 2000) // wait 2 sec
      {
        short ts = 0;
        if (BK2_Client(CMD_GET_TS))
          ts = CL_RecvShort();
        if ((ts >> 8) > 1) // ready or error (!busy)
          break;
      }
      BK2_Client(CMD_TEST, "b", 0); // TEST_STOP
      memset(ie, 0, sizeof(ie));
      for (t = 0; t < 512; t++)
        if (BK2_Client(CMD_GET_IESTAT, "b", 99))
          ie[t] = CL_RecvLong();
      ie_test();
    }
    else if (!_stricmp(cmd, "C54"))
    {
      unsigned t = msec_timer();
      BK2_Client(CMD_START_DSNSC);
      while ((msec_timer() - t) < 5000) // wait 5 sec
      {
        short dps = 0;
        if (BK2_Client(CMD_GET_DWV))
          dps = CL_RecvShort();
        if (dps > 900) // 150 rpm
          break;
      }
      BK2_Client(CMD_START_DSNSC);
    }
    else if (!strncmp(cmd, "po", 2))
    {
      po_cmd(cmd+2); // PO#[=x|-PO#]
    }
    else if (!strncmp(cmd, "SN", 2))
    {
      char sn [32];
      memset(sn, 0, sizeof(sn));
      BYTE eep = (cmd[2] > '0') ? 1 : 0;
      if (cmd[3] == '=')
      {
        BK2_Client(CMD_SET_SN, "bz", eep, &cmd[4]);
      }
      if (BK2_Client(CMD_GET_SN, "b", eep))
        CL_RecvString(sn, sizeof(sn)-1);
      if(isascii(sn[0]))
      {
        printf("SN%d=%s\n", eep, sn);
        printf("SN%d=", eep);
        for(int i=0; i<sizeof(sn); i++)
            printf(" %02x", sn[i]);
        printf("\n");
      }
      else
        printf("SN%d=<value NOT set>\n", eep);
    }
    else if (!strncmp(cmd, "lift-", 5))
    {
      BYTE b = 0;
      if (!strncmp(cmd+5, "up", 2)) b = 8;
      if (!strncmp(cmd+5, "dis", 3)) b = 10;
      if (!strncmp(cmd+5, "ena", 3)) b = 11;
      if (b) BK2_Client(CMD_LIFT, "b", b);
    }
    else if (!strncmp(cmd, "rear-", 5))
    {
      BYTE b = 0;
      if (!strncmp(cmd+5, "ena", 3)) b = 1;
      if (!strncmp(cmd+5, "dis", 3)) b = 2;
      if (b) BK2_Client(CMD_REAR_SCANNER, "b", b);
    }
    else if (!strncmp(cmd, "run@", 4))
    {
      short dps = 6 * atoi(cmd + 4);
      BK2_Client(CMD_SET_DWVOR, "s", dps);
      BK2_Client(CMD_START_DSVOR);
    }
    else if (!strncmp(cmd, "ccode", 5)) // CCode used
    {
#define CCODE_ARRAY_SIZE 5
        char i;
        unsigned char arr[CCODE_ARRAY_SIZE];
        if(BK2_Client(CMD_GET_CCODE))
        {
            for(i=0; i<CCODE_ARRAY_SIZE; i++)
                arr[i] = CL_RecvByte();
            if(arr[0] == 0)
                printf("CCode: nothing available\n");
            else
            {
                printf("CCode: ");
                for(i=0; i<CCODE_ARRAY_SIZE; i++)
                {
                    if(arr[i] != 0)
                        printf("C%d; ", arr[i]-1);
                    else
                        break;
                }
                printf("\n");
            }
        }
        else
            printf("Command NOT recognized!\n");
    }
    else if (!strncmp(cmd, "setbal", 6)) // set balancing event
    {
	printf("Setting a fake Balancing event for testing purpose!\n");
        BALANCING_EVENT balev;
        balev.is_data_valid = 0;
        balev.imbalance.weight_left = 1.09;
        balev.imbalance.weight_right = 2.23;
        balev.imbalance.weight_static = 3.05;
        balev.imbalance.weight_unit_type = 3;
        balev.wheel.offset = 5.6;
        balev.wheel.diameter = 6.7;
        balev.wheel.width = 7.8;
        balev.wheel.aspect_ratio = 8.9;
        balev.wheel.length_unit_type = 21;
        balev.wheel.vehicle_type = 23;
        balev.alu_mode = 13;
        balev.balancing_progr=55;
        balev.flange_offset=14.05;

        BK2_Client(CMD_SET_BALANCING_EVENT, "bxxxbxxxxbbbbx",
              balev.is_data_valid,
              balev.imbalance.weight_left,
              balev.imbalance.weight_right,
              balev.imbalance.weight_static,
              balev.imbalance.weight_unit_type,
              balev.wheel.offset,
              balev.wheel.diameter,
              balev.wheel.width,
              balev.wheel.aspect_ratio,
              balev.wheel.length_unit_type,
              balev.wheel.vehicle_type,
              balev.alu_mode,
              balev.balancing_progr,
              balev.flange_offset
        );
    }
    else if (!strncmp(cmd, "getbal", 6)) // get balancing event
    {
        if(BK2_Client(CMD_GET_BALANCING_EVENT))
        {
            BALANCING_EVENT balev;
            memset(&balev, 0, sizeof(BALANCING_EVENT));

            balev.is_data_valid = CL_RecvByte();
            balev.imbalance.weight_left = CL_RecvDouble();
            balev.imbalance.weight_right = CL_RecvDouble();
            balev.imbalance.weight_static = CL_RecvDouble();
            balev.imbalance.weight_unit_type = CL_RecvByte();
            balev.wheel.offset = CL_RecvDouble();
            balev.wheel.diameter = CL_RecvDouble();
            balev.wheel.width = CL_RecvDouble();
            balev.wheel.aspect_ratio = CL_RecvDouble();
            balev.wheel.length_unit_type = CL_RecvByte();
            balev.wheel.vehicle_type = CL_RecvByte();
            balev.alu_mode = CL_RecvByte();
            balev.balancing_progr = CL_RecvByte();
            balev.flange_offset = CL_RecvDouble();

            printf("Balancing event:\n");
            printf("is_data_valid:      %d\n", balev.is_data_valid);
            printf("weight_left:        %.03f\n", balev.imbalance.weight_left);
            printf("weight_right:       %.03f\n", balev.imbalance.weight_right);
            printf("weight_static:      %.03f\n", balev.imbalance.weight_static);
            printf("weight_unit_type:   %d\n", balev.imbalance.weight_unit_type);
            printf("offset:             %.03f\n", balev.wheel.offset);
            printf("diameter:           %.03f\n", balev.wheel.diameter);
            printf("width:              %.03f\n", balev.wheel.width);
            printf("aspect_ratio:       %.03f\n", balev.wheel.aspect_ratio);
            printf("length_unit_type:   %d\n", balev.wheel.length_unit_type);
            printf("vehicle_type:       %d\n", balev.wheel.vehicle_type);
            printf("alu_mode:           %d\n", balev.alu_mode);
            printf("balancing_progr:    %d\n", balev.balancing_progr);
            printf("flange_offset:      %.03f\n", balev.flange_offset);
        }
    }
    else if (!strncmp(cmd, "event_type", 10)) // get the type of async event available
    {
        unsigned char async_event;
        if(BK2_Client(CMD_GET_ASYNC_EVT_TYPE))
        {
            async_event = CL_RecvByte();
            if(async_event == ASYNC_EVT_NOTHING)
            {
                printf("No async event available\n");
            }
            else
            {
                printf("Available async event (%d): ", async_event);
                if(async_event & ASYNC_EVT_BAL)
                    printf(" ASYNC_EVT_BAL ");
                if(async_event & ASYNC_EVT_CCODE)
                    printf(" ASYNC_EVT_CCODE ");
                printf("\n");
            }
        }
    }
    else if (!strncmp(cmd, "progr_reset", 11))
    {
        if(BK2_Client(CMD_RESET_BALANCING_PROGR))
            printf("Reset balancing progressive counter OK!\n");
        else
            printf("Reset balancing progressive counter FAILED!\n");
    }
    else if (!strncmp(cmd, "machine_enable", strlen("machine_enable")))
    {
        unsigned char machine_enable = 0;
        if(argc > 1)
        {
            machine_enable = (unsigned short)atoi(argv[1]);
            printf("Setting machine_enable flag to %u\n", machine_enable);
            BK2_Client(CMD_SET_MACHINE_ENABLE, "b", machine_enable);
        }
        BK2_Client(CMD_GET_MACHINE_ENABLE);
        machine_enable = CL_RecvByte();
        printf("-> machine_enable flag is %d\n", machine_enable);
    }
    else if (!strncmp(cmd, "feed", strlen("feed")))
    {
        unsigned char feed = 0;
        if(argc > 1)
        {
            feed = (unsigned char)atoi(argv[1]);
            printf("Setting IoT feed to %u\n", feed);
            BK2_Client(CMD_SET_IOT_MODULE_FEED, "b", feed);
        }
        BK2_Client(CMD_GET_IOT_MODULE_FEED);
        feed = CL_RecvByte();
        printf("-> feed flag value is %d\n", feed);
    }
    else if (!strncmp(cmd, "force_enable", strlen("force_enable")))
    {
        unsigned char force_enable = 0;
        if(argc > 1)
        {
            force_enable = (unsigned short)atoi(argv[1]);
            printf("Setting force_machine_enable flag to %u\n", force_enable);
            BK2_Client(CMD_SET_FORCE_MACHINE_ENABLE, "b", force_enable);
        }
        BK2_Client(CMD_GET_FORCE_MACHINE_ENABLE);
        force_enable = CL_RecvByte();
        printf("-> force_machine_enable flag is %d\n", force_enable);
    }
    else if (!strncmp(cmd, "seterr", strlen("seterr")))
    {
        if(argc > 1)
        {
            unsigned short err_code = (unsigned short)atoi(argv[1]);
            BK2_Client(CMD_SET_CUSTOM_ERROR_CONDITION, "s", err_code);
        }
    }
    else if (!strncmp(cmd, "geterr", strlen("geterr")))
    {
        unsigned char async_event;
        do
        {
            BK2_Client(CMD_GET_ASYNC_EVT_TYPE);
            async_event = CL_RecvByte();
            if(async_event & ASYNC_EVT_UIERR)
            {
                UIERR_EVENT uierr_event;
                BK2_Client(CMD_GET_UIERR);
                uierr_event.uierr_code = CL_RecvLong();
                uierr_event.uierr_type = CL_RecvByte();
                uierr_event.is_data_valid = CL_RecvByte();
                if(uierr_event.is_data_valid)
                {
                    if(uierr_event.uierr_type < 2)
                        printf("Error code: 0x%06x; error type: %d\n", uierr_event.uierr_code, uierr_event.uierr_type);
                    else
                        printf("Error code: %lu; error type: %d\n", uierr_event.uierr_code, uierr_event.uierr_type);
                }
            }
        }
        while(async_event & ASYNC_EVT_UIERR);
    }
    else if (!_stricmp(cmd, "BIN") && argc > 1) // Firmware upgrade
    {
      BK2_Update(argv[1]);
      goto exit_without_logout;
    }
  }
  BK2_Client(CMD_LOGOUT_RO, "-1");
exit_without_logout:
  BK2_Close();
  return 0;
}

// -----------------------------------------------------------------------------
