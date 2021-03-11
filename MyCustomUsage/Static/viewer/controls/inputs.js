import {seconds_to_timestring, timestring_to_seconds} from "../helpers/datetime_helper.js";

L.Control.Inputs = L.Control.extend({
    onAdd: function(map) {
        this._div = L.DomUtil.create("div", "panel");

        this._label = L.DomUtil.create("label", "", this._div);
        this._label.setAttribute("for", "departure-time");
        this._label.innerHTML = "departure time : ";

        this._textInput = L.DomUtil.create("input", "", this._div);
        this._textInput.setAttribute("type", "time");
        this._textInput.setAttribute("name", "departure-time");
        this._textInput.setAttribute("required", "true");
        this._textInput.setAttribute("value", "14:15");
        this._textInput.setAttribute("step", "1");

        this._paragraph = L.DomUtil.create("p", "", this._div);
        this._paragraph.innerHTML = "(journey is updated when focusing out of time input)";

        this._centerOnParis = L.DomUtil.create("button", "", this._div);
        this._centerOnParis.innerHTML = "PARIS";

        this._centerOnBordeaux = L.DomUtil.create("button", "", this._div);
        this._centerOnBordeaux.innerHTML = "BORDEAUX";

        return this._div;
    },

    setDepartureTimeChangedHandler(handler) {
        this._textInput.onblur = handler;
    },

    setCenterOnParisHandler(handler) {
        this._centerOnParis.onclick = handler;
    },

    setCenterOnBordeauxHandler(handler) {
        this._centerOnBordeaux.onclick = handler;
    },

    onRemove: function(map) {
    },

    valueAsNbSeconds: function() {
        return timestring_to_seconds(this._textInput.value);
    },

    valueAsString: function() {
        return this._textInput.value;
    },

    setValueAsNbSeconds: function(value) {
        this._textInput.value = seconds_to_timestring(value);
    },
});

L.control.inputs = function(opts) {
    return new L.Control.Inputs(opts);
}
