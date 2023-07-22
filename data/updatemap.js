var map = L.map('map').setView([53.03171, 13.30597], 16);

L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 19,
    attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a>'
}).addTo(map);

var lok = L.icon({
    iconUrl: 'lok.png',
    iconSize: [25, 25],
    iconAnchor: [12, 25],
    popupAnchor: [-3, -76]
});
var station = L.icon({
    iconUrl: 'station.png',
    iconSize: [25, 25],
    iconAnchor: [12, 25],
    popupAnchor: [-3, -76]
});
var markers = {};
var markersToRemove = {};

async function getStations() {
    let json;
    try {
        const res = await fetch("tracks.json");
        json = await res.json();
    } catch (e) {
        setTimeout(getStations, 30_000);
    }
    for (var name in json.waypoints) {
        waypoint = json.waypoints[name];
        if (waypoint.type === "Hp" || waypoint.type === "Bf") {
            marker = L.marker([waypoint.lat, waypoint.lon], {icon: station})
                .bindTooltip(waypoint.name, {
                    direction: 'auto',
                    offset: [12, -12],
                    permanent: false,
                })
                .addTo(map);
        }
    }
}

async function getTrains() {
    let json;
    try {
        const res = await fetch("trains.json");
        json = await res.json();
    } catch (e) {
        setTimeout(getTrains, 30_000);
    }
    for (var name in json.trains) {
        train = json.trains[name];
        delete markersToRemove[name];
        var marker = markers[name];
        if (marker === undefined) {
            marker = L.marker([train.lat, train.lon], {
                icon: lok,
                zIndexOffset: 1000
            })
                .bindTooltip(name, {
                    direction: 'bottom',
                    permanent: true,
                })
                .addTo(map);
            markers[name] = marker;
        }
        marker.setLatLng(new L.latLng(train.lat, train.lon));
    }
    for (name in markersToRemove) {
        map.removeLayer(markers[name])
        delete markers[name]
    }
    setTimeout(getTrains, 5_000);
}

getStations();
getTrains();
