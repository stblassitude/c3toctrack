#!/usr/bin/env python

import gpxpy
import gpxpy.gpx

from c3toctrack import Point, Track

gpx_file = open('trainlines.gpx', 'r')

gpx = gpxpy.parse(gpx_file)

tracks = {}

for track in gpx.tracks:
    # print(f'Track {track.name}')
    if track.name is None:
        continue
    if track.name in tracks:
        t = tracks[track.name]
    else:
        t = Track(track.name)
        tracks[track.name] = t
    for segment in track.segments:
        if track.name == 'Airolostrecke':
            t.trackmiles.append(563)
        if track.name == 'Bäderbahn':
            t.trackmiles.append(690)
        if track.name == 'Gotthard-Basistunnel':
            t.trackmiles.append(486)
        if track.name == 'Gotthardbahn':
            t.trackmiles.append(462)
        if track.name == 'Y-Trasse':
            t.trackmiles.append(918)
        if track.name == 'Berliner Außenring':
            t.trackmiles.append(918)
        if track.name == 'Marschbahn':
            t.trackmiles.append(1704)
        for point in segment.points:
            t.add(Point(point.latitude, point.longitude, point.name))
            # if point.name is not None:
            #     print(f'  {point.latitude:2.5f}, {point.longitude:2.5f}: {point.name}')
            # else:
            #     print(f'  {point.latitude:2.5f}, {point.longitude:2.5f}')

hp = {}
for n, t in tracks.items():
    print(f'{t.name}: {len(t.points)}')
    for p, tm in zip(t.points, t.trackmiles):
        if p.name is not None:
            hp[tm] = p.name
            print(f'  {p.lat:0.5f}/{p.lon:0.5f}: {tm/1000: 4.3f} {p.name}')
        else:
            print(f'  {p.lat:0.5f}/{p.lon:0.5f}: {tm/1000: 4.3f} ')

print("Haltepunkte, Bahnhöfe und Bahnübergänge")
for tm in sorted(hp):
    print(f'{tm/1000: 4.3f} {hp[tm]}')

# for waypoint in gpx.waypoints:
#     print('waypoint {0} -> ({1},{2})'.format(waypoint.name, waypoint.latitude, waypoint.longitude))
#
# for route in gpx.routes:
#     print('Route:')
#     for point.py in route.points:
#         print('Point at ({0},{1}) -> {2}'.format(point.py.latitude, point.py.longitude, point.py.elevation))
