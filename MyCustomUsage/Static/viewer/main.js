import {initMap, on_map_changed} from "./map.js";
import {initControls} from "./controls/init.js";
import {create_markers, handle_markers_changed} from "./markers.js";
import {loadGeojson} from "./geojson.js";
import {parse_url_params, update_url} from "./url.js";

const PARIS = [48.8495, 2.3568];
const BORDEAUX = [44.8357, -0.6048];
const DEFAULT_CENTER = PARIS;
const DEFAULT_ZOOM = 12;
const DEFAULT_DEPARTURE_TIME = 51300;
const DEFAULT_CUSTOM_PORT = 10001;


document.addEventListener("DOMContentLoaded", function() {

    const [center, zoom, src_coords, dst_coords, departure_time, compute_journey_at_startup, custom_port] = parse_url_params(
        DEFAULT_CENTER,
        DEFAULT_ZOOM,
        DEFAULT_DEPARTURE_TIME,
        DEFAULT_CUSTOM_PORT,
    );
    console.log(`parsed center = ${center}`);
    console.log(`parsed zoom = ${zoom}`);
    console.log(`parsed src_coords = ${src_coords}`);
    console.log(`parsed dst_coords = ${dst_coords}`);
    console.log(`parsed departure_time = ${departure_time}`);
    console.log(`parsed custom_port = ${custom_port}`);

    const the_map = initMap(center, zoom);
    const [infoControl, inputsControl] = initControls(the_map, departure_time, custom_port);
    const [src_marker, dst_marker] = create_markers(the_map, src_coords, dst_coords);

    infoControl.url_updater = update_url;

    // shorterns call :
    const handle = (e) => handle_markers_changed(the_map, src_marker, dst_marker, loadGeojson, infoControl, inputsControl, update_url);

    inputsControl.setDepartureTimeChangedHandler(handle);
    inputsControl.setCenterOnParisHandler((e) => { the_map.setView(PARIS, 12); });
    inputsControl.setCenterOnBordeauxHandler((e) => { the_map.setView(BORDEAUX, 12); });

    src_marker.on("dragend", handle);
    dst_marker.on("dragend", handle);

    function on_map_ctrl_click(e, marker) {
        if (!e.originalEvent.ctrlKey) return;  // do nothing if CTRL is not pressed
        marker.setLatLng(e.latlng);
        handle(null);
    };

    // Ctrl + left-click to set source marker
    the_map.on("click", (e) => on_map_ctrl_click(e, src_marker));

    // Ctrl + right-click to set destination marker
    the_map.on("contextmenu", (e) => on_map_ctrl_click(e, dst_marker));

    // update URL when map is moved :
    the_map.on("moveend", (e) => on_map_changed(the_map, update_url));

    if (compute_journey_at_startup) { handle(); }
});
