function truncate(number, precision) {
    return parseFloat(number).toFixed(precision);
}

export function get_coords(obj, reverse=false) {
    if(typeof obj.getLatLng === 'function') {
        return format_coords(obj.getLatLng());
    }

    if ("lat" in obj && "lng" in obj) {
        return format_coords(obj);
    }

    if (Array.isArray(obj) && obj.length === 2) {
        if (!reverse) {
            // proper order = expecting [lat, lng] :
            return format_coords({lat: obj[0], lng: obj[1]});
        }
        else {
            return format_coords({lat: obj[1], lng: obj[0]});
        }
    }

    throw `unable to get_coords from obj ${obj.toString()} (${JSON.stringify(obj)})`;
}


function format_coords(coords) {
    // precondition : coords is an object with "lng" and "lat" properties
    return `${truncate(coords.lng, 6)},${truncate(coords.lat, 6)}`;
}


// helper function to get 1/3 and 2/3 of the displayed map :
export function get_map_thirds(the_map) {
    var north_east = the_map.getBounds()._northEast;
    var south_west = the_map.getBounds()._southWest;

    var min_lat = Math.min(north_east.lat, south_west.lat);
    var max_lat = Math.max(north_east.lat, south_west.lat);
    var delta_lat = max_lat - min_lat;

    var min_lng = Math.min(north_east.lng, south_west.lng);
    var max_lng = Math.max(north_east.lng, south_west.lng);
    var delta_lng = max_lng - min_lng;

    var third_one_lat = min_lat + (delta_lat / 3);
    var third_one_lng = min_lng + (delta_lng / 3);

    var third_two_lat = min_lat + 2 * (delta_lat / 3);
    var third_two_lng = min_lng + 2 * (delta_lng / 3);

    return [
        [third_one_lat, third_one_lng],
        [third_two_lat, third_two_lng]
    ];
}


export function to_m_per_s(km_per_h) {
    return km_per_h / 3.6;  // 1 km/h = 1000 m / 3600 s
}
