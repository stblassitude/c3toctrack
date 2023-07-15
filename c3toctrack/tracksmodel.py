import json
import os

import geojson
import gpxpy
from atomicwrites import atomic_write

from .track import Track


class TracksModel:
    def __init__(self, filename: str, starts: dict):
        self.tracks = {}
        self.waypoints = {}

        gpx_file = open(filename, 'r')

        gpx = gpxpy.parse(gpx_file)

        for track in gpx.tracks:
            # print(f'Track {track.name}')
            if track.name is None:
                continue
            if track.name in self.tracks:
                t = self.tracks[track.name]
            else:
                t = Track(track.name)
                self.tracks[track.name] = t

            start = 0
            if track.name in starts:
                start = starts[track.name]

            for segment in track.segments:
                for point in segment.points:
                    t.add(point.latitude, point.longitude, point.name, start)

        for n, t in self.tracks.items():
            for point in t.points:
                if point.waypoint is not None:
                    self.waypoints[point.waypoint.trackmarker] = point.waypoint

    def write_json(self, filename: str):
        with atomic_write(filename, overwrite=True, encoding='utf8') as f:
            os.fchmod(f.fileno(), 0o664)
            json.dump({
                'tracks': self.tracks,
                'waypoints': self.waypoints
            }, f, default=vars, ensure_ascii=False, sort_keys=True, indent=2)

    def write_geojson(self, filename: str):
        features = []
        for name, track in self.tracks.items():
            points = []
            for point in track.points:
                points.append((point.lon, point.lat))
            features.append(geojson.Feature(geometry=geojson.LineString(points), properties={
                'name': track.name,
                'stroke': '#ff0000',
                'stroke-width': 4,
                'title': track.name,
            }))
        for name, waypoint in self.waypoints.items():
            properties = {
                'description': f'{waypoint.type} {waypoint.name}',
                'name': waypoint.name,
                'title': waypoint.name,
                'type': waypoint.type
            }
            match waypoint.type:
                case 'Bf':
                    properties['marker'] = 'rail'
                case 'Bü':
                    properties['marker'] = 'fence'
                case 'Hp':
                    properties['marker'] = 'rail'
                case 'W':
                    properties['marker'] = 'cross'
            features.append(
                geojson.Feature(geometry=geojson.Point((waypoint.lon, waypoint.lat)), properties=properties))
        feature_collection = geojson.FeatureCollection(features)

        with atomic_write(filename, overwrite=True, encoding='utf8') as f:
            os.fchmod(f.fileno(), 0o664)
            geojson.dump(feature_collection, f, ensure_ascii=False, sort_keys=True, indent=2)
