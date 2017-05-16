/* by Clonk-Karl, 2006 */

/* GTK+ version of StdWindow */

#ifndef INC_STDGTKWINDOW
#define INC_STDGTKWINDOW

#include <StdWindow.h>

#include <gtk/gtkwindow.h>

class CStdGtkWindow : public CStdWindow
{
public:
	CStdGtkWindow();
	virtual ~CStdGtkWindow();

	virtual void Clear();

	virtual CStdWindow *Init(CStdApp *pApp, const char *Title, CStdWindow *pParent = 0, bool HideCursor = true);

	GtkWidget *window;

protected:
	// InitGUI should either return a widget which is used as a
	// render target or return what the base class returns, in which
	// case the whole window is used as render target.
	virtual GtkWidget *InitGUI();

private:
	static void OnDestroyStatic(GtkWidget *widget, gpointer data);
	static GdkFilterReturn OnFilter(GdkXEvent *xevent, GdkEvent *event, gpointer user_data);
	static gboolean OnUpdateKeyMask(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

	gulong handlerDestroy;
};

#endif // INC_STDGTKWINDOW
