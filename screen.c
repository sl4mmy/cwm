/*
 * calmwm - the calm window manager
 *
 * Copyright (c) 2004 Marius Aamodt Eriksen <marius@monkey.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * $OpenBSD: screen.c,v 1.73 2015/06/26 16:11:21 okan Exp $
 */

#include <sys/types.h>
#include <sys/queue.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "calmwm.h"

void
screen_init(int which)
{
	struct screen_ctx	*sc;
	Window			*wins, w0, w1;
	XSetWindowAttributes	 rootattr;
	unsigned int		 nwins, i;

	sc = xmalloc(sizeof(*sc));

	TAILQ_INIT(&sc->clientq);
	TAILQ_INIT(&sc->regionq);
	TAILQ_INIT(&sc->groupq);

	sc->which = which;
	sc->rootwin = RootWindow(X_Dpy, sc->which);
	sc->cycling = 0;
	sc->hideall = 0;

	conf_screen(sc);

	xu_ewmh_net_supported(sc);
	xu_ewmh_net_supported_wm_check(sc);

	screen_update_geometry(sc);

	for (i = 0; i < CALMWM_NGROUPS; i++)
		group_init(sc, i);

	xu_ewmh_net_desktop_names(sc);
	xu_ewmh_net_wm_desktop_viewport(sc);
	xu_ewmh_net_wm_number_of_desktops(sc);
	xu_ewmh_net_showing_desktop(sc);
	xu_ewmh_net_virtual_roots(sc);

	rootattr.cursor = Conf.cursor[CF_NORMAL];
	rootattr.event_mask = SubstructureRedirectMask|SubstructureNotifyMask|
	    PropertyChangeMask|EnterWindowMask|LeaveWindowMask|
	    ColormapChangeMask|BUTTONMASK;

	XChangeWindowAttributes(X_Dpy, sc->rootwin,
	    CWEventMask|CWCursor, &rootattr);

	/* Deal with existing clients. */
	if (XQueryTree(X_Dpy, sc->rootwin, &w0, &w1, &wins, &nwins)) {
		for (i = 0; i < nwins; i++)
			(void)client_init(wins[i], sc);

		XFree(wins);
	}
	screen_updatestackingorder(sc);

	if (HasRandr)
		XRRSelectInput(X_Dpy, sc->rootwin, RRScreenChangeNotifyMask);

	TAILQ_INSERT_TAIL(&Screenq, sc, entry);

	XSync(X_Dpy, False);
}

struct screen_ctx *
screen_find(Window win)
{
	struct screen_ctx	*sc;

	TAILQ_FOREACH(sc, &Screenq, entry) {
		if (sc->rootwin == win)
			return(sc);
	}
	/* XXX FAIL HERE */
	return(TAILQ_FIRST(&Screenq));
}

void
screen_updatestackingorder(struct screen_ctx *sc)
{
	Window			*wins, w0, w1;
	struct client_ctx	*cc;
	unsigned int		 nwins, i, s;

	if (XQueryTree(X_Dpy, sc->rootwin, &w0, &w1, &wins, &nwins)) {
		for (s = 0, i = 0; i < nwins; i++) {
			/* Skip hidden windows */
			if ((cc = client_find(wins[i])) == NULL ||
			    cc->flags & CLIENT_HIDDEN)
				continue;

			cc->stackingorder = s++;
		}
		XFree(wins);
	}
}

/*
 * Find which xinerama screen the coordinates (x,y) is on.
 */
struct geom
screen_find_xinerama(struct screen_ctx *sc, int x, int y, int flags)
{
	struct region_ctx	*region;
	struct geom		 geom = sc->work;

	TAILQ_FOREACH(region, &sc->regionq, entry) {
		if (x >= region->area.x && x < region->area.x+region->area.w &&
		    y >= region->area.y && y < region->area.y+region->area.h) {
			geom = region->area;
			break;
		}
	}
	if (flags & CWM_GAP)
		geom = screen_apply_gap(sc, geom);
	return(geom);
}

void
screen_update_geometry(struct screen_ctx *sc)
{
	struct region_ctx	*region;
	int			 i;

	sc->view.x = 0;
	sc->view.y = 0;
	sc->view.w = DisplayWidth(X_Dpy, sc->which);
	sc->view.h = DisplayHeight(X_Dpy, sc->which);

	sc->work = screen_apply_gap(sc, sc->view);

	while ((region = TAILQ_FIRST(&sc->regionq)) != NULL) {
		TAILQ_REMOVE(&sc->regionq, region, entry);
		free(region);
	}

	if (HasRandr) {
		XRRScreenResources *sr;
		XRRCrtcInfo *ci;

		sr = XRRGetScreenResources(X_Dpy, sc->rootwin);
		for (i = 0, ci = NULL; i < sr->ncrtc; i++) {
			ci = XRRGetCrtcInfo(X_Dpy, sr, sr->crtcs[i]);
			if (ci == NULL)
				continue;
			if (ci->noutput == 0) {
				XRRFreeCrtcInfo(ci);
				continue;
			}

			region = xmalloc(sizeof(*region));
			region->num = i;
			region->area.x = ci->x;
			region->area.y = ci->y;
			region->area.w = ci->width;
			region->area.h = ci->height;
			TAILQ_INSERT_TAIL(&sc->regionq, region, entry);

			XRRFreeCrtcInfo(ci);
		}
		XRRFreeScreenResources(sr);
	}

	xu_ewmh_net_desktop_geometry(sc);
	xu_ewmh_net_workarea(sc);
}

struct geom
screen_apply_gap(struct screen_ctx *sc, struct geom geom)
{
	geom.x += sc->gap.left;
	geom.y += sc->gap.top;
	geom.w -= (sc->gap.left + sc->gap.right);
	geom.h -= (sc->gap.top + sc->gap.bottom);

	return(geom);
}
