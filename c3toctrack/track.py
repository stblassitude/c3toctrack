from math import sqrt
from .point import Point
from .waypoint import makeWaypoint


# 13°18'30 - 13°18'20 == 186.3 m
# 53°02'02 - 53°01'57 == 154.5 m

class Track:
    """
    A track combines multiple Points.
    """
    meter_per_degree_lon = 1.4910240353067286e-05
    meter_per_degree_lat = 8.989572096375368e-06

    def __init__(self, name):
        self.name = name
        self.points = []

    def add(self, lat: float, lon: float, name: str, start: float):
        point = Point(lat, lon, name)
        if len(self.points) > 0:
            previous = self.points[len(self.points) - 1]
            start = previous.trackmarker
            lon = abs(previous.lon - point.lon) / self.meter_per_degree_lon
            lat = abs(previous.lat - point.lat) / self.meter_per_degree_lat
            point.distance = sqrt(lon * lon + lat * lat)
        point.trackmarker = start + point.distance
        if point.name is not None:
            point.waypoint = makeWaypoint(point.lat, point.lon, int(point.trackmarker), point.name)
        self.points.append(point)
