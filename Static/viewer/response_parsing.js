import {get_next_monday_with_this_time, format_for_routemm, format_for_navitia} from "./helpers/datetime_helper.js";
import {get_coords, to_m_per_s} from "./helpers/location_helper.js";

export function parse_used_coords(response) {
    // A bit of explanation : currently, the unrestricted walking POC can only compute journey
    // from a given STOP (and not from a location). Thus, when requested with a location, the
    // location is snapped on the closest stop, and we IGNORE the time needed to reach this stop
    // from the requested location.
    //
    // In order to get a routemm journey that is comparable to this, we must use the snapped positions.
    // We extract them from the first and last geojson feature, which are the departure/arrival points :
    const first_feature_type = response["geojson"]["features"][0]["geometry"]["type"];
    if (first_feature_type !== "Point") { throw `unexpected first feature type : ${first_feature_type}`; }

    const nb_features = response["geojson"]["features"].length;
    const last_feature_type = response["geojson"]["features"][nb_features-1]["geometry"]["type"];
    if (last_feature_type !== "Point") { throw `unexpected last feature type : ${last_feature_type}`; }

    const used_src_coords = response["geojson"]["features"][0]["geometry"]["coordinates"];
    const used_dst_coords = response["geojson"]["features"][nb_features-1]["geometry"]["coordinates"];
    return [used_src_coords, used_dst_coords];
}

export function parse_times_of_first_leg(response) {
    // Each leg of the journey has two times :
    //      - leg's start_time = time at which the leg begins (for the first leg, it is the journey departure_time)
    //      - leg's departure_time = time at which we begin to travel
    // Between them, there can be some waiting time.
    //
    // NOTE : the first leg is the SECOND feature of the response (the first one is the starting point)
    const second_feature = response["geojson"]["features"][1]
    const second_feature_type = second_feature["geometry"]["type"];
    if (second_feature_type !== "LineString") { throw `unexpected second feature type : ${second_feature_type}`; }
    const props = second_feature["properties"];
    return [props["departure_time_str"], props["departure_time"], props["waiting_duration_str"], props["waiting_duration"]]
}

export function get_navitia_url_computer(response) {

    function computer(speed_kmh) {

        const [used_src_coords, used_dst_coords] = parse_used_coords(response)
        const departure_time = response["journey_params"]["departure_time"];
        const departure_time_navitia = format_for_navitia(get_next_monday_with_this_time(departure_time));

        // example of navitia URL :
        // https://canaltp.github.io/navitia-playground/play.html?request=https%3A%2F%2Fapi.navitia.io%2Fv1%2Fjourneys%3Ffrom%3D2.289919%253B48.847920%26to%3D2.389442%253B48.852088%26first_section_mode%25255B%25255D%3Dwalking%26last_section_mode%25255B%25255D%3Dwalking%26datetime_represents%3Ddeparture%26datetime%3D20201214T141500%26walking_speed%3D4.17
        //
        // request parameter is actually a percent-encoded URL :
        // https%3A%2F%2Fapi.navitia.io%2Fv1%2Fjourneys%3Ffrom%3D2.289919%253B48.847920%26to%3D2.389442%253B48.852088%26first_section_mode%25255B%25255D%3Dwalking%26last_section_mode%25255B%25255D%3Dwalking%26datetime_represents%3Ddeparture%26datetime%3D20201214T141500%26walking_speed%3D4.17
        // decoded URL :
        // https://api.navitia.io/v1/journeys?from=2.289919%3B48.847920&to=2.389442%3B48.852088&first_section_mode%255B%255D=walking&last_section_mode%255B%255D=walking&datetime_represents=departure&datetime=20201214T141500&walking_speed=4.17
        //
        // (note that this URL *also* has some percent encoded characters, namely first_section_mode%5B%5D)
        //
        // Thus, to build navitia URL, we first build request-URL, then encoded it in the full URL :
        //
        // NOTE : some useful percent-encoding :
        //   %3A  :
        //   %2F  /
        //   %3D  =
        //   %3B  ;
        //   %26  &
        //   %25  %
        //   %5B  [
        //   %5D  ]

        const request_url = new URL("https://api.navitia.io/v1/journeys");
        const params = new URLSearchParams();
        const from_param = get_coords(used_src_coords, true).replace(",", ";");
        const to_param = get_coords(used_dst_coords, true).replace(",", ";");
        params.set("from", from_param);
        params.set("to", to_param);
        params.set("first_section_mode%5B%5D", "walking");
        params.set("last_section_mode%5B%5D", "walking");
        params.set("datetime_represents", "departure");
        params.set("datetime", departure_time_navitia);
        params.set("walking_speed", to_m_per_s(speed_kmh));
        request_url.search = params.toString();

        const full_url = `https://canaltp.github.io/navitia-playground/play.html?request=${encodeURIComponent(request_url.toString())}`;
        return full_url;
    }
    return computer;
}


export function compute_routemm_url(the_map, response) {
    let maplat = the_map.getCenter().lat.toFixed(6);
    let maplng = the_map.getCenter().lng.toFixed(6);
    let mapzoom = the_map.getZoom();

    const [used_src_coords, used_dst_coords] = parse_used_coords(response)
    const departure_time = response["journey_params"]["departure_time"];

    // now that we have all parameters, we can build the URL :
    const routemm_url = format_routemm_url(maplat, maplng, mapzoom, used_src_coords, used_dst_coords, departure_time);
    return routemm_url;
}


function format_routemm_url(maplat, maplng, mapzoom, src_coords, dst_coords, departure_time) {
    let params = new URLSearchParams();
    params.set("zoom", mapzoom)
    params.set("lng", maplng)
    params.set("lat", maplat)
    params.set("from", get_coords(src_coords, true));
    params.set("to", get_coords(dst_coords, true));
    params.set("datetime", format_for_routemm(get_next_monday_with_this_time(departure_time)));

    params.set("departure", true)
    params.set("providers", "tc");
    params.set("walk_speed", "normal");

    let url = new URL("http://routemm.mappysnap.net/multipath/viewer/map.html");
    url.hash = params.toString();
    return url;
}

export function parse_walk_durations(response) {
    const features = response["geojson"]["features"];
    const legs = features.filter(feature => feature["geometry"]["type"] === "LineString");

    const get_duration = leg => leg["properties"]["full_duration"];
    const is_walk = leg => leg["properties"]["type"] === "walk";

    let first_walk = 0;
    if (is_walk(legs[0])) { first_walk = get_duration(legs[0]); }

    let final_walk = 0;
    if (is_walk(legs[legs.length-1])) { final_walk = get_duration(legs[legs.length-1]); }

    let total_walk = 0;
    legs.filter(is_walk).forEach(walk_leg =>  {total_walk += get_duration(walk_leg);});

    // here, total_walk is the sum of ALL walks, including first and final.
    // For middle_walks, we have to substract first and final :
    let middle_walks = total_walk - first_walk - final_walk;

    return [first_walk, middle_walks, final_walk, total_walk];
}
