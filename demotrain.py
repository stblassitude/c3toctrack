import json
import logging
import random
import sys
import time
from datetime import datetime

import paho.mqtt.client as mqtt


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
        # self.client.subscribe(topic)

    def on_connect(self, client, userdata, flags, rc):
        print("Connected with result code " + str(rc))

    def on_message(self, client, userdata, msg):
        print(msg.topic + " " + str(msg.payload))

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


mqttClient = MqttClient(sys.argv[1], sys.argv[2], sys.argv[3], 'c3toc/train/demo')
mqttClient.client.loop_start()

with open('webroot/tracks.json') as f:
    tracks = json.load(f)

points = []
for n in ('Rollbahn', 'Gotthard-Basistunnel', 'Airolostrecke', 'Bäderbahn', 'Berliner Außenring', 'Marschbahn'):
    points += tracks['tracks'][n]['points']

pos = 0
trackmarker = 0
random.seed()

while True:
    mqttClient.client.publish('c3toc/train/demo/status', 'alive')
    point = points[pos]
    msg = {
        'lat': point['lat'] + ((0.5 - random.random()) * 0.0001),
        'lon': point['lon'] + ((0.5 - random.random()) * 0.0001),
        'sat': 99,
        'speed': 3.6,
        'ts': datetime.now().isoformat()
    }
    if ('waypoint' in point) and point['waypoint'] is not None and ('type' in point['waypoint']) and (point['waypoint']['type'] == 'Bf' or point['waypoint']['type'] == 'Hp'):
        waypoint = point['waypoint']
        print(f'stopping at {waypoint["name"]}')
        msg['speed'] = 0
        mqttClient.client.publish('c3toc/train/demo/pos', json.dumps(msg))
        time.sleep(30)
    else:
        mqttClient.client.publish('c3toc/train/demo/pos', json.dumps(msg))
    pos += 1
    if pos >= len(points):
        pos = 0
        trackmarker = 0
    else:
        t = point['trackmarker'] - trackmarker
        if t > 0:
            time.sleep(point['trackmarker'] - trackmarker) # 1 second for each meter, giving 3.6 km/h
        trackmarker = point['trackmarker']

