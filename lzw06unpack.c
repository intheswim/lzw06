/* This code is based on Mark Nelson's 1995 book. */

/* MY ORIGINAL 1996 COMMENT: */

/*************************************************/
/*   Программа распаковщика для алгоритма LZW    */
/*   полная очистка словаря при заполнении       */
/*   длина выходных кодов постоянна (12 бит)     */
/*************************************************/

/* TRANSLATION: */

/**************************************************/
/*  LZW decompression program with full           */        
/*  dictionary reset when filled up. Constant     */
/*  12-bit codes in input.                        */
/**************************************************/

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>

#define CLEAR_BYTE      0x10  /* it can be any value between 0x10 and 0xFF */
#define NOT_CODE        (CLEAR_BYTE | (CLEAR_BYTE << 8))

static int16_t GetPrefixChar(int16_t code, const uint16_t * prefix)
{
  while (code >= 256)
  {
    assert (code < HT_MAX_CODE && code >= 0);
    code = prefix[code];
  }

  return code;
}

int Decompress(const char *filename, const char *outfile, int flags)
{
  int i = 0, k, len, bpos = 0;
  int16_t RunCode = 256, OldCode = NOT_CODE, CurPrefix;
  uint16_t code;
  uint8_t *buffer = NULL;
  uint8_t infoBits = 0;
  uint8_t infoFlag = 0;
  uint32_t expectedSize = 0;
  FILE *fp = NULL;
  FILE *fout = NULL; 
  uint16_t StackCount = 0;
  char label[4] = { 0 };
  uint8_t version = 255;
  uint16_t stack [BUFFLEN];
  uint16_t suffix [HT_MAX_CODE];
  uint8_t outline [BUFFLEN];
  uint16_t prefix [HT_MAX_CODE];

  if (is_big_endian())
  {
    fprintf (stderr, "Not supported on big endian machines.\n");
    return 0;
  }

  if (!(flags & OVERWRITE_FLAG) &&  file_exists(outfile))
  {
    /* file exists and no overwrite flag set */
    fprintf (stderr, "File \'%s\' already exists. Use overwrite flag.\n", outfile);
    return 0;
  }

  fp = fopen(filename, "rb");

  if (NULL == fp)
  {
    fprintf (stderr, "Cannot open file \'%s\'.\n", filename);
    perror (NULL);
    return 0;
  }
  
  if (4 != fread(label, 1, 4, fp))
  {
    printf("Not an LZW file!\n");
    fclose (fp);
    return 0;
  }

  if (memcmp(label, "LZW", 3) != 0)
  {
    printf("Not an LZW file!\n");
    fclose (fp);
    return 0;
  }

  if (1 != fread (&version, 1, 1, fp))
  {
    fprintf(stderr, "Unexpected read error.\n");
    fclose (fp);
    return 0;
  }

  if (version != PACKER_VERSION)
  {
    fprintf(stderr, "Packer/unpacker version mismatch.\n");
    fclose (fp);
    return 0;
  }

  infoBits |= (is_big_endian() ? 1 : 0);
  infoBits |= VARIABLE_WIDTH ? 2 : 0;
  infoBits |= ((MAX_BITS - 8) << 4);

  /* get infoFlags byte: */

  if (1 != fread (&infoFlag, 1, 1, fp))
  {
    fprintf(stderr, "Unexpected read error.\n");
    fclose (fp);
    return 0;
  }

  if (infoBits != infoFlag)
  {
    fprintf(stderr, "Encoding flags mismatch.\n");
    fclose (fp);
    return 0;
  }

  /* get expected output size: */

  expectedSize = 0;
  if (4 != fread (&expectedSize, 1, sizeof(uint32_t), fp))
  {
    fprintf(stderr, "Unexpected read error.\n");
    fclose (fp);
    return 0;
  }

  if (flags & VERBOSE_OUTPUT)
  {
    printf ("expected output size: %ld.\n", (long)expectedSize);
  }

  fout = fopen(outfile, "wb");

  if (NULL == fout)
  {
    fprintf (stderr, "Cannot open file \'%s\'.\n", outfile);
    perror (NULL);
    fclose (fp);
    return 0;
  }

  buffer = (unsigned char *)malloc(OUTLEN);

  if (!buffer)
  {
    perror (NULL);
    fclose(fout);
    fclose(fp);
    cleanup (outfile, flags);
    return 0;
  }

  memset(prefix, CLEAR_BYTE, HT_SIZE);

  memset (suffix, 0, sizeof(suffix)); /* this is just to make static analyzer happy */
  
  while (true)
  {
    len = (int)fread(buffer, 1, OUTLEN, fp);
    for (k = 0; k < len;)
    {
      if (bpos == 0)
      {
        code = *(uint16_t *)(buffer + k) & 0x0FFF; /* assuming little endian */
        bpos = 4;
        k++;
      }
      else
      {
        code = *(uint16_t *)(buffer + k) >> 4;    /* assuming little endian */
        bpos = 0;
        k += 2;
      }

      if (code == EOF_CODE)
      {
        if (i != (int)fwrite (outline, 1, i, fout))
        {
          /* write error */
          free (buffer);
          fclose (fout);
          fclose (fp);
          cleanup (outfile, flags);
          fprintf (stderr, "Write error. Out of disk space?\n");
          return 0;
        }

        free (buffer);
        fflush (fout);
        fclose (fp);

        /* compare expected size with actual size. */

        if (expectedSize != ftell (fout))
        {
          fprintf (stderr, "Expected and actual sizes dont match.\n");
          fclose (fout);
          cleanup (outfile, flags);
          return 0;
        }

        fclose(fout);

        return 1;
      }

      else if (code == HT_CLEAR_CODE)
      {
        memset(prefix, CLEAR_BYTE, HT_SIZE);
        RunCode = 256;
        OldCode = NOT_CODE;
      }
      else
      {
        if (code < 256)
          outline[i++] = (uint8_t)code;
        else
        {
          if (prefix[code] == NOT_CODE)
          {
            CurPrefix = OldCode;
            suffix[RunCode] = GetPrefixChar(OldCode, prefix);
            stack[StackCount++] = suffix[RunCode];
          }
          else
            CurPrefix = code;
          while (CurPrefix > 255)
          {
            stack[StackCount++] = suffix[CurPrefix];
            CurPrefix = prefix[CurPrefix];
          }
          stack[StackCount++] = CurPrefix;
          while (StackCount != 0)
            outline[i++] = (uint8_t)stack[--StackCount];
        }
        if ((OldCode != NOT_CODE))
        {
          prefix[RunCode] = OldCode;
          if (code != RunCode)
            suffix[RunCode] = GetPrefixChar(code, prefix);
          RunCode++;
        }
        OldCode = code;
        if (i == BUFFLEN)
        {
          if (BUFFLEN != fwrite(outline, 1, BUFFLEN, fout))
          {
            /* write error */
            free (buffer);
            fclose (fout);
            fclose (fp);
            cleanup (outfile, flags);
            fprintf (stderr, "Write error. Out of disk space?\n");
            return 0;
          }

          i = 0;
          OldCode = NOT_CODE;
        }
      }
    }
  }
} 
