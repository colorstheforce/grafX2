/* vim:expandtab:ts=2 sw=2:
*/
/*  Grafx2 - The Ultimate 256-color bitmap paint program

    Copyright 2018-2019 Thomas Bernard
    Copyright 2011 Pawel Góralski
    Copyright 2009 Petter Lindquist
    Copyright 2008 Yves Rizoud
    Copyright 2008 Franck Charlet
    Copyright 2007-2011 Adrien Destugues
    Copyright 1996-2001 Sunset Design (Guillaume Dorme & Karl Maritaud)

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

///@file msxformats.c
/// Formats for the MSX machines

#include <stdlib.h>
#include <string.h>
#include "fileformats.h"
#include "loadsavefuncs.h"
#include "io.h"
#include "misc.h"
#include "gfx2log.h"
#include "gfx2mem.h"

/**
 * @defgroup msx MSX picture formats
 * @ingroup loadsaveformats
 *
 * MODE2 : 256x192
 * Support for the MSX pictures
 * - SC2 "BSAVE" files
 *
 * @{
 */

/**
 * Check if the file is in the MSX BSAVE format. Reads 7 bytes.
 *
 * @param file
 * @param begin load address
 * @param end end address
 * @param exec execution address for BLOAD ,R
 * @return 0 if the file is not in BSAVE format
 */
static int Check_MSX_BSAVE_format(FILE * file, word * begin, word * end, word * exec)
{
  byte type;
  word b, e, r;
  if (!Read_byte(file, &type) || !Read_word_le(file, &b) || !Read_word_le(file, &e) || !Read_word_le(file, &r))
    return 0;
  if (type != 0xfe)
    return 0;
  if (b > e)
    return 0;
  if (begin != NULL)
    *begin = b;
  if (end != NULL)
    *end = e;
  if (exec != NULL)
    *exec = r;
  return 1;
}

void Test_MSX(T_IO_Context * context, FILE * file)
{
  word begin, end, exec;
  (void)context;
  if (Check_MSX_BSAVE_format(file, &begin, &end, &exec))
  {
    GFX2_Log(GFX2_DEBUG, "MSX : &H%04X-&H%04X,&H%04X\n", begin, end, exec);
    if (begin < 0x4000 && end <= 0x4000)
      File_error = 0;
  }
}

/**
 * Load a MSX Screen 2 picture
 *
 * TMS9918 Mode II VRAM layout :
 * - 0000-17FF : Pattern table
 * - 1800-1AFF : Name table
 * - 1B00-1B7F : Sprite attributes
 * - 1B80-1FFF : UNUSED (1B80-1BAF = Palette)
 * - 2000-37FF : Color table
 * - 3800-3FFF : Sprite patterns
 */
void Load_MSX(T_IO_Context * context)
{
  unsigned long file_size;
  FILE * file;
  word begin, end, exec;
  byte vram[16*1024];
  int row, col, sprite;
  word addr;

  File_error = 1;
  file = Open_file_read(context);
  if (!Check_MSX_BSAVE_format(file, &begin, &end, &exec))
  {
    fclose(file);
    return;
  }
  if (begin >= 0x4000 || end > 0x4000)
  {
    fclose(file);
    return;
  }
  memset(vram, 0, sizeof(vram));
  if (!Read_bytes(file, vram + begin, end - begin))
  {
    fclose(file);
    return;
  }
  file_size = File_length_file(file);
  fclose(file);

  File_error = 0;
  // TODO: load MSX palette
  context->Transparent_color = 0; // Set color 0 as transparent
  Pre_load(context, 256, 192, file_size, FORMAT_MSX, PIXEL_SIMPLE, 4);
  for (row = 0; row < 24; row++)
  {
    for (col = 0; col < 32; col++)
    {
      int x, y;
      byte pattern = vram[0x1800 + row*32 + col];
      addr = ((row & 0x18) << 8) + (pattern << 3);
      for (y = 0; y < 8; y++, addr++)
      {
        byte bits = vram[addr];
        byte colors = vram[0x2000 + addr];
        for (x = 0; x < 8; x++)
        {
          Set_pixel(context, (col << 3) + x, (row << 3) + y, bits & 0x80 ? colors >> 4 : colors & 0x0f);
          bits <<= 1;
        }
      }
    }
  }
  if (end <= 0x3800)
    return;  // no sprites
  // load sprites
  Set_loading_layer(context, 1);
  for (sprite = 0; sprite < 32; sprite++)
  {
    int x_pos, y_pos, x, y;
    byte pattern;
    byte color;

    y_pos = vram[0x1B00 + sprite * 4];
    x_pos = vram[0x1B00 + sprite * 4 + 1];
    pattern = vram[0x1B00 + sprite * 4 + 2];
    color = vram[0x1B00 + sprite * 4 + 3];
    GFX2_Log(GFX2_DEBUG, "Sprite #%02d (%d,%d) %02X %02X\n", sprite, x_pos, y_pos, pattern, color);
    if (y_pos == 208)
      break;
    y_pos = (y_pos + 1) & 0xff;
    if (color & 0x80)
      x_pos -= 32;
    color &= 0x0f;
    addr = 0x3800 + (pattern << 3);
    // 16x16 pixels sprites
    for (y = 0; y < 16; y++, addr++)
    {
      word bits = (vram[addr] << 8) | vram[addr+16];
      for (x = 0; x < 16; x++)
      {
        if (bits & 0x8000)
          Set_pixel(context, x_pos + x, y_pos + y, color);
        bits <<= 1;
      }
    }
  }
}

/* @} */
