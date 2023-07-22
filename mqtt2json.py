#!/usr/bin/env python

import os
import sys
import time
from shutil import copyfileobj

from c3toctrack import TracksModel, MqttTrainReporterClient


def copy(dst: str, src: str):
    with open(src, 'rb') as input, open(dst, 'wb') as output:
        os.fchmod(output.fileno(), 0o664)
        copyfileobj(input, output)


for f in ('index.html', 'lok.png', 'map.html', 'station.png', 'updatemap.js'):
    copy(f'webroot/{f}', f'data/{f}')

tracksmodel = TracksModel('data/trainlines.gpx', {
    'Airolostrecke': 563,
    'Bäderbahn': 690,
    'Gotthard-Basistunnel': 486,
    'Gotthardbahn': 462,
    'Y-Trasse': 918,
    'Berliner Außenring': 918,
    'Marschbahn': 1704
})
tracksmodel.write_json('webroot/tracks.json')
tracksmodel.write_geojson('webroot/tracks.geojson')

mqttClient = MqttTrainReporterClient(sys.argv[1], sys.argv[2], sys.argv[3], 'c3toc/train/#', tracksmodel,
                                     'webroot/trains.json', 'webroot/trains.geojson')
mqttClient.client.loop_start()

while True:
    mqttClient.cleanup()
    time.sleep(1)
