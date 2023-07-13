#!/usr/bin/env python

import json
import logging
import sys
import time

import gpxpy
import gpxpy.gpx
import paho.mqtt.client as mqtt

from c3toctrack import Point, Track, makeWaypoint

def gpx2tracks():
    gpx_file = open('data/trainlines.gpx', 'r')

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
                t.trackmarker.append(563)
            if track.name == 'Bäderbahn':
                t.trackmarker.append(690)
            if track.name == 'Gotthard-Basistunnel':
                t.trackmarker.append(486)
            if track.name == 'Gotthardbahn':
                t.trackmarker.append(462)
            if track.name == 'Y-Trasse':
                t.trackmarker.append(918)
            if track.name == 'Berliner Außenring':
                t.trackmarker.append(918)
            if track.name == 'Marschbahn':
                t.trackmarker.append(1704)
            for point in segment.points:
                t.add(Point(point.latitude, point.longitude, point.name))
                # if point.name is not None:
                #     print(f'  {point.latitude:2.5f}, {point.longitude:2.5f}: {point.name}')
                # else:
                #     print(f'  {point.latitude:2.5f}, {point.longitude:2.5f}')

    waypoints = {}
    for n, t in tracks.items():
        # print(f'{t.name}: {len(t.points)}')
        for p, tm in zip(t.points, t.trackmarker):
            if p.name is not None:
                wp = makeWaypoint(p.lat, p.lon, tm, p.name)
                waypoints[wp.trackmarker] = wp
            #     print(f'  {p.lat:0.5f}/{p.lon:0.5f}: {tm/1000: 4.3f} {p.name}')
            # else:
            #     print(f'  {p.lat:0.5f}/{p.lon:0.5f}: {tm/1000: 4.3f} ')

    # print("   km  type  name")
    # for trackmarker in sorted(waypoints):
    #     print(f'{trackmarker/1000:1.3f}  {waypoints[trackmarker].typecode()}    {waypoints[trackmarker].name}')

    return {
        'tracks': tracks,
        'waypoints': waypoints
    }

class MqttClient():
    FIRST_RECONNECT_DELAY = 1
    RECONNECT_RATE = 2
    MAX_RECONNECT_COUNT = 12
    MAX_RECONNECT_DELAY = 60

    def __init__(self, hostname, username, password, topic):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.username_pw_set(username, password)
        self.client.connect(hostname, 1883, 60)
        self.client.subscribe(topic)

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code "+str(rc))


    def on_message(self, client, userdata, msg):
        print(msg.topic+" "+str(msg.payload))

    def on_disconnect(self, client, userdata, rc):
        logging.info("Disconnected with result code: %s", rc)
        reconnect_count, reconnect_delay = 0, MqttClient.FIRST_RECONNECT_DELAY
        while reconnect_count < MqttClient.MAX_RECONNECT_COUNT:
            logging.info("Reconnecting in %d seconds...", reconnect_delay)
            time.sleep(reconnect_delay)

            try:
                self.client.reconnect()
                logging.info("Reconnected successfully!")
                return
            except Exception as err:
                logging.error("%s. Reconnect failed. Retrying...", err)

            reconnect_delay *= MqttClient.RECONNECT_RATE
            reconnect_delay = min(reconnect_delay, MqttClient.MAX_RECONNECT_DELAY)
            reconnect_count += 1
        logging.info("Reconnect failed after %s attempts. Exiting...", reconnect_count)


tracks = gpx2tracks()
with open('tracks.json', 'w', encoding="utf-8") as f:
    print(json.dump(tracks, f, default=vars, sort_keys=True, indent=2))

mqttClient = MqttClient(sys.argv[1], sys.argv[2], sys.argv[3], 'c3toc/#')
mqttClient.client.loop_forever()