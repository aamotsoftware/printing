#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <stdio.h>

#define MARGIN 50
#define LINE_HEIGHT 18  // Adjusted for font size

GPtrArray *lines = NULL;
int lines_per_page = 0;
gboolean use_dialog = TRUE;  // Set to FALSE to print directly

// Read lines from a file
static gboolean read_lines_from_file(const char *filename) {
    gchar *contents = NULL;
    gsize length;
    if (!g_file_get_contents(filename, &contents, &length, NULL)) return FALSE;

    if (lines) g_ptr_array_free(lines, TRUE);
    lines = g_ptr_array_new_with_free_func(g_free);

    gchar **raw_lines = g_strsplit(contents, "\n", -1);
    for (int i = 0; raw_lines[i]; i++) {
        g_ptr_array_add(lines, g_strdup(raw_lines[i]));
    }
    g_strfreev(raw_lines);
    g_free(contents);
    return TRUE;
}

// Determine number of pages
static void begin_print(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data) {
    double height = gtk_print_context_get_height(context);
    lines_per_page = (int)((height - 2 * MARGIN) / LINE_HEIGHT);
    int total_lines = lines ? lines->len : 0;
    int n_pages = (total_lines + lines_per_page - 1) / lines_per_page;
    gtk_print_operation_set_n_pages(operation, n_pages);
}

// Draw one page
static void draw_page(GtkPrintOperation *operation, GtkPrintContext *context, int page_nr, gpointer user_data) {
    cairo_t *cr = gtk_print_context_get_cairo_context(context);
    PangoLayout *layout = gtk_print_context_create_pango_layout(context);
    PangoFontDescription *font_desc = pango_font_description_from_string("Monospace 10");
    pango_layout_set_font_description(layout, font_desc);

    double width = gtk_print_context_get_width(context);
    double height = gtk_print_context_get_height(context);
    int y = MARGIN;

    int start = page_nr * lines_per_page;
    int end = MIN(start + lines_per_page, lines->len);

    // Header
    cairo_move_to(cr, MARGIN, y);
    pango_layout_set_text(layout, "Aamot Innovation - PrintApp", -1);
    pango_cairo_show_layout(cr, layout);
    y += LINE_HEIGHT * 2;

    // Text lines with wrapping
    for (int i = start; i < end; i++) {
        const char *line = g_ptr_array_index(lines, i);
        pango_layout_set_width(layout, (width - 2 * MARGIN) * PANGO_SCALE);
        pango_layout_set_text(layout, line, -1);

        cairo_move_to(cr, MARGIN, y);
        pango_cairo_show_layout(cr, layout);

        int layout_height;
        pango_layout_get_size(layout, NULL, &layout_height);
        y += layout_height / PANGO_SCALE;
    }

    // Footer
    gchar *footer = g_strdup_printf("Page %d", page_nr + 1);
    pango_layout_set_text(layout, footer, -1);
    cairo_move_to(cr, MARGIN, height - MARGIN);
    pango_cairo_show_layout(cr, layout);
    g_free(footer);

    g_object_unref(layout);
    pango_font_description_free(font_desc);
}

// Run the print operation
static void run_print_operation(GtkWindow *parent) {
    GtkPrintOperation *print = gtk_print_operation_new();
    g_signal_connect(print, "begin-print", G_CALLBACK(begin_print), NULL);
    g_signal_connect(print, "draw-page", G_CALLBACK(draw_page), NULL);

    GtkPrintOperationAction action = use_dialog
        ? GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG
        : GTK_PRINT_OPERATION_ACTION_PRINT;

    gtk_print_operation_run(print, action, parent, NULL);
    g_object_unref(print);
}

// File dialog for GUI
static void open_file_dialog(GtkApplication *app, gpointer user_data) {
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open File", NULL,
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (read_lines_from_file(filename)) {
            run_print_operation(NULL);
            g_ptr_array_free(lines, TRUE);
            lines = NULL;
        }
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

// GTK app activation
static void activate(GtkApplication *app, gpointer user_data) {
    open_file_dialog(app, user_data);
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        gtk_init(&argc, &argv);
        for (int i = 1; i < argc; i++) {
            if (read_lines_from_file(argv[i])) {
                run_print_operation(NULL);
                g_ptr_array_free(lines, TRUE);
                lines = NULL;
            } else {
                fprintf(stderr, "Error: Cannot read file '%s'\n", argv[i]);
            }
        }
        return 0;
    } else {
        GtkApplication *app = gtk_application_new("com.aamotinnovation.printapp", G_APPLICATION_DEFAULT_FLAGS);
        g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
        int status = g_application_run(G_APPLICATION(app), argc, argv);
        g_object_unref(app);
        if (lines) {
            g_ptr_array_free(lines, TRUE);
            lines = NULL;
        }
        return status;
    }
}
