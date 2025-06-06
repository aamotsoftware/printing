#include <gtk/gtk.h>
#include <stdio.h>

char *file_content = NULL;

#define MARGIN 50
#define LINE_HEIGHT 14

void print_text_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        g_printerr("Cannot open file: %s\n", filename);
        return;
    }
    fclose(file);
}

static void begin_print(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data) {
    gtk_print_operation_set_n_pages(operation, 1);
}

static void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, gpointer user_data) {
    cairo_t *cr = gtk_print_context_get_cairo_context(context);
    if (file_content) {
        cairo_move_to(cr, 100, 100);
        cairo_show_text(cr, file_content);
    }
}

static void run_print_operation(GtkWindow *parent) {
    GtkPrintOperation *print = gtk_print_operation_new();
    g_signal_connect(print, "begin-print", G_CALLBACK(begin_print), NULL);
    g_signal_connect(print, "draw-page", G_CALLBACK(draw_page), NULL);

    gtk_print_operation_run(print, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, parent, NULL);
    g_object_unref(print);
}

static char *read_file(const char *filename) {
    gchar *contents = NULL;
    gsize length;
    if (g_file_get_contents(filename, &contents, &length, NULL)) {
        return contents;
    }
    return NULL;
}

static void open_file_dialog(GtkApplication *app, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        file_content = read_file(filename);
        if (file_content) {
            run_print_operation(NULL);
            g_free(file_content);
        }
    }
    gtk_widget_destroy(dialog);
}

static void activate(GtkApplication *app, gpointer user_data) {
    open_file_dialog(app, user_data);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        gtk_init(&argc, &argv);
	if (argc > 1) {
	  for (int i = 1; i < argc; i++) {
	    file_content = read_file(argv[i]);
	    if (file_content) {
	      run_print_operation(NULL);
	      g_free(file_content);
	    }
	  }
	  return 0;
        } else {
	  fprintf(stderr, "Error: Cannot read file '%s'\n", argv[1]);
	  return 1;
        }
    } else {
      GtkApplication *app = gtk_application_new("com.aamotinnovation.printapp", G_APPLICATION_DEFAULT_FLAGS);
        g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
        int status = g_application_run(G_APPLICATION(app), argc, argv);
        g_object_unref(app);
        return status;
    }
}
