export function initMap(center, zoom) {
    const the_map = L.map('the_map');
    the_map.zoomControl.remove();
    the_map.doubleClickZoom.disable();
    the_map.setView(center, zoom);
    const mappy_url_template = "https://map.mappysnap.net/map/1.0/slab/standard/256/{z}/{x}/{y}";
    const osm_url_template = "https://a.tile.openstreetmap.org/{z}/{x}/{y}.png";

    // switch tile server if need be :
    const url_template = mappy_url_template;
    // const url_template = osm_url_template;

    L.tileLayer(url_template, {
        maxZoom: 19,
    }).addTo(the_map);

    return the_map;
};

export function on_map_changed(the_map, update_window_url) {
    let lat = the_map.getCenter().lat.toFixed(6);
    let lng = the_map.getCenter().lng.toFixed(6);
    update_window_url({
        mapcenter: [lat, lng],
        mapzoom: the_map.getZoom(),
    });
};
