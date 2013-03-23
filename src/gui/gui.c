#include "gui.h"

#include "buffer.h"
#include "function.h"
#include "graph.h"
#include "index.h"
#include "loader.h"
#include "map.h"
#include "queue.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    FUNCTION_FUNCTION,
    FUNCTION_ADDR,
    FUNCTION_NAME,
    FUNCTION_N
};

struct _gui * gui_create ()
{
    struct _gui * gui = (struct _gui *) malloc(sizeof(struct _gui));

    gui->memory_map = map_create();
    gui->arch       = NULL;

    gui->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gui->vbox   = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gui->hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);

    gui->imageScrolled = gtk_scrolled_window_new(NULL, NULL);
    gui->image         = gtk_image_new_from_file(NULL);

    gui->functionsScrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gui->functionsStore = gtk_list_store_new(FUNCTION_N, G_TYPE_POINTER, G_TYPE_STRING, G_TYPE_STRING);
    gui->functionsView  = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gui->functionsStore));

    // treeView stuff
    GtkCellRenderer * renderer;
    GtkTreeViewColumn * column;
    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes(LANG_ADDRESS,
                                                        renderer,
                                                        "text", FUNCTION_ADDR,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, FUNCTION_ADDR);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->functionsView), column);

    renderer = gtk_cell_renderer_text_new();
    column   = gtk_tree_view_column_new_with_attributes(LANG_FUNCTION_NAME,
                                                        renderer,
                                                        "text", FUNCTION_NAME,
                                                        NULL);
    g_object_set(renderer, "font", "monospace", "size-points", 9.0, NULL);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, FUNCTION_NAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(gui->functionsView), column);

    // menu stuff
    gui->menu   = gtk_menu_bar_new();
    GtkWidget * menuItemLoadExecFile = gtk_menu_item_new_with_label(LANG_LOAD_EXEC_FILE);
    GtkWidget * menuItemFile         = gtk_menu_item_new_with_label(LANG_MENU_FILE);
    GtkWidget * menuFile             = gtk_menu_new();

    gtk_container_add(GTK_CONTAINER(menuFile), menuItemLoadExecFile);
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuItemFile), menuFile);
    gtk_container_add(GTK_CONTAINER(gui->menu), menuItemFile);


    // signal stuff
    g_signal_connect(menuItemLoadExecFile,
                     "activate",
                     G_CALLBACK(gui_load_executable_file),
                     gui);

    g_signal_connect(gui->functionsView,
                     "row-activated",
                     G_CALLBACK(gui_function_activated),
                     gui);

    // layout stuff
    gtk_box_pack_start(GTK_BOX(gui->vbox), gui->menu, FALSE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(gui->functionsScrolledWindow), gui->functionsView);
    gtk_paned_add1(GTK_PANED(gui->hpaned), gui->functionsScrolledWindow);

    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(gui->imageScrolled), gui->image);
    gtk_paned_add2(GTK_PANED(gui->hpaned), gui->imageScrolled);
    /*
    gtk_box_pack_start(GTK_BOX(gui->hbox), gui->functionsScrolledWindow, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(gui->hbox), gui->image, FALSE, TRUE, 0);
    */
    gtk_box_pack_start(GTK_BOX(gui->vbox), gui->hpaned, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(gui->window), gui->vbox);

    gtk_window_set_default_size(GTK_WINDOW(gui->window), 800, 600);
    gtk_widget_set_size_request(gui->functionsView, 240, 600);
    gtk_widget_set_size_request(gui->imageScrolled, 560, 600);

    gtk_widget_show_all(gui->window);

    return gui;
}


void gui_delete (struct _gui * gui)
{
    object_delete(gui->memory_map);
    free(gui);
}


int gui_init_from_buf (struct _gui * gui, struct _buffer * buffer)
{
    const struct _loader * loader = loader_select(buffer);
    if (loader == NULL)
        return GUI_NO_LOADER;

    object_delete(gui->memory_map);
    gui->memory_map = loader->memory_map(buffer);
    if (gui->memory_map == NULL)
        return GUI_LOADER_NO_MEMORY_MAP;

    gui->arch = loader->arch(buffer);
    if (gui->arch == NULL)
        return GUI_LOADER_NO_ARCH;

    struct _list * entries = loader->entries(buffer);
    if (entries == NULL)
        return GUI_LOADER_NO_ENTRIES;

    arch_disassemble disassemble = gui->arch->default_dis_option.disassemble;

    struct _queue * queue = queue_create();

    struct _list_it * lit;
    for (lit = list_iterator(entries); lit != NULL; lit = lit->next) {
        queue_push(queue, lit->data);
    }

    struct _map * added = map_create();

    while (queue->size > 0) {
        struct _index * index = queue_peek(queue);
        if (map_fetch(added, index->index)) {
            queue_pop(queue);
            continue;
        }

        map_insert(added, index->index, index);

        struct _graph * graph = disassemble(gui->memory_map, index->index);
        struct _list * call_dests = ins_graph_to_list_index_call_dest(graph);
        for (lit = list_iterator(call_dests); lit != NULL; lit = lit->next)
            queue_push(queue, lit->data);
        object_delete(call_dests);

        struct _function * function = function_create(index->index,
                                                      graph,
                                                      loader->label(buffer, index->index));
        object_delete(graph);

        GtkTreeIter treeIter;
        char addrText[64];
        snprintf(addrText, 64, "%04llx", (unsigned long long) function->address);

        gtk_list_store_append(gui->functionsStore, &treeIter);
        gtk_list_store_set(gui->functionsStore, &treeIter,
                           FUNCTION_FUNCTION, function,
                           FUNCTION_ADDR,     addrText,
                           FUNCTION_NAME,     function->name,
                           -1);

        queue_pop(queue);
    }

    objects_delete(queue, added, NULL);

    return GUI_SUCCESS;
}


void gui_load_executable_file (GtkWidget * widget, struct _gui * gui)
{
    GtkWidget * dialog;

    dialog = gtk_file_chooser_dialog_new(LANG_LOAD_EXEC_FILE,
                                         GTK_WINDOW(gui->window),
                                         GTK_FILE_CHOOSER_ACTION_OPEN,
                                         GTK_STOCK_CANCEL,
                                         GTK_RESPONSE_CANCEL,
                                         GTK_STOCK_OPEN,
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        const char * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        struct _buffer * buffer = buffer_load_file(filename);
        if (buffer != NULL) {
            int error = gui_init_from_buf(gui, buffer);
            if (error) {
                fprintf(stderr, "gui error %d\n", error);
            }
            object_delete(buffer);
        }
    }

    gtk_widget_destroy(dialog);
}


void gui_function_activated (GtkTreeView * treeView,
                             GtkTreePath * treePath,
                             GtkTreeViewColumn * treeViewColumn,
                             struct _gui * gui)
{
    GtkTreeIter treeIter;
    struct _function * function;

    gtk_tree_model_get_iter(GTK_TREE_MODEL(gui->functionsStore),
                            &treeIter,
                            treePath);
    gtk_tree_model_get(GTK_TREE_MODEL(gui->functionsStore),
                       &treeIter,
                       FUNCTION_FUNCTION, &function,
                       -1);

    char * dotstr = ins_graph_to_dot_string(function->graph);

    FILE * fh = fopen("/tmp/rdis2", "w");
    if (fh == NULL) {
        free(dotstr);
        return;
    }

    fwrite(dotstr, 1, strlen(dotstr), fh);
    fclose(fh);

    free(dotstr);

    system("dot -Tpng -O /tmp/rdis2");

    gtk_image_set_from_file(GTK_IMAGE(gui->image), "/tmp/rdis2.png");
}


int main (int argc, char * argv[])
{
    gtk_init(&argc, &argv);

    struct _gui * gui = gui_create();

    struct _buffer * buffer = buffer_load_file("/home/endeavor/code/hsvm/assembler");
    if (buffer != NULL) {
        int error = gui_init_from_buf(gui, buffer);
        if (error) {
            fprintf(stderr, "gui error %d\n", error);
        }
        object_delete(buffer);
    }

    gtk_main ();

    gui_delete(gui);

    return 0;
}