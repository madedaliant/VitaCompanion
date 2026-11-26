
#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>
#include <psp2/kernel/threadmgr.h>

int main(void)
{
    vita2d_init();
    vita2d_set_clear_color(RGBA8(30, 30, 30, 255));

    json_t *root = json_object();

    if (root == NULL) {
        printf("Gagal membuat JSON\n");
        sceKernelDelayThread(1000000);
        return 1;
    }

    json_object_set_new(root, "name", json_string("VitaCompanion"));
    json_object_set_new(root, "version", json_integer(1));

    char *json_text = json_dumps(root, JSON_INDENT(2));

    if (json_text != NULL) {
    printf("%s\n", json_text);

    if (json_text[0] == '{') {
        printf("JSON valid dan berhasil dibuat!\n");
    }

    free(json_text);
}

    if (json_text != NULL) {
        printf("%s\n", json_text);
        free(json_text);
    }

    json_decref(root);

    printf("Jansson berhasil!\n");

    // Tahan aplikasi agar tidak langsung keluar.
    while (1) {
        vita2d_start_drawing();
        vita2d_clear_screen();

        vita2d_draw_rectangle(
            40, 40, 880, 460,
            RGBA8(45, 45, 45, 255)
        );

        vita2d_end_drawing();
        vita2d_swap_buffers();

        sceKernelDelayThread(1000000);
    }

    return 0;
}
