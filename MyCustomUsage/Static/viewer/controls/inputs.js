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

        this._timeSetterContainer = L.DomUtil.create("div", "", this._div);
        this._timePlusContainer = L.DomUtil.create("div", "", this._timeSetterContainer);
        this._timeMinusContainer = L.DomUtil.create("div", "", this._timeSetterContainer);

        this._plus1hour = L.DomUtil.create("button", "", this._timePlusContainer);
        this._plus1hour.innerHTML = "+ 1h";
        this._plus15min = L.DomUtil.create("button", "", this._timePlusContainer);
        this._plus15min.innerHTML = "+ 15m";
        this._plus5min = L.DomUtil.create("button", "", this._timePlusContainer);
        this._plus5min.innerHTML = "+ 5m";
        this._plus1min = L.DomUtil.create("button", "", this._timePlusContainer);
        this._plus1min.innerHTML = "+ 1m";
        this._minus1hour = L.DomUtil.create("button", "", this._timeMinusContainer);
        this._minus1hour.innerHTML = "- 1h";
        this._minus15min = L.DomUtil.create("button", "", this._timeMinusContainer);
        this._minus15min.innerHTML = "- 15m";
        this._minus5min = L.DomUtil.create("button", "", this._timeMinusContainer);
        this._minus5min.innerHTML = "- 5m";
        this._minus1min = L.DomUtil.create("button", "", this._timeMinusContainer);
        this._minus1min.innerHTML = "- 1m";

        return this._div;
    },

    setDepartureTimeChangedHandler(handler) {
        this._textInput.onblur = handler;

        this._plus1hour.onclick = (e) => { this.addSeconds(3600); handler(e); };
        this._plus15min.onclick = (e) => { this.addSeconds(900); handler(e); };
        this._plus5min.onclick = (e) => { this.addSeconds(300); handler(e); };
        this._plus1min.onclick = (e) => { this.addSeconds(60); handler(e); };

        this._minus1hour.onclick = (e) => { this.addSeconds(-3600); handler(e); };
        this._minus15min.onclick = (e) => { this.addSeconds(-900); handler(e); };
        this._minus5min.onclick = (e) => { this.addSeconds(-300); handler(e); };
        this._minus1min.onclick = (e) => { this.addSeconds(-60); handler(e); };
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

    addSeconds: function(nb_added_seconds) {
        this.setValueAsNbSeconds(this.valueAsNbSeconds() + nb_added_seconds);
    },
});

L.control.inputs = function(opts) {
    return new L.Control.Inputs(opts);
}
