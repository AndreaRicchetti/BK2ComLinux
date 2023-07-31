#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define UI32 unsigned int
#define BYTE unsigned char

const char Magic [16] = {"SnapOn Correggio"}; // on-Mmc card and applcation magic flag.

#if 0
HEADER: FF(256) + Magic(16) + FF(48) + CheckSum(4) + FileSize(4) + ID(4) + FF(180)
BOBY:   File(size)
FOOTER: FF(1024*1024-size-512)
#endif

UI32 CheckSum ( BYTE *Ptr, UI32 Len )
{
  UI32 sum = 0;
  while (Len--)
    sum += *Ptr++;
  return sum;
}

int load_file(const char* file_name, unsigned char* buffer, size_t buffer_size)
{
  FILE *f;
  
  memset((void*)buffer, 0, buffer_size);
  f = fopen(file_name, "r");
  if(!f)
    return 0;

  long n = 0;
  long ret = 0;
  fseek(f, 0L, SEEK_END);
  n = ftell(f);
  if(n > buffer_size)
    return -1;
  fseek(f, 0L, SEEK_SET);
  ret = fread(buffer, buffer_size, 1, f);
  if(feof(f))
    ret = n;
  fclose(f);
  
  return ret;
}

int save_file(const char* out_file_name, const unsigned char* buffer, size_t buffer_size, int do_append)
{
  int ret = 0;
  FILE *f;
  if(do_append)
    f = fopen(out_file_name, "a+");
  else
   f = fopen(out_file_name, "wb");
  if(!f)
    return 1;
  
  ret = fwrite((const void*)buffer, 1, buffer_size, f);
  fclose(f);

  if(ret != buffer_size)
    return 2;

  return 0;
}

int main(int argc, char** argv)
{
  int ret = 0;
  const size_t buffer_size = 1024*1024;
  unsigned char in_buffer[buffer_size];
  unsigned char out_buffer[buffer_size];
  char web_file_name[512];
  int file_size = 0;
  UI32 check_sum = 0;
  UI32 kID = 0;

  if(argc < 4)
  {
    printf("syntax: %s <out-bin-file> <key-ID> <in-bin-file>\n", argv[0]);
    return 1;
  }

  file_size = load_file(argv[3], in_buffer, buffer_size);
  if(file_size <= 0)
  {
    printf("Failed to load file, is it bigger than 1MB?\n");
    return 2;
  }

  /************************ STUFFING ALL FILE ******************************/
  memset((void*)out_buffer, 0xff, buffer_size);

  /************************ BUILDING HEADER ******************************/
  check_sum = CheckSum(in_buffer, file_size);
  kID = atoi(argv[2]);
  memcpy((void*)out_buffer+256, (const void*)Magic, 16); // adding Magic
  memcpy((void*)out_buffer+320, (const void*)&check_sum, 4); // adding CheckSum
  memcpy((void*)out_buffer+324, (const void*)&file_size, 4); // adding file size
  memcpy((void*)out_buffer+328, (const void*)&kID, 4); // adding keyboard ID

  /************************ BUILDING BODY ******************************/
  memcpy((void*)out_buffer+512, (const void*)in_buffer, file_size); // adding bin file

  ret = save_file(argv[1], out_buffer, buffer_size, 1 /* append */);
  if(ret == 0)
    printf("file '%s' appended succesfully!\n", argv[1]);
  
  snprintf(web_file_name, sizeof(web_file_name), "web_%s", argv[1]);
  size_t single_size = ((file_size/512)+2)*512;
  ret = save_file(web_file_name, out_buffer, single_size, 0 /* do not append */);
  if(ret == 0)
    printf("file '%s' written succesfully!\n", web_file_name);

  return ret;
}
