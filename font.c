#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "post.h"

struct font {
	char name[FNLEN];
	char fontname[FNLEN];
	struct glyph glyphs[NGLYPHS];
	int nglyphs;
	int spacewid;
	/* glyph list based on the first character of their id fields in glyphs[] */
	int ghead[256];			/* glyph list head */
	int gnext[NGLYPHS];		/* next item in glyph list */
	/* charset section characters */
	char c[NGLYPHS][GNLEN];		/* character names in charset */
	struct glyph *g[NGLYPHS];	/* character glyphs in charset */
	int n;				/* number of characters in charset */
	/* glyph list based on the first character of glyph names in c[] */
	int chead[256];			/* glyph list head */
	int cnext[NGLYPHS];		/* next item in glyph list */
};

struct glyph *font_find(struct font *fn, char *name)
{
	int i = fn->chead[(unsigned char) name[0]];
	while (i >= 0) {
		if (!strcmp(name, fn->c[i]))
			return fn->g[i];
		i = fn->cnext[i];
	}
	return NULL;
}

struct glyph *font_glyph(struct font *fn, char *id)
{
	int i = fn->ghead[(unsigned char) id[0]];
	while (i >= 0) {
		if (!strcmp(fn->glyphs[i].id, id))
			return &fn->glyphs[i];
		i = fn->gnext[i];
	}
	return NULL;
}

static struct glyph *font_glyphput(struct font *fn, char *id,
				char *name, int wid, int type)
{
	int i = fn->nglyphs++;
	struct glyph *g;
	g = &fn->glyphs[i];
	strcpy(g->id, id);
	strcpy(g->name, name);
	g->wid = wid;
	g->type = type;
	g->font = fn;
	fn->gnext[i] = fn->ghead[(unsigned char) id[0]];
	fn->ghead[(unsigned char) id[0]] = i;
	return g;
}

static void tilloel(FILE *fin, char *s)
{
	int c = fgetc(fin);
	while (c != EOF && c != '\n') {
		*s++ = c;
		c = fgetc(fin);
	}
	*s = '\0';
	if (c != EOF)
		ungetc(c, fin);
}

static int font_readchar(struct font *fn, FILE *fin)
{
	char tok[ILNLEN];
	char name[ILNLEN];
	char id[ILNLEN];
	struct glyph *glyph = NULL;
	int wid, type;
	if (fn->n >= NGLYPHS)
		return 1;
	if (fscanf(fin, "%s %s", name, tok) != 2)
		return 1;
	if (!strcmp("---", name))
		sprintf(name, "c%04d", fn->n);
	if (strcmp("\"", tok)) {
		wid = atoi(tok);
		if (fscanf(fin, "%d %s", &type, id) != 2)
			return 1;
		glyph = font_glyph(fn, id);
		if (!glyph) {
			glyph = font_glyphput(fn, id, name, wid, type);
			tilloel(fin, tok);
			if (sscanf(tok, "%d", &glyph->pos) < 1)
				glyph->pos = 0;
		}
	} else {
		glyph = fn->g[fn->n - 1];
	}
	strcpy(fn->c[fn->n], name);
	fn->g[fn->n] = glyph;
	fn->cnext[fn->n] = fn->chead[(unsigned char) name[0]];
	fn->chead[(unsigned char) name[0]] = fn->n;
	fn->n++;
	return 0;
}

static void skipline(FILE* filp)
{
	int c;
	do {
		c = getc(filp);
	} while (c != '\n' && c != EOF);
}

struct font *font_open(char *path)
{
	struct font *fn;
	char tok[ILNLEN];
	FILE *fin;
	int i;
	fin = fopen(path, "r");
	if (!fin)
		return NULL;
	fn = malloc(sizeof(*fn));
	if (!fn) {
		fclose(fin);
		return NULL;
	}
	memset(fn, 0, sizeof(*fn));
	for (i = 0; i < LEN(fn->ghead); i++)
		fn->ghead[i] = -1;
	for (i = 0; i < LEN(fn->chead); i++)
		fn->chead[i] = -1;
	while (fscanf(fin, "%s", tok) == 1) {
		if (!strcmp("char", tok)) {
			font_readchar(fn, fin);
		} else if (!strcmp("spacewidth", tok)) {
			fscanf(fin, "%d", &fn->spacewid);
		} else if (!strcmp("name", tok)) {
			fscanf(fin, "%s", fn->name);
		} else if (!strcmp("fontname", tok)) {
			fscanf(fin, "%s", fn->fontname);
		} else if (!strcmp("ligatures", tok)) {
			while (fscanf(fin, "%s", tok) == 1)
				if (!strcmp("0", tok))
					break;
		} else if (!strcmp("charset", tok)) {
			while (!font_readchar(fn, fin))
				;
			break;
		}
		skipline(fin);
	}
	fclose(fin);
	return fn;
}

void font_close(struct font *fn)
{
	free(fn);
}

int font_wid(struct font *fn, int sz, int w)
{
	return (w * sz + dev_uwid / 2) / dev_uwid;
}

int font_swid(struct font *fn, int sz)
{
	return font_wid(fn, sz, fn->spacewid);
}

char *font_name(struct font *fn)
{
	return fn->fontname;
}
