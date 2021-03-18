function get_int_param(params, key) {
    if (params.get(key) === null) {
            throw `inexisting param's key : ${key}`;
    }
    const parsed_int = parseInt(params.get(key), 10);
    if (isNaN(parsed_int)) {
        throw `unable to parse int param : ${params.get(key)}`;
    }
    return parsed_int;
}

function get_latlng_param(params, key) {
    if (params.get(key) === null) {
            throw `inexisting param's key : ${key}`;
    }
    const [lat_str,lng_str] = params.get(key).split(",");
    const lat = parseFloat(lat_str);
    const lng = parseFloat(lng_str);
    if (isNaN(lat) || isNaN(lng)) {
        throw `unable to parse coords param : ${params.get(key)}`;
    }
    return [lat, lng];
}

export function parse_url_params(default_center, default_zoom, default_departure_time, default_comparison_port) {
    let center = default_center;
    let zoom = default_zoom;
    let src_coords = null;
    let dst_coords = null;
    let departure_time = default_departure_time;
    let params = (new URL(document.location)).searchParams;
    let is_src_in_url = false;
    let is_dst_in_url = false;
    let is_time_in_url = false;
    let comparison_port = default_comparison_port;
    try { zoom = get_int_param(params, "mapzoom");                               } catch(e) { console.log(e); }
    try { center = get_latlng_param(params, "mapcenter");                        } catch(e) { console.log(e); }
    try { src_coords = get_latlng_param(params, "src"); is_src_in_url = true;    } catch(e) { console.log(e); }
    try { dst_coords = get_latlng_param(params, "dst"); is_dst_in_url = true;    } catch(e) { console.log(e); }
    try { departure_time = get_int_param(params, "time"); is_time_in_url = true; } catch(e) { console.log(e); }
    try { comparison_port = get_int_param(params, "comparisonport");                     } catch(e) { console.log(e); }
    // if user provided src+dst+time in URL, we should compute and display journey at startup :
    const compute_journey_at_startup = is_src_in_url && is_dst_in_url && is_time_in_url;
    return [center, zoom, src_coords, dst_coords, departure_time, compute_journey_at_startup, comparison_port];
}

export function get_updated_url(updated_params) {
    let updated_url = new URL(window.location.href);
    let params = new URLSearchParams(updated_url.search);
    for (const name in updated_params) {
        params.set(name, updated_params[name]);
    }
    updated_url.search = params.toString();
    return updated_url;
}

export function update_window_url(updated_params) {
    // update URL without reloading page :
    const updated_url = get_updated_url(updated_params);
    window.history.replaceState({}, "", updated_url);
}
