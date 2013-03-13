#ifndef gui_HEADER
#define gui_HEADER

#include <gtk/gtk.h>
#include <glib.h>

// we'll break this out later
#define LANG_MENU_FILE "File"
#define LANG_LOAD_EXEC_FILE "Load Executable File"
#define LANG_ADDRESS "Address"
#define LANG_FUNCTION_NAME "Function Name"

enum {
    GUI_SUCCESS,
    GUI_NO_LOADER,
    GUI_LOADER_NO_ARCH,
    GUI_LOADER_NO_MEMORY_MAP,
    GUI_LOADER_NO_ENTRIES
};

struct _gui {
    GtkWidget * window;
    GtkWidget * vbox;
    GtkWidget * menu;

    GtkWidget    * functionsView;
    GtkListStore * functionsStore;

    struct _map  * memory_map;
    struct _arch * arch;
};


void gui_load_executable_file (GtkWidget * widget, struct _gui * gui);

#endif