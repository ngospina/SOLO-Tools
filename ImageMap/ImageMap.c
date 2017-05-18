/*
	ImageMap.c
	Copyright (C) 2017, Gerardo Ospina

	This program creates C header files with page map vectors for .dsk and .rk05 
	SOLO Images

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
	GERARDO OSPINA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
	IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

	Except as contained in this notice, the name of Gerardo Ospina shall not
	be used in advertising or otherwise to promote the sale, use or other dealings
	in this Software without prior written authorization from Gerardo Ospina.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "DskImageMap.h"

#define HEADER_NAME "DskRK05Map.h"

#define PAGE_LENGTH	512
#define DISK_PAGES	4800

typedef unsigned char TPage[PAGE_LENGTH];

static unsigned int dsk_rk05_map[DISK_PAGES];
static unsigned int rk05_dsk_map[DISK_PAGES];

static void genSectorMap(int page, int index)
{
	int sectorMap[24] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
	int sector;

	for (sector = 0; sector < 24; sector++)
	{
		if (sectorMap[index] != -1)
		{
			index = (index + 1) % 24;
		}
		sectorMap[index] = page + sector;
		index = (index + 6) % 24;
	}
	for (sector = 0; sector < 24; sector++)
	{
		dsk_rk05_map[page + sector] = sectorMap[sector];
		rk05_dsk_map[sectorMap[sector]] = page + sector;
	}
}

static void genImageMaps(void)
{
	int cylinder;
	int index;
	int basePage;

	for (cylinder = 0; cylinder < 200; cylinder++)
	{
		index = (cylinder * 5) % 24;
		basePage = cylinder * 24;
		dsk_rk05_map[basePage + index] = basePage;
		rk05_dsk_map[basePage] = basePage + index;
		genSectorMap(basePage, index);
	}
}

static void write_dsk_rk05_map(FILE *file)
{
	int page;

	fprintf(file, "/*\n\tDskRK05Map.h\n*/\n\n");
	fprintf(file, "#ifndef _DSKRK05MAP_H_\n\n");
	fprintf(file, "#define _DSKRK05MAP_H_\n");
	fprintf(file, "\nstatic int dsk_rk05_map[%d] = {", DISK_PAGES);
	for (page = 0; page < DISK_PAGES; page++)
	{
		if (page % 12 == 0)
		{
			fprintf(file, "\n/*%4d*/\t", page);
		}
		fprintf(file, "%4d", dsk_rk05_map[page]);
		if (page != DISK_PAGES - 1)
		{
			fprintf(file, ",");
		}
	}
	fprintf(file, "\n}\n\n");
	fprintf(file, "#endif\n");
}

static void write_rk05_dsk_map(FILE *file)
{
	int page;

	fprintf(file, "/*\n\tRK05DskMap.h\n*/\n\n");
	fprintf(file, "#ifndef _RK05DSKMAP_H_\n\n");
	fprintf(file, "#define _RK05DSKMAP_H_\n");
	fprintf(file, "\nstatic int rk05_dsk_map[%d] = {", DISK_PAGES);
	for (page = 0; page < DISK_PAGES; page++)
	{
		if (page % 12 == 0)
		{
			fprintf(file, "\n/*%4d*/\t", page);
		}
		fprintf(file, "%4d", rk05_dsk_map[page]);
		if (page != DISK_PAGES - 1)
		{
			fprintf(file, ",");
		}
	}
	fprintf(file, "\n}\n\n");
	fprintf(file, "#endif\n");
}

static void write_geometry(FILE *file, int page)
{
	int surface, cylinder, sector;

	surface = (page / 12) % 2;
	cylinder = page / 24;
	sector = page % 12;
	fprintf(file, "(H:%1d C:%3d S:%2d)", surface, cylinder, sector);
}

static void write_dsk_rk05(FILE *file)
{
	unsigned int page;

	for (page = 0; page < DISK_PAGES; page++)
	{
		fprintf(file, "%4d ", page);
		write_geometry(file, page);
		fprintf(file, " %4d ", dsk_rk05_map[page]);
		write_geometry(file, dsk_rk05_map[page]);
		fprintf(file, "\n");
	}
}

static unsigned int openFiles(FILE *files[3])
{
	char *filenames[3] = {"DskRK05Map.h", "RK05DskMap.h", "DskRK05Map.txt" };
	unsigned int rc = 0;
	unsigned int i;

	for (i = 0; i < 3; i++) {
		files[i] = fopen(filenames[i], "wb");
		if (files[i] == NULL)
		{
			printf("Error creating file: %s\n", filenames[i]);
		}
		else
		{
			rc++;
		}
	}
	return rc == 3;
}

static void closeFiles(FILE *files[3])
{
	unsigned int i;

	for (i = 0; i < 3; i++)
	{
		if (files[i] != NULL)
		{
			fclose(files[i]);
			files[i] = NULL;
		}
	}
}

static void checkDskImageMap(void)
{
	unsigned int nomatch = 0;
	unsigned int valid = 0;
	unsigned int invalid = 0;
	unsigned int total = 0;
	unsigned int page;

	for (page = 0; page < DISK_PAGES; page++)
	{
		if (dsk_image_map[page] != 4800) {
			if (dsk_image_LCS_length[page] < 512)
			{
				dsk_image_map[page] = 4800;
				dsk_image_LCS_length[page] = 0;
				nomatch++;
			}
			else if (dsk_image_map[page] != dsk_rk05_map[page])
			{
				invalid++;
			}
			else
			{
				valid++;
			}
			total++;
		}
	}
	printf("%4d %4d/%4d/%4d\n", nomatch, valid, invalid, total);
	for (page = 0; page < DISK_PAGES; page++)
	{
		if (dsk_image_map[page] == page)
		{
			printf("%d\n", page);
		}
	}
	getchar();
}

int main(int argc, char *argv[])
{
	FILE *file[3];

	if (openFiles(file))
	{
		genImageMaps();
		checkDskImageMap();
		write_dsk_rk05_map(file[0]);
		write_rk05_dsk_map(file[1]);
		write_dsk_rk05(file[2]);
	}
	closeFiles(file);
	return EXIT_SUCCESS;
}