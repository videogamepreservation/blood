#ifndef __GUI_H
#define __GUI_H

#include <stdlib.h>
#include "typedefs.h"
#include "resource.h"

// modal result codes
enum MODAL_RESULT {
	mrNone   = 0,
	mrOk,
	mrCancel,
	mrAbort,
	mrRetry,
	mrIgnore,
	mrYes,
	mrNo,
	mrUser,
};

enum GEVENT_TYPE {
	evNone			= 0x00,
	evMouseDown		= 0x01,
	evMouseUp		= 0x02,
	evMouseRepeat	= 0x04,
	evMouseDrag		= 0x08,
	evMouse			= 0xFF,

	evKeyDown		= 0x100,
	evKeyUp			= 0x200,
	evKey			= 0xF00,

	evCommand		= 0x1000,
	evBroadcast		= 0x2000,
	evMessage		= 0xF000,
};


struct MOUSE_GEVENT
{
	int time;
	int button;
	int x, y;
	int dx, dy;
	BOOL doubleClick;
};

struct KEY_GEVENT
{
	BYTE ascii;
	BYTE make;
	union
	{
		int shiftFlags;
		struct
		{
			unsigned shift		: 1;
			unsigned control	: 1;
			unsigned alt		: 1;
			unsigned lshift		: 1;
			unsigned rshift		: 1;
			unsigned lcontrol	: 1;
			unsigned rcontrol	: 1;
			unsigned lalt		: 1;
			unsigned ralt		: 1;
		};
	};
};

struct MESSAGE
{
	int command;
};

struct GEVENT
{
	GEVENT_TYPE type;
	int time;
	union
	{
		MOUSE_GEVENT mouse;
		KEY_GEVENT key;
		MESSAGE message;
	};
	void Clear( void ) { type = evNone; }
};

extern Resource gGuiRes;

void SetBlinkOn( void );
void SetBlinkOff( void );
BOOL IsBlinkOn( void );
void UpdateBlinkClock( int ticks );
void CenterString(int x, int y, char *s, int foreColor);
void DrawBevel( int x0, int y0, int x1, int y1, int color1, int color2 );
void DrawRect( int x0, int y0, int x1, int y1, int color );
int GetStringBox( char *title, char *s );
int GetNumberBox( char *title, int n, int nDefault );


class Widget
{
public:
	int left, top, width, height;
	BOOL Contains(int x, int y)
		{ return x >= left && y >= top && x < left + width && y < top + height; }
	Widget *prev, *next, *owner;
	BOOL canFocus;
	BOOL canDefault;
	BOOL isContainer;
	Widget( int left, int top, int width, int height ) :
		left(left), top(top), width(width), height(height), canFocus(FALSE), canDefault(FALSE),
		isContainer(FALSE) {};
	virtual ~Widget() {};
	virtual void Paint( int x, int y, BOOL hasFocus ) = 0;
	virtual void HandleEvent( GEVENT *event ) = 0;
	virtual void EndModal( MODAL_RESULT result ) { owner->EndModal(result); }
};

typedef void (*CLICK_PROC)( Widget * );


class HeadWidget : public Widget
{
public:
	HeadWidget() : Widget(0, 0, 0, 0) {};
	virtual void Paint( int, int, BOOL ) {};
	virtual void HandleEvent( GEVENT * ) {}
};

class Container : public Widget
{
public:
	BOOL isModal;
	MODAL_RESULT endState;
	Widget *focus;
	Widget *drag;	// receives drag and up events
	HeadWidget head;
	void Insert( Widget *widget );
	void Remove( Widget *widget );
	Container( int left, int top, int width, int height);
	virtual ~Container();
	virtual BOOL SetFocus( int dir );
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
	virtual void EndModal( MODAL_RESULT result );
};


class Panel : public Container
{
public:
	int size1, size2, size3;
	Panel( int left, int top, int width, int height, int size1, int size2, int size3) :
		Container(left, top, width, height), size1(size1), size2(size2), size3(size3)
		{ canFocus = FALSE; };
	virtual void Paint( int x, int y, BOOL hasFocus );
};


class TitleBar : public Widget
{
public:
	char string[256];
	int len;
	TitleBar( int left, int top, int width, int height, char *s );
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT * );
};


class Window : public Panel
{
	Container *client;
	TitleBar *titleBar;
public:
	void Insert( Widget *widget ) { client->Insert(widget); }
	Window( int left, int top, int width, int height, char *title);
};


class Button : public Widget
{
protected:
	MODAL_RESULT result;
	CLICK_PROC clickProc;
	BOOL pressed;
public:
	Button( int left, int top, int width, int height, MODAL_RESULT result ) :
		Widget(left, top, width, height), result(result), pressed(FALSE), clickProc(NULL)
		{}
	Button( int left, int top, int width, int height, CLICK_PROC clickProc ) :
		Widget(left, top, width, height), result(mrNone), pressed(FALSE), clickProc(clickProc)
		{}
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class TextButton : public Button
{
	char *text;
public:
	TextButton( int left, int top, int width, int height, char *text, MODAL_RESULT result ) :
		Button(left, top, width, height, result), text(text)
		{ canFocus = TRUE; }
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class BitButton : public Button
{
	RESHANDLE hBitmap;
public:
	BitButton( int left, int top, int width, int height, RESHANDLE hBitmap, CLICK_PROC clickProc) :
		Button( left, top, width, height, clickProc), hBitmap(hBitmap) {};
	virtual void Paint( int x, int y, BOOL hasFocus );
};

class EditText : public Widget
{
public:
	char string[256];
	int len;
	int pos;
	int maxlen;
	EditText( int left, int top, int width, int height, char *s );
	virtual void Paint( int x, int y, BOOL hasFocus );
	virtual void HandleEvent( GEVENT *event );
};

class EditNumber : public EditText
{
public:
	int value;
	EditNumber( int left, int top, int width, int height, int n );
	virtual void HandleEvent( GEVENT *event );
};


#define kSBWidth		11
#define kSBHeight		10

class ThumbButton : public Button
{
public:
	ThumbButton( int left, int top, int height ) :
		Button( left, top, kSBWidth, height, mrNone) {};
	virtual void HandleEvent( GEVENT *event );
};

class ScrollButton : public BitButton
{
public:
	ScrollButton( int left, int top, RESHANDLE hBitmap, CLICK_PROC clickProc ) :
		BitButton( left, top, kSBWidth, kSBHeight, hBitmap, clickProc) {};
	virtual void HandleEvent( GEVENT *event );
};


class ScrollBar : public Container
{
private:
	int min;
	int max;
	int size;
	ScrollButton *pbUp, *pbDown;
	Container *pcThumbBar;
	ThumbButton *pbThumb;
public:
	int value;
	ScrollBar( int left, int top, int height, int min, int max, int value);
	virtual void Paint( int x, int y, BOOL hasFocus );
	void ScrollRelative( int offset );
	void SetValue( int value, int size = 0 );
	void SetRange( int min, int max );
};


MODAL_RESULT ShowModal( Container *dialog );

#endif
