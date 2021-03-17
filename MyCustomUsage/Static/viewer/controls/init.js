import "./info.js";
import "./inputs.js";

export function initControls(the_map, departure_time, custom_port) {
    const infoControl = L.control.sectioninfo({ position: 'topright'}, custom_port).addTo(the_map);
    const inputsControl = L.control.inputs({position: 'topleft'}).addTo(the_map);
    inputsControl.setValueAsNbSeconds(departure_time);
    return [infoControl, inputsControl];
};

