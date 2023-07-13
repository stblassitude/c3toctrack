#!/usr/bin/env python

import json
import logging
import os
import sys
import time
from shutil import copyfileobj

import gpxpy
import gpxpy.gpx
import paho.mqtt.client as mqtt
from atomicwrites import atomic_write

from c3toctrack import Track


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
        match track.name:
            case 'Airolostrecke':
                start = 563
            case 'Bäderbahn':
                start = 690
            case 'Gotthard-Basistunnel':
                start = 486
            case 'Gotthardbahn':
                start = 462
            case 'Y-Trasse':
                start = 918
            case 'Berliner Außenring':
                start = 918
            case 'Marschbahn':
                start = 1704
            case _:
                start = 0
        for segment in track.segments:
            for point in segment.points:
                t.add(point.latitude, point.longitude, point.name, start)

    waypoints = {}
    for n, t in tracks.items():
        for point in t.points:
            if point.waypoint is not None:
                waypoints[point.waypoint.trackmarker] = point.waypoint

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
        self.trains = {}

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))

    def on_message(self, client, userdata, msg):
        print(msg.topic + " " + str(msg.payload))
        (_, _, name, kind) = msg.topic.split('/')
        if kind == 'pos':
            pos = json.loads(msg.payload.decode('utf-8'))
            print(pos)
            self.trains[name] = pos
        with atomic_write('webroot/trains.json', overwrite=True, encoding='utf8') as f:
            os.fchmod(f.fileno(), 0o664)
            print(json.dump(self.trains, f, default=vars, ensure_ascii=False, sort_keys=True, indent=2))


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


with open('data/index.html', 'r', encoding='utf8') as input, atomic_write('webroot/index.html', overwrite=True, encoding='utf8') as output:
    os.fchmod(output.fileno(), 0o664)
    copyfileobj(input, output)

tracks = gpx2tracks()
with atomic_write('webroot/tracks.json', overwrite=True, encoding='utf8') as f:
    os.fchmod(f.fileno(), 0o664)
    print(json.dump(tracks, f, default=vars, ensure_ascii=False, sort_keys=True, indent=2))

mqttClient = MqttClient(sys.argv[1], sys.argv[2], sys.argv[3], 'c3toc/train/#')
mqttClient.client.loop_forever()
