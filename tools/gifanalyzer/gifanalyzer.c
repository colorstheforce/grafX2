/* vim:expandtab:ts=2 sw=2:
*/
/*  Grafx2 - The Ultimate 256-color bitmap paint program
 *  Gif Analyzer tool

    Copyright 2018 Thomas Bernard
    Copyright 2010 Adrien Destugues

    Grafx2 is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; version 2
    of the License.

    Grafx2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Grafx2; if not, see <http://www.gnu.org/licenses/>
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

static void read_image(FILE * theFile);
static void read_palette(FILE * theFile, int count);

int main(int argc, char* argv[])
{
  FILE* theFile;
  uint8_t buffer[256];
  uint16_t w,h;

  if(argc < 2)
  {
    printf("%s file.gif\n",argv[0]);
    exit(-1);
  }

  theFile = fopen(argv[1], "r");
  if (theFile == NULL)
  {
    fprintf(stderr, "Failed to open file '%s'.\n", argv[1]);
    exit(1);
  }

	fread(buffer, 1, 6, theFile);
  buffer[6] = 0;

	printf("Signature: %6s\n", buffer);

	fread(&w,2,1,theFile);
	fread(&h,2,1,theFile);

	printf("Resolution: %dx%d\n",w,h);

  fread(buffer,1,3,theFile);

  int colors = 1 << (((buffer[0] & 0x70)>>4)+1); // 0x70 == 0b01110000
  int color_table_size = 1 << ((buffer[0]&0x7)+1);

  printf("Color palette: 0x%02x\n",buffer[0]&0xFF);
  if (buffer[0] & 0x80) { // 0x80 == 0b10000000
    printf("\tGlobal palette");
    if(buffer[0] & 0x08)  // 0x08 == 0b00001000
      printf(" SORTED");
    printf("\n");
  }
  printf("\tColor count: %d\n", colors);
  printf("\tSize of color table : %d\t", color_table_size);

  printf("Index of background color: %d\n",buffer[1]);
  printf("pixel aspect ratio = %d\n",buffer[2]);
  if(buffer[2] != 0) printf("\t aspect ratio = %d/64\n", 15 + buffer[2]);

  printf("Color palette:\n");
  read_palette(theFile, color_table_size);

  int i = 0;
  do {
    fread(buffer,1,1,theFile);
    printf("%02X ", buffer[0]);
    i++;
  } while (buffer[0] != ',');
  printf("\n");

  if (i > 1)
    printf("Skipped %d meaningless bytes before image descriptor\n",i);

  read_image(theFile);

	fclose(theFile);
}

static void read_image(FILE * theFile)
{
  uint8_t buffer[256];
  uint16_t x,y;
  uint16_t w,h;
  unsigned long total_bytes = 0, chunk_count = 0;
  int color_table_size;

  fread(&x,2,1,theFile);
  fread(&y,2,1,theFile);
  fread(&w,2,1,theFile);
  fread(&h,2,1,theFile);
  fread(buffer,1,1,theFile);

  color_table_size =  1 << ((buffer[0]&0x07)+1);
  printf("Image descriptor\n");
  printf("\tx=%d y=%d w=%d h=%d\n",x,y,w,h);
  printf("\t%sLocal Color Table (%d colors) %sSorted, %s\n",
         (buffer[0]&0x80)?"":"No ", color_table_size,
         (buffer[0]&0x20)?"":"Not ",
         (buffer[0]&0x40)?"Interlaced":"Progressive");

  if (buffer[0]&0x80)
    read_palette(theFile, color_table_size);

  while (!feof(theFile))
  {
    fread(buffer,1,1,theFile);
    if (buffer[0] == 0)
      break;
    fread(buffer+1,1,buffer[0],theFile);
    total_bytes += buffer[0];
    chunk_count++;
  }
  printf("  %lu bytes in %lu chunks\n", total_bytes, chunk_count);
}

static void read_palette(FILE * theFile, int count)
{
  uint8_t buffer[3];
  int i;

  for (i = 0; i < count; i++)
  {
    fread(buffer,1,3,theFile);
    /*printf("\t%d: %u %u %u\t",i,buffer[0], buffer[1], buffer[2]);*/
    printf("   %3d: #%02x%02x%02x",i,buffer[0], buffer[1], buffer[2]);
    if ((i+1)%8 ==0)puts("");
  }
}
