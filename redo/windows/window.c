// 27 april 2015
#include "uipriv_windows.h"

#define windowClass L"libui_uiWindowClass"

struct uiWindow {
	uiWindowsControl c;
	HWND hwnd;
	HMENU menubar;
	struct child *child;
	BOOL shownOnce;
	int (*onClosing)(uiWindow *, void *);
	void *onClosingData;
	int margined;
};

static void onDestroy(uiWindow *);

uiWindowsDefineControlWithOnDestroy(
	uiWindow,							// type name
	uiWindowType,							// type function
	onDestroy(this);						// on destroy
)

static LRESULT CALLBACK windowWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	uiWindow *w;
	CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
	WINDOWPOS *wp = (WINDOWPOS *) lParam;
	LRESULT lResult;

	w = uiWindow((void *) GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	if (w == NULL) {
		if (uMsg == WM_CREATE)
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) (cs->lpCreateParams));
		// fall through to DefWindowProc() anyway
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
	if (handleParentMessages(hwnd, uMsg, wParam, lParam, &lResult) != FALSE)
		return lResult;
	switch (uMsg) {
	case WM_COMMAND:
		// not a menu
		if (lParam != 0)
			break;
		if (HIWORD(wParam) != 0)
			break;
		runMenuEvent(LOWORD(wParam), uiWindow(w));
		return 0;
	case WM_WINDOWPOSCHANGED:
		if ((wp->flags & SWP_NOSIZE) != 0)
			break;
		if (w->child != NULL)
			uiControlQueueResize(uiControl(w));
		return 0;
	case WM_PRINTCLIENT:
		// we do no special painting; just erase the background
		// don't worry about the return value; we let DefWindowProcW() handle this message
		SendMessageW(hwnd, WM_ERASEBKGND, wParam, lParam);
		return 0;
	case WM_CLOSE:
		if ((*(w->onClosing))(w, w->onClosingData))
			uiControlDestroy(uiControl(w));
		return 0;		// we destroyed it already
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

ATOM registerWindowClass(HICON hDefaultIcon, HCURSOR hDefaultCursor)
{
	WNDCLASSW wc;

	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = windowClass;
	wc.lpfnWndProc = windowWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = hDefaultIcon;
	wc.hCursor = hDefaultCursor;
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	return RegisterClassW(&wc);
}

void unregisterWindowClass(void)
{
	if (UnregisterClassW(windowClass, hInstance) == 0)
		logLastError("error unregistering uiWindow window class in unregisterWindowClass()");
}

static int defaultOnClosing(uiWindow *w, void *data)
{
	return 0;
}

static void onDestroy(uiWindow *w)
{
	// first hide ourselves
	ShowWindow(w->hwnd, SW_HIDE);
	// now destroy the child
	if (w->child != NULL)
		childDestroy(w->child);
	// now free the menubar, if any
	if (w->menubar != NULL)
		freeMenubar(w->menubar);
	// and finally destroy ourselves
	dialogHelperUnregisterWindow(w->hwnd);
}

static void windowCommitShow(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	if (w->shownOnce) {
		ShowWindow(w->hwnd, SW_SHOW);
		return;
	}
	w->shownOnce = TRUE;
	// make sure the child is the correct size
	uiControlQueueResize(uiControl(w));
	ShowWindow(w->hwnd, nCmdShow);
	if (UpdateWindow(w->hwnd) == 0)
		logLastError("error calling UpdateWindow() after showing uiWindow for the first time in windowShow()");
}

static void windowContainerUpdateState(uiControl *c)
{
	uiWindow *w = uiWindow(c);

	if (w->child != NULL)
		childContainerUpdateState(w->child);
}

char *uiWindowTitle(uiWindow *w)
{
	return uiWindowsUtilText(w->hwnd);
}

void uiWindowSetTitle(uiWindow *w, const char *title)
{
	uiWindowsUtilSetText(w->hwnd, title);
	// don't queue resize; the caption isn't part of what affects layout and sizing of the client area (it'll be ellipsized if too long)
}

void uiWindowOnClosing(uiWindow *ww, int (*f)(uiWindow *, void *), void *data)
{
	w->onClosing = f;
	w->onClosingData = data;
}

void uiWindowSetChild(uiWindow *w, uiControl *child)
{
	if (w->child != NULL)
		childRemove(w->child);
	w->child = newChild(child, uiControl(w), w->hwnd);
	if (w->child != NULL)
		uiControlQueueResize(w->child);
}

int uiWindowMargined(uiWindow *w)
{
	return w->margined;
}

void uiWindowSetMargined(uiWindow *w, int margined)
{
	w->margined = margined;
	uiControlQueueResize(uiControl(w));
}

// from https://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define windowMargin 7

static void windowResizeChild(uiWindow *ww)
{
	struct window *w = (struct window *) ww;
	RECT r;
	uiSizing *d;

	if (w->child == NULL)
		return;
	if (GetClientRect(w->hwnd, &r) == 0)
		logLastError("error getting uiWindow client rect in windowComputeChildSize()");
	d = uiControlSizing(uiControl(w));
	if (w->margined) {
		r.left += uiWindowsDlgUnitsToX(windowMargin, d->Sys->BaseX);
		r.top += uiWindowsDlgUnitsToY(windowMargin, d->Sys->BaseY);
		r.right -= uiWindowsDlgUnitsToX(windowMargin, d->Sys->BaseX);
		r.bottom -= uiWindowsDlgUnitsToY(windowMargin, d->Sys->BaseY);
	}
	uiControlResize(w->child, r.left, r.top, r.right - r.left, r.bottom - r.top, d);
	uiFreeSizing(d);
}

// see http://blogs.msdn.com/b/oldnewthing/archive/2003/09/11/54885.aspx and http://blogs.msdn.com/b/oldnewthing/archive/2003/09/13/54917.aspx
static void setClientSize(uiWindow *w, int width, int height, BOOL hasMenubar, DWORD style, DWORD exstyle)
{
	RECT window;

	window.left = 0;
	window.top = 0;
	window.right = width;
	window.bottom = height;
	if (AdjustWindowRectEx(&window, style, hasMenubar, exstyle) == 0)
		logLastError("error getting real window coordinates in setClientSize()");
	if (hasMenubar) {
		RECT temp;

		temp = window;
		temp.bottom = 0x7FFF;		// infinite height
		SendMessageW(w->hwnd, WM_NCCALCSIZE, (WPARAM) FALSE, (LPARAM) (&temp));
		window.bottom += temp.top;
	}
	if (SetWindowPos(w->hwnd, NULL, 0, 0, window.right - window.left, window.bottom - window.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		logLastError("error resizing window in setClientSize()");
}

uiWindow *uiNewWindow(const char *title, int width, int height, int hasMenubar)
{
	uiWindow *w;
	WCHAR *wtitle;
	BOOL hasMenubarBOOL;

	w = (uiWindow *) uiNewControl(uiWindowType());

	hasMenubarBOOL = FALSE;
	if (hasMenubar)
		hasMenubarBOOL = TRUE;

#define style WS_OVERLAPPEDWINDOW
#define exstyle 0

	wtitle = toUTF16(title);
	w->hwnd = CreateWindowExW(exstyle,
		windowClass, wtitle,
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		// use the raw width and height for now
		// this will get CW_USEDEFAULT (hopefully) predicting well
		// even if it doesn't, we're adjusting it later
		width, height,
		NULL, NULL, hInstance, w);
	if (w->hwnd == NULL)
		logLastError("error creating window in uiWindow()");
	uiFree(wtitle);

	dialogHelperRegisterWindow(w->hwnd);

	if (hasMenubar) {
		w->menubar = makeMenubar();
		if (SetMenu(w->hwnd, w->menubar) == 0)
			logLastError("error giving menu to window in uiNewWindow()");
	}

	// and use the proper size
	setClientSize(w, width, height, hasMenubarBOOL, style, exstyle);

	uiWindowSetOnClosing(w, defaultOnClosing, NULL);

	uiWindowsFinishNewControl(w, uiWindow);
	uiControl(w)->CommitShow = windowCommitShow;
	uiControl(w)->ContainerUpdateState = windowContainerUpdateState;

	return w;
}