#include "room_network_owner.h"
#include "room_wifi_identity_provider.h"

bool room_network_owner_prepare(unsigned key_id,
    const room_network_setup_backend *backend, room_secure_status *status) {
    room_enroll_profile profile={0};
    room_secure_status result=ROOM_SEC_UNAVAILABLE;
    bool ok=false;
    if (status) *status=result;
    if (key_id>=6u) return false;
    if (backend && (!backend->io.open || !backend->io.receive ||
        !backend->io.send || !backend->io.close || !backend->close_until)) return false;
    if (!room_secure_init(key_id)) return false;
    result=room_secure_load(&profile);
    if (status) *status=result;
    if (result!=ROOM_SEC_VALID && result!=ROOM_SEC_MISSING) goto done;
    if (!backend) {
        if (result==ROOM_SEC_VALID)
            ok=room_wifi_idf_init(profile.ssid,profile.ssid_len,profile.pass,profile.pass_len);
        goto done;
    }
    room_wifi_apsta_provider provider={0};
    if (!room_wifi_identity_provider_init(&provider)) goto done;
    if (result==ROOM_SEC_VALID) {
        if (!room_wifi_idf_init(profile.ssid,profile.ssid_len,profile.pass,profile.pass_len)) goto done;
        if (!room_wifi_apsta_link_bind(provider)) goto done;
    } else if (!room_wifi_apsta_link_init_missing(provider)) goto done;
    /* Do not step the owner until the transport has been attached. No legacy
     * close fallback; the existing absolute deadline is mandatory. */
    ok=room_wifi_apsta_link_attach_io_until(backend->io,backend->close_until);
done:
    enroll_wipe(&profile,sizeof(profile));
    return ok;
}
