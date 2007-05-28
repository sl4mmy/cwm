/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2004 Marius Aamodt Eriksen <marius@monkey.org>
 * All rights reserved.
 *
 * $Id: screen.c,v 1.2 2007/05/28 18:34:27 jasper Exp $
 */

#include "headers.h"
#include "calmwm.h"

extern struct screen_ctx_q	Screenq;
extern struct screen_ctx       *Curscreen;

static void
_clearwindow_cb(int sig)
{
	struct screen_ctx *sc = screen_current();
	XUnmapWindow(X_Dpy, sc->infowin);
}

struct screen_ctx *
screen_fromroot(Window rootwin)
{
	struct screen_ctx *sc;

	TAILQ_FOREACH(sc, &Screenq, entry)
		if (sc->rootwin == rootwin)
			return (sc);

	/* XXX FAIL HERE */
	return (TAILQ_FIRST(&Screenq));
}

struct screen_ctx *
screen_current(void)
{
	return (Curscreen);
}

void
screen_updatestackingorder(void)
{
	Window *wins, w0, w1;
	struct screen_ctx *sc = screen_current();
	u_int nwins, i, s;
	struct client_ctx *cc;

	if (!XQueryTree(X_Dpy, sc->rootwin, &w0, &w1, &wins, &nwins))
		return;

	for (s = 0, i = 0; i < nwins; i++) {
		/* Skip hidden windows */
		if ((cc = client_find(wins[i])) == NULL ||
		    cc->flags & CLIENT_HIDDEN)
			continue;

		cc->stackingorder = s++;
	}

	XFree(wins);
}

void
screen_init(void)
{

	struct screen_ctx *sc = screen_current();

	sc->cycle_client = NULL;

	sc->infowin = XCreateSimpleWindow(X_Dpy, sc->rootwin, 0, 0,
	    1, 1, 1, sc->blackpixl, sc->whitepixl);

	/* XXX - marius. */
	if (signal(SIGALRM, _clearwindow_cb) == SIG_ERR)
		err(1, "signal");
}

void
screen_infomsg(char *msg)
{
	struct screen_ctx *sc = screen_current();
	char buf[1024];
	int dy, dx;
	struct fontdesc *font = DefaultFont;

	XUnmapWindow(X_Dpy, sc->infowin);
	alarm(0);

	snprintf(buf, sizeof(buf), ">%s", msg);
	dy = font_ascent(font) + font_descent(font) + 1;
	dx = font_width(font, buf, strlen(buf));

	XMoveResizeWindow(X_Dpy, sc->infowin, 0, 0, dx, dy);
	XMapRaised(X_Dpy, sc->infowin);

	font_draw(font, buf, strlen(buf), sc->infowin,
	    0, font_ascent(font) + 1);

	alarm(1);
}
