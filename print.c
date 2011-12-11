/* See LICENSE file for license and copyright information */

#include "print.h"
#include "document.h"
#include "render.h"

#include <girara/utils.h>
#include <girara/statusbar.h>

void
print(zathura_t* zathura)
{
  g_return_if_fail(zathura           != NULL);
  g_return_if_fail(zathura->document != NULL);

  GtkPrintOperation* print_operation = gtk_print_operation_new();

  /* print operation settings */
  if (zathura->print.settings != NULL) {
    gtk_print_operation_set_print_settings(print_operation, zathura->print.settings);
  }

  if (zathura->print.page_setup != NULL) {
    gtk_print_operation_set_default_page_setup(print_operation, zathura->print.page_setup);
  }

  gtk_print_operation_set_allow_async(print_operation, TRUE);
  gtk_print_operation_set_n_pages(print_operation, zathura->document->number_of_pages);
  gtk_print_operation_set_current_page(print_operation, zathura->document->current_page_number);

  /* print operation signals */
  g_signal_connect(print_operation, "draw-page", G_CALLBACK(cb_print_draw_page), zathura);
  g_signal_connect(print_operation, "end-print", G_CALLBACK(cb_print_end),       zathura);

  /* print */
  GtkPrintOperationResult result = gtk_print_operation_run(print_operation,
      GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, NULL, NULL);

  if (result == GTK_PRINT_OPERATION_RESULT_APPLY) {
    if (zathura->print.settings != NULL) {
      g_object_unref(zathura->print.settings);
    }
    if (zathura->print.page_setup != NULL) {
      g_object_unref(zathura->print.page_setup);
    }

    /* save previous settings */
    zathura->print.settings   = g_object_ref(gtk_print_operation_get_print_settings(print_operation));
    zathura->print.page_setup = g_object_ref(gtk_print_operation_get_default_page_setup(print_operation));
  } else if (result == GTK_PRINT_OPERATION_RESULT_ERROR) {
    girara_error("Error occured while printing progress");
  }

  g_object_unref(print_operation);
}

void
cb_print_end(GtkPrintOperation* UNUSED(print_operation), GtkPrintContext*
    UNUSED(context), zathura_t* zathura)
{
  if (zathura == NULL || zathura->ui.session == NULL || zathura->document == NULL
      || zathura->document->file_path == NULL) {
    return;
  }

  girara_statusbar_item_set_text(zathura->ui.session,
      zathura->ui.statusbar.file, zathura->document->file_path);
}

void
cb_print_draw_page(GtkPrintOperation* UNUSED(print_operation), GtkPrintContext*
    context, gint page_number, zathura_t* zathura)
{
  if (context == NULL || zathura == NULL || zathura->document->pages == NULL ||
      zathura->ui.session == NULL || zathura->ui.statusbar.file == NULL) {
    return;
  }

  /* update statusbar */
  char* tmp = g_strdup_printf("Printing %d...", page_number);
  girara_statusbar_item_set_text(zathura->ui.session,
      zathura->ui.statusbar.file, tmp);
  g_free(tmp);

  /* render page */
  cairo_t* cairo           = gtk_print_context_get_cairo_context(context);
  zathura_page_t* page     = zathura->document->pages[page_number];
  if (cairo == NULL || page == NULL) {
    return;
  }

  zathura_page_render(page, cairo);
}
