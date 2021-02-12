import {seconds_to_duration} from "./helpers/datetime_helper.js";
import {parse_times_of_first_leg, compute_routemm_url, get_navitia_url_computer, parse_walk_durations} from "./response_parsing.js";

var geojsonLayer = null;

const pt_style = {
    "color": "red",
    "weight": 6,
};

const walk_style = {
    "color": "blue",
    "weight": 6,
};


function format_field(key, value) {
    return `<b>${key}=</b>${value}<br/>`;
}

function format_properties(properties) {
    let formatted = "";
    for (const property in properties) {
        formatted += format_field(property, properties[property]);
    }
    return formatted;
}

function pointToLayer(geojsonPoint, latlng) {
    return L.circleMarker(latlng, {
        radius: 12,
        stroke: true,
        color: "green",
        fill: true,
        fillOpacity: 0.5,
        fillColor: "grey",
    });
}

export function loadGeojson(the_map, requested_url, status_code, error_msg, response, infoControl) {

    function onEachFeature(feature, layer) {
        if (feature.geometry === undefined) { throw "feature has no geometry !"; }
        if (feature.properties === undefined) { throw "feature has no properties !"; }
        layer.bindPopup(format_properties(feature.properties));
    }

    const geojsonOptions = {
        onEachFeature: onEachFeature,
        style: function(feature) {
            switch (feature.properties.type) {
                case "pt": return pt_style;
                case "walk":   return walk_style;
            }
        },
        pointToLayer: pointToLayer,
    };

    const routemm_url = compute_routemm_url(the_map, response);
    const navitia_url_computer = get_navitia_url_computer(response);

    // clean previous request :
    if (geojsonLayer != null && the_map.hasLayer(geojsonLayer)) {
        the_map.removeLayer(geojsonLayer);
    }
    infoControl.reset(requested_url, status_code);

    const [travel_begins_at_str, travel_begins_at, initial_wait_str, initial_wait] = parse_times_of_first_leg(response);
    const [first_walk, middle_walks, final_walk, total_walk] = parse_walk_durations(response);

    // then, fill in forms :
    geojsonLayer = L.geoJson(response["geojson"], geojsonOptions).addTo(the_map);

    const response_data = {
        // request :
        src_id: response["journey_params"]["srcid"],
        src_name: response["journey_params"]["srcname"],
        src_snap_distance: response["journey_params"]["src_snap_distance"],
        dst_id: response["journey_params"]["dstid"],
        dst_name: response["journey_params"]["dstname"],
        dst_snap_distance: response["journey_params"]["dst_snap_distance"],
        departure_time_str: response["journey_params"]["departure_time_str"],
        departure_time: response["journey_params"]["departure_time"],

        // response :
        initial_wait_str: initial_wait_str,
        initial_wait: initial_wait,
        travel_begin_str: travel_begins_at_str,
        travel_begin: travel_begins_at,
        eat_str: response["EAT_str"],
        eat: response["EAT"],
        journey_duration_str: response["journey_duration_str"],
        journey_duration: response["journey_duration"],

        // walks :
        first_walk_duration_str: seconds_to_duration(first_walk),
        first_walk_duration: first_walk,
        middle_walks_duration_str: seconds_to_duration(middle_walks),
        middle_walks_duration: middle_walks,
        final_walk_duration_str: seconds_to_duration(final_walk),
        final_walk_duration: final_walk,
        total_walk_duration_str: seconds_to_duration(total_walk),
        total_walk_duration: total_walk,
        walkspeed_km_per_hour: response["walkspeed_km_per_hour"],

        // backend :
        computing_time_microseconds: response["computing_time_microseconds"],
        requested_url: requested_url,
        routemm_url: routemm_url,
        navitia_url_computer: navitia_url_computer,
        status_code: status_code,
        error_msg: error_msg,
    }

    infoControl.update(response_data);

    return routemm_url;
};
