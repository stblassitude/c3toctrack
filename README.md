# c3toctrack

Tracker Infrastructure for the c3toc trains at #cccamp23

# JSON API

The python script `mqtt2json.py` generates two JSON files: `tracks.json` and `trains.json`.

## `tracks.json`

The file is updated on every start of the script, and contains information about the track segments that make up the
entry system. The contents are converted from `data/trainlines.gpx`, which in turn are exported using JOSM
from `docs/trainlines.gpx`.

There are two main objects in that JSON, `tracks` and `waypoints`.

### Tracks

Tracks is a dict of track segments, with the track
segments' name as the key. Eack segment has a number of geo coordinates under the key `points`, which include three key
entries:

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
  * W: Weiche, or switch
* trackmarker: the linear position on the track, in meters
* name: the name of the waypoint

## `trains.json`

tbd

# Setting Up

```shell
POETRY_VIRTUALENVS_IN_PROJECT=true poetry install
```

# Running the API gateway

```shell
poetry run python mqtt2json.py mqtthostname username password
```