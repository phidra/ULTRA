import {to_m_per_s} from "../helpers/location_helper.js";

L.Control.SectionInfo = L.Control.extend({
    onAdd: function(map) {
        this._div = L.DomUtil.create('div', 'panel');
        this.reset();
        return this._div;
    },

    update: function(response_data) {
        const d = response_data;
        const to_navitia_url = d.navitia_url_computer;

        let local_8888_url_normal = d.routemm_url.toString().replace("routemm.mappysnap.net", "localhost:8888");
        let local_8888_url_fast = local_8888_url_normal.replace("walk_speed=normal", "walk_speed=fast");

        function to_other_port(port) {
            let url = new URL(window.location.href);
            url.port = port;
            return url.toString();
        }

        function to_patator(url) {
            let parsed_url = new URL(url);
            // this doesn't work, because exotic ports on patator are blocked by firewall :
            // let parsed_host = parsed_url.host;
            // return parsed_url.toString().replace(parsed_host, "build-route-datacook-001.mappy.priv").replace("https", "http");
            //
            // instead, we suppose that patator ports are locally redirected on "port+1000" (e.g. 9042 to access 8042) :
            const old_port = parsed_url.port;
            const new_port = (parseInt(old_port) + 1000).toString();
            return url.replace(`:${old_port}`, `:${new_port}`);
        }

        const navitia_token ="389fbc1a-1a37-4410-9f73-c6f9c04f423d";


        this._div.innerHTML = (

            // WALKSPEED en gros :
            '<p style="font-weight: bold; text-align: center;">' +
            `walkspeed = <span style="font-size: 130%; color: blue;">${Math.round(10*d.walkspeed_km_per_hour) / 10} km/h<br/></span>` +
            "</p>" +

            // request :
            '<p>' +
            `<b>src</b>: ${d.src_name}  <small style="color: grey;">(id=${d.src_id} at ${Math.round(d.src_snap_distance)} m)</small><br/>` +
            `<b>dst</b>: ${d.dst_name}  <small style="color: grey;">(id=${d.dst_id} at ${Math.round(d.dst_snap_distance)} m)</small><br/>` +
            `<b>departure_time</b>: ${d.departure_time_str}    <small style="color: grey;">${d.departure_time}</small> <br/>` +
            '</p>' +

            // response :
            '<p> </p>' +
            `<b>initial wait</b>: ${d.initial_wait_str}    <small style="color: grey;">${d.initial_wait}</small> <br/>` +
            `<b>travel begins at</b>: ${d.travel_begin_str}    <small style="color: grey;">${d.travel_begin}</small>` +
            `<div style="border: 2px solid black; padding: 3px;"><b>EAT</b>: ${d.eat_str}    <small style="color: grey;">${d.eat}</small> </div>` +
            `<b>total duration</b>: ${d.journey_duration_str}    <small style="color: grey;">${d.journey_duration}</small> <br/>` +

            // walks :
            '<p>' +
            `<b>first walk</b>: ${d.first_walk_duration_str}   <small style="color: grey;">${d.first_walk_duration}</small> <br/>` +
            `<b>middle walks</b>: ${d.middle_walks_duration_str}   <small style="color: grey;">${d.middle_walks_duration}</small> <br/>` +
            `<b>final walk</b>: ${d.final_walk_duration_str}   <small style="color: grey;">${d.final_walk_duration}</small> <br/>` +
            `<b>total walk</b>: ${d.total_walk_duration_str}   <small style="color: grey;">${d.total_walk_duration}</small> <br/>` +
            '</p>' +

            // backend :
            '<p>' +
            (d.status_code == 200 ?
                `<b>backend status</b>: <b style="color:green;">${d.status_code}</b><br/>`
                :
                `<b>backend status</b>: <b style="color:red;">${d.status_code}</b><br/>`
            ) +
            (d.error_msg === "" ?
                `<b>error</b>: <i style="color:grey;">none</i><br/>`
                :
                `<b>error</b>: <b style="color:red;">${d.error_msg}</b><br/>`
            ) +
            `<b>backend url</b>: <a href="${d.requested_url}">url</a><br/>` +
            `<b>time took</b>: ${d.computing_time_microseconds} µs<br/>` +
            '</p>' +

            // links :
            '<p>' +
            `<b>routemm</b>: <a href="${d.routemm_url}">snap</a>|<a href="${local_8888_url_normal}">8888+4.7</a>|<a href="${local_8888_url_fast}">8888+9.3</a><br/>` +
            '</p>' +

            '<p style="font-size: 110%">' +
            `<b>local UW</b>:` +
                `<a href="${to_other_port(8041)}">4.0</a>|` +
                `<a href="${to_other_port(8042)}">4.7</a>|` +
                `<a href="${to_other_port(8043)}">5.4</a>|` +
                `<a href="${to_other_port(8044)}">6</a>|` +
                `<a href="${to_other_port(8045)}">8</a>|` +
                `<a href="${to_other_port(8046)}">9.3</a>|` +
                `<a href="${to_other_port(8047)}">10</a>|` +
                `<a href="${to_other_port(8048)}">12</a>|` +
                `<a href="${to_other_port(8049)}">15</a>|` +
                `<a href="${to_other_port(8050)}">18</a>` +
                "<br/>" +
            `<b>patator UW</b>:` +
                `<a href="${to_patator(to_other_port(8041))}">4.0</a>|` +
                `<a href="${to_patator(to_other_port(8042))}">4.7</a>|` +
                `<a href="${to_patator(to_other_port(8043))}">5.4</a>|` +
                `<a href="${to_patator(to_other_port(8044))}">6</a>|` +
                `<a href="${to_patator(to_other_port(8045))}">8</a>|` +
                `<a href="${to_patator(to_other_port(8046))}">9.3</a>|` +
                `<a href="${to_patator(to_other_port(8047))}">10</a>|` +
                `<a href="${to_patator(to_other_port(8048))}">12</a>|` +
                `<a href="${to_patator(to_other_port(8049))}">15</a>|` +
                `<a href="${to_patator(to_other_port(8050))}">18</a>` +
                "<br/>" +
            '</p>' +

            '<p style="font-size: 110%">' +
            `<b>navitia</b>:` +
                `<a href="${to_navitia_url(4.0)}">4.0</a>|` +
                `<a href="${to_navitia_url(4.7)}">4.7</a>|` +
                `<a href="${to_navitia_url(5.4)}">5.4</a>|` +
                `<a href="${to_navitia_url(6)}">6</a>|` +
                `<a href="${to_navitia_url(8)}">8</a>|` +
                `<a href="${to_navitia_url(8)}">8</a>|` +
                `<a href="${to_navitia_url(9.3)}">9.3</a>|` +
                `<a href="${to_navitia_url(10)}">10</a>|` +
                `<a href="${to_navitia_url(12)}">12</a>|` +
                `<a href="${to_navitia_url(15)}">15</a>|` +
                `<a href="${to_navitia_url(18)}">18</a>` +
                "<br/>" +
                // button to copy navitia token :
                `<button onclick="navigator.clipboard.writeText('${navitia_token}')">copy navitia token to clipboard</button>` +
            '</p>' +

            '<p style="font-size: 110%">' +
            `<b>comparison port </b>:&nbsp;` +
            `<input id="comparison_port_input" style="width: 80px;" type="number" min="1025" max="65535" value="${this.comparison_port}" />` +
            "<br/>" +
            `<b>compare with </b>:&nbsp; <a id="link_to_comparison_port"></a>` +
            '</p>' +

            // help :
            '<p>' +
            'CTRL + left-click  to set <span style="color: green;">SOURCE</span> <br/>' +
            'CTRL + right-click to set <span style="color: red;">DESTINATION</span> <br/>' +
            'Total duration INCLUDES initial wait<br/>' +
            'Local URLs: 8041 -> 8049<br/>' +
            'Patator URLs: 9041 -> 9049 (redirect if needed)<br/>' +
            '</p>'
        );

        // this is not very optimal (bc done again and again for each route), but for now, this will do :
        function update_link_to_comparison_port(new_port) {
            const link_to_comparison_port = document.getElementById("link_to_comparison_port");
            if (link_to_comparison_port !== null) {
                const new_url =  `${to_other_port(new_port)}`;
                link_to_comparison_port.setAttribute("href", new_url);
                link_to_comparison_port.innerHTML = `port ${new_port}`;
            }
        };
        update_link_to_comparison_port(this.comparison_port, to_other_port(this.comparison_port));
        const comparison_port_input = document.getElementById("comparison_port_input");
        if (comparison_port_input !== null) {
            comparison_port_input.onchange = (e) => {
                this.comparison_port = comparison_port_input.value;
                this.window_url_updater({comparisonport: this.comparison_port});
                update_link_to_comparison_port(this.comparison_port);
            };
        }
    },

    reset: function() {
        const dummy_data = {
            // request :
            src_id:"void",
            src_name:"void",
            src_snap_distance: -1,
            dst_id: "void",
            dst_name: "void",
            dst_snap_distance: -1,
            departure_time_str: "void",
            departure_time: -1,

            // response :
            initial_wait_str: "void",
            initial_wait: -1,
            travel_begin_str: "void",
            travel_begin: -1,
            eat_str: "void",
            eat: -1,
            journey_duration_str: "void",
            journey_duration: -1,

            // walks :
            first_walk_duration_str: "void",
            first_walk_duration: -1,
            middle_walks_duration_str: "void",
            middle_walks_duration: -1,
            final_walk_duration_str: "void",
            final_walk_duration: -1,
            walkspeed_km_per_hour: -1,

            // backend :
            computing_time_microseconds: -1,
            requested_url: "",
            routemm_url: "",
            navitia_url_computer: function() {},
            status_code: -1,
            error_msg: "void",
        }
        this.update(dummy_data);
    },
});

L.control.sectioninfo = function(opts, comparison_port) {
    const toReturn = new L.Control.SectionInfo(opts);
    toReturn.comparison_port = comparison_port;
    return toReturn;
}
