/*
HOST_URL.C

halo:// links opened with the app. macOS routes them here as an Apple
Event (CFBundleURLTypes in the bundle); SDL receives the same event but
posts SDL_EVENT_DROP_FILE with no data, since it reads a URL as a file to
open and a custom scheme has no path.

The guest already watches for a link in join_link.txt once a second
(p2p.c, poll_invite_file), which is how the Android app hands one over, so
the handler writes the file and leaves the rest alone. Written to a
temporary name and renamed, as the guest renames it away to claim it: a
half-written file is never seen.
*/

#include "host.h"

#include <ApplicationServices/ApplicationServices.h>
#include <stdio.h>
#include <string.h>

/* host_main.c: 0 is the data root, where the guest looks for the link */
void host_android_path(int which, char *buffer, uint32_t size);

static OSErr url_opened(const AppleEvent *event, AppleEvent *reply, SRefCon context) {
    char url[512], path[4096], temporary[4096];
    DescType type;
    Size length = 0;
    FILE *file;

    (void)reply;
    (void)context;
    if (AEGetParamPtr(event, keyDirectObject, typeUTF8Text, &type, url, sizeof(url) - 1, &length) != noErr)
        return errAEEventNotHandled;
    url[length] = '\0';
    if (strncmp(url, "halo://", 7) != 0)
        return errAEEventNotHandled;
    host_android_path(0, path, sizeof(path));
    snprintf(temporary, sizeof(temporary), "%s/join_link.writing", path);
    snprintf(path, sizeof(path), "%s/join_link.txt", path);
    file = fopen(temporary, "w");
    if (!file)
        return errAEEventNotHandled;
    fputs(url, file);
    fclose(file);
    if (rename(temporary, path) != 0)
        remove(temporary);
    host_logf(HOST_LOG_INFO, "Internet play: an invite was opened with the app");
    return noErr;
}

void host_url_listen(void) {
    AEInstallEventHandler(kInternetEventClass, kAEGetURL,
        NewAEEventHandlerUPP(url_opened), 0, false);
}
