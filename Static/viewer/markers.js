import {get_coords, get_map_thirds} from "./helpers/location_helper.js";

const src_icon_url = "/viewer/icons/marker-icon-green.png";
const dst_icon_url = "/viewer/icons/marker-icon-red.png";

let src_marker = null;
let dst_marker = null;

function get_icon(icon_url) {
    return L.icon({
        iconUrl: icon_url,
        iconAnchor: [12, 41],
        popupAnchor: [0, -41],
    });
}

function create_marker(icon_url, coordinates) {
    var icon = get_icon(icon_url);
    var marker = L.marker(coordinates, { draggable: true, icon: icon });
    return marker;
}


export function create_markers(the_map, src, dst) {
    // by default, initial markers are always at 1/3 and 2/3 of the displayed map :
    let [default_src, default_dst] = get_map_thirds(the_map);
    if (src === null || src === undefined) { src = default_src; }
    if (dst === null || dst === undefined) { dst = default_dst; }

    src_marker = create_marker(src_icon_url, src);
    dst_marker = create_marker(dst_icon_url, dst);

    src_marker.addTo(the_map);
    dst_marker.addTo(the_map);

    return [src_marker, dst_marker];
}


export function handle_markers_changed(the_map, src_marker, dst_marker, loadGeojson, infoControl, inputsControl, update_url) {
    const src_lat = src_marker.getLatLng().lat.toFixed(6);
    const src_lng = src_marker.getLatLng().lng.toFixed(6);
    const dst_lat = dst_marker.getLatLng().lat.toFixed(6);
    const dst_lng = dst_marker.getLatLng().lng.toFixed(6);
    const departure_time = inputsControl.valueAsNbSeconds();
    update_url({
        src: [src_lat, src_lng],
        dst: [dst_lat, dst_lng],
        time: departure_time,
    });

    const backend_url = `${window.location.origin}/journey_between_locations?src=${get_coords(src_marker)}&dst=${get_coords(dst_marker)}&departure-time=${departure_time}`;
    console.log("about to request backend_url = " + backend_url);
    fetch(backend_url)
        .then(data=>data.json())
        .then(jsondata=>loadGeojson(the_map, backend_url, jsondata["http_status"], jsondata["error_msg"], jsondata["response"], infoControl))
        .catch(error=>console.log(error));
};
