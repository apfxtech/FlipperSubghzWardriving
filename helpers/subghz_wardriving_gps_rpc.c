#include "subghz_wardriving_gps.h"

#define RPC_STREAM_FREQ       10
#define RPC_POLL_PERIOD_MS    1000
#define RPC_STREAM_TIMEOUT_MS 3000

static float subghz_gps_rpc_deg2rad(float deg) {
    return (deg * M_PI / 180);
}

static float subghz_gps_rpc_calc_distance(float lat1d, float lon1d, float lat2d, float lon2d) {
    float lat1r, lon1r, lat2r, lon2r;
    double u, v;
    lat1r = subghz_gps_rpc_deg2rad(lat1d);
    lon1r = subghz_gps_rpc_deg2rad(lon1d);
    lat2r = subghz_gps_rpc_deg2rad(lat2d);
    lon2r = subghz_gps_rpc_deg2rad(lon2d);
    u = sin((lat2r - lat1r) / 2);
    v = sin((lon2r - lon1r) / 2);
    return 2 * 6371 * asin(sqrt(u * u + cos(lat1r) * cos(lat2r) * v * v));
}

static float subghz_gps_rpc_calc_angle(float lat1, float lon1, float lat2, float lon2) {
    return atan2(lat1 - lat2, lon1 - lon2) * 180 / (double)M_PI;
}

static void subghz_gps_rpc_cat_realtime(
    SubGhzGPS* subghz_gps,
    FuriString* descr,
    float latitude,
    float longitude) {
    float distance = subghz_gps_rpc_calc_distance(
        latitude, longitude, subghz_gps->latitude, subghz_gps->longitude);
    float angle = subghz_gps_rpc_calc_angle(
        latitude, longitude, subghz_gps->latitude, subghz_gps->longitude);

    char* angle_str = "?";
    if(angle > -22.5 && angle <= 22.5) {
        angle_str = "E";
    } else if(angle > 22.5 && angle <= 67.5) {
        angle_str = "NE";
    } else if(angle > 67.5 && angle <= 112.5) {
        angle_str = "N";
    } else if(angle > 112.5 && angle <= 157.5) {
        angle_str = "NW";
    } else if(angle < -22.5 && angle >= -67.5) {
        angle_str = "SE";
    } else if(angle < -67.5 && angle >= -112.5) {
        angle_str = "S";
    } else if(angle < -112.5 && angle >= -157.5) {
        angle_str = "SW";
    } else if(angle < -157.5 || angle >= 157.5) {
        angle_str = "W";
    }

    furi_string_cat_printf(
        descr,
        "Realtime:  Sats: %d\r\n"
        "Distance: %.2f%s Dir: %s\r\n"
        "GPS time: %02d:%02d:%02d UTC",
        subghz_gps->satellites,
        (double)(subghz_gps->satellites > 0 ? distance > 1 ? distance : distance * 1000 : 0),
        distance > 1 ? "km" : "m",
        angle_str,
        subghz_gps->fix_hour,
        subghz_gps->fix_minute,
        subghz_gps->fix_second);
}

static void subghz_gps_rpc_callback(GpsStatus status, const GpsLocation* location, void* context) {
    SubGhzGPS* subghz_gps = context;
    subghz_gps->rpc_last_rx = furi_get_tick();

    if(status == GpsStatusOk && location) {
        subghz_gps->latitude = location->latitude * 1e-7f;
        subghz_gps->longitude = location->longitude * 1e-7f;
        subghz_gps->satellites = location->satellites;
        DateTime datetime;
        furi_hal_rtc_get_datetime(&datetime);
        subghz_gps->fix_hour = datetime.hour;
        subghz_gps->fix_minute = datetime.minute;
        subghz_gps->fix_second = datetime.second;
    } else {
        subghz_gps->latitude = NAN;
        subghz_gps->longitude = NAN;
        subghz_gps->satellites = 0;
        subghz_gps->fix_hour = 0;
        subghz_gps->fix_minute = 0;
        subghz_gps->fix_second = 0;
    }
}

static void subghz_gps_rpc_poll(void* context) {
    SubGhzGPS* subghz_gps = context;
    if(furi_get_tick() - subghz_gps->rpc_last_rx >= furi_ms_to_ticks(RPC_STREAM_TIMEOUT_MS)) {
        gps_request_stream(subghz_gps->rpc, RPC_STREAM_FREQ);
    }
}

SubGhzGPS* subghz_gps_rpc_start(void) {
    SubGhzGPS* subghz_gps = malloc(sizeof(SubGhzGPS));
    memset(subghz_gps, 0, sizeof(SubGhzGPS));

    subghz_gps->plugin_app = NULL;
    subghz_gps->protocol = SubGhzGpsProtocolRpc;
    subghz_gps->latitude = NAN;
    subghz_gps->longitude = NAN;
    subghz_gps->cat_realtime = &subghz_gps_rpc_cat_realtime;

    subghz_gps->rpc = furi_record_open(RECORD_GPS);
    gps_set_location_callback(subghz_gps->rpc, subghz_gps_rpc_callback, subghz_gps);

    gps_request_stream(subghz_gps->rpc, RPC_STREAM_FREQ);
    subghz_gps->rpc_timer =
        furi_timer_alloc(subghz_gps_rpc_poll, FuriTimerTypePeriodic, subghz_gps);
    furi_timer_start(subghz_gps->rpc_timer, furi_ms_to_ticks(RPC_POLL_PERIOD_MS));

    return subghz_gps;
}

void subghz_gps_rpc_stop(SubGhzGPS* subghz_gps) {
    furi_assert(subghz_gps);

    furi_timer_stop(subghz_gps->rpc_timer);
    furi_timer_free(subghz_gps->rpc_timer);

    gps_stop_stream(subghz_gps->rpc);
    gps_set_location_callback(subghz_gps->rpc, NULL, NULL);
    furi_record_close(RECORD_GPS);

    free(subghz_gps);
}
