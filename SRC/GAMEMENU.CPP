#include "gui.h"
#include "gfx.h"

class PageTextButton : public Button
{
	char	*zText;
	int		nFont;
	uchar	uForeHue;
	uchar	uBackHue;

public:
	PageTextButton( int left, int top, int width, int height, char *text, CLICK_PROC clickProc ) :
		Button( left, top, width, height, clickProc),
		zText(text), nFont(0), uForeHue(31), uBackHue(-1) { canFocus = TRUE; };

	SetHue( int foreHue, int backHue );
	virtual void Paint( int x, int y, BOOL hasFocus );
};

virtual void PageTextButton::Paint( int x, int y, BOOL hasFocus )
{
	gfxDrawText( x, y, uForeHue, zText, NULL );
}





class PageButton : public Button
{
	int		nTile;
public:
	virtual void Paint( int x, int y, BOOL hasFocus );

	// assumes nTile + 1 is focus state bitmap
	PageButton( int top, int left, int width, int height, int nTile, CLICK_PROC clickProc ) :
		Button( int top, int left ) { canFocus = TRUE; };
};

PageButton::PageButton( int top, int left, RESHANDLE hBitmap1,
	RESHANDLE hBitmap1, CLICK_PROC clickProc ) :
	Button( left, top, width, height, clickProc), hBitmap(hBitmap), text(NULL) {};

{
}

PageButton::PageButton( int top, int left, char *text, CLICK_PROC clickProc ) :
	Button( int left, int top, int width, int height, CLICK_PROC clickProc ):
		Button(left, top, width, height, result), text(text)

{
}

virtual void PageButton::Paint( int x, int y, BOOL hasFocus )
{
	if (text != NULL
}


class PageBook : public Container
{
protected:
	Container *focusPage;

public:
	PageBook( int top, int left, int width, int height );
	~PageBook();

	virtual BOOL SetFocus( int dir );
	virtual MODAL_RESULT HandleKey( BYTE key );
//	virtual MODAL_RESULT HandleMouse( MEVENT event );
};


PageBook::PageBook( int top, int left, int width, int height )
{
	focusPage = NULL;
}

virtual BOOL PageBook::SetFocus( int dir )
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


virtual MODAL_RESULT PageBook::HandleKey( BYTE key )
{
	switch (key)
	{
		case KEY_UP:
			break;

		case KEY_DOWN:
			break;

		case KEY_ESC:
			break;

		default:
		{
			char c;
			if (keystatus[KEY_LSHIFT] || keystatus[KEY_RSHIFT])
				c = ScanToAsciiShifted[key];
			else
				c = ScanToAscii[key];

			if ( c != 0 )
			{
				// read through the MenuLine hotkeys
			}
			break;
		}
	}
	return mrNone;
}





ShowMenu( Container *dialog )
{
	BYTE key;
	MODAL_RESULT result = mrNone;

	Container desktop(0, 0, xdim, ydim);
	desktop.Insert(dialog);

	// find first item for focus
	while (!desktop.SetFocus(+1));

	while ( (key = keyGet()) != 0 )
	{
		switch (key)
		{
			case KEY_ESC:
				keystatus[KEY_ESC] = 0;	// some of Ken's stuff still checks this!
				result = mrCancel;
				break;

			case KEY_ENTER:
				result = mrOk;
				break;

			case KEY_TAB:
				if (keystatus[KEY_LSHIFT] || keystatus[KEY_RSHIFT])
					while (!desktop.SetFocus(-1));
				else
					while (!desktop.SetFocus(+1));
				break;

			default:
				result = desktop.HandleKey(key);
		}
	}

	desktop.Paint(0, 0, FALSE);

	// draw cursors

	desktop.Remove(dialog);
	return result;
}
///////////////////////////////////////////

enum MOBTYPE {
	MOBTYPE_Menu,
	MOBTYPE_Button,
	MOBTYPE_HSlider
};

struct MOBFLAGS {
	ushort	disabled	: 1;
	ushort	invisible	: 1;
	ushort	pad			: 14;
};

struct MenuObject {
	MOBTYPE		type;
	void		*object;
	MOBFLAGS	flags;
};

class HSlider
{
private:
protected:
public:
	HSlider( void );
	~HSlider();

	void Draw( void );

};

class GameMenu
{
private:
	MenuObject *head;

protected:
public:
	GameMenu( void );
	~GameMenu();

	void Draw( void );

	void AddButton( char *text, void (*MenuProc)( void ) );
	void AddButton( char *text, GameMenu &subMenu );

	void AddHSlider( char *text, HSlider &theSlider );
	void AddSpace( void );
};




// draw the
void GameMenu::Draw( void )
{

}


GameMenu gMainMenu;
GameMenu gOptMenu;
GameMenu gSoundMenu;
GameMenu gSwitchMenu;

//
// Global menu objects initialized in main() in BLOOD.C
//
void InitMenus( void )
{
	gMainMenu.AddButton( "New Game", menuNewGame );
	gMainMenu.AddButton( "Load Game", menuLoadGame );
	gMainMenu.AddButton( "Save Game", menuSaveGame );
	gMainMenu.AddButton( "Options", gOptMenu );
	gMainMenu.AddButton( "Order Info", menuOrderInfo );
	gMainMenu.AddButton( "Quit", menuQuit );

	gOptMenu.AddButton( "End Game" );
	gOptMenu.AddHSlider( "Detail", SliderDrawDetail );
	gOptMenu.AddHSlider( "Mouse Sensitivity", SliderDrawMouse );
	gOptMenu.AddSpace();
	gOptMenu.AddButton( "Sound Volume", gSoundMenu );
	gOptMenu.AddButton( "Game Toggles", gSwitchMenu );

	gSoundMenu.AddHSlider( "FX Volume", SliderDrawFXVolume );
	gSoundMenu.AddSpace();
	gSoundMenu.AddHSlider( "Music Volume", SliderDrawMusicVolume );
	gSoundMenu.AddSpace();

	gSwitchMenu.AddButton( "Mouse", menuMouseToggle );
	gSwitchMenu.AddButton( "Joystick", menuJoyToggle );
	gSwitchMenu.AddButton( "Bobbing", menuBobToggle );
	gSwitchMenu.AddButton( "Detail", menuDetailToggle );
}

//
// Draw method for global menu object called in DrawScreen in BLOOD.C
//
//void DrawScreen( void )
//{
//	...
//
//	if ( bMenuUp )
//		bMenuUp = (ShowMenu() == mrNone);

// //LocalKeys should set bMenuUp => TRUE
//	scrSetDac(gFrameTicks);	// this could be a problem for menus
//	scrNextPage();
//}




void OptionClick( Widget* /* widget */ )
{
	gGameMenuNotebook.SetPage(kPageOptionMenu);
}

gGameMenuNotebook.Insert(gMainPage);
gGameMenuNotebook.Insert(gOptionPage);
gGameMenuNotebook.Insert(gSoundPage);

gMainPage.Insert(new MenuLine("Options", OptionClick));

