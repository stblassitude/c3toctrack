# c3toctrack

Tracker Infrastructure for the c3toc trains at #cccamp23. See [c3toc.de](https://c3toc.de)
and [api.c3toc.de](https://api.c3toc.de).

# Hardware

The trackers have been built using [Lilygo T-Beam](https://www.lilygo.cc/products/t-beam-v1-1-esp32-lora-module) modules, which combine an ESP32 with a Neo 6M GPS receiver, a Lora radio, and a 18650 LiIon battery holder, with power controller to allow running off the battery and charge it. The module is about 50€ and comes with a small GPS antenna that is not very good; you can easily replace it with a better one. Note that there are many clones of this board that use slightly different hardware or pinouts, so some experimentation might be needed. The Lora radio is not currently used, but it would be easy to (also) transmit the data via LoraWan, for example to The Things Network. With a brand-name 18650, the tracker ran for up to 36 hours at Camp.

# JSON API

The python script `mqtt2json.py` generates four JSON files: `tracks.json`, `tracks.geojson`, `trains.json`,
and `trains.geojson`.

## `tracks.json`

The file is updated on every start of the script, and contains information about the track segments that make up the
entire system. The contents are converted from `data/trainlines.gpx`, which in turn are exported using JOSM
from `docs/trainlines.osm`.

There are two main objects in that JSON, `tracks` and `waypoints`.

### Tracks

Tracks is a dict of track segments, with the track segments' name as the key. Each segment has a number of geo
coordinates under the key `points`, which include three key entries:

* lat & lon, the geo position of this point
* trackmarker, the milestone location of that point, in meters.
* waypoint, an optional waypoint (see below)

### Waypoints

In addition to being attached to a point, the separate `waypoints` dict also contains all waypoints. The key is the
trackmarker (again in meters). Each waypoint has these properties

* type: one of:
    * Bf: Bahnhof, or station,
    * Bü: Bahnübergang, or level crossing,
    * Hp: Haltepunkt, or stop
    * W: Weiche, or switch/turnout
* trackmarker: the linear position on the track, in meters
* name: the name of the waypoint
* ds100: the [DS100](https://de.wikipedia.org/wiki/Betriebsstellenverzeichnis#Deutsche_Bundesbahn) ([official list](http://www.bahnseite.de/DS100/DS100_main.html)) designation for the stop or station

## `trains.json`

There is one property `trains', which contains one property per train, named after the train. Each train has these
properties:

* `lat` and `lon` GPS position
* `dir`: direction of travel in degrees; 0 is north, 90 is east.
* `sat`: number of satellites reported by the receiver, can be used to gauge accuracy of the fix
* `speed`: in km/h
* `trackmarker`: in meters, relative to the track.
* `trackname`: name of the track segment the train is on.
* `next_stop`: details of the next stop the train will reach, with these properties:
  * `eta`: time in UTC when the train will arrive at this stop
  * `name`
  * `trackmarker`
  * `type` 

## GeoJson

Both tracks and trains are also available in [GeoJson](https://geojson.org) format.

The features have additional properties according
to [simplestyle](https://github.com/mapbox/simplestyle-spec/tree/master/1.1.0).

You can look at the tracks GeoJSON on MapBox' [geojson.io](https://geojson.io/#data=data:text/x-url,https%3A%2F%2Fapi.c3toc.de%2Ftracks.geojson)

# Setting Up

```shell
POETRY_VIRTUALENVS_IN_PROJECT=true poetry install
```

# Running the API gateway

```shell
poetry run python mqtt2json.py mqtthostname username password
```

# Running unit tests

```shell
poetry run py.test
```