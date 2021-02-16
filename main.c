/* LZW fixed 12-bit data compression implementation. 
 * Copyright (c) 1996-2021 Yuriy Yakimenko
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/*------------------------------------------------------------*/
/*                                                            */ 
/*  Includes test option (Linux and Windows only).            */
/*  Uses cksum or certUtil to verify results.                 */
/*  Replace with custom cksum on other platforms.             */
/*                                                            */ 
/*------------------------------------------------------------*/

#define _GNU_SOURCE /* for popen */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ONE_KILOBYTE 1024

enum ByteSequence { SEQ_CONSTANT = 0, SEQ_INCREASING, SEQ_RANDOM };

enum ArgOption { PARSE_ERROR = -1, SYNTHETIC_TEST = 0, FLAG_PACK = 1, FLAG_UNPACK = 2, FLAG_TEST = 3 };

struct progArguments
{
    char *inputFile;
    char *outputFile;
    int flags;
};

/*--------------------------------------------------------------------*/

static long fileSize (const char *filename)
{
  long ret = -1;

  FILE *fp = fopen (filename, "rb");

  if (!fp) return -1;

  fseek (fp, 0, SEEK_END);

  ret = ftell (fp);

  fclose (fp);

  return ret;
}

/*--------------------------------------------------------------------*/

static void show_command (const char* cmd) 
{
  char buffer[128];

#if defined(__linux__)
  FILE* pipe = popen(cmd, "r");
#elif defined(_WIN32)
  FILE* pipe = _popen(cmd, "r");
#endif

  if (!pipe) 
  {
    fprintf(stderr, "popen() failed!");
    return;
  }

  while (fgets(buffer, sizeof(buffer), pipe) != NULL) 
  {
    printf ("%s", buffer);    
  } 

#if defined(__linux__)
  pclose(pipe);
#elif defined(_WIN32)
  _pclose(pipe);
#endif 
}

/*--------------------------------------------------------------------*/

static void printSyntax ()
{
  printf ("syntax: lzw06 -(p|u|t) [-v -f -k -t] inputFile outputFile \n");
  printf ("        lzw06 -large [N] \n");
  printf ("\t -p - pack \n");
  printf ("\t -u - unpack \n");
  printf ("\t -v - verbose \n");
  printf ("\t -f - force overwrite; applicable with -u option only \n");
  printf ("\t -k - keep dirty/incomplete output file on failure \n");
  printf ("\t -t - test option; requires only inputFile \n");
  printf ("\t -large - synthetic data test; N is size in 256 Kb units. Default N is 32.\n");
}

/*--------------------------------------------------------------------*/

static enum ArgOption parseArguments (int argc, char *argv[], struct progArguments *params)
{
    int fileNameSet = false;

    int flagPack = 0;
    int flagUnpack = 0;
    int flagForce = 0;
    int flagVerbose = 0;
    int flagKeepDirty = 0;
    int flagTest = 0;

    int ret = 0, i, j;

    char combined_flags[32] = { 0 };

    params->inputFile = NULL;
    params->outputFile = NULL;
    params->flags = 0;


    if (argc == 1)
    {
        return -1;
    }

    for (i = 1; i < argc; i++)
    {
        if (strncmp (argv[i], "-", 1) == 0) 
        {
            if (fileNameSet)
            {
                return -1;
            }

            if ((i == 1) && 0 == strcmp(argv[1], "-large"))
            {
                return SYNTHETIC_TEST; /* large test */
            }

            memset (combined_flags, 0, sizeof (combined_flags));

            strncpy (combined_flags, argv[i], sizeof (combined_flags) - 1);

            for (j = 1; combined_flags[j]; j++)
            {
                char flag = combined_flags[j];

                if (flag == 'p')
                {
                    flagPack = true;
                }
                else if (flag == 'u')
                {
                    flagUnpack = true;
                }
                else if (flag == 'f')
                {
                    flagForce = true;
                }
                else if (flag == 'v')
                {
                    flagVerbose = true;
                }
                else if (flag == 'k')
                {
                    flagKeepDirty = true;
                }
                else if (flag == 't')
                {
                    flagTest = true;
                }
                else 
                {
                    fprintf (stderr, "Unknown flag -%c\n", flag);
                    return -1;
                }
            }
        }
        else /* file names */
        {
            fileNameSet = true;

            if (params->inputFile == NULL)
            {
                params->inputFile = str_dup (argv[i]);
            }
            else if (params->outputFile == NULL)
            {
                params->outputFile = str_dup (argv[i]);
            }
        }
    }

    if (flagTest + flagPack + flagUnpack > 1) /* inconsistent args */
    {
        fprintf (stderr, "Cannot combine -p, -u and -t flags.\n");
        return -1;
    }

    if (flagTest + flagPack + flagUnpack == 0) 
    {
        fprintf (stderr, "No pack, unpack or test flags given.\n");
        return -1;
    }

    if (flagTest)
    {
      if (NULL == params->inputFile)
        return -1;
    }
    else
    {
      if (NULL == params->inputFile || NULL == params->outputFile)
        return -1;
    }

    if (flagForce) params->flags |= OVERWRITE_FLAG;
    if (flagVerbose) params->flags |= VERBOSE_OUTPUT;
    if (flagKeepDirty) params->flags |= KEEP_ON_ERROR;
    
    if (flagTest) ret = FLAG_TEST;
    else if (flagPack) ret = FLAG_PACK;
    else if (flagUnpack) ret = FLAG_UNPACK;

    return ret;
}

static void run_cksum (const char *file)
{
  size_t size = strlen (file) + 1;

  char * command = (char *)malloc (size + 32);
#if defined(__linux__)
  sprintf (command, "cksum %s", file); 
#elif defined(_WIN32)
  sprintf(command, "certutil -hashfile %s", file);
#endif
 
  show_command ( command );

  free (command);
}

/*--------------------------------------------------------------------*/
/* For testing purposes only */
/*--------------------------------------------------------------------*/

static int syntheticDataTest (int kilobytes256, enum ByteSequence option)
{
  const char input [] = "synth.bin";
  const char packed_input[] = "synth.lzw";
  const char unpacked_input[] = "synth.out";
  long orig_size, compressed_size;

  if (true) 
  { 
    FILE *fp = fopen (input, "w+b");

    const int size = ONE_KILOBYTE;

    char buffer[ONE_KILOBYTE];

    int i = 0;

    memset (buffer, 0x0A, sizeof(buffer));

    if (option == SEQ_INCREASING)
    {
      for (i = 0; i < size; i++) buffer[i] = (char)(i & 0xFF);
    }
    else if (option == SEQ_RANDOM)
    {
      for (i = 0; i < size; i++) buffer[i] = (char)(rand() & 0xFF);
    }

    for (i = 0; i < 256 * kilobytes256; i++)
    {
        fwrite (buffer, 1, sizeof(buffer), fp);
    }

    fclose (fp);
  } 

  if (0 == Compress(input, packed_input, VERBOSE_OUTPUT))
  {
    printf ("Synthetic input compression failed.\n");
    return EXIT_FAILURE;
  }

  printf ("Sythetic input compression successful.\n");

  if (0 == Decompress(packed_input, unpacked_input, VERBOSE_OUTPUT | OVERWRITE_FLAG))
  {
    printf ("Synthetic input decompression failed.\n");
    return EXIT_FAILURE;
  }

  printf ("Synthetic input decompression successful.\n");

  orig_size = fileSize (input);
  compressed_size = fileSize (packed_input);

  printf ("Compression ratio %.2f%%\n", 100.0 * compressed_size / orig_size);

  /* compare two files by running cksum */

  run_cksum (input);
  run_cksum (unpacked_input);

  remove (unpacked_input);
  remove (packed_input);
  remove (input);

  return EXIT_SUCCESS;
}

/*--------------------------------------------------------------------*/

static void freeFilenames (struct progArguments * args)
{
  free (args->inputFile);
  free (args->outputFile);
}

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{
  struct progArguments params;

  int ret = EXIT_FAILURE;

  const char temp_name [] = "lzw06_temp.lzw";
  const char out_name [] = "lzw06_out.bin";
  long orig_size = 0, compressed_size = 0;

  enum ArgOption option = parseArguments (argc, argv, &params);

  if (option == PARSE_ERROR)
  {
    printSyntax ();
  }

  else if (option == SYNTHETIC_TEST) /* synthetic test */
  {
    int kb256 = 32;

    if (argc == 3)
    {
      int k = atoi(argv[2]);

      if (k > 0)
      {
        kb256 = k;
      }
    }

    return syntheticDataTest(kb256, SEQ_CONSTANT);
  }

  else if (option == FLAG_PACK)
  {
    if (0 == Compress(params.inputFile, params.outputFile, params.flags))
    {
      printf ("Compression failed.\n");
      ret = EXIT_FAILURE;
    }
    else 
    {
      printf ("Compression successful.\n");

      if (params.flags & VERBOSE_OUTPUT)
      {
        orig_size = fileSize (params.inputFile);
        compressed_size = fileSize (temp_name);

        printf ("Compression ratio %.2f%%\n", 100.0 * compressed_size / orig_size);
      }

      ret = EXIT_SUCCESS;
    }
  }
  else if (option == FLAG_UNPACK)
  {
    if (0 == Decompress(params.inputFile, params.outputFile, params.flags))
    {
      printf ("Decompression failed.\n");
      ret = EXIT_FAILURE;
    }
    else 
    {
      printf ("Decompresson successfull.\n");
      ret = EXIT_SUCCESS;
    }
  }
  else if (option == FLAG_TEST)
  {
    if (0 == Compress(params.inputFile, temp_name, params.flags ))
    {
      printf ("Compression failed.\n");
      freeFilenames (&params);
      return EXIT_FAILURE;
    }
    else 
    {
      printf ("Compression successful.\n");
    }

    if (0 == Decompress(temp_name, out_name, params.flags | OVERWRITE_FLAG))
    {
      printf ("Decompression failed.\n");
      freeFilenames (&params);
      return EXIT_FAILURE;
    }
    else 
    {
      printf ("Decompression successful.\n");
    }

    orig_size = fileSize (params.inputFile);
    compressed_size = fileSize (temp_name);

    printf ("Compression ratio %.2f%%\n", 100.0 * compressed_size / orig_size);

    /* compare 2 files. */

    run_cksum (params.inputFile);
    run_cksum (out_name);
  }

  freeFilenames (&params);

  return (ret);
}
