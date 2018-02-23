#ifndef __GUIEDIT_H
#define __GUIEDIT_H

#include "gui.h"

class EButton : public TextButton
{
public:
	EButton( int left, int top, int width, int height, char *text, MODAL_RESULT result ) :
		TextButton(left, top, width, height, text, result) {};
	virtual void HandleEvent( GEVENT *event );
};

#endif


