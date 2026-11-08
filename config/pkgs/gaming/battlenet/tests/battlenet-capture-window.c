#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *runtime_dir(void)
{
    const char *xdg_runtime_dir = g_getenv("XDG_RUNTIME_DIR");
    return xdg_runtime_dir && *xdg_runtime_dir ? xdg_runtime_dir : "/tmp";
}

static char *capture_dir(void)
{
    return g_build_filename(runtime_dir(), "battlenet-proton", "window-capture", NULL);
}

static void write_status(const char *status_path, gboolean ok, const char *message)
{
    char *tmp_path = g_strdup_printf("%s.tmp", status_path);
    FILE *file;

    if (!(file = fopen(tmp_path, "w"))) return;
    fprintf(file, "ok=%d\n", ok ? 1 : 0);
    fprintf(file, "message=%s\n", message ? message : "");
    fclose(file);
    rename(tmp_path, status_path);
    g_free(tmp_path);
}

static gboolean read_request(const char *request_path, char **handle, char **output, char **error)
{
    gchar *contents = NULL;
    gchar **lines;

    if (!g_file_get_contents(request_path, &contents, NULL, NULL))
    {
        *error = g_strdup("could not read capture request");
        return FALSE;
    }

    lines = g_strsplit(contents, "\n", 3);
    if (!lines[0] || !*lines[0] || !lines[1] || !*lines[1])
    {
        *error = g_strdup("invalid capture request");
        g_strfreev(lines);
        g_free(contents);
        return FALSE;
    }

    *handle = g_strdup(lines[0]);
    *output = g_strdup(lines[1]);
    g_strfreev(lines);
    g_free(contents);
    return TRUE;
}

static gboolean write_all(int fd, const char *path, char **error)
{
    char buffer[1024 * 1024];
    int out;
    ssize_t count;

    if ((out = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644)) < 0)
    {
        *error = g_strdup_printf("could not open output: %s", g_strerror(errno));
        return FALSE;
    }

    while ((count = read(fd, buffer, sizeof(buffer))) > 0)
    {
        char *cursor = buffer;
        ssize_t remaining = count;

        while (remaining > 0)
        {
            ssize_t written = write(out, cursor, remaining);
            if (written < 0)
            {
                *error = g_strdup_printf("could not write output: %s", g_strerror(errno));
                close(out);
                return FALSE;
            }
            cursor += written;
            remaining -= written;
        }
    }

    close(out);
    if (count < 0)
    {
        *error = g_strdup_printf("could not read screenshot pipe: %s", g_strerror(errno));
        return FALSE;
    }

    return TRUE;
}

struct read_task
{
    int fd;
    const char *path;
    gboolean ok;
    char *error;
};

static gpointer read_capture(gpointer data)
{
    struct read_task *task = data;

    task->ok = write_all(task->fd, task->path, &task->error);
    close(task->fd);
    return NULL;
}

int main(void)
{
    g_autofree char *dir = capture_dir();
    g_autofree char *request_path = g_build_filename(dir, "request", NULL);
    g_autofree char *status_path = g_build_filename(dir, "status", NULL);
    g_autofree char *handle = NULL;
    g_autofree char *output = NULL;
    g_autofree char *error_message = NULL;
    g_autoptr(GError) error = NULL;
    GDBusConnection *connection;
    GUnixFDList *fd_list;
    GVariantBuilder options;
    struct read_task task = {0};
    GThread *reader;
    int pipe_fds[2] = {-1, -1};
    int fd_index;

    g_mkdir_with_parents(dir, 0700);

    if (!read_request(request_path, &handle, &output, &error_message))
    {
        write_status(status_path, FALSE, error_message);
        return 1;
    }

    if (pipe(pipe_fds) < 0)
    {
        write_status(status_path, FALSE, g_strerror(errno));
        return 1;
    }

    connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!connection)
    {
        write_status(status_path, FALSE, error ? error->message : "could not connect to session bus");
        return 1;
    }

    fd_list = g_unix_fd_list_new();
    fd_index = g_unix_fd_list_append(fd_list, pipe_fds[1], &error);
    close(pipe_fds[1]);
    pipe_fds[1] = -1;
    if (fd_index < 0)
    {
        write_status(status_path, FALSE, error ? error->message : "could not append screenshot fd");
        close(pipe_fds[0]);
        g_object_unref(fd_list);
        g_object_unref(connection);
        return 1;
    }

    g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&options, "{sv}", "include-cursor", g_variant_new_boolean(FALSE));
    g_variant_builder_add(&options, "{sv}", "include-decoration", g_variant_new_boolean(FALSE));
    g_variant_builder_add(&options, "{sv}", "native-resolution", g_variant_new_boolean(TRUE));

    task.fd = pipe_fds[0];
    task.path = output;
    reader = g_thread_new("capture-reader", read_capture, &task);

    g_dbus_connection_call_with_unix_fd_list_sync(connection,
                                                  "org.kde.KWin",
                                                  "/org/kde/KWin/ScreenShot2",
                                                  "org.kde.KWin.ScreenShot2",
                                                  "CaptureWindow",
                                                  g_variant_new("(sa{sv}h)", handle, &options, fd_index),
                                                  G_VARIANT_TYPE("(a{sv})"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  -1,
                                                  fd_list,
                                                  NULL,
                                                  NULL,
                                                  &error);
    g_object_unref(fd_list);

    if (error)
    {
        write_status(status_path, FALSE, error->message);
        close(pipe_fds[0]);
        g_thread_join(reader);
        g_object_unref(connection);
        return 1;
    }

    g_thread_join(reader);
    if (!task.ok)
    {
        write_status(status_path, FALSE, task.error);
        g_free(task.error);
        g_object_unref(connection);
        return 1;
    }
    g_free(task.error);

    write_status(status_path, TRUE, "kwin-window");
    g_object_unref(connection);
    return 0;
}
