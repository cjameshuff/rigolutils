
#ifndef GTKHELPERS_H
#define GTKHELPERS_H

#include <gtk/gtk.h>
#include <string>
#include <stack>


//******************************************************************************
std::string OpenFileDlg(GtkWidget * window)
{
    std::string path;
    GtkWidget * dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        path = filename;
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    return path;
}


std::string SaveFileDlg(GtkWidget * window, const std::string & existPath = "")
{
    std::string path;
    GtkWidget * dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
    if(existPath == "")
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "Untitled document");
    else
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), existPath.c_str());
    
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        path = filename;
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
    return path;
}


//******************************************************************************
class WidgetDict {
    std::map<std::string, GtkWidget *> widgets;
  public:
    WidgetDict() {}
    ~WidgetDict() {
        std::map<std::string, GtkWidget *>::iterator w;
        for(w = widgets.begin(); w != widgets.end(); ++w)
            g_object_unref(w->second);
    }
    // Does not check for duplicate keys!
    void Add(const std::string & key, GtkWidget * widget) {
        g_object_ref(widget);
        widgets[key] = widget;
    }
    GtkWidget * Get(const std::string & key) {
        std::map<std::string, GtkWidget *>::iterator w = widgets.find(key);
        return (w == widgets.end())? NULL : w->second;
    }
};


//******************************************************************************

class MenuBuilder {
    std::stack<GMenu *> menuStk;
  public:
    MenuBuilder() {Reset();}
    
    void Reset() {menuStk.push(g_menu_new());}
    
    void Item(const char * label, const char * action, const char * accel = NULL) {
        GMenuItem * item = g_menu_item_new(label, action);
        if(accel) {
            GVariant * keyCode = g_variant_new_string(accel);
            g_menu_item_set_attribute_value(item, "accel", keyCode);
        }
        g_menu_append_item(menuStk.top(), item);
    }
    void StartSection(const char * label) {
        GMenu * top = menuStk.top();
        menuStk.push(g_menu_new());
        g_menu_append_section(top, label, G_MENU_MODEL(menuStk.top()));
    }
    void EndSection() {menuStk.pop();}
    
    void StartSubmenu(const char * label) {
        GMenu * top = menuStk.top();
        menuStk.push(g_menu_new());
        g_menu_append_submenu(top, label, G_MENU_MODEL(menuStk.top()));
    }
    void EndSubmenu() {menuStk.pop();}
    
    GMenu * Close() {
        GMenu * top = menuStk.top();
        menuStk.pop();
        return top;
    }
};
class MB_Section {
    MenuBuilder & mb;
  public:
    MB_Section(MenuBuilder & _mb, const char * label): mb(_mb) {mb.StartSection(label);}
    ~MB_Section() {mb.EndSection();}
};
class MB_Submenu {
    MenuBuilder & mb;
  public:
    MB_Submenu(MenuBuilder & _mb, const char * label): mb(_mb) {mb.StartSubmenu(label);}
    ~MB_Submenu() {mb.EndSubmenu();}
};


//******************************************************************************
#endif // GTKHELPERS_H
