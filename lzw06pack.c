/* This code is based on Mark Nelson's 1995 book. */

/* MY ORIGINAL 1996 COMMENT:  */

/*************************************************/
/*   Программа упаковщика для алгоритма LZW      */
/*   полная очистка словаря при заполнении       */
/*   длина выходных кодов постоянна (12 бит)     */
/*************************************************/

/* TRANSLATION: */

/**************************************************/
/*  LZW compression program with full dictionary  */
/*  reset when filled up. Constant 12-bit codes   */ 
/*  in output.                                    */   
/**************************************************/

#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdint.h>

#define HT_GET_KEY(l)   (l >> 12)
#define HT_GET_CODE(l)  (l & 0x0FFF)
#define HT_PUT_KEY(l)   (l << 12)
#define HT_PUT_CODE(l)  (l & 0x0FFF)

struct packHelper {
  uint32_t *table ;
  uint8_t outline[OUTLEN];

  FILE *fp ;
  FILE *fout ;

  int bpos;
  size_t len;
} ;

static void initializeHelper (struct packHelper *ph)
{
  ph->table = NULL;
  ph->fp = NULL;
  ph->fout = NULL;
  ph->bpos = 0;
  ph->len = 0;
  memset(ph->outline, 0, OUTLEN);
}
/*------------------------------------*/
static void ClearHashTable(struct packHelper *ph)
{
  memset(ph->table, 0xFF, HT_SIZE * sizeof(uint32_t));
}
/*-----------------------------------*/
static void DeleteHashTable (struct packHelper *ph)
{
  free (ph->table);
  ph->table = NULL;
}
/*-----------------------------------*/
static int OutCode (const int16_t code, struct packHelper *ph)
{
  if (ph->bpos == 0)
  {
    *(uint16_t *)(ph->outline + ph->len++) = code; /* assuming little endian */

    ph->bpos = 4;

    if (code == EOF_CODE)
    {
      size_t written = fwrite(ph->outline, 1, ph->len + 1, ph->fout);
      if (written != ph->len + 1) 
      {
        fprintf (stderr, "Write error. Out of disk space? \n");
        return 0;
      }
    }
  }
  else
  {
    *(uint16_t *)(ph->outline + ph->len) |= (code << 4); /* assuming little endian */

    ph->len += 2;
    
    ph->bpos = 0;

    if (ph->len == OUTLEN || code == EOF_CODE)
    {
      size_t written = fwrite(ph->outline, 1, ph->len, ph->fout);
      
      if (written != ph->len) 
      {
        fprintf (stderr, "Write error. Out of disk space? \n");
        return 0;
      }

      ph->len = 0;

      memset(ph->outline, 0, OUTLEN);
    }
  }

  return 1;
}
/*-----------------------------------*/
static int InitHashTable(struct packHelper *ph)
{
  ph->table = (uint32_t *)malloc(HT_SIZE * sizeof(uint32_t));

  if (ph->table == NULL)
    return 0;
  
  ClearHashTable(ph);
  
  return 1;
}
/*------------------------------------*/
static int16_t KeyItem (const uint16_t Item)
{
  return ((Item >> 12) ^ Item) & HT_KEY_MASK;
}
/*-------------------------------------*/
static void InsertHashTable (const uint32_t Key, int16_t Code, struct packHelper *ph)
{
  int HKey = KeyItem(Key);

  while (HT_GET_KEY(ph->table[HKey]) != 0xFFFFFL)
    HKey = (HKey + 1) & HT_KEY_MASK;
  
  ph->table[HKey] = HT_PUT_KEY(Key) | HT_PUT_CODE(Code);
}
/*--------------------------------------------*/
static int ExistHashTable (const uint32_t Key, struct packHelper *ph)
{
  int16_t HKey = KeyItem(Key);
  uint32_t HTKey;

  while ((HTKey = HT_GET_KEY(ph->table[HKey])) != 0xFFFFFL)
  {
    if (Key == HTKey)
      return HT_GET_CODE(ph->table[HKey]);
  
    HKey = (HKey + 1) & HT_KEY_MASK;
  }
  
  return -1;
}
/*-------------------------------------------------*/
int Compress(const char *filename, const char *outfile, int flags)
{
  uint8_t *buffer;
  int16_t CurCode, RunCode = 256, NewCode;
  uint32_t NewKey;
  int len, i;
  const char label[4] = "LZW";
  const uint8_t version = PACKER_VERSION;
  uint8_t infoBits = 0;
  uint32_t inputSize = 0, outputSize = 0;
  int compress_ok = true;
  struct packHelper ph;

  if (is_big_endian())
  {
    fprintf (stderr, "Not supported on big endian machines.\n");
    return 0;
  }

  initializeHelper (&ph);

  ph.fp = fopen(filename, "rb");

  if (NULL == ph.fp)
  {
    fprintf (stderr, "Cannot open input file \'%s\'.\n", filename);
    perror (NULL);
    return 0;
  }

  ph.fout = fopen (outfile, "wb");

  if (NULL == ph.fout)
  {
    fprintf (stderr, "Cannot open output file \'%s\'.\n", outfile);
    perror (NULL);
    fclose (ph.fp);
    return 0;
  }

  if (!InitHashTable(&ph))
  {
    perror(NULL);
    fclose (ph.fp);
    fclose (ph.fout);
    cleanup (outfile, flags);
    return 0;
  }

  buffer = (unsigned char *)malloc(BUFFLEN);
  
  if (!buffer)
  {
    fclose (ph.fp);
    fclose (ph.fout);
    perror (NULL);
    DeleteHashTable(&ph);
    cleanup (outfile, flags);
    return 0;
  }

  fwrite(label, 1, 4, ph.fout);

  fwrite (&version, 1, 1, ph.fout);

  infoBits |= (is_big_endian() ? 1 : 0);
  infoBits |= VARIABLE_WIDTH ? 2 : 0;

  /* leaving 2 bits reserved. */
  infoBits |= ((MAX_BITS - 8) << 4); /* we use left 4 bits for MAX_BITS information; can be between 8 and 23. */

  fwrite (&infoBits, 1, 1, ph.fout);

  /* write size of input file. */
  fseek (ph.fp, 0, SEEK_END);
  inputSize = ftell (ph.fp);
  fseek (ph.fp, 0, SEEK_SET);

  fwrite (&inputSize, 1, sizeof(uint32_t), ph.fout);

  while (compress_ok)
  {
    len = (int)fread(buffer, 1, BUFFLEN, ph.fp);

    if (len == 0)
      break;
    
    CurCode = *buffer;

    for (i = 1; i < len; i++)
    {
      NewKey = (((uint32_t)CurCode) << 8) + buffer[i];

      if ((NewCode = ExistHashTable(NewKey, &ph)) >= 0)
      {
        CurCode = NewCode;
      }
      else
      {
        if (!OutCode (CurCode, &ph))
        {
          compress_ok = false;
          break;
        }

        CurCode = buffer[i];
        if (RunCode == HT_CLEAR_CODE)
        {
          ClearHashTable(&ph);
          RunCode = 256;
          if (!OutCode (HT_CLEAR_CODE, &ph))
          {
            compress_ok = false;
            break;
          }
        }
        else
        {
          InsertHashTable (NewKey, RunCode++, &ph);
        }
      }
    }

    if (!OutCode (CurCode, &ph))
      compress_ok = false;
  }

  if (compress_ok)
    OutCode (EOF_CODE, &ph);

  DeleteHashTable(&ph);

  free(buffer);

  outputSize = ftell (ph.fout);

  fclose(ph.fp);
  fclose (ph.fout);

  if (!compress_ok)
  {
    cleanup (outfile, flags);
  }

  if (compress_ok && (VERBOSE_OUTPUT & flags))
  {
    printf ("Compression ratio %.2f%%\n", 100.0 * outputSize / inputSize );
  }

  return compress_ok ? 1 : 0;
}
