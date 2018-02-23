#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <helix.h>
#include <ctype.h>

#include "typedefs.h"
#include "bstub.h"
#include "gfx.h"
#include "key.h"
#include "misc.h"
#include "gameutil.h"
#include "debug4g.h"
#include "resource.h"
#include "globals.h"
#include "gui.h"
#include "screen.h"
#include "mouse.h"
#include "globals.h"

#include <memcheck.h>

#define kColorBackground	gStdColor[20]
#define kColorHighlight		gStdColor[16]
#define kColorShadow		gStdColor[24]

#define kBlinkTicks		100
#define kBlinkOnTicks	60

static int blinkClock = 0;

Resource gGuiRes;

static QBITMAP *pMouseCursor = NULL;
static QFONT *pFont;


void SetBlinkOn( void )
{
	blinkClock = 0;
}


void SetBlinkOff( void )
{
	blinkClock = kBlinkOnTicks;
}


BOOL IsBlinkOn( void )
{
	return blinkClock < kBlinkOnTicks;
}


void UpdateBlinkClock( int ticks )
{
	blinkClock += ticks;
	while (blinkClock >= kBlinkTicks)
		blinkClock -= kBlinkTicks;
}

static void CenterLabel(int x, int y, char *s, int foreColor)
{
	if ( pFont != NULL )
		y -= pFont->height / 2;

	gfxDrawLabel(x - gfxGetLabelLen(s, pFont) / 2, y, foreColor, s, pFont);
}


void DrawBevel( int x0, int y0, int x1, int y1, int color1, int color2 )
{
	Video.SetColor(color1);
	gfxHLine(y0, x0, x1 - 2);
	gfxVLine(x0, y0 + 1, y1 - 2);
	Video.SetColor(color2);
	gfxHLine(y1 - 1, x0 + 1, x1 - 1);
	gfxVLine(x1 - 1, y0 + 1, y1 - 2);
}


void DrawRect( int x0, int y0, int x1, int y1, int color )
{
	Video.SetColor(color);
	gfxHLine(y0, x0, x1 - 1);
	gfxHLine(y1 - 1, x0, x1 - 1);
	gfxVLine(x0, y0 + 1, y1 - 2);
	gfxVLine(x1 - 1, y0 + 1, y1 - 2);
}


void DrawButtonFace( int x0, int y0, int x1, int y1, BOOL pressed )
{
	Video.SetColor(kColorBackground);
	gfxFillBox(x0, y0, x1, y1);
	if (pressed)
	{
		Video.SetColor(gStdColor[26]);
		gfxHLine(y0, x0, x1 - 1);
		gfxVLine(x0, y0 + 1, y1 - 1);
		Video.SetColor(gStdColor[24]);
		gfxHLine(y0 + 1, x0 + 1, x1 - 1);
		gfxVLine(x0 + 1, y0 + 2, y1 - 1);
		DrawBevel(x0 + 2, y0 + 2, x1, y1, gStdColor[19], gStdColor[22]);
	}
	else
	{
		DrawBevel(x0, y0, x1, y1, gStdColor[16], gStdColor[24]);
		DrawBevel(x0 + 1, y0 + 1, x1 - 1, y1 - 1, gStdColor[18], gStdColor[22]);
	}
}


void DrawMouseCursor( int x, int y )
{
	dassert(pMouseCursor != NULL);
	gfxDrawBitmap(pMouseCursor, x, y);
}


char GetHotKey( char *string )
{
	for ( char *p = string; *p; p++ )
	{
		if ( *p == '&' )
			return (char)toupper(p[1]);
	}

	return '\0';
}


Container::Container( int left, int top, int width, int height) : Widget(left, top, width, height)
{
	head.next = &head;
	head.prev = &head;
	focus = &head;
	isContainer = TRUE;
	isModal = FALSE;
	endState = mrNone;
}


virtual Container::~Container()
{
	for (Widget *w = head.next; w != &head; w = head.next)
	{
		Remove(w);
		delete w;
	}
}


BOOL Container::SetFocus( int dir )
{
	do
	{
		if (focus->isContainer)
		{
			if ( ((Container *)focus)->SetFocus(dir) )
				return TRUE;
		}

		if (dir > 0)
			focus = focus->next;
		else
			focus = focus->prev;

		if (focus == &head)
			return FALSE;
	} while (!focus->canFocus);
	return TRUE;
}


void Container::Insert( Widget *widget )
{
	dassert(widget != NULL);
	widget->prev = head.prev;
	widget->next = &head;
	widget->prev->next = widget;
	widget->next->prev = widget;
	widget->owner = this;
}


void Container::Remove( Widget *widget )
{
	dassert(widget != NULL);
	widget->prev->next = widget->next;
	widget->next->prev = widget->prev;
}


virtual void Container::Paint( int x, int y, BOOL /* hasFocus */ )
{
	for ( Widget *w = head.next; w != &head; w = w->next)
	{
		w->Paint(x + w->left, y + w->top, w == focus);
	}
}


virtual void Container::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		// make event relative to this container
		event->mouse.x -= left;
		event->mouse.y -= top;

		// find child owning location
		if ( event->type == evMouseDown )
		{
			drag = NULL;
			// iterate in reverse order since widgets on top have priority
			for ( Widget *w = head.prev; w != &head; w = w->prev)
			{
				if ( w->Contains(event->mouse.x, event->mouse.y) )
				{
					drag = w;
					if (drag->canFocus)
						focus = w;
					break;
				}
			}
		}

		if (drag != NULL)
			drag->HandleEvent(event);
	}
	else if ( event->type == evKeyDown )
	{
		focus->HandleEvent(event);

		// if key event not handled by focus, then broadcast to all childen
		if ( event->type != evNone )
		{
			for ( Widget *w = head.prev; w != &head; w = w->prev)
			{
				w->HandleEvent(event);
				if ( event->type == evNone )
					break;
			}
		}
	}
}


virtual void Container::EndModal( MODAL_RESULT result )
{
	if ( isModal )
	{
		endState = result;
		isModal = FALSE;
	}
	else
		owner->EndModal(result);
}


virtual void Panel::Paint( int x, int y, BOOL hasFocus )
{
	int i, n = 0;

	Video.SetColor(kColorBackground);
	gfxFillBox(x, y, x + width, y + height);

	for (i = qabs(size1); i > 0; n++, i--)
	{
		if (size1 > 0)
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorHighlight, kColorShadow);
		else
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorShadow, kColorHighlight);
	}

	n += size2;

	for (i = qabs(size3); i > 0; n++, i--)
	{
		if (size3 > 0)
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorHighlight, kColorShadow);
		else
			DrawBevel(x + n, y + n, x + width - n, y + height - n, kColorShadow, kColorHighlight);
	}

	Container::Paint(x, y, hasFocus);
}


TitleBar::TitleBar( int left, int top, int width, int height, char *s ) : Widget(left, top, width, height)
{
	strcpy(string, s);
	len = strlen(string);
}


virtual void TitleBar::Paint( int x, int y, BOOL /* hasFocus */ )
{
	Video.SetColor(gStdColor[1]);
	gfxFillBox(x, y, x + width, y + height);
	DrawBevel(x, y, x + width, y + height, gStdColor[9], gStdColor[30]);
	CenterLabel(x + width / 2, y + height / 2, string, gStdColor[15]);
}


virtual void TitleBar::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse && event->mouse.button == 0 )
	{
		switch (event->type)
		{
			case evMouseDrag:
				owner->left += event->mouse.dx;
				owner->top += event->mouse.dy;
				event->Clear();
				break;

			case evMouseUp:
				break;
		}
	}
}


Window::Window( int left, int top, int width, int height, char *title) : Panel(left, top, width, height, 1, 1, -1)
{
	titleBar = new TitleBar( 3, 3, width - 6, 12, title);
	client = new Container(3, 15, width - 6, height - 18);
	Container::Insert(titleBar);
	Container::Insert(client);
	drag = NULL;
}


virtual void Button::Paint( int x, int y, BOOL /* hasFocus */ )
{
	Video.SetColor(gStdColor[0]);
	gfxHLine(y, x + 1, x + width - 2);
	gfxHLine(y + height - 1, x + 1, x + width - 2);
	gfxVLine(x, y + 1, y + height - 2);
	gfxVLine(x + width - 1, y + 1, y + height - 2);

	DrawButtonFace(x + 1, y + 1, x + width - 1, y + height - 1, pressed);
}


virtual void Button::HandleEvent( GEVENT *event )
{
	if ( event->type == evKeyDown )
	{
		if ( event->key.ascii == ' ' )
		{
			pressed = !pressed;
			if (clickProc != NULL)
				clickProc(this);
			if ( result )
				EndModal(result);
			event->Clear();
		}
	}
	else if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
				pressed = TRUE;
				event->Clear();
				break;

			case evMouseDrag:
				pressed = Contains(event->mouse.x, event->mouse.y);
				event->Clear();
				break;

			case evMouseUp:
				pressed = FALSE;
				if ( Contains(event->mouse.x, event->mouse.y) )
				{
					if (clickProc != NULL)
						clickProc(this);
					if ( result )
						EndModal(result);
				}
				event->Clear();
				break;
		}
	}
}


virtual void TextButton::Paint( int x, int y, BOOL hasFocus )
{
	Video.SetColor(gStdColor[0]);
	gfxHLine(y + 1, x + 2, x + width - 3);
	gfxHLine(y + height - 2, x + 2, x + width - 3);
	gfxVLine(x + 1, y + 2, y + height - 3);
	gfxVLine(x + width - 2, y + 2, y + height - 3);

	if (hasFocus)
	{
		Video.SetColor(gStdColor[15]);
		gfxHLine(y, x + 1, x + width - 2);
		gfxHLine(y + height - 1, x + 1, x + width - 2);
		gfxVLine(x, y + 1, y + height - 2);
		gfxVLine(x + width - 1, y + 1, y + height - 2);
		gfxPixel(x + 1, y + 1);
		gfxPixel(x + width - 2, y + 1);
		gfxPixel(x + 1, y + height - 2);
		gfxPixel(x + width - 2, y + height - 2);
	}
	DrawButtonFace(x + 2, y + 2, x + width - 2, y + height - 2, pressed);

	if ( pressed )
		CenterLabel(x + width / 2 + 1, y + height / 2 + 1, text, gStdColor[0]);
	else
		CenterLabel(x + width / 2, y + height / 2, text, gStdColor[0]);
}


virtual void TextButton::HandleEvent( GEVENT *event )
{
	if ( event->type == evKeyDown )
	{
		if ( ScanToAsciiShifted[event->key.make] == GetHotKey(text) )
		{
			pressed = !pressed;
			if (clickProc != NULL)
				clickProc(this);
			if ( result )
				EndModal(result);
			event->Clear();
		}
	}

	Button::HandleEvent(event);
}


virtual void BitButton::Paint( int x, int y, BOOL /* hasFocus */ )
{
	Video.SetColor(gStdColor[0]);
	gfxHLine(y, x + 1, x + width - 2);
	gfxHLine(y + height - 1, x + 1, x + width - 2);
	gfxVLine(x, y + 1, y + height - 2);
	gfxVLine(x + width - 1, y + 1, y + height - 2);

	DrawButtonFace(x + 1, y + 1, x + width - 1, y + height - 1, pressed);

	int cx = x + width / 2;
	int cy = y + height / 2;

	QBITMAP *pBitmap = (QBITMAP *)gGuiRes.Load(hBitmap);
	if ( pressed )
		gfxDrawBitmap(pBitmap, cx - pBitmap->cols / 2 + 1, cy - pBitmap->rows / 2 + 1);
	else
		gfxDrawBitmap(pBitmap, cx - pBitmap->cols / 2, cy - pBitmap->rows / 2);
}


EditText::EditText( int left, int top, int width, int height, char *s ) : Widget(left, top, width, height)
{
	canFocus = TRUE;
	strcpy(string, s);
	len = strlen(string);
	pos = len;
	maxlen = width / 8 - 1;
}


virtual void EditText::Paint( int x, int y, BOOL hasFocus )
{
	DrawBevel(x, y, x + width - 1, y + height - 1, kColorShadow, kColorHighlight);
	DrawRect(x + 1, y + 1, x + width - 2, y + height - 2, gStdColor[0]);
	Video.SetColor(gStdColor[hasFocus ? 15 : 20]);
	gfxFillBox(x + 2, y + 2, x + width - 3, y + height - 3);
	gfxDrawText(x + 3, y + height / 2 - 4, gStdColor[0], string, pFont);

	if ( hasFocus && IsBlinkOn() )
	{
		Video.SetColor(gStdColor[0]);
		gfxVLine(x + gfxGetTextNLen(string, pFont, pos) + 3, y + height / 2 - 4, y + height / 2 + 3);
	}
}


virtual void EditText::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
			case evMouseDrag:
				pos = gfxFindTextPos(string, pFont, event->mouse.x - left);
				SetBlinkOn();
				event->Clear();
				break;
		}
	}
	else if ( event->type == evKeyDown )
	{
		switch ( event->key.make )
		{
			case KEY_BACKSPACE:
				if (pos > 0)
				{
					memmove(&string[pos - 1], &string[pos], len - pos);
					pos--;
					len--;
					string[len] = '\0';
				}
				event->Clear();
				break;

			case KEY_DELETE:
				if (pos < len)
				{
					len--;
					memmove(&string[pos], &string[pos + 1], len - pos);
					string[len] = '\0';
				}
				event->Clear();
				break;

			case KEY_LEFT:
				if (pos > 0)
					pos--;
				event->Clear();
				break;

			case KEY_RIGHT:
				if (pos < len)
					pos++;
				event->Clear();
				break;

			case KEY_HOME:
				pos = 0;
				event->Clear();
				break;

			case KEY_END:
				pos = len;
				event->Clear();
				break;

			default:
				if ( event->key.ascii != 0 )
				{
					if ( len < maxlen )
					{
						memmove(&string[pos+1], &string[pos], len - pos);
						string[pos++] = event->key.ascii;
						string[++len] = '\0';
					}
					event->Clear();
				}
				break;
		}
		SetBlinkOn();
	}
}



EditNumber::EditNumber( int left, int top, int width, int height, int n ) : EditText( left, top, width, height, "")
{
	value = n;
	itoa(n, string, 10);
	len = strlen(string);
	pos = len;
}


virtual void EditNumber::HandleEvent( GEVENT *event )
{
	if ( event->type == evKeyDown )
	{
		switch ( event->key.make )
		{
			case KEY_BACKSPACE:
			case KEY_DELETE:
			case KEY_LEFT:
			case KEY_RIGHT:
			case KEY_HOME:
			case KEY_END:
				break;

			case KEY_MINUS:
				if (pos == 0 && string[0] != '-' && len < maxlen)
				{
					memmove(&string[1], &string[0], len);
					string[pos++] = '-';
					string[++len] = '\0';
				}
				event->Clear();
				break;

			default:
				if ( event->key.ascii != 0 )
				{
					if (event->key.ascii >= '0' && event->key.ascii <= '9' && len < maxlen)
					{
						memmove(&string[pos+1], &string[pos], len - pos);
						string[pos++] = event->key.ascii;
						string[++len] = '\0';
					}
					event->Clear();
				}
				break;
		}
	}
	EditText::HandleEvent( event );
	value = atoi(string);
}


virtual void ThumbButton::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDrag:
				top = ClipRange(event->mouse.y - height / 2, kSBHeight, owner->height - kSBHeight - height);
				break;

			case evMouseDown:
				pressed = TRUE;
				break;

			case evMouseUp:
				pressed = FALSE;
				break;
		}
	}
}


virtual void ScrollButton::HandleEvent( GEVENT *event )
{
	if ( event->type & evMouse )
	{
		if (event->mouse.button != 0)
			return;

		switch (event->type)
		{
			case evMouseDown:
				pressed = TRUE;
				if (clickProc != NULL)
					clickProc(this);
				break;

			case evMouseDrag:
				pressed = Contains(event->mouse.x, event->mouse.y);
				break;

			case evMouseRepeat:
				if ( pressed && clickProc != NULL )
					clickProc(this);
				break;

			case evMouseUp:
				pressed = FALSE;
				break;
		}
	}
}


void ScrollLineUp( Widget *widget )
{
	ScrollBar *pScrollBar = (ScrollBar *)widget->owner;
	pScrollBar->ScrollRelative(-1);
}


void ScrollLineDown( Widget *widget )
{
	ScrollBar *pScrollBar = (ScrollBar *)widget->owner;
	pScrollBar->ScrollRelative(+1);
}


ScrollBar::ScrollBar( int left, int top, int height, int min, int max, int value ) :
	Container(left, top, kSBWidth + 2, height), min(min), max(max), value(value)
{
	pbUp = new ScrollButton(1, 1, gGuiRes.Lookup("UPARROW", ".QBM"), ScrollLineUp);
	pbDown = new ScrollButton(1, height - kSBHeight - 1, gGuiRes.Lookup("DNARROW", ".QBM"), ScrollLineDown);
//	pcThumbBar = new Container(1,
	pbThumb = new ThumbButton(1, 10, 20);
	Insert(pbUp);
	Insert(pbDown);
	Insert(pbThumb);
	size = 0;
}

void ScrollBar::ScrollRelative( int offset )
{
	pbThumb->top = ClipRange(pbThumb->top + offset, kSBHeight, height - kSBHeight - pbThumb->height);
}



virtual void ScrollBar::Paint( int x, int y, BOOL hasFocus )
{
	DrawBevel(x, y, x + width, y + height, kColorShadow, kColorHighlight);
	DrawRect(x + 1, y + 1, x + width - 1, y + height - 1, gStdColor[0]);
	Video.SetColor(kColorShadow);
	gfxFillBox(x + 2, y + 2, x + width - 2, y + height - 2);
	Container::Paint(x, y, hasFocus);
}


#define kRepeatDelay	60
#define kRepeatInterval	6
#define kDoubleClick	60

GEVENT_TYPE GetEvent( GEVENT *event )
{
	static int clickTime[3], downTime[3];
	static BYTE oldbuttons = 0;
	BYTE newbuttons;
	BYTE key;

	memset(event, 0, sizeof(GEVENT));

	if ( (key = keyGet()) != 0 )
	{
		if ( keystatus[KEY_LSHIFT] )
			event->key.lshift = 1;
		if ( keystatus[KEY_RSHIFT] )
			event->key.rshift = 1;
		event->key.shift = event->key.lshift | event->key.rshift;

		if ( keystatus[KEY_LCTRL] )
			event->key.lcontrol = 1;
		if ( keystatus[KEY_RCTRL] )
			event->key.rcontrol = 1;
		event->key.control = event->key.lcontrol | event->key.rcontrol;

		if ( keystatus[KEY_LALT] )
			event->key.lalt = 1;
		if ( keystatus[KEY_RALT] )
			event->key.ralt = 1;
		event->key.alt = event->key.lalt | event->key.ralt;

		if ( event->key.alt )
			event->key.ascii = 0;
		else if ( event->key.control )
			event->key.ascii = 0;
		else if ( event->key.shift )
			event->key.ascii = ScanToAsciiShifted[key];
		else
			event->key.ascii = ScanToAscii[key];

		event->key.make = key;
		event->type = evKeyDown;

		if ( key == KEY_ESC )
			keystatus[KEY_ESC] = 0;	// some of Ken's stuff still checks this!

		return event->type;
	}

	event->mouse.dx = Mouse::dX;
	event->mouse.dy = Mouse::dY;
	event->mouse.x = Mouse::X;
	event->mouse.y = Mouse::Y;

	// which buttons just got pressed?
	newbuttons = (BYTE)(~oldbuttons & Mouse::buttons);

	BYTE buttonMask = 1;
	for (int nButton = 0; nButton < 3; nButton++, buttonMask <<= 1)
	{
		event->mouse.button = nButton;

		if (newbuttons & buttonMask)
		{
			oldbuttons |= buttonMask;
			event->type = evMouseDown;

			// create double click event if in time interval
			if ( gFrameClock < clickTime[nButton] + kDoubleClick )
				event->mouse.doubleClick = TRUE;

			clickTime[nButton] = gFrameClock;
			downTime[nButton] = 0;

			return event->type;
		}
		else if (oldbuttons & buttonMask)
		{
			if (Mouse::buttons & buttonMask)
			{
				downTime[nButton] += gFrameTicks;

				if ( event->mouse.dx || event->mouse.dy )
				{
					event->type = evMouseDrag;
					return event->type;
				}

				if ( downTime[nButton] > kRepeatDelay )
				{
					downTime[nButton] -= kRepeatInterval;
					event->type = evMouseRepeat;
					return event->type;
				}
			}
			else
			{
				oldbuttons &= ~buttonMask;
				event->type = evMouseUp;
				return event->type;
			}
		}
	}
	return evNone;
}



MODAL_RESULT ShowModal( Container *dialog )
{
	GEVENT event;

	Container desktop(0, 0, xdim, ydim);
	desktop.Insert(dialog);

	// center the dialog
	dialog->left = (xdim - dialog->width) / 2;
	dialog->top = (ydim - dialog->height) / 2;

	int saveSize = xdim * ydim;

	// find first item for focus
	while (!desktop.SetFocus(+1));

	BYTE *saveUnder = (BYTE *)Resource::Alloc(saveSize);

	RESHANDLE hMouseCursor = gGuiRes.Lookup("MOUSE1", ".QBM");
	dassert(hMouseCursor != NULL);
	pMouseCursor = (QBITMAP *)gGuiRes.Lock(hMouseCursor);

	RESHANDLE hFont = gGuiRes.Lookup("FONT1", ".QFN");
	dassert(hFont != NULL);
	pFont = (QFONT *)gGuiRes.Lock(hFont);

	// copy save under from last displayed page
	switch (vidoption)
	{
		case 0:	// unchained mode
			break;

		case 1:	// linear frame buffer mode
		case 2:	// screen buffer mode
			memcpy(saveUnder, frameplace, saveSize);
			break;

		case 3:	// tseng mode
			page = (page + 3) & 3;
			outp(0x3CD, page + (page << 4));
			memcpy(saveUnder, frameplace, saveSize);
			page = (page + 1) & 3;
			outp(0x3CD, page + (page << 4));
			break;

		case 4:	// paradise
			page = (page + 3) & 3;
			outp(0x3CE, 0x09); outp(0x3CF, page << 4);
			memcpy(saveUnder, frameplace, saveSize);
			page = (page + 1) & 3;
			outp(0x3CE, 0x09); outp(0x3CF, page << 4);
			break;

		case 5:	// s3
			page = (page + 3) & 3;
			outp(0x3D4, 0x35); outp(0x3D5, page);
			memcpy(saveUnder, frameplace, saveSize);
			page = (page + 1) & 3;
			outp(0x3D4, 0x35); outp(0x3D5, page);
			break;
	}

	dialog->isModal = TRUE;

	while ( dialog->isModal )
	{
		gFrameTicks = gGameClock - gFrameClock;
		gFrameClock += gFrameTicks;
		UpdateBlinkClock(gFrameTicks);
		Mouse::Read(gFrameTicks);

		GetEvent(&event) ;

		// trap certain dialog keys
		if ( event.type == evKeyDown )
		{
			switch ( event.key.ascii )
			{
				case 27:
					dialog->EndModal(mrCancel);
					continue;

				case 13:
					dialog->EndModal(mrOk);
					continue;

				case 9:
					if ( event.key.shift )
						while (!desktop.SetFocus(-1));
					else
						while (!desktop.SetFocus(+1));
					continue;
			}
		}

		desktop.HandleEvent(&event);

		// copy the save under first
		memcpy(frameplace, saveUnder, saveSize);

		desktop.Paint(0, 0, FALSE);

		DrawMouseCursor(Mouse::X, Mouse::Y);

		if (vidoption == 2)
			WaitVSync();

		scrNextPage();

		// restore the save under after page flipping
		memcpy(frameplace, saveUnder, saveSize);
	}

	memcpy(frameplace, saveUnder, saveSize);
	scrNextPage();

	gGuiRes.Unlock(hMouseCursor);
	pMouseCursor = NULL;
	gGuiRes.Unlock(hFont);
	pFont = NULL;

	Resource::Free(saveUnder);
	desktop.Remove(dialog);
	return dialog->endState;
}


int GetStringBox( char *title, char *s )
{
	// create the dialog
	Window dialog(0, 0, 168, 40, title);

	// this will automatically be destroyed when the dialog is destroyed
	EditText *el = new EditText(4, 4, 154, 16, s);
	dialog.Insert(el);

	ShowModal(&dialog);
	if ( dialog.endState != mrOk)
		return 0;		// pressed escape

	// copy the edited string
	strcpy(s, el->string);

	return 1;
}


int GetNumberBox( char *title, int n, int nDefault )
{
	// create the dialog
	Window dialog(0, 0, 168, 40, title);

	// this will automatically be destroyed when the dialog is destroyed
	EditNumber *en = new EditNumber(4, 4, 154, 16, n);
	dialog.Insert(en);

	ShowModal(&dialog);
	if ( dialog.endState != mrOk)
		return nDefault;		// pressed escape

	return en->value;
}
