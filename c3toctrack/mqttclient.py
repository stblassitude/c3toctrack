import json
import logging
import os
import time
from datetime import datetime, timedelta, UTC
from dateutil.parser import parse

import geojson
import paho.mqtt.client as mqtt
from atomicwrites import atomic_write

from .point import Point
from .tracksmodel import TracksModel


class MqttTrainReporterClient:
    FIRST_RECONNECT_DELAY = 1
    RECONNECT_RATE = 2
    MAX_RECONNECT_COUNT = 12
    MAX_RECONNECT_DELAY = 60

    def __init__(self, hostname, username, password, topic, tracksmodel: TracksModel, trains_json: str,
                 trains_geojson: str):
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect
        self.client.on_message = self.on_message
        self.client.username_pw_set(username, password)
        self.client.connect(hostname, 1883, 60)
        self.client.subscribe(topic)
        self.trains = {
            'trains': {}
        }
        self.tracksmodel = tracksmodel
        self.trains_json = trains_json
        self.trains_geojson = trains_geojson
        self.waypoints = []
        for trackmarker, point in sorted(tracksmodel.waypoints.items()):
            if point.is_stop():
                self.waypoints.append(point)


    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))

    def on_message(self, client, userdata, msg):
        print(f'Message received for topic "{msg.topic}": "{msg.payload}"')
        (_, _, name, kind) = msg.topic.split('/')
        if kind == 'pos':
            pos = json.loads(msg.payload.decode('utf-8'))
            point = Point(pos['lat'], pos['lon'])
            distance = 1e36
            closest = None
            second = None
            trackname = None
            for track in self.tracksmodel.tracks.values():
                for i in range(0, len(track.points)):
                    d = Point.distance(point, track.points[i])
                    if d < distance:
                        distance = d
                        closest = track.points[i]
                        trackname = track.name
                        dp = 1e36
                        dn = 1e36
                        if i > 0:
                            dp = Point.distance(point, track.points[i - 1])
                        if i < len(track.points) - 1:
                            dn = Point.distance(point, track.points[i + 1])
                        if dp < dn:
                            second = track.points[i - 1]
                        else:
                            second = track.points[i + 1]
            if second is None:
                pos['trackmarker'] = closest.trackmarker
            else:
                ab = Point.distance(closest, second)
                bc = Point.distance(second, point)
                ca = Point.distance(point, closest)
                if bc == 0:
                    pos['trackmarker'] = second.trackmarker
                elif ca == 0:
                    pos['trackmarker'] = closest.trackmarker
                else:
                    d = Point.nearest_distance(ab, bc, ca)
                    if second.trackmarker > closest.trackmarker:
                        pos['trackmarker'] = closest.trackmarker + d
                    else:
                        pos['trackmarker'] = closest.trackmarker - d
            pos['trackmarker'] = int(pos['trackmarker'])
            pos['trackname'] = trackname
            if 'ts' in pos:
                pos['timestamp'] = pos['ts']
                del pos['ts']
            else:
                pos['timestamp'] = datetime.now(tz=UTC).isoformat()
            print(
                f"closest {closest.trackmarker:4.0f} - loco {pos['trackmarker']:4.0f} - second {second.trackmarker:4.0f}")

            if name in self.trains['trains']:
                lastpos = self.trains['trains'][name]
                lastpoint = Point(lastpos['lat'], lastpos['lon'])
                if Point.distance(lastpoint, point) < 2:
                    pos['dir'] = lastpos['dir']
                else:
                    pos['dir'] = int(lastpoint.angle(point))
            else:
                pos['dir'] = 0
            next_stop = self.find_next_stop(pos)
            eta = None
            if pos['speed'] > 0:
                eta = (datetime.utcnow()
                       + timedelta(seconds=int(abs(pos['trackmarker'] - next_stop.trackmarker) / (pos['speed'] / 3.6)))
                       ).isoformat() + 'Z'
            if next_stop is not None:
                pos['next_stop'] = {
                    'eta': eta,
                    'name': next_stop.name,
                    'trackmarker': next_stop.trackmarker,
                    'type': next_stop.type
                }
            print(f'JSON {pos}')
            self.trains['trains'][name] = pos
            self.update_trains()
            self.write_geojson()

    def find_next_stop(self, pos: dict) -> 'Waypoint':
        """
        Given the trains current position on the track, find the next stop.
        :return:
        """
        stop = None
        # search for stops after current position
        for waypoint in self.waypoints:
            if waypoint.trackmarker > pos['trackmarker']:
                stop = waypoint
                break
        if stop is None:
            stop = self.waypoints[0]
        return stop


    def update_trains(self):
        with atomic_write(self.trains_json, overwrite=True, encoding='utf8') as f:
            os.fchmod(f.fileno(), 0o664)
            json.dump(self.trains, f, default=vars, ensure_ascii=False, sort_keys=True, indent=2)

    def write_geojson(self):
        features = []
        for name, train in self.trains['trains'].items():
            properties = train.copy()
            properties.update({
                'marker-symbol': 'rocket', # no good loco in Maki set
                'marker-color': '#cc0',
                'name': name,
            })
            features.append(
                geojson.Feature(geometry=geojson.Point((train['lon'], train['lat'])), properties=properties))
        feature_collection = geojson.FeatureCollection(features)

        with atomic_write(self.trains_geojson, overwrite=True, encoding='utf8') as f:
            os.fchmod(f.fileno(), 0o664)
            geojson.dump(feature_collection, f, ensure_ascii=False, sort_keys=True, indent=2)

    def on_disconnect(self, client, userdata, rc):
        logging.info("Disconnected with result code: %s", rc)
        reconnect_count, reconnect_delay = 0, MqttTrainReporterClient.FIRST_RECONNECT_DELAY
        while reconnect_count < MqttTrainReporterClient.MAX_RECONNECT_COUNT:
            logging.info("Reconnecting in %d seconds...", reconnect_delay)
            time.sleep(reconnect_delay)

            try:
                self.client.reconnect()
                logging.info("Reconnected successfully!")
                return
            except Exception as err:
                logging.error("%s. Reconnect failed. Retrying...", err)

            reconnect_delay *= MqttTrainReporterClient.RECONNECT_RATE
            reconnect_delay = min(reconnect_delay, MqttTrainReporterClient.MAX_RECONNECT_DELAY)
            reconnect_count += 1
        logging.info("Reconnect failed after %s attempts. Exiting...", reconnect_count)

    def cleanup(self):
        """
        Remove trains that have not pinged in a while
        :return:
        """
        mod = False
        for name in list(self.trains['trains'].keys()):
            train = self.trains['trains'][name]
            t = train['timestamp']
            if t[-1:] != "Z":
                    t = t + "Z"
            # timestamp = datetime.fromisoformat(t)
            timestamp = parse(t)
            if datetime.now(tz=UTC) - timestamp > timedelta(minutes=10):
                print(f'Dropping train {name} due to no more GPS fixes {timestamp}')
                del self.trains['trains'][name]
                mod = True
        if mod:
            self.update_trains()
